cmake_minimum_required(VERSION 3.22)

project(StutterClone
    VERSION 0.1.0
    DESCRIPTION "Tempo-synced MIDI-triggered stutter effect"
    LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(APPLE AND NOT DEFINED CMAKE_OSX_DEPLOYMENT_TARGET)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "Minimum macOS version" FORCE)
endif()

option(STUTTERCLONE_COPY_PLUGIN_AFTER_BUILD
    "Copy built plug-ins into the system plug-in folders"
    ON)

set(STUTTERCLONE_FORMATS "VST3;AU;Standalone" CACHE STRING "Formats passed to juce_add_plugin")

include(FetchContent)

set(JUCE_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(JUCE_BUILD_EXTRAS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.15
    GIT_SHALLOW    TRUE)

FetchContent_MakeAvailable(juce)

# AU must be a Music Effect (aumf) so hosts expose a MIDI input on an audio FX.
# juce_add_plugin ignores AU on Windows/Linux automatically.
juce_add_plugin(StutterClone
    COMPANY_NAME "StutterClone"
    BUNDLE_ID com.stutterclone.plugin
    PLUGIN_MANUFACTURER_CODE StCl
    PLUGIN_CODE Stc1
    FORMATS ${STUTTERCLONE_FORMATS}
    PRODUCT_NAME "StutterClone"
    PLUGIN_NAME "StutterClone"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD ${STUTTERCLONE_COPY_PLUGIN_AFTER_BUILD}
    AU_MAIN_TYPE kAudioUnitType_MusicEffect
    VST3_CATEGORIES Fx
    MICROPHONE_PERMISSION_ENABLED TRUE
    MICROPHONE_PERMISSION_TEXT "StutterClone needs microphone access to process live audio in standalone mode.")

target_sources(StutterClone
    PRIVATE
        Source/DspChain.cpp
        Source/DspChain.h
        Source/WaveformDisplay.cpp
        Source/WaveformDisplay.h
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h
        Source/PluginEditor.cpp
        Source/PluginEditor.h)

target_include_directories(StutterClone
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Source)

target_compile_definitions(StutterClone
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)

target_compile_features(StutterClone PRIVATE cxx_std_20)

target_link_libraries(StutterClone
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
