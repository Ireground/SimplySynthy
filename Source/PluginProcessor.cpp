#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Presets.h"

namespace
{
    juce::String pct (float v, int)  { return juce::String (juce::roundToInt (v)) + " %"; }
    juce::String secs (float v, int)
    {
        if (v < 1.0f) return juce::String (juce::roundToInt (v * 1000.0f)) + " ms";
        return juce::String (v, 2) + " s";
    }
    juce::String hz (float v, int)
    {
        if (v >= 1000.0f) return juce::String (v / 1000.0f, 2) + " kHz";
        return juce::String (juce::roundToInt (v)) + " Hz";
    }
    juce::String level (float v, int) { return juce::String (juce::roundToInt (v * 100.0f)) + " %"; }

    juce::NormalisableRange<float> skewed (float lo, float hi, float centre)
    {
        juce::NormalisableRange<float> r (lo, hi);
        r.setSkewForCentre (centre);
        return r;
    }

    const juce::StringArray delayNames { "1/16", "1/8", "1/8 D", "1/4", "1/4 D", "1/2" };
    constexpr float delayBeats[] { 0.25f, 0.5f, 0.75f, 1.0f, 1.5f, 2.0f };
}

juce::AudioProcessorValueTreeState::ParameterLayout SimplySynthyProcessor::createLayout()
{
    using F = juce::AudioParameterFloat;
    using I = juce::AudioParameterInt;
    using FA = juce::AudioParameterFloatAttributes;
    using IA = juce::AudioParameterIntAttributes;
    using ID = juce::ParameterID;

    std::vector<std::unique_ptr<juce::RangedAudioParameter>> ps;

    ps.push_back (std::make_unique<F> (ID { "saw", 1 },    "Saw",    juce::NormalisableRange<float> (0.0f, 1.0f), 0.8f, FA().withStringFromValueFunction (level)));
    ps.push_back (std::make_unique<F> (ID { "square", 1 }, "Square", juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f, FA().withStringFromValueFunction (level)));
    ps.push_back (std::make_unique<F> (ID { "pw", 1 },     "Pulse Width", juce::NormalisableRange<float> (5.0f, 95.0f), 50.0f, FA().withStringFromValueFunction (pct)));
    ps.push_back (std::make_unique<I> (ID { "octave", 1 }, "Octave", -2, 2, 0,
                                       IA().withStringFromValueFunction ([] (int v, int) { return v > 0 ? "+" + juce::String (v) : juce::String (v); })));
    ps.push_back (std::make_unique<I> (ID { "voices", 1 }, "Unison Voices", 1, 8, 1));
    ps.push_back (std::make_unique<F> (ID { "detune", 1 }, "Detune", juce::NormalisableRange<float> (0.0f, 100.0f), 25.0f, FA().withStringFromValueFunction (pct)));
    ps.push_back (std::make_unique<F> (ID { "spread", 1 }, "Spread", juce::NormalisableRange<float> (0.0f, 100.0f), 50.0f, FA().withStringFromValueFunction (pct)));

    ps.push_back (std::make_unique<F> (ID { "cutoff", 1 }, "Cutoff", skewed (20.0f, 20000.0f, 1000.0f), 20000.0f, FA().withStringFromValueFunction (hz)));
    ps.push_back (std::make_unique<F> (ID { "reso", 1 },   "Resonance", juce::NormalisableRange<float> (0.0f, 100.0f), 0.0f, FA().withStringFromValueFunction (pct)));
    ps.push_back (std::make_unique<F> (ID { "envamt", 1 }, "Filter Env", juce::NormalisableRange<float> (0.0f, 100.0f), 0.0f, FA().withStringFromValueFunction (pct)));

    ps.push_back (std::make_unique<F> (ID { "attack", 1 },  "Attack",  skewed (0.001f, 5.0f, 0.3f), 0.005f, FA().withStringFromValueFunction (secs)));
    ps.push_back (std::make_unique<F> (ID { "decay", 1 },   "Decay",   skewed (0.001f, 5.0f, 0.5f), 0.3f,   FA().withStringFromValueFunction (secs)));
    ps.push_back (std::make_unique<F> (ID { "sustain", 1 }, "Sustain", juce::NormalisableRange<float> (0.0f, 100.0f), 100.0f, FA().withStringFromValueFunction (pct)));
    ps.push_back (std::make_unique<F> (ID { "release", 1 }, "Release", skewed (0.005f, 8.0f, 0.6f), 0.2f, FA().withStringFromValueFunction (secs)));

    ps.push_back (std::make_unique<juce::AudioParameterChoice> (ID { "dtime", 1 }, "Delay Time", delayNames, 2));
    ps.push_back (std::make_unique<F> (ID { "dfb", 1 },   "Delay Feedback", juce::NormalisableRange<float> (0.0f, 90.0f), 35.0f, FA().withStringFromValueFunction (pct)));
    ps.push_back (std::make_unique<F> (ID { "dmix", 1 },  "Delay Mix",  juce::NormalisableRange<float> (0.0f, 100.0f), 0.0f, FA().withStringFromValueFunction (pct)));
    ps.push_back (std::make_unique<F> (ID { "rsize", 1 }, "Reverb Size", juce::NormalisableRange<float> (0.0f, 100.0f), 50.0f, FA().withStringFromValueFunction (pct)));
    ps.push_back (std::make_unique<F> (ID { "rmix", 1 },  "Reverb Mix",  juce::NormalisableRange<float> (0.0f, 100.0f), 15.0f, FA().withStringFromValueFunction (pct)));

    ps.push_back (std::make_unique<F> (ID { "volume", 1 }, "Volume", juce::NormalisableRange<float> (-24.0f, 6.0f), 0.0f,
                                       FA().withStringFromValueFunction ([] (float v, int) { return juce::String (v, 1) + " dB"; })));

    return { ps.begin(), ps.end() };
}

SimplySynthyProcessor::SimplySynthyProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "SimplySynthy", createLayout())
{
    auto r = [this] (const char* id) { return apvts.getRawParameterValue (id); };
    raw = { r ("saw"), r ("square"), r ("pw"), r ("octave"), r ("voices"), r ("detune"), r ("spread"),
            r ("cutoff"), r ("reso"), r ("envamt"), r ("attack"), r ("decay"), r ("sustain"), r ("release"),
            r ("dtime"), r ("dfb"), r ("dmix"), r ("rsize"), r ("rmix"), r ("volume") };

    for (int i = 0; i < numVoices; ++i)
        synth.addVoice (new SynthVoice());
    synth.addSound (new SynthSound());
    synth.setNoteStealingEnabled (true);
}

bool SimplySynthyProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::stereo() && out != juce::AudioChannelSet::mono())
        return false;
    return layouts.getMainInputChannelSet().isDisabled();
}

void SimplySynthyProcessor::prepareToPlay (double sampleRate, int)
{
    sr = sampleRate > 0.0 ? sampleRate : 44100.0;
    synth.setCurrentPlaybackSampleRate (sr);

    for (int i = 0; i < synth.getNumVoices(); ++i)
        if (auto* v = dynamic_cast<SynthVoice*> (synth.getVoice (i)))
            v->prepare (sr);

    delayBuf.setSize (2, (int) (sr * 4.5) + 4);
    delayBuf.clear();
    delayWrite = 0;
    dampL = dampR = 0.0f;
    delaySamples.reset (sr, 0.08);
    delaySamples.setCurrentAndTargetValue ((float) (sr * 0.375));

    reverb.setSampleRate (sr);
    reverb.reset();

    outGain.reset (sr, 0.03);
    outGain.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (raw.volume->load()));
}

void SimplySynthyProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    const int numSamples = buffer.getNumSamples();
    const int numCh = buffer.getNumChannels();
    if (numSamples <= 0 || numCh <= 0)
        return;

    // ---- gather parameters ----
    SynthParams sp;
    sp.sawLevel    = raw.saw->load();
    sp.squareLevel = raw.square->load();
    sp.pulseWidth  = raw.pw->load() / 100.0f;
    sp.octave      = (int) std::round (raw.octave->load());
    sp.unison      = (int) std::round (raw.voices->load());
    sp.detune      = raw.detune->load() / 100.0f;
    sp.spread      = raw.spread->load() / 100.0f;
    sp.cutoff      = raw.cutoff->load();
    sp.resonance   = raw.reso->load() / 100.0f;
    sp.envAmount   = raw.envamt->load() / 100.0f;
    sp.adsr        = { raw.attack->load(), raw.decay->load(), raw.sustain->load() / 100.0f, raw.release->load() };

    for (int i = 0; i < synth.getNumVoices(); ++i)
        if (auto* v = dynamic_cast<SynthVoice*> (synth.getVoice (i)))
            v->setParams (sp);

    synth.renderNextBlock (buffer, midi, 0, numSamples);
    midi.clear();

    int active = 0;
    for (int i = 0; i < synth.getNumVoices(); ++i)
        if (synth.getVoice (i)->isVoiceActive()) ++active;
    activeVoices.store (active);

    float* L = buffer.getWritePointer (0);
    float* R = numCh > 1 ? buffer.getWritePointer (1) : nullptr;

    // ---- delay (tempo synced ping-pong, darkened repeats) ----
    double bpm = 120.0;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (auto b = pos->getBpm())
                if (*b > 20.0 && *b < 999.0)
                    bpm = *b;

    const int dIdx = juce::jlimit (0, 5, (int) std::round (raw.dtime->load()));
    const int bufLen = delayBuf.getNumSamples();
    const float target = juce::jlimit (1.0f, (float) (bufLen - 3), (float) (60.0 / bpm * delayBeats[dIdx] * sr));
    delaySamples.setTargetValue (target);

    const float fb = raw.dfb->load() / 100.0f;
    const float dMix = raw.dmix->load() / 100.0f;
    const float dampCoef = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * 4500.0f / (float) sr);
    float* dl = delayBuf.getWritePointer (0);
    float* dr = delayBuf.getWritePointer (1);

    for (int i = 0; i < numSamples; ++i)
    {
        const float inL = L[i];
        const float inR = R != nullptr ? R[i] : inL;

        const float d = delaySamples.getNextValue();
        float rp = (float) delayWrite - d;
        if (rp < 0.0f) rp += (float) bufLen;
        const int i0 = (int) rp;
        const int i1 = (i0 + 1) % bufLen;
        const float fr = rp - (float) i0;
        const float wetL = dl[i0] + fr * (dl[i1] - dl[i0]);
        const float wetR = dr[i0] + fr * (dr[i1] - dr[i0]);

        dampL += dampCoef * (wetL - dampL);
        dampR += dampCoef * (wetR - dampR);

        dl[delayWrite] = 0.5f * (inL + inR) + fb * dampR;
        dr[delayWrite] = fb * dampL;
        if (++delayWrite >= bufLen) delayWrite = 0;

        if (dMix > 0.0f)
        {
            const float dry = 1.0f - 0.25f * dMix;
            L[i] = inL * dry + wetL * dMix;
            if (R != nullptr) R[i] = inR * dry + wetR * dMix;
        }
    }

    // ---- reverb ----
    const float rMix = raw.rmix->load() / 100.0f;
    juce::Reverb::Parameters rp;
    rp.roomSize   = 0.35f + 0.63f * (raw.rsize->load() / 100.0f);
    rp.damping    = 0.45f;
    rp.width      = 1.0f;
    rp.wetLevel   = rMix * 0.3f;
    rp.dryLevel   = 0.5f * (1.0f - 0.35f * rMix);   // juce::Reverb doubles the dry level internally
    rp.freezeMode = 0.0f;
    reverb.setParameters (rp);

    if (R != nullptr) reverb.processStereo (L, R, numSamples);
    else              reverb.processMono (L, numSamples);

    // ---- volume, soft clip, NaN guard, meter ----
    outGain.setTargetValue (juce::Decibels::decibelsToGain (raw.volume->load()));
    float peakL = 0.0f, peakR = 0.0f;
    bool blewUp = false;

    auto safe = [&blewUp] (float x)
    {
        if (! std::isfinite (x)) { blewUp = true; return 0.0f; }
        const float a = std::abs (x);
        if (a <= 0.8f) return x;
        const float s = 0.8f + 0.2f * std::tanh ((a - 0.8f) / 0.2f);
        return x < 0.0f ? -s : s;
    };

    for (int i = 0; i < numSamples; ++i)
    {
        const float g = outGain.getNextValue();
        L[i] = safe (L[i] * g);
        peakL = juce::jmax (peakL, std::abs (L[i]));
        if (R != nullptr)
        {
            R[i] = safe (R[i] * g);
            peakR = juce::jmax (peakR, std::abs (R[i]));
        }
    }
    if (R == nullptr) peakR = peakL;

    if (blewUp)
    {
        delayBuf.clear();
        dampL = dampR = 0.0f;
        reverb.reset();
        buffer.clear();
    }

    meterL.store (juce::jmax (meterL.load(), peakL));
    meterR.store (juce::jmax (meterR.load(), peakR));
}

// ---------------- presets ----------------
int SimplySynthyProcessor::getNumPresets() { return (int) getPresets().size(); }

juce::String SimplySynthyProcessor::getPresetName (int index)
{
    const auto& ps = getPresets();
    if (! juce::isPositiveAndBelow (index, (int) ps.size())) return {};
    if (index == 0) return "Init";
    return juce::String (ps[(size_t) index].category).toLowerCase().substring (0, 1).toUpperCase()
         + juce::String (ps[(size_t) index].category).toLowerCase().substring (1)
         + " - " + ps[(size_t) index].name;
}

void SimplySynthyProcessor::loadPreset (int index)
{
    const auto& ps = getPresets();
    if (! juce::isPositiveAndBelow (index, (int) ps.size()))
        return;

    currentPreset.store (index);
    const auto& preset = ps[(size_t) index];

    for (auto* ap : getParameters())
    {
        auto* param = dynamic_cast<juce::RangedAudioParameter*> (ap);
        if (param == nullptr) continue;

        float norm = param->getDefaultValue();
        for (const auto& kv : preset.values)
            if (param->paramID == kv.first)
                norm = param->convertTo0to1 (kv.second);

        param->beginChangeGesture();
        param->setValueNotifyingHost (norm);
        param->endChangeGesture();
    }
}

// ---------------- state ----------------
void SimplySynthyProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();
    state.setProperty ("preset", currentPreset.load(), nullptr);
    state.setProperty ("uiWidth", editorWidth.load(), nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, dest);
}

void SimplySynthyProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
    {
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto tree = juce::ValueTree::fromXml (*xml);
            currentPreset.store (juce::jlimit (0, getNumPresets() - 1, (int) tree.getProperty ("preset", 0)));
            editorWidth.store (juce::jlimit (630, 1800, (int) tree.getProperty ("uiWidth", 900)));
            apvts.replaceState (tree);
        }
    }
}

juce::AudioProcessorEditor* SimplySynthyProcessor::createEditor() { return new SimplySynthyEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new SimplySynthyProcessor(); }
