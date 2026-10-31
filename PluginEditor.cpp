#include "PluginEditor.h"

// ================================================================
//  COLORES
// ================================================================
const juce::Colour REV5LookAndFeel::colPanel         = juce::Colour(0xff181818);
const juce::Colour REV5LookAndFeel::colKnob          = juce::Colour(0xff2a2a2a);
const juce::Colour REV5LookAndFeel::colKnobLine      = juce::Colour(0xffdddddd);
const juce::Colour REV5LookAndFeel::colDisplay       = juce::Colour(0xffdcdcaa);
const juce::Colour REV5LookAndFeel::colButtonFace    = juce::Colour(0xff323232);
const juce::Colour REV5LookAndFeel::colButtonText    = juce::Colour(0xffdddddd);
const juce::Colour REV5LookAndFeel::colLedRed        = juce::Colour(0xffff3333);
const juce::Colour REV5LookAndFeel::colLedGreen      = juce::Colour(0xff44ff44);
const juce::Colour REV5LookAndFeel::colButtonTop     = juce::Colour(0xff484848);
const juce::Colour REV5LookAndFeel::colButtonBot     = juce::Colour(0xff1a1a1a);
const juce::Colour REV5LookAndFeel::colButtonPressed = juce::Colour(0xff181818);

// ================================================================
//  LOOK AND FEEL
// ================================================================
REV5LookAndFeel::REV5LookAndFeel()
{
    setColour(juce::Slider::thumbColourId,       colKnob);
    setColour(juce::TextButton::textColourOffId, colButtonText);
    setColour(juce::TextButton::textColourOnId,  colLedRed);
}

void REV5LookAndFeel::drawRotarySlider(juce::Graphics& g,
    int x, int y, int width, int height, float sliderPos,
    float rotaryStartAngle, float rotaryEndAngle, juce::Slider&)
{
    auto bounds = juce::Rectangle<float>(x,y,width,height).reduced(3);
    auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.f;
    auto cx = bounds.getCentreX(), cy = bounds.getCentreY();

    g.setColour(juce::Colours::black.withAlpha(0.6f));
    g.fillEllipse(cx-radius-1, cy-radius+2, (radius+1)*2, (radius+1)*2);

    juce::ColourGradient grad(colButtonTop, cx-radius*0.3f, cy-radius*0.3f,
                               colButtonBot, cx+radius*0.4f, cy+radius*0.6f, true);
    g.setGradientFill(grad);
    g.fillEllipse(cx-radius, cy-radius, radius*2, radius*2);
    g.setColour(juce::Colours::white.withAlpha(0.18f));
    g.drawEllipse(cx-radius+0.5f, cy-radius+0.5f, radius*2-1.f, radius*2-1.f, 1.2f);

    float angle = rotaryStartAngle + sliderPos*(rotaryEndAngle-rotaryStartAngle);
    float ll = radius*0.65f, dr = radius*0.12f;
    float ex = cx + std::cos(angle - juce::MathConstants<float>::halfPi)*ll;
    float ey = cy + std::sin(angle - juce::MathConstants<float>::halfPi)*ll;
    g.setColour(colKnobLine);
    g.drawLine(cx,cy,ex,ey,2.f);
    g.fillEllipse(ex-dr,ey-dr,dr*2,dr*2);
}

void REV5LookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& btn,
    const juce::Colour&, bool highlighted, bool down)
{
    auto  bounds    = btn.getLocalBounds().toFloat().reduced(1.f);
    float corner    = 3.f;
    bool  isToggled = btn.getToggleState() && btn.getClickingTogglesState();
    bool  isLedOnly = btn.getButtonText().isEmpty();
    bool  pressed   = down || isToggled;

    if (pressed)
    {
        g.setColour(colButtonPressed);
        g.fillRoundedRectangle(bounds, corner);
        g.setColour(juce::Colours::black.withAlpha(0.85f));
        g.drawLine(bounds.getX(), bounds.getY(), bounds.getRight(), bounds.getY(), 1.5f);
        g.drawLine(bounds.getX(), bounds.getY(), bounds.getX(), bounds.getBottom(), 1.5f);
        g.setColour(juce::Colours::white.withAlpha(0.1f));
        g.drawLine(bounds.getX(), bounds.getBottom(), bounds.getRight(), bounds.getBottom(), 1.f);
        g.drawLine(bounds.getRight(), bounds.getY(), bounds.getRight(), bounds.getBottom(), 1.f);
    }
    else
    {
        juce::ColourGradient grad(
            highlighted ? colButtonTop.brighter(0.15f) : colButtonTop,
            bounds.getX(), bounds.getY(),
            colButtonBot, bounds.getX(), bounds.getBottom(), false);
        g.setGradientFill(grad);
        g.fillRoundedRectangle(bounds, corner);
        g.setColour(juce::Colours::white.withAlpha(0.25f));
        g.drawLine(bounds.getX()+corner, bounds.getY(), bounds.getRight()-corner, bounds.getY(), 1.2f);
        g.drawLine(bounds.getX(), bounds.getY()+corner, bounds.getX(), bounds.getBottom()-corner, 1.2f);
        g.setColour(juce::Colours::black.withAlpha(0.6f));
        g.drawLine(bounds.getX()+corner, bounds.getBottom(), bounds.getRight()-corner, bounds.getBottom(), 1.2f);
        g.drawLine(bounds.getRight(), bounds.getY()+corner, bounds.getRight(), bounds.getBottom()-corner, 1.2f);
        g.setColour(juce::Colours::black.withAlpha(0.35f));
        g.drawRoundedRectangle(bounds, corner, 0.8f);
    }

    if (isLedOnly)
    {
        float ls = 7.f;
        juce::Rectangle<float> lr(bounds.getCentreX()-ls/2, bounds.getCentreY()-ls/2, ls, ls);
        if (isToggled) { g.setColour(colLedGreen.withAlpha(0.3f)); g.fillEllipse(lr.expanded(2.f)); }
        g.setColour(isToggled ? colLedGreen : colLedRed.darker(0.5f));
        g.fillEllipse(lr);
        g.setColour(juce::Colours::white.withAlpha(0.4f));
        g.fillEllipse(lr.reduced(ls*0.3f).translated(-1.f,-1.f));
    }

    if (!isLedOnly && btn.getClickingTogglesState())
    {
        juce::Rectangle<float> ledBar(bounds.getX()+6, bounds.getY()+3,
                                       bounds.getWidth()-12, 3.f);
        if (isToggled)
        {
            g.setColour(colLedRed.withAlpha(0.95f));
            g.fillRoundedRectangle(ledBar, 1.f);
            g.setColour(juce::Colours::white.withAlpha(0.5f));
            g.fillRoundedRectangle(ledBar.reduced(1.5f, 0.5f), 1.f);
        }
        else
        {
            g.setColour(colLedRed.darker(0.7f));
            g.fillRoundedRectangle(ledBar, 1.f);
        }
    }
}

void REV5LookAndFeel::drawButtonText(juce::Graphics& g, juce::TextButton& btn,
    bool, bool down)
{
    if (btn.getButtonText().isEmpty()) return;
    g.setFont(getTextButtonFont(btn, btn.getHeight()));
    auto tb = btn.getLocalBounds().reduced(2).translated(down?1:0, down?1:0);
    if (!down) {
        g.setColour(juce::Colours::black.withAlpha(0.5f));
        g.drawFittedText(btn.getButtonText(), tb.translated(0,1), juce::Justification::centred, 1);
    }
    bool lit = btn.getToggleState() && btn.getClickingTogglesState();
    g.setColour(lit ? colLedRed.brighter(0.2f) : colButtonText);
    g.drawFittedText(btn.getButtonText(), tb, juce::Justification::centred, 1);
}

juce::Font REV5LookAndFeel::getTextButtonFont(juce::TextButton& btn, int height)
{
    const juce::String t = btn.getButtonText();
    float size = juce::jlimit(7.f, 11.f, height * 0.38f);
    if (t=="PARAMETER"||t=="INITIAL"||t=="RECALL"||t=="MEMORY"||t=="OTHERS"||t=="UTILITY")
        size = juce::jlimit(6.f, 9.f, height * 0.30f);
    return juce::Font(juce::FontOptions("Arial", size, juce::Font::bold));
}

// ================================================================
//  VUMETER
// ================================================================
void REV5Meter::paint(juce::Graphics& g)
{
    const int n = 6;
    const float lh = (float)getHeight()/n, pad = 2.f;
    const float dL = juce::jlimit(0.f,1.f, level*2.5f);
    for (int i = 0; i < n; ++i)
    {
        float yPos = i*lh+pad, h = lh-pad*2;
        juce::Colour c;
        if      (i==0)       c = juce::Colour(0xffff2222);
        else if (i<=2)       c = juce::Colour(0xffffff22);
        else                 c = juce::Colour(0xff22ff22);
        float thr = 1.f - i/(float)(n-1);
        bool  on  = (dL >= thr);
        g.setColour(on ? c : c.darker(0.85f));
        g.fillRect(juce::Rectangle<float>(0,yPos,(float)getWidth(),h));
        if (on) {
            g.setColour(juce::Colours::white.withAlpha(0.25f));
            g.fillRect(juce::Rectangle<float>(0,yPos,(float)getWidth(),h*0.35f));
        }
    }
}

// ================================================================
//  REV5DISPLAY — constructor + timer
// ================================================================
REV5Display::REV5Display()
{
    heatA.fill(0.f);
    heatB.fill(0.f);
    startTimerHz(24);
}

void REV5Display::setAudioLevel(float lvl)
{
    audioLevel = juce::jlimit(0.f, 1.f, lvl);
}

void REV5Display::timerCallback()
{
    stepFlame();
    repaint();
}

// ── Paleta: mapeamos calor (0-1) a colores oscuros sobre fondo LCD ─
juce::Colour REV5Display::flamePixelColor(float h) const
{
    // fondo LCD: #d4d49a   pixel frío: #606050   pixel caliente: #1a1a00
    if (h < 0.05f) return juce::Colour(0xffd4d49a);  // transparente/fondo
    if (h < 0.35f)
    {
        // Sombra muy suave — penumbra de la llama
        float t = h / 0.35f;
        return juce::Colour(0xffd4d49a).interpolatedWith(juce::Colour(0xff888860), t);
    }
    if (h < 0.70f)
    {
        float t = (h - 0.35f) / 0.35f;
        return juce::Colour(0xff888860).interpolatedWith(juce::Colour(0xff3a3a10), t);
    }
    // Núcleo de la llama: negro-verdoso máximo
    float t = (h - 0.70f) / 0.30f;
    return juce::Colour(0xff3a3a10).interpolatedWith(juce::Colour(0xff1a1a00), t);
}

// ── Simulación de fuego tipo demoscene ────────────────────────────
void REV5Display::stepFlame()
{
    // Suavizado exponencial del nivel de audio
    smoothLevel += (audioLevel - smoothLevel) * 0.20f;

    auto& rng = juce::Random::getSystemRandom();

    // 1) Inyectar calor en la fila base (FR-1) según nivel de audio
    //    Llama piloto mínima siempre presente (0.15) + audio
    float baseFuel = 0.15f + smoothLevel * 0.85f;

    for (int c = 0; c < FC; ++c)
    {
        float distCenter = std::abs(c - (FC-1) * 0.5f) / ((FC-1) * 0.5f);
        float shape      = 1.f - distCenter * 0.45f;  // más intenso en el centro
        float noise      = 0.75f + rng.nextFloat() * 0.25f;
        heatA[(FR-1)*FC + c] = juce::jlimit(0.f, 1.f, baseFuel * shape * noise);
    }

    // 2) Propagar hacia arriba (difusión + enfriamiento)
    for (int r = 0; r < FR-1; ++r)
    {
        for (int c = 0; c < FC; ++c)
        {
            // Promedio ponderado de tres vecinos de abajo
            float left   = (c > 0)    ? heatA[(r+1)*FC + c-1] : heatA[(r+1)*FC + c];
            float center =              heatA[(r+1)*FC + c];
            float right  = (c < FC-1) ? heatA[(r+1)*FC + c+1] : heatA[(r+1)*FC + c];

            // Peso central mayor → llama sube más vertical
            float avg = (left*0.25f + center*0.5f + right*0.25f);

            // Cooling: más fuerte en la cima, casi nulo en la base
            float rowNorm = 1.f - (float)r / (float)(FR-2);   // 1=cima, 0=base
            float cooling = 0.04f + rowNorm * 0.10f;

            // Deriva lateral aleatoria pequeña (turbulencia)
            float drift = (rng.nextFloat() - 0.5f) * 0.04f * (0.4f + smoothLevel * 0.6f);

            heatB[r*FC + c] = juce::jlimit(0.f, 1.f, avg - cooling + drift);
        }
    }

    // 3) Copiar fila base (no se modifica en la propagación)
    for (int c = 0; c < FC; ++c)
        heatB[(FR-1)*FC + c] = heatA[(FR-1)*FC + c];

    // 4) Ping-pong
    heatA = heatB;
}

// ── 7-SEGMENTOS ───────────────────────────────────────────────────
void REV5Display::draw7Segment(juce::Graphics& g, int num,
                                int x, int y, int w, int h)
{
    bool seg[10][7] = {
        {1,1,1,1,1,1,0},{0,1,1,0,0,0,0},{1,1,0,1,1,0,1},{1,1,1,1,0,0,1},
        {0,1,1,0,0,1,1},{1,0,1,1,0,1,1},{1,0,1,1,1,1,1},{1,1,1,0,0,0,0},
        {1,1,1,1,1,1,1},{1,1,1,1,0,1,1}
    };
    if (num < 0 || num > 9) return;
    auto active = REV5LookAndFeel::colLedRed, inactive = juce::Colour(0xff3a0000);
    int t=4, gp=2;
    auto sr = [&](bool on, int rx, int ry, int rw, int rh)
    {
        g.setColour(on ? active : inactive);
        g.fillRect(rx,ry,rw,rh);
        if (on) { g.setColour(juce::Colours::white.withAlpha(0.28f)); g.fillRect(rx,ry,rw,rh/3); }
    };
    sr(seg[num][0], x+t+gp, y,        w-t*2-gp*2, t);
    sr(seg[num][1], x+w-t,  y+t+gp,   t, h/2-t-gp*2);
    sr(seg[num][2], x+w-t,  y+h/2+gp, t, h/2-t-gp*2);
    sr(seg[num][3], x+t+gp, y+h-t,    w-t*2-gp*2, t);
    sr(seg[num][4], x,      y+h/2+gp, t, h/2-t-gp*2);
    sr(seg[num][5], x,      y+t+gp,   t, h/2-t-gp*2);
    sr(seg[num][6], x+t+gp, y+h/2-t/2,w-t*2-gp*2, t);
}

void REV5Display::drawTwoDigit(juce::Graphics& g, int number,
                                int x, int y, int dW, int dH)
{
    number = juce::jlimit(0,99,number);
    int tens=number/10, ones=number%10, gap=6;

    auto drawOff = [&](int px, int py)
    {
        int t=4,gp=2,w=dW-4,h=dH-4;
        g.setColour(juce::Colour(0xff2a0000));
        g.fillRect(px+t+gp,py,w-t*2-gp*2,t);
        g.fillRect(px,py+t+gp,t,h/2-t-gp*2);
        g.fillRect(px+w-t,py+t+gp,t,h/2-t-gp*2);
        g.fillRect(px+t+gp,py+h/2-t/2,w-t*2-gp*2,t);
        g.fillRect(px,py+h/2+gp,t,h/2-t-gp*2);
        g.fillRect(px+w-t,py+h/2+gp,t,h/2-t-gp*2);
        g.fillRect(px+t+gp,py+h-t,w-t*2-gp*2,t);
    };

    g.setColour(juce::Colours::black); g.fillRect(x,y,dW,dH);
    if (tens > 0) draw7Segment(g, tens, x+2, y+2, dW-4, dH-4);
    else          drawOff(x+2, y+2);

    g.setColour(juce::Colours::black); g.fillRect(x+dW+gap, y, dW, dH);
    draw7Segment(g, ones, x+dW+gap+2, y+2, dW-4, dH-4);
}

void REV5Display::update(const juce::String& l1, const juce::String& l2, int memNum)
{
    line1=l1; line2=l2; memoryNum=memNum; repaint();
}

// ================================================================
//  REV5Display::paint — todo en un solo canvas
// ================================================================
void REV5Display::paint(juce::Graphics& g)
{
    const int W = getWidth(), H = getHeight();

    // ── 1) Marco exterior ─────────────────────────────────────────
    g.setColour(juce::Colour(0xff0a0a0a));
    g.fillRoundedRectangle(0.f, 0.f, (float)W, (float)H, 4.f);

    // ── 2) Fondo LCD completo ─────────────────────────────────────
    juce::Rectangle<float> lcd(5.f, 5.f, W-10.f, H-10.f);
    juce::ColourGradient lcdGrad(
        juce::Colour(0xffe8e8b0), lcd.getX(), lcd.getY(),
        juce::Colour(0xffd0d098), lcd.getX(), lcd.getBottom(), false);
    g.setGradientFill(lcdGrad);
    g.fillRoundedRectangle(lcd, 3.f);

    // Reflejo top
    g.setColour(juce::Colours::white.withAlpha(0.10f));
    g.fillRoundedRectangle(lcd.withHeight(lcd.getHeight()*0.22f), 3.f);

    // ── 3) Zona de la LLAMA (derecha del LCD) ─────────────────────
    //  La llama ocupa los últimos ~108px horizontales del LCD interior
    const int FLAME_W    = 108;
    const int FLAME_PAD  = 4;
    const int flameLeft  = W - FLAME_W - 5;    // X inicio zona llama (coords locales)
    const int flameTop   = 5 + FLAME_PAD;
    const int flameRight = W - 5 - FLAME_PAD;
    const int flameBotm  = H - 5 - FLAME_PAD;
    const int flameH     = flameBotm - flameTop;

    // Dibuja la grilla de píxeles de la llama
    float cellW = (float)(flameRight - flameLeft) / FC;
    float cellH = (float)flameH / FR;

    for (int r = 0; r < FR; ++r)
    {
        for (int c = 0; c < FC; ++c)
        {
            float hv = heatA[r * FC + c];
            g.setColour(flamePixelColor(hv));
            g.fillRect(juce::Rectangle<float>(
                flameLeft + c * cellW,
                flameTop  + r * cellH,
                cellW, cellH));
        }
    }

    // Borde sutil separando zona llama del texto
    g.setColour(juce::Colour(0xffa0a070).withAlpha(0.6f));
    g.drawVerticalLine(flameLeft - 2, (float)flameTop, (float)flameBotm);

    // ── 4) Caja 7-segmentos (arriba-izquierda) ────────────────────
    int boxX=8, boxY=6, boxW=80, boxH=60;
    g.setColour(juce::Colour(0xff050505));
    g.fillRoundedRectangle((float)boxX,(float)boxY,(float)boxW,(float)boxH,3.f);
    g.setColour(juce::Colours::white.withAlpha(0.04f));
    g.fillRoundedRectangle((float)boxX+1,(float)boxY+1,(float)boxW-2,(float)boxH*0.4f,2.f);

    int dW=28,dH=46,dGap=6;
    int sX=boxX+(boxW-(dW*2+dGap))/2;
    int sY=boxY+(boxH-dH)/2;
    drawTwoDigit(g, memoryNum, sX, sY, dW, dH);

    // ── 5) Texto del programa (zona central, entre 7-seg y llama) ─
    //  Área de texto: desde x=92 hasta flameLeft-8
    int textX   = 92;
    int textW   = flameLeft - 8 - textX;
    int textMidY= H / 2;

    g.setColour(juce::Colour(0xff1a1a00));
    g.setFont(juce::Font(juce::FontOptions("Courier New", 14.f, juce::Font::bold)));
    g.drawText(line1,
               juce::Rectangle<int>(textX, 8, textW, textMidY - 8),
               juce::Justification::centredLeft, true);

    g.setFont(juce::Font(juce::FontOptions("Courier New", 12.f, juce::Font::plain)));
    g.drawText(line2,
               juce::Rectangle<int>(textX, textMidY, textW, H - textMidY - 6),
               juce::Justification::centredLeft, true);
}

// ================================================================
//  CONSTRUCTOR EDITOR
// ================================================================
YamahaREV5AudioProcessorEditor::YamahaREV5AudioProcessorEditor
    (YamahaREV5AudioProcessor& p)
    : AudioProcessorEditor(&p), proc(p)
{
    setLookAndFeel(&laf);
    addAndMakeVisible(lcdDisplay);
    addAndMakeVisible(inMeter);

    setupToggleButton(btnMonoStereo);
    setupToggleButton(btnEqOn);
    setupToggleButton(btnMemory);
    setupToggleButton(btnMute);
    setupToggleButton(btnBypass);
    setupPushButton(btnPower);

    setupKnob(slInputLevel); setupKnob(slMix);
    setupKnob(slEqLoFreq);   setupKnob(slEqLoLevel);
    setupKnob(slEqMidFreq);  setupKnob(slEqMidLevel);
    setupKnob(slEqHiFreq);   setupKnob(slEqHiLevel);

    for (auto* b : {
            &btnRev1,&btnER1,&btnRev2,&btnER2,
            &btnRev3,&btnOthers,&btnRev4,&btnUser,
            &btnParam,&btnLevel,&btnInitDly,&btn1stRef,
            &btnEqDigital,&btnEqDigOn,&btnDown,&btnUp,
            &btn7,&btn8,&btn9,&btn4,&btn5,&btn6,
            &btn1,&btn2,&btn3,&btn0,&btnDot,&btnClear,
            &btnStore,&btnRecall,&btnMinus,&btnIntParam,&btnUtility })
    {
        setupPushButton(*b);
    }

    attInput      = std::make_unique<SliderAtt>(proc.apvts,"inputLevel",   slInputLevel);
    attMix        = std::make_unique<SliderAtt>(proc.apvts,"mix",          slMix);
    attEqLoF      = std::make_unique<SliderAtt>(proc.apvts,"preEQLowFreq", slEqLoFreq);
    attEqLoG      = std::make_unique<SliderAtt>(proc.apvts,"preEQLowGain", slEqLoLevel);
    attEqMidF     = std::make_unique<SliderAtt>(proc.apvts,"preEQMidFreq", slEqMidFreq);
    attEqMidG     = std::make_unique<SliderAtt>(proc.apvts,"preEQMidGain", slEqMidLevel);
    attEqHiF      = std::make_unique<SliderAtt>(proc.apvts,"preEQHighFreq",slEqHiFreq);
    attEqHiG      = std::make_unique<SliderAtt>(proc.apvts,"preEQHighGain",slEqHiLevel);
    attBypass     = std::make_unique<ButtonAtt>(proc.apvts,"bypass",       btnBypass);
    attMute       = std::make_unique<ButtonAtt>(proc.apvts,"mute",         btnMute);
    attEqOnToggle = std::make_unique<ButtonAtt>(proc.apvts,"eqOn",         btnEqOn);
    attStereo     = std::make_unique<ButtonAtt>(proc.apvts,"monoStereo",   btnMonoStereo);

    setSize(1230, 240);
    startTimerHz(30);
    updateLCDMenu();
}

YamahaREV5AudioProcessorEditor::~YamahaREV5AudioProcessorEditor()
{
    setLookAndFeel(nullptr);
}

void YamahaREV5AudioProcessorEditor::setupKnob(juce::Slider& s)
{
    s.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    s.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible(s);
}

void YamahaREV5AudioProcessorEditor::setupPushButton(juce::TextButton& b)
{
    b.setClickingTogglesState(false);
    b.addListener(this);
    addAndMakeVisible(b);
}

void YamahaREV5AudioProcessorEditor::setupToggleButton(juce::TextButton& b)
{
    b.setClickingTogglesState(true);
    b.addListener(this);
    addAndMakeVisible(b);
}

// ================================================================
//  PAINT
// ================================================================
void YamahaREV5AudioProcessorEditor::paint(juce::Graphics& g)
{
    juce::ColourGradient bgGrad(juce::Colour(0xff181818),0,0,
                                 juce::Colour(0xff111111),0,(float)getHeight(),false);
    g.setGradientFill(bgGrad); g.fillAll();

    auto pb = getLocalBounds().reduced(8).toFloat();
    juce::ColourGradient pGrad(juce::Colour(0xff252525),pb.getX(),pb.getY(),
                                juce::Colour(0xff141414),pb.getX(),pb.getBottom(),false);
    g.setGradientFill(pGrad);
    g.fillRoundedRectangle(pb,6.f);
    g.setColour(juce::Colours::white.withAlpha(0.08f));
    g.drawRoundedRectangle(pb,6.f,1.f);
    g.setColour(juce::Colours::black.withAlpha(0.5f));
    g.drawRoundedRectangle(pb.reduced(1.f),5.f,1.f);

    // Título
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(juce::FontOptions("Arial",26.f,juce::Font::bold)));
    g.drawText("LLAMAJA",22,22,140,32,juce::Justification::centredLeft);
    g.setColour(juce::Colour(0xffaaaaaa));
    g.setFont(juce::Font(juce::FontOptions("Arial",10.f,juce::Font::plain)));
    g.drawText("DIGITAL REVERBERATOR",22,50,170,16,juce::Justification::centredLeft);
    g.setFont(juce::Font(juce::FontOptions("Arial",28.f,juce::Font::italic)));
    g.setColour(juce::Colour(0xff888888));
    g.drawText("REV5",22,65,110,30,juce::Justification::centredLeft);

    // VUmeter labels
    g.setColour(juce::Colour(0xffaaaaaa));
    g.setFont(juce::Font(juce::FontOptions("Arial",7.5f,juce::Font::plain)));
    int vuX=197;
    g.drawText("CLIP",vuX,40,28,10,juce::Justification::centredLeft);
    g.drawText("-3",  vuX,50,28,10,juce::Justification::centredLeft);
    g.drawText("-6",  vuX,60,28,10,juce::Justification::centredLeft);
    g.drawText("-10", vuX,70,28,10,juce::Justification::centredLeft);
    g.drawText("-20", vuX,80,28,10,juce::Justification::centredLeft);
    g.drawText("-30", vuX,90,28,10,juce::Justification::centredLeft);

    // Serigrafía EQ
    auto cx=[](const juce::Slider& s){ return s.getX()+s.getWidth()/2; };
    int yTT=slEqLoFreq.getY()-30, yTL=slEqLoFreq.getY()-18;
    int yMT=slEqLoFreq.getY()-9,  yBT=slEqLoFreq.getBottom()+6;

    g.setColour(juce::Colour(0xffbbbbbb));
    g.setFont(juce::Font(juce::FontOptions("Arial",9.f,juce::Font::bold)));
    g.drawText("INPUT",  slInputLevel.getX(),    slInputLevel.getY()-24,50,10,juce::Justification::centred);
    g.drawText("LEVEL",  slInputLevel.getX(),    slInputLevel.getY()-14,50,10,juce::Justification::centred);
    g.drawText("MONO",   btnMonoStereo.getX()-5,  btnMonoStereo.getY()-20,52,10,juce::Justification::centred);
    g.drawText("STEREO", btnMonoStereo.getX()-5,  btnMonoStereo.getY()-11,52,10,juce::Justification::centred);
    g.drawText("EQ",     btnEqOn.getX(),           btnEqOn.getY()-20,42,10,juce::Justification::centred);
    g.drawText("ON/OFF", btnEqOn.getX()-5,          btnEqOn.getY()-11,52,10,juce::Justification::centred);

    g.setColour(juce::Colour(0xff555555));
    g.drawLine(cx(slEqLoFreq)+14, yTL, cx(slEqLoLevel)-14,  yTL, 1.f);
    g.drawLine(cx(slEqMidFreq)+14,yTL, cx(slEqMidLevel)-14, yTL, 1.f);
    g.drawLine(cx(slEqHiFreq)+14, yTL, cx(slEqHiLevel)-14,  yTL, 1.f);

    g.setColour(juce::Colour(0xffcccccc));
    g.setFont(juce::Font(juce::FontOptions("Arial",9.f,juce::Font::bold)));
    g.drawText("LO",  cx(slEqLoFreq), yTT,cx(slEqLoLevel)-cx(slEqLoFreq),  10,juce::Justification::centred);
    g.drawText("MID", cx(slEqMidFreq),yTT,cx(slEqMidLevel)-cx(slEqMidFreq),10,juce::Justification::centred);
    g.drawText("HI",  cx(slEqHiFreq), yTT,cx(slEqHiLevel)-cx(slEqHiFreq),  10,juce::Justification::centred);

    g.setColour(juce::Colour(0xffaaaaaa));
    g.setFont(juce::Font(juce::FontOptions("Arial",8.f,juce::Font::plain)));
    for (auto* sl:{&slEqLoFreq,&slEqMidFreq,&slEqHiFreq})
        g.drawText("FREQ", sl->getX(),yMT,45,10,juce::Justification::centred);
    for (auto* sl:{&slEqLoLevel,&slEqMidLevel,&slEqHiLevel})
        g.drawText("LEVEL",sl->getX(),yMT,45,10,juce::Justification::centred);

    g.setColour(juce::Colour(0xff777777));
    g.setFont(juce::Font(juce::FontOptions("Arial",7.f,juce::Font::plain)));
    g.drawText("50~700",  slEqLoFreq.getX()-5,  yBT,55,10,juce::Justification::centred);
    g.drawText("-15~15",  slEqLoLevel.getX()-5, yBT,55,10,juce::Justification::centred);
    g.drawText("350~5k",  slEqMidFreq.getX()-5, yBT,55,10,juce::Justification::centred);
    g.drawText("-15~15",  slEqMidLevel.getX()-5,yBT,55,10,juce::Justification::centred);
    g.drawText("2k~16k",  slEqHiFreq.getX()-5,  yBT,55,10,juce::Justification::centred);
    g.drawText("-15~15",  slEqHiLevel.getX()-5, yBT,55,10,juce::Justification::centred);

    g.setColour(juce::Colour(0xffbbbbbb));
    g.setFont(juce::Font(juce::FontOptions("Arial",9.f,juce::Font::bold)));
    g.drawText("MIXING",      slMix.getX(),    slMix.getY()-24,50,10,juce::Justification::centred);
    g.setColour(juce::Colour(0xff777777));
    g.setFont(juce::Font(juce::FontOptions("Arial",7.5f,juce::Font::plain)));
    g.drawText("DIRECT  REV", slMix.getX()-10, slMix.getBottom()+4,70,10,juce::Justification::centred);

    g.setColour(juce::Colour(0xff444444)); g.drawVerticalLine(665,18,getHeight()-18);
    g.setColour(juce::Colour(0xff222222)); g.drawVerticalLine(666,18,getHeight()-18);

    g.setFont(juce::Font(juce::FontOptions("Baskerville",12.f,juce::Font::italic)));
    g.setColour(juce::Colour(0xffb8960a).withAlpha(0.7f));
    g.drawText("By David Signals ",getWidth()-158,getHeight()-22,148,15,
               juce::Justification::centredRight);
}

// ================================================================
//  RESIZED
// ================================================================
void YamahaREV5AudioProcessorEditor::resized()
{
    btnPower.setBounds(22,135,42,42);
    slInputLevel.setBounds(82,132,50,50);
    btnMonoStereo.setBounds(148,153,42,20);
    btnEqOn.setBounds(202,153,42,20);

    int startX=252, gap=52;
    slEqLoFreq.setBounds  (startX,          136,45,45);
    slEqLoLevel.setBounds (startX+gap,      136,45,45);
    slEqMidFreq.setBounds (startX+gap*2+10, 136,45,45);
    slEqMidLevel.setBounds(startX+gap*3+10, 136,45,45);
    slEqHiFreq.setBounds  (startX+gap*4+20, 136,45,45);
    slEqHiLevel.setBounds (startX+gap*5+20, 136,45,45);
    slMix.setBounds(598,132,50,50);

    inMeter.setBounds(180,37,12,62);
    lcdDisplay.setBounds(222,28,392,72);

    int bx=678, by=42, bw=56, bh=33, pad=6;
    btnRev1.setBounds(bx,by,bw,bh);
    btnER1.setBounds(bx+bw+pad,by,bw,bh);
    btnRev2.setBounds(bx,by+(bh+pad),bw,bh);
    btnER2.setBounds(bx+bw+pad,by+(bh+pad),bw,bh);
    btnRev3.setBounds(bx,by+(bh+pad)*2,bw,bh);
    btnOthers.setBounds(bx+bw+pad,by+(bh+pad)*2,bw,bh);
    btnRev4.setBounds(bx,by+(bh+pad)*3,bw,bh);
    btnUser.setBounds(bx+bw+pad,by+(bh+pad)*3,bw,bh);

    bx+=(bw+pad)*2+8;
    btnParam.setBounds(bx,by,bw,bh);
    btnLevel.setBounds(bx+bw+pad,by,bw,bh);
    btnInitDly.setBounds(bx,by+(bh+pad),bw,bh);
    btn1stRef.setBounds(bx+bw+pad,by+(bh+pad),bw,bh);
    btnEqDigital.setBounds(bx,by+(bh+pad)*2,bw,bh);
    btnEqDigOn.setBounds(bx+bw+pad,by+(bh+pad)*2,bw,bh);
    btnDown.setBounds(bx,by+(bh+pad)*3,bw,bh);
    btnUp.setBounds(bx+bw+pad,by+(bh+pad)*3,bw,bh);

    bx+=(bw+pad)*2+8;
    int kw=40;
    btn7.setBounds(bx,by,kw,bh);
    btn8.setBounds(bx+kw+pad,by,kw,bh);
    btn9.setBounds(bx+(kw+pad)*2,by,kw,bh);
    btnMemory.setBounds(bx+(kw+pad)*3,by,kw,bh);
    btn4.setBounds(bx,by+(bh+pad),kw,bh);
    btn5.setBounds(bx+kw+pad,by+(bh+pad),kw,bh);
    btn6.setBounds(bx+(kw+pad)*2,by+(bh+pad),kw,bh);
    btnStore.setBounds(bx+(kw+pad)*3,by+(bh+pad),kw,bh);
    btn1.setBounds(bx,by+(bh+pad)*2,kw,bh);
    btn2.setBounds(bx+kw+pad,by+(bh+pad)*2,kw,bh);
    btn3.setBounds(bx+(kw+pad)*2,by+(bh+pad)*2,kw,bh);
    btnRecall.setBounds(bx+(kw+pad)*3,by+(bh+pad)*2,kw,bh);
    btn0.setBounds(bx,by+(bh+pad)*3,kw,bh);
    btnDot.setBounds(bx+kw+pad,by+(bh+pad)*3,kw,bh);
    btnClear.setBounds(bx+(kw+pad)*2,by+(bh+pad)*3,kw,bh);
    btnMinus.setBounds(bx+(kw+pad)*3,by+(bh+pad)*3,kw,bh);

    bx=getWidth()-82;
    btnMute.setBounds(bx,by,70,bh);
    btnIntParam.setBounds(bx,by+(bh+pad),70,bh);
    btnUtility.setBounds(bx,by+(bh+pad)*2,70,bh);
    btnBypass.setBounds(bx,by+(bh+pad)*3,70,bh);
}

// ================================================================
//  buttonClicked
// ================================================================
void YamahaREV5AudioProcessorEditor::buttonClicked(juce::Button* b)
{
    const int TOTAL=YamahaREV5AudioProcessor::TOTAL_PROGRAMS;
    const int NFACT=YamahaREV5AudioProcessor::NUM_FACTORY;

    if      (b==&btnRev1)   changeProgram(1,"LARGE HALL 1");
    else if (b==&btnRev2)   changeProgram(2,"LARGE HALL 2");
    else if (b==&btnRev3)   changeProgram(3,"SMALL HALL");
    else if (b==&btnRev4)   changeProgram(7,"ROOM 1");
    else if (b==&btnER1)    changeProgram(11,"ER SMALL");
    else if (b==&btnER2)    changeProgram(12,"ER MEDIUM");
    else if (b==&btnOthers) changeProgram(22,"CHORUS");
    else if (b==&btnUser)   changeProgram(NFACT+1,"USER 1");
    else if (b==&btnParam)   { currentPage=MenuPage::ReverbTime;   updateLCDMenu(); }
    else if (b==&btnInitDly) { currentPage=MenuPage::InitialDelay; updateLCDMenu(); }
    else if (b==&btnLevel)   { currentPage=MenuPage::Main;         updateLCDMenu(); }
    else if (b==&btnUp)
    {
        if (currentPage==MenuPage::ReverbTime)
        { if(auto*p=proc.apvts.getParameter("reverbTime")) p->setValueNotifyingHost(juce::jlimit(0.f,1.f,p->getValue()+0.02f)); }
        else if (currentPage==MenuPage::InitialDelay)
        { if(auto*p=proc.apvts.getParameter("erRevDelay")) p->setValueNotifyingHost(juce::jlimit(0.f,1.f,p->getValue()+0.02f)); }
        else
        {
            int n=juce::jlimit(1,TOTAL,currentProgramNumber+1);
            currentProgramNumber=n;
            currentProgramName=proc.getProgramName(n-1).trimCharactersAtStart("0123456789 ");
            proc.setCurrentProgram(n-1);
        }
        updateLCDMenu();
    }
    else if (b==&btnDown)
    {
        if (currentPage==MenuPage::ReverbTime)
        { if(auto*p=proc.apvts.getParameter("reverbTime")) p->setValueNotifyingHost(juce::jlimit(0.f,1.f,p->getValue()-0.02f)); }
        else if (currentPage==MenuPage::InitialDelay)
        { if(auto*p=proc.apvts.getParameter("erRevDelay")) p->setValueNotifyingHost(juce::jlimit(0.f,1.f,p->getValue()-0.02f)); }
        else
        {
            int n=juce::jlimit(1,TOTAL,currentProgramNumber-1);
            currentProgramNumber=n;
            currentProgramName=proc.getProgramName(n-1).trimCharactersAtStart("0123456789 ");
            proc.setCurrentProgram(n-1);
        }
        updateLCDMenu();
    }
    else if (b==&btnStore)
    {
        int idx=currentProgramNumber-1;
        if (idx>=NFACT && idx<TOTAL)
        { proc.saveToUserSlot(idx-NFACT); lcdDisplay.update("STORED OK","SLOT U"+juce::String(idx-NFACT+1),currentProgramNumber); }
        else
        { lcdDisplay.update("READ ONLY!","USE USER SLOT",currentProgramNumber); }
    }
    else if (b==&btnRecall)
    {
        int idx=juce::jlimit(0,TOTAL-1,currentProgramNumber-1);
        proc.setCurrentProgram(idx);
        currentProgramName=proc.getProgramName(idx).trimCharactersAtStart("0123456789 ");
        updateLCDMenu();
    }
    else if (b==&btnClear)
    {
        digitEntryBuffer=-1; currentPage=MenuPage::Main; updateLCDMenu();
    }
    else
    {
        int digit=-1;
        if(b==&btn0)digit=0; else if(b==&btn1)digit=1;
        else if(b==&btn2)digit=2; else if(b==&btn3)digit=3;
        else if(b==&btn4)digit=4; else if(b==&btn5)digit=5;
        else if(b==&btn6)digit=6; else if(b==&btn7)digit=7;
        else if(b==&btn8)digit=8; else if(b==&btn9)digit=9;
        if (digit>=0)
        {
            if (digitEntryBuffer<0)
            { digitEntryBuffer=digit; lcdDisplay.update("ENTER: "+juce::String(digit)+"_","",digit); }
            else
            {
                int t=juce::jlimit(1,TOTAL,digitEntryBuffer*10+digit);
                digitEntryBuffer=-1;
                currentProgramNumber=t;
                currentProgramName=proc.getProgramName(t-1).trimCharactersAtStart("0123456789 ");
                proc.setCurrentProgram(t-1);
                currentPage=MenuPage::Main;
                updateLCDMenu();
            }
        }
    }
}

// ================================================================
//  changeProgram / updateLCDMenu / timerCallback
// ================================================================
void YamahaREV5AudioProcessorEditor::changeProgram(int progNum,
                                                    const juce::String& name)
{
    currentProgramNumber=juce::jlimit(1,YamahaREV5AudioProcessor::TOTAL_PROGRAMS,progNum);
    currentProgramName=name; currentPage=MenuPage::Main; digitEntryBuffer=-1;
    proc.setCurrentProgram(currentProgramNumber-1);
    updateLCDMenu();
}

void YamahaREV5AudioProcessorEditor::updateLCDMenu()
{
    float rt = *proc.apvts.getRawParameterValue("reverbTime");
    float id = *proc.apvts.getRawParameterValue("erRevDelay");
    bool  iF = (currentProgramNumber<=YamahaREV5AudioProcessor::NUM_FACTORY);
    juce::String pre = iF?"F":"U";
    juce::String ns  = juce::String(currentProgramNumber).paddedLeft('0',2);
    juce::String l1,l2;
    switch(currentPage)
    {
        case MenuPage::Main:
            l1=pre+ns+" "+currentProgramName;
            l2="REV TIME "+juce::String(rt,1)+"s"; break;
        case MenuPage::ReverbTime:
            l1="PARAMETER EDIT";
            l2="REV TIME "+juce::String(rt,1)+"s"; break;
        case MenuPage::InitialDelay:
            l1="PARAMETER EDIT";
            l2="INIT DLY "+juce::String(id,1)+"ms"; break;
        default: break;
    }
    lcdDisplay.update(l1,l2,currentProgramNumber);
}

void YamahaREV5AudioProcessorEditor::timerCallback()
{
    if (currentPage!=MenuPage::Main) updateLCDMenu();

    float rms=(proc.inputLevelL.load()+proc.inputLevelR.load())*0.5f;
    inMeter.setLevel(rms);

    // Pasar nivel de audio a la llama (amplificado para que reaccione bien)
    lcdDisplay.setAudioLevel(juce::jlimit(0.f,1.f, rms*3.0f));
}
