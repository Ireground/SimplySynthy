#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <cmath>

// Everything the voices need for one block, read once by the processor.
struct SynthParams
{
    float sawLevel = 0.8f;
    float squareLevel = 0.0f;
    float pulseWidth = 0.5f;   // 0.05 .. 0.95
    int   octave = 0;
    int   unison = 1;          // 1 .. 8
    float detune = 0.25f;      // 0 .. 1
    float spread = 0.5f;       // 0 .. 1
    float cutoff = 20000.0f;   // Hz
    float resonance = 0.0f;    // 0 .. 1
    float envAmount = 0.0f;    // 0 .. 1
    juce::ADSR::Parameters adsr { 0.005f, 0.3f, 1.0f, 0.2f };
};

struct SynthSound : public juce::SynthesiserSound
{
    bool appliesToNote (int) override    { return true; }
    bool appliesToChannel (int) override { return true; }
};

class SynthVoice : public juce::SynthesiserVoice
{
public:
    static constexpr int maxUnison = 8;

    using juce::SynthesiserVoice::renderNextBlock;

    void setParams (const SynthParams& p) noexcept { params = p; }

    void prepare (double sampleRate)
    {
        sr = sampleRate > 0.0 ? sampleRate : 44100.0;
        adsr.setSampleRate (sr);
        resetFilter();
    }

    bool canPlaySound (juce::SynthesiserSound* s) override
    {
        return dynamic_cast<SynthSound*> (s) != nullptr;
    }

    void startNote (int midiNote, float velocity, juce::SynthesiserSound*, int pitchWheel) override
    {
        note = midiNote;
        velGain = 0.35f + 0.65f * juce::jlimit (0.0f, 1.0f, velocity);
        pitchWheelMoved (pitchWheel);

        if (! adsr.isActive())
            resetFilter();

        for (int i = 0; i < maxUnison; ++i)
            phase[i] = (i == 0 && params.unison == 1) ? 0.0 : (double) random.nextFloat();

        adsr.setParameters (params.adsr);
        adsr.noteOn();
    }

    void stopNote (float, bool allowTailOff) override
    {
        if (allowTailOff)
        {
            adsr.noteOff();
        }
        else
        {
            adsr.reset();
            clearCurrentNote();
        }
    }

    void pitchWheelMoved (int value) override
    {
        bendSemis = 2.0f * ((float) value - 8192.0f) / 8192.0f;
    }

    void controllerMoved (int, int) override {}

    void renderNextBlock (juce::AudioBuffer<float>& out, int start, int num) override
    {
        if (! isVoiceActive())
            return;

        adsr.setParameters (params.adsr);

        const int n = juce::jlimit (1, maxUnison, params.unison);
        const double baseFreq = 440.0 * std::pow (2.0, ((double) note - 69.0 + 12.0 * params.octave + bendSemis) / 12.0);

        double inc[maxUnison];
        float gL[maxUnison], gR[maxUnison];
        const float norm = 1.0f / std::sqrt ((float) n);

        for (int i = 0; i < n; ++i)
        {
            const float pos = n > 1 ? ((float) i / (float) (n - 1)) * 2.0f - 1.0f : 0.0f; // -1 .. 1
            const double cents = pos * params.detune * 50.0;
            const double f = baseFreq * std::pow (2.0, cents / 1200.0);
            inc[i] = juce::jlimit (0.0, 0.45, f / sr);

            const float pan = pos * params.spread;                      // -1 .. 1
            const float angle = (pan + 1.0f) * juce::MathConstants<float>::pi * 0.25f;
            gL[i] = std::cos (angle) * juce::MathConstants<float>::sqrt2 * norm;
            gR[i] = std::sin (angle) * juce::MathConstants<float>::sqrt2 * norm;
        }

        const float pw = juce::jlimit (0.05f, 0.95f, params.pulseWidth);
        const float dc = 2.0f * pw - 1.0f;
        const float sawL = params.sawLevel, sqL = params.squareLevel;
        const float k = 1.414f * (1.0f - params.resonance) + 0.06f;
        const float outGain = 0.2f * velGain;

        float* left  = out.getWritePointer (0);
        float* right = out.getNumChannels() > 1 ? out.getWritePointer (1) : nullptr;

        int pos = start;
        const int end = start + num;

        while (pos < end)
        {
            const int chunk = juce::jmin (16, end - pos);

            // filter coefficients once per small chunk (cheap, smooth enough)
            const float envNow = adsr.getNextSample();
            float fc = params.cutoff * std::pow (2.0f, params.envAmount * envNow * 7.0f);
            fc = juce::jlimit (20.0f, (float) (sr * 0.45), fc);
            const float g  = std::tan (juce::MathConstants<float>::pi * fc / (float) sr);
            const float a1 = 1.0f / (1.0f + g * (g + k));
            const float a2 = g * a1;
            const float a3 = g * a2;

            for (int s = 0; s < chunk; ++s)
            {
                const float env = (s == 0) ? envNow : adsr.getNextSample();
                float l = 0.0f, r = 0.0f;

                for (int i = 0; i < n; ++i)
                {
                    const double dt = inc[i];
                    const float t = (float) phase[i];
                    const float fdt = (float) dt;

                    float v = 0.0f;
                    if (sawL > 0.0f)
                        v += sawL * (2.0f * t - 1.0f - polyBlep (t, fdt));
                    if (sqL > 0.0f)
                    {
                        float sq = (t < pw ? 1.0f : -1.0f);
                        sq += polyBlep (t, fdt);
                        float t2 = t + 1.0f - pw;
                        if (t2 >= 1.0f) t2 -= 1.0f;
                        sq -= polyBlep (t2, fdt);
                        v += sqL * (sq - dc);
                    }

                    l += v * gL[i];
                    r += v * gR[i];

                    phase[i] += dt;
                    if (phase[i] >= 1.0) phase[i] -= 1.0;
                }

                l = svf (l, a1, a2, a3, ic1L, ic2L);
                r = svf (r, a1, a2, a3, ic1R, ic2R);

                const float gOut = env * outGain;
                if (right != nullptr)
                {
                    left[pos + s]  += l * gOut;
                    right[pos + s] += r * gOut;
                }
                else
                {
                    left[pos + s] += 0.5f * (l + r) * gOut;
                }
            }

            pos += chunk;

            if (! adsr.isActive())
            {
                clearCurrentNote();
                resetFilter();
                break;
            }
        }

        // safety: a blown-up filter never stays blown up
        if (! std::isfinite (ic1L) || ! std::isfinite (ic2L) || ! std::isfinite (ic1R) || ! std::isfinite (ic2R))
            resetFilter();
    }

private:
    static inline float polyBlep (float t, float dt) noexcept
    {
        if (dt <= 0.0f) return 0.0f;
        if (t < dt)        { t /= dt;              return t + t - t * t - 1.0f; }
        if (t > 1.0f - dt) { t = (t - 1.0f) / dt;  return t * t + t + t + 1.0f; }
        return 0.0f;
    }

    static inline float svf (float x, float a1, float a2, float a3, float& ic1, float& ic2) noexcept
    {
        const float v3 = x - ic2;
        const float v1 = a1 * ic1 + a2 * v3;
        const float v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = 2.0f * v1 - ic1;
        ic2 = 2.0f * v2 - ic2;
        return v2;
    }

    void resetFilter() noexcept { ic1L = ic2L = ic1R = ic2R = 0.0f; }

    SynthParams params;
    juce::ADSR adsr;
    juce::Random random;
    double sr = 44100.0;
    double phase[maxUnison] {};
    int note = 60;
    float velGain = 1.0f;
    float bendSemis = 0.0f;
    float ic1L = 0, ic2L = 0, ic1R = 0, ic2R = 0;
};
