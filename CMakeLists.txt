cmake_minimum_required(VERSION 3.22)
project(YamahaREV5 VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(/Users/david/Downloads/JUCE JUCE)

juce_add_plugin(YamahaREV5
    PRODUCT_NAME "YamahaREV5"
    COMPANY_NAME "DRabago"
    FORMATS AU VST3 Standalone
    PLUGIN_MANUFACTURER_CODE DRab
    PLUGIN_CODE_ReV5
    VERSION "1.0.0"
    DESCRIPTION "Yamaha REV5 Digital Reverb Emulation"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
)

juce_add_binary_data(YamahaREV5Assets
    SOURCES
        Assets/panel_bg.png
        Assets/knob_strip.png
        Assets/button_normal.png
        Assets/button_pressed.png
        Assets/display_bg.png
        Assets/led_green.png
        Assets/led_red.png
        Assets/screw.png
)

target_sources(YamahaREV5 PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
)

target_link_libraries(YamahaREV5 PRIVATE
    YamahaREV5Assets
    juce::juce_audio_basics
    juce::juce_audio_plugin_client
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_graphics
    juce::juce_gui_basics
    juce::juce_gui_extra
)

if(APPLE)
    set_target_properties(YamahaREV5 PROPERTIES
        XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "Apple Development"
        MACOSX_DEPLOYMENT_TARGET "10.15"
    )
target_compile_definitions(YamahaREV5 PRIVATE JUCE_VST3_CAN_REPLACE_VST2=0)
endif()
