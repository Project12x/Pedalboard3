# Tests CMake configuration
# Using Catch2 testing framework

# Add test executable
add_executable(Pedalboard2_Tests
    smoke_test.cpp
)

# Link Catch2 and project dependencies needed for testing
target_link_libraries(Pedalboard2_Tests PRIVATE
    Catch2::Catch2WithMain
    fmt::fmt
    spdlog::spdlog
)

# Enable C++17
target_compile_features(Pedalboard2_Tests PRIVATE cxx_std_17)

# Register tests with CTest
include(Catch)
catch_discover_tests(Pedalboard2_Tests)
