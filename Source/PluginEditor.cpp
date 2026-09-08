#include "PluginEditor.h"
#include "UiColours.h"
#include "Version.h"

namespace
{
    class SavePresetOverlay final : public juce::Component
    {
    public:
        std::function<void (juce::String)> onSave;
        std::function<void()> onCancel;

        SavePresetOverlay()
        {
            setOpaque (false);
            setWantsKeyboardFocus (true);

            title.setText ("Save Preset As", juce::dontSendNotification);
            title.setJustificationType (juce::Justification::centred);
            title.setColour (juce::Label::textColourId, UiColours::text);
            title.setFont (juce::Font { juce::FontOptions { 16.0f, juce::Font::bold } });
            addAndMakeVisible (title);

            nameEditor.setColour (juce::TextEditor::backgroundColourId, UiColours::background);
            nameEditor.setColour (juce::TextEditor::textColourId, UiColours::text);
            nameEditor.setColour (juce::TextEditor::outlineColourId, UiColours::accent.withAlpha (0.45f));
            nameEditor.setColour (juce::TextEditor::focusedOutlineColourId, UiColours::accent);
            nameEditor.setColour (juce::TextEditor::highlightColourId, UiColours::accent.withAlpha (0.35f));
            nameEditor.setJustification (juce::Justification::centredLeft);
            nameEditor.setFont (juce::Font { juce::FontOptions { 15.0f } });
            nameEditor.onReturnKey = [this] { confirm(); };
            nameEditor.onEscapeKey = [this] { cancel(); };
            addAndMakeVisible (nameEditor);

            saveButton.setButtonText ("Save");
            saveButton.setColour (juce::TextButton::buttonColourId, UiColours::accent);
            saveButton.setColour (juce::TextButton::textColourOffId, UiColours::background);
            saveButton.onClick = [this] { confirm(); };
            addAndMakeVisible (saveButton);

            cancelButton.setButtonText ("Cancel");
            cancelButton.setColour (juce::TextButton::buttonColourId, UiColours::background);
            cancelButton.setColour (juce::TextButton::textColourOffId, UiColours::text);
            cancelButton.onClick = [this] { cancel(); };
            addAndMakeVisible (cancelButton);
        }

        void setNameText (const juce::String& name)
        {
            nameEditor.setText (name, juce::dontSendNotification);
            nameEditor.selectAll();
        }

        void focusNameEditor()
        {
            nameEditor.grabKeyboardFocus();
        }

        void paint (juce::Graphics& g) override
        {
            g.fillAll (juce::Colours::black.withAlpha (0.55f));

            auto panel = panelBounds().toFloat();
            g.setColour (UiColours::panel);
            g.fillRoundedRectangle (panel, 8.0f);
            g.setColour (UiColours::accent.withAlpha (0.45f));
            g.drawRoundedRectangle (panel, 8.0f, 1.5f);
        }

        void resized() override
        {
            auto panel = panelBounds().reduced (16);
            title.setBounds (panel.removeFromTop (24));
            panel.removeFromTop (12);
            nameEditor.setBounds (panel.removeFromTop (28));
            panel.removeFromTop (14);
            auto buttons = panel.removeFromTop (28);
            cancelButton.setBounds (buttons.removeFromRight (90));
            buttons.removeFromRight (8);
            saveButton.setBounds (buttons.removeFromRight (90));
        }

        void mouseDown (const juce::MouseEvent& event) override
        {
            if (! panelBounds().contains (event.getPosition()))
                cancel();
        }

        bool keyPressed (const juce::KeyPress& key) override
        {
            if (key == juce::KeyPress::escapeKey)
            {
                cancel();
                return true;
            }

            return false;
        }

    private:
        juce::Rectangle<int> panelBounds() const
        {
            return getLocalBounds().withSizeKeepingCentre (320, 140);
        }

        void confirm()
        {
            if (onSave)
                onSave (nameEditor.getText());
        }

        void cancel()
        {
            if (onCancel)
                onCancel();
        }

        juce::Label title;
        juce::TextEditor nameEditor;
        juce::TextButton saveButton;
        juce::TextButton cancelButton;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SavePresetOverlay)
    };
}

StutterCloneAudioProcessorEditor::StutterCloneAudioProcessorEditor (StutterCloneAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p), waveformDisplay (p)
{
    processorRef.setEditorOpen (true);
    processorRef.addChangeListener (this);

    titleLabel.setText ("StutterClone", juce::dontSendNotification);
    titleLabel.setJustificationType (juce::Justification::centredLeft);
    titleLabel.setColour (juce::Label::textColourId, UiColours::text);
    titleLabel.setFont (juce::Font { juce::FontOptions { 22.0f, juce::Font::bold } });
    addAndMakeVisible (titleLabel);

    versionLabel.setText (STUTTERCLONE_VERSION_STRING, juce::dontSendNotification);
    versionLabel.setJustificationType (juce::Justification::centredRight);
    versionLabel.setColour (juce::Label::textColourId, UiColours::text.withAlpha (0.45f));
    versionLabel.setFont (juce::Font { juce::FontOptions { 13.0f } });
    addAndMakeVisible (versionLabel);

    auto setupCaption = [this] (juce::Label& label, const juce::String& text)
    {
        label.setText (text, juce::dontSendNotification);
        label.setJustificationType (juce::Justification::centredLeft);
        label.setColour (juce::Label::textColourId, UiColours::text.withAlpha (0.65f));
        label.setFont (juce::Font { juce::FontOptions { 13.0f } });
        addAndMakeVisible (label);
    };

    auto setupValue = [this] (juce::Label& label)
    {
        label.setJustificationType (juce::Justification::centred);
        label.setColour (juce::Label::backgroundColourId, UiColours::panel);
        label.setColour (juce::Label::textColourId, UiColours::accent);
        label.setFont (juce::Font { juce::FontOptions { 18.0f, juce::Font::bold } });
        addAndMakeVisible (label);
    };

    styleCombo (presetBox);
    presetBox.onChange = [this]
    {
        const auto name = presetBox.getText();

        if (name.isNotEmpty() && name != processorRef.getWorkingPreset().name)
            processorRef.loadNamedPreset (name);
    };

    styleButton (saveButton);
    saveButton.onClick = [this]
    {
        if (processorRef.getPresetBank().isFactoryName (processorRef.getWorkingPreset().name))
            promptSaveAs();
        else
            processorRef.saveWorkingPreset();
    };

    styleButton (saveAsButton);
    saveAsButton.onClick = [this] { promptSaveAs(); };

    styleButton (deleteButton);
    deleteButton.onClick = [this]
    {
        const auto name = processorRef.getWorkingPreset().name;

        if (! processorRef.getPresetBank().isFactoryName (name))
            processorRef.deleteNamedPreset (name);
    };

    setupCaption (quantizeLabel, "Quantize");
    styleCombo (quantizeBox);

    if (auto* choice = dynamic_cast<juce::AudioParameterChoice*> (
            processorRef.getAPVTS().getParameter (StutterCloneAudioProcessor::quantizeParamId)))
        quantizeBox.addItemList (choice->choices, 1);

    quantizeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        processorRef.getAPVTS(), StutterCloneAudioProcessor::quantizeParamId, quantizeBox);

    quantizeBox.onChange = [this]
    {
        processorRef.setWorkingQuantizeIndex (quantizeBox.getSelectedItemIndex());
    };

    addAndMakeVisible (waveformDisplay);

    setupCaption (bpmTitleLabel, "BPM");
    setupCaption (midiTitleLabel, "MIDI Trigger");
    setupCaption (gestureTitleLabel, "Gesture");
    setupValue (bpmValueLabel);
    setupValue (midiValueLabel);
    setupValue (gestureValueLabel);

    keyboard.onNoteClicked = [this] (int index)
    {
        openActionEditor (index);
    };
    addAndMakeVisible (keyboard);

    styleButton (editButton);
    editButton.onClick = [this]
    {
        openActionEditor (keyboard.getSelectedIndex());
    };

    refreshPresetList();
    setSize (540, 520);
    updateStatusDisplay();
    startTimerHz (30);
}

StutterCloneAudioProcessorEditor::~StutterCloneAudioProcessorEditor()
{
    stopTimer();
    processorRef.removeChangeListener (this);
    actionWindow.reset();
    processorRef.setEditorOpen (false);
}

void StutterCloneAudioProcessorEditor::styleCombo (juce::ComboBox& box)
{
    box.setColour (juce::ComboBox::backgroundColourId, UiColours::panel);
    box.setColour (juce::ComboBox::textColourId, UiColours::text);
    box.setColour (juce::ComboBox::outlineColourId, UiColours::accent.withAlpha (0.35f));
    addAndMakeVisible (box);
}

void StutterCloneAudioProcessorEditor::styleButton (juce::TextButton& button)
{
    button.setColour (juce::TextButton::buttonColourId, UiColours::panel);
    button.setColour (juce::TextButton::textColourOffId, UiColours::text);
    button.setColour (juce::TextButton::textColourOnId, UiColours::accent);
    addAndMakeVisible (button);
}

void StutterCloneAudioProcessorEditor::refreshPresetList()
{
    const auto current = processorRef.getWorkingPreset().name;
    presetBox.clear (juce::dontSendNotification);

    const auto names = processorRef.getPresetBank().getPresetNames();
    int selected = 1;

    for (int i = 0; i < names.size(); ++i)
    {
        presetBox.addItem (names[i], i + 1);

        if (names[i] == current)
            selected = i + 1;
    }

    presetBox.setSelectedId (selected, juce::dontSendNotification);
    deleteButton.setEnabled (! processorRef.getPresetBank().isFactoryName (current));
}

void StutterCloneAudioProcessorEditor::openActionEditor (int gestureIndex)
{
    keyboard.setSelectedIndex (gestureIndex);

    if (actionWindow == nullptr)
        actionWindow = std::make_unique<ActionEditorWindow> (processorRef, gestureIndex);
    else
        actionWindow->setGestureIndex (gestureIndex);

    actionWindow->setVisible (true);
    actionWindow->toFront (true);
}

void StutterCloneAudioProcessorEditor::dismissSaveAsOverlay()
{
    saveAsOverlay.reset();
}

void StutterCloneAudioProcessorEditor::layoutSaveAsOverlay()
{
    if (saveAsOverlay != nullptr)
        saveAsOverlay->setBounds (getLocalBounds());
}

void StutterCloneAudioProcessorEditor::promptSaveAs()
{
    auto overlay = std::make_unique<SavePresetOverlay>();
    overlay->setNameText (processorRef.getWorkingPreset().isFactory
                              ? "My Preset"
                              : processorRef.getWorkingPreset().name);
    overlay->onSave = [this] (juce::String name)
    {
        processorRef.saveWorkingPresetAs (name);
        dismissSaveAsOverlay();
    };
    overlay->onCancel = [this] { dismissSaveAsOverlay(); };

    addAndMakeVisible (*overlay);
    overlay->toFront (true);
    saveAsOverlay = std::move (overlay);
    layoutSaveAsOverlay();

    if (auto* overlayComponent = dynamic_cast<SavePresetOverlay*> (saveAsOverlay.get()))
        overlayComponent->focusNameEditor();
}

void StutterCloneAudioProcessorEditor::changeListenerCallback (juce::ChangeBroadcaster*)
{
    refreshPresetList();

    if (actionWindow != nullptr)
        actionWindow->getEditor().setGestureIndex (actionWindow->getEditor().getGestureIndex());
}

void StutterCloneAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (UiColours::background);
    g.setColour (UiColours::panel);
    g.fillRect (0, 0, getWidth(), 56);
    g.setColour (UiColours::accent);
    g.fillRect (0, 56, getWidth(), 2);
}

void StutterCloneAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced (16);
    auto titleRow = bounds.removeFromTop (24);
    versionLabel.setBounds (titleRow.removeFromRight (48));
    titleLabel.setBounds (titleRow);
    bounds.removeFromTop (10);

    auto presetRow = bounds.removeFromTop (26);
    presetBox.setBounds (presetRow.removeFromLeft (180));
    presetRow.removeFromLeft (6);
    saveButton.setBounds (presetRow.removeFromLeft (64));
    presetRow.removeFromLeft (4);
    saveAsButton.setBounds (presetRow.removeFromLeft (72));
    presetRow.removeFromLeft (4);
    deleteButton.setBounds (presetRow.removeFromLeft (64));

    bounds.removeFromTop (8);
    auto quantRow = bounds.removeFromTop (26);
    quantizeLabel.setBounds (quantRow.removeFromLeft (72));
    quantizeBox.setBounds (quantRow.removeFromLeft (110));

    bounds.removeFromTop (10);
    waveformDisplay.setBounds (bounds.removeFromTop (96));
    bounds.removeFromTop (12);

    auto status = bounds.removeFromTop (58);
    auto bpmArea = status.removeFromLeft ((status.getWidth() - 12) / 2);
    status.removeFromLeft (12);
    bpmTitleLabel.setBounds (bpmArea.removeFromTop (16));
    bpmValueLabel.setBounds (bpmArea);
    midiTitleLabel.setBounds (status.removeFromTop (16));
    midiValueLabel.setBounds (status);

    bounds.removeFromTop (8);
    gestureTitleLabel.setBounds (bounds.removeFromTop (16));
    gestureValueLabel.setBounds (bounds.removeFromTop (26));

    bounds.removeFromTop (10);
    keyboard.setBounds (bounds.removeFromTop (78));
    bounds.removeFromTop (8);
    editButton.setBounds (bounds.removeFromTop (28).removeFromLeft (140));

    layoutSaveAsOverlay();
}

void StutterCloneAudioProcessorEditor::timerCallback()
{
    waveformDisplay.pullSnapshot();
    waveformDisplay.repaint();
    keyboard.setHeldMask (processorRef.getHeldGestureMask());
    updateStatusDisplay();
}

void StutterCloneAudioProcessorEditor::updateStatusDisplay()
{
    bpmValueLabel.setText (juce::String (processorRef.getCurrentBpm(), 1), juce::dontSendNotification);

    const bool active = processorRef.isStutterActive();
    const bool pending = processorRef.isGesturePending();
    midiValueLabel.setText (active ? "Active" : (pending ? "Pending" : "Inactive"),
                            juce::dontSendNotification);
    midiValueLabel.setColour (juce::Label::textColourId,
                              active ? UiColours::accent
                                     : (pending ? juce::Colour (0xffffc857) : UiColours::inactive));

    const int note = active ? processorRef.getGestureNote() : processorRef.getPendingNote();
    const int division = processorRef.getActiveDivisionIndex();
    const int step = processorRef.getActiveStep();

    if (note >= 0)
    {
        const auto noteName = juce::MidiMessage::getMidiNoteName (note, true, true, 3);
        auto text = noteName + "  |  " + stutter::divisionNames[juce::jlimit (0, stutter::numDivisions - 1, division)];

        if (active)
            text += "  |  step " + juce::String (step + 1);

        if (pending && ! active)
            text += "  |  waiting";

        gestureValueLabel.setText (text, juce::dontSendNotification);
        gestureValueLabel.setColour (juce::Label::textColourId, UiColours::accent);
    }
    else
    {
        gestureValueLabel.setText ("C3-B3  |  hold a note", juce::dontSendNotification);
        gestureValueLabel.setColour (juce::Label::textColourId, UiColours::text.withAlpha (0.7f));
    }
}
