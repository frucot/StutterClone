#include "ResonatorDSP.h"

#include <algorithm>
#include <cmath>

void ResonatorDSP::prepare (const juce::dsp::ProcessSpec& spec)
{
    currentSampleRate = spec.sampleRate > 0.0 ? spec.sampleRate : 44100.0;
    maxChannels = juce::jlimit (1, 8, static_cast<int> (spec.numChannels));
    ringSize = juce::jmax (64, juce::roundToInt (currentSampleRate * (kMaxDelayMs / 1000.0)) + 1);

    for (int channel = 0; channel < maxChannels; ++channel)
        ring[static_cast<size_t> (channel)].assign (static_cast<size_t> (ringSize), 0.0f);

    mixSmoothed.reset (currentSampleRate, kMixSmoothSeconds);
    mixSmoothed.setCurrentAndTargetValue (0.0f);
    delaySmoothed.reset (currentSampleRate, kDelaySmoothSeconds);
    delaySmoothed.setCurrentAndTargetValue (static_cast<float> (currentSampleRate / 440.0));
    reset();
}

void ResonatorDSP::reset() noexcept
{
    for (int channel = 0; channel < maxChannels; ++channel)
    {
        auto& buf = ring[static_cast<size_t> (channel)];

        if (! buf.empty())
            std::fill (buf.begin(), buf.end(), 0.0f);
    }

    writeIndex = 0;
    mixSmoothed.setCurrentAndTargetValue (0.0f);
}

void ResonatorDSP::beginGesture() noexcept
{
    reset();
}

void ResonatorDSP::updateTargetDelay (int midiNote) noexcept
{
    const int midi = juce::jlimit (24, 96, midiNote);
    const float freq = 440.0f * std::pow (2.0f, (static_cast<float> (midi) - 69.0f) / 12.0f);
    const float safeFreq = juce::jmax (kMinFreqHz, freq);
    const float delay = static_cast<float> (currentSampleRate / static_cast<double> (safeFreq));
    delaySmoothed.setTargetValue (juce::jlimit (2.0f, static_cast<float> (ringSize - 2), delay));
}

float ResonatorDSP::readDelay (int channel, float delaySamples) const noexcept
{
    const auto& buf = ring[static_cast<size_t> (channel)];

    if (buf.empty() || ringSize <= 1)
        return 0.0f;

    float readIndex = static_cast<float> (writeIndex) - delaySamples;

    while (readIndex < 0.0f)
        readIndex += static_cast<float> (ringSize);

    while (readIndex >= static_cast<float> (ringSize))
        readIndex -= static_cast<float> (ringSize);

    const int indexA = juce::jlimit (0, ringSize - 1, static_cast<int> (readIndex));
    const int indexB = (indexA + 1) % ringSize;
    const float fraction = readIndex - static_cast<float> (indexA);
    return buf[static_cast<size_t> (indexA)] + fraction * (buf[static_cast<size_t> (indexB)] - buf[static_cast<size_t> (indexA)]);
}

void ResonatorDSP::processFrame (int numChannels, float* frame, bool on, int midiNote, float mix) noexcept
{
    const int channels = juce::jmin (numChannels, maxChannels);
    updateTargetDelay (midiNote);
    mixSmoothed.setTargetValue (on ? juce::jlimit (0.0f, 1.0f, mix) : 0.0f);
    const float mixNow = mixSmoothed.getNextValue();
    const float delayNow = delaySmoothed.getNextValue();

    for (int channel = 0; channel < channels; ++channel)
    {
        const float dry = frame[channel];
        const float delayed = readDelay (channel, delayNow);
        float wet = dry;

        if (on)
        {
            wet = dry + delayed * kFeedback;
            ring[static_cast<size_t> (channel)][static_cast<size_t> (writeIndex)] = juce::jlimit (-kWriteLimit, kWriteLimit, wet);
        }
        else
        {
            ring[static_cast<size_t> (channel)][static_cast<size_t> (writeIndex)] = delayed * 0.5f;
        }

        if (mixNow > 1.0e-6f)
            frame[channel] = dry + mixNow * (wet - dry);
    }

    writeIndex = (writeIndex + 1) % juce::jmax (1, ringSize);
}
