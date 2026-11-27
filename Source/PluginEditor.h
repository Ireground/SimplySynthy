#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

namespace ss
{
    namespace col
    {
        const juce::Colour rail      { 0xff2b2f33 };
        const juce::Colour topA      { 0xff464c52 }, topB { 0xff2b2f34 };
        const juce::Colour panel     { 0xffd8dbdf };
        const juce::Colour sectionA  { 0xfff4f5f7 }, sectionB { 0xffe2e5e8 };
        const juce::Colour headerA   { 0xffe0e3e6 }, headerB  { 0xffcdd1d5 };
        const juce::Colour border    { 0xffbfc4c9 };
        const juce::Colour text      { 0xff3b4147 };
        const juce::Colour accent    { 0xff2fa4e7 };
        const juce::Colour accentLt  { 0xff7fd0ff };
        const juce::Colour screenA   { 0xff2b3137 }, screenB { 0xff1f2327 };
        const juce::Colour field     { 0xff3a4148 };
        const juce::Colour lcdText   { 0xffe8ebee };
    }

    class Look : public juce::LookAndFeel_V4
    {
    public:
        Look();
        void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                               float startAngle, float endAngle, juce::Slider&) override;
        void drawLinearSlider (juce::Graphics&, int x, int y, int w, int h, float sliderPos,
                               float minPos, float maxPos, juce::Slider::SliderStyle, juce::Slider&) override;
    };

    // Slider that reports when the mouse hovers it (for the screen readout)
    class Knob : public juce::Slider
    {
    public:
        std::function<void()> onHover;
        void mouseEnter (const juce::MouseEvent& e) override { juce::Slider::mouseEnter (e); if (onHover) onHover(); }
    };

    // LCD-style number box with up/down arrows (Octave, Voices)
    class StepBox : public juce::Component
    {
    public:
        StepBox (juce::RangedAudioParameter& p);
        void paint (juce::Graphics&) override;
        void mouseDown (const juce::MouseEvent&) override;
        void mouseDrag (const juce::MouseEvent&) override;
        void mouseUp (const juce::MouseEvent&) override;
        void mouseDoubleClick (const juce::MouseEvent&) override;
        void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
        void mouseEnter (const juce::MouseEvent&) override { if (onHover) onHover(); }

        std::function<void()> onHover, onChange;
        juce::RangedAudioParameter& param;

    private:
        void step (int delta);
        float value = 0.0f;
        float dragStartValue = 0.0f;
        bool dragging = false;
        juce::ParameterAttachment attachment;
    };
}

class SynthScreen : public juce::Component
{
public:
    explicit SynthScreen (SimplySynthyProcessor& p) : proc (p) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void setReadout (const juce::String& name, const juce::String& value) { readName = name; readValue = value; repaint(); }

private:
    juce::Rectangle<int> nameArea() const  { return { 10, 10, 228, 26 }; }
    juce::Rectangle<int> prevArea() const  { return { 242, 10, 23, 26 }; }
    juce::Rectangle<int> nextArea() const  { return { 268, 10, 23, 26 }; }
    void showMenu();

    SimplySynthyProcessor& proc;
    juce::String readName { "Ready" }, readValue;
};

class SynthContent : public juce::Component, private juce::Timer
{
public:
    static constexpr int W = 900, H = 560;

    explicit SynthContent (SimplySynthyProcessor&);
    ~SynthContent() override;
    void paint (juce::Graphics&) override;
    void paintOverChildren (juce::Graphics&) override;

private:
    void timerCallback() override;
    void addKnob (const char* id, const char* label, int cx, int cy, int size = 64);
    void addFader (const char* id, const char* label, int cx);
    void addStep (const char* id, const char* label, int cx, int cy);
    void showParam (const juce::String& id);
    void drawSection (juce::Graphics&, juce::Rectangle<int>, const juce::String& title);

    SimplySynthyProcessor& proc;
    ss::Look look;
    SynthScreen screen;

    struct Label { juce::String text; juce::Rectangle<int> area; };
    std::vector<Label> labels;
    juce::OwnedArray<ss::Knob> knobs;
    juce::OwnedArray<juce::AudioProcessorValueTreeState::SliderAttachment> attachments;
    juce::OwnedArray<ss::StepBox> steps;

    float levelL = 0.0f, levelR = 0.0f;
    int shownVoices = -1;
};

class SimplySynthyEditor : public juce::AudioProcessorEditor
{
public:
    explicit SimplySynthyEditor (SimplySynthyProcessor&);
    void resized() override;
    void paint (juce::Graphics& g) override { g.fillAll (ss::col::rail); }

private:
    SimplySynthyProcessor& proc;
    SynthContent content;
};
