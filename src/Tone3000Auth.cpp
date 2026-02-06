/*
  ==============================================================================

    Tone3000Auth.cpp
    OAuth authentication handler for TONE3000 API

  ==============================================================================
*/

#include "Tone3000Auth.h"
#include "Tone3000Client.h"
#include "ColourScheme.h"

#include <spdlog/spdlog.h>
#include <random>

//==============================================================================
// Tone3000Auth
//==============================================================================

Tone3000Auth::Tone3000Auth()
    : Thread("Tone3000Auth")
{
}

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
        callback(false, "Authentication already in progress");
        return;
    }

    completionCallback = std::move(callback);
    shouldStop = false;
    expectedState = generateState();

    // Build OAuth URL
    juce::String redirectUri = "http://localhost:" + juce::String(callbackPort) + "/callback";
    juce::URL authUrl(AUTH_URL);
    authUrl = authUrl.withParameter("redirect_url", redirectUri);

    spdlog::info("[Tone3000Auth] Starting OAuth flow, redirect: {}", redirectUri.toStdString());

    // Start the callback server thread
    startThread();

    // Give the server a moment to start
    Thread::sleep(100);

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

void Tone3000Auth::run()
{
    spdlog::info("[Tone3000Auth] Starting callback server on port {}", callbackPort);

    // Create server socket
    serverSocket = std::make_unique<juce::StreamingSocket>();

    if (!serverSocket->createListener(callbackPort, "127.0.0.1"))
    {
        spdlog::error("[Tone3000Auth] Failed to create listener on port {}", callbackPort);

        juce::MessageManager::callAsync([this]() {
            if (completionCallback)
                completionCallback(false, "Failed to start authentication server. Port may be in use.");
        });
        return;
    }

    // Wait for connection with timeout
    constexpr int timeoutMs = 300000;  // 5 minutes
    auto startTime = juce::Time::getMillisecondCounter();

    while (!shouldStop)
    {
        if (juce::Time::getMillisecondCounter() - startTime > timeoutMs)
        {
            spdlog::warn("[Tone3000Auth] Timed out waiting for callback");

            juce::MessageManager::callAsync([this]() {
                if (completionCallback)
                    completionCallback(false, "Authentication timed out. Please try again.");
            });
            return;
        }

        // Check for incoming connection
        if (serverSocket->waitUntilReady(true, 500) == 1)
        {
            auto client = std::unique_ptr<juce::StreamingSocket>(serverSocket->waitForNextConnection());

            if (client)
            {
                // Read HTTP request
                char buffer[4096];
                int bytesRead = client->read(buffer, sizeof(buffer) - 1, false);

                if (bytesRead > 0)
                {
                    buffer[bytesRead] = '\0';
                    juce::String request(buffer);

                    spdlog::debug("[Tone3000Auth] Received callback request");

                    // Extract the first line (GET /callback?code=XXX&state=YYY HTTP/1.1)
                    juce::String firstLine = request.upToFirstOccurrenceOf("\r\n", false, false);

                    // Extract authorization code and state
                    juce::String authCode = extractAuthCode(firstLine);
                    juce::String state = extractState(firstLine);

                    if (authCode.isNotEmpty())
                    {
                        // Verify state matches (CSRF protection)
                        if (state != expectedState)
                        {
                            spdlog::error("[Tone3000Auth] State mismatch! Expected: {}, got: {}",
                                expectedState.toStdString(), state.toStdString());

                            sendResponse(*client, 400, "Bad Request",
                                "<html><body><h1>Authentication Failed</h1>"
                                "<p>Security verification failed. Please try again.</p></body></html>");

                            juce::MessageManager::callAsync([this]() {
                                if (completionCallback)
                                    completionCallback(false, "Security verification failed");
                            });
                            return;
                        }

                        // Exchange code for tokens
                        bool success = exchangeCodeForTokens(authCode);

                        if (success)
                        {
                            sendResponse(*client, 200, "OK",
                                "<html><body><h1>Authentication Successful!</h1>"
                                "<p>You can close this window and return to Pedalboard.</p>"
                                "<script>window.close();</script></body></html>");

                            juce::MessageManager::callAsync([this]() {
                                if (completionCallback)
                                    completionCallback(true, "");
                            });
                        }
                        else
                        {
                            sendResponse(*client, 500, "Internal Server Error",
                                "<html><body><h1>Authentication Failed</h1>"
                                "<p>Failed to exchange authorization code. Please try again.</p></body></html>");

                            juce::MessageManager::callAsync([this]() {
                                if (completionCallback)
                                    completionCallback(false, "Failed to exchange authorization code");
                            });
                        }
                        return;
                    }
                    else
                    {
                        // Check for error in callback
                        if (request.contains("error="))
                        {
                            sendResponse(*client, 200, "OK",
                                "<html><body><h1>Authentication Cancelled</h1>"
                                "<p>You cancelled the authentication. You can close this window.</p></body></html>");

                            juce::MessageManager::callAsync([this]() {
                                if (completionCallback)
                                    completionCallback(false, "Authentication was cancelled");
                            });
                            return;
                        }

                        // Unknown request, send simple response
                        sendResponse(*client, 404, "Not Found",
                            "<html><body><h1>Not Found</h1></body></html>");
                    }
                }
            }
        }
    }

    spdlog::info("[Tone3000Auth] Callback server stopped");
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
    // Parse: GET /callback?code=XXX&state=YYY HTTP/1.1
    int codeStart = requestLine.indexOf("code=");
    if (codeStart < 0)
        return {};

    codeStart += 5;  // Skip "code="

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

    stateStart += 6;  // Skip "state="

    int stateEnd = requestLine.indexOf(stateStart, "&");
    if (stateEnd < 0)
        stateEnd = requestLine.indexOf(stateStart, " ");
    if (stateEnd < 0)
        stateEnd = requestLine.length();

    return juce::URL::removeEscapeChars(requestLine.substring(stateStart, stateEnd));
}

bool Tone3000Auth::exchangeCodeForTokens(const juce::String& code)
{
    spdlog::info("[Tone3000Auth] Exchanging authorization code for tokens");

    juce::URL tokenUrl(TOKEN_URL);

    juce::String postData = "code=" + juce::URL::addEscapeChars(code, true);

    auto options = juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inPostData)
        .withHttpRequestCmd("POST")
        .withConnectionTimeoutMs(10000)
        .withExtraHeaders("Content-Type: application/x-www-form-urlencoded");

    auto stream = tokenUrl.withPOSTData(postData).createInputStream(options);

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
    auto nowSecs = std::chrono::duration_cast<std::chrono::seconds>(
        now.time_since_epoch()).count();

    Tone3000::AuthTokens tokens;
    tokens.accessToken = accessToken.toStdString();
    tokens.refreshToken = refreshToken.toStdString();
    tokens.expiresAt = nowSecs + expiresIn;

    // Store tokens in client
    Tone3000Client::getInstance().setTokens(tokens);

    spdlog::info("[Tone3000Auth] Successfully obtained tokens, expires in {} seconds", expiresIn);
    return true;
}

void Tone3000Auth::sendResponse(juce::StreamingSocket& client, int statusCode,
                                 const juce::String& statusText, const juce::String& body)
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
    instructionsLabel = std::make_unique<juce::Label>("instructions",
        "Automatic login failed. Please:\n\n"
        "1. Click 'Open Browser' to get your API key\n"
        "2. Copy the API key from TONE3000\n"
        "3. Paste it below and click Submit");
    instructionsLabel->setFont(juce::Font(13.0f));
    instructionsLabel->setColour(juce::Label::textColourId, colours["Text Colour"]);
    instructionsLabel->setJustificationType(juce::Justification::topLeft);
    addAndMakeVisible(instructionsLabel.get());

    // API key input
    apiKeyInput = std::make_unique<juce::TextEditor>("apiKey");
    apiKeyInput->setTextToShowWhenEmpty("Paste your API key here...",
        colours["Text Colour"].withAlpha(0.5f));
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
        // Open TONE3000 API key page with OTP-only mode
        juce::URL url("https://www.tone3000.com/api/v1/auth");
        url = url.withParameter("otp_only", "true");
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
        apiKeyInput->setColour(juce::TextEditor::outlineColourId, juce::Colours::red);
        return;
    }

    // Try to use the API key to get a session
    // The API key from OTP flow can be used directly as access token
    Tone3000::AuthTokens tokens;
    tokens.accessToken = apiKey.toStdString();
    tokens.refreshToken = "";  // OTP keys don't have refresh tokens

    // Set expiry to 24 hours from now (typical for API keys)
    auto now = std::chrono::system_clock::now();
    auto nowSecs = std::chrono::duration_cast<std::chrono::seconds>(
        now.time_since_epoch()).count();
    tokens.expiresAt = nowSecs + 86400;

    Tone3000Client::getInstance().setTokens(tokens);

    if (completionCallback)
        completionCallback(true);

    if (auto* dialog = findParentComponentOfClass<juce::DialogWindow>())
        dialog->closeButtonPressed();
}
