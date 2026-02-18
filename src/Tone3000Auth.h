/*
  ==============================================================================

    Tone3000Auth.h
    OAuth authentication handler for TONE3000 API

  ==============================================================================
*/

#pragma once

#include "Tone3000Types.h"
#include <JuceHeader.h>

#include <functional>
#include <memory>
#include <atomic>
#include <mutex>

//==============================================================================
/**
    Handles OAuth authentication flow for TONE3000.

    Flow:
    1. Opens browser to TONE3000 OAuth page
    2. Starts local HTTP server to receive callback
    3. Captures authorization code from redirect
    4. Exchanges code for access/refresh tokens
    5. Stores tokens via Tone3000Client
*/
class Tone3000Auth : public juce::Thread
{
public:
    Tone3000Auth();
    ~Tone3000Auth() override;

    //==========================================================================
    // Authentication Flow

    /// Start the OAuth authentication process
    /// Opens browser and waits for callback
    void startAuthentication(std::function<void(bool success, juce::String errorMessage)> callback);

    /// Cancel ongoing authentication
    void cancelAuthentication();

    /// Check if authentication is in progress
    bool isAuthenticating() const;

    //==========================================================================
    // Configuration

    /// Set the callback port (default: 43821)
    void setCallbackPort(int port) { callbackPort = port; }

    /// Get the callback port
    int getCallbackPort() const { return callbackPort; }

private:
    void run() override;

    /// Generate random state string for CSRF protection
    static juce::String generateState();

    /// Parse authorization code from callback URL
    static juce::String extractAuthCode(const juce::String& requestLine);

    /// Extract state parameter from callback URL
    static juce::String extractState(const juce::String& requestLine);

    /// Exchange authorization code for tokens
    bool exchangeCodeForTokens(const juce::String& code);

    /// Dispatch completion callback exactly once.
    void dispatchCompletion(bool success, const juce::String& errorMessage);

    /// Send HTTP response back to browser
    void sendResponse(juce::StreamingSocket& client, int statusCode,
                      const juce::String& statusText, const juce::String& body);

    //==========================================================================
    // Members

    std::function<void(bool, juce::String)> completionCallback;
    std::mutex completionCallbackMutex;
    std::atomic<bool> completionDispatched{false};
    std::unique_ptr<juce::StreamingSocket> serverSocket;
    std::atomic<bool> shouldStop{false};
    std::atomic<bool> serverReady{false};

    juce::String expectedState;
    int callbackPort{43821};

    // OAuth configuration
    static constexpr const char* AUTH_URL = "https://www.tone3000.com/api/v1/auth";
    static constexpr const char* TOKEN_URL = "https://www.tone3000.com/api/v1/auth/session";

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Tone3000Auth)
};

//==============================================================================
/**
    Simple dialog for manual token entry (fallback when local server fails)
*/
class Tone3000ManualAuthDialog : public juce::Component,
                                   public juce::Button::Listener,
                                   public juce::TextEditor::Listener
{
public:
    Tone3000ManualAuthDialog(std::function<void(bool success)> callback);
    ~Tone3000ManualAuthDialog() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void buttonClicked(juce::Button* button) override;
    void textEditorReturnKeyPressed(juce::TextEditor& editor) override;

private:
    void submitApiKey();

    std::function<void(bool)> completionCallback;

    std::unique_ptr<juce::Label> instructionsLabel;
    std::unique_ptr<juce::TextEditor> apiKeyInput;
    std::unique_ptr<juce::TextButton> submitButton;
    std::unique_ptr<juce::TextButton> cancelButton;
    std::unique_ptr<juce::TextButton> openBrowserButton;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Tone3000ManualAuthDialog)
};
