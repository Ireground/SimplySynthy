#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "SynthVoice.h"

class SimplySynthyProcessor : public juce::AudioProcessor
{
public:
    SimplySynthyProcessor();
    ~SimplySynthyProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "SimplySynthy"; }
    bool acceptsMidi() const override  { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 4.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override { return currentPreset.load(); }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    juce::AudioProcessorValueTreeState apvts;

    std::atomic<int> currentPreset { 0 };
    std::atomic<int> editorWidth { 900 };
    std::atomic<float> meterL { 0.0f }, meterR { 0.0f };
    std::atomic<int> activeVoices { 0 };

    static constexpr int numVoices = 16;

private:
    struct Raw
    {
        std::atomic<float> *saw, *square, *pw, *octave, *voices, *detune, *spread,
                           *cutoff, *reso, *envamt, *attack, *decay, *sustain, *release,
                           *dtime, *dfb, *dmix, *rsize, *rmix, *volume;
    } raw;

    juce::Synthesiser synth;

    // ---- delay ----
    juce::AudioBuffer<float> delayBuf;
    int delayWrite = 0;
    float dampL = 0.0f, dampR = 0.0f;
    juce::SmoothedValue<float> delaySamples;

    // ---- reverb ----
    juce::Reverb reverb;

    juce::SmoothedValue<float> outGain;
    double sr = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimplySynthyProcessor)
};
