#include "PluginEditor.h"
#include "Presets.h"

using namespace ss;

static juce::Font font (float size, bool bold = true)
{
    return juce::Font (juce::FontOptions (size, bold ? juce::Font::bold : juce::Font::plain));
}

//==============================================================================
Look::Look()
{
    setColour (juce::PopupMenu::backgroundColourId, col::screenA);
    setColour (juce::PopupMenu::textColourId, col::lcdText);
    setColour (juce::PopupMenu::headerTextColourId, col::accentLt);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, col::accent);
    setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
}

void Look::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                             float a0, float a1, juce::Slider&)
{
    const auto b = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (2.0f);
    const float r = juce::jmin (b.getWidth(), b.getHeight()) * 0.5f;
    const auto c = b.getCentre();
    const float angle = a0 + pos * (a1 - a0);
    const float arcR = r - 3.0f;

    // track
    juce::Path track;
    track.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, a0, a1, true);
    g.setColour (juce::Colour (0xffc4c9ce));
    g.strokePath (track, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // value arc + glow
    if (pos > 0.001f)
    {
        juce::Path val;
        val.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, a0, angle, true);
        g.setColour (col::accent.withAlpha (0.25f));
        g.strokePath (val, juce::PathStrokeType (8.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (col::accent);
        g.strokePath (val, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // body
    const float rb = r - 9.0f;
    g.setColour (juce::Colours::black.withAlpha (0.22f));
    g.fillEllipse (c.x - rb + 1.0f, c.y - rb + 3.0f, rb * 2.0f, rb * 2.0f);

    juce::ColourGradient body (juce::Colour (0xffffffff), c.x, c.y - rb, juce::Colour (0xffb3bac1), c.x, c.y + rb, false);
    g.setGradientFill (body);
    g.fillEllipse (c.x - rb, c.y - rb, rb * 2.0f, rb * 2.0f);
    g.setColour (juce::Colour (0xff868e96));
    g.drawEllipse (c.x - rb, c.y - rb, rb * 2.0f, rb * 2.0f, 1.0f);

    const float rc = rb * 0.78f;
    juce::ColourGradient cap (juce::Colour (0xffe3e7eb), c.x, c.y - rc, juce::Colour (0xfff7f8fa), c.x, c.y + rc, false);
    g.setGradientFill (cap);
    g.fillEllipse (c.x - rc, c.y - rc, rc * 2.0f, rc * 2.0f);

    // pointer
    const juce::Point<float> p0 (c.x + std::sin (angle) * rb * 0.22f, c.y - std::cos (angle) * rb * 0.22f);
    const juce::Point<float> p1 (c.x + std::sin (angle) * rb * 0.92f, c.y - std::cos (angle) * rb * 0.92f);
    g.setColour (juce::Colour (0xff2e3439));
    g.drawLine ({ p0, p1 }, 3.2f);
}

void Look::drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float sliderPos,
                             float, float, juce::Slider::SliderStyle, juce::Slider&)
{
    const float cx = (float) x + (float) w * 0.5f;
    const float top = (float) y + 4.0f, bottom = (float) (y + h) - 4.0f;

    // tick marks
    g.setColour (juce::Colour (0xffa9afb5));
    for (int i = 0; i <= 10; ++i)
    {
        const float ty = top + (bottom - top) * (float) i / 10.0f;
        const float len = (i % 5 == 0) ? 7.0f : 4.0f;
        g.drawLine (cx - 10.0f - len, ty, cx - 10.0f, ty, 1.0f);
        g.drawLine (cx + 10.0f, ty, cx + 10.0f + len, ty, 1.0f);
    }

    // groove
    g.setColour (juce::Colour (0xff353b41));
    g.fillRoundedRectangle (cx - 2.5f, top, 5.0f, bottom - top, 2.5f);
    g.setColour (col::accent);
    g.fillRoundedRectangle (cx - 1.5f, sliderPos, 3.0f, juce::jmax (0.0f, bottom - sliderPos), 1.5f);

    // cap
    const juce::Rectangle<float> capR (cx - 15.0f, sliderPos - 8.0f, 30.0f, 16.0f);
    g.setColour (juce::Colours::black.withAlpha (0.25f));
    g.fillRoundedRectangle (capR.translated (0.0f, 2.0f), 2.5f);
    juce::ColourGradient cg (juce::Colour (0xfffdfefe), capR.getX(), capR.getY(), juce::Colour (0xffb8bfc6), capR.getX(), capR.getBottom(), false);
    g.setGradientFill (cg);
    g.fillRoundedRectangle (capR, 2.5f);
    g.setColour (juce::Colour (0xff7b838b));
    g.drawRoundedRectangle (capR, 2.5f, 1.0f);
    g.setColour (juce::Colour (0xff353b41));
    g.drawLine (capR.getX() + 3.0f, capR.getCentreY(), capR.getRight() - 3.0f, capR.getCentreY(), 2.0f);
}

//==============================================================================
StepBox::StepBox (juce::RangedAudioParameter& p)
    : param (p),
      attachment (p, [this] (float v) { value = v; repaint(); if (onChange) onChange(); }, nullptr)
{
    attachment.sendInitialUpdate();
}

void StepBox::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    auto box = b.withTrimmedRight (16.0f);

    g.setColour (juce::Colour (0xff1c2024));
    g.fillRoundedRectangle (box, 3.0f);
    g.setColour (juce::Colour (0xff4a525a));
    g.drawRoundedRectangle (box.reduced (0.5f), 3.0f, 1.0f);

    g.setColour (col::lcdText);
    g.setFont (font (box.getHeight() * 0.58f));
    g.drawText (param.getCurrentValueAsText(), box, juce::Justification::centred);

    const float ax = b.getRight() - 8.0f;
    const float cy = b.getCentreY();
    juce::Path up, dn;
    up.addTriangle (ax - 5.0f, cy - 3.0f, ax + 5.0f, cy - 3.0f, ax, cy - 10.0f);
    dn.addTriangle (ax - 5.0f, cy + 3.0f, ax + 5.0f, cy + 3.0f, ax, cy + 10.0f);
    g.setColour (juce::Colour (0xff6b737b));
    g.fillPath (up);
    g.fillPath (dn);
}

void StepBox::step (int delta)
{
    const auto& range = param.getNormalisableRange();
    const float nv = juce::jlimit (range.start, range.end, std::round (value) + (float) delta);
    if (! juce::exactlyEqual (nv, value))
        attachment.setValueAsCompleteGesture (nv);
}

void StepBox::mouseDown (const juce::MouseEvent& e)
{
    if (e.x > getWidth() - 16)
    {
        step (e.y < getHeight() / 2 ? 1 : -1);
        dragging = false;
        return;
    }
    dragging = true;
    dragStartValue = value;
    attachment.beginGesture();
}

void StepBox::mouseDrag (const juce::MouseEvent& e)
{
    if (! dragging) return;
    const auto& range = param.getNormalisableRange();
    const float nv = juce::jlimit (range.start, range.end, std::round (dragStartValue - (float) e.getDistanceFromDragStartY() / 14.0f));
    if (! juce::exactlyEqual (nv, value))
        attachment.setValueAsPartOfGesture (nv);
}

void StepBox::mouseUp (const juce::MouseEvent&)
{
    if (dragging)
        attachment.endGesture();
    dragging = false;
}

void StepBox::mouseDoubleClick (const juce::MouseEvent&)
{
    attachment.setValueAsCompleteGesture (param.convertFrom0to1 (param.getDefaultValue()));
}

void StepBox::mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w)
{
    if (dragging) return;
    if (w.deltaY > 0.0f) step (1);
    else if (w.deltaY < 0.0f) step (-1);
}

//==============================================================================
void SynthScreen::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setColour (juce::Colour (0xff15181b));
    g.fillRoundedRectangle (b, 5.0f);
    juce::ColourGradient bg (col::screenA, 0, 0, col::screenB, 0, b.getBottom(), false);
    g.setGradientFill (bg);
    g.fillRoundedRectangle (b.reduced (3.0f), 4.0f);

    const auto& presets = getPresets();
    const int idx = juce::jlimit (0, (int) presets.size() - 1, proc.currentPreset.load());
    const auto& pr = presets[(size_t) idx];

    // preset field
    auto na = nameArea().toFloat();
    g.setColour (col::field);
    g.fillRoundedRectangle (na, 3.0f);
    g.setColour (col::lcdText);
    g.setFont (font (14.0f));
    g.drawText (juce::String (idx).paddedLeft ('0', 3) + ": " + pr.name, na.reduced (8.0f, 0.0f), juce::Justification::centredLeft);
    g.setColour (col::accentLt);
    g.setFont (font (10.0f));
    g.drawText (pr.category, na.reduced (8.0f, 0.0f), juce::Justification::centredRight);

    for (auto r : { prevArea(), nextArea() })
    {
        g.setColour (col::field);
        g.fillRoundedRectangle (r.toFloat(), 3.0f);
    }
    {
        juce::Path pl, pr2;
        auto p = prevArea().toFloat().getCentre();
        auto n = nextArea().toFloat().getCentre();
        pl.addTriangle (p.x + 3.5f, p.y - 5.0f, p.x + 3.5f, p.y + 5.0f, p.x - 4.0f, p.y);
        pr2.addTriangle (n.x - 3.5f, n.y - 5.0f, n.x - 3.5f, n.y + 5.0f, n.x + 4.0f, n.y);
        g.setColour (col::lcdText);
        g.fillPath (pl);
        g.fillPath (pr2);
    }

    // readout
    juce::Rectangle<float> ro (10.0f, 42.0f, 281.0f, 22.0f);
    g.setColour (juce::Colour (0xff30363c));
    g.fillRoundedRectangle (ro, 3.0f);
    g.setColour (juce::Colour (0xff9aa5af));
    g.setFont (font (12.0f, false));
    g.drawText (readName, ro.reduced (8.0f, 0.0f), juce::Justification::centredLeft);
    g.setColour (col::lcdText);
    g.drawText (readValue, ro.reduced (8.0f, 0.0f), juce::Justification::centredRight);

    // waveform view
    juce::Rectangle<float> wa (10.0f, 70.0f, 281.0f, (float) getHeight() - 80.0f);
    g.setColour (juce::Colour (0xff181c20));
    g.fillRoundedRectangle (wa, 3.0f);
    g.setColour (juce::Colour (0xff2a3036));
    for (int i = 1; i < 8; ++i)
        g.drawVerticalLine ((int) (wa.getX() + wa.getWidth() * (float) i / 8.0f), wa.getY() + 2.0f, wa.getBottom() - 2.0f);
    for (int i = 1; i < 4; ++i)
        g.drawHorizontalLine ((int) (wa.getY() + wa.getHeight() * (float) i / 4.0f), wa.getX() + 2.0f, wa.getRight() - 2.0f);

    const float saw = proc.apvts.getRawParameterValue ("saw")->load();
    const float sq  = proc.apvts.getRawParameterValue ("square")->load();
    const float pw  = proc.apvts.getRawParameterValue ("pw")->load() / 100.0f;
    const int voices = (int) proc.apvts.getRawParameterValue ("voices")->load();

    const int pts = 240;
    std::vector<float> ys ((size_t) pts);
    float peak = 0.0001f;
    for (int i = 0; i < pts; ++i)
    {
        float t = std::fmod (2.0f * (float) i / (float) (pts - 1), 1.0f);
        if (i == pts - 1) t = 0.9999f;
        const float v = saw * (2.0f * t - 1.0f) + sq * ((t < pw ? 1.0f : -1.0f) - (2.0f * pw - 1.0f));
        ys[(size_t) i] = v;
        peak = juce::jmax (peak, std::abs (v));
    }
    const float scale = (saw + sq) > 0.001f ? 0.8f / juce::jmax (peak, 0.5f) : 0.0f;

    juce::Path wave;
    const auto inner = wa.reduced (8.0f, 6.0f);
    for (int i = 0; i < pts; ++i)
    {
        const float px = inner.getX() + inner.getWidth() * (float) i / (float) (pts - 1);
        const float py = inner.getCentreY() - ys[(size_t) i] * scale * inner.getHeight() * 0.5f;
        if (i == 0) wave.startNewSubPath (px, py); else wave.lineTo (px, py);
    }
    g.setColour (col::accent.withAlpha (0.25f));
    g.strokePath (wave, juce::PathStrokeType (5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (col::accentLt);
    g.strokePath (wave, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    g.setColour (juce::Colour (0xff6f7a84));
    g.setFont (font (10.0f));
    g.drawText ("UNISON x" + juce::String (voices), wa.reduced (8.0f, 4.0f), juce::Justification::bottomRight);
}

void SynthScreen::mouseDown (const juce::MouseEvent& e)
{
    const int n = (int) getPresets().size();
    const int cur = proc.currentPreset.load();

    if (prevArea().contains (e.getPosition()))
        proc.loadPreset ((cur - 1 + n) % n);
    else if (nextArea().contains (e.getPosition()))
        proc.loadPreset ((cur + 1) % n);
    else if (nameArea().contains (e.getPosition()))
        showMenu();
    else
        return;

    repaint();
}

void SynthScreen::showMenu()
{
    const auto& presets = getPresets();
    juce::PopupMenu menu;
    menu.setLookAndFeel (&getLookAndFeel());
    const int cur = proc.currentPreset.load();

    menu.addItem (1, "Init", true, cur == 0);
    for (const char* cat : { "PAD", "PLUCK", "LEAD", "BASS" })
    {
        juce::PopupMenu sub;
        for (int i = 1; i < (int) presets.size(); ++i)
            if (juce::String (presets[(size_t) i].category) == cat)
                sub.addItem (i + 1, presets[(size_t) i].name, true, cur == i);
        menu.addSubMenu (cat, sub, true, nullptr, false);
    }

    juce::Component::SafePointer<SynthScreen> safe (this);
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this).withTargetScreenArea (localAreaToGlobal (nameArea())),
                        [safe] (int result)
                        {
                            if (safe == nullptr || result <= 0) return;
                            safe->proc.loadPreset (result - 1);
                            safe->repaint();
                        });
}

//==============================================================================
SynthContent::SynthContent (SimplySynthyProcessor& p) : proc (p), screen (p)
{
    setLookAndFeel (&look);

    // Row 1: oscillator + unison
    const int r1 = 128;
    addKnob ("saw",    "SAW",         73,  r1);
    addKnob ("square", "SQUARE",      190, r1);
    addKnob ("pw",     "PULSE WIDTH", 308, r1);
    addStep ("octave", "OCTAVE",      425, r1);

    addStep ("voices", "VOICES",      558, r1);
    addKnob ("detune", "DETUNE",      689, r1);
    addKnob ("spread", "SPREAD",      820, r1);

    // Row 2: filter, screen, amp env
    const int r2 = 296;
    addKnob ("cutoff", "CUTOFF",    64,  r2);
    addKnob ("reso",   "RESONANCE", 164, r2);
    addKnob ("envamt", "ENV AMT",   264, r2);

    addAndMakeVisible (screen);
    screen.setBounds (322, 210, 300, 170);

    addFader ("attack",  "A", 662);
    addFader ("decay",   "D", 726);
    addFader ("sustain", "S", 790);
    addFader ("release", "R", 854);

    // Row 3: delay, reverb, master
    const int r3 = 470;
    addKnob ("dtime", "TIME",     69,  r3);
    addKnob ("dfb",   "FEEDBACK", 179, r3);
    addKnob ("dmix",  "MIX",      289, r3);
    addKnob ("rsize", "SIZE",     410, r3);
    addKnob ("rmix",  "MIX",      524, r3);
    addKnob ("volume","VOLUME",   670, r3, 72);

    startTimerHz (30);
}

SynthContent::~SynthContent()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void SynthContent::showParam (const juce::String& id)
{
    if (auto* prm = proc.apvts.getParameter (id))
        screen.setReadout (prm->getName (40), prm->getCurrentValueAsText());
}

void SynthContent::addKnob (const char* id, const char* label, int cx, int cy, int size)
{
    auto* k = knobs.add (new ss::Knob());
    k->setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    k->setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    k->setBounds (cx - size / 2, cy - size / 2, size, size);
    k->setMouseDragSensitivity (220);
    addAndMakeVisible (k);
    attachments.add (new juce::AudioProcessorValueTreeState::SliderAttachment (proc.apvts, id, *k));

    if (auto* prm = proc.apvts.getParameter (id))
        k->setDoubleClickReturnValue (true, (double) prm->convertFrom0to1 (prm->getDefaultValue()));

    const juce::String pid (id);
    k->onHover = [this, pid] { showParam (pid); };
    k->onValueChange = [this, pid] { showParam (pid); };

    labels.push_back ({ label, { cx - 55, cy + size / 2 + 2, 110, 16 } });
}

void SynthContent::addFader (const char* id, const char* label, int cx)
{
    auto* k = knobs.add (new ss::Knob());
    k->setSliderStyle (juce::Slider::LinearVertical);
    k->setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    k->setBounds (cx - 22, 262, 44, 112);
    addAndMakeVisible (k);
    attachments.add (new juce::AudioProcessorValueTreeState::SliderAttachment (proc.apvts, id, *k));

    if (auto* prm = proc.apvts.getParameter (id))
        k->setDoubleClickReturnValue (true, (double) prm->convertFrom0to1 (prm->getDefaultValue()));

    const juce::String pid (id);
    k->onHover = [this, pid] { showParam (pid); };
    k->onValueChange = [this, pid] { showParam (pid); };

    labels.push_back ({ label, { cx - 20, 242, 40, 16 } });
}

void SynthContent::addStep (const char* id, const char* label, int cx, int cy)
{
    auto* prm = proc.apvts.getParameter (id);
    jassert (prm != nullptr);
    auto* s = steps.add (new ss::StepBox (*prm));
    s->setBounds (cx - 34, cy - 17, 68, 34);
    addAndMakeVisible (s);
    const juce::String pid (id);
    s->onHover  = [this, pid] { showParam (pid); };
    s->onChange = [this, pid] { showParam (pid); };
    labels.push_back ({ label, { cx - 55, cy + 34, 110, 16 } });
}

void SynthContent::drawSection (juce::Graphics& g, juce::Rectangle<int> r, const juce::String& title)
{
    const auto rf = r.toFloat();
    g.setColour (juce::Colours::black.withAlpha (0.12f));
    g.fillRoundedRectangle (rf.translated (0.0f, 2.0f), 4.0f);

    juce::ColourGradient body (col::sectionA, 0, rf.getY(), col::sectionB, 0, rf.getBottom(), false);
    g.setGradientFill (body);
    g.fillRoundedRectangle (rf, 4.0f);

    auto header = rf.withHeight (26.0f);
    juce::ColourGradient hg (col::headerA, 0, header.getY(), col::headerB, 0, header.getBottom(), false);
    g.setGradientFill (hg);
    g.fillRoundedRectangle (header, 4.0f);
    g.fillRect (header.withTrimmedTop (10.0f));
    g.setColour (col::border);
    g.drawHorizontalLine ((int) header.getBottom(), header.getX(), header.getRight());
    g.drawRoundedRectangle (rf.reduced (0.5f), 4.0f, 1.0f);

    g.setColour (col::text);
    g.setFont (font (15.0f).withExtraKerningFactor (0.03f));
    g.drawText (title, header, juce::Justification::centred);
}

void SynthContent::paint (juce::Graphics& g)
{
    g.fillAll (col::panel);

    // side rails with screws
    for (int x : { 0, W - 10 })
    {
        g.setColour (col::rail);
        g.fillRect (x, 0, 10, H);
    }

    // top bar
    juce::ColourGradient top (col::topA, 0, 0, col::topB, 0, 44, false);
    g.setGradientFill (top);
    g.fillRect (0, 0, W, 44);
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.drawHorizontalLine (44, 0.0f, (float) W);

    g.setColour (juce::Colour (0xffc9ced3));
    g.setFont (font (12.0f));
    g.drawText ("POLYPHONY", 24, 0, 80, 44, juce::Justification::centredLeft);
    g.drawText ("VOICES", 168, 0, 60, 44, juce::Justification::centredLeft);
    for (auto r : { juce::Rectangle<float> (104, 12, 38, 20), juce::Rectangle<float> (222, 12, 66, 20) })
    {
        g.setColour (juce::Colour (0xff1c2024));
        g.fillRoundedRectangle (r, 3.0f);
        g.setColour (juce::Colour (0xff50585f));
        g.drawRoundedRectangle (r, 3.0f, 1.0f);
    }
    g.setColour (col::lcdText);
    g.setFont (font (12.0f));
    g.drawText (juce::String (SimplySynthyProcessor::numVoices), 104, 12, 38, 20, juce::Justification::centred);
    g.drawText (juce::String (juce::jmax (0, shownVoices)) + " / " + juce::String (SimplySynthyProcessor::numVoices),
                222, 12, 66, 20, juce::Justification::centred);

    g.setColour (juce::Colour (0xff8c959e));
    g.setFont (font (11.0f));
    g.drawText ("SKETCH SYNTH", 330, 0, 240, 44, juce::Justification::centred);

    // logo
    juce::Rectangle<float> logo (676, 9, 150, 26);
    g.setColour (juce::Colour (0xffd5dade));
    g.drawRoundedRectangle (logo, 3.0f, 1.4f);
    g.setFont (font (15.0f).withExtraKerningFactor (0.18f));
    g.drawText ("SIMPLYSYNTHY", logo, juce::Justification::centred);
    g.setColour (juce::Colour (0xffaab2ba));
    g.setFont (font (8.0f));
    g.drawText ("BY", 832, 9, 50, 13, juce::Justification::centredLeft);
    g.drawText ("IREGROUND", 832, 21, 60, 13, juce::Justification::centredLeft);

    // sections
    drawSection (g, { 14, 52, 470, 150 },  "OSCILLATOR");
    drawSection (g, { 492, 52, 394, 150 }, "UNISON");
    drawSection (g, { 14, 210, 300, 170 }, "FILTER");
    drawSection (g, { 630, 210, 256, 170 }, "AMP ENV");
    drawSection (g, { 14, 388, 330, 162 }, "DELAY");
    drawSection (g, { 352, 388, 230, 162 }, "REVERB");
    drawSection (g, { 590, 388, 296, 162 }, "MASTER");

    g.setColour (col::text);
    g.setFont (font (12.0f));
    for (const auto& l : labels)
        g.drawText (l.text, l.area, juce::Justification::centred);

    // meter scale
    g.setColour (juce::Colour (0xff6c747c));
    g.setFont (font (9.0f));
    const juce::Rectangle<int> meterArea (770, 424, 44, 108);
    for (float db : { 0.0f, -6.0f, -12.0f, -24.0f, -48.0f })
    {
        const float norm = juce::jmap (db, -60.0f, 0.0f, 0.0f, 1.0f);
        const int yy = meterArea.getBottom() - (int) (norm * (float) meterArea.getHeight());
        g.drawText (db > -0.5f ? "0" : juce::String ((int) db), 820, yy - 6, 30, 12, juce::Justification::centredLeft);
    }
    g.setColour (col::text);
    g.setFont (font (12.0f));
    g.drawText ("L", 770, 534, 18, 14, juce::Justification::centred);
    g.drawText ("R", 796, 534, 18, 14, juce::Justification::centred);
}

void SynthContent::paintOverChildren (juce::Graphics& g)
{
    // level meters
    auto drawBar = [&] (int x, float lvl)
    {
        const juce::Rectangle<float> r ((float) x, 424.0f, 18.0f, 108.0f);
        g.setColour (juce::Colour (0xff1c2024));
        g.fillRoundedRectangle (r, 2.0f);

        const float db = juce::Decibels::gainToDecibels (lvl, -60.0f);
        const float norm = juce::jlimit (0.0f, 1.0f, juce::jmap (db, -60.0f, 0.0f, 0.0f, 1.0f));
        if (norm > 0.0f)
        {
            auto fill = r.reduced (3.0f);
            fill = fill.withTop (fill.getBottom() - fill.getHeight() * norm);
            juce::ColourGradient cg (col::accent, 0, r.getBottom(), col::accentLt, 0, r.getY(), false);
            cg.addColour (0.9, juce::Colour (0xffffb347));
            g.setGradientFill (cg);
            g.fillRect (fill);
        }
        for (int i = 1; i < 12; ++i)
        {
            g.setColour (juce::Colour (0xff1c2024));
            g.drawHorizontalLine ((int) (r.getY() + r.getHeight() * (float) i / 12.0f), r.getX(), r.getRight());
        }
    };
    drawBar (770, levelL);
    drawBar (796, levelR);
}

void SynthContent::timerCallback()
{
    const float pl = proc.meterL.exchange (0.0f);
    const float pr = proc.meterR.exchange (0.0f);
    levelL = juce::jmax (pl, levelL * 0.82f);
    levelR = juce::jmax (pr, levelR * 0.82f);
    repaint (766, 420, 52, 116);

    const int v = proc.activeVoices.load();
    if (v != shownVoices)
    {
        shownVoices = v;
        repaint (220, 10, 70, 24);
    }

    screen.repaint();
}

//==============================================================================
SimplySynthyEditor::SimplySynthyEditor (SimplySynthyProcessor& p)
    : AudioProcessorEditor (p), proc (p), content (p)
{
    addAndMakeVisible (content);

    const int w = juce::jlimit (630, 1800, proc.editorWidth.load());
    setResizable (true, true);
    setResizeLimits (630, 392, 1800, 1120);
    if (auto* c = getConstrainer())
        c->setFixedAspectRatio ((double) SynthContent::W / (double) SynthContent::H);
    setSize (w, juce::roundToInt ((double) w * SynthContent::H / SynthContent::W));
}

void SimplySynthyEditor::resized()
{
    const float scale = (float) getWidth() / (float) SynthContent::W;
    content.setTransform (juce::AffineTransform::scale (scale));
    content.setBounds (0, 0, SynthContent::W, SynthContent::H);
    proc.editorWidth.store (getWidth());
}
