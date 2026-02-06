/*
  ==============================================================================

    Tone3000Client.h
    HTTP client for TONE3000 API integration

  ==============================================================================
*/

#pragma once

#include "Tone3000Types.h"
#include <JuceHeader.h>

#include <functional>
#include <memory>
#include <mutex>
#include <atomic>
#include <chrono>
#include <deque>

//==============================================================================
/**
    Singleton HTTP client for TONE3000 API.
    Handles authentication, search, and model info retrieval.
    Implements rate limiting (100 requests per minute).
*/
class Tone3000Client
{
public:
    static Tone3000Client& getInstance();

    //==========================================================================
    // Authentication

    /// Returns true if we have valid (non-expired) tokens
    bool isAuthenticated() const;

    /// Get current auth tokens (may be empty)
    Tone3000::AuthTokens getTokens() const;

    /// Set tokens (called after OAuth flow completes)
    void setTokens(const Tone3000::AuthTokens& tokens);

    /// Clear tokens (logout)
    void logout();

    /// Refresh tokens if needed. Returns true if refresh succeeded or wasn't needed.
    bool refreshTokenIfNeeded();

    /// Get current user info (requires authentication)
    void getCurrentUser(std::function<void(Tone3000::UserInfo, Tone3000::ApiError)> callback);

    //==========================================================================
    // Search API

    /// Search for tones/models
    void search(
        const juce::String& query,
        Tone3000::GearType gearType,
        Tone3000::SortOrder sortOrder,
        int page,
        std::function<void(Tone3000::SearchResult, Tone3000::ApiError)> callback);

    /// Get user's favorited tones
    void getFavorites(
        int page,
        std::function<void(Tone3000::SearchResult, Tone3000::ApiError)> callback);

    //==========================================================================
    // Model Details

    /// Get download URL and details for a specific tone
    void getModelDownloadInfo(
        const juce::String& toneId,
        std::function<void(juce::String url, int64_t fileSize, Tone3000::ApiError)> callback);

    //==========================================================================
    // Rate Limiting

    /// Returns true if we can make another request without exceeding rate limit
    bool canMakeRequest() const;

    /// Get number of requests remaining in current window
    int getRemainingRequests() const;

    /// Get seconds until rate limit window resets
    int getSecondsUntilReset() const;

private:
    Tone3000Client();
    ~Tone3000Client() = default;

    //==========================================================================
    // HTTP Helpers

    /// Make synchronous GET request (runs on calling thread)
    juce::var makeGetRequest(const juce::URL& url, Tone3000::ApiError& error);

    /// Make asynchronous GET request (runs on background thread)
    void makeAsyncGetRequest(
        const juce::URL& url,
        std::function<void(juce::var result, Tone3000::ApiError error)> callback);

    /// Build URL with auth header
    juce::URL buildAuthorizedUrl(const juce::String& endpoint) const;

    /// Record a request for rate limiting
    void recordRequest();

    //==========================================================================
    // Token Persistence

    void loadTokensFromSettings();
    void saveTokensToSettings();

    //==========================================================================
    // JSON Parsing

    Tone3000::ToneInfo parseToneInfo(const juce::var& json);
    Tone3000::SearchResult parseSearchResult(const juce::var& json);
    Tone3000::UserInfo parseUserInfo(const juce::var& json);

    //==========================================================================
    // Members

    Tone3000::AuthTokens authTokens;
    mutable std::mutex tokenMutex;

    // Rate limiting: track requests in sliding window
    mutable std::mutex rateLimitMutex;
    mutable std::deque<std::chrono::steady_clock::time_point> requestTimestamps;

    // API configuration
    static constexpr const char* API_BASE_URL = "https://www.tone3000.com/api/v1";
    static constexpr int RATE_LIMIT = 100;
    static constexpr int RATE_WINDOW_SECONDS = 60;
    static constexpr int DEFAULT_PAGE_SIZE = 25;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Tone3000Client)
};
