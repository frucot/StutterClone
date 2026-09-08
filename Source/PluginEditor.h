#pragma once

#include "ActionEditor.h"
#include "NoteKeyboard.h"
#include "PluginProcessor.h"
#include "WaveformDisplay.h"

#include <memory>

class StutterCloneAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                               private juce::Timer,
                                               private juce::ChangeListener
{
public:
    explicit StutterCloneAudioProcessorEditor (StutterCloneAudioProcessor&);
    ~StutterCloneAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void updateStatusDisplay();
    void refreshPresetList();
    void styleCombo (juce::ComboBox& box);
    void styleButton (juce::TextButton& button);
    void openActionEditor (int gestureIndex);
    void promptSaveAs();
    void dismissSaveAsOverlay();
    void layoutSaveAsOverlay();

    StutterCloneAudioProcessor& processorRef;

    juce::Label titleLabel;
    juce::Label versionLabel;
    juce::ComboBox presetBox;
    juce::TextButton saveButton { "Save" };
    juce::TextButton saveAsButton { "Save As" };
    juce::TextButton deleteButton { "Delete" };
    juce::Label quantizeLabel;
    juce::ComboBox quantizeBox;
    WaveformDisplay waveformDisplay;
    juce::Label bpmTitleLabel;
    juce::Label bpmValueLabel;
    juce::Label midiTitleLabel;
    juce::Label midiValueLabel;
    juce::Label gestureTitleLabel;
    juce::Label gestureValueLabel;
    NoteKeyboard keyboard;
    juce::TextButton editButton { "Edit Action" };

    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> quantizeAttachment;
    std::unique_ptr<ActionEditorWindow> actionWindow;
    std::unique_ptr<juce::Component> saveAsOverlay;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StutterCloneAudioProcessorEditor)
};
