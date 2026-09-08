#include "NoteKeyboard.h"
#include "UiColours.h"

#include <juce_audio_basics/juce_audio_basics.h>

void NoteKeyboard::setSelectedIndex (int index)
{
    selectedIndex = juce::jlimit (0, stutter::numGestureNotes - 1, index);
    repaint();
}

void NoteKeyboard::setHeldMask (uint16_t mask)
{
    if (mask == heldMask)
        return;

    heldMask = mask;
    repaint();
}

juce::Rectangle<int> NoteKeyboard::whiteKeyBounds (int whiteIndex) const
{
    const int w = getWidth() / whiteCount;
    return { whiteIndex * w, 0, w - 1, getHeight() };
}

juce::Rectangle<int> NoteKeyboard::blackKeyBounds (int gestureIndex) const
{
    const int whiteW = getWidth() / whiteCount;
    const int blackW = juce::jmax (10, whiteW * 2 / 3);
    const int blackH = getHeight() * 3 / 5;

    int whiteBefore = 0;

    switch (gestureIndex)
    {
        case 1:  whiteBefore = 1; break; // C#
        case 3:  whiteBefore = 2; break; // D#
        case 6:  whiteBefore = 4; break; // F#
        case 8:  whiteBefore = 5; break; // G#
        case 10: whiteBefore = 6; break; // A#
        default: return {};
    }

    const int centre = whiteBefore * whiteW;
    return { centre - blackW / 2, 0, blackW, blackH };
}

int NoteKeyboard::hitTestKey (juce::Point<int> pos) const
{
    for (int black : blackNotes)
        if (blackKeyBounds (black).contains (pos))
            return black;

    for (int w = 0; w < whiteCount; ++w)
        if (whiteKeyBounds (w).contains (pos))
            return whiteNotes[w];

    return -1;
}

void NoteKeyboard::paint (juce::Graphics& g)
{
    g.fillAll (UiColours::background);

    for (int w = 0; w < whiteCount; ++w)
    {
        const int note = whiteNotes[w];
        auto bounds = whiteKeyBounds (w).toFloat();
        const bool selected = note == selectedIndex;
        const bool held = (heldMask & (1u << note)) != 0;

        g.setColour (selected ? UiColours::accent : UiColours::keyWhite);
        g.fillRoundedRectangle (bounds.reduced (0.5f), 3.0f);

        if (held)
        {
            g.setColour (UiColours::accent.withAlpha (0.45f));
            g.fillRect (bounds.getX(), bounds.getBottom() - 6.0f, bounds.getWidth(), 6.0f);
        }

        g.setColour (UiColours::grid);
        g.drawRoundedRectangle (bounds, 3.0f, 1.0f);

        g.setColour (selected ? UiColours::background : UiColours::text.withAlpha (0.7f));
        g.setFont (juce::Font { juce::FontOptions { 11.0f, juce::Font::bold } });
        const auto name = juce::MidiMessage::getMidiNoteName (stutter::noteForGestureIndex (note), true, true, 3);
        g.drawText (name, bounds.removeFromBottom (18.0f), juce::Justification::centred);
    }

    for (int black : blackNotes)
    {
        auto bounds = blackKeyBounds (black).toFloat();
        const bool selected = black == selectedIndex;
        const bool held = (heldMask & (1u << black)) != 0;

        g.setColour (selected ? UiColours::accent : UiColours::keyBlack);
        g.fillRoundedRectangle (bounds, 3.0f);

        if (held)
        {
            g.setColour (UiColours::accent);
            g.fillEllipse (bounds.getCentreX() - 3.0f, bounds.getBottom() - 10.0f, 6.0f, 6.0f);
        }

        g.setColour (UiColours::accent.withAlpha (0.35f));
        g.drawRoundedRectangle (bounds, 3.0f, 1.0f);
    }
}

void NoteKeyboard::mouseDown (const juce::MouseEvent& event)
{
    const int note = hitTestKey (event.getPosition());

    if (note < 0)
        return;

    setSelectedIndex (note);

    if (onNoteClicked)
        onNoteClicked (note);
}
