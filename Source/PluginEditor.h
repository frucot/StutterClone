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
    void parentHierarchyChanged() override;
    void visibilityChanged() override;

private:
    void timerCallback() override;
    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void updateStatusDisplay();
    void refreshPresetList();
    void styleCombo (juce::ComboBox& box);
    void styleButton (juce::TextButton& button);
    void selectGesture (int gestureIndex);
    void setEditorExpanded (bool shouldExpand);
    void updateEditorToggleText();
    void applyResizeLimits();
    int preferredExpandedHeight() const noexcept;
    void updateHostViewAttached() noexcept;
    void armUpdateCheck();
    void maybeStartUpdateCheck();
    void showUpdateBanner (const juce::String& version, const juce::String& tag);
    void dismissUpdateBanner();
    void layoutUpdateBanner();
    void promptSaveAs();
    void dismissSaveAsOverlay();
    void layoutSaveAsOverlay();
    void promptHelp();
    void dismissHelpOverlay();
    void layoutHelpOverlay();
    void applyOctaveOffset (int delta);
    void syncOctaveControls();
    juce::String gestureRangeText() const;

    StutterCloneAudioProcessor& processorRef;

    juce::Label titleLabel;
    juce::Label versionLabel;
    juce::Label gestureValueLabel;
    juce::Label bpmValueLabel;
    juce::Label midiValueLabel;
    juce::ComboBox presetBox;
    juce::TextButton saveButton { "Save" };
    juce::TextButton saveAsButton { "Save As" };
    juce::TextButton deleteButton { "Delete" };
    juce::Label quantizeLabel;
    juce::ComboBox quantizeBox;
    juce::TextButton editorToggle;
    juce::TextButton helpButton { "?" };
    juce::TextButton octaveDownButton { "-" };
    juce::TextButton octaveUpButton { "+" };
    std::unique_ptr<WaveformDisplay> waveformDisplay;
    std::unique_ptr<NoteKeyboard> keyboard;
    std::unique_ptr<ActionEditor> actionEditor;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> quantizeAttachment;
    std::unique_ptr<juce::Component> saveAsOverlay;
    std::unique_ptr<juce::Component> helpOverlay;
    std::unique_ptr<juce::Component> updateBanner;

    bool editorExpanded = true;
    bool hostViewAttached = false;
    bool updateCheckStarted = false;
    int updateCheckDelayTicks = 0;
    int lastExpandedHeight = 760;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StutterCloneAudioProcessorEditor)
};
