#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <array>
#include <cmath>
#include <vector>

namespace
{
    constexpr std::array<double, StutterCloneAudioProcessor::numDivisions> beatsPerDivision
    {
        1.0,    // 1/4
        0.5,    // 1/8
        0.25,   // 1/16
        0.125,  // 1/32
        0.0625  // 1/64
    };

    int divisionIndexForNote (int note) noexcept
    {
        switch (note)
        {
            case 60: case 61: return 1; // C3 / C#3 → 1/8
            case 62: case 63: return 2; // D3 / D#3 → 1/16
            case 64:          return 3; // E3       → 1/32
            case 65: case 66: return 4; // F3 / F#3 → 1/64
            case 67:          return 0; // G3       → 1/4
            default:          return -1;
        }
    }

    bool paramOn (const std::atomic<float>* param) noexcept
    {
        return param != nullptr && param->load (std::memory_order_relaxed) >= 0.5f;
    }
}

StutterCloneAudioProcessor::StutterCloneAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createParameterLayout())
{
    loopDivisionParam = apvts.getRawParameterValue (loopDivisionParamId);
    sweepParam = apvts.getRawParameterValue (sweepParamId);
    reverseParam = apvts.getRawParameterValue (reverseParamId);
    alternatePanParam = apvts.getRawParameterValue (alternatePanParamId);
    filterOnParam = apvts.getRawParameterValue (filterOnParamId);
    filterTypeParam = apvts.getRawParameterValue (filterTypeParamId);
    filterCutoffStartParam = apvts.getRawParameterValue (filterCutoffStartParamId);
    filterCutoffEndParam = apvts.getRawParameterValue (filterCutoffEndParamId);
    filterResonanceParam = apvts.getRawParameterValue (filterResonanceParamId);
    loFiOnParam = apvts.getRawParameterValue (loFiOnParamId);
    loFiBitsParam = apvts.getRawParameterValue (loFiBitsParamId);
    loFiDownsampleParam = apvts.getRawParameterValue (loFiDownsampleParamId);
    delayOnParam = apvts.getRawParameterValue (delayOnParamId);
    delayMixParam = apvts.getRawParameterValue (delayMixParamId);
    delayDivisionParam = apvts.getRawParameterValue (delayDivisionParamId);
    delayFeedbackParam = apvts.getRawParameterValue (delayFeedbackParamId);
    delayCutParam = apvts.getRawParameterValue (delayCutParamId);
    reverbOnParam = apvts.getRawParameterValue (reverbOnParamId);
    reverbMixParam = apvts.getRawParameterValue (reverbMixParamId);
    reverbSizeParam = apvts.getRawParameterValue (reverbSizeParamId);
    reverbDampingParam = apvts.getRawParameterValue (reverbDampingParamId);
    reverbCutParam = apvts.getRawParameterValue (reverbCutParamId);
}

StutterCloneAudioProcessor::~StutterCloneAudioProcessor() = default;

juce::AudioProcessorValueTreeState::ParameterLayout StutterCloneAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { loopDivisionParamId, 1 },
        "Loop Division",
        juce::StringArray { "1/4", "1/8", "1/16", "1/32", "1/64" },
        2));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { sweepParamId, 1 },
        "Sweep",
        false));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { reverseParamId, 1 },
        "Reverse",
        false));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { alternatePanParamId, 1 },
        "Alt Pan",
        false));

    const juce::NormalisableRange<float> hzRange { 20.0f, 20000.0f, 0.01f, 0.25f };

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { filterOnParamId, 1 }, "Filter", false));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { filterTypeParamId, 1 }, "Filter Type",
        juce::StringArray { "Lowpass", "Highpass", "Bandpass" }, 0));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { filterCutoffStartParamId, 1 }, "Filter Start",
        hzRange, 12000.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { filterCutoffEndParamId, 1 }, "Filter End",
        hzRange, 250.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { filterResonanceParamId, 1 }, "Resonance",
        juce::NormalisableRange<float> { 0.1f, 4.0f, 0.001f, 0.4f }, 0.707f));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { loFiOnParamId, 1 }, "Lo-Fi", false));
    params.push_back (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { loFiBitsParamId, 1 }, "Bit Depth", 1, 16, 8));
    params.push_back (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { loFiDownsampleParamId, 1 }, "Downsample", 1, 16, 1));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { delayOnParamId, 1 }, "Delay", false));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { delayMixParamId, 1 }, "Delay Mix",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.01f }, 0.35f));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { delayDivisionParamId, 1 }, "Delay Time",
        juce::StringArray { "1/4", "1/8", "1/16", "1/32" }, 1));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { delayFeedbackParamId, 1 }, "Delay Feedback",
        juce::NormalisableRange<float> { 0.0f, 0.95f, 0.01f }, 0.35f));
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { delayCutParamId, 1 }, "Delay Cut on Release", false));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { reverbOnParamId, 1 }, "Reverb", false));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { reverbMixParamId, 1 }, "Reverb Mix",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.01f }, 0.25f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { reverbSizeParamId, 1 }, "Reverb Size",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.01f }, 0.55f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { reverbDampingParamId, 1 }, "Reverb Damping",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.01f }, 0.45f));
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { reverbCutParamId, 1 }, "Reverb Cut on Release", false));

    return { params.begin(), params.end() };
}

void StutterCloneAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    crossfadeSamples = juce::jmax (1, juce::roundToInt (sampleRate * crossfadeSeconds));
    ringBufferSize = juce::jmax (1, juce::roundToInt (sampleRate * ringBufferSeconds));

    const int numChannels = juce::jmax (getTotalNumInputChannels(),
                                        getTotalNumOutputChannels(),
                                        2);

    ringBuffer.setSize (numChannels, ringBufferSize, false, false, true);
    ringBuffer.clear();
    dspChain.prepare (sampleRate, samplesPerBlock, numChannels);

    writePosition = 0;
    validSamplesInRing = 0;
    stutterIsOn = false;
    hasWrapped = false;
    reversePlayback = false;
    heldNoteCount = 0;
    stutterReadOffset = 0;
    loopLengthSamples = 0;
    loopStartInRing = 0;
    fadeInRemaining = 0;
    fadeOutRemaining = 0;
    midiDivisionOverride = -1;
    sweepSamplesElapsed = 0;
    sweepLengthSamples = 1;
    loopCycleCount = 0;
    samplesUntilWaveformUpdate = 0;
    waveformUpdateInterval = juce::jmax (256, juce::roundToInt (sampleRate * 0.025));
    waveformSnapshots[0] = {};
    waveformSnapshots[1] = {};
    waveformPublished.store (0, std::memory_order_relaxed);
    resetHeldNotes();
    stutterActive.store (false, std::memory_order_relaxed);
    gestureNote.store (-1, std::memory_order_relaxed);
}

void StutterCloneAudioProcessor::releaseResources()
{
    stutterIsOn = false;
    heldNoteCount = 0;
    fadeInRemaining = 0;
    fadeOutRemaining = 0;
    midiDivisionOverride = -1;
    resetHeldNotes();
    dspChain.reset();
    stutterActive.store (false, std::memory_order_relaxed);
    gestureNote.store (-1, std::memory_order_relaxed);
}

bool StutterCloneAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& mainOut = layouts.getMainOutputChannelSet();
    const auto& mainIn  = layouts.getMainInputChannelSet();

    if (mainOut != juce::AudioChannelSet::mono()
        && mainOut != juce::AudioChannelSet::stereo())
        return false;

    return mainIn == mainOut;
}

void StutterCloneAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                               juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    const int totalNumInputChannels  = getTotalNumInputChannels();
    const int totalNumOutputChannels = getTotalNumOutputChannels();

    for (int channel = totalNumInputChannels; channel < totalNumOutputChannels; ++channel)
        buffer.clear (channel, 0, buffer.getNumSamples());

    capturePlayHead();

    int samplePos = 0;
    const int numSamples = buffer.getNumSamples();

    for (const auto metadata : midiMessages)
    {
        const int eventPos = juce::jlimit (0, numSamples, metadata.samplePosition);

        if (eventPos > samplePos)
            processAudioSlice (buffer, samplePos, eventPos - samplePos);

        handleMidiEvent (metadata.data, metadata.numBytes);
        samplePos = eventPos;
    }

    if (samplePos < numSamples)
        processAudioSlice (buffer, samplePos, numSamples - samplePos);

    midiMessages.clear();

    samplesUntilWaveformUpdate += numSamples;

    if (samplesUntilWaveformUpdate >= waveformUpdateInterval)
    {
        samplesUntilWaveformUpdate = 0;
        publishWaveformSnapshot();
    }
}

void StutterCloneAudioProcessor::capturePlayHead() noexcept
{
    if (auto* playHead = getPlayHead())
    {
        if (const auto position = playHead->getPosition())
        {
            if (const auto bpm = position->getBpm())
                currentBpm.store (static_cast<float> (*bpm), std::memory_order_relaxed);

            if (const auto ppq = position->getPpqPosition())
                ppqPosition.store (*ppq, std::memory_order_relaxed);
        }
    }
}

void StutterCloneAudioProcessor::handleMidiEvent (const juce::uint8* data, int numBytes) noexcept
{
    if (data == nullptr || numBytes < 2)
        return;

    const auto status = static_cast<juce::uint8> (data[0] & 0xf0);
    const int note = static_cast<int> (data[1] & 0x7f);
    const auto velocity = numBytes > 2 ? data[2] : static_cast<juce::uint8> (0);
    const bool isNoteOn = status == 0x90;
    const bool isNoteOff = status == 0x80 || (isNoteOn && velocity == 0);

    if (isNoteOn && velocity > 0)
    {
        if (notesHeld[static_cast<size_t> (note)] == 0)
        {
            notesHeld[static_cast<size_t> (note)] = 1;
            ++heldNoteCount;
        }

        startStutter (note);
        stutterActive.store (stutterIsOn, std::memory_order_relaxed);
        return;
    }

    if (isNoteOff)
    {
        if (notesHeld[static_cast<size_t> (note)] != 0)
        {
            notesHeld[static_cast<size_t> (note)] = 0;
            heldNoteCount = juce::jmax (0, heldNoteCount - 1);
        }

        if (heldNoteCount == 0)
        {
            stopStutter();
            stutterActive.store (false, std::memory_order_relaxed);
            gestureNote.store (-1, std::memory_order_relaxed);
            midiDivisionOverride = -1;
        }
        else if (const int remaining = findHighestHeldNote(); remaining >= 0)
        {
            startStutter (remaining);
            stutterActive.store (stutterIsOn, std::memory_order_relaxed);
        }
    }
}

void StutterCloneAudioProcessor::processAudioSlice (juce::AudioBuffer<float>& buffer,
                                                    int startSample,
                                                    int numSamples) noexcept
{
    if (numSamples <= 0)
        return;

    writeToRingBuffer (buffer, startSample, numSamples);

    if (stutterIsOn || fadeOutRemaining > 0)
    {
    reversePlayback = paramOn (reverseParam);
    const bool altPan = paramOn (alternatePanParam);
    const int numChannels = juce::jmin (buffer.getNumChannels(), ringBuffer.getNumChannels());
    const int fadeNorm = juce::jmax (1, crossfadeSamples);

    for (int i = 0; i < numSamples; ++i)
    {
        float gainStutter = 1.0f;
        float gainLive = 0.0f;

        if (stutterIsOn)
        {
            if (fadeInRemaining > 0)
            {
                gainStutter = 1.0f - (static_cast<float> (fadeInRemaining) / static_cast<float> (fadeNorm));
                gainLive = 1.0f - gainStutter;
                --fadeInRemaining;
            }

            ++sweepSamplesElapsed;
        }
        else if (fadeOutRemaining > 0)
        {
            gainStutter = static_cast<float> (fadeOutRemaining) / static_cast<float> (fadeNorm);
            gainLive = 1.0f - gainStutter;
            --fadeOutRemaining;
        }
        else
        {
            break;
        }

        if (loopLengthSamples < 2)
        {
            advanceLoopReadHead();
            continue;
        }

        float panL = 1.0f;
        float panR = 1.0f;

        if (altPan && numChannels > 1)
        {
            const bool leftCycle = (loopCycleCount & 1) == 0;
            panL = leftCycle ? 1.0f : 0.0f;
            panR = leftCycle ? 0.0f : 1.0f;
        }

        const int sampleIndex = startSample + i;

        for (int channel = 0; channel < numChannels; ++channel)
        {
            const float live = buffer.getSample (channel, sampleIndex);
            const float stutter = readLoopedSample (channel);
            const float pan = channel == 0 ? panL : (channel == 1 ? panR : 1.0f);
            buffer.setSample (channel, sampleIndex, live * gainLive + stutter * gainStutter * pan);
        }

        advanceLoopReadHead();
    }
    }

    processFxSlice (buffer, startSample, numSamples);
}

void StutterCloneAudioProcessor::processFxSlice (juce::AudioBuffer<float>& buffer,
                                                 int startSample,
                                                 int numSamples) noexcept
{
    GestureDspChain::Settings settings;
    settings.feedEffects = stutterIsOn || fadeOutRemaining > 0;
    settings.filterOn = paramOn (filterOnParam);
    settings.filterType = juce::roundToInt (filterTypeParam->load (std::memory_order_relaxed));
    settings.resonance = filterResonanceParam->load (std::memory_order_relaxed);

    const float startHz = juce::jmax (20.0f, filterCutoffStartParam->load (std::memory_order_relaxed));
    const float endHz = juce::jmax (20.0f, filterCutoffEndParam->load (std::memory_order_relaxed));
    const float t = juce::jlimit (0.0f, 1.0f,
        static_cast<float> (sweepSamplesElapsed) / static_cast<float> (juce::jmax (1, sweepLengthSamples)));
    settings.cutoffHz = startHz * std::pow (endHz / startHz, t);

    settings.loFiOn = paramOn (loFiOnParam);
    settings.bitDepth = loFiBitsParam->load (std::memory_order_relaxed);
    settings.downsample = juce::jmax (1, juce::roundToInt (loFiDownsampleParam->load (std::memory_order_relaxed)));

    settings.delayOn = paramOn (delayOnParam);
    settings.delayMix = delayMixParam->load (std::memory_order_relaxed);
    settings.delayFeedback = delayFeedbackParam->load (std::memory_order_relaxed);

    const int delayDivision = juce::jlimit (0, 3, juce::roundToInt (delayDivisionParam->load (std::memory_order_relaxed)));
    const double bpm = juce::jmax (20.0, static_cast<double> (currentBpm.load (std::memory_order_relaxed)));
    settings.delaySamples = static_cast<float> ((beatsPerDivision[static_cast<size_t> (delayDivision)] * 60.0 / bpm) * currentSampleRate);

    settings.reverbOn = paramOn (reverbOnParam);
    settings.reverbMix = reverbMixParam->load (std::memory_order_relaxed);
    settings.reverbSize = reverbSizeParam->load (std::memory_order_relaxed);
    settings.reverbDamping = reverbDampingParam->load (std::memory_order_relaxed);

    dspChain.process (buffer, startSample, numSamples, settings);
}

void StutterCloneAudioProcessor::writeToRingBuffer (const juce::AudioBuffer<float>& buffer,
                                                    int startSample,
                                                    int numSamples) noexcept
{
    if (ringBufferSize <= 0)
        return;

    const int numChannels = juce::jmin (buffer.getNumChannels(), ringBuffer.getNumChannels());
    int remaining = numSamples;
    int inputPos = startSample;

    while (remaining > 0)
    {
        const int chunk = juce::jmin (remaining, ringBufferSize - writePosition);

        for (int channel = 0; channel < numChannels; ++channel)
            ringBuffer.copyFrom (channel, writePosition, buffer, channel, inputPos, chunk);

        writePosition += chunk;

        if (writePosition >= ringBufferSize)
            writePosition = 0;

        inputPos += chunk;
        remaining -= chunk;
    }

    validSamplesInRing = juce::jmin (ringBufferSize, validSamplesInRing + numSamples);
}

void StutterCloneAudioProcessor::startStutter (int midiNote) noexcept
{
    midiDivisionOverride = divisionIndexForNote (midiNote);
    const int division = currentDivisionIndex();
    sweepStartBeats = beatsPerDivision[static_cast<size_t> (division)];
    sweepSamplesElapsed = 0;

    const double bpm = juce::jmax (20.0, static_cast<double> (currentBpm.load (std::memory_order_relaxed)));
    sweepLengthSamples = juce::jmax (1, juce::roundToInt ((sweepDurationBeats * 60.0 / bpm) * currentSampleRate));

    loopLengthSamples = juce::jmin (computeLoopLengthSamples(), validSamplesInRing);

    if (loopLengthSamples < 2)
        return;

    loopStartInRing = writePosition - loopLengthSamples;

    if (loopStartInRing < 0)
        loopStartInRing += ringBufferSize;

    stutterReadOffset = 0;
    hasWrapped = false;
    fadeInRemaining = juce::jmin (crossfadeSamples, loopLengthSamples);
    fadeOutRemaining = 0;
    loopCycleCount = 0;
    stutterIsOn = true;
    dspChain.beginGesture();
    gestureNote.store (midiNote, std::memory_order_relaxed);
    activeDivisionIndex.store (division, std::memory_order_relaxed);
}

void StutterCloneAudioProcessor::stopStutter() noexcept
{
    if (! stutterIsOn)
        return;

    stutterIsOn = false;
    fadeInRemaining = 0;
    fadeOutRemaining = juce::jmin (crossfadeSamples, juce::jmax (1, loopLengthSamples));

    if (paramOn (delayCutParam))
        dspChain.resetDelay();

    if (paramOn (reverbCutParam))
        dspChain.resetReverb();
}

void StutterCloneAudioProcessor::resetHeldNotes() noexcept
{
    notesHeld.fill (0);
    heldNoteCount = 0;
}

int StutterCloneAudioProcessor::findHighestHeldNote() const noexcept
{
    for (int note = 127; note >= 0; --note)
        if (notesHeld[static_cast<size_t> (note)] != 0)
            return note;

    return -1;
}

int StutterCloneAudioProcessor::currentDivisionIndex() const noexcept
{
    if (midiDivisionOverride >= 0)
        return juce::jlimit (0, numDivisions - 1, midiDivisionOverride);

    return juce::jlimit (0, numDivisions - 1,
                         juce::roundToInt (loopDivisionParam->load (std::memory_order_relaxed)));
}

int StutterCloneAudioProcessor::computeLoopLengthSamples() const noexcept
{
    const int divisionIndex = currentDivisionIndex();
    double beats = beatsPerDivision[static_cast<size_t> (divisionIndex)];

    if (paramOn (sweepParam))
    {
        const double t = juce::jlimit (0.0, 1.0,
            static_cast<double> (sweepSamplesElapsed) / static_cast<double> (juce::jmax (1, sweepLengthSamples)));
        const double targetBeats = beatsPerDivision.back();
        beats = sweepStartBeats + (targetBeats - sweepStartBeats) * t;
    }

    const double bpm = juce::jmax (20.0, static_cast<double> (currentBpm.load (std::memory_order_relaxed)));
    const int samples = juce::roundToInt ((beats * 60.0 / bpm) * currentSampleRate);
    return juce::jlimit (2, juce::jmax (2, ringBufferSize), samples);
}

float StutterCloneAudioProcessor::readRingAtLoopOffset (int channel, int offset) const noexcept
{
    int index = loopStartInRing + offset;

    if (index >= ringBufferSize)
        index -= ringBufferSize;

    if (index < 0)
        index += ringBufferSize;

    const int safeChannel = juce::jlimit (0, ringBuffer.getNumChannels() - 1, channel);
    return ringBuffer.getSample (safeChannel, index);
}

float StutterCloneAudioProcessor::readLoopedSample (int channel) const noexcept
{
    const int last = juce::jmax (0, loopLengthSamples - 1);
    const int playOffset = reversePlayback ? (last - stutterReadOffset) : stutterReadOffset;
    float sample = readRingAtLoopOffset (channel, playOffset);

    const int wrapFade = juce::jmin (crossfadeSamples, loopLengthSamples / 4);

    if (hasWrapped && wrapFade > 0 && stutterReadOffset < wrapFade)
    {
        const float fadeIn = static_cast<float> (stutterReadOffset) / static_cast<float> (wrapFade);
        const int tailOffset = reversePlayback
            ? (wrapFade - 1 - stutterReadOffset)
            : (loopLengthSamples - wrapFade + stutterReadOffset);
        const float fromEnd = readRingAtLoopOffset (channel, tailOffset);
        sample = sample * fadeIn + fromEnd * (1.0f - fadeIn);
    }

    return sample;
}

void StutterCloneAudioProcessor::advanceLoopReadHead() noexcept
{
    if (loopLengthSamples < 2)
        return;

    ++stutterReadOffset;

    if (stutterReadOffset >= loopLengthSamples)
    {
        stutterReadOffset = 0;
        hasWrapped = true;
        ++loopCycleCount;
        loopLengthSamples = juce::jmin (computeLoopLengthSamples(), validSamplesInRing);
        loopLengthSamples = juce::jmax (2, loopLengthSamples);
        activeDivisionIndex.store (currentDivisionIndex(), std::memory_order_relaxed);
    }
}

juce::AudioProcessorEditor* StutterCloneAudioProcessor::createEditor()
{
    return new StutterCloneAudioProcessorEditor (*this);
}

bool StutterCloneAudioProcessor::hasEditor() const { return true; }
const juce::String StutterCloneAudioProcessor::getName() const { return JucePlugin_Name; }
bool StutterCloneAudioProcessor::acceptsMidi() const { return true; }
bool StutterCloneAudioProcessor::producesMidi() const { return false; }
bool StutterCloneAudioProcessor::isMidiEffect() const { return false; }
double StutterCloneAudioProcessor::getTailLengthSeconds() const { return 2.0; }
int StutterCloneAudioProcessor::getNumPrograms() { return 1; }
int StutterCloneAudioProcessor::getCurrentProgram() { return 0; }
void StutterCloneAudioProcessor::setCurrentProgram (int) {}
const juce::String StutterCloneAudioProcessor::getProgramName (int) { return {}; }
void StutterCloneAudioProcessor::changeProgramName (int, const juce::String&) {}

void StutterCloneAudioProcessor::setEditorOpen (bool shouldBeOpen) noexcept
{
    editorOpen.store (shouldBeOpen, std::memory_order_relaxed);
}

void StutterCloneAudioProcessor::copyWaveformSnapshot (WaveformSnapshot& dest) const noexcept
{
    const int index = waveformPublished.load (std::memory_order_acquire);
    dest = waveformSnapshots[static_cast<size_t> (juce::jlimit (0, 1, index))];
}

void StutterCloneAudioProcessor::publishWaveformSnapshot() noexcept
{
    if (! editorOpen.load (std::memory_order_relaxed))
        return;

    const int size = ringBufferSize;

    if (size <= 1)
        return;

    const int dest = 1 - waveformPublished.load (std::memory_order_relaxed);
    auto& snap = waveformSnapshots[static_cast<size_t> (dest)];
    const int channels = juce::jmin (2, ringBuffer.getNumChannels());

    for (int bin = 0; bin < waveformBins; ++bin)
    {
        const int start = static_cast<int> ((static_cast<juce::int64> (bin) * size) / waveformBins);
        const int end = static_cast<int> ((static_cast<juce::int64> (bin + 1) * size) / waveformBins);
        const int span = juce::jmax (1, end - start);
        const int stride = juce::jmax (1, span / 16);

        float mn = 1.0f;
        float mx = -1.0f;

        for (int i = start; i < end; i += stride)
        {
            float sample = ringBuffer.getSample (0, i);

            if (channels > 1)
                sample = 0.5f * (sample + ringBuffer.getSample (1, i));

            mn = juce::jmin (mn, sample);
            mx = juce::jmax (mx, sample);
        }

        snap.mins[static_cast<size_t> (bin)] = mn;
        snap.maxs[static_cast<size_t> (bin)] = mx;
    }

    const float inv = 1.0f / static_cast<float> (size);
    int playIndex = loopStartInRing + stutterReadOffset;

    if (playIndex >= size)
        playIndex -= size;

    snap.writePos = static_cast<float> (writePosition) * inv;
    snap.loopStart = static_cast<float> (loopStartInRing) * inv;
    snap.loopLength = static_cast<float> (juce::jmax (0, loopLengthSamples)) * inv;
    snap.playPos = static_cast<float> (playIndex) * inv;
    snap.loopActive = stutterIsOn;
    snap.valid = validSamplesInRing > 1;

    waveformPublished.store (dest, std::memory_order_release);
}

void StutterCloneAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void StutterCloneAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new StutterCloneAudioProcessor();
}
