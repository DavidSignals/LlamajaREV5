#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"
#include <array>

// ================================================================
//  LOOK AND FEEL REV5
// ================================================================
class REV5LookAndFeel : public juce::LookAndFeel_V4
{
public:
    REV5LookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h,
                           float sliderPos, float rotaryStart, float rotaryEnd,
                           juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&,
                                const juce::Colour&, bool highlighted, bool down) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&,
                         bool highlighted, bool down) override;
    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;

    static const juce::Colour colPanel, colKnob, colKnobLine, colDisplay;
    static const juce::Colour colButtonFace, colButtonText, colLedRed, colLedGreen;
    static const juce::Colour colButtonTop,  colButtonBot,  colButtonPressed;
};

// ================================================================
//  REV5 DISPLAY — LCD + doble 7-seg + llama pixel
// ================================================================
class REV5Display : public juce::Component,
                    public juce::Timer
{
public:
    REV5Display();
    ~REV5Display() override { stopTimer(); }

    void update       (const juce::String& l1, const juce::String& l2, int memNum);
    void setAudioLevel(float lvl);    // 0.0 – 1.0, llamado desde timerCallback del editor

    void paint   (juce::Graphics&) override;
    void timerCallback() override;    // anima la llama a 24fps

private:
    // ── LCD text ─────────────────────────────────────────────────
    juce::String line1     = "F01 LARGE HALL 1";
    juce::String line2     = "REV TIME 2.5s";
    int          memoryNum = 1;

    // ── Flame simulation ─────────────────────────────────────────
    static constexpr int FC = 16;   // columnas de la cuadrícula de fuego
    static constexpr int FR = 12;   // filas

    std::array<float, FC * FR> heatA{}, heatB{};   // ping-pong buffers
    float audioLevel  = 0.f;
    float smoothLevel = 0.f;

    void  stepFlame();
    juce::Colour flamePixelColor(float h) const;

    // ── 7-seg helpers ────────────────────────────────────────────
    void draw7Segment (juce::Graphics&, int num, int x, int y, int w, int h);
    void drawTwoDigit (juce::Graphics&, int num, int x, int y, int dW, int dH);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(REV5Display)
};

// ================================================================
//  VUMETER
// ================================================================
class REV5Meter : public juce::Component
{
public:
    void setLevel(float v) { level = v; repaint(); }
    void paint(juce::Graphics&) override;
private:
    float level = 0.f;
};

// ================================================================
//  EDITOR PRINCIPAL
// ================================================================
class YamahaREV5AudioProcessorEditor
    : public juce::AudioProcessorEditor,
      public juce::Timer,
      public juce::Button::Listener
{
public:
    explicit YamahaREV5AudioProcessorEditor(YamahaREV5AudioProcessor&);
    ~YamahaREV5AudioProcessorEditor() override;

    void paint      (juce::Graphics&) override;
    void resized    () override;
    void timerCallback() override;
    void buttonClicked(juce::Button*) override;

private:
    YamahaREV5AudioProcessor& proc;
    REV5LookAndFeel laf;

    enum class MenuPage { Main, ReverbTime, InitialDelay };
    MenuPage     currentPage          = MenuPage::Main;
    int          currentProgramNumber = 1;
    juce::String currentProgramName   = "LARGE HALL 1";
    int          digitEntryBuffer     = -1;

    void changeProgram (int progNum, const juce::String& name);
    void updateLCDMenu ();
    void setupKnob         (juce::Slider&);
    void setupPushButton   (juce::TextButton&);
    void setupToggleButton (juce::TextButton&);

    REV5Display lcdDisplay;
    REV5Meter   inMeter;

    juce::TextButton btnPower      {"POWER"};
    juce::TextButton btnMonoStereo {""};
    juce::TextButton btnEqOn       {""};
    juce::Slider     slInputLevel, slMix;
    juce::Slider     slEqLoFreq,   slEqLoLevel;
    juce::Slider     slEqMidFreq,  slEqMidLevel;
    juce::Slider     slEqHiFreq,   slEqHiLevel;

    juce::TextButton btnRev1{"REV1"}, btnER1{"ER1"};
    juce::TextButton btnRev2{"REV2"}, btnER2{"ER2"};
    juce::TextButton btnRev3{"REV3"}, btnOthers{"OTHERS"};
    juce::TextButton btnRev4{"REV4"}, btnUser{"USER"};

    juce::TextButton btnParam{"PARAMETER"}, btnLevel{"LEVEL"};
    juce::TextButton btnInitDly{"INITIAL"},  btn1stRef{"1ST"};
    juce::TextButton btnEqDigital{"EQ"},     btnEqDigOn{"EQ ON"};
    juce::TextButton btnDown{"DOWN"},        btnUp{"UP"};

    juce::TextButton btn7{"7"},btn4{"4"},btn1{"1"},btn0{"0"};
    juce::TextButton btn8{"8"},btn5{"5"},btn2{"2"},btnDot{"."};
    juce::TextButton btn9{"9"},btn6{"6"},btn3{"3"},btnClear{"CLEAR"};
    juce::TextButton btnStore{"STORE"}, btnRecall{"RECALL"}, btnMinus{"-"};
    juce::TextButton btnIntParam{"INT"}, btnUtility{"UTILITY"};

    juce::TextButton btnMemory{"MEMORY"};
    juce::TextButton btnMute  {"MUTE"};
    juce::TextButton btnBypass{"BYPASS"};

    using SliderAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAtt = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::unique_ptr<SliderAtt> attInput,attMix;
    std::unique_ptr<SliderAtt> attEqLoF,attEqLoG,attEqMidF,attEqMidG,attEqHiF,attEqHiG;
    std::unique_ptr<ButtonAtt> attStereo,attEqOnToggle,attBypass,attMute;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(YamahaREV5AudioProcessorEditor)
};
