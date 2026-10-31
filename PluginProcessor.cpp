#include "PluginProcessor.h"
#include "PluginEditor.h"

// ================================================================
//  CONSTANTES DE DELAY — en namespace anónimo para evitar
//  problemas de linkage con constexpr float/array en clases
// ================================================================
namespace
{
    // FDN delays (ms) — números primos, spread estéreo en R
    constexpr std::array<float,8> FDN_MS_L { 29.f,37.f,41.f,43.f,47.f,53.f,59.f,61.f };
    constexpr std::array<float,8> FDN_MS_R { 30.f,38.f,43.f,44.f,49.f,55.f,61.f,63.f };

    // Early Reflection delays (ms)
    constexpr std::array<float,8> ER_L { 3.f, 7.f,11.f,17.f,23.f,31.f,41.f,53.f };
    constexpr std::array<float,8> ER_R { 5.f, 9.f,13.f,19.f,29.f,37.f,43.f,59.f };

    // Allpass pre-diffuser delays (ms)
    constexpr std::array<int,4>   AP_MS { 5, 13, 23, 31 };

    // Fast Walsh-Hadamard Transform 8x8 — solo sumas/restas, O(N log N)
    // Mezcla lossless: cada delay "escucha" a los otros 7
    static void hadamard8(std::array<float,8>& v)
    {
        // Butterfly etapa 1 — pares adyacentes
        for(int i = 0; i < 8; i += 2)
        {
            float a = v[i], b = v[i+1];
            v[i] = a + b;  v[i+1] = a - b;
        }
        // Butterfly etapa 2 — grupos de 4
        for(int i = 0; i < 8; i += 4)
            for(int j = 0; j < 2; ++j)
            {
                float a = v[i+j], b = v[i+j+2];
                v[i+j] = a + b;  v[i+j+2] = a - b;
            }
        // Butterfly etapa 3 — dos mitades
        for(int j = 0; j < 4; ++j)
        {
            float a = v[j], b = v[j+4];
            v[j] = a + b;  v[j+4] = a - b;
        }
        // Normalización: mantiene ganancia unitaria (lossless)
        constexpr float k = 1.f / 2.8284271f;   // 1/sqrt(8)
        for(auto& x : v) x *= k;
    }
}

// ================================================================
//  PARAMETER LAYOUT
// ================================================================
juce::AudioProcessorValueTreeState::ParameterLayout
YamahaREV5AudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "reverbTime","Reverb Time",
        juce::NormalisableRange<float>(0.3f,99.f,0.01f,0.35f), 2.5f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float v, int){ return juce::String(v,1)+" s"; },
        [](const juce::String& t){ return t.dropLastCharacters(2).getFloatValue(); }));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "hfRatio","HF Ratio",
        juce::NormalisableRange<float>(0.1f,1.0f,0.01f), 0.5f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "lfRatio","LF Ratio",
        juce::NormalisableRange<float>(0.1f,2.4f,0.01f), 1.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "diffusion","Diffusion",
        juce::NormalisableRange<float>(0.0f,10.f,0.1f), 7.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "erRevBalance","ER/Rev Balance",
        juce::NormalisableRange<float>(-63.f,63.f,1.f), 0.f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "erRevDelay","ER/Rev Delay",
        juce::NormalisableRange<float>(0.1f,200.f,0.1f), 20.f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float v, int){ return juce::String(v,1)+" ms"; },
        [](const juce::String& t){ return t.dropLastCharacters(3).getFloatValue(); }));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "lpfCutoff","LPF Cutoff",
        juce::NormalisableRange<float>(500.f,16000.f,1.f,0.4f), 8000.f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float v, int){ return juce::String(v,0)+" Hz"; },
        [](const juce::String& t){ return t.dropLastCharacters(3).getFloatValue(); }));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "secRevTime","Sec Rev Time",
        juce::NormalisableRange<float>(0.1f,30.f,0.01f,0.35f), 1.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float v, int){ return juce::String(v,1)+" s"; },
        [](const juce::String& t){ return t.dropLastCharacters(2).getFloatValue(); }));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "secDelay","Sec Delay",
        juce::NormalisableRange<float>(0.f,500.f,1.f), 80.f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float v, int){ return juce::String(v,0)+" ms"; },
        [](const juce::String& t){ return t.dropLastCharacters(3).getFloatValue(); }));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "secLevel","Sec Level",
        juce::NormalisableRange<float>(0.f,1.f,0.01f), 0.3f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "spaceModRate","Mod Rate",
        juce::NormalisableRange<float>(0.1f,5.f,0.01f), 0.5f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float v, int){ return juce::String(v,2)+" Hz"; },
        [](const juce::String& t){ return t.dropLastCharacters(3).getFloatValue(); }));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "spaceModDepth","Mod Depth",
        juce::NormalisableRange<float>(0.f,1.f,0.01f), 0.2f));

    // Pre-EQ
    layout.add(std::make_unique<juce::AudioParameterFloat>("preEQLowFreq","Pre EQ Low Freq",
        juce::NormalisableRange<float>(50.f,700.f,1.f,0.4f),200.f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("preEQLowGain","Pre EQ Low Gain",
        juce::NormalisableRange<float>(-15.f,15.f,0.1f),0.f, juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        [](float v,int){ return juce::String(v,1)+" dB"; },
        [](const juce::String& t){ return t.dropLastCharacters(3).getFloatValue(); }));
    layout.add(std::make_unique<juce::AudioParameterFloat>("preEQMidFreq","Pre EQ Mid Freq",
        juce::NormalisableRange<float>(350.f,5000.f,1.f,0.4f),1000.f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("preEQMidGain","Pre EQ Mid Gain",
        juce::NormalisableRange<float>(-15.f,15.f,0.1f),0.f, juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        [](float v,int){ return juce::String(v,1)+" dB"; },
        [](const juce::String& t){ return t.dropLastCharacters(3).getFloatValue(); }));
    layout.add(std::make_unique<juce::AudioParameterFloat>("preEQHighFreq","Pre EQ High Freq",
        juce::NormalisableRange<float>(2000.f,16000.f,1.f,0.4f),8000.f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("preEQHighGain","Pre EQ High Gain",
        juce::NormalisableRange<float>(-15.f,15.f,0.1f),0.f, juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        [](float v,int){ return juce::String(v,1)+" dB"; },
        [](const juce::String& t){ return t.dropLastCharacters(3).getFloatValue(); }));

    // Program EQ
    layout.add(std::make_unique<juce::AudioParameterFloat>("progEQLowFreq","Prog EQ Low Freq",
        juce::NormalisableRange<float>(50.f,700.f,1.f,0.4f),200.f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("progEQLowGain","Prog EQ Low Gain",
        juce::NormalisableRange<float>(-15.f,15.f,0.1f),0.f, juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        [](float v,int){ return juce::String(v,1)+" dB"; },
        [](const juce::String& t){ return t.dropLastCharacters(3).getFloatValue(); }));
    layout.add(std::make_unique<juce::AudioParameterFloat>("progEQMidFreq","Prog EQ Mid Freq",
        juce::NormalisableRange<float>(350.f,5000.f,1.f,0.4f),1000.f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("progEQMidGain","Prog EQ Mid Gain",
        juce::NormalisableRange<float>(-15.f,15.f,0.1f),0.f, juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        [](float v,int){ return juce::String(v,1)+" dB"; },
        [](const juce::String& t){ return t.dropLastCharacters(3).getFloatValue(); }));
    layout.add(std::make_unique<juce::AudioParameterFloat>("progEQHighFreq","Prog EQ High Freq",
        juce::NormalisableRange<float>(2000.f,16000.f,1.f,0.4f),8000.f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("progEQHighGain","Prog EQ High Gain",
        juce::NormalisableRange<float>(-15.f,15.f,0.1f),0.f, juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        [](float v,int){ return juce::String(v,1)+" dB"; },
        [](const juce::String& t){ return t.dropLastCharacters(3).getFloatValue(); }));

    // Routing
    layout.add(std::make_unique<juce::AudioParameterFloat>("inputLevel","Input Level",
        juce::NormalisableRange<float>(-24.f,24.f,0.1f),0.f, juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        [](float v,int){ return juce::String(v,1)+" dB"; },
        [](const juce::String& t){ return t.dropLastCharacters(3).getFloatValue(); }));
    layout.add(std::make_unique<juce::AudioParameterFloat>("outputLevel","Output Level",
        juce::NormalisableRange<float>(-24.f,24.f,0.1f),0.f, juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        [](float v,int){ return juce::String(v,1)+" dB"; },
        [](const juce::String& t){ return t.dropLastCharacters(3).getFloatValue(); }));
    layout.add(std::make_unique<juce::AudioParameterFloat>("mix","Mix",
        juce::NormalisableRange<float>(0.f,100.f,0.1f),50.f, juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        [](float v,int){ return juce::String(v,1)+"%"; },
        [](const juce::String& t){ return t.dropLastCharacters(1).getFloatValue(); }));

    // Switches
    layout.add(std::make_unique<juce::AudioParameterBool>("monoStereo",  "Stereo",    true));
    layout.add(std::make_unique<juce::AudioParameterBool>("bypass",      "Bypass",    false));
    layout.add(std::make_unique<juce::AudioParameterBool>("mute",        "Mute",      false));
    layout.add(std::make_unique<juce::AudioParameterBool>("eqOn",        "EQ On",     true));
    layout.add(std::make_unique<juce::AudioParameterBool>("phaseInvert", "Phase Inv", false));

    return layout;
}

// ================================================================
//  CONSTRUCTOR
// ================================================================
YamahaREV5AudioProcessor::YamahaREV5AudioProcessor()
    : AudioProcessor(BusesProperties()
        .withInput ("Input",  juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
{
    initPresets();
}

// ================================================================
//  PRESETS
// ================================================================
void YamahaREV5AudioProcessor::initPresets()
{
    //                  name               rT    hfR   lfR   diff  erB    erD    lpf     sRT   sD     sL    mR    mD    pLg  pMg  pHg  prL  prM  prH   mix
    presets =
    {
    { "LARGE HALL 1",  2.5f, 0.40f, 1.20f, 8.0f,  10.f, 20.f, 6800.f, 1.5f, 100.f, 0.30f, 0.30f, 0.20f, 0,0,0, 0,0,0, 50.f },
    { "LARGE HALL 2",  3.2f, 0.38f, 1.30f, 8.5f,  12.f, 25.f, 6200.f, 2.0f, 120.f, 0.35f, 0.25f, 0.22f, 0,0,0, 0,0,0, 52.f },
    { "SMALL HALL",    1.2f, 0.50f, 1.00f, 7.0f,   5.f, 15.f, 8000.f, 0.8f,  60.f, 0.20f, 0.40f, 0.15f, 0,0,0, 0,0,0, 45.f },
    { "MEDIUM HALL",   1.8f, 0.45f, 1.10f, 7.5f,   8.f, 18.f, 7200.f, 1.1f,  80.f, 0.25f, 0.35f, 0.17f, 0,0,0, 0,0,0, 47.f },
    { "CONCERT HALL",  4.5f, 0.35f, 1.40f, 9.0f,  15.f, 30.f, 5800.f, 2.5f, 150.f, 0.40f, 0.20f, 0.25f, 0,0,0, 0,0,0, 55.f },
    { "BRIGHT HALL",   2.2f, 0.70f, 1.00f, 7.5f,   8.f, 18.f,12000.f, 1.3f,  80.f, 0.30f, 0.40f, 0.18f, 0,0,0, 0,0,0, 48.f },
    { "ROOM 1",        0.8f, 0.55f, 0.90f, 6.0f,  -5.f,  8.f,10000.f, 0.5f,  30.f, 0.15f, 0.60f, 0.08f, 0,0,0, 0,0,0, 35.f },
    { "ROOM 2",        1.0f, 0.52f, 0.95f, 6.5f,  -3.f, 10.f, 9500.f, 0.6f,  35.f, 0.17f, 0.55f, 0.09f, 0,0,0, 0,0,0, 37.f },
    { "LARGE ROOM",    1.5f, 0.50f, 1.00f, 6.5f,   2.f, 12.f, 9000.f, 0.9f,  50.f, 0.20f, 0.50f, 0.10f, 0,0,0, 0,0,0, 38.f },
    { "STUDIO",        0.6f, 0.60f, 0.80f, 5.0f,  -8.f,  5.f,10000.f, 0.4f,  20.f, 0.10f, 0.80f, 0.05f, 0,0,0, 0,0,0, 30.f },
    { "EARLY REF 1",   0.5f, 0.80f, 0.70f, 4.0f, -20.f,  3.f,14000.f, 0.3f,  10.f, 0.05f, 1.00f, 0.02f, 0,0,0, 0,0,0, 40.f },
    { "EARLY REF 2",   0.7f, 0.75f, 0.75f, 4.5f, -15.f,  5.f,13000.f, 0.4f,  15.f, 0.08f, 0.90f, 0.03f, 0,0,0, 0,0,0, 38.f },
    { "PLATE",         1.8f, 0.60f, 0.80f, 9.0f,   0.f, 10.f, 9000.f, 1.0f,  40.f, 0.25f, 0.50f, 0.10f, 0,0,0, 0,0,0, 40.f },
    { "VOCAL PLATE",   2.0f, 0.45f, 1.10f, 7.5f,   8.f, 18.f, 8500.f, 1.2f,  70.f, 0.20f, 0.35f, 0.12f, 0,0,0, 0,0,0, 42.f },
    { "VOCAL HALL",    2.8f, 0.42f, 1.15f, 8.0f,  10.f, 22.f, 7500.f, 1.6f, 110.f, 0.30f, 0.28f, 0.20f, 0,0,0, 0,0,0, 48.f },
    { "AMBIENCE",      1.0f, 0.60f, 0.90f, 6.0f,  -3.f, 10.f,11000.f, 0.6f,  35.f, 0.12f, 0.60f, 0.06f, 0,0,0, 0,0,0, 25.f },
    { "GATED REV",     0.4f, 0.70f, 0.60f,10.0f,  15.f,  5.f,12000.f, 0.3f,  20.f, 0.10f, 1.00f, 0.05f, 0,0,0, 0,0,0, 55.f },
    { "REVERSE REV",   2.0f, 0.55f, 1.00f, 8.0f,   5.f, 15.f, 8000.f, 1.2f,  60.f, 0.25f, 0.45f, 0.15f, 0,0,0, 0,0,0, 50.f },
    { "TUNNEL",        3.5f, 0.40f, 1.30f, 8.5f,  15.f, 30.f, 6500.f, 2.0f, 120.f, 0.35f, 0.25f, 0.22f, 0,0,0, 0,0,0, 52.f },
    { "CANYON",        8.0f, 0.30f, 1.60f, 9.5f,  20.f, 40.f, 5000.f, 4.0f, 200.f, 0.50f, 0.15f, 0.30f, 0,0,0, 0,0,0, 60.f },
    { "CHURCH",        4.0f, 0.36f, 1.35f, 9.0f,  14.f, 28.f, 6000.f, 2.2f, 140.f, 0.38f, 0.22f, 0.24f, 0,0,0, 0,0,0, 54.f },
    { "DRUM ROOM",     0.9f, 0.65f, 0.85f, 7.0f,  -2.f,  8.f,11000.f, 0.6f,  28.f, 0.18f, 0.70f, 0.07f, 0,0,0, 0,0,-2.f, 36.f },
    { "SPRING SIM",    1.4f, 0.75f, 0.70f, 8.5f,  -5.f,  7.f,10000.f, 0.9f,  35.f, 0.20f, 0.90f, 0.10f, 0,2.f,0, 0,0,0, 45.f },
    { "CHORUS HALL",   2.3f, 0.48f, 1.10f, 8.0f,   9.f, 20.f, 7800.f, 1.4f,  90.f, 0.28f, 0.35f, 0.18f, 0,0,0, 0,0,0, 50.f },
    // ── User slots ───────────────────────────────────────────────
    { "USER 1", 2.0f,0.5f,1.0f,7.0f,0.f,15.f,8000.f,1.0f,60.f,0.2f,0.4f,0.15f,0,0,0,0,0,0,50.f },
    { "USER 2", 2.0f,0.5f,1.0f,7.0f,0.f,15.f,8000.f,1.0f,60.f,0.2f,0.4f,0.15f,0,0,0,0,0,0,50.f },
    { "USER 3", 2.0f,0.5f,1.0f,7.0f,0.f,15.f,8000.f,1.0f,60.f,0.2f,0.4f,0.15f,0,0,0,0,0,0,50.f },
    { "USER 4", 2.0f,0.5f,1.0f,7.0f,0.f,15.f,8000.f,1.0f,60.f,0.2f,0.4f,0.15f,0,0,0,0,0,0,50.f },
    { "USER 5", 2.0f,0.5f,1.0f,7.0f,0.f,15.f,8000.f,1.0f,60.f,0.2f,0.4f,0.15f,0,0,0,0,0,0,50.f },
    { "USER 6", 2.0f,0.5f,1.0f,7.0f,0.f,15.f,8000.f,1.0f,60.f,0.2f,0.4f,0.15f,0,0,0,0,0,0,50.f },
    { "USER 7", 2.0f,0.5f,1.0f,7.0f,0.f,15.f,8000.f,1.0f,60.f,0.2f,0.4f,0.15f,0,0,0,0,0,0,50.f },
    { "USER 8", 2.0f,0.5f,1.0f,7.0f,0.f,15.f,8000.f,1.0f,60.f,0.2f,0.4f,0.15f,0,0,0,0,0,0,50.f },
    };
}

void YamahaREV5AudioProcessor::saveToUserSlot(int slot)
{
    int idx = NUM_FACTORY + juce::jlimit(0, NUM_USER - 1, slot);
    auto& p = presets[idx];
    p.reverbTime    = *apvts.getRawParameterValue("reverbTime");
    p.hfRatio       = *apvts.getRawParameterValue("hfRatio");
    p.lfRatio       = *apvts.getRawParameterValue("lfRatio");
    p.diffusion     = *apvts.getRawParameterValue("diffusion");
    p.erRevBalance  = *apvts.getRawParameterValue("erRevBalance");
    p.erRevDelay    = *apvts.getRawParameterValue("erRevDelay");
    p.lpfCutoff     = *apvts.getRawParameterValue("lpfCutoff");
    p.secRevTime    = *apvts.getRawParameterValue("secRevTime");
    p.secDelay      = *apvts.getRawParameterValue("secDelay");
    p.secLevel      = *apvts.getRawParameterValue("secLevel");
    p.spaceModRate  = *apvts.getRawParameterValue("spaceModRate");
    p.spaceModDepth = *apvts.getRawParameterValue("spaceModDepth");
    p.mix           = *apvts.getRawParameterValue("mix");
}

// ================================================================
//  PROGRAM MANAGEMENT
// ================================================================
int  YamahaREV5AudioProcessor::getNumPrograms()    { return (int)presets.size(); }
int  YamahaREV5AudioProcessor::getCurrentProgram() { return currentProgram; }

const juce::String YamahaREV5AudioProcessor::getProgramName(int i)
{
    if(i >= 0 && i < (int)presets.size()) return presets[i].name;
    return "Unknown";
}

void YamahaREV5AudioProcessor::setCurrentProgram(int index)
{
    if(index < 0 || index >= (int)presets.size()) return;
    currentProgram = index;
    const auto& p = presets[index];

    auto set = [&](const juce::String& id, float val)
    {
        if(auto* param = apvts.getParameter(id))
            param->setValueNotifyingHost(param->convertTo0to1(val));
    };
    set("reverbTime",    p.reverbTime);
    set("hfRatio",       p.hfRatio);
    set("lfRatio",       p.lfRatio);
    set("diffusion",     p.diffusion);
    set("erRevBalance",  p.erRevBalance);
    set("erRevDelay",    p.erRevDelay);
    set("lpfCutoff",     p.lpfCutoff);
    set("secRevTime",    p.secRevTime);
    set("secDelay",      p.secDelay);
    set("secLevel",      p.secLevel);
    set("spaceModRate",  p.spaceModRate);
    set("spaceModDepth", p.spaceModDepth);
    set("preEQLowGain",  p.preEQLowGain);
    set("preEQMidGain",  p.preEQMidGain);
    set("preEQHighGain", p.preEQHighGain);
    set("progEQLowGain", p.progEQLowGain);
    set("progEQMidGain", p.progEQMidGain);
    set("progEQHighGain",p.progEQHighGain);
    set("mix",           p.mix);
}

// ================================================================
//  PREPARE TO PLAY
// ================================================================
void YamahaREV5AudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    // Pre-delay (max 250ms)
    const int maxPre = (int)(sampleRate * 0.25) + 8;
    preDelL.prepare(maxPre);
    preDelR.prepare(maxPre);

    // FDN lines — delay máximo ~63ms + margen
    for(int i = 0; i < FDN_N; ++i)
    {
        const int szL = (int)(FDN_MS_L[i] * 0.001 * sampleRate) + 32;
        const int szR = (int)(FDN_MS_R[i] * 0.001 * sampleRate) + 32;
        fdnL[i].prepare(szL);
        fdnR[i].prepare(szR);
    }
    fdnStateL.fill(0.f);
    fdnStateR.fill(0.f);

    // Allpass pre-diffuser
    for(int i = 0; i < NUM_AP; ++i)
    {
        const int sz = (int)(AP_MS[i] * 0.001 * sampleRate) + 16;
        apL[i].prepare(sz);
        apR[i].prepare(sz);
    }

    // Early reflections (max 150ms)
    const int erMax = (int)(0.15 * sampleRate) + 4;
    for(int i = 0; i < NUM_ER; ++i)
    {
        erL[i].prepare(erMax);
        erR[i].prepare(erMax);
    }

    // EQ
    preEQL.prepare(sampleRate);  preEQR.prepare(sampleRate);
    progEQL.prepare(sampleRate); progEQR.prepare(sampleRate);

    // Smoothed values
    smoothMix.reset        (sampleRate, 0.05);
    smoothInputGain.reset  (sampleRate, 0.05);
    smoothOutputGain.reset (sampleRate, 0.05);
    smoothRevTime.reset    (sampleRate, 0.20);
    smoothHFRatio.reset    (sampleRate, 0.10);
    smoothDiffusion.reset  (sampleRate, 0.05);
    smoothModDepth.reset   (sampleRate, 0.05);

    smoothMix.setCurrentAndTargetValue        (0.5f);
    smoothInputGain.setCurrentAndTargetValue  (1.0f);
    smoothOutputGain.setCurrentAndTargetValue (1.0f);

    lfoPhaseL = lfoPhaseR = 0.f;
    dcL = dcR = 0.f;

    juce::ignoreUnused(samplesPerBlock);
}

// ================================================================
//  PROCESS BLOCK
// ================================================================
void YamahaREV5AudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                              juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const bool isBypassed = *apvts.getRawParameterValue("bypass")      > 0.5f;
    const bool isMuted    = *apvts.getRawParameterValue("mute")        > 0.5f;
    const bool isStereo   = *apvts.getRawParameterValue("monoStereo")  > 0.5f;
    const bool eqEnabled  = *apvts.getRawParameterValue("eqOn")        > 0.5f;
    const bool phaseInv   = *apvts.getRawParameterValue("phaseInvert") > 0.5f;

    if(isBypassed) return;

    const int N  = buffer.getNumSamples();
    const int nC = buffer.getNumChannels();

    // ── Lectura de parámetros (fuera del loop) ───────────────────
    const float inputGainDb  = *apvts.getRawParameterValue("inputLevel");
    const float outputGainDb = *apvts.getRawParameterValue("outputLevel");
    const float mixPct       = *apvts.getRawParameterValue("mix");
    const float reverbTime   = *apvts.getRawParameterValue("reverbTime");
    const float hfRatio      = *apvts.getRawParameterValue("hfRatio");
    const float diffusion    = *apvts.getRawParameterValue("diffusion");
    const float erRevDelay   = *apvts.getRawParameterValue("erRevDelay");
    const float lpfCutoff    = *apvts.getRawParameterValue("lpfCutoff");
    const float secLevel     = *apvts.getRawParameterValue("secLevel");
    const float secDelayMs   = *apvts.getRawParameterValue("secDelay");
    const float modRate      = *apvts.getRawParameterValue("spaceModRate");
    const float modDepth     = *apvts.getRawParameterValue("spaceModDepth");
    const float erRevBalance = *apvts.getRawParameterValue("erRevBalance");

    smoothInputGain.setTargetValue  (juce::Decibels::decibelsToGain(inputGainDb));
    smoothOutputGain.setTargetValue (juce::Decibels::decibelsToGain(outputGainDb));
    smoothMix.setTargetValue        (mixPct / 100.f);
    smoothRevTime.setTargetValue    (reverbTime);
    smoothHFRatio.setTargetValue    (hfRatio);
    smoothDiffusion.setTargetValue  (diffusion);
    smoothModDepth.setTargetValue   (modDepth);

    // ── EQ update (una vez por bloque, no por muestra) ───────────
    if(eqEnabled)
    {
        const float pLF = *apvts.getRawParameterValue("preEQLowFreq");
        const float pLG = *apvts.getRawParameterValue("preEQLowGain");
        const float pMF = *apvts.getRawParameterValue("preEQMidFreq");
        const float pMG = *apvts.getRawParameterValue("preEQMidGain");
        const float pHF = *apvts.getRawParameterValue("preEQHighFreq");
        const float pHG = *apvts.getRawParameterValue("preEQHighGain");
        preEQL.setParams(pLF,pLG,pMF,pMG,pHF,pHG);
        preEQR.setParams(pLF,pLG,pMF,pMG,pHF,pHG);

        const float gLF = *apvts.getRawParameterValue("progEQLowFreq");
        const float gLG = *apvts.getRawParameterValue("progEQLowGain");
        const float gMF = *apvts.getRawParameterValue("progEQMidFreq");
        const float gMG = *apvts.getRawParameterValue("progEQMidGain");
        const float gHF = *apvts.getRawParameterValue("progEQHighFreq");
        const float gHG = *apvts.getRawParameterValue("progEQHighGain");
        progEQL.setParams(gLF,gLG,gMF,gMG,gHF,gHG);
        progEQR.setParams(gLF,gLG,gMF,gMG,gHF,gHG);
    }

    // ── LPF damp: frecuencia → coeficiente 1-polo (Julius O. Smith)
    // lpfDamp=0 → sin rolloff | lpfDamp→1 → rolloff total
    const float lpfOmega = juce::MathConstants<float>::twoPi
                           * lpfCutoff / (float)currentSampleRate;
    const float cosW  = std::cos(lpfOmega);
    const float xVal  = 2.f - cosW;
    const float lpfDamp = juce::jlimit(0.0f, 0.98f,
                              xVal - std::sqrt(xVal * xVal - 1.f));

    // ── ER/Rev balance — equal-power crossfade ───────────────────
    // t=0 → solo ER | t=0.5 → igual | t=1 → solo Rev
    const float t       = juce::jlimit(0.f, 1.f,
                              (erRevBalance + 63.f) / 126.f);
    const float erGain  = std::cos(t * juce::MathConstants<float>::halfPi);
    const float revGain = std::sin(t * juce::MathConstants<float>::halfPi);

    // ── Diffusion gain: 0–10 → allpass gain 0.20–0.72 ───────────
    const float diffGain = 0.20f + smoothDiffusion.getCurrentValue() / 10.f * 0.52f;

    // ── LFOs: L=seno, R=coseno (90° → decorrelación total) ───────
    const float lfoIncL = modRate / (float)currentSampleRate;
    const float lfoIncR = lfoIncL * 1.013f;   // frecuencia ligeramente distinta

    // ── Secondary delay en muestras ──────────────────────────────
    const int secDelaySmp = juce::jlimit(1, preDelL.size - 4,
                                (int)(secDelayMs * 0.001f * (float)currentSampleRate));

    float peakL    = 0.f, peakR    = 0.f;
    float outPeakL = 0.f, outPeakR = 0.f;

    // ================================================================
    //  LOOP POR MUESTRA
    // ================================================================
    for(int s = 0; s < N; ++s)
    {
        float inL = (nC > 0) ? buffer.getSample(0, s) : 0.f;
        float inR = (nC > 1 && isStereo) ? buffer.getSample(1, s) : inL;

        // Input gain (suavizado para evitar clicks al mover el knob)
        const float ig = smoothInputGain.getNextValue();
        inL *= ig;  inR *= ig;

        peakL = std::max(peakL, std::abs(inL));
        peakR = std::max(peakR, std::abs(inR));

        // Pre-EQ
        if(eqEnabled) { inL = preEQL.process(inL); inR = preEQR.process(inR); }

        // ── LFO flotante con interpolación ────────────────────────
        const float lfoL = std::sin(juce::MathConstants<float>::twoPi * lfoPhaseL);
        const float lfoR = std::cos(juce::MathConstants<float>::twoPi * lfoPhaseR);
        lfoPhaseL += lfoIncL; if(lfoPhaseL >= 1.f) lfoPhaseL -= 1.f;
        lfoPhaseR += lfoIncR; if(lfoPhaseR >= 1.f) lfoPhaseR -= 1.f;

        const float curDepth = smoothModDepth.getNextValue();
        const float modL = lfoL * curDepth * 8.f;   // ±8 muestras max
        const float modR = lfoR * curDepth * 8.f;

        // ── Pre-delay ─────────────────────────────────────────────
        const float preDelSmp = erRevDelay * 0.001f * (float)currentSampleRate;
        preDelL.write(inL); preDelR.write(inR);
        const float pdL = preDelL.read(preDelSmp);
        const float pdR = preDelR.read(preDelSmp);

        // ── Early Reflections (Moorer, 8 taps) ───────────────────
        float erOutL = 0.f, erOutR = 0.f;
        for(int i = 0; i < NUM_ER; ++i)
        {
            const float dL = juce::jlimit(1.f, (float)(erL[i].size - 2),
                               ER_L[i] * 0.001f * (float)currentSampleRate + modL * 0.3f);
            const float dR = juce::jlimit(1.f, (float)(erR[i].size - 2),
                               ER_R[i] * 0.001f * (float)currentSampleRate + modR * 0.3f);
            erL[i].write(pdL);  erR[i].write(pdR);
            const float decay = std::pow(0.68f, (float)(i + 1));
            erOutL += erL[i].read(dL) * decay;
            erOutR += erR[i].read(dR) * decay;
        }
        erOutL *= 0.20f;
        erOutR *= 0.20f;

        // ── Pre-diffuser: 4 allpass en serie (Schroeder) ──────────
        // Aumenta densidad de eco antes de entrar al FDN
        // L y R con tiempos ligeramente distintos → spread estéreo
        float diffL = pdL + erOutL * 0.15f;
        float diffR = pdR + erOutR * 0.15f;

        for(int i = 0; i < NUM_AP; ++i)
        {
            const int dL = juce::jlimit(1, apL[i].size - 2,
                               (int)(AP_MS[i] * 0.001 * currentSampleRate));
            const int dR = juce::jlimit(1, apR[i].size - 2,
                               (int)(AP_MS[i] * 0.001 * currentSampleRate * 1.017));
            diffL = apL[i].process(diffL, dL, diffGain);
            diffR = apR[i].process(diffR, dR, diffGain);
        }

        // ── FDN 8×8 con matriz Hadamard ───────────────────────────
        const float rt60 = smoothRevTime.getNextValue();
        const float hfR  = smoothHFRatio.getNextValue();

        // Paso 1: leer las 8 salidas con modulación + LPF
        std::array<float, FDN_N> outL, outR;
        for(int i = 0; i < FDN_N; ++i)
        {
            const float dL = juce::jlimit(1.f, (float)(fdnL[i].size - 2),
                               FDN_MS_L[i] * 0.001f * (float)currentSampleRate + modL);
            const float dR = juce::jlimit(1.f, (float)(fdnR[i].size - 2),
                               FDN_MS_R[i] * 0.001f * (float)currentSampleRate + modR);

            // hfRatio controla el LPF: 1.0=brillante, 0.1=oscuro
            const float damp = lpfDamp * (1.f - hfR * 0.4f);

            outL[i] = fdnL[i].tick(fdnStateL[i], dL, damp);
            outR[i] = fdnR[i].tick(fdnStateR[i], dR, damp);
        }

        // Paso 2: mezcla Hadamard — cada delay escucha a los otros 7
        // (lossless: conserva energía exactamente)
        std::array<float, FDN_N> mixedL = outL;
        std::array<float, FDN_N> mixedR = outR;
        hadamard8(mixedL);
        hadamard8(mixedR);

        // Paso 3: calcular feedback RT60 por delay y actualizar estado
        // g = 10^(-3 * delayTime / rt60) → decae exactamente rt60 segundos
        const float safeRT60 = juce::jmax(0.01f, rt60);
        for(int i = 0; i < FDN_N; ++i)
        {
            const float fbL = juce::jlimit(0.f, 0.975f,
                std::pow(10.f, -3.f * FDN_MS_L[i] * 0.001f / safeRT60));
            const float fbR = juce::jlimit(0.f, 0.975f,
                std::pow(10.f, -3.f * FDN_MS_R[i] * 0.001f / safeRT60));

            fdnStateL[i] = diffL * 0.5f + mixedL[i] * fbL;
            fdnStateR[i] = diffR * 0.5f + mixedR[i] * fbR;
        }

        // Paso 4: sumar y normalizar salidas del FDN
        float fdnOutL = 0.f, fdnOutR = 0.f;
        for(int i = 0; i < FDN_N; ++i) { fdnOutL += outL[i]; fdnOutR += outR[i]; }
        fdnOutL /= (float)FDN_N;
        fdnOutR /= (float)FDN_N;

        // ── Secondary tail (densidad extra tardía) ────────────────
        // Lee el pre-delay retardado sin escribir → sin loops indeseados
        const float secL = preDelL.read((float)(secDelaySmp + 3)) * secLevel * 0.5f;
        const float secR = preDelR.read((float)(secDelaySmp + 7)) * secLevel * 0.5f;

        // ── Mezcla ER + FDN + Sec (equal-power balance) ──────────
        float wetL = erOutL * erGain + (fdnOutL + secL) * revGain;
        float wetR = erOutR * erGain + (fdnOutR + secR) * revGain;

        // Headroom -3dB preventivo
        wetL *= 0.70f;
        wetR *= 0.70f;

        // ── DC Blocker (Julius O. Smith, forma estándar) ──────────
        // Elimina offset DC acumulado en colas largas
        // dcL/dcR siguen la media DC lentamente (constante ~15 seg)
        dcL  += (wetL - dcL) * 0.0003f;
        wetL -= dcL;
        dcR  += (wetR - dcR) * 0.0003f;
        wetR -= dcR;

        // ── Program EQ sobre la señal wet ─────────────────────────
        if(eqEnabled) { wetL = progEQL.process(wetL); wetR = progEQR.process(wetR); }

        // ── Phase invert ──────────────────────────────────────────
        if(phaseInv) wetR = -wetR;

        // ── Dry / Wet mix ─────────────────────────────────────────
        const float wet = smoothMix.getNextValue();
        const float dry = 1.f - wet;
        float outL2 = isMuted ? 0.f : (wetL * wet + inL * dry);
        float outR2 = isMuted ? 0.f : (wetR * wet + inR * dry);

        // ── Output gain ───────────────────────────────────────────
        const float og = smoothOutputGain.getNextValue();
        outL2 *= og;
        outR2 *= og;

        outPeakL = std::max(outPeakL, std::abs(outL2));
        outPeakR = std::max(outPeakR, std::abs(outR2));

        if(nC > 0) buffer.setSample(0, s, outL2);
        if(nC > 1) buffer.setSample(1, s, outR2);
    }

    inputLevelL.store(peakL);     inputLevelR.store(peakR);
    outputLevelL.store(outPeakL); outputLevelR.store(outPeakR);
}

// ================================================================
//  STATE
// ================================================================
void YamahaREV5AudioProcessor::getStateInformation(juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    xml->setAttribute("currentProgram", currentProgram);
    copyXmlToBinary(*xml, dest);
}

void YamahaREV5AudioProcessor::setStateInformation(const void* data, int size)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, size));
    if(xml && xml->hasTagName(apvts.state.getType()))
    {
        currentProgram = xml->getIntAttribute("currentProgram", 0);
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
    }
}

// ================================================================
//  FACTORY
// ================================================================
juce::AudioProcessorEditor* YamahaREV5AudioProcessor::createEditor()
{
    return new YamahaREV5AudioProcessorEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new YamahaREV5AudioProcessor();
}
