cmake_minimum_required(VERSION 3.24)

project(JGKSongwriterVocalSuite VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
)
FetchContent_MakeAvailable(JUCE)

set(JGK_COMMON_SOURCES
    Source/Common/JGKLookAndFeel.h
    Source/Common/JGKLookAndFeel.cpp
    Source/DSP/DSPModules.h
    Source/DSP/DSPModules.cpp
    Source/Plugin/PluginProcessor.h
    Source/Plugin/PluginProcessor.cpp
    Source/Plugin/PluginEditor.h
    Source/Plugin/PluginEditor.cpp
)

function(add_jgk_plugin target product_name plugin_code plugin_kind)
    juce_add_plugin(${target}
        COMPANY_NAME "JGK"
        BUNDLE_ID "com.jgk.songwriter.${target}"
        PLUGIN_MANUFACTURER_CODE JgkA
        PLUGIN_CODE ${plugin_code}
        FORMATS VST3 Standalone
        PRODUCT_NAME "${product_name}"
        DESCRIPTION "JGK Songwriter Series - ${product_name}"
        IS_SYNTH FALSE
        NEEDS_MIDI_INPUT FALSE
        NEEDS_MIDI_OUTPUT FALSE
        IS_MIDI_EFFECT FALSE
        EDITOR_WANTS_KEYBOARD_FOCUS FALSE
        COPY_PLUGIN_AFTER_BUILD FALSE
    )

    juce_generate_juce_header(${target})
    target_sources(${target} PRIVATE ${JGK_COMMON_SOURCES})
    target_compile_definitions(${target} PRIVATE
        JGK_PLUGIN_KIND=${plugin_kind}
        JGK_PRODUCT_NAME="${product_name}"
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
    )
    target_link_libraries(${target} PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
        PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
    )
endfunction()

# One monorepo, nine separate products/binaries.
add_jgk_plugin(JGKTune       "TUNE"        JTuN 1)
add_jgk_plugin(JGKEQ         "EQ"          JEq1 2)
add_jgk_plugin(JGKComp       "COMP"        JCmp 3)
add_jgk_plugin(JGKDeEss      "DE-ESS"      JDes 4)
add_jgk_plugin(JGKSaturation "SAT"         JSat 5)
add_jgk_plugin(JGKSpace      "SPACE"       JSpc 6)
add_jgk_plugin(JGKEcho       "ECHO"        JEch 7)
add_jgk_plugin(JGKLimiter    "LIMITER"     JLim 8)
add_jgk_plugin(JGKVocalChain "VOCAL CHAIN" JChn 9)
