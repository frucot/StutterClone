#pragma once

#include <juce_core/juce_core.h>

#include <array>
#include <cmath>
#include <cstring>

namespace stutter
{
    constexpr int maxSteps = 32;
    constexpr int numGestureNotes = 12;
    constexpr int defaultFirstGestureNote = 60; // slot 0 = MIDI 60 (C3)
    constexpr int minFirstGestureNote = 0;
    constexpr int maxFirstGestureNote = 127 - numGestureNotes + 1; // 116
    constexpr int noteNameMiddleCOctave = 3; // Ableton: MIDI 60 = C3
    constexpr int numDivisions = 12;
    constexpr int legacyNumDivisions = 11;
    constexpr int currentDivTable = 2;
    constexpr int numQuantizeChoices = 5;
    constexpr double measureBeats = 4.0;

    constexpr int div1Bar = 0;
    constexpr int div1_2 = 1;
    constexpr int div1_4 = 2;
    constexpr int div1_8 = 3;
    constexpr int div1_16 = 4;
    constexpr int div1_32 = 5;
    constexpr int div1_64 = 6;
    constexpr int div1BarT = 7;
    constexpr int div1_2T = 8;
    constexpr int div1_8T = 9;
    constexpr int div1_16T = 10;
    constexpr int div1_32T = 11;

    constexpr const char* divisionNames[numDivisions] {
        "1/1", "1/2", "1/4", "1/8", "1/16", "1/32", "1/64",
        "1/1T", "1/2T", "1/8T", "1/16T", "1/32T"
    };

    constexpr double beatsPerDivision[numDivisions] {
        4.0,            // 1/1
        2.0,            // 1/2
        1.0,            // 1/4
        0.5,            // 1/8
        0.25,           // 1/16
        0.125,          // 1/32
        0.0625,         // 1/64
        8.0 / 3.0,      // 1/1T
        4.0 / 3.0,      // 1/2T
        1.0 / 3.0,      // 1/8T
        1.0 / 6.0,      // 1/16T
        1.0 / 12.0      // 1/32T
    };

    // Old 11-slot table (with sextuplets) -> current indices. S maps to the matching T.
    constexpr int legacyDivisionToCurrent[legacyNumDivisions] {
        div1_4, div1_8, div1_16, div1_32, div1_64,
        div1_8T, div1_16T, div1_32T,
        div1_8T, div1_16T, div1_32T
    };

    static_assert (legacyDivisionToCurrent[0] == div1_4);
    static_assert (legacyDivisionToCurrent[8] == div1_8T);
    static_assert (numDivisions == 12);

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

    constexpr int numLoopPeriods = 5;

    constexpr const char* loopPeriodNames[numLoopPeriods] {
        "1 Beat", "2 Beats", "3 Beats", "1 Bar", "2 Bars"
    };

    constexpr double loopPeriodBeats[numLoopPeriods] {
        1.0, 2.0, 3.0, measureBeats, measureBeats * 2.0
    };

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

    struct LaneGroup
    {
        const char* title;
        int start;
        int count;
    };

    constexpr int numLaneGroups = 5;
    constexpr int leftColumnGroups = 3;
    constexpr int filterGroupIndex = 1;
    constexpr int delayGroupIndex = 3;

    constexpr LaneGroup laneGroups[numLaneGroups] {
        { "Stutter", 0,  3 },
        { "Filter",  3,  3 },
        { "Lo-Fi",   6,  3 },
        { "Delay",   9,  3 },
        { "Reverb",  12, 4 }
    };

    inline int clampFirstGestureNote (int firstNote) noexcept
    {
        int first = juce::jlimit (minFirstGestureNote, maxFirstGestureNote, firstNote);
        first -= first % 12; // keep C (MIDI 60 = C)
        return juce::jlimit (minFirstGestureNote, maxFirstGestureNote, first);
    }

    inline int lastGestureNoteFor (int firstNote) noexcept
    {
        return clampFirstGestureNote (firstNote) + numGestureNotes - 1;
    }

    inline bool isGestureNote (int midiNote, int firstNote) noexcept
    {
        const int first = clampFirstGestureNote (firstNote);
        return midiNote >= first && midiNote <= lastGestureNoteFor (first);
    }

    inline bool isCanonicalSlotNote (int midiNote) noexcept
    {
        return isGestureNote (midiNote, defaultFirstGestureNote);
    }

    inline int gestureIndexForNote (int midiNote, int firstNote) noexcept
    {
        return midiNote - clampFirstGestureNote (firstNote);
    }

    inline int noteForGestureIndex (int index, int firstNote) noexcept
    {
        return clampFirstGestureNote (firstNote) + index;
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

    // Bottom → top in the editor: longest loop to shortest.
    constexpr int divisionByDuration[numDivisions] {
        div1Bar,   // 1/1
        div1BarT,  // 1/1T
        div1_2,    // 1/2
        div1_2T,   // 1/2T
        div1_4,    // 1/4
        div1_8,    // 1/8
        div1_8T,   // 1/8T
        div1_16,   // 1/16
        div1_16T,  // 1/16T
        div1_32,   // 1/32
        div1_32T,  // 1/32T
        div1_64    // 1/64
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

    inline float remapLegacyDivisionNorm (float norm) noexcept
    {
        const int oldIndex = juce::jlimit (0, legacyNumDivisions - 1,
                                           juce::roundToInt (norm * static_cast<float> (legacyNumDivisions - 1)));
        return divisionToNorm (legacyDivisionToCurrent[oldIndex]);
    }

    inline double wrapCycle (double beat, double cycle) noexcept
    {
        if (! std::isfinite (beat) || cycle <= 0.0)
            return 0.0;

        beat = std::fmod (beat, cycle);

        if (beat < 0.0)
            beat += cycle;

        return beat;
    }

    inline double wrapLinearGestureBeat (double beat) noexcept
    {
        return wrapCycle (beat, measureBeats * 2.0);
    }

    inline double pingPongMeasureBeat (double linearBeat) noexcept
    {
        const double cycle = wrapLinearGestureBeat (linearBeat);

        if (cycle < measureBeats)
            return cycle;

        double reversed = 2.0 * measureBeats - cycle;

        if (reversed >= measureBeats)
            reversed = measureBeats * (1.0 - 1.0e-12);

        return reversed;
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
        int loopUnfreeze = 0;    // 0 keeps the capture from the gesture start
        int loopPeriod = 3;      // 0=1 beat .. 4=2 bars
        int pingPong = 0;        // 0 wrap 4 beats, 1 forward then reverse (8-beat cycle)
        float curves[numCurves][maxSteps] {};
    };

    inline double measureBeatForAction (const Action& action, double linearBeat) noexcept
    {
        if (action.pingPong != 0)
            return pingPongMeasureBeat (linearBeat);

        return wrapCycle (linearBeat, measureBeats);
    }

    inline void migrateDivisionCurveIfNeeded (Action& action, int divTable) noexcept
    {
        if (divTable >= currentDivTable)
            return;

        auto* steps = action.curves[static_cast<int> (Curve::Division)];
        const int n = clampGridResolution (action.gridResolution);

        for (int i = 0; i < n; ++i)
            steps[i] = remapLegacyDivisionNorm (steps[i]);

        if (n > 0)
            for (int i = n; i < maxSteps; ++i)
                steps[i] = steps[n - 1];
    }

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
        action.loopUnfreeze = 0;
        action.loopPeriod = 3;
        action.pingPong = 0;

        fillCurve (action, Curve::Division, divisionToNorm (div1_16));
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
            case 0:  case 1:  return div1_8;   // C  / C# → 1/8
            case 2:  case 3:  return div1_16;  // D  / D# → 1/16
            case 4:           return div1_32;  // E       → 1/32
            case 5:  case 6:  return div1_64;  // F  / F# → 1/64
            case 7:           return div1_4;   // G       → 1/4
            case 8:           return div1_8T;  // G#      → 1/8T
            case 9:           return div1_16T; // A       → 1/16T
            case 10:          return div1_32T; // A#      → 1/32T
            default:          return div1Bar;  // B       → 1/1
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
        int divisionIndex = div1_16;
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
