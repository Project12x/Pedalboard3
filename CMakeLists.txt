# ==============================================================================
# Pedalboard2 Modern - CMake Build Configuration
# Based on JUCE 8.x with VST3 plugin hosting support
# ==============================================================================

cmake_minimum_required(VERSION 3.22)

project(Pedalboard2
    VERSION 3.0.0
    DESCRIPTION "Modern VST3 Pedalboard Software for Live Performance"
    LANGUAGES CXX
)

# ==============================================================================
# CPM.cmake - Dependency Manager
# ==============================================================================
include(cmake/CPM.cmake)

# ==============================================================================
# Modern C++ Libraries
# ==============================================================================

# fmt - Modern string formatting
CPMAddPackage(
    NAME fmt
    GITHUB_REPOSITORY fmtlib/fmt
    GIT_TAG 11.0.2
    OPTIONS "FMT_INSTALL OFF"
)

# spdlog - Fast async logging (uses fmt)
CPMAddPackage(
    NAME spdlog
    GITHUB_REPOSITORY gabime/spdlog
    GIT_TAG v1.14.1
    OPTIONS "SPDLOG_FMT_EXTERNAL ON"
)

# nlohmann_json - JSON parsing
CPMAddPackage(
    NAME nlohmann_json
    GITHUB_REPOSITORY nlohmann/json
    GIT_TAG v3.11.3
    OPTIONS "JSON_BuildTests OFF"
)

# ==============================================================================
# Testing Framework
# ==============================================================================
option(PEDALBOARD2_BUILD_TESTS "Build tests" ON)

if(PEDALBOARD2_BUILD_TESTS)
    CPMAddPackage(
        NAME Catch2
        GITHUB_REPOSITORY catchorg/Catch2
        GIT_TAG v3.7.0
    )
    list(APPEND CMAKE_MODULE_PATH ${Catch2_SOURCE_DIR}/extras)
    enable_testing()
endif()

# ==============================================================================
# Compiler Warnings
# ==============================================================================
if(MSVC)
    add_compile_options(/W4 /WX-)  # High warnings, don't treat as errors yet
    add_compile_options(/wd4100)   # Unreferenced formal parameter (common in JUCE)
    add_compile_options(/wd4244)   # Conversion warnings (JUCE has many)
    add_compile_options(/wd4267)   # Size_t conversion (JUCE has many)
    add_compile_options(/FS)       # Force synchronous PDB writes (required for Ninja parallel builds)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
    add_compile_options(-Wno-unused-parameter)  # Common in JUCE callbacks
endif()

# Fix for potential CMake/MSVC 2026 compatibility issues where compile rules are missing
if(MSVC)
    if(NOT DEFINED CMAKE_C_COMPILE_OBJECT)
        set(CMAKE_C_COMPILE_OBJECT "<CMAKE_C_COMPILER> <DEFINES> <INCLUDES> <FLAGS> /Fo<OBJECT> /Fd<TARGET_COMPILE_PDB> -c <SOURCE>")
    endif()
    if(NOT DEFINED CMAKE_CXX_COMPILE_OBJECT)
        set(CMAKE_CXX_COMPILE_OBJECT "<CMAKE_CXX_COMPILER> <DEFINES> <INCLUDES> <FLAGS> /Fo<OBJECT> /Fd<TARGET_COMPILE_PDB> -c <SOURCE>")
    endif()
endif()

# C++17 standard required for modern JUCE
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add JUCE as a subdirectory
add_subdirectory(JUCE)

# Define the application
juce_add_gui_app(Pedalboard2
    PRODUCT_NAME "Pedalboard3"
    COMPANY_NAME "Pedalboard3 Project"
    BUNDLE_ID "com.pedalboard3.app"
    ICON_BIG "${CMAKE_CURRENT_SOURCE_DIR}/icon/icon-512.png"
    ICON_SMALL "${CMAKE_CURRENT_SOURCE_DIR}/icon/icon-48.png"
)

# Generate JUCE header
juce_generate_juce_header(Pedalboard2)

# Source files - Core
target_sources(Pedalboard2 PRIVATE
    # Application
    src/App.cpp
    src/App.h
    src/MainPanel.cpp
    src/MainPanel.h
    
    # Audio Core
    src/FilterGraph.cpp
    src/FilterGraph.h
    src/AudioSingletons.cpp
    src/AudioSingletons.h
    src/BypassableInstance.cpp
    src/BypassableInstance.h
    src/InternalFilters.cpp
    src/InternalFilters.h
    
    # Plugin Components
    src/PluginComponent.cpp
    src/PluginComponent.h
    src/PluginField.cpp
    src/PluginField.h
    
    # Internal Processors
    src/PedalboardProcessors.cpp
    src/PedalboardProcessors.h
    src/PedalboardProcessorEditors.cpp
    src/PedalboardProcessorEditors.h
    
    # MIDI Handling
    src/MidiMappingManager.cpp
    src/MidiMappingManager.h
    src/MidiAppFifo.cpp
    src/MidiAppFifo.h
    src/MidiCcAlertWindow.cpp
    src/MidiCcAlertWindow.h
    src/Mapping.cpp
    src/Mapping.h
    src/MappingSlider.cpp
    src/MappingSlider.h
    src/MappingEntryMidi.cpp
    src/MappingEntryMidi.h
    
    # OSC Handling
    src/OscMappingManager.cpp
    src/OscMappingManager.h
    src/MappingEntryOsc.cpp
    src/MappingEntryOsc.h
    src/NiallsOSCLib/OSCBundle.cpp
    src/NiallsOSCLib/OSCMessage.cpp
    src/NiallsOSCLib/OSCTimeTag.cpp
    src/NiallsSocketLib/UDPSocket.cpp
    
    # UI Components
    src/MappingsDialog.cpp
    src/MappingsDialog.h
    src/PreferencesDialog.cpp
    src/PreferencesDialog.h
    src/ApplicationMappingsEditor.cpp
    src/ApplicationMappingsEditor.h
    src/ColourScheme.cpp
    src/ColourScheme.h
    src/ColourSchemeEditor.cpp
    src/ColourSchemeEditor.h
    src/BranchesLAF.cpp
    src/BranchesLAF.h
    
    # Patch Management
    src/PatchOrganiser.cpp
    src/PatchOrganiser.h
    src/PresetBar.cpp
    src/PresetBar.h
    src/PresetManager.cpp
    src/PresetManager.h
    src/UserPresetWindow.cpp
    src/UserPresetWindow.h
    
    # Internal Processor Controls
    src/FilePlayerControl.cpp
    src/FilePlayerControl.h
    src/AudioRecorderControl.cpp
    src/AudioRecorderControl.h
    src/MetronomeControl.cpp
    src/MetronomeControl.h
    src/LooperControl.cpp
    src/LooperControl.h
    src/LooperEditor.cpp
    src/LooperEditor.h
    src/WaveformDisplay.cpp
    src/WaveformDisplay.h
    
    # Utilities
    src/SettingsManager.cpp
    src/SettingsManager.h
    src/JuceHelperStuff.cpp
    src/JuceHelperStuff.h
    src/LogDisplay.cpp
    src/LogDisplay.h
    src/LogFile.cpp
    src/LogFile.h
    # src/PropertiesSingleton.cpp
    # src/PropertiesSingleton.h
    src/TapTempoBox.cpp
    src/TapTempoBox.h
    src/TapTempoHelper.cpp
    src/TapTempoHelper.h
    src/TrayIcon.cpp
    src/TrayIcon.h
    src/MainTransport.cpp
    src/MainTransport.h
    src/AboutPage.cpp
    src/AboutPage.h
    
    # Images and Vectors
    src/Images.cpp
    src/Images.h
    src/Vectors.cpp
    src/Vectors.h
    src/LookAndFeelImages.cpp
    src/LookAndFeelImages.h
    
    # Custom Plugin Formats (disabled temporarily - needs JUCE 8 API updates)
    # TODO: Update NiallsAudioPlugin for JUCE 8 compatibility
    # src/NiallsAudioPlugin.cpp
    # src/NiallsAudioPlugin.h
    # src/NiallsAudioPluginFormat.cpp
    # src/NiallsAudioPluginFormat.h
)

# Compile definitions
target_compile_definitions(Pedalboard2 PRIVATE
    # JUCE Settings
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:Pedalboard2,JUCE_PRODUCT_NAME>"
    JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:Pedalboard2,JUCE_VERSION>"
    
    # Plugin Hosting - VST3 is primary
    JUCE_PLUGINHOST_VST3=1
    JUCE_PLUGINHOST_VST=0
    JUCE_PLUGINHOST_AU=0
    JUCE_PLUGINHOST_LADSPA=0
    
    # Audio Settings
    JUCE_ASIO=1
    JUCE_WASAPI=1
    JUCE_DIRECTSOUND=1
    JUCE_USE_FLAC=1
    JUCE_USE_OGGVORBIS=1
    
    # Graphics - Enable HiDPI
    JUCE_WIN_PER_MONITOR_DPI_AWARE=1
    
    # Modern features
    JUCE_MODAL_LOOPS_PERMITTED=1
    JUCE_STRICT_REFCOUNTEDPOINTER=1
)

# Link JUCE modules
target_link_libraries(Pedalboard2 PRIVATE
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_core
    juce::juce_cryptography
    juce::juce_data_structures
    juce::juce_events
    juce::juce_graphics
    juce::juce_gui_basics
    juce::juce_gui_extra
    juce::juce_opengl
    
    # Recommended flags
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
    
    # Modern C++ libraries
    fmt::fmt
    spdlog::spdlog
    nlohmann_json::nlohmann_json
)

# Windows-specific settings
if(WIN32)
    target_compile_definitions(Pedalboard2 PRIVATE
        _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
    )
    
    # Link Windows libraries for network/socket support
    target_link_libraries(Pedalboard2 PRIVATE
        ws2_32
    )
endif()

# Copy resources to build directory (for development)
add_custom_command(TARGET Pedalboard2 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/documentation"
        "$<TARGET_FILE_DIR:Pedalboard2>/documentation"
)

# ==============================================================================
# Tests
# ==============================================================================
if(PEDALBOARD2_BUILD_TESTS)
    add_subdirectory(tests)
endif()
