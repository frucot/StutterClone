#include "WaveformDisplay.h"

#include <cmath>

namespace
{
    const juce::Colour background { 0xff12151c };
    const juce::Colour gridColour { 0xff2a3140 };
    const juce::Colour waveColour { 0xff7d8aa3 };
    const juce::Colour loopFill   { 0x336ee7ff };
    const juce::Colour loopEdge   { 0xff6ee7ff };
    const juce::Colour writeColour { 0xffffc857 };
    const juce::Colour playColour { 0xff6ee7ff };
}

WaveformDisplay::WaveformDisplay (StutterCloneAudioProcessor& p)
    : processor (p)
{
    setOpaque (true);
}

void WaveformDisplay::pullSnapshot()
{
    processor.copyWaveformSnapshot (snapshot);
}

void WaveformDisplay::fillWrappedRegion (juce::Graphics& g,
                                         juce::Rectangle<float> area,
                                         float startNorm,
                                         float lengthNorm,
                                         juce::Colour colour) const
{
    if (lengthNorm <= 0.0f)
        return;

    startNorm = juce::jlimit (0.0f, 1.0f, startNorm);
    lengthNorm = juce::jlimit (0.0f, 1.0f, lengthNorm);

    g.setColour (colour);

    const float x0 = area.getX() + startNorm * area.getWidth();
    const float width = lengthNorm * area.getWidth();

    if (startNorm + lengthNorm <= 1.0f)
    {
        g.fillRect (x0, area.getY(), width, area.getHeight());
        return;
    }

    g.fillRect (x0, area.getY(), area.getRight() - x0, area.getHeight());
    g.fillRect (area.getX(), area.getY(), (startNorm + lengthNorm - 1.0f) * area.getWidth(), area.getHeight());
}

void WaveformDisplay::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced (1.0f);
    g.setColour (background);
    g.fillRoundedRectangle (bounds, 6.0f);

    g.setColour (gridColour);
    g.drawHorizontalLine (juce::roundToInt (bounds.getCentreY()), bounds.getX(), bounds.getRight());

    if (! snapshot.valid)
    {
        g.setColour (waveColour.withAlpha (0.45f));
        g.setFont (juce::Font { juce::FontOptions { 13.0f } });
        g.drawText ("Waiting for audio…", bounds, juce::Justification::centred);
        return;
    }

    if (snapshot.loopActive)
        fillWrappedRegion (g, bounds, snapshot.loopStart, snapshot.loopLength, loopFill);

    const float centre = bounds.getCentreY();
    const float halfH = bounds.getHeight() * 0.42f;
    const float binW = bounds.getWidth() / static_cast<float> (StutterCloneAudioProcessor::waveformBins);

    g.setColour (waveColour);

    for (int bin = 0; bin < StutterCloneAudioProcessor::waveformBins; ++bin)
    {
        const float x = bounds.getX() + static_cast<float> (bin) * binW;
        const float yMax = centre - snapshot.maxs[static_cast<size_t> (bin)] * halfH;
        const float yMin = centre - snapshot.mins[static_cast<size_t> (bin)] * halfH;
        g.fillRect (x, juce::jmin (yMax, yMin), juce::jmax (1.0f, binW), juce::jmax (1.0f, std::abs (yMin - yMax)));
    }

    auto drawPlayhead = [&g, &bounds] (float posNorm, juce::Colour colour)
    {
        const float x = bounds.getX() + juce::jlimit (0.0f, 1.0f, posNorm) * bounds.getWidth();
        g.setColour (colour);
        g.drawLine (x, bounds.getY() + 2.0f, x, bounds.getBottom() - 2.0f, 1.5f);
    };

    if (snapshot.loopActive)
    {
        fillWrappedRegion (g, { bounds.getX(), bounds.getY(), bounds.getWidth(), 3.0f },
                           snapshot.loopStart, snapshot.loopLength, loopEdge);
        drawPlayhead (snapshot.playPos, playColour);
    }

    drawPlayhead (snapshot.writePos, writeColour);
}
