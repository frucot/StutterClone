#include "PluginEditor.h"
#include "UiColours.h"
#include "UpdateChecker.h"
#include "Version.h"

#include <juce_audio_basics/juce_audio_basics.h>

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

    class HelpOverlay final : public juce::Component
    {
    public:
        std::function<void()> onClose;

        HelpOverlay()
        {
            setOpaque (false);
            setWantsKeyboardFocus (true);

            title.setText ("How StutterClone works", juce::dontSendNotification);
            title.setJustificationType (juce::Justification::centred);
            title.setColour (juce::Label::textColourId, UiColours::text);
            title.setFont (juce::Font { juce::FontOptions { 16.0f, juce::Font::bold } });
            addAndMakeVisible (title);

            body.setMultiLine (true);
            body.setReadOnly (true);
            body.setCaretVisible (false);
            body.setScrollbarsShown (true);
            body.setColour (juce::TextEditor::backgroundColourId, UiColours::background);
            body.setColour (juce::TextEditor::textColourId, UiColours::text);
            body.setColour (juce::TextEditor::outlineColourId, UiColours::accent.withAlpha (0.35f));
            body.setColour (juce::TextEditor::focusedOutlineColourId, UiColours::accent.withAlpha (0.35f));
            body.setFont (juce::Font { juce::FontOptions { 13.0f } });
            body.setText (
                "StutterClone is an audio insert with a MIDI input, not a synth.\n"
                "\n"
                "Routing\n"
                "- Put it on an audio track (or group / return).\n"
                "- Send MIDI into the plugin itself (a MIDI track pointed at the insert).\n"
                "- FX before the plugin are captured in the loop; FX after process the stutter.\n"
                "\n"
                "Gestures\n"
                "- Hold a MIDI note, or click-and-hold a key on the plugin keyboard, to capture and loop.\n"
                "- Cyan key = slot selected for editing. Orange overlay = note currently sounding.\n"
                "- Octave - / + move every slot by 12 MIDI notes.\n"
                "- Default: slot 1 is MIDI 60 (C3). Octave - -> 48 (C2), Octave + -> 72 (C4).\n"
                "- Quantize waits for the next grid, or starts now if None.\n"
                "- Highest held note wins. Other pitches are ignored.\n"
                "- No gesture = dry. The ring keeps recording.\n"
                "\n"
                "Actions\n"
                "- Each note has step curves over one 4/4 bar (Division, Reverse, pan, FX).\n"
                "- Ping-Pong reads that bar forward, then backward (8 beats), then repeats.\n"
                "- FX chain while a gesture is active: fuzz -> filter -> lo-fi -> delay -> reverb.\n"
                "\n"
                "A short click selects the action to edit. Hold the click to play that slot.",
                false);
            addAndMakeVisible (body);

            closeButton.setButtonText ("Close");
            closeButton.setColour (juce::TextButton::buttonColourId, UiColours::accent);
            closeButton.setColour (juce::TextButton::textColourOffId, UiColours::background);
            closeButton.onClick = [this] { close(); };
            addAndMakeVisible (closeButton);
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
            panel.removeFromTop (10);
            closeButton.setBounds (panel.removeFromBottom (28).removeFromRight (90));
            panel.removeFromBottom (10);
            body.setBounds (panel);
        }

        void mouseDown (const juce::MouseEvent& event) override
        {
            if (! panelBounds().contains (event.getPosition()))
                close();
        }

        bool keyPressed (const juce::KeyPress& key) override
        {
            if (key == juce::KeyPress::escapeKey)
            {
                close();
                return true;
            }

            return false;
        }

    private:
        juce::Rectangle<int> panelBounds() const
        {
            return getLocalBounds().withSizeKeepingCentre (520, 420);
        }

        void close()
        {
            if (onClose)
                onClose();
        }

        juce::Label title;
        juce::TextEditor body;
        juce::TextButton closeButton;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HelpOverlay)
    };

    class UpdateBanner final : public juce::Component
    {
    public:
        std::function<void()> onOpenReleases;
        std::function<void()> onDismiss;

        UpdateBanner()
        {
            message.setJustificationType (juce::Justification::centredLeft);
            message.setColour (juce::Label::textColourId, UiColours::text);
            message.setFont (juce::Font { juce::FontOptions { 13.0f, juce::Font::bold } });
            addAndMakeVisible (message);

            releasesButton.setButtonText ("Releases");
            releasesButton.setColour (juce::TextButton::buttonColourId, UiColours::accent);
            releasesButton.setColour (juce::TextButton::textColourOffId, UiColours::background);
            releasesButton.onClick = [this]
            {
                if (onOpenReleases)
                    onOpenReleases();
            };
            addAndMakeVisible (releasesButton);

            dismissButton.setButtonText (juce::String::fromUTF8 ("×"));
            dismissButton.setColour (juce::TextButton::buttonColourId, UiColours::background);
            dismissButton.setColour (juce::TextButton::textColourOffId, UiColours::text);
            dismissButton.setTooltip ("Dismiss");
            dismissButton.onClick = [this]
            {
                if (onDismiss)
                    onDismiss();
            };
            addAndMakeVisible (dismissButton);
        }

        void setVersion (const juce::String& version)
        {
            message.setText ("Version " + version + " available", juce::dontSendNotification);
        }

        void paint (juce::Graphics& g) override
        {
            g.fillAll (UiColours::panel);
            g.setColour (UiColours::accent.withAlpha (0.65f));
            g.fillRect (0, getHeight() - 2, getWidth(), 2);
        }

        void resized() override
        {
            auto bounds = getLocalBounds().reduced (10, 4);
            dismissButton.setBounds (bounds.removeFromRight (28));
            bounds.removeFromRight (6);
            releasesButton.setBounds (bounds.removeFromRight (88));
            bounds.removeFromRight (10);
            message.setBounds (bounds);
        }

    private:
        juce::Label message;
        juce::TextButton releasesButton;
        juce::TextButton dismissButton;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (UpdateBanner)
    };

    constexpr int kEditorWidth = 920;
    constexpr int kUpdateBannerHeight = 32;
    constexpr int kUpdateCheckDelayTicks = 24;
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

    styleButton (helpButton);
    helpButton.setTooltip ("How it works");
    helpButton.onClick = [this] { promptHelp(); };

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
    keyboard->onNoteHeld = [this] (int index, bool held)
    {
        processorRef.setUiGestureHeld (index, held);
    };
    addAndMakeVisible (*keyboard);

    styleButton (octaveDownButton);
    octaveDownButton.setTooltip ("Move slots down one octave");
    octaveDownButton.onClick = [this] { applyOctaveOffset (-1); };
    styleButton (octaveUpButton);
    octaveUpButton.setTooltip ("Move slots up one octave");
    octaveUpButton.onClick = [this] { applyOctaveOffset (1); };

    actionEditor = std::make_unique<ActionEditor> (processorRef);
    addAndMakeVisible (*actionEditor);

    syncOctaveControls();

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
    armUpdateCheck();
}

void StutterCloneAudioProcessorEditor::visibilityChanged()
{
    updateHostViewAttached();
    armUpdateCheck();
}

void StutterCloneAudioProcessorEditor::armUpdateCheck()
{
    if (updateCheckStarted || updateCheckDelayTicks > 0 || ! hostViewAttached)
        return;

    updateCheckDelayTicks = kUpdateCheckDelayTicks;
}

void StutterCloneAudioProcessorEditor::maybeStartUpdateCheck()
{
    if (updateCheckStarted || ! hostViewAttached || getPeer() == nullptr)
        return;

    updateCheckStarted = true;
    juce::Component::SafePointer<StutterCloneAudioProcessorEditor> safeThis (this);

    UpdateChecker::startCheck ([safeThis] (UpdateChecker::Result result)
    {
        if (safeThis == nullptr || ! result.updateAvailable || safeThis->getPeer() == nullptr)
            return;

        safeThis->showUpdateBanner (result.latestVersion, result.latestTag);
    });
}

void StutterCloneAudioProcessorEditor::showUpdateBanner (const juce::String& version, const juce::String& tag)
{
    if (updateBanner != nullptr || getPeer() == nullptr)
        return;

    auto banner = std::make_unique<UpdateBanner>();
    banner->setVersion (version);
    banner->onOpenReleases = [] { UpdateChecker::openReleasesPage(); };
    banner->onDismiss = [this, tag]
    {
        UpdateChecker::dismissTag (tag);
        dismissUpdateBanner();
    };

    addAndMakeVisible (*banner);
    updateBanner = std::move (banner);
    updateBanner->toFront (false);

    if (saveAsOverlay != nullptr)
        saveAsOverlay->toFront (false);

    if (helpOverlay != nullptr)
        helpOverlay->toFront (false);

    resized();
    repaint();
}

void StutterCloneAudioProcessorEditor::dismissUpdateBanner()
{
    updateBanner.reset();
    resized();
    repaint();
}

void StutterCloneAudioProcessorEditor::layoutUpdateBanner()
{
    if (updateBanner != nullptr)
        updateBanner->setBounds (getLocalBounds().removeFromTop (kUpdateBannerHeight));
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
    dismissHelpOverlay();
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

void StutterCloneAudioProcessorEditor::promptHelp()
{
    dismissSaveAsOverlay();

    auto overlay = std::make_unique<HelpOverlay>();
    overlay->onClose = [this] { dismissHelpOverlay(); };

    addAndMakeVisible (*overlay);
    overlay->toFront (true);
    helpOverlay = std::move (overlay);
    layoutHelpOverlay();
    helpOverlay->grabKeyboardFocus();
}

void StutterCloneAudioProcessorEditor::dismissHelpOverlay()
{
    helpOverlay.reset();
}

void StutterCloneAudioProcessorEditor::layoutHelpOverlay()
{
    if (helpOverlay != nullptr)
        helpOverlay->setBounds (getLocalBounds());
}

void StutterCloneAudioProcessorEditor::applyOctaveOffset (int delta)
{
    processorRef.setFirstGestureNote (processorRef.getFirstGestureNote() + delta * 12);
    syncOctaveControls();
    updateStatusDisplay();
}

void StutterCloneAudioProcessorEditor::syncOctaveControls()
{
    const int first = processorRef.getFirstGestureNote();

    if (keyboard != nullptr)
        keyboard->setFirstGestureNote (first);

    octaveDownButton.setEnabled (first > stutter::minFirstGestureNote);
    octaveUpButton.setEnabled (first < stutter::maxFirstGestureNote);
}

juce::String StutterCloneAudioProcessorEditor::gestureRangeText() const
{
    const int first = processorRef.getFirstGestureNote();
    const int last = stutter::lastGestureNoteFor (first);
    const auto firstName = juce::MidiMessage::getMidiNoteName (first, true, true, stutter::noteNameMiddleCOctave);
    const auto lastName = juce::MidiMessage::getMidiNoteName (last, true, true, stutter::noteNameMiddleCOctave);
    return firstName + "-" + lastName;
}

void StutterCloneAudioProcessorEditor::changeListenerCallback (juce::ChangeBroadcaster*)
{
    refreshPresetList();

    if (actionEditor != nullptr)
        actionEditor->setGestureIndex (actionEditor->getGestureIndex());

    syncOctaveControls();
}

void StutterCloneAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (UiColours::background);

    const int headerY = updateBanner != nullptr ? kUpdateBannerHeight : 0;
    g.setColour (UiColours::panel);
    g.fillRect (0, headerY, getWidth(), kHeaderBarHeight);
    g.setColour (UiColours::accent);
    g.fillRect (0, headerY + kHeaderBarHeight, getWidth(), 2);
}

void StutterCloneAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();

    if (updateBanner != nullptr)
        bounds.removeFromTop (kUpdateBannerHeight);

    bounds = bounds.reduced (12, 8);

    auto titleRow = bounds.removeFromTop (22);
    midiValueLabel.setBounds (titleRow.removeFromRight (80));
    titleRow.removeFromRight (6);
    bpmValueLabel.setBounds (titleRow.removeFromRight (72));
    titleRow.removeFromRight (10);
    titleLabel.setBounds (titleRow.removeFromLeft (140));
    versionLabel.setBounds (titleRow.removeFromLeft (40));
    titleRow.removeFromLeft (6);
    helpButton.setBounds (titleRow.removeFromLeft (26));
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

    auto keyboardRow = bounds.removeFromTop (56);
    octaveDownButton.setBounds (keyboardRow.removeFromLeft (28));
    keyboardRow.removeFromLeft (4);
    octaveUpButton.setBounds (keyboardRow.removeFromRight (28));
    keyboardRow.removeFromRight (4);

    if (keyboard != nullptr)
        keyboard->setBounds (keyboardRow);

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
    layoutHelpOverlay();
    layoutUpdateBanner();
}

void StutterCloneAudioProcessorEditor::timerCallback()
{
    if (updateCheckDelayTicks > 0 && hostViewAttached)
    {
        --updateCheckDelayTicks;

        if (updateCheckDelayTicks == 0)
            maybeStartUpdateCheck();
    }

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
        const auto noteName = juce::MidiMessage::getMidiNoteName (note, true, true, stutter::noteNameMiddleCOctave);
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
        gestureValueLabel.setText (gestureRangeText() + "  |  hold a note", juce::dontSendNotification);
        gestureValueLabel.setColour (juce::Label::textColourId, UiColours::text.withAlpha (0.7f));
    }
}
