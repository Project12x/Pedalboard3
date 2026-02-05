# Tests CMake configuration
# Using Catch2 testing framework

# Add test executable
add_executable(Pedalboard3_Tests
    smoke_test.cpp
    tone_generator_test.cpp
    plugin_pool_manager_test.cpp
    subgraph_test.cpp
    integration_test.cpp
    audio_component_test.cpp
    filtergraph_test.cpp
    protection_test.cpp
    audio_thread_stress_test.cpp
    ../src/PluginPoolManager.cpp
    ../src/AudioSingletons.cpp
)



# Link Catch2 and project dependencies needed for testing
target_link_libraries(Pedalboard3_Tests PRIVATE
    Catch2::Catch2WithMain
    fmt::fmt
    spdlog::spdlog
    juce::juce_audio_processors
    juce::juce_audio_formats
    juce::juce_audio_utils
)

# Add JUCE include directories for test target
target_include_directories(Pedalboard3_Tests PRIVATE
    $<TARGET_PROPERTY:Pedalboard3,JUCE_GENERATED_SOURCES_DIRECTORY>
    ${CMAKE_SOURCE_DIR}/JUCE/modules
    ${CMAKE_BINARY_DIR}
)

# Enable C++17
target_compile_features(Pedalboard3_Tests PRIVATE cxx_std_17)

target_compile_definitions(Pedalboard3_Tests PRIVATE
    PEDALBOARD3_TESTS
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
)

# Register tests with CTest
include(Catch)
catch_discover_tests(Pedalboard3_Tests)
