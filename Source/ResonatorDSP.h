#pragma once

#include <juce_dsp/juce_dsp.h>

#include <array>
#include <vector>

// Comb-filter resonator: delay = sampleRate / f0 with linear interpolation.
class ResonatorDSP
{
public:
    ResonatorDSP() = default;

    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset() noexcept;
    void beginGesture() noexcept;

    void processFrame (int numChannels, float* frame, bool on, int midiNote, float mix) noexcept;

private:
    float readDelay (int channel, float delaySamples) const noexcept;
    void updateTargetDelay (int midiNote) noexcept;

    std::array<std::vector<float>, 8> ring {};
    juce::SmoothedValue<float> mixSmoothed;
    juce::SmoothedValue<float> delaySmoothed;

    int maxChannels = 2;
    int ringSize = 1;
    int writeIndex = 0;
    double currentSampleRate = 44100.0;

    static constexpr float kMaxDelayMs = 50.0f;
    static constexpr float kFeedback = 0.95f;
    static constexpr float kMixSmoothSeconds = 0.01f;
    static constexpr float kDelaySmoothSeconds = 0.01f;
    static constexpr float kMinFreqHz = 20.0f;
    static constexpr float kWriteLimit = 2.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ResonatorDSP)
};
