#pragma once

#include "PluginProcessor.h"
#include "WaveformDisplay.h"

#include <memory>

class StutterCloneAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                               private juce::Timer
{
public:
    explicit StutterCloneAudioProcessorEditor (StutterCloneAudioProcessor&);
    ~StutterCloneAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void updateStatusDisplay();
    void styleToggle (juce::ToggleButton& button);
    void styleCombo (juce::ComboBox& box);
    void setupSlider (juce::Slider& slider, juce::Label& label, const juce::String& text);

    StutterCloneAudioProcessor& processorRef;

    juce::Label titleLabel;
    WaveformDisplay waveformDisplay;
    juce::Label bpmTitleLabel;
    juce::Label bpmValueLabel;
    juce::Label midiTitleLabel;
    juce::Label midiValueLabel;
    juce::Label gestureTitleLabel;
    juce::Label gestureValueLabel;
    juce::Label divisionLabel;
    juce::ComboBox loopDivisionBox;
    juce::ToggleButton sweepButton { "Sweep" };
    juce::ToggleButton reverseButton { "Reverse" };
    juce::ToggleButton panButton { "Alt Pan" };

    juce::ToggleButton filterOnButton { "Filter" };
    juce::ComboBox filterTypeBox;
    juce::Slider filterStartSlider;
    juce::Slider filterEndSlider;
    juce::Slider filterResSlider;
    juce::Label filterStartLabel;
    juce::Label filterEndLabel;
    juce::Label filterResLabel;

    juce::ToggleButton loFiOnButton { "Lo-Fi" };
    juce::Slider loFiBitsSlider;
    juce::Slider loFiDownsampleSlider;
    juce::Label loFiBitsLabel;
    juce::Label loFiDownsampleLabel;

    juce::ToggleButton delayOnButton { "Delay" };
    juce::ComboBox delayTimeBox;
    juce::Slider delayMixSlider;
    juce::Slider delayFeedbackSlider;
    juce::Label delayMixLabel;
    juce::Label delayFeedbackLabel;
    juce::ToggleButton delayCutButton { "Cut on Release" };

    juce::ToggleButton reverbOnButton { "Reverb" };
    juce::Slider reverbMixSlider;
    juce::Slider reverbSizeSlider;
    juce::Slider reverbDampSlider;
    juce::Label reverbMixLabel;
    juce::Label reverbSizeLabel;
    juce::Label reverbDampLabel;
    juce::ToggleButton reverbCutButton { "Cut on Release" };

    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;

    std::unique_ptr<ComboAttachment> loopAttachment;
    std::unique_ptr<ButtonAttachment> sweepAttachment;
    std::unique_ptr<ButtonAttachment> reverseAttachment;
    std::unique_ptr<ButtonAttachment> panAttachment;

    std::unique_ptr<ButtonAttachment> filterOnAttachment;
    std::unique_ptr<ComboAttachment> filterTypeAttachment;
    std::unique_ptr<SliderAttachment> filterStartAttachment;
    std::unique_ptr<SliderAttachment> filterEndAttachment;
    std::unique_ptr<SliderAttachment> filterResAttachment;

    std::unique_ptr<ButtonAttachment> loFiOnAttachment;
    std::unique_ptr<SliderAttachment> loFiBitsAttachment;
    std::unique_ptr<SliderAttachment> loFiDownsampleAttachment;

    std::unique_ptr<ButtonAttachment> delayOnAttachment;
    std::unique_ptr<ComboAttachment> delayTimeAttachment;
    std::unique_ptr<SliderAttachment> delayMixAttachment;
    std::unique_ptr<SliderAttachment> delayFeedbackAttachment;
    std::unique_ptr<ButtonAttachment> delayCutAttachment;

    std::unique_ptr<ButtonAttachment> reverbOnAttachment;
    std::unique_ptr<SliderAttachment> reverbMixAttachment;
    std::unique_ptr<SliderAttachment> reverbSizeAttachment;
    std::unique_ptr<SliderAttachment> reverbDampAttachment;
    std::unique_ptr<ButtonAttachment> reverbCutAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StutterCloneAudioProcessorEditor)
};
