/**
 * @file smoke_test.cpp
 * @brief Basic smoke tests to verify build and library integration
 *
 * These tests verify that:
 * 1. The project builds correctly
 * 2. Core libraries (fmt, spdlog) are linked properly
 * 3. Basic functionality works
 */

#include <catch2/catch_test_macros.hpp>
#include <fmt/core.h>
#include <optional>
#include <spdlog/spdlog.h>
#include <utility>


TEST_CASE("Build verification", "[smoke]")
{
    SECTION("Project compiles")
    {
        // If we get here, compilation succeeded
        REQUIRE(true);
    }
}

TEST_CASE("fmt library integration", "[smoke][fmt]")
{
    SECTION("Basic string formatting")
    {
        std::string result = fmt::format("Hello, {}!", "World");
        REQUIRE(result == "Hello, World!");
    }

    SECTION("Number formatting")
    {
        std::string result = fmt::format("Value: {:.2f}", 3.14159);
        REQUIRE(result == "Value: 3.14");
    }

    SECTION("Integer formatting")
    {
        std::string result = fmt::format("{:04d}", 42);
        REQUIRE(result == "0042");
    }
}

TEST_CASE("spdlog library integration", "[smoke][spdlog]")
{
    SECTION("Logger creation")
    {
        // Just verify we can create and use a logger without crashing
        spdlog::info("Smoke test log message");
        REQUIRE(true);
    }

    SECTION("Format string with spdlog")
    {
        // spdlog uses fmt internally
        spdlog::debug("Debug message with value: {}", 123);
        REQUIRE(true);
    }
}

TEST_CASE("C++17 features", "[smoke][cpp17]")
{
    SECTION("Structured bindings")
    {
        std::pair<int, std::string> p{42, "test"};
        auto [num, str] = p;
        REQUIRE(num == 42);
        REQUIRE(str == "test");
    }

    SECTION("std::optional")
    {
        std::optional<int> opt = 42;
        REQUIRE(opt.has_value());
        REQUIRE(*opt == 42);
    }

    SECTION("if constexpr compiles")
    {
        if constexpr (sizeof(int) >= 4)
        {
            REQUIRE(true);
        }
    }
}
