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
    plugin_search_logic_test.cpp
    stage_layout_test.cpp
    ui_scale_test.cpp
    ui_regression_harness_test.cpp
    meter_source_test.cpp
    tuner_analysis_test.cpp
    tuner_processor_test.cpp
    nam_processor_test.cpp
    patch_switch_test.cpp
    vst3_loading_test.cpp
    scratch_recorder_test.cpp
    rt_hosting_sprint_test.cpp
    reverbsc_processor_test.cpp
    midi_mapping_test.cpp
    mixer_splitter_test.cpp
    master_bus_test.cpp
    font_manager_test.cpp
    ../src/ScratchRecorder.cpp
    ../src/ScratchTake.cpp
    ../src/PluginPoolManager.cpp
    ../src/MidiAppFifo.cpp
    ../src/AudioSingletons.cpp
    ../src/BypassableInstance.cpp
    ../src/dsp/ReverbSC.cpp
    ../src/dsp/MeterSource.cpp
    ../src/dsp/TunerAnalysis.cpp
    ../src/TunerProcessor.cpp
    ../src/TunerControl.cpp
    ../src/ReverbSCProcessor.cpp
    ../src/SafetyLimiter.cpp
    ../src/DeviceMeterTap.cpp
    ../src/VirtualMidiInputProcessor.cpp
    ../src/ColourScheme.cpp
    ../src/FontManager.cpp
    ../src/StageLayout.cpp
    ../src/UiScale.cpp
)



# Link Catch2 and project dependencies needed for testing
target_link_libraries(Pedalboard3_Tests PRIVATE
    Catch2::Catch2WithMain
    fmt::fmt
    spdlog::spdlog
    nlohmann_json::nlohmann_json
    juce::juce_audio_processors
    juce::juce_audio_formats
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_gui_basics
    Pedalboard3_fonts
)

# Add JUCE include directories for test target
target_include_directories(Pedalboard3_Tests PRIVATE
    $<TARGET_PROPERTY:Pedalboard3,JUCE_GENERATED_SOURCES_DIRECTORY>
    ${CMAKE_SOURCE_DIR}/JUCE/modules
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}
)

# Enable C++17
target_compile_features(Pedalboard3_Tests PRIVATE cxx_std_17)

target_compile_definitions(Pedalboard3_Tests PRIVATE
    PEDALBOARD3_TESTS
    PEDALBOARD3_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_PLUGINHOST_VST3=1
)

set(PEDALBOARD3_NAM_RUNTIME_SOURCES
    ../src/NAMCore.cpp
    ../src/NAMCore.h
    ../src/NAMCoreA2.h
    ../external/NeuralAmpModelerCore/NAM/activations.cpp
    ../external/NeuralAmpModelerCore/NAM/convnet.cpp
    ../external/NeuralAmpModelerCore/NAM/dsp.cpp
    ../external/NeuralAmpModelerCore/NAM/get_dsp.cpp
    ../external/NeuralAmpModelerCore/NAM/lstm.cpp
    ../external/NeuralAmpModelerCore/NAM/util.cpp
    ../external/NeuralAmpModelerCore/NAM/wavenet.cpp
    ../external/AudioDSPTools/dsp/dsp.cpp
    ../external/AudioDSPTools/dsp/ImpulseResponse.cpp
    ../external/AudioDSPTools/dsp/NoiseGate.cpp
    ../external/AudioDSPTools/dsp/RecursiveLinearFilter.cpp
    ../external/AudioDSPTools/dsp/wav.cpp
    ../external/NeuralAmpModelerCore/wrapper/ToneStack.cpp
)

add_executable(Pedalboard3_NAMCoreRuntimeTests
    nam_core_runtime_test.cpp
    ${PEDALBOARD3_NAM_RUNTIME_SOURCES}
)

target_link_libraries(Pedalboard3_NAMCoreRuntimeTests PRIVATE
    Catch2::Catch2WithMain
    nlohmann_json::nlohmann_json
    Eigen3::Eigen
    Pedalboard3_NAMCoreA2Adapter
)

target_include_directories(Pedalboard3_NAMCoreRuntimeTests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/external/NeuralAmpModelerCore
    ${CMAKE_SOURCE_DIR}/external/NeuralAmpModelerCore/NAM
    ${CMAKE_SOURCE_DIR}/external/NeuralAmpModelerCore/wrapper
    ${CMAKE_SOURCE_DIR}/external/AudioDSPTools
    ${CMAKE_SOURCE_DIR}/external/AudioDSPTools/dsp
    ${CMAKE_SOURCE_DIR}/external/ResamplingContainer
    ${CMAKE_BINARY_DIR}
)

target_compile_features(Pedalboard3_NAMCoreRuntimeTests PRIVATE cxx_std_17)

target_compile_definitions(Pedalboard3_NAMCoreRuntimeTests PRIVATE
    PEDALBOARD3_TESTS
    NAM_SAMPLE_FLOAT
    DSP_SAMPLE_FLOAT
)

if(WIN32)
    target_compile_definitions(Pedalboard3_NAMCoreRuntimeTests PRIVATE
        NOMINMAX
        WIN32_LEAN_AND_MEAN
        _SILENCE_CXX20_U8PATH_DEPRECATION_WARNING
    )
endif()

# Register tests with CTest
include(Catch)
catch_discover_tests(Pedalboard3_Tests)
catch_discover_tests(Pedalboard3_NAMCoreRuntimeTests)
