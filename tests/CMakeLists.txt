# Tests CMake configuration
# Using Catch2 testing framework

# Add test executable
add_executable(Pedalboard3_Tests
    smoke_test.cpp
    tone_generator_test.cpp
    plugin_pool_manager_test.cpp
)

# Link Catch2 and project dependencies needed for testing
target_link_libraries(Pedalboard3_Tests PRIVATE
    Catch2::Catch2WithMain
    fmt::fmt
    spdlog::spdlog
)

# Enable C++17
target_compile_features(Pedalboard3_Tests PRIVATE cxx_std_17)

# Register tests with CTest
include(Catch)
catch_discover_tests(Pedalboard3_Tests)
