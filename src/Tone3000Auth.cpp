/*
  ==============================================================================

    Tone3000Auth.cpp
    OAuth authentication handler for TONE3000 API

  ==============================================================================
*/

#include "Tone3000Auth.h"

#include "ColourScheme.h"
#include "Tone3000Client.h"

#include <random>
#include <spdlog/spdlog.h>


#ifdef _WIN32
    #include <WS2tcpip.h>
    #include <WinSock2.h>
    #pragma comment(lib, "Ws2_32.lib")
#endif

//==============================================================================
// Tone3000Auth
//==============================================================================

Tone3000Auth::Tone3000Auth() : Thread("Tone3000Auth") {}

Tone3000Auth::~Tone3000Auth()
{
    cancelAuthentication();
}

bool Tone3000Auth::isAuthenticating() const
{
    return isThreadRunning();
}

void Tone3000Auth::startAuthentication(std::function<void(bool, juce::String)> callback)
{
    if (isThreadRunning())
    {
        spdlog::warn("[Tone3000Auth] Authentication already in progress");
        juce::MessageManager::callAsync([callback = std::move(callback)]() mutable
                                        { callback(false, "Authentication already in progress"); });
        return;
    }

    {
        std::lock_guard<std::mutex> lock(completionCallbackMutex);
        completionCallback = std::move(callback);
    }
    completionDispatched.store(false, std::memory_order_release);
    shouldStop = false;
    expectedState = generateState();

    // Build OAuth URL
    juce::String redirectUri = "http://localhost:" + juce::String(callbackPort) + "/callback";
    juce::URL authUrl(AUTH_URL);
    authUrl = authUrl.withParameter("redirect_url", redirectUri);
    authUrl = authUrl.withParameter("state", expectedState);

    spdlog::info("[Tone3000Auth] Starting OAuth flow, redirect: {}, state: {}", redirectUri.toStdString(),
                 expectedState.toStdString());

    // Reset ready flag
    serverReady = false;

    // Start the callback server thread
    startThread();

    // Wait for server to be ready (up to 3 seconds)
    for (int i = 0; i < 30; ++i)
    {
        Thread::sleep(100);
        if (serverReady)
            break;
    }

    if (!serverReady)
    {
        spdlog::error("[Tone3000Auth] Server failed to start in time");
        shouldStop = true;
        stopThread(5000);
        dispatchCompletion(false, "Failed to start authentication server");
        return;
    }

    spdlog::info("[Tone3000Auth] Server ready on port {}, opening browser...", callbackPort);

    // Open browser to auth URL
    authUrl.launchInDefaultBrowser();
}

void Tone3000Auth::cancelAuthentication()
{
    shouldStop = true;

    if (serverSocket)
        serverSocket->close();

    stopThread(5000);
    spdlog::info("[Tone3000Auth] Authentication cancelled");
}

void Tone3000Auth::dispatchCompletion(bool success, const juce::String& errorMessage)
{
    if (completionDispatched.exchange(true, std::memory_order_acq_rel))
        return;

    std::function<void(bool, juce::String)> callbackCopy;
    {
        std::lock_guard<std::mutex> lock(completionCallbackMutex);
        callbackCopy = completionCallback;
    }

    if (!callbackCopy)
        return;

    juce::MessageManager::callAsync([callback = std::move(callbackCopy), success, errorMessage]() mutable
                                    { callback(success, errorMessage); });
}

void Tone3000Auth::run()
{
    spdlog::info("[Tone3000Auth] Starting callback server on port {}", callbackPort);

#ifdef _WIN32
    // Use native WinSock2 for better reliability on Windows
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        spdlog::error("[Tone3000Auth] WSAStartup failed");
        serverReady = false;
        dispatchCompletion(false, "Failed to initialize network.");
        return;
    }

    SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket == INVALID_SOCKET)
    {
        spdlog::error("[Tone3000Auth] Failed to create socket: {}", WSAGetLastError());
        WSACleanup();
        serverReady = false;
        dispatchCompletion(false, "Failed to create socket.");
        return;
    }

    // Allow address reuse
    int optval = 1;
    setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&optval, sizeof(optval));

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY); // Bind to all interfaces for better compatibility
    serverAddr.sin_port = htons(static_cast<u_short>(callbackPort));

    if (bind(listenSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
    {
        spdlog::error("[Tone3000Auth] Failed to bind socket: {}", WSAGetLastError());
        closesocket(listenSocket);
        WSACleanup();
        serverReady = false;
        dispatchCompletion(false, "Failed to bind to port. It may be in use.");
        return;
    }

    if (listen(listenSocket, 1) == SOCKET_ERROR)
    {
        spdlog::error("[Tone3000Auth] Failed to listen on socket: {}", WSAGetLastError());
        closesocket(listenSocket);
        WSACleanup();
        serverReady = false;
        dispatchCompletion(false, "Failed to start listening.");
        return;
    }

    spdlog::info("[Tone3000Auth] Server listening on port {} (native WinSock)", callbackPort);

    // Small delay to ensure socket is fully established before signaling ready
    Sleep(100);

    serverReady = true;

    // Keep socket in blocking mode but use select() for timeout

    constexpr int timeoutMs = 300000; // 5 minutes
    auto startTime = juce::Time::getMillisecondCounter();

    while (!shouldStop)
    {
        if (juce::Time::getMillisecondCounter() - startTime > timeoutMs)
        {
            spdlog::warn("[Tone3000Auth] Timed out waiting for callback");
            closesocket(listenSocket);
            WSACleanup();
            dispatchCompletion(false, "Authentication timed out. Please try again.");
            return;
        }

        // Use select for timeout-based waiting
        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(listenSocket, &readSet);

        timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 500000; // 500ms

        int selectResult = select(0, &readSet, nullptr, nullptr, &timeout);
        if (selectResult > 0 && FD_ISSET(listenSocket, &readSet))
        {
            sockaddr_in clientAddr{};
            int clientAddrLen = sizeof(clientAddr);
            SOCKET clientSocket = accept(listenSocket, (sockaddr*)&clientAddr, &clientAddrLen);

            if (clientSocket != INVALID_SOCKET)
            {
                spdlog::debug("[Tone3000Auth] Accepted connection from client");

                // Read HTTP request
                char buffer[4096];
                int bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);

                if (bytesRead > 0)
                {
                    buffer[bytesRead] = '\0';
                    juce::String request(buffer);

                    spdlog::debug("[Tone3000Auth] Received callback request: {}",
                                  request.substring(0, 100).toStdString());

                    juce::String firstLine = request.upToFirstOccurrenceOf("\r\n", false, false);
                    juce::String authCode = extractAuthCode(firstLine);
                    juce::String state = extractState(firstLine);

                    auto sendHttpResponse =
                        [&](int statusCode, const juce::String& statusText, const juce::String& body)
                    {
                        juce::String response;
                        response << "HTTP/1.1 " << statusCode << " " << statusText << "\r\n";
                        response << "Content-Type: text/html; charset=utf-8\r\n";
                        response << "Content-Length: " << body.length() << "\r\n";
                        response << "Connection: close\r\n";
                        response << "\r\n";
                        response << body;
                        send(clientSocket, response.toRawUTF8(), response.getNumBytesAsUTF8(), 0);
                    };

                    if (authCode.isNotEmpty())
                    {
                        spdlog::info("[Tone3000Auth] Received auth code: {}...",
                                     authCode.substring(0, 10).toStdString());

                        if (state.isNotEmpty() && state != expectedState)
                        {
                            spdlog::error("[Tone3000Auth] State mismatch!");
                            sendHttpResponse(400, "Bad Request",
                                             "<html><body><h1>Authentication Failed</h1>"
                                             "<p>Security verification failed.</p></body></html>");
                            closesocket(clientSocket);
                            closesocket(listenSocket);
                            WSACleanup();
                            dispatchCompletion(false, "Security verification failed");
                            return;
                        }

                        // Exchange api_key for session tokens via POST to /auth/session
                        spdlog::info("[Tone3000Auth] Exchanging api_key for session tokens...");
                        bool success = exchangeCodeForTokens(authCode);

                        if (success)
                        {
                            sendHttpResponse(200, "OK",
                                             "<html><body><h1>Authentication Successful!</h1>"
                                             "<p>You can close this window and return to Pedalboard.</p>"
                                             "<script>window.close();</script></body></html>");
                            closesocket(clientSocket);
                            closesocket(listenSocket);
                            WSACleanup();
                            dispatchCompletion(true, "");
                        }
                        else
                        {
                            sendHttpResponse(500, "Internal Server Error",
                                             "<html><body><h1>Authentication Failed</h1>"
                                             "<p>Failed to process authentication.</p></body></html>");
                            closesocket(clientSocket);
                            closesocket(listenSocket);
                            WSACleanup();
                            dispatchCompletion(false, "Failed to exchange authorization code");
                        }
                        return;
                    }
                    else if (request.contains("error="))
                    {
                        sendHttpResponse(200, "OK", "<html><body><h1>Authentication Cancelled</h1></body></html>");
                        closesocket(clientSocket);
                        closesocket(listenSocket);
                        WSACleanup();
                        dispatchCompletion(false, "Authentication was cancelled");
                        return;
                    }
                    else
                    {
                        sendHttpResponse(404, "Not Found", "<html><body><h1>Not Found</h1></body></html>");
                    }
                }
                closesocket(clientSocket);
            }
        }
    }

    closesocket(listenSocket);
    WSACleanup();
    spdlog::info("[Tone3000Auth] Callback server stopped");

#else
    // Use JUCE sockets on non-Windows platforms
    serverSocket = std::make_unique<juce::StreamingSocket>();
    bool bound = serverSocket->createListener(callbackPort, "127.0.0.1");

    if (!bound)
    {
        spdlog::error("[Tone3000Auth] Failed to create listener on port {}", callbackPort);
        serverReady = false;
        dispatchCompletion(false, "Failed to start authentication server.");
        return;
    }

    spdlog::info("[Tone3000Auth] Server listening on port {}", callbackPort);
    serverReady = true;

    constexpr int timeoutMs = 300000;
    auto startTime = juce::Time::getMillisecondCounter();

    while (!shouldStop)
    {
        if (juce::Time::getMillisecondCounter() - startTime > timeoutMs)
        {
            dispatchCompletion(false, "Authentication timed out.");
            return;
        }

        if (serverSocket->waitUntilReady(true, 500) == 1)
        {
            auto client = std::unique_ptr<juce::StreamingSocket>(serverSocket->waitForNextConnection());
            if (client)
            {
                char buffer[4096];
                int bytesRead = client->read(buffer, sizeof(buffer) - 1, false);
                if (bytesRead > 0)
                {
                    buffer[bytesRead] = '\0';
                    juce::String request(buffer);
                    juce::String firstLine = request.upToFirstOccurrenceOf("\r\n", false, false);
                    juce::String authCode = extractAuthCode(firstLine);

                    if (authCode.isNotEmpty())
                    {
                        Tone3000::AuthTokens tokens;
                        tokens.accessToken = authCode.toStdString();
                        auto now = std::chrono::system_clock::now();
                        auto nowSecs = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
                        tokens.expiresAt = nowSecs + (30 * 24 * 60 * 60);
                        Tone3000Client::getInstance().setTokens(tokens);

                        sendResponse(*client, 200, "OK",
                                     "<html><body><h1>Authentication Successful!</h1></body></html>");
                        dispatchCompletion(true, "");
                        return;
                    }
                }
            }
        }
    }
    spdlog::info("[Tone3000Auth] Callback server stopped");
#endif
}

juce::String Tone3000Auth::generateState()
{
    static const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, sizeof(charset) - 2);

    juce::String state;
    for (int i = 0; i < 32; ++i)
        state += charset[dis(gen)];

    return state;
}

juce::String Tone3000Auth::extractAuthCode(const juce::String& requestLine)
{
    // Parse: GET /callback?api_key=XXX HTTP/1.1 (TONE3000 uses api_key, not code)
    int codeStart = requestLine.indexOf("api_key=");
    if (codeStart >= 0)
    {
        codeStart += 8; // Skip "api_key="
    }
    else
    {
        // Fallback to standard OAuth "code" parameter
        codeStart = requestLine.indexOf("code=");
        if (codeStart < 0)
            return {};
        codeStart += 5; // Skip "code="
    }

    int codeEnd = requestLine.indexOf(codeStart, "&");
    if (codeEnd < 0)
        codeEnd = requestLine.indexOf(codeStart, " ");
    if (codeEnd < 0)
        codeEnd = requestLine.length();

    return juce::URL::removeEscapeChars(requestLine.substring(codeStart, codeEnd));
}

juce::String Tone3000Auth::extractState(const juce::String& requestLine)
{
    int stateStart = requestLine.indexOf("state=");
    if (stateStart < 0)
        return {};

    stateStart += 6; // Skip "state="

    int stateEnd = requestLine.indexOf(stateStart, "&");
    if (stateEnd < 0)
        stateEnd = requestLine.indexOf(stateStart, " ");
    if (stateEnd < 0)
        stateEnd = requestLine.length();

    return juce::URL::removeEscapeChars(requestLine.substring(stateStart, stateEnd));
}

bool Tone3000Auth::exchangeCodeForTokens(const juce::String& apiKey)
{
    spdlog::info("[Tone3000Auth] Exchanging api_key for session tokens");

    juce::URL tokenUrl(TOKEN_URL);

    // TONE3000 expects JSON body with api_key
    juce::String jsonBody = "{\"api_key\":\"" + apiKey + "\"}";

    auto options = juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inPostData)
                       .withHttpRequestCmd("POST")
                       .withConnectionTimeoutMs(10000)
                       .withExtraHeaders("Content-Type: application/json");

    auto stream = tokenUrl.withPOSTData(jsonBody).createInputStream(options);

    if (!stream)
    {
        spdlog::error("[Tone3000Auth] Failed to connect to token endpoint");
        return false;
    }

    juce::String response = stream->readEntireStreamAsString();
    spdlog::debug("[Tone3000Auth] Token response: {}", response.toStdString());

    auto json = juce::JSON::parse(response);

    if (json.isVoid())
    {
        spdlog::error("[Tone3000Auth] Invalid JSON in token response");
        return false;
    }

    // Check for error
    if (json.hasProperty("error"))
    {
        spdlog::error("[Tone3000Auth] Token exchange error: {}",
                      json.getProperty("error", "Unknown").toString().toStdString());
        return false;
    }

    // Extract tokens
    juce::String accessToken = json.getProperty("access_token", "").toString();
    juce::String refreshToken = json.getProperty("refresh_token", "").toString();
    int expiresIn = static_cast<int>(json.getProperty("expires_in", 3600));

    if (accessToken.isEmpty())
    {
        spdlog::error("[Tone3000Auth] No access_token in response");
        return false;
    }

    // Calculate expiry timestamp
    auto now = std::chrono::system_clock::now();
    auto nowSecs = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();

    Tone3000::AuthTokens tokens;
    tokens.accessToken = accessToken.toStdString();
    tokens.refreshToken = refreshToken.toStdString();
    tokens.expiresAt = nowSecs + expiresIn;

    // Store tokens in client
    Tone3000Client::getInstance().setTokens(tokens);

    spdlog::info("[Tone3000Auth] Successfully obtained tokens, expires in {} seconds", expiresIn);
    return true;
}

void Tone3000Auth::sendResponse(juce::StreamingSocket& client, int statusCode, const juce::String& statusText,
                                const juce::String& body)
{
    juce::String response;
    response << "HTTP/1.1 " << statusCode << " " << statusText << "\r\n";
    response << "Content-Type: text/html; charset=utf-8\r\n";
    response << "Content-Length: " << body.length() << "\r\n";
    response << "Connection: close\r\n";
    response << "\r\n";
    response << body;

    client.write(response.toRawUTF8(), response.getNumBytesAsUTF8());
}

//==============================================================================
// Tone3000ManualAuthDialog
//==============================================================================

Tone3000ManualAuthDialog::Tone3000ManualAuthDialog(std::function<void(bool)> callback)
    : completionCallback(std::move(callback))
{
    auto& colours = ColourScheme::getInstance().colours;

    // Instructions
    instructionsLabel = std::make_unique<juce::Label>("instructions", "Automatic login failed. Please:\n\n"
                                                                      "1. Click 'Open Browser' to get your API key\n"
                                                                      "2. Copy the API key from TONE3000\n"
                                                                      "3. Paste it below and click Submit");
    instructionsLabel->setFont(juce::Font(13.0f));
    instructionsLabel->setColour(juce::Label::textColourId, colours["Text Colour"]);
    instructionsLabel->setJustificationType(juce::Justification::topLeft);
    addAndMakeVisible(instructionsLabel.get());

    // API key input
    apiKeyInput = std::make_unique<juce::TextEditor>("apiKey");
    apiKeyInput->setTextToShowWhenEmpty("Paste your API key here...", colours["Text Colour"].withAlpha(0.5f));
    apiKeyInput->setColour(juce::TextEditor::backgroundColourId, colours["Dialog Inner Background"]);
    apiKeyInput->setColour(juce::TextEditor::textColourId, colours["Text Colour"]);
    apiKeyInput->setColour(juce::TextEditor::outlineColourId, colours["Text Colour"].withAlpha(0.3f));
    apiKeyInput->addListener(this);
    addAndMakeVisible(apiKeyInput.get());

    // Buttons
    openBrowserButton = std::make_unique<juce::TextButton>("Open Browser");
    openBrowserButton->addListener(this);
    addAndMakeVisible(openBrowserButton.get());

    submitButton = std::make_unique<juce::TextButton>("Submit");
    submitButton->addListener(this);
    addAndMakeVisible(submitButton.get());

    cancelButton = std::make_unique<juce::TextButton>("Cancel");
    cancelButton->addListener(this);
    addAndMakeVisible(cancelButton.get());

    setSize(400, 220);
}

void Tone3000ManualAuthDialog::paint(juce::Graphics& g)
{
    auto& colours = ColourScheme::getInstance().colours;
    g.fillAll(colours["Window Background"]);
}

void Tone3000ManualAuthDialog::resized()
{
    auto bounds = getLocalBounds().reduced(16);

    instructionsLabel->setBounds(bounds.removeFromTop(80));
    bounds.removeFromTop(8);

    apiKeyInput->setBounds(bounds.removeFromTop(28));
    bounds.removeFromTop(16);

    auto buttonRow = bounds.removeFromTop(28);
    openBrowserButton->setBounds(buttonRow.removeFromLeft(110));
    buttonRow.removeFromLeft(8);

    cancelButton->setBounds(buttonRow.removeFromRight(70));
    buttonRow.removeFromRight(8);
    submitButton->setBounds(buttonRow.removeFromRight(70));
}

void Tone3000ManualAuthDialog::buttonClicked(juce::Button* button)
{
    if (button == openBrowserButton.get())
    {
        // Open TONE3000 auth page - redirect_url is required by the API
        // We use a localhost URL even though we won't receive the callback
        // (the user will manually copy the api_key from the redirect URL)
        juce::URL url("https://www.tone3000.com/api/v1/auth");
        url = url.withParameter("redirect_url", "http://localhost:43821/callback");
        url.launchInDefaultBrowser();
    }
    else if (button == submitButton.get())
    {
        submitApiKey();
    }
    else if (button == cancelButton.get())
    {
        if (completionCallback)
            completionCallback(false);

        if (auto* dialog = findParentComponentOfClass<juce::DialogWindow>())
            dialog->closeButtonPressed();
    }
}

void Tone3000ManualAuthDialog::textEditorReturnKeyPressed(juce::TextEditor&)
{
    submitApiKey();
}

void Tone3000ManualAuthDialog::submitApiKey()
{
    juce::String apiKey = apiKeyInput->getText().trim();

    if (apiKey.isEmpty())
    {
        apiKeyInput->setColour(juce::TextEditor::outlineColourId, ColourScheme::getInstance().colours["Danger Colour"]);
        return;
    }

    spdlog::info("[Tone3000Auth] Exchanging API key for session tokens...");

    // Exchange API key for session tokens via POST to /auth/session
    juce::URL sessionUrl("https://www.tone3000.com/api/v1/auth/session");

    // Create JSON body
    juce::String jsonBody = "{\"api_key\":\"" + apiKey + "\"}";

    auto options = juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inPostData)
                       .withHttpRequestCmd("POST")
                       .withConnectionTimeoutMs(10000)
                       .withExtraHeaders("Content-Type: application/json");

    auto stream = sessionUrl.withPOSTData(jsonBody).createInputStream(options);

    if (!stream)
    {
        spdlog::error("[Tone3000Auth] Failed to connect to session endpoint");
        apiKeyInput->setColour(juce::TextEditor::outlineColourId, ColourScheme::getInstance().colours["Danger Colour"]);
        return;
    }

    juce::String response = stream->readEntireStreamAsString();
    spdlog::debug("[Tone3000Auth] Session response: {}", response.toStdString());

    auto json = juce::JSON::parse(response);

    if (json.isVoid())
    {
        spdlog::error("[Tone3000Auth] Invalid JSON in session response");
        apiKeyInput->setColour(juce::TextEditor::outlineColourId, ColourScheme::getInstance().colours["Danger Colour"]);
        return;
    }

    // Check for error
    if (json.hasProperty("error"))
    {
        spdlog::error("[Tone3000Auth] Session exchange error: {}",
                      json.getProperty("error", "Unknown").toString().toStdString());
        apiKeyInput->setColour(juce::TextEditor::outlineColourId, ColourScheme::getInstance().colours["Danger Colour"]);
        return;
    }

    // Extract tokens
    juce::String accessToken = json.getProperty("access_token", "").toString();
    juce::String refreshToken = json.getProperty("refresh_token", "").toString();
    int expiresIn = static_cast<int>(json.getProperty("expires_in", 3600));

    if (accessToken.isEmpty())
    {
        spdlog::error("[Tone3000Auth] No access_token in session response");
        apiKeyInput->setColour(juce::TextEditor::outlineColourId, ColourScheme::getInstance().colours["Danger Colour"]);
        return;
    }

    // Calculate expiry timestamp
    auto now = std::chrono::system_clock::now();
    auto nowSecs = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();

    Tone3000::AuthTokens tokens;
    tokens.accessToken = accessToken.toStdString();
    tokens.refreshToken = refreshToken.toStdString();
    tokens.expiresAt = nowSecs + expiresIn;

    Tone3000Client::getInstance().setTokens(tokens);

    spdlog::info("[Tone3000Auth] Successfully obtained session tokens, expires in {} seconds", expiresIn);

    if (completionCallback)
        completionCallback(true);

    if (auto* dialog = findParentComponentOfClass<juce::DialogWindow>())
        dialog->closeButtonPressed();
}
