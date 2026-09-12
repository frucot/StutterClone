#include "GranularTunerDSP.h"

#include <algorithm>
#include <cmath>

void GranularTunerDSP::prepare (const juce::dsp::ProcessSpec& spec)
{
    currentSampleRate = spec.sampleRate > 0.0 ? spec.sampleRate : 44100.0;
    maxChannels = juce::jlimit (1, 8, static_cast<int> (spec.numChannels));
    ringSize = juce::jmax (256, juce::roundToInt (currentSampleRate * kBufferSeconds));

    for (int channel = 0; channel < maxChannels; ++channel)
    {
        ring[static_cast<size_t> (channel)].assign (static_cast<size_t> (ringSize), 0.0f);
    }

    mixSmoothed.reset (currentSampleRate, kMixSmoothSeconds);
    mixSmoothed.setCurrentAndTargetValue (0.0f);
    reset();
}

void GranularTunerDSP::reset() noexcept
{
    for (int channel = 0; channel < maxChannels; ++channel)
    {
        auto& buf = ring[static_cast<size_t> (channel)];

        if (! buf.empty())
            std::fill (buf.begin(), buf.end(), 0.0f);
    }

    writeIndex = 0;
    frozenWriteIndex = 0;
    samplesCaptured = 0;
    grainLengthSamples = 0.0f;
    readIndex = 0.0f;
    isFrozen = false;
    mixSmoothed.setCurrentAndTargetValue (0.0f);
}

void GranularTunerDSP::beginGesture() noexcept
{
    isFrozen = false;
    samplesCaptured = 0;
    readIndex = 0.0f;
}

void GranularTunerDSP::writeFrame (int numChannels, const float* frame) noexcept
{
    if (ringSize <= 0)
        return;

    for (int channel = 0; channel < numChannels; ++channel)
        ring[static_cast<size_t> (channel)][static_cast<size_t> (writeIndex)] = frame[channel];

    writeIndex = (writeIndex + 1) % ringSize;
}

void GranularTunerDSP::freeze() noexcept
{
    isFrozen = true;
    frozenWriteIndex = writeIndex;
    readIndex = 0.0f;
}

void GranularTunerDSP::updateGrainLength (int midiNote) noexcept
{
    const int midi = juce::jlimit (24, 96, midiNote);
    const float freq = 440.0f * std::pow (2.0f, (static_cast<float> (midi) - 69.0f) / 12.0f);
    const float safeFreq = juce::jmax (kMinFreqHz, freq);
    grainLengthSamples = juce::jlimit (kMinGrainSamples,
                                       static_cast<float> (ringSize - 1),
                                       static_cast<float> (currentSampleRate / static_cast<double> (safeFreq)));
}

float GranularTunerDSP::readGrain (int channel, float pos) const noexcept
{
    const auto& buf = ring[static_cast<size_t> (channel)];
    const int size = ringSize;
    const int len = juce::jmax (2, juce::roundToInt (grainLengthSamples));

    if (buf.empty() || size <= 1)
        return 0.0f;

    const int start = (frozenWriteIndex - len + size) % size;
    float idx = static_cast<float> (start) + pos;

    while (idx >= static_cast<float> (size))
        idx -= static_cast<float> (size);

    while (idx < 0.0f)
        idx += static_cast<float> (size);

    const int i0 = juce::jlimit (0, size - 1, static_cast<int> (idx));
    const int i1 = (i0 + 1) % size;
    const float frac = idx - static_cast<float> (i0);
    return buf[static_cast<size_t> (i0)] + frac * (buf[static_cast<size_t> (i1)] - buf[static_cast<size_t> (i0)]);
}

void GranularTunerDSP::processFrame (int numChannels, float* frame, bool on, int midiNote, float mix) noexcept
{
    const int channels = juce::jmin (numChannels, maxChannels);
    updateGrainLength (midiNote);
    mixSmoothed.setTargetValue (on ? juce::jlimit (0.0f, 1.0f, mix) : 0.0f);
    const float mixNow = mixSmoothed.getNextValue();

    if (on)
    {
        if (! isFrozen)
        {
            writeFrame (channels, frame);
            ++samplesCaptured;

            if (samplesCaptured >= juce::jmax (2, juce::roundToInt (std::ceil (grainLengthSamples))))
                freeze();
        }
    }
    else
    {
        writeFrame (channels, frame);
        isFrozen = false;
        samplesCaptured = 0;
    }

    if (mixNow <= 1.0e-6f || ! isFrozen || grainLengthSamples < kMinGrainSamples)
        return;

    const float fade = juce::jmin (32.0f, grainLengthSamples * 0.25f);
    const float pos = readIndex;

    for (int channel = 0; channel < channels; ++channel)
    {
        const float dry = frame[channel];
        float wet = readGrain (channel, pos);

        if (fade > 1.0f && pos < fade)
        {
            const float t = pos / fade;
            const float fromEnd = readGrain (channel, grainLengthSamples - fade + pos);
            wet = fromEnd * (1.0f - t) + wet * t;
        }

        frame[channel] = dry + mixNow * (wet - dry);
    }

    readIndex += 1.0f;

    if (readIndex >= grainLengthSamples)
        readIndex -= grainLengthSamples;
}
