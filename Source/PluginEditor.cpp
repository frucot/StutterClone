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

    constexpr int kEditorWidth = 920;
    constexpr int kCollapsedHeight = 230;
    constexpr int kMinWidth = 760;
    constexpr int kMaxWidth = 1200;
    constexpr int kMaxHeight = 1100;
    constexpr int kHeaderBarHeight = 36;
    constexpr int kCompactChromeHeight = 8 + 22 + 10 + 26 + 8 + 72 + 8 + 56 + 8 + 8;

    void setupBadge (juce::Label& label)
    {
        label.setJustificationType (juce::Justification::centred);
        label.setColour (juce::Label::backgroundColourId, UiColours::background);
        label.setColour (juce::Label::textColourId, UiColours::accent);
        label.setFont (juce::Font { juce::FontOptions { 13.0f, juce::Font::bold } });
    }
}

StutterCloneAudioProcessorEditor::StutterCloneAudioProcessorEditor (StutterCloneAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    processorRef.setEditorOpen (true);
    processorRef.addChangeListener (this);

    titleLabel.setText ("StutterClone", juce::dontSendNotification);
    titleLabel.setJustificationType (juce::Justification::centredLeft);
    titleLabel.setColour (juce::Label::textColourId, UiColours::text);
    titleLabel.setFont (juce::Font { juce::FontOptions { 18.0f, juce::Font::bold } });
    addAndMakeVisible (titleLabel);

    versionLabel.setText (STUTTERCLONE_VERSION_STRING, juce::dontSendNotification);
    versionLabel.setJustificationType (juce::Justification::centredLeft);
    versionLabel.setColour (juce::Label::textColourId, UiColours::text.withAlpha (0.45f));
    versionLabel.setFont (juce::Font { juce::FontOptions { 13.0f } });
    addAndMakeVisible (versionLabel);

    gestureValueLabel.setJustificationType (juce::Justification::centred);
    gestureValueLabel.setColour (juce::Label::textColourId, UiColours::accent);
    gestureValueLabel.setFont (juce::Font { juce::FontOptions { 13.0f } });
    addAndMakeVisible (gestureValueLabel);

    setupBadge (bpmValueLabel);
    addAndMakeVisible (bpmValueLabel);
    setupBadge (midiValueLabel);
    addAndMakeVisible (midiValueLabel);

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

    quantizeLabel.setText ("Quantize", juce::dontSendNotification);
    quantizeLabel.setJustificationType (juce::Justification::centredRight);
    quantizeLabel.setColour (juce::Label::textColourId, UiColours::text.withAlpha (0.65f));
    quantizeLabel.setFont (juce::Font { juce::FontOptions { 13.0f } });
    addAndMakeVisible (quantizeLabel);
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

    styleButton (editorToggle);
    editorToggle.onClick = [this] { setEditorExpanded (! editorExpanded); };
    updateEditorToggleText();

    waveformDisplay = std::make_unique<WaveformDisplay> (processorRef);
    addAndMakeVisible (*waveformDisplay);

    keyboard = std::make_unique<NoteKeyboard>();
    keyboard->onNoteClicked = [this] (int index)
    {
        selectGesture (index);
    };
    addAndMakeVisible (*keyboard);

    actionEditor = std::make_unique<ActionEditor> (processorRef);
    addAndMakeVisible (*actionEditor);

    refreshPresetList();
    lastExpandedHeight = preferredExpandedHeight();
    setSize (kEditorWidth, lastExpandedHeight);
    applyResizeLimits();
    setResizable (true, true);
    updateStatusDisplay();
    startTimerHz (30);
}

StutterCloneAudioProcessorEditor::~StutterCloneAudioProcessorEditor()
{
    stopTimer();
    processorRef.removeChangeListener (this);
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

void StutterCloneAudioProcessorEditor::selectGesture (int gestureIndex)
{
    jassert (keyboard != nullptr && actionEditor != nullptr);

    if (keyboard == nullptr || actionEditor == nullptr)
        return;

    keyboard->setSelectedIndex (gestureIndex);
    actionEditor->setGestureIndex (gestureIndex);

    if (! editorExpanded)
        setEditorExpanded (true);
}

void StutterCloneAudioProcessorEditor::updateEditorToggleText()
{
    editorToggle.setButtonText (editorExpanded ? juce::String::fromUTF8 ("▾ Editor")
                                               : juce::String::fromUTF8 ("▸ Editor"));
}

void StutterCloneAudioProcessorEditor::applyResizeLimits()
{
    if (editorExpanded)
        setResizeLimits (kMinWidth, preferredExpandedHeight(), kMaxWidth, kMaxHeight);
    else
        setResizeLimits (kMinWidth, kCollapsedHeight, kMaxWidth, kCollapsedHeight);
}

int StutterCloneAudioProcessorEditor::preferredExpandedHeight() const noexcept
{
    jassert (actionEditor != nullptr);

    if (actionEditor == nullptr)
        return kCollapsedHeight;

    return kCompactChromeHeight + actionEditor->getPreferredHeight();
}

void StutterCloneAudioProcessorEditor::updateHostViewAttached() noexcept
{
    hostViewAttached = getPeer() != nullptr;
}

void StutterCloneAudioProcessorEditor::parentHierarchyChanged()
{
    updateHostViewAttached();
}

void StutterCloneAudioProcessorEditor::visibilityChanged()
{
    updateHostViewAttached();
}

void StutterCloneAudioProcessorEditor::setEditorExpanded (bool shouldExpand)
{
    if (editorExpanded == shouldExpand)
        return;

    if (editorExpanded)
        lastExpandedHeight = juce::jmax (getHeight(), preferredExpandedHeight());

    editorExpanded = shouldExpand;

    if (actionEditor != nullptr)
        actionEditor->setVisible (editorExpanded);

    updateEditorToggleText();

    if (! hostViewAttached || getPeer() == nullptr)
        return;

    applyResizeLimits();

    const int width = juce::jlimit (kMinWidth, kMaxWidth, getWidth());

    if (editorExpanded)
    {
        lastExpandedHeight = juce::jmax (lastExpandedHeight, preferredExpandedHeight());
        setSize (width, juce::jlimit (preferredExpandedHeight(), kMaxHeight, lastExpandedHeight));
    }
    else
    {
        setSize (width, kCollapsedHeight);
    }
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

    if (actionEditor != nullptr)
        actionEditor->setGestureIndex (actionEditor->getGestureIndex());
}

void StutterCloneAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (UiColours::background);
    g.setColour (UiColours::panel);
    g.fillRect (0, 0, getWidth(), kHeaderBarHeight);
    g.setColour (UiColours::accent);
    g.fillRect (0, kHeaderBarHeight, getWidth(), 2);
}

void StutterCloneAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced (12, 8);

    auto titleRow = bounds.removeFromTop (22);
    midiValueLabel.setBounds (titleRow.removeFromRight (80));
    titleRow.removeFromRight (6);
    bpmValueLabel.setBounds (titleRow.removeFromRight (72));
    titleRow.removeFromRight (10);
    titleLabel.setBounds (titleRow.removeFromLeft (140));
    versionLabel.setBounds (titleRow.removeFromLeft (40));
    titleRow.removeFromLeft (8);
    gestureValueLabel.setBounds (titleRow);

    bounds.removeFromTop (10);

    auto presetRow = bounds.removeFromTop (26);
    editorToggle.setBounds (presetRow.removeFromRight (88));
    presetRow.removeFromRight (8);
    quantizeBox.setBounds (presetRow.removeFromRight (110));
    presetRow.removeFromRight (4);
    quantizeLabel.setBounds (presetRow.removeFromRight (72));
    presetBox.setBounds (presetRow.removeFromLeft (180));
    presetRow.removeFromLeft (6);
    saveButton.setBounds (presetRow.removeFromLeft (64));
    presetRow.removeFromLeft (4);
    saveAsButton.setBounds (presetRow.removeFromLeft (72));
    presetRow.removeFromLeft (4);
    deleteButton.setBounds (presetRow.removeFromLeft (64));

    bounds.removeFromTop (8);

    if (waveformDisplay != nullptr)
        waveformDisplay->setBounds (bounds.removeFromTop (72));
    else
        bounds.removeFromTop (72);

    bounds.removeFromTop (8);

    if (keyboard != nullptr)
        keyboard->setBounds (bounds.removeFromTop (56));
    else
        bounds.removeFromTop (56);

    if (editorExpanded)
    {
        bounds.removeFromTop (8);

        if (actionEditor != nullptr)
            actionEditor->setBounds (bounds);

        lastExpandedHeight = juce::jmax (getHeight(), preferredExpandedHeight());
    }
    else if (actionEditor != nullptr)
    {
        actionEditor->setBounds ({});
    }

    layoutSaveAsOverlay();
}

void StutterCloneAudioProcessorEditor::timerCallback()
{
    if (waveformDisplay != nullptr)
    {
        waveformDisplay->pullSnapshot();
        waveformDisplay->repaint();
    }

    if (keyboard != nullptr)
        keyboard->setHeldMask (processorRef.getHeldGestureMask());

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
