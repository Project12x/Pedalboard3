/*
  ==============================================================================

    Tone3000Client.cpp
    HTTP client for TONE3000 API integration

  ==============================================================================
*/

#include "Tone3000Client.h"
#include "SettingsManager.h"

#include <spdlog/spdlog.h>

//==============================================================================
Tone3000Client& Tone3000Client::getInstance()
{
    static Tone3000Client instance;
    return instance;
}

Tone3000Client::Tone3000Client()
{
    loadTokensFromSettings();
}

//==============================================================================
// Authentication

bool Tone3000Client::isAuthenticated() const
{
    std::lock_guard<std::mutex> lock(tokenMutex);
    return authTokens.isValid() && !authTokens.isExpired();
}

Tone3000::AuthTokens Tone3000Client::getTokens() const
{
    std::lock_guard<std::mutex> lock(tokenMutex);
    return authTokens;
}

void Tone3000Client::setTokens(const Tone3000::AuthTokens& tokens)
{
    {
        std::lock_guard<std::mutex> lock(tokenMutex);
        authTokens = tokens;
    }
    saveTokensToSettings();
    spdlog::info("[Tone3000Client] Tokens updated, expires at {}", tokens.expiresAt);
}

void Tone3000Client::logout()
{
    {
        std::lock_guard<std::mutex> lock(tokenMutex);
        authTokens = Tone3000::AuthTokens{};
    }
    saveTokensToSettings();
    spdlog::info("[Tone3000Client] Logged out");
}

bool Tone3000Client::refreshTokenIfNeeded()
{
    Tone3000::AuthTokens currentTokens;
    {
        std::lock_guard<std::mutex> lock(tokenMutex);
        currentTokens = authTokens;
    }

    if (!currentTokens.needsRefresh())
        return true;

    if (currentTokens.refreshToken.empty())
    {
        spdlog::warn("[Tone3000Client] No refresh token available");
        return false;
    }

    spdlog::info("[Tone3000Client] Refreshing access token...");

    // Build refresh request
    juce::URL refreshUrl(juce::String(API_BASE_URL) + "/auth/session/refresh");

    juce::String postData = "refresh_token=" + juce::URL::addEscapeChars(
        juce::String(currentTokens.refreshToken), true);

    Tone3000::ApiError error;
    auto options = juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inPostData)
        .withHttpRequestCmd("POST")
        .withExtraHeaders("Content-Type: application/x-www-form-urlencoded");

    auto stream = refreshUrl.withPOSTData(postData).createInputStream(options);

    if (!stream)
    {
        spdlog::error("[Tone3000Client] Failed to connect for token refresh");
        return false;
    }

    juce::String response = stream->readEntireStreamAsString();
    auto json = juce::JSON::parse(response);

    if (json.isVoid())
    {
        spdlog::error("[Tone3000Client] Invalid JSON response from refresh");
        return false;
    }

    // Parse new tokens
    juce::String newAccessToken = json.getProperty("access_token", "").toString();
    juce::String newRefreshToken = json.getProperty("refresh_token", juce::String(currentTokens.refreshToken)).toString();
    int expiresIn = json.getProperty("expires_in", 3600);

    if (newAccessToken.isEmpty())
    {
        spdlog::error("[Tone3000Client] No access_token in refresh response");
        return false;
    }

    auto now = std::chrono::system_clock::now();
    auto nowSecs = std::chrono::duration_cast<std::chrono::seconds>(
        now.time_since_epoch()).count();

    Tone3000::AuthTokens newTokens;
    newTokens.accessToken = newAccessToken.toStdString();
    newTokens.refreshToken = newRefreshToken.toStdString();
    newTokens.expiresAt = nowSecs + expiresIn;

    setTokens(newTokens);
    spdlog::info("[Tone3000Client] Token refresh successful");
    return true;
}

void Tone3000Client::getCurrentUser(std::function<void(Tone3000::UserInfo, Tone3000::ApiError)> callback)
{
    if (!isAuthenticated())
    {
        callback({}, Tone3000::ApiError::fromMessage("Not authenticated"));
        return;
    }

    makeAsyncGetRequest(buildAuthorizedUrl("/user"),
        [this, callback](juce::var result, Tone3000::ApiError error) {
            if (error.isError())
            {
                callback({}, error);
                return;
            }
            callback(parseUserInfo(result), Tone3000::ApiError::none());
        });
}

//==============================================================================
// Search API

void Tone3000Client::search(
    const juce::String& query,
    Tone3000::GearType gearType,
    Tone3000::SortOrder sortOrder,
    int page,
    std::function<void(Tone3000::SearchResult, Tone3000::ApiError)> callback)
{
    juce::URL url(juce::String(API_BASE_URL) + "/tones/search");

    // Add query parameters
    if (query.isNotEmpty())
        url = url.withParameter("query", query);

    url = url.withParameter("page", juce::String(page));
    url = url.withParameter("page_size", juce::String(DEFAULT_PAGE_SIZE));
    url = url.withParameter("sort", juce::String(Tone3000::sortOrderToString(sortOrder)));

    // Filter by gear type
    if (gearType != Tone3000::GearType::All)
        url = url.withParameter("gear", juce::String(Tone3000::gearTypeToString(gearType)));

    // Filter to NAM models only
    url = url.withParameter("platform", "nam");

    spdlog::debug("[Tone3000Client] Search: {}", url.toString(true).toStdString());

    makeAsyncGetRequest(url,
        [this, callback](juce::var result, Tone3000::ApiError error) {
            if (error.isError())
            {
                callback({}, error);
                return;
            }
            callback(parseSearchResult(result), Tone3000::ApiError::none());
        });
}

void Tone3000Client::getFavorites(
    int page,
    std::function<void(Tone3000::SearchResult, Tone3000::ApiError)> callback)
{
    if (!isAuthenticated())
    {
        callback({}, Tone3000::ApiError::fromMessage("Not authenticated"));
        return;
    }

    juce::URL url = buildAuthorizedUrl("/tones/favorited");
    url = url.withParameter("page", juce::String(page));
    url = url.withParameter("page_size", juce::String(DEFAULT_PAGE_SIZE));

    makeAsyncGetRequest(url,
        [this, callback](juce::var result, Tone3000::ApiError error) {
            if (error.isError())
            {
                callback({}, error);
                return;
            }
            callback(parseSearchResult(result), Tone3000::ApiError::none());
        });
}

//==============================================================================
// Model Details

void Tone3000Client::getModelDownloadInfo(
    const juce::String& toneId,
    std::function<void(juce::String url, int64_t fileSize, Tone3000::ApiError)> callback)
{
    if (!isAuthenticated())
    {
        callback("", 0, Tone3000::ApiError::fromMessage("Not authenticated"));
        return;
    }

    juce::URL url(juce::String(API_BASE_URL) + "/models");
    url = url.withParameter("tone_id", toneId);

    spdlog::debug("[Tone3000Client] Getting model info for tone: {}", toneId.toStdString());

    makeAsyncGetRequest(url,
        [callback](juce::var result, Tone3000::ApiError error) {
            if (error.isError())
            {
                spdlog::error("[Tone3000Client] Failed to get model info: {}", error.message);
                callback("", 0, error);
                return;
            }

            // Parse model download info - TONE3000 API returns "data" array with "model_url" field
            auto data = result.getProperty("data", juce::var());
            if (data.isArray() && data.size() > 0)
            {
                auto firstModel = data[0];
                juce::String modelUrl = firstModel.getProperty("model_url", "").toString();
                // Size might be in "size" or "file_size" field
                int64_t fileSize = static_cast<int64_t>(firstModel.getProperty("file_size",
                    firstModel.getProperty("size", 0)));

                spdlog::info("[Tone3000Client] Got model URL: {}...", modelUrl.substring(0, 50).toStdString());
                callback(modelUrl, fileSize, Tone3000::ApiError::none());
            }
            else
            {
                spdlog::error("[Tone3000Client] No model data in response");
                callback("", 0, Tone3000::ApiError::fromMessage("No model data found"));
            }
        });
}

//==============================================================================
// Rate Limiting

bool Tone3000Client::canMakeRequest() const
{
    std::lock_guard<std::mutex> lock(rateLimitMutex);

    auto now = std::chrono::steady_clock::now();
    auto windowStart = now - std::chrono::seconds(RATE_WINDOW_SECONDS);

    // Remove old timestamps outside the window
    while (!requestTimestamps.empty() && requestTimestamps.front() < windowStart)
        requestTimestamps.pop_front();

    return static_cast<int>(requestTimestamps.size()) < RATE_LIMIT;
}

int Tone3000Client::getRemainingRequests() const
{
    std::lock_guard<std::mutex> lock(rateLimitMutex);

    auto now = std::chrono::steady_clock::now();
    auto windowStart = now - std::chrono::seconds(RATE_WINDOW_SECONDS);

    // Count requests in current window
    int count = 0;
    for (const auto& ts : requestTimestamps)
    {
        if (ts >= windowStart)
            ++count;
    }

    return std::max(0, RATE_LIMIT - count);
}

int Tone3000Client::getSecondsUntilReset() const
{
    std::lock_guard<std::mutex> lock(rateLimitMutex);

    if (requestTimestamps.empty())
        return 0;

    auto now = std::chrono::steady_clock::now();
    auto oldestInWindow = requestTimestamps.front();
    auto resetTime = oldestInWindow + std::chrono::seconds(RATE_WINDOW_SECONDS);

    if (resetTime <= now)
        return 0;

    return static_cast<int>(
        std::chrono::duration_cast<std::chrono::seconds>(resetTime - now).count());
}

void Tone3000Client::recordRequest()
{
    std::lock_guard<std::mutex> lock(rateLimitMutex);
    requestTimestamps.push_back(std::chrono::steady_clock::now());

    // Cleanup old entries
    auto windowStart = std::chrono::steady_clock::now() - std::chrono::seconds(RATE_WINDOW_SECONDS);
    while (!requestTimestamps.empty() && requestTimestamps.front() < windowStart)
        requestTimestamps.pop_front();
}

//==============================================================================
// HTTP Helpers

juce::var Tone3000Client::makeGetRequest(const juce::URL& url, Tone3000::ApiError& error)
{
    if (!canMakeRequest())
    {
        error = Tone3000::ApiError::fromMessage("Rate limit exceeded. Please wait.");
        return {};
    }

    recordRequest();

    juce::String headers = "Content-Type: application/json\r\n";
    {
        std::lock_guard<std::mutex> lock(tokenMutex);
        if (authTokens.isValid())
            headers += "Authorization: Bearer " + juce::String(authTokens.accessToken);
    }

    spdlog::debug("[Tone3000Client] GET {}", url.toString(true).toStdString());

    auto options = juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
        .withConnectionTimeoutMs(10000)
        .withExtraHeaders(headers);

    auto stream = url.createInputStream(options);

    if (!stream)
    {
        error = Tone3000::ApiError::fromMessage("Failed to connect to TONE3000");
        return {};
    }

    juce::String response = stream->readEntireStreamAsString();

    if (response.isEmpty())
    {
        error = Tone3000::ApiError::fromMessage("Empty response from server");
        return {};
    }

    auto json = juce::JSON::parse(response);

    if (json.isVoid())
    {
        error = Tone3000::ApiError::fromMessage("Invalid JSON response");
        return {};
    }

    // Check for API error in response
    if (json.hasProperty("error"))
    {
        error = Tone3000::ApiError::fromMessage(
            json.getProperty("error", "Unknown error").toString().toStdString());
        return {};
    }

    error = Tone3000::ApiError::none();
    return json;
}

void Tone3000Client::makeAsyncGetRequest(
    const juce::URL& url,
    std::function<void(juce::var result, Tone3000::ApiError error)> callback)
{
    // Capture URL string for the thread
    juce::String urlString = url.toString(true);

    std::thread([this, urlString, callback]() {
        juce::URL url(urlString);
        Tone3000::ApiError error;
        auto result = makeGetRequest(url, error);

        // Call back on message thread
        juce::MessageManager::callAsync([callback, result, error]() {
            callback(result, error);
        });
    }).detach();
}

juce::URL Tone3000Client::buildAuthorizedUrl(const juce::String& endpoint) const
{
    return juce::URL(juce::String(API_BASE_URL) + endpoint);
}

//==============================================================================
// Token Persistence

void Tone3000Client::loadTokensFromSettings()
{
    auto& settings = SettingsManager::getInstance();

    std::lock_guard<std::mutex> lock(tokenMutex);
    authTokens.accessToken = settings.getString("tone3000_access_token", "").toStdString();
    authTokens.refreshToken = settings.getString("tone3000_refresh_token", "").toStdString();
    authTokens.expiresAt = static_cast<int64_t>(settings.getDouble("tone3000_token_expires", 0.0));

    if (authTokens.isValid())
        spdlog::info("[Tone3000Client] Loaded tokens from settings, expires at {}", authTokens.expiresAt);
}

void Tone3000Client::saveTokensToSettings()
{
    auto& settings = SettingsManager::getInstance();

    std::lock_guard<std::mutex> lock(tokenMutex);
    settings.setValue("tone3000_access_token", juce::String(authTokens.accessToken));
    settings.setValue("tone3000_refresh_token", juce::String(authTokens.refreshToken));
    settings.setValue("tone3000_token_expires", static_cast<double>(authTokens.expiresAt));
}

//==============================================================================
// JSON Parsing

Tone3000::ToneInfo Tone3000Client::parseToneInfo(const juce::var& json)
{
    Tone3000::ToneInfo info;

    info.id = json.getProperty("id", "").toString().toStdString();
    info.name = json.getProperty("title", json.getProperty("name", "")).toString().toStdString();
    info.description = json.getProperty("description", "").toString().toStdString();
    info.gearType = json.getProperty("gear_type", json.getProperty("gear", "")).toString().toStdString();
    info.platform = json.getProperty("platform", "nam").toString().toStdString();
    info.thumbnailUrl = json.getProperty("thumbnail_url", "").toString().toStdString();
    info.createdAt = json.getProperty("created_at", "").toString().toStdString();
    info.licenseType = json.getProperty("license_type", "").toString().toStdString();

    // Parse user/author info
    auto user = json.getProperty("user", juce::var());
    if (!user.isVoid())
    {
        info.authorId = user.getProperty("id", "").toString().toStdString();
        info.authorName = user.getProperty("username", user.getProperty("name", "")).toString().toStdString();
    }
    else
    {
        info.authorName = json.getProperty("author", "Unknown").toString().toStdString();
    }

    info.downloads = static_cast<int>(json.getProperty("downloads", json.getProperty("download_count", 0)));
    info.favorites = static_cast<int>(json.getProperty("favorites", json.getProperty("favorite_count", 0)));

    // Parse available sizes
    auto sizes = json.getProperty("sizes", json.getProperty("available_sizes", juce::var()));
    if (sizes.isArray())
    {
        for (int i = 0; i < sizes.size(); ++i)
            info.availableSizes.push_back(sizes[i].toString().toStdString());
    }

    return info;
}

Tone3000::SearchResult Tone3000Client::parseSearchResult(const juce::var& json)
{
    Tone3000::SearchResult result;

    // Parse pagination info
    result.page = static_cast<int>(json.getProperty("page", 1));
    result.pageSize = static_cast<int>(json.getProperty("page_size", DEFAULT_PAGE_SIZE));
    result.totalCount = static_cast<int>(json.getProperty("total", 0));
    result.totalPages = static_cast<int>(json.getProperty("total_pages",
        (result.totalCount + result.pageSize - 1) / result.pageSize));

    // Parse tones array
    auto data = json.getProperty("data", juce::var());
    if (data.isArray())
    {
        for (int i = 0; i < data.size(); ++i)
        {
            result.tones.push_back(parseToneInfo(data[i]));
        }
    }

    spdlog::debug("[Tone3000Client] Parsed {} tones, page {}/{}, total {}",
        result.tones.size(), result.page, result.totalPages, result.totalCount);

    return result;
}

Tone3000::UserInfo Tone3000Client::parseUserInfo(const juce::var& json)
{
    Tone3000::UserInfo info;

    info.id = json.getProperty("id", "").toString().toStdString();
    info.username = json.getProperty("username", "").toString().toStdString();
    info.email = json.getProperty("email", "").toString().toStdString();
    info.avatarUrl = json.getProperty("avatar_url", "").toString().toStdString();
    info.toneCount = static_cast<int>(json.getProperty("tone_count", 0));
    info.downloadCount = static_cast<int>(json.getProperty("download_count", 0));
    info.favoriteCount = static_cast<int>(json.getProperty("favorite_count", 0));

    return info;
}
