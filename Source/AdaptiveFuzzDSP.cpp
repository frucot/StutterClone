#include "AdaptiveFuzzDSP.h"

#include <cmath>

void AdaptiveFuzzDSP::prepare (const juce::dsp::ProcessSpec& spec)
{
    maxChannels = juce::jlimit (1, 8, static_cast<int> (spec.numChannels));
    const double sampleRate = spec.sampleRate > 0.0 ? spec.sampleRate : 44100.0;

    crossover.prepare (spec);
    crossover.setType (juce::dsp::LinkwitzRileyFilterType::lowpass);
    crossover.setCutoffFrequency (kCrossoverHz);
    crossover.reset();

    // Complementary LR split at 250 Hz: lows stay round, highs take the grit.
    attackCoeff = std::exp (-1.0f / (0.005f * static_cast<float> (sampleRate)));
    releaseCoeff = std::exp (-1.0f / (0.050f * static_cast<float> (sampleRate)));
    tanhBias = std::tanh (kBias);

    gainSmoothed.reset (sampleRate, kGainSmoothSeconds);
    gainSmoothed.setCurrentAndTargetValue (0.0f);
    dryEnv.fill (0.0f);
    wetEnv.fill (0.0f);
}

void AdaptiveFuzzDSP::reset() noexcept
{
    crossover.reset();
    dryEnv.fill (0.0f);
    wetEnv.fill (0.0f);
    gainSmoothed.setCurrentAndTargetValue (0.0f);
}

void AdaptiveFuzzDSP::processFrame (int numChannels, float* frame, float fuzzGain) noexcept
{
    gainSmoothed.setTargetValue (juce::jlimit (0.0f, 1.0f, fuzzGain));
    const float gain = gainSmoothed.getNextValue();
    const int channels = juce::jmin (numChannels, maxChannels);

    for (int channel = 0; channel < channels; ++channel)
        frame[channel] = processChannel (channel, frame[channel], gain);

    crossover.snapToZero();
}

float AdaptiveFuzzDSP::processChannel (int channel, float x, float gain) noexcept
{
    float low = 0.0f;
    float high = 0.0f;
    crossover.processSample (channel, x, low, high);

    // Gain 0 must be bit-identical: LR split/sum is an all-pass, not a wire.
    if (gain <= 1.0e-6f)
        return x;

    const float lowWet = std::sin (low * juce::MathConstants<float>::pi * (1.0f + gain * kLowDrive));
    const float biased = (high * (1.0f + gain * kHighDrive)) + kBias;
    const float highWet = std::tanh (biased) - tanhBias;
    float wet = lowWet + highWet;

    const size_t index = static_cast<size_t> (channel);
    const float absDry = std::abs (x);
    const float absWet = std::abs (wet);
    const float dryCoeff = absDry > dryEnv[index] ? attackCoeff : releaseCoeff;
    const float wetCoeff = absWet > wetEnv[index] ? attackCoeff : releaseCoeff;
    dryEnv[index] = dryCoeff * dryEnv[index] + (1.0f - dryCoeff) * absDry;
    wetEnv[index] = wetCoeff * wetEnv[index] + (1.0f - wetCoeff) * absWet;

    constexpr float eps = 1.0e-4f;
    wet *= juce::jlimit (0.15f, 8.0f, (dryEnv[index] + eps) / (wetEnv[index] + eps));
    return x + gain * (wet - x);
}
