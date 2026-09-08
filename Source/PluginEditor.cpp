#include "PluginEditor.h"

namespace
{
    const juce::Colour backgroundColour { 0xff101218 };
    const juce::Colour panelColour     { 0xff1b1f2b };
    const juce::Colour accentColour    { 0xff6ee7ff };
    const juce::Colour inactiveColour  { 0xffff6b7d };
    const juce::Colour textColour      { 0xffe8edf7 };

    const char* divisionName (int index)
    {
        constexpr const char* names[] { "1/4", "1/8", "1/16", "1/32", "1/64" };
        const int safe = juce::jlimit (0, 4, index);
        return names[safe];
    }
}

StutterCloneAudioProcessorEditor::StutterCloneAudioProcessorEditor (StutterCloneAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p), waveformDisplay (p)
{
    processorRef.setEditorOpen (true);
    titleLabel.setText ("StutterClone", juce::dontSendNotification);
    titleLabel.setJustificationType (juce::Justification::centredLeft);
    titleLabel.setColour (juce::Label::textColourId, textColour);
    titleLabel.setFont (juce::Font { juce::FontOptions { 22.0f, juce::Font::bold } });
    addAndMakeVisible (titleLabel);
    addAndMakeVisible (waveformDisplay);

    auto setupCaption = [this] (juce::Label& label, const juce::String& text)
    {
        label.setText (text, juce::dontSendNotification);
        label.setJustificationType (juce::Justification::centredLeft);
        label.setColour (juce::Label::textColourId, textColour.withAlpha (0.65f));
        label.setFont (juce::Font { juce::FontOptions { 13.0f } });
        addAndMakeVisible (label);
    };

    auto setupValue = [this] (juce::Label& label)
    {
        label.setJustificationType (juce::Justification::centred);
        label.setColour (juce::Label::backgroundColourId, panelColour);
        label.setColour (juce::Label::textColourId, accentColour);
        label.setFont (juce::Font { juce::FontOptions { 18.0f, juce::Font::bold } });
        addAndMakeVisible (label);
    };

    setupCaption (bpmTitleLabel, "BPM");
    setupCaption (midiTitleLabel, "MIDI Trigger");
    setupCaption (gestureTitleLabel, "Gesture");
    setupCaption (divisionLabel, "Loop Division (fallback)");
    setupValue (bpmValueLabel);
    setupValue (midiValueLabel);
    setupValue (gestureValueLabel);

    auto& apvts = processorRef.getAPVTS();

    if (auto* choice = dynamic_cast<juce::AudioParameterChoice*> (apvts.getParameter (StutterCloneAudioProcessor::loopDivisionParamId)))
        loopDivisionBox.addItemList (choice->choices, 1);

    if (auto* choice = dynamic_cast<juce::AudioParameterChoice*> (apvts.getParameter (StutterCloneAudioProcessor::filterTypeParamId)))
        filterTypeBox.addItemList (choice->choices, 1);

    if (auto* choice = dynamic_cast<juce::AudioParameterChoice*> (apvts.getParameter (StutterCloneAudioProcessor::delayDivisionParamId)))
        delayTimeBox.addItemList (choice->choices, 1);

    styleCombo (loopDivisionBox);
    styleCombo (filterTypeBox);
    styleCombo (delayTimeBox);

    styleToggle (sweepButton);
    styleToggle (reverseButton);
    styleToggle (panButton);
    styleToggle (filterOnButton);
    styleToggle (loFiOnButton);
    styleToggle (delayOnButton);
    styleToggle (reverbOnButton);
    styleToggle (delayCutButton);
    styleToggle (reverbCutButton);

    setupSlider (filterStartSlider, filterStartLabel, "Start");
    setupSlider (filterEndSlider, filterEndLabel, "End");
    setupSlider (filterResSlider, filterResLabel, "Res");
    filterStartSlider.setTextValueSuffix (" Hz");
    filterEndSlider.setTextValueSuffix (" Hz");

    setupSlider (loFiBitsSlider, loFiBitsLabel, "Bits");
    setupSlider (loFiDownsampleSlider, loFiDownsampleLabel, "Downsample");
    loFiBitsSlider.setNumDecimalPlacesToDisplay (0);
    loFiDownsampleSlider.setNumDecimalPlacesToDisplay (0);

    setupSlider (delayMixSlider, delayMixLabel, "Mix");
    setupSlider (delayFeedbackSlider, delayFeedbackLabel, "Feedback");

    setupSlider (reverbMixSlider, reverbMixLabel, "Mix");
    setupSlider (reverbSizeSlider, reverbSizeLabel, "Size");
    setupSlider (reverbDampSlider, reverbDampLabel, "Damp");

    loopAttachment = std::make_unique<ComboAttachment> (apvts, StutterCloneAudioProcessor::loopDivisionParamId, loopDivisionBox);
    sweepAttachment = std::make_unique<ButtonAttachment> (apvts, StutterCloneAudioProcessor::sweepParamId, sweepButton);
    reverseAttachment = std::make_unique<ButtonAttachment> (apvts, StutterCloneAudioProcessor::reverseParamId, reverseButton);
    panAttachment = std::make_unique<ButtonAttachment> (apvts, StutterCloneAudioProcessor::alternatePanParamId, panButton);

    filterOnAttachment = std::make_unique<ButtonAttachment> (apvts, StutterCloneAudioProcessor::filterOnParamId, filterOnButton);
    filterTypeAttachment = std::make_unique<ComboAttachment> (apvts, StutterCloneAudioProcessor::filterTypeParamId, filterTypeBox);
    filterStartAttachment = std::make_unique<SliderAttachment> (apvts, StutterCloneAudioProcessor::filterCutoffStartParamId, filterStartSlider);
    filterEndAttachment = std::make_unique<SliderAttachment> (apvts, StutterCloneAudioProcessor::filterCutoffEndParamId, filterEndSlider);
    filterResAttachment = std::make_unique<SliderAttachment> (apvts, StutterCloneAudioProcessor::filterResonanceParamId, filterResSlider);

    loFiOnAttachment = std::make_unique<ButtonAttachment> (apvts, StutterCloneAudioProcessor::loFiOnParamId, loFiOnButton);
    loFiBitsAttachment = std::make_unique<SliderAttachment> (apvts, StutterCloneAudioProcessor::loFiBitsParamId, loFiBitsSlider);
    loFiDownsampleAttachment = std::make_unique<SliderAttachment> (apvts, StutterCloneAudioProcessor::loFiDownsampleParamId, loFiDownsampleSlider);

    delayOnAttachment = std::make_unique<ButtonAttachment> (apvts, StutterCloneAudioProcessor::delayOnParamId, delayOnButton);
    delayTimeAttachment = std::make_unique<ComboAttachment> (apvts, StutterCloneAudioProcessor::delayDivisionParamId, delayTimeBox);
    delayMixAttachment = std::make_unique<SliderAttachment> (apvts, StutterCloneAudioProcessor::delayMixParamId, delayMixSlider);
    delayFeedbackAttachment = std::make_unique<SliderAttachment> (apvts, StutterCloneAudioProcessor::delayFeedbackParamId, delayFeedbackSlider);
    delayCutAttachment = std::make_unique<ButtonAttachment> (apvts, StutterCloneAudioProcessor::delayCutParamId, delayCutButton);

    reverbOnAttachment = std::make_unique<ButtonAttachment> (apvts, StutterCloneAudioProcessor::reverbOnParamId, reverbOnButton);
    reverbMixAttachment = std::make_unique<SliderAttachment> (apvts, StutterCloneAudioProcessor::reverbMixParamId, reverbMixSlider);
    reverbSizeAttachment = std::make_unique<SliderAttachment> (apvts, StutterCloneAudioProcessor::reverbSizeParamId, reverbSizeSlider);
    reverbDampAttachment = std::make_unique<SliderAttachment> (apvts, StutterCloneAudioProcessor::reverbDampingParamId, reverbDampSlider);
    reverbCutAttachment = std::make_unique<ButtonAttachment> (apvts, StutterCloneAudioProcessor::reverbCutParamId, reverbCutButton);

    setSize (540, 760);
    updateStatusDisplay();
    startTimerHz (30);
}

StutterCloneAudioProcessorEditor::~StutterCloneAudioProcessorEditor()
{
    stopTimer();
    processorRef.setEditorOpen (false);
}

void StutterCloneAudioProcessorEditor::styleToggle (juce::ToggleButton& button)
{
    button.setColour (juce::ToggleButton::textColourId, textColour);
    button.setColour (juce::ToggleButton::tickColourId, accentColour);
    addAndMakeVisible (button);
}

void StutterCloneAudioProcessorEditor::styleCombo (juce::ComboBox& box)
{
    box.setColour (juce::ComboBox::backgroundColourId, panelColour);
    box.setColour (juce::ComboBox::textColourId, textColour);
    box.setColour (juce::ComboBox::outlineColourId, accentColour.withAlpha (0.35f));
    addAndMakeVisible (box);
}

void StutterCloneAudioProcessorEditor::setupSlider (juce::Slider& slider, juce::Label& label, const juce::String& text)
{
    slider.setSliderStyle (juce::Slider::LinearHorizontal);
    slider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 64, 18);
    slider.setColour (juce::Slider::thumbColourId, accentColour);
    slider.setColour (juce::Slider::trackColourId, accentColour.withAlpha (0.35f));
    slider.setColour (juce::Slider::backgroundColourId, panelColour);
    slider.setColour (juce::Slider::textBoxTextColourId, textColour);
    slider.setColour (juce::Slider::textBoxBackgroundColourId, panelColour);
    slider.setColour (juce::Slider::textBoxOutlineColourId, accentColour.withAlpha (0.25f));
    addAndMakeVisible (slider);

    label.setText (text, juce::dontSendNotification);
    label.setColour (juce::Label::textColourId, textColour.withAlpha (0.7f));
    label.setFont (juce::Font { juce::FontOptions { 12.0f } });
    addAndMakeVisible (label);
}

void StutterCloneAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (backgroundColour);

    g.setColour (panelColour);
    g.fillRect (0, 0, getWidth(), 56);
    g.setColour (accentColour);
    g.fillRect (0, 56, getWidth(), 2);
}

void StutterCloneAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced (16);
    titleLabel.setBounds (bounds.removeFromTop (24));
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

    bounds.removeFromTop (8);
    divisionLabel.setBounds (bounds.removeFromTop (16));
    loopDivisionBox.setBounds (bounds.removeFromTop (24));

    bounds.removeFromTop (8);
    auto toggles = bounds.removeFromTop (22);
    const int toggleW = toggles.getWidth() / 3;
    sweepButton.setBounds (toggles.removeFromLeft (toggleW));
    reverseButton.setBounds (toggles.removeFromLeft (toggleW));
    panButton.setBounds (toggles);

    bounds.removeFromTop (10);
    auto fx = bounds;
    auto left = fx.removeFromLeft ((fx.getWidth() - 12) / 2);
    fx.removeFromLeft (12);
    auto right = fx;

    auto layoutSlider = [] (juce::Rectangle<int>& area, juce::Label& label, juce::Slider& slider)
    {
        auto row = area.removeFromTop (32);
        label.setBounds (row.removeFromLeft (72));
        slider.setBounds (row);
    };

    filterOnButton.setBounds (left.removeFromTop (22));
    filterTypeBox.setBounds (left.removeFromTop (24));
    left.removeFromTop (4);
    layoutSlider (left, filterStartLabel, filterStartSlider);
    layoutSlider (left, filterEndLabel, filterEndSlider);
    layoutSlider (left, filterResLabel, filterResSlider);

    left.removeFromTop (10);
    loFiOnButton.setBounds (left.removeFromTop (22));
    layoutSlider (left, loFiBitsLabel, loFiBitsSlider);
    layoutSlider (left, loFiDownsampleLabel, loFiDownsampleSlider);

    delayOnButton.setBounds (right.removeFromTop (22));
    delayTimeBox.setBounds (right.removeFromTop (24));
    right.removeFromTop (4);
    layoutSlider (right, delayMixLabel, delayMixSlider);
    layoutSlider (right, delayFeedbackLabel, delayFeedbackSlider);
    delayCutButton.setBounds (right.removeFromTop (22));

    right.removeFromTop (10);
    reverbOnButton.setBounds (right.removeFromTop (22));
    layoutSlider (right, reverbMixLabel, reverbMixSlider);
    layoutSlider (right, reverbSizeLabel, reverbSizeSlider);
    layoutSlider (right, reverbDampLabel, reverbDampSlider);
    reverbCutButton.setBounds (right.removeFromTop (22));
}

void StutterCloneAudioProcessorEditor::timerCallback()
{
    waveformDisplay.pullSnapshot();
    waveformDisplay.repaint();
    updateStatusDisplay();
}

void StutterCloneAudioProcessorEditor::updateStatusDisplay()
{
    bpmValueLabel.setText (juce::String (processorRef.getCurrentBpm(), 1), juce::dontSendNotification);

    const bool active = processorRef.isStutterActive();
    midiValueLabel.setText (active ? "Active" : "Inactive", juce::dontSendNotification);
    midiValueLabel.setColour (juce::Label::textColourId, active ? accentColour : inactiveColour);

    const int note = processorRef.getGestureNote();
    const int division = processorRef.getActiveDivisionIndex();

    if (active && note >= 0)
    {
        const auto noteName = juce::MidiMessage::getMidiNoteName (note, true, true, 3);
        gestureValueLabel.setText (noteName + "  ·  " + divisionName (division), juce::dontSendNotification);
        gestureValueLabel.setColour (juce::Label::textColourId, accentColour);
    }
    else
    {
        gestureValueLabel.setText ("GUI  ·  " + juce::String (divisionName (division)), juce::dontSendNotification);
        gestureValueLabel.setColour (juce::Label::textColourId, textColour.withAlpha (0.7f));
    }
}
