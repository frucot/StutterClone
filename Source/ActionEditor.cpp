#include "ActionEditor.h"
#include "UiColours.h"

namespace
{
    constexpr int laneHeight = 44;
    constexpr int gateLaneHeight = 18;
    constexpr int groupHeaderHeight = 22;
    constexpr int columnGap = 10;

    int lanesContentHeight() noexcept
    {
        int yLeft = 0;
        int yRight = 0;
        int curveIndex = 0;

        for (int g = 0; g < stutter::numLaneGroups; ++g)
        {
            int& y = g >= stutter::leftColumnGroups ? yRight : yLeft;
            y += groupHeaderHeight;

            for (int i = 0; i < stutter::laneGroups[g].count; ++i, ++curveIndex)
            {
                const auto curve = static_cast<stutter::Curve> (curveIndex);
                y += (stutter::isGateCurve (curve) ? gateLaneHeight : laneHeight) + 3;
            }

            y += 4;
        }

        return juce::jmax (yLeft, yRight);
    }
}

ActionEditor::ActionEditor (StutterCloneAudioProcessor& p)
    : processor (p)
{
    setOpaque (false);

    gridLabel.setText ("Grid", juce::dontSendNotification);
    gridLabel.setColour (juce::Label::textColourId, UiColours::text.withAlpha (0.7f));
    addAndMakeVisible (gridLabel);

    gridBox.addItem ("4", 4);
    gridBox.addItem ("8", 8);
    gridBox.addItem ("16", 16);
    gridBox.addItem ("32", 32);
    styleCombo (gridBox);
    addAndMakeVisible (gridBox);
    gridBox.onChange = [this]
    {
        stutter::resampleGrid (localAction, gridBox.getSelectedId());
        commitAction();
        reloadFromProcessor();
    };

    filterTypeBox.addItemList (juce::StringArray { "Lowpass", "Highpass", "Bandpass" }, 1);
    styleCombo (filterTypeBox);
    lanesContainer.addAndMakeVisible (filterTypeBox);
    filterTypeBox.onChange = [this]
    {
        localAction.filterType = juce::jlimit (0, 2, filterTypeBox.getSelectedItemIndex());
        commitAction();
    };

    delayDivBox.addItemList (juce::StringArray { "1/4", "1/8", "1/16", "1/32" }, 1);
    styleCombo (delayDivBox);
    lanesContainer.addAndMakeVisible (delayDivBox);
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
    addAndMakeVisible (loopPeriodBox);
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

    styleToggle (pingPongButton);
    pingPongButton.onClick = [this]
    {
        localAction.pingPong = pingPongButton.getToggleState() ? 1 : 0;
        commitAction();
    };

    for (size_t i = 0; i < groupLabels.size(); ++i)
    {
        groupLabels[i].setText (stutter::laneGroups[i].title, juce::dontSendNotification);
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

    filterTypeBox.toFront (false);
    delayDivBox.toFront (false);

    viewport.setViewedComponent (&lanesContainer, false);
    viewport.setScrollBarsShown (true, false);
    viewport.setOpaque (false);
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

    gridBox.setSelectedId (stutter::clampGridResolution (localAction.gridResolution), juce::dontSendNotification);
    filterTypeBox.setSelectedItemIndex (juce::jlimit (0, 2, localAction.filterType), juce::dontSendNotification);
    delayDivBox.setSelectedItemIndex (juce::jlimit (0, 3, localAction.delayDivision), juce::dontSendNotification);
    delayCutButton.setToggleState (localAction.delayCut != 0, juce::dontSendNotification);
    reverbCutButton.setToggleState (localAction.reverbCut != 0, juce::dontSendNotification);
    loopPeriodBox.setSelectedItemIndex (juce::jlimit (0, stutter::numLoopPeriods - 1, localAction.loopPeriod),
                                        juce::dontSendNotification);
    unfreezeButton.setToggleState (localAction.loopUnfreeze != 0, juce::dontSendNotification);
    loopPeriodBox.setEnabled (localAction.loopUnfreeze != 0);
    pingPongButton.setToggleState (localAction.pingPong != 0, juce::dontSendNotification);

    for (int i = 0; i < stutter::numCurves; ++i)
        lanes[static_cast<size_t> (i)]->setAction (&localAction, static_cast<stutter::Curve> (i));

    resized();
    repaint();
}

void ActionEditor::commitAction()
{
    processor.updateAction (gestureIndex, localAction);
}

int ActionEditor::getPreferredHeight() const noexcept
{
    constexpr int controlsHeight = 26;
    constexpr int controlsGap = 8;
    return controlsHeight + controlsGap + lanesContentHeight();
}

void ActionEditor::resized()
{
    auto bounds = getLocalBounds();

    auto controls = bounds.removeFromTop (26);
    gridLabel.setBounds (controls.removeFromLeft (36));
    gridBox.setBounds (controls.removeFromLeft (56));
    controls.removeFromLeft (8);
    pingPongButton.setBounds (controls.removeFromLeft (92));
    controls.removeFromLeft (8);
    unfreezeButton.setBounds (controls.removeFromLeft (120));
    controls.removeFromLeft (8);
    loopPeriodLabel.setBounds (controls.removeFromLeft (36));
    loopPeriodBox.setBounds (controls.removeFromLeft (84));
    controls.removeFromLeft (10);
    delayCutButton.setBounds (controls.removeFromLeft (92));
    controls.removeFromLeft (8);
    reverbCutButton.setBounds (controls.removeFromLeft (100));

    bounds.removeFromTop (8);
    viewport.setBounds (bounds);

    const int availableW = juce::jmax (0, viewport.getWidth() - viewport.getScrollBarThickness() - 2);
    const int colW = juce::jmax (0, (availableW - columnGap) / 2);
    const int rightX = colW + columnGap;
    const auto headerFont = juce::Font { juce::FontOptions { 13.0f, juce::Font::bold } };

    int yLeft = 0;
    int yRight = 0;
    int curveIndex = 0;

    for (size_t g = 0; g < groupLabels.size(); ++g)
    {
        const bool rightCol = g >= static_cast<size_t> (stutter::leftColumnGroups);
        int& y = rightCol ? yRight : yLeft;
        const int x = rightCol ? rightX : 0;
        const int titleW = juce::jmin (colW,
            juce::GlyphArrangement::getStringWidthInt (headerFont, stutter::laneGroups[g].title) + 8);

        groupLabels[g].setBounds (x, y, titleW, groupHeaderHeight);

        if (g == static_cast<size_t> (stutter::filterGroupIndex))
        {
            const int comboW = juce::jlimit (titleW, colW - titleW - 4, titleW + 52);
            filterTypeBox.setBounds (x + titleW, y, comboW, groupHeaderHeight);
        }
        else if (g == static_cast<size_t> (stutter::delayGroupIndex))
        {
            const int comboW = juce::jlimit (titleW, colW - titleW - 4, titleW + 28);
            delayDivBox.setBounds (x + titleW, y, comboW, groupHeaderHeight);
        }

        y += groupHeaderHeight;

        for (int i = 0; i < stutter::laneGroups[g].count; ++i, ++curveIndex)
        {
            const auto curve = static_cast<stutter::Curve> (curveIndex);
            const int height = stutter::isGateCurve (curve) ? gateLaneHeight : laneHeight;
            lanes[static_cast<size_t> (curveIndex)]->setBounds (x, y, colW, height);
            y += height + 3;
        }

        y += 4;
    }

    lanesContainer.setBounds (0, 0, availableW, juce::jmax (yLeft, yRight));
}

void ActionEditor::timerCallback()
{
    const bool playingThis = processor.isStutterActive()
                          && processor.getGestureNote() == stutter::noteForGestureIndex (gestureIndex, processor.getFirstGestureNote());
    const float beat = processor.getGestureBeat();

    for (auto& lane : lanes)
        lane->setPlayhead (playingThis, beat);
}
