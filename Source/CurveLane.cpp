#include "CurveLane.h"
#include "UiColours.h"

void CurveLane::setAction (stutter::Action* actionToEdit, stutter::Curve curveToEdit)
{
    action = actionToEdit;
    curve = curveToEdit;
    lastEditedStep = -1;
    repaint();
}

void CurveLane::setPlayhead (bool shouldShow, float beatInMeasure)
{
    showPlayhead = shouldShow;
    playheadBeat = beatInMeasure;
    repaint();
}

juce::String CurveLane::valueLabel (float norm) const
{
    if (stutter::isDivisionCurve (curve))
        return stutter::divisionNames[stutter::divisionFromNorm (norm)];

    if (stutter::isGateCurve (curve))
        return stutter::gateFromNorm (norm) ? "On" : "Off";

    if (curve == stutter::Curve::FilterCutoff)
        return juce::String (juce::roundToInt (stutter::cutoffHzFromNorm (norm))) + " Hz";

    if (curve == stutter::Curve::FilterResonance)
        return juce::String (stutter::resonanceFromNorm (norm), 2);

    if (curve == stutter::Curve::LoFiBits)
        return juce::String (juce::roundToInt (stutter::bitsFromNorm (norm)));

    if (curve == stutter::Curve::LoFiDownsample)
        return juce::String (stutter::downsampleFromNorm (norm));

    return juce::String (juce::roundToInt (norm * 100.0f)) + "%";
}

float CurveLane::valueFromY (int y) const noexcept
{
    const float n = 1.0f - juce::jlimit (0.0f, 1.0f,
        (static_cast<float> (y) - 2.0f) / juce::jmax (1.0f, static_cast<float> (getHeight() - 4)));

    if (stutter::isGateCurve (curve))
        return n >= 0.5f ? 1.0f : 0.0f;

    if (stutter::isDivisionCurve (curve))
        return stutter::divisionToNorm (stutter::divisionFromDurationNorm (n));

    return n;
}

void CurveLane::applyMouse (juce::Point<int> pos)
{
    if (action == nullptr || getWidth() <= 0)
        return;

    const int steps = stutter::clampGridResolution (action->gridResolution);
    const int step = juce::jlimit (0, steps - 1, pos.x * steps / juce::jmax (1, getWidth()));
    const float value = valueFromY (pos.y);
    action->curves[static_cast<int> (curve)][step] = value;
    lastEditedStep = step;
    repaint();

    if (onChanged)
        onChanged();
}

void CurveLane::mouseDown (const juce::MouseEvent& event)
{
    applyMouse (event.getPosition());
}

void CurveLane::mouseDrag (const juce::MouseEvent& event)
{
    applyMouse (event.getPosition());
}

void CurveLane::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.setColour (UiColours::panel);
    g.fillRoundedRectangle (bounds, 4.0f);

    if (action == nullptr)
        return;

    const int steps = stutter::clampGridResolution (action->gridResolution);
    const float stepW = bounds.getWidth() / static_cast<float> (steps);
    const auto* values = action->curves[static_cast<int> (curve)];

    for (int i = 0; i < steps; ++i)
    {
        const float stored = juce::jlimit (0.0f, 1.0f, values[i]);
        const float n = stutter::isDivisionCurve (curve)
                            ? stutter::durationNormFromDivision (stutter::divisionFromNorm (stored))
                            : stored;
        const float h = juce::jmax (2.0f, n * (bounds.getHeight() - 4.0f));
        const float x = bounds.getX() + static_cast<float> (i) * stepW;
        const float y = bounds.getBottom() - 2.0f - h;

        g.setColour (UiColours::accent.withAlpha (stutter::isGateCurve (curve) && stored < 0.5f ? 0.18f : 0.75f));
        g.fillRect (x + 1.0f, y, stepW - 2.0f, h);

        g.setColour (UiColours::grid);
        g.drawVerticalLine (juce::roundToInt (x), bounds.getY(), bounds.getBottom());
    }

    g.setColour (UiColours::text.withAlpha (0.55f));
    g.setFont (juce::Font { juce::FontOptions { 11.0f } });
    g.drawText (stutter::curveLabel (curve),
                bounds.reduced (6.0f, 2.0f),
                juce::Justification::centredLeft);

    const int labelStep = lastEditedStep >= 0 ? juce::jlimit (0, steps - 1, lastEditedStep) : -1;
    g.setColour (UiColours::text.withAlpha (0.7f));
    g.drawText (labelStep >= 0 ? valueLabel (values[labelStep]) : juce::String(),
                bounds.reduced (6.0f, 2.0f),
                juce::Justification::centredRight);

    if (showPlayhead)
    {
        const float x = bounds.getX() + (playheadBeat / static_cast<float> (stutter::measureBeats)) * bounds.getWidth();
        g.setColour (UiColours::inactive);
        g.drawLine (x, bounds.getY(), x, bounds.getBottom(), 1.5f);
    }
}
