#pragma once

#include "CurveLane.h"
#include "PluginProcessor.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <array>
#include <memory>

class ActionEditor final : public juce::Component,
                           private juce::Timer
{
public:
    explicit ActionEditor (StutterCloneAudioProcessor&);
    ~ActionEditor() override;

    void setGestureIndex (int index);
    int getGestureIndex() const noexcept { return gestureIndex; }
    int getPreferredHeight() const noexcept;

    void resized() override;

private:
    void timerCallback() override;
    void reloadFromProcessor();
    void commitAction();
    void styleCombo (juce::ComboBox& box);
    void styleToggle (juce::ToggleButton& button);

    StutterCloneAudioProcessor& processor;
    int gestureIndex = 0;
    stutter::Action localAction {};

    juce::Label gridLabel;
    juce::ComboBox gridBox;
    juce::ComboBox filterTypeBox;
    juce::ComboBox delayDivBox;
    juce::ComboBox granularEngineBox;
    juce::ComboBox granularRootBox;
    juce::ComboBox granularScaleBox;
    juce::Slider granularMixSlider;
    juce::Label loopPeriodLabel;
    juce::ComboBox loopPeriodBox;
    juce::ToggleButton delayCutButton { "Delay Cut" };
    juce::ToggleButton reverbCutButton { "Reverb Cut" };
    juce::ToggleButton unfreezeButton { "Unfreeze Loop" };
    juce::ToggleButton pingPongButton { "Ping-Pong" };

    juce::Viewport viewport;
    juce::Component lanesContainer;
    std::array<std::unique_ptr<CurveLane>, stutter::numCurves> lanes;
    std::array<juce::Label, stutter::numLaneGroups> groupLabels;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ActionEditor)
};
