#pragma once

#include "PluginProcessor.h"

class WaveformDisplay final : public juce::Component
{
public:
    explicit WaveformDisplay (StutterCloneAudioProcessor&);

    void paint (juce::Graphics&) override;
    void pullSnapshot();

private:
    void fillWrappedRegion (juce::Graphics& g,
                            juce::Rectangle<float> area,
                            float startNorm,
                            float lengthNorm,
                            juce::Colour colour) const;

    StutterCloneAudioProcessor& processor;
    StutterCloneAudioProcessor::WaveformSnapshot snapshot;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WaveformDisplay)
};
