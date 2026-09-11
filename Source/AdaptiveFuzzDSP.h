#pragma once

#include <juce_dsp/juce_dsp.h>

#include <array>

// Two-band fuzz after the stutter loop: LR crossover ~250 Hz, wavefold lows, tanh highs.
class AdaptiveFuzzDSP
{
public:
    AdaptiveFuzzDSP() = default;

    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset() noexcept;

    // fuzzGain is 0..1 from the action step. Smoothed internally (~10 ms) to avoid zipper.
    void processFrame (int numChannels, float* frame, float fuzzGain) noexcept;

private:
    float processChannel (int channel, float x, float gain) noexcept;

    juce::dsp::LinkwitzRileyFilter<float> crossover;
    juce::SmoothedValue<float> gainSmoothed;
    std::array<float, 8> dryEnv {};
    std::array<float, 8> wetEnv {};

    float attackCoeff = 0.0f;
    float releaseCoeff = 0.0f;
    float tanhBias = 0.0f;
    int maxChannels = 2;

    static constexpr float kCrossoverHz = 250.0f;
    static constexpr float kBias = 0.15f;
    static constexpr float kGainSmoothSeconds = 0.01f;
    static constexpr float kLowDrive = 8.0f;
    static constexpr float kHighDrive = 18.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AdaptiveFuzzDSP)
};
