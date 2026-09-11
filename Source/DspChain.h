#pragma once

#include "AdaptiveFuzzDSP.h"

#include <juce_dsp/juce_dsp.h>

#include <array>

class GestureDspChain
{
public:
    GestureDspChain() = default;

    struct Settings
    {
        bool feedEffects = false;

        float fuzzGain = 0.0f;

        bool filterOn = false;
        int filterType = 0; // 0 LP, 1 HP, 2 BP
        float cutoffHz = 1000.0f;
        float resonance = 0.707f;

        bool loFiOn = false;
        float bitDepth = 16.0f;
        int downsample = 1;

        bool delayOn = false;
        float delayMix = 0.0f;
        float delaySamples = 0.0f;
        float delayFeedback = 0.35f;

        bool reverbOn = false;
        float reverbMix = 0.0f;
        float reverbSize = 0.5f;
        float reverbDamping = 0.5f;
    };

    void prepare (double sampleRate, int samplesPerBlock, int numChannels);
    void reset() noexcept;
    void beginGesture() noexcept;
    void resetDelay() noexcept;
    void resetReverb() noexcept;
    void process (juce::AudioBuffer<float>& buffer, int startSample, int numSamples, const Settings& settings) noexcept;

    // Ordered FX stages: fuzz -> filter -> lo-fi -> delay -> reverb.
    // To add a module: Settings fields, a process* stage here, Curve + EvaluatedStep,
    // XML id in PresetBank, and a row in stutter::laneGroups.

private:
    void processScratchSample (int numChannels, float* frame, const Settings& settings) noexcept;
    void processFuzz (int numChannels, float* frame, const Settings& settings) noexcept;
    void processFilter (int numChannels, float* frame, const Settings& settings) noexcept;
    void processLoFi (int numChannels, float* frame, const Settings& settings) noexcept;
    void processDelay (int numChannels, float* frame, const Settings& settings) noexcept;
    void processReverb (int numChannels, int numSamples, const Settings& settings) noexcept;

    AdaptiveFuzzDSP fuzz;
    juce::dsp::StateVariableTPTFilter<float> filter;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> delayLine;
    juce::dsp::Reverb reverb;
    juce::AudioBuffer<float> scratch;

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> cutoffSmoothed;
    juce::SmoothedValue<float> delayMixSmoothed;
    juce::SmoothedValue<float> reverbMixSmoothed;

    int maxChannels = 2;
    int downsampleHold = 0;
    std::array<float, 8> heldSample {};
    int lastFilterType = -1;
    double currentSampleRate = 44100.0;
    int maxDelaySamples = 1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GestureDspChain)
};
