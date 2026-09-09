#pragma once

#include "CurveLane.h"
#include "NoteKeyboard.h"
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

    void paint (juce::Graphics&) override;
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

    juce::Label titleLabel;
    NoteKeyboard keyboard;
    juce::Label gridLabel;
    juce::ComboBox gridBox;
    juce::Label filterTypeLabel;
    juce::ComboBox filterTypeBox;
    juce::Label delayDivLabel;
    juce::ComboBox delayDivBox;
    juce::Label loopPeriodLabel;
    juce::ComboBox loopPeriodBox;
    juce::ToggleButton delayCutButton { "Delay Cut on Release" };
    juce::ToggleButton reverbCutButton { "Reverb Cut on Release" };
    juce::ToggleButton unfreezeButton { "Unfreeze Loop" };

    juce::Viewport viewport;
    juce::Component lanesContainer;
    std::array<std::unique_ptr<CurveLane>, stutter::numCurves> lanes;
    std::array<juce::Label, 5> groupLabels;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ActionEditor)
};

class ActionEditorWindow final : public juce::DocumentWindow
{
public:
    ActionEditorWindow (StutterCloneAudioProcessor&, int gestureIndex);
    ~ActionEditorWindow() override;
    void closeButtonPressed() override;
    void setGestureIndex (int index);
    ActionEditor& getEditor() noexcept { return editor; }

private:
    ActionEditor editor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ActionEditorWindow)
};
