#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <vector>
#include <cmath>

// ================================================================
//  DELAY LINE con interpolación lineal
// ================================================================
struct DelayLine
{
    void prepare(int maxSamples)
    {
        buffer.assign(maxSamples + 4, 0.f);
        size = (int)buffer.size();
        writePos = 0;
    }

    void write(float x)
    {
        buffer[writePos] = x;
        writePos = (writePos + 1) % size;
    }

    // Lectura con interpolación lineal — sin clicks con LFO
    float read(float delaySamples) const
    {
        float d   = juce::jlimit(0.f, (float)(size - 2), delaySamples);
        int   i0  = (int)d;
        float f   = d - (float)i0;
        int   r0  = (writePos - i0     + size) % size;
        int   r1  = (writePos - i0 - 1 + size) % size;
        return buffer[r0] * (1.f - f) + buffer[r1] * f;
    }

    // Write + read en un paso (para pre-delay simple)
    float process(float input, int delaySamples)
    {
        int d    = juce::jlimit(1, size - 2, delaySamples);
        int rPos = (writePos - d + size) % size;
        float out = buffer[rPos];
        buffer[writePos] = input;
        writePos = (writePos + 1) % size;
        return out;
    }

    std::vector<float> buffer;
    int size = 0, writePos = 0;
};

// ================================================================
//  ALLPASS FILTER — Schroeder (difusión)
// ================================================================
struct AllpassFilter
{
    void prepare(int maxDelaySamples)
    {
        buffer.assign(maxDelaySamples + 4, 0.f);
        size = (int)buffer.size();
        writePos = 0;
    }

    float process(float input, int delaySamples, float gain)
    {
        int d    = juce::jlimit(1, size - 2, delaySamples);
        int rPos = (writePos - d + size) % size;
        float delayed = buffer[rPos];
        float v = input + gain * delayed;
        buffer[writePos] = v;
        writePos = (writePos + 1) % size;
        return delayed - gain * v;   // forma Schroeder correcta
    }

    std::vector<float> buffer;
    int size = 0, writePos = 0;
};

// ================================================================
//  FDN LINE — delay con LPF 1-polo y modulación (núcleo del FDN)
// ================================================================
struct FDNLine
{
    void prepare(int maxDelaySamples)
    {
        buffer.assign(maxDelaySamples + 4, 0.f);
        size = (int)buffer.size();
        writePos = 0;
        lpfState = 0.f;
    }

    // Escribe 'input', lee con interpolación y aplica LPF
    float tick(float input, float delaySamples, float lpfDamp)
    {
        // Lectura con interpolación lineal (modulación sin clicks)
        float d    = juce::jlimit(1.f, (float)(size - 2), delaySamples);
        int   i0   = (int)d;
        float frac = d - (float)i0;
        int   r0   = (writePos - i0     + size) % size;
        int   r1   = (writePos - i0 - 1 + size) % size;
        float out  = buffer[r0] * (1.f - frac) + buffer[r1] * frac;

        // LPF 1-polo: absorción de altos por vuelta (simula aire + materiales)
        lpfState = out * (1.f - lpfDamp) + lpfState * lpfDamp;

        buffer[writePos] = input;
        writePos = (writePos + 1) % size;

        return lpfState;
    }

    std::vector<float> buffer;
    int   size = 0, writePos = 0;
    float lpfState = 0.f;
};

// ================================================================
//  BIQUAD EQ 3 BANDAS
// ================================================================
struct ThreeBandEQ
{
    void prepare(double sampleRate) { sr = sampleRate; updateCoeffs(); }

    void setParams(float lowFreq,  float lowGainDb,
                   float midFreq,  float midGainDb,
                   float highFreq, float highGainDb)
    {
        lf = lowFreq;  lg = lowGainDb;
        mf = midFreq;  mg = midGainDb;
        hf = highFreq; hg = highGainDb;
        updateCoeffs();
    }

    float process(float x)
    {
        x = processBiquad(x, lowCoeffs,  lowState);
        x = processBiquad(x, midCoeffs,  midState);
        x = processBiquad(x, highCoeffs, highState);
        return x;
    }

private:
    double sr = 44100.0;
    float  lf = 200.f, lg = 0.f;
    float  mf = 1000.f, mg = 0.f;
    float  hf = 8000.f, hg = 0.f;

    struct Biquad { float b0, b1, b2, a1, a2; };
    Biquad lowCoeffs{}, midCoeffs{}, highCoeffs{};
    std::array<float,2> lowState{}, midState{}, highState{};

    static float processBiquad(float x, const Biquad& c, std::array<float,2>& s)
    {
        float y = c.b0 * x + s[0];
        s[0]    = c.b1 * x - c.a1 * y + s[1];
        s[1]    = c.b2 * x - c.a2 * y;
        return y;
    }

    Biquad makePeakEQ(float freq, float gainDb, float Q = 1.0f)
    {
        float A  = std::pow(10.f, gainDb / 40.f);
        float w0 = juce::MathConstants<float>::twoPi * freq / (float)sr;
        float cs = std::cos(w0), sn = std::sin(w0);
        float al = sn / (2.f * Q);
        float b0 = 1.f + al * A,  b1 = -2.f * cs, b2 = 1.f - al * A;
        float a0 = 1.f + al / A,  a1 = -2.f * cs, a2 = 1.f - al / A;
        return { b0/a0, b1/a0, b2/a0, a1/a0, a2/a0 };
    }

    Biquad makeLowShelf(float freq, float gainDb)
    {
        float A  = std::pow(10.f, gainDb / 40.f);
        float w0 = juce::MathConstants<float>::twoPi * freq / (float)sr;
        float cs = std::cos(w0), sn = std::sin(w0);
        float al = sn / 2.f * std::sqrt((A + 1.f/A) * (1.f/1.f - 1.f) + 2.f);
        float b0 =  A*((A+1)-(A-1)*cs + 2*std::sqrt(A)*al);
        float b1 =  2*A*((A-1)-(A+1)*cs);
        float b2 =  A*((A+1)-(A-1)*cs - 2*std::sqrt(A)*al);
        float a0 =    (A+1)+(A-1)*cs + 2*std::sqrt(A)*al;
        float a1 = -2*((A-1)+(A+1)*cs);
        float a2 =    (A+1)+(A-1)*cs - 2*std::sqrt(A)*al;
        return { b0/a0, b1/a0, b2/a0, a1/a0, a2/a0 };
    }

    Biquad makeHighShelf(float freq, float gainDb)
    {
        float A  = std::pow(10.f, gainDb / 40.f);
        float w0 = juce::MathConstants<float>::twoPi * freq / (float)sr;
        float cs = std::cos(w0), sn = std::sin(w0);
        float al = sn / 2.f * std::sqrt((A + 1.f/A) * (1.f/1.f - 1.f) + 2.f);
        float b0 =  A*((A+1)+(A-1)*cs + 2*std::sqrt(A)*al);
        float b1 = -2*A*((A-1)+(A+1)*cs);
        float b2 =  A*((A+1)+(A-1)*cs - 2*std::sqrt(A)*al);
        float a0 =    (A+1)-(A-1)*cs + 2*std::sqrt(A)*al;
        float a1 =  2*((A-1)-(A+1)*cs);
        float a2 =    (A+1)-(A-1)*cs - 2*std::sqrt(A)*al;
        return { b0/a0, b1/a0, b2/a0, a1/a0, a2/a0 };
    }

    void updateCoeffs()
    {
        lowCoeffs  = makeLowShelf (lf, lg);
        midCoeffs  = makePeakEQ   (mf, mg);
        highCoeffs = makeHighShelf(hf, hg);
    }
};

// ================================================================
//  PROCESSOR PRINCIPAL
// ================================================================
class YamahaREV5AudioProcessor : public juce::AudioProcessor
{
public:
    YamahaREV5AudioProcessor();
    ~YamahaREV5AudioProcessor() override = default;

    void prepareToPlay   (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock    (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool        hasEditor()       const override { return true; }
    const juce::String getName()  const override { return "YamahaREV5"; }
    bool        acceptsMidi()     const override { return false; }
    bool        producesMidi()    const override { return false; }
    double      getTailLengthSeconds() const override { return 8.0; }

    int  getNumPrograms()              override;
    int  getCurrentProgram()           override;
    void setCurrentProgram(int index)  override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock&)    override;
    void setStateInformation(const void*, int)      override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    juce::AudioProcessorValueTreeState apvts;

    std::atomic<float> inputLevelL  { 0.f };
    std::atomic<float> inputLevelR  { 0.f };
    std::atomic<float> outputLevelL { 0.f };
    std::atomic<float> outputLevelR { 0.f };

    static constexpr int NUM_FACTORY    = 24;
    static constexpr int NUM_USER       = 8;
    static constexpr int TOTAL_PROGRAMS = NUM_FACTORY + NUM_USER;

    void saveToUserSlot(int slot);

private:
    // ── FDN 8 líneas (late reverb) ───────────────────────────────
    // Delays primos en ms. Declarados en el .cpp (namespace anónimo)
    // para evitar problemas de linkage con constexpr float arrays.
    static constexpr int FDN_N  = 8;
    static constexpr int NUM_AP = 4;
    static constexpr int NUM_ER = 8;

    std::array<FDNLine,       FDN_N>  fdnL, fdnR;
    std::array<float,         FDN_N>  fdnStateL{}, fdnStateR{};

    std::array<AllpassFilter, NUM_AP> apL, apR;
    std::array<DelayLine,     NUM_ER> erL, erR;

    DelayLine preDelL, preDelR;

    // LFOs separados por canal (L=seno, R=coseno → 90° desfase)
    float lfoPhaseL = 0.f, lfoPhaseR = 0.f;

    ThreeBandEQ preEQL, preEQR, progEQL, progEQR;

    // DC blocker: un estado por canal
    float dcL = 0.f, dcR = 0.f;

    juce::SmoothedValue<float> smoothMix, smoothInputGain, smoothOutputGain;
    juce::SmoothedValue<float> smoothRevTime, smoothHFRatio;
    juce::SmoothedValue<float> smoothDiffusion, smoothModDepth;

    double currentSampleRate = 44100.0;

    // ── Presets ──────────────────────────────────────────────────
    int currentProgram = 0;

    struct Preset {
        juce::String name;
        float reverbTime, hfRatio, lfRatio, diffusion;
        float erRevBalance, erRevDelay, lpfCutoff;
        float secRevTime, secDelay, secLevel;
        float spaceModRate, spaceModDepth;
        float preEQLowGain, preEQMidGain, preEQHighGain;
        float progEQLowGain, progEQMidGain, progEQHighGain;
        float mix;
    };

    std::vector<Preset> presets;
    void initPresets();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(YamahaREV5AudioProcessor)
};
