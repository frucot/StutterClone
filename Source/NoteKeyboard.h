#pragma once

#include "Action.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

class NoteKeyboard final : public juce::Component
{
public:
    NoteKeyboard() = default;
    std::function<void (int gestureIndex)> onNoteClicked;

    void setSelectedIndex (int index);
    void setHeldMask (uint16_t mask);
    int getSelectedIndex() const noexcept { return selectedIndex; }

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;

private:
    int hitTestKey (juce::Point<int> pos) const;
    juce::Rectangle<int> whiteKeyBounds (int whiteIndex) const;
    juce::Rectangle<int> blackKeyBounds (int gestureIndex) const;

    int selectedIndex = 0;
    uint16_t heldMask = 0;

    static constexpr int whiteCount = 7;
    static constexpr int whiteNotes[whiteCount] { 0, 2, 4, 5, 7, 9, 11 };
    static constexpr int blackNotes[] { 1, 3, 6, 8, 10 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NoteKeyboard)
};
