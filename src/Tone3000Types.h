/*
  ==============================================================================

    Tone3000Types.h
    Data structures for TONE3000 API integration

  ==============================================================================
*/

#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <chrono>

namespace Tone3000
{

//==============================================================================
// Authentication

struct AuthTokens
{
    std::string accessToken;
    std::string refreshToken;
    int64_t expiresAt{0};  // Unix timestamp (seconds)

    bool isValid() const
    {
        return !accessToken.empty() && expiresAt > 0;
    }

    bool isExpired() const
    {
        if (!isValid())
            return true;
        auto now = std::chrono::system_clock::now();
        auto nowSecs = std::chrono::duration_cast<std::chrono::seconds>(
            now.time_since_epoch()).count();
        return nowSecs >= expiresAt;
    }

    bool needsRefresh() const
    {
        if (!isValid())
            return true;
        auto now = std::chrono::system_clock::now();
        auto nowSecs = std::chrono::duration_cast<std::chrono::seconds>(
            now.time_since_epoch()).count();
        // Refresh if within 5 minutes of expiry
        return nowSecs >= (expiresAt - 300);
    }
};

//==============================================================================
// Search Filters

enum class GearType
{
    All,
    Amp,
    Pedal,
    FullRig,
    Outboard,
    IR
};

inline std::string gearTypeToString(GearType type)
{
    switch (type)
    {
        case GearType::Amp:      return "amp";
        case GearType::Pedal:    return "pedal";
        case GearType::FullRig:  return "full-rig";
        case GearType::Outboard: return "outboard";
        case GearType::IR:       return "ir";
        default:                 return "";
    }
}

enum class SortOrder
{
    BestMatch,
    Newest,
    Oldest,
    Trending,
    DownloadsAllTime
};

inline std::string sortOrderToString(SortOrder order)
{
    switch (order)
    {
        case SortOrder::BestMatch:        return "best-match";
        case SortOrder::Newest:           return "newest";
        case SortOrder::Oldest:           return "oldest";
        case SortOrder::Trending:         return "trending";
        case SortOrder::DownloadsAllTime: return "downloads-all-time";
        default:                          return "best-match";
    }
}

enum class ModelSize
{
    All,
    Standard,
    Lite,
    Feather,
    Nano,
    Custom
};

inline std::string modelSizeToString(ModelSize size)
{
    switch (size)
    {
        case ModelSize::Standard: return "standard";
        case ModelSize::Lite:     return "lite";
        case ModelSize::Feather:  return "feather";
        case ModelSize::Nano:     return "nano";
        case ModelSize::Custom:   return "custom";
        default:                  return "";
    }
}

//==============================================================================
// Tone/Model Information

struct ToneInfo
{
    std::string id;
    std::string name;
    std::string authorName;
    std::string authorId;
    std::string description;
    std::string gearType;       // "amp", "pedal", "full-rig", etc.
    std::string platform;       // "nam", "ir", "aida-x", etc.
    std::string thumbnailUrl;
    std::string createdAt;
    std::string licenseType;

    int downloads{0};
    int favorites{0};

    // Available sizes for this tone
    std::vector<std::string> availableSizes;

    // Model-specific info (populated when fetching download details)
    std::string modelUrl;
    std::string architecture;
    int64_t fileSize{0};

    // Local cache info
    std::string localPath;      // Empty if not cached
    bool isCached() const { return !localPath.empty(); }
};

//==============================================================================
// Search Results

struct SearchResult
{
    std::vector<ToneInfo> tones;
    int totalCount{0};
    int page{1};
    int pageSize{25};
    int totalPages{0};

    bool hasMore() const
    {
        return page < totalPages;
    }

    bool hasPrevious() const
    {
        return page > 1;
    }
};

//==============================================================================
// Download Management

enum class DownloadState
{
    Pending,
    Downloading,
    Completed,
    Failed,
    Cancelled
};

inline std::string downloadStateToString(DownloadState state)
{
    switch (state)
    {
        case DownloadState::Pending:     return "Pending";
        case DownloadState::Downloading: return "Downloading";
        case DownloadState::Completed:   return "Completed";
        case DownloadState::Failed:      return "Failed";
        case DownloadState::Cancelled:   return "Cancelled";
        default:                         return "Unknown";
    }
}

struct DownloadTask
{
    std::string toneId;
    std::string toneName;
    std::string url;
    std::string targetPath;
    DownloadState state{DownloadState::Pending};
    float progress{0.0f};       // 0.0 to 1.0
    int64_t bytesDownloaded{0};
    int64_t totalBytes{0};
    std::string errorMessage;

    bool isActive() const
    {
        return state == DownloadState::Pending || state == DownloadState::Downloading;
    }
};

//==============================================================================
// User Information

struct UserInfo
{
    std::string id;
    std::string username;
    std::string email;
    std::string avatarUrl;
    int toneCount{0};
    int downloadCount{0};
    int favoriteCount{0};
};

//==============================================================================
// API Error

struct ApiError
{
    int statusCode{0};
    std::string message;
    std::string details;

    bool isError() const { return statusCode != 0 || !message.empty(); }

    static ApiError none() { return {}; }

    static ApiError fromMessage(const std::string& msg)
    {
        ApiError err;
        err.message = msg;
        return err;
    }

    static ApiError fromHttp(int code, const std::string& msg)
    {
        ApiError err;
        err.statusCode = code;
        err.message = msg;
        return err;
    }
};

} // namespace Tone3000
