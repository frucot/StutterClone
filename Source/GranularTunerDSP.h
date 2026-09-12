#pragma once

#include <juce_dsp/juce_dsp.h>

#include <array>
#include <vector>

// Period-locked grain tuner after the stutter loop: freeze ~100 ms of audio and
// loop the last 1/f0 samples at the sequenced MIDI note.
class GranularTunerDSP
{
public:
    GranularTunerDSP() = default;

    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset() noexcept;
    void beginGesture() noexcept;

    void processFrame (int numChannels, float* frame, bool on, int midiNote, float mix) noexcept;

private:
    void writeFrame (int numChannels, const float* frame) noexcept;
    void freeze() noexcept;
    void updateGrainLength (int midiNote) noexcept;
    float readGrain (int channel, float pos) const noexcept;

    std::array<std::vector<float>, 8> ring {};
    juce::SmoothedValue<float> mixSmoothed;

    int maxChannels = 2;
    int ringSize = 1;
    int writeIndex = 0;
    int frozenWriteIndex = 0;
    int samplesCaptured = 0;
    double currentSampleRate = 44100.0;
    float grainLengthSamples = 0.0f;
    float readIndex = 0.0f;
    bool isFrozen = false;

    static constexpr float kBufferSeconds = 0.1f;
    static constexpr float kMixSmoothSeconds = 0.01f;
    static constexpr float kMinFreqHz = 20.0f;
    static constexpr float kMinGrainSamples = 8.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GranularTunerDSP)
};
