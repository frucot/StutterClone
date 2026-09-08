#pragma once

#include <juce_core/juce_core.h>

#include <array>
#include <cmath>
#include <cstring>

namespace stutter
{
    constexpr int maxSteps = 32;
    constexpr int numGestureNotes = 12;
    constexpr int firstGestureNote = 60; // C3 (middle C = C3)
    constexpr int lastGestureNote = firstGestureNote + numGestureNotes - 1; // B3
    constexpr int numDivisions = 11;
    constexpr int numQuantizeChoices = 5;
    constexpr double measureBeats = 4.0;

    constexpr const char* divisionNames[numDivisions] {
        "1/4", "1/8", "1/16", "1/32", "1/64",
        "1/8T", "1/16T", "1/32T",
        "1/8S", "1/16S", "1/32S"
    };

    constexpr double beatsPerDivision[numDivisions] {
        1.0,        // 1/4
        0.5,        // 1/8
        0.25,       // 1/16
        0.125,      // 1/32
        0.0625,     // 1/64
        1.0 / 3.0,  // 1/8T
        1.0 / 6.0,  // 1/16T
        1.0 / 12.0, // 1/32T
        1.0 / 3.0,  // 1/8S
        1.0 / 6.0,  // 1/16S
        1.0 / 12.0  // 1/32S
    };

    constexpr const char* quantizeNames[numQuantizeChoices] {
        "1/4", "1/8", "1/16", "1/32", "None"
    };

    constexpr double quantizeGridBeats[numQuantizeChoices] {
        1.0, 0.5, 0.25, 0.125, 0.0
    };

    inline bool isQuantizeOff (int index) noexcept
    {
        const int safe = juce::jlimit (0, numQuantizeChoices - 1, index);
        return quantizeGridBeats[safe] <= 0.0;
    }

    constexpr const char* delayDivisionNames[] { "1/4", "1/8", "1/16", "1/32" };

    enum class Curve : int
    {
        Division = 0,
        Reverse,
        AltPan,
        FilterOn,
        FilterCutoff,
        FilterResonance,
        LoFiOn,
        LoFiBits,
        LoFiDownsample,
        DelayOn,
        DelayMix,
        DelayFeedback,
        ReverbOn,
        ReverbMix,
        ReverbSize,
        ReverbDamping,
        Count
    };

    constexpr int numCurves = static_cast<int> (Curve::Count);

    inline bool isGestureNote (int midiNote) noexcept
    {
        return midiNote >= firstGestureNote && midiNote <= lastGestureNote;
    }

    inline int gestureIndexForNote (int midiNote) noexcept
    {
        return midiNote - firstGestureNote;
    }

    inline int noteForGestureIndex (int index) noexcept
    {
        return firstGestureNote + index;
    }

    inline int clampGridResolution (int value) noexcept
    {
        if (value <= 4)  return 4;
        if (value <= 8)  return 8;
        if (value <= 16) return 16;
        return 32;
    }

    inline int stepIndexForBeat (double gestureBeat, int gridResolution) noexcept
    {
        const int steps = clampGridResolution (gridResolution);
        const double wrapped = gestureBeat - measureBeats * std::floor (gestureBeat / measureBeats);
        const double stepBeats = measureBeats / static_cast<double> (steps);
        const int step = static_cast<int> (std::floor (wrapped / stepBeats));
        return juce::jlimit (0, steps - 1, step);
    }

    inline float curveAt (const float* steps, int gridResolution, int step) noexcept
    {
        const int n = clampGridResolution (gridResolution);
        return steps[juce::jlimit (0, n - 1, step)];
    }

    inline int divisionFromNorm (float norm) noexcept
    {
        return juce::jlimit (0, numDivisions - 1,
                             juce::roundToInt (norm * static_cast<float> (numDivisions - 1)));
    }

    inline float divisionToNorm (int index) noexcept
    {
        return static_cast<float> (juce::jlimit (0, numDivisions - 1, index))
             / static_cast<float> (numDivisions - 1);
    }

    // Bottom → top in the editor: longest loop to shortest (straight, then T, then S).
    constexpr int divisionByDuration[numDivisions] {
        0,  // 1/4
        1,  // 1/8
        5,  // 1/8T
        8,  // 1/8S
        2,  // 1/16
        6,  // 1/16T
        9,  // 1/16S
        3,  // 1/32
        7,  // 1/32T
        10, // 1/32S
        4   // 1/64
    };

    inline int durationRankForDivision (int index) noexcept
    {
        const int safe = juce::jlimit (0, numDivisions - 1, index);

        for (int rank = 0; rank < numDivisions; ++rank)
            if (divisionByDuration[rank] == safe)
                return rank;

        return 0;
    }

    inline int divisionFromDurationNorm (float norm) noexcept
    {
        const int rank = juce::jlimit (0, numDivisions - 1,
                                       juce::roundToInt (norm * static_cast<float> (numDivisions - 1)));
        return divisionByDuration[rank];
    }

    inline float durationNormFromDivision (int index) noexcept
    {
        return static_cast<float> (durationRankForDivision (index))
             / static_cast<float> (numDivisions - 1);
    }

    inline bool gateFromNorm (float norm) noexcept
    {
        return norm >= 0.5f;
    }

    inline float cutoffHzFromNorm (float norm) noexcept
    {
        const float n = juce::jlimit (0.0f, 1.0f, norm);
        return 20.0f * std::pow (1000.0f, n);
    }

    inline float cutoffNormFromHz (float hz) noexcept
    {
        const float safe = juce::jmax (20.0f, hz);
        return juce::jlimit (0.0f, 1.0f, std::log (safe / 20.0f) / std::log (1000.0f));
    }

    inline float resonanceFromNorm (float norm) noexcept
    {
        return 0.1f + juce::jlimit (0.0f, 1.0f, norm) * 3.9f;
    }

    inline float resonanceToNorm (float value) noexcept
    {
        return juce::jlimit (0.0f, 1.0f, (value - 0.1f) / 3.9f);
    }

    inline float bitsFromNorm (float norm) noexcept
    {
        return 1.0f + juce::jlimit (0.0f, 1.0f, norm) * 15.0f;
    }

    inline int downsampleFromNorm (float norm) noexcept
    {
        return juce::jlimit (1, 16, 1 + juce::roundToInt (juce::jlimit (0.0f, 1.0f, norm) * 15.0f));
    }

    struct Action
    {
        int gridResolution = 8;
        int filterType = 0;      // 0 LP, 1 HP, 2 BP
        int delayDivision = 1;   // 0=1/4 .. 3=1/32
        int delayCut = 0;
        int reverbCut = 0;
        float curves[numCurves][maxSteps] {};
    };

    inline void fillCurve (Action& action, Curve curve, float value) noexcept
    {
        auto* dest = action.curves[static_cast<int> (curve)];
        for (int i = 0; i < maxSteps; ++i)
            dest[i] = value;
    }

    inline void initActionDefaults (Action& action) noexcept
    {
        action.gridResolution = 8;
        action.filterType = 0;
        action.delayDivision = 1;
        action.delayCut = 0;
        action.reverbCut = 0;

        fillCurve (action, Curve::Division, divisionToNorm (2)); // 1/16
        fillCurve (action, Curve::Reverse, 0.0f);
        fillCurve (action, Curve::AltPan, 0.0f);
        fillCurve (action, Curve::FilterOn, 0.0f);
        fillCurve (action, Curve::FilterCutoff, cutoffNormFromHz (12000.0f));
        fillCurve (action, Curve::FilterResonance, resonanceToNorm (0.707f));
        fillCurve (action, Curve::LoFiOn, 0.0f);
        fillCurve (action, Curve::LoFiBits, 7.0f / 15.0f); // 8 bits
        fillCurve (action, Curve::LoFiDownsample, 0.0f);
        fillCurve (action, Curve::DelayOn, 0.0f);
        fillCurve (action, Curve::DelayMix, 0.35f);
        fillCurve (action, Curve::DelayFeedback, 0.35f / 0.95f);
        fillCurve (action, Curve::ReverbOn, 0.0f);
        fillCurve (action, Curve::ReverbMix, 0.25f);
        fillCurve (action, Curve::ReverbSize, 0.55f);
        fillCurve (action, Curve::ReverbDamping, 0.45f);
    }

    inline void resampleGrid (Action& action, int newResolution) noexcept
    {
        const int oldN = clampGridResolution (action.gridResolution);
        const int newN = clampGridResolution (newResolution);

        if (oldN == newN)
        {
            action.gridResolution = newN;
            return;
        }

        float temp[numCurves][maxSteps] {};

        for (int c = 0; c < numCurves; ++c)
        {
            for (int i = 0; i < newN; ++i)
            {
                const int src = juce::jlimit (0, oldN - 1, (i * oldN) / newN);
                temp[c][i] = action.curves[c][src];
            }

            for (int i = newN; i < maxSteps; ++i)
                temp[c][i] = temp[c][newN - 1];
        }

        std::memcpy (action.curves, temp, sizeof (temp));
        action.gridResolution = newN;
    }

    inline int classicDivisionForGestureIndex (int gestureIndex) noexcept
    {
        switch (juce::jlimit (0, numGestureNotes - 1, gestureIndex))
        {
            case 0:  case 1:  return 1; // C3 / C#3 → 1/8
            case 2:  case 3:  return 2; // D3 / D#3 → 1/16
            case 4:           return 3; // E3       → 1/32
            case 5:  case 6:  return 4; // F3 / F#3 → 1/64
            case 7:           return 0; // G3       → 1/4
            case 8:           return 5; // G#3      → 1/8T
            case 9:           return 6; // A3       → 1/16T
            case 10:          return 7; // A#3      → 1/32T
            default:          return 8; // B3       → 1/8S
        }
    }

    inline Action makeClassicAction (int gestureIndex) noexcept
    {
        Action action;
        initActionDefaults (action);
        fillCurve (action, Curve::Division, divisionToNorm (classicDivisionForGestureIndex (gestureIndex)));
        return action;
    }

    struct EvaluatedStep
    {
        int divisionIndex = 2;
        bool reverse = false;
        bool altPan = false;
        bool filterOn = false;
        float cutoffHz = 12000.0f;
        float resonance = 0.707f;
        bool loFiOn = false;
        float bitDepth = 8.0f;
        int downsample = 1;
        bool delayOn = false;
        float delayMix = 0.35f;
        float delayFeedback = 0.35f;
        bool reverbOn = false;
        float reverbMix = 0.25f;
        float reverbSize = 0.55f;
        float reverbDamping = 0.45f;
    };

    inline EvaluatedStep evaluateAction (const Action& action, double gestureBeat) noexcept
    {
        const int step = stepIndexForBeat (gestureBeat, action.gridResolution);
        const auto at = [&action, step] (Curve curve) noexcept
        {
            return curveAt (action.curves[static_cast<int> (curve)], action.gridResolution, step);
        };

        EvaluatedStep out;
        out.divisionIndex = divisionFromNorm (at (Curve::Division));
        out.reverse = gateFromNorm (at (Curve::Reverse));
        out.altPan = gateFromNorm (at (Curve::AltPan));
        out.filterOn = gateFromNorm (at (Curve::FilterOn));
        out.cutoffHz = cutoffHzFromNorm (at (Curve::FilterCutoff));
        out.resonance = resonanceFromNorm (at (Curve::FilterResonance));
        out.loFiOn = gateFromNorm (at (Curve::LoFiOn));
        out.bitDepth = bitsFromNorm (at (Curve::LoFiBits));
        out.downsample = downsampleFromNorm (at (Curve::LoFiDownsample));
        out.delayOn = gateFromNorm (at (Curve::DelayOn));
        out.delayMix = juce::jlimit (0.0f, 1.0f, at (Curve::DelayMix));
        out.delayFeedback = juce::jlimit (0.0f, 0.95f, at (Curve::DelayFeedback) * 0.95f);
        out.reverbOn = gateFromNorm (at (Curve::ReverbOn));
        out.reverbMix = juce::jlimit (0.0f, 1.0f, at (Curve::ReverbMix));
        out.reverbSize = juce::jlimit (0.0f, 1.0f, at (Curve::ReverbSize));
        out.reverbDamping = juce::jlimit (0.0f, 1.0f, at (Curve::ReverbDamping));
        return out;
    }

    inline const char* curveLabel (Curve curve) noexcept
    {
        switch (curve)
        {
            case Curve::Division:        return "Division";
            case Curve::Reverse:         return "Reverse";
            case Curve::AltPan:          return "Alt Pan";
            case Curve::FilterOn:        return "Filter On";
            case Curve::FilterCutoff:    return "Cutoff";
            case Curve::FilterResonance: return "Resonance";
            case Curve::LoFiOn:          return "Lo-Fi On";
            case Curve::LoFiBits:        return "Bits";
            case Curve::LoFiDownsample:  return "Downsample";
            case Curve::DelayOn:         return "Delay On";
            case Curve::DelayMix:        return "Delay Mix";
            case Curve::DelayFeedback:   return "Feedback";
            case Curve::ReverbOn:        return "Reverb On";
            case Curve::ReverbMix:       return "Reverb Mix";
            case Curve::ReverbSize:      return "Size";
            case Curve::ReverbDamping:   return "Damping";
            case Curve::Count:           break;
        }

        return "";
    }

    inline bool isGateCurve (Curve curve) noexcept
    {
        return curve == Curve::Reverse
            || curve == Curve::AltPan
            || curve == Curve::FilterOn
            || curve == Curve::LoFiOn
            || curve == Curve::DelayOn
            || curve == Curve::ReverbOn;
    }

    inline bool isDivisionCurve (Curve curve) noexcept
    {
        return curve == Curve::Division;
    }

    inline double nextGridPpq (double ppq, double gridBeats) noexcept
    {
        if (gridBeats <= 0.0)
            return ppq;

        const double units = ppq / gridBeats;
        const double next = std::ceil (units - 1.0e-9);
        return next * gridBeats;
    }
}

struct Preset
{
    juce::String name { "Classic" };
    int quantizeIndex = 0; // 1/4
    bool isFactory = true;
    std::array<stutter::Action, stutter::numGestureNotes> actions {};
};

inline Preset makeClassicPreset() noexcept
{
    Preset preset;
    preset.name = "Classic";
    preset.quantizeIndex = 0;
    preset.isFactory = true;

    for (int i = 0; i < stutter::numGestureNotes; ++i)
        preset.actions[static_cast<size_t> (i)] = stutter::makeClassicAction (i);

    return preset;
}
