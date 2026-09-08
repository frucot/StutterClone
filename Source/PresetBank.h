#pragma once

#include "Action.h"

#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>

class PresetBank
{
public:
    PresetBank();

    juce::File getUserPresetsDirectory() const;
    juce::StringArray getPresetNames() const;
    bool isFactoryName (const juce::String& name) const;

    Preset loadPreset (const juce::String& name) const;
    bool saveUserPreset (const Preset& preset) const;
    bool deleteUserPreset (const juce::String& name) const;

    static juce::ValueTree presetToValueTree (const Preset& preset);
    static Preset presetFromValueTree (const juce::ValueTree& tree);

private:
    juce::File userDir;
    juce::File fileForName (const juce::String& name) const;
};
