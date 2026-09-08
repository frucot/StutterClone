#pragma once

#include "Action.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

class CurveLane final : public juce::Component
{
public:
    CurveLane() = default;
    std::function<void()> onChanged;

    void setAction (stutter::Action* actionToEdit, stutter::Curve curveToEdit);
    void setPlayhead (bool shouldShow, float beatInMeasure);

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;

private:
    void applyMouse (juce::Point<int> pos);
    float valueFromY (int y) const noexcept;
    juce::String valueLabel (float norm) const;

    stutter::Action* action = nullptr;
    stutter::Curve curve = stutter::Curve::Division;
    bool showPlayhead = false;
    float playheadBeat = 0.0f;
    int lastEditedStep = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CurveLane)
};
