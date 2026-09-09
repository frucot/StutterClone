#include "ActionEditor.h"
#include "UiColours.h"

#include <juce_audio_basics/juce_audio_basics.h>

namespace
{
    constexpr int laneHeight = 52;
    constexpr int groupHeaderHeight = 22;

    struct LaneGroup
    {
        const char* title;
        int start;
        int count;
    };

    constexpr LaneGroup groups[] {
        { "Stutter", 0, 3 },
        { "Filter",  3, 3 },
        { "Lo-Fi",   6, 3 },
        { "Delay",   9, 3 },
        { "Reverb",  12, 4 }
    };
}

ActionEditor::ActionEditor (StutterCloneAudioProcessor& p)
    : processor (p)
{
    titleLabel.setText ("Action", juce::dontSendNotification);
    titleLabel.setColour (juce::Label::textColourId, UiColours::text);
    titleLabel.setFont (juce::Font { juce::FontOptions { 18.0f, juce::Font::bold } });
    addAndMakeVisible (titleLabel);

    keyboard.onNoteClicked = [this] (int index)
    {
        setGestureIndex (index);
    };
    addAndMakeVisible (keyboard);

    gridLabel.setText ("Grid", juce::dontSendNotification);
    gridLabel.setColour (juce::Label::textColourId, UiColours::text.withAlpha (0.7f));
    addAndMakeVisible (gridLabel);

    gridBox.addItem ("4", 4);
    gridBox.addItem ("8", 8);
    gridBox.addItem ("16", 16);
    gridBox.addItem ("32", 32);
    styleCombo (gridBox);
    gridBox.onChange = [this]
    {
        stutter::resampleGrid (localAction, gridBox.getSelectedId());
        commitAction();
        reloadFromProcessor();
    };

    filterTypeLabel.setText ("Filter", juce::dontSendNotification);
    filterTypeLabel.setColour (juce::Label::textColourId, UiColours::text.withAlpha (0.7f));
    addAndMakeVisible (filterTypeLabel);

    filterTypeBox.addItemList (juce::StringArray { "Lowpass", "Highpass", "Bandpass" }, 1);
    styleCombo (filterTypeBox);
    filterTypeBox.onChange = [this]
    {
        localAction.filterType = juce::jlimit (0, 2, filterTypeBox.getSelectedItemIndex());
        commitAction();
    };

    delayDivLabel.setText ("Delay", juce::dontSendNotification);
    delayDivLabel.setColour (juce::Label::textColourId, UiColours::text.withAlpha (0.7f));
    addAndMakeVisible (delayDivLabel);

    delayDivBox.addItemList (juce::StringArray { "1/4", "1/8", "1/16", "1/32" }, 1);
    styleCombo (delayDivBox);
    delayDivBox.onChange = [this]
    {
        localAction.delayDivision = juce::jlimit (0, 3, delayDivBox.getSelectedItemIndex());
        commitAction();
    };

    loopPeriodLabel.setText ("Loop", juce::dontSendNotification);
    loopPeriodLabel.setColour (juce::Label::textColourId, UiColours::text.withAlpha (0.7f));
    addAndMakeVisible (loopPeriodLabel);

    for (int i = 0; i < stutter::numLoopPeriods; ++i)
        loopPeriodBox.addItem (stutter::loopPeriodNames[i], i + 1);

    styleCombo (loopPeriodBox);
    loopPeriodBox.onChange = [this]
    {
        localAction.loopPeriod = juce::jlimit (0, stutter::numLoopPeriods - 1, loopPeriodBox.getSelectedItemIndex());
        commitAction();
    };

    styleToggle (delayCutButton);
    delayCutButton.onClick = [this]
    {
        localAction.delayCut = delayCutButton.getToggleState() ? 1 : 0;
        commitAction();
    };

    styleToggle (reverbCutButton);
    reverbCutButton.onClick = [this]
    {
        localAction.reverbCut = reverbCutButton.getToggleState() ? 1 : 0;
        commitAction();
    };

    styleToggle (unfreezeButton);
    unfreezeButton.onClick = [this]
    {
        localAction.loopUnfreeze = unfreezeButton.getToggleState() ? 1 : 0;
        loopPeriodBox.setEnabled (localAction.loopUnfreeze != 0);
        commitAction();
    };

    for (size_t i = 0; i < groupLabels.size(); ++i)
    {
        groupLabels[i].setText (groups[i].title, juce::dontSendNotification);
        groupLabels[i].setColour (juce::Label::textColourId, UiColours::accent);
        groupLabels[i].setFont (juce::Font { juce::FontOptions { 13.0f, juce::Font::bold } });
        lanesContainer.addAndMakeVisible (groupLabels[i]);
    }

    for (int i = 0; i < stutter::numCurves; ++i)
    {
        lanes[static_cast<size_t> (i)] = std::make_unique<CurveLane>();
        lanes[static_cast<size_t> (i)]->onChanged = [this] { commitAction(); };
        lanesContainer.addAndMakeVisible (*lanes[static_cast<size_t> (i)]);
    }

    viewport.setViewedComponent (&lanesContainer, false);
    viewport.setScrollBarsShown (true, false);
    addAndMakeVisible (viewport);

    setGestureIndex (0);
    startTimerHz (30);
}

ActionEditor::~ActionEditor()
{
    stopTimer();
    viewport.setViewedComponent (nullptr, false);
}

void ActionEditor::styleCombo (juce::ComboBox& box)
{
    box.setColour (juce::ComboBox::backgroundColourId, UiColours::panel);
    box.setColour (juce::ComboBox::textColourId, UiColours::text);
    box.setColour (juce::ComboBox::outlineColourId, UiColours::accent.withAlpha (0.35f));
    addAndMakeVisible (box);
}

void ActionEditor::styleToggle (juce::ToggleButton& button)
{
    button.setColour (juce::ToggleButton::textColourId, UiColours::text);
    button.setColour (juce::ToggleButton::tickColourId, UiColours::accent);
    addAndMakeVisible (button);
}

void ActionEditor::setGestureIndex (int index)
{
    gestureIndex = juce::jlimit (0, stutter::numGestureNotes - 1, index);
    reloadFromProcessor();
}

void ActionEditor::reloadFromProcessor()
{
    localAction = processor.getWorkingPreset().actions[static_cast<size_t> (gestureIndex)];
    keyboard.setSelectedIndex (gestureIndex);

    const auto noteName = juce::MidiMessage::getMidiNoteName (
        stutter::noteForGestureIndex (gestureIndex), true, true, 3);
    titleLabel.setText ("Action  |  " + noteName, juce::dontSendNotification);

    gridBox.setSelectedId (stutter::clampGridResolution (localAction.gridResolution), juce::dontSendNotification);
    filterTypeBox.setSelectedItemIndex (juce::jlimit (0, 2, localAction.filterType), juce::dontSendNotification);
    delayDivBox.setSelectedItemIndex (juce::jlimit (0, 3, localAction.delayDivision), juce::dontSendNotification);
    delayCutButton.setToggleState (localAction.delayCut != 0, juce::dontSendNotification);
    reverbCutButton.setToggleState (localAction.reverbCut != 0, juce::dontSendNotification);
    loopPeriodBox.setSelectedItemIndex (juce::jlimit (0, stutter::numLoopPeriods - 1, localAction.loopPeriod),
                                        juce::dontSendNotification);
    unfreezeButton.setToggleState (localAction.loopUnfreeze != 0, juce::dontSendNotification);
    loopPeriodBox.setEnabled (localAction.loopUnfreeze != 0);

    for (int i = 0; i < stutter::numCurves; ++i)
        lanes[static_cast<size_t> (i)]->setAction (&localAction, static_cast<stutter::Curve> (i));

    resized();
    repaint();
}

void ActionEditor::commitAction()
{
    processor.updateAction (gestureIndex, localAction);
}

void ActionEditor::paint (juce::Graphics& g)
{
    g.fillAll (UiColours::background);
}

void ActionEditor::resized()
{
    auto bounds = getLocalBounds().reduced (12);
    titleLabel.setBounds (bounds.removeFromTop (24));
    bounds.removeFromTop (8);
    keyboard.setBounds (bounds.removeFromTop (72));
    bounds.removeFromTop (8);

    auto controls = bounds.removeFromTop (28);
    gridLabel.setBounds (controls.removeFromLeft (36));
    gridBox.setBounds (controls.removeFromLeft (72));
    controls.removeFromLeft (8);
    filterTypeLabel.setBounds (controls.removeFromLeft (44));
    filterTypeBox.setBounds (controls.removeFromLeft (110));
    controls.removeFromLeft (8);
    delayDivLabel.setBounds (controls.removeFromLeft (44));
    delayDivBox.setBounds (controls.removeFromLeft (80));
    controls.removeFromLeft (8);
    loopPeriodLabel.setBounds (controls.removeFromLeft (36));
    loopPeriodBox.setBounds (controls.removeFromLeft (84));

    bounds.removeFromTop (6);
    auto cuts = bounds.removeFromTop (22);
    const int cutWidth = cuts.getWidth() / 3;
    delayCutButton.setBounds (cuts.removeFromLeft (cutWidth));
    reverbCutButton.setBounds (cuts.removeFromLeft (cutWidth));
    unfreezeButton.setBounds (cuts);

    bounds.removeFromTop (8);
    viewport.setBounds (bounds);

    int y = 0;
    int curveIndex = 0;

    for (size_t g = 0; g < groupLabels.size(); ++g)
    {
        groupLabels[g].setBounds (0, y, viewport.getWidth() - 12, groupHeaderHeight);
        y += groupHeaderHeight;

        for (int i = 0; i < groups[g].count; ++i, ++curveIndex)
        {
            lanes[static_cast<size_t> (curveIndex)]->setBounds (0, y, viewport.getWidth() - 18, laneHeight);
            y += laneHeight + 4;
        }

        y += 6;
    }

    lanesContainer.setBounds (0, 0, juce::jmax (0, viewport.getWidth() - 8), y);
}

void ActionEditor::timerCallback()
{
    keyboard.setHeldMask (processor.getHeldGestureMask());

    const bool playingThis = processor.isStutterActive()
                          && processor.getGestureNote() == stutter::noteForGestureIndex (gestureIndex);
    const float beat = processor.getGestureBeat();

    for (auto& lane : lanes)
        lane->setPlayhead (playingThis, beat);
}

ActionEditorWindow::ActionEditorWindow (StutterCloneAudioProcessor& processor, int gestureIndex)
    : DocumentWindow ("Action Editor", UiColours::panel, DocumentWindow::closeButton),
      editor (processor)
{
    setUsingNativeTitleBar (true);
    setResizable (true, true);
    setResizeLimits (560, 480, 1200, 1400);
    editor.setSize (720, 820);
    setContentNonOwned (&editor, true);
    editor.setGestureIndex (gestureIndex);
    centreWithSize (720, 860);
}

ActionEditorWindow::~ActionEditorWindow()
{
    clearContentComponent();
}

void ActionEditorWindow::closeButtonPressed()
{
    setVisible (false);
}

void ActionEditorWindow::setGestureIndex (int index)
{
    editor.setGestureIndex (index);
    setVisible (true);
    toFront (true);
}
