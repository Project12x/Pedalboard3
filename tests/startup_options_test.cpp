#include "StartupOptions.h"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("startup smoke test option is explicit", "[startup][macos]")
{
    CHECK_FALSE(pedalboard3::StartupOptions::parse("").startupSmokeTest);
    CHECK_FALSE(pedalboard3::StartupOptions::parse("--debug").startupSmokeTest);
    CHECK(pedalboard3::StartupOptions::parse("--debug --startup-smoke-test").startupSmokeTest);
}
