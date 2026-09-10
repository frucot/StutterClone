#include "PresetBank.h"

namespace
{
    constexpr const char* rootType = "PRESET";
    constexpr const char* actionType = "ACTION";
    constexpr const char* curveType = "CURVE";

    const char* curveXmlName (stutter::Curve curve) noexcept
    {
        switch (curve)
        {
            case stutter::Curve::Division:        return "division";
            case stutter::Curve::Reverse:         return "reverse";
            case stutter::Curve::AltPan:          return "altPan";
            case stutter::Curve::FilterOn:        return "filterOn";
            case stutter::Curve::FilterCutoff:    return "cutoff";
            case stutter::Curve::FilterResonance: return "resonance";
            case stutter::Curve::LoFiOn:          return "loFiOn";
            case stutter::Curve::LoFiBits:        return "bits";
            case stutter::Curve::LoFiDownsample:  return "downsample";
            case stutter::Curve::DelayOn:         return "delayOn";
            case stutter::Curve::DelayMix:        return "delayMix";
            case stutter::Curve::DelayFeedback:   return "delayFeedback";
            case stutter::Curve::ReverbOn:        return "reverbOn";
            case stutter::Curve::ReverbMix:       return "reverbMix";
            case stutter::Curve::ReverbSize:      return "reverbSize";
            case stutter::Curve::ReverbDamping:   return "reverbDamping";
            case stutter::Curve::Count:           break;
        }

        return "unknown";
    }

    juce::ValueTree actionToTree (const stutter::Action& action, int noteIndex)
    {
        juce::ValueTree tree (actionType);
        tree.setProperty ("note", stutter::noteForGestureIndex (noteIndex, stutter::defaultFirstGestureNote), nullptr);
        tree.setProperty ("grid", action.gridResolution, nullptr);
        tree.setProperty ("filterType", action.filterType, nullptr);
        tree.setProperty ("delayDivision", action.delayDivision, nullptr);
        tree.setProperty ("delayCut", action.delayCut != 0, nullptr);
        tree.setProperty ("reverbCut", action.reverbCut != 0, nullptr);
        tree.setProperty ("loopUnfreeze", action.loopUnfreeze != 0, nullptr);
        tree.setProperty ("loopPeriod", action.loopPeriod, nullptr);
        tree.setProperty ("pingPong", action.pingPong != 0, nullptr);
        tree.setProperty ("divTable", stutter::currentDivTable, nullptr);

        for (int c = 0; c < stutter::numCurves; ++c)
        {
            juce::ValueTree curveTree (curveType);
            curveTree.setProperty ("id", curveXmlName (static_cast<stutter::Curve> (c)), nullptr);

            juce::String values;
            const int steps = stutter::clampGridResolution (action.gridResolution);

            for (int i = 0; i < steps; ++i)
            {
                if (i > 0)
                    values += ",";

                values += juce::String (action.curves[c][i], 5);
            }

            curveTree.setProperty ("values", values, nullptr);
            tree.appendChild (curveTree, nullptr);
        }

        return tree;
    }

    stutter::Action actionFromTree (const juce::ValueTree& tree, int fallbackDivTable)
    {
        stutter::Action action;
        stutter::initActionDefaults (action);

        action.gridResolution = stutter::clampGridResolution (static_cast<int> (tree.getProperty ("grid", 8)));
        action.filterType = juce::jlimit (0, 2, static_cast<int> (tree.getProperty ("filterType", 0)));
        action.delayDivision = juce::jlimit (0, 3, static_cast<int> (tree.getProperty ("delayDivision", 1)));
        action.delayCut = static_cast<bool> (tree.getProperty ("delayCut", false)) ? 1 : 0;
        action.reverbCut = static_cast<bool> (tree.getProperty ("reverbCut", false)) ? 1 : 0;
        action.loopUnfreeze = static_cast<bool> (tree.getProperty ("loopUnfreeze", false)) ? 1 : 0;
        action.loopPeriod = juce::jlimit (0, stutter::numLoopPeriods - 1, static_cast<int> (tree.getProperty ("loopPeriod", 3)));
        action.pingPong = static_cast<bool> (tree.getProperty ("pingPong", false)) ? 1 : 0;
        const int divTable = juce::jlimit (1, stutter::currentDivTable,
                                           static_cast<int> (tree.getProperty ("divTable", fallbackDivTable)));

        for (const auto& child : tree)
        {
            if (! child.hasType (curveType))
                continue;

            const auto id = child.getProperty ("id").toString();
            int curveIndex = -1;

            for (int c = 0; c < stutter::numCurves; ++c)
            {
                if (id == curveXmlName (static_cast<stutter::Curve> (c)))
                {
                    curveIndex = c;
                    break;
                }
            }

            if (curveIndex < 0)
                continue;

            const auto tokens = juce::StringArray::fromTokens (child.getProperty ("values").toString(), ",", "");
            const int count = juce::jmin (stutter::maxSteps, tokens.size());

            for (int i = 0; i < count; ++i)
                action.curves[curveIndex][i] = juce::jlimit (0.0f, 1.0f, tokens[i].getFloatValue());

            if (count > 0)
                for (int i = count; i < stutter::maxSteps; ++i)
                    action.curves[curveIndex][i] = action.curves[curveIndex][count - 1];
        }

        stutter::migrateDivisionCurveIfNeeded (action, divTable);
        return action;
    }
}

PresetBank::PresetBank()
{
    userDir = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                  .getChildFile ("StutterClone")
                  .getChildFile ("Presets");
    userDir.createDirectory();
}

juce::File PresetBank::getUserPresetsDirectory() const
{
    return userDir;
}

bool PresetBank::isFactoryName (const juce::String& name) const
{
    return name == "Classic";
}

juce::File PresetBank::fileForName (const juce::String& name) const
{
    auto safe = juce::File::createLegalFileName (name.trim());

    if (safe.isEmpty())
        safe = "Untitled";

    return userDir.getChildFile (safe + ".xml");
}

juce::StringArray PresetBank::getPresetNames() const
{
    juce::StringArray names;

    for (const auto& file : userDir.findChildFiles (juce::File::findFiles, false, "*.xml"))
    {
        const auto name = file.getFileNameWithoutExtension();

        if (name.isNotEmpty() && ! isFactoryName (name) && ! names.contains (name))
            names.add (name);
    }

    names.sortNatural();
    names.insert (0, "Classic");
    return names;
}

Preset PresetBank::loadPreset (const juce::String& name) const
{
    if (isFactoryName (name) || name.isEmpty())
        return makeClassicPreset();

    const auto file = fileForName (name);

    if (! file.existsAsFile())
        return makeClassicPreset();

    if (auto xml = juce::XmlDocument::parse (file))
    {
        auto tree = juce::ValueTree::fromXml (*xml);
        auto preset = presetFromValueTree (tree);
        preset.name = name;
        preset.isFactory = false;
        return preset;
    }

    return makeClassicPreset();
}

bool PresetBank::saveUserPreset (const Preset& preset) const
{
    if (preset.name.trim().isEmpty() || isFactoryName (preset.name.trim()))
        return false;

    auto toSave = preset;
    toSave.isFactory = false;

    if (auto xml = presetToValueTree (toSave).createXml())
        return xml->writeTo (fileForName (toSave.name));

    return false;
}

bool PresetBank::deleteUserPreset (const juce::String& name) const
{
    if (isFactoryName (name))
        return false;

    return fileForName (name).deleteFile();
}

juce::ValueTree PresetBank::presetToValueTree (const Preset& preset)
{
    juce::ValueTree tree (rootType);
    tree.setProperty ("name", preset.name, nullptr);
    tree.setProperty ("quantize", juce::jlimit (0, stutter::numQuantizeChoices - 1, preset.quantizeIndex), nullptr);
    tree.setProperty ("divTable", stutter::currentDivTable, nullptr);

    for (int i = 0; i < stutter::numGestureNotes; ++i)
        tree.appendChild (actionToTree (preset.actions[static_cast<size_t> (i)], i), nullptr);

    return tree;
}

Preset PresetBank::presetFromValueTree (const juce::ValueTree& tree)
{
    Preset preset = makeClassicPreset();
    preset.isFactory = false;

    if (! tree.hasType (rootType))
        return preset;

    preset.name = tree.getProperty ("name", "Classic").toString();
    preset.quantizeIndex = juce::jlimit (0, stutter::numQuantizeChoices - 1,
                                         static_cast<int> (tree.getProperty ("quantize", 0)));
    const int fallbackDivTable = juce::jlimit (1, stutter::currentDivTable,
                                               static_cast<int> (tree.getProperty ("divTable", 1)));

    for (const auto& child : tree)
    {
        if (! child.hasType (actionType))
            continue;

        const int note = static_cast<int> (child.getProperty ("note", -1));

        if (! stutter::isCanonicalSlotNote (note))
            continue;

        preset.actions[static_cast<size_t> (stutter::gestureIndexForNote (note, stutter::defaultFirstGestureNote))] = actionFromTree (child, fallbackDivTable);
    }

    return preset;
}
