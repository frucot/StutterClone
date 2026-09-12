#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <cmath>
#include <vector>

namespace
{
    void copyActions (std::array<stutter::Action, stutter::numGestureNotes>& dest,
                      const std::array<stutter::Action, stutter::numGestureNotes>& src) noexcept
    {
        dest = src;
    }

    int wrapRingIndex (int index, int size) noexcept
    {
        if (size <= 0)
            return 0;

        if (index >= size)
            index -= size;

        if (index < 0)
            index += size;

        return juce::jlimit (0, size - 1, index);
    }
}

StutterCloneAudioProcessor::StutterCloneAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createParameterLayout()),
      workingPreset (makeClassicPreset())
{
    quantizeParam = apvts.getRawParameterValue (quantizeParamId);
    copyActions (rtActions[0], workingPreset.actions);
    copyActions (rtActions[1], workingPreset.actions);
    stutter::initActionDefaults (playingAction);
}

StutterCloneAudioProcessor::~StutterCloneAudioProcessor() = default;

juce::AudioProcessorValueTreeState::ParameterLayout StutterCloneAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    juce::StringArray quantizeChoices;
    for (int i = 0; i < stutter::numQuantizeChoices; ++i)
        quantizeChoices.add (stutter::quantizeNames[i]);

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { quantizeParamId, 2 },
        "Quantize",
        quantizeChoices,
        0));

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
    clampedBpm = 120.0;
    hasSyncedPpq = false;
    stutterIsOn = false;
    reversePlayback = false;
    pendingArmed = false;
    stutterReadOffset = 0;
    loopLengthSamples = 0;
    loopStartInRing = 0;
    seamMargin = 0;
    seamOffset = 0;
    seamLength = 0;
    seamRemaining = 0;
    seamReverse = false;
    panGainL = 1.0f;
    panGainR = 1.0f;
    fadeInRemaining = 0;
    fadeOutRemaining = 0;
    loopCycleCount = 0;
    pendingNote = -1;
    playingNote = -1;
    lastStepIndex = -1;
    gestureBeat = 0.0;
    beatsSinceCapture = 0.0;
    samplesUntilWaveformUpdate = 0;
    waveformUpdateInterval = juce::jmax (256, juce::roundToInt (sampleRate * 0.025));
    waveformSnapshots[0] = {};
    waveformSnapshots[1] = {};
    waveformPublished.store (0, std::memory_order_relaxed);
    waveformSequence.store (waveformSequence.load (std::memory_order_relaxed) + 1u, std::memory_order_release);
    resetHeldNotes();
    cancelPending();
    stutterActive.store (false, std::memory_order_relaxed);
    gestureNote.store (-1, std::memory_order_relaxed);
    activeStep.store (0, std::memory_order_relaxed);
    gestureBeatAtomic.store (0.0f, std::memory_order_relaxed);
    audioFirstGestureNote = currentFirstGestureNote();
}

void StutterCloneAudioProcessor::releaseResources()
{
    clampedBpm = 120.0;
    hasSyncedPpq = false;
    stutterIsOn = false;
    fadeInRemaining = 0;
    fadeOutRemaining = 0;
    seamRemaining = 0;
    beatsSinceCapture = 0.0;
    resetHeldNotes();
    cancelPending();
    dspChain.reset();
    stutterActive.store (false, std::memory_order_relaxed);
    gestureNote.store (-1, std::memory_order_relaxed);
}

bool StutterCloneAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.inputBuses.size() != 1 || layouts.outputBuses.size() != 1)
        return false;

    const auto& mainIn  = layouts.getMainInputChannelSet();
    const auto& mainOut = layouts.getMainOutputChannelSet();

    if (mainIn.isDisabled() || mainOut.isDisabled())
        return false;

    if (mainIn != mainOut)
        return false;

    return mainIn == juce::AudioChannelSet::mono()
        || mainIn == juce::AudioChannelSet::stereo();
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
    applyGestureOctaveIfChanged();
    applyHeldMask();

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
    ppqPosition.store (ppqCursor, std::memory_order_relaxed);

    samplesUntilWaveformUpdate += numSamples;

    if (samplesUntilWaveformUpdate >= waveformUpdateInterval)
    {
        samplesUntilWaveformUpdate = 0;
        publishWaveformSnapshot();
    }
}

void StutterCloneAudioProcessor::capturePlayHead() noexcept
{
    bool usedHostPpq = false;

    if (auto* playHead = getPlayHead())
    {
        if (const auto position = playHead->getPosition())
        {
            if (const auto bpm = position->getBpm())
            {
                const double hostBpm = std::isfinite (*bpm) ? *bpm : 120.0;
                currentBpm.store (static_cast<float> (hostBpm), std::memory_order_relaxed);
                clampedBpm = juce::jlimit (20.0, 999.0, hostBpm);
            }

            if (const auto ppq = position->getPpqPosition())
            {
                // Resync only while the transport advances, otherwise a frozen host ppq would stall pending gestures.
                if (std::isfinite (*ppq) && (position->getIsPlaying() || ! hasSyncedPpq))
                {
                    ppqCursor = *ppq;
                    hasSyncedPpq = true;
                    usedHostPpq = true;
                }
            }
        }
    }

    ppqPerSample = (clampedBpm / 60.0) / juce::jmax (1.0, currentSampleRate);

    if (! std::isfinite (ppqPerSample) || ppqPerSample <= 0.0)
        ppqPerSample = 0.0;

    if (! usedHostPpq)
    {
        const double fallback = ppqPosition.load (std::memory_order_relaxed);
        ppqCursor = std::isfinite (fallback) ? fallback : 0.0;
    }
}

int StutterCloneAudioProcessor::currentFirstGestureNote() const noexcept
{
    return stutter::clampFirstGestureNote (firstGestureNote.load (std::memory_order_relaxed));
}

void StutterCloneAudioProcessor::applyGestureOctaveIfChanged() noexcept
{
    const int first = currentFirstGestureNote();

    if (first == audioFirstGestureNote)
        return;

    audioFirstGestureNote = first;
    resetHeldNotes();
    cancelPending();

    if (stutterIsOn)
        stopStutter();

    stutterActive.store (stutterIsOn, std::memory_order_relaxed);
    gestureNote.store (-1, std::memory_order_relaxed);
}

int StutterCloneAudioProcessor::currentQuantizeIndex() const noexcept
{
    if (quantizeParam == nullptr)
        return 0;

    return juce::jlimit (0, stutter::numQuantizeChoices - 1,
                         juce::roundToInt (quantizeParam->load (std::memory_order_relaxed)));
}

const stutter::Action& StutterCloneAudioProcessor::rtActionForNote (int midiNote) const noexcept
{
    const int dest = juce::jlimit (0, 1, rtActionIndex.load (std::memory_order_acquire));
    const int first = currentFirstGestureNote();
    const int index = juce::jlimit (0, stutter::numGestureNotes - 1,
                                    stutter::gestureIndexForNote (midiNote, first));
    return rtActions[static_cast<size_t> (dest)][static_cast<size_t> (index)];
}

void StutterCloneAudioProcessor::armPending (int midiNote) noexcept
{
    if (stutter::isQuantizeOff (currentQuantizeIndex()))
    {
        cancelPending();
        startStutter (midiNote);
        stutterActive.store (stutterIsOn, std::memory_order_relaxed);
        return;
    }

    pendingNote = midiNote;
    pendingArmed = true;
    pendingGridPpq = stutter::nextGridPpq (ppqCursor,
                                           stutter::quantizeGridBeats[static_cast<size_t> (currentQuantizeIndex())]);
    pendingNoteAtomic.store (midiNote, std::memory_order_relaxed);
    gesturePending.store (true, std::memory_order_relaxed);
}

void StutterCloneAudioProcessor::cancelPending() noexcept
{
    pendingArmed = false;
    pendingNote = -1;
    pendingNoteAtomic.store (-1, std::memory_order_relaxed);
    gesturePending.store (false, std::memory_order_relaxed);
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

    if (! stutter::isGestureNote (note, currentFirstGestureNote()))
        return;

    const int gestureIndex = stutter::gestureIndexForNote (note, currentFirstGestureNote());
    const auto bit = static_cast<uint16_t> (1u << gestureIndex);
    uint16_t mask = midiHeldGestureMask.load (std::memory_order_relaxed);

    if (isNoteOn && velocity > 0)
        mask = static_cast<uint16_t> (mask | bit);
    else if (isNoteOff)
        mask = static_cast<uint16_t> (mask & static_cast<uint16_t> (~bit));
    else
        return;

    midiHeldGestureMask.store (mask, std::memory_order_relaxed);
    applyHeldMask();
}

void StutterCloneAudioProcessor::setUiGestureHeld (int gestureIndex, bool held) noexcept
{
    const int index = juce::jlimit (0, stutter::numGestureNotes - 1, gestureIndex);
    const auto bit = static_cast<uint16_t> (1u << index);
    uint16_t mask = uiHeldGestureMask.load (std::memory_order_relaxed);

    for (;;)
    {
        const uint16_t next = held ? static_cast<uint16_t> (mask | bit)
                                   : static_cast<uint16_t> (mask & static_cast<uint16_t> (~bit));

        if (next == mask)
            return;

        if (uiHeldGestureMask.compare_exchange_weak (mask, next, std::memory_order_relaxed))
            return;
    }
}

void StutterCloneAudioProcessor::applyHeldMask() noexcept
{
    const uint16_t now = static_cast<uint16_t> (midiHeldGestureMask.load (std::memory_order_relaxed)
                                             | uiHeldGestureMask.load (std::memory_order_relaxed));
    const uint16_t prev = lastCombinedHeldMask;

    if (now == prev)
        return;

    const uint16_t added = static_cast<uint16_t> (now & ~prev);
    const uint16_t removed = static_cast<uint16_t> (prev & ~now);
    lastCombinedHeldMask = now;
    heldGestureMask.store (now, std::memory_order_relaxed);

    if (added != 0)
    {
        const int highest = findHighestHeldGestureNote();

        if (highest >= 0)
        {
            if (stutterIsOn)
                startStutter (highest);
            else
                armPending (highest);
        }
    }

    if (removed != 0)
    {
        if (now == 0)
        {
            cancelPending();
            stopStutter();
            stutterActive.store (false, std::memory_order_relaxed);
            gestureNote.store (-1, std::memory_order_relaxed);
            playingNote = -1;
        }
        else if (const int remaining = findHighestHeldGestureNote(); remaining >= 0)
        {
            if (stutterIsOn)
                startStutter (remaining);
            else
                armPending (remaining);
        }
    }
}

void StutterCloneAudioProcessor::processAudioSlice (juce::AudioBuffer<float>& buffer,
                                                    int startSample,
                                                    int numSamples) noexcept
{
    if (numSamples <= 0)
        return;

    int offset = 0;
    int remaining = numSamples;

    // Two passes suffice: the first trigger disarms the pending gesture, so the second can never split again.
    for (int pass = 0; remaining > 0 && pass < 2; ++pass)
    {
        int split = remaining;

        if (pendingArmed && pendingNote >= 0 && ppqPerSample > 0.0
            && std::isfinite (ppqPerSample) && std::isfinite (ppqCursor)
            && std::isfinite (pendingGridPpq))
        {
            if (ppqCursor >= pendingGridPpq)
            {
                split = 0;
            }
            else
            {
                const double samplesAway = (pendingGridPpq - ppqCursor) / ppqPerSample;

                if (! std::isfinite (samplesAway) || samplesAway <= 0.0)
                    split = 0;
                else if (samplesAway >= static_cast<double> (remaining))
                    split = remaining;
                else
                    split = juce::jlimit (0, remaining, juce::roundToInt (samplesAway));
            }
        }

        if (pendingArmed && pendingNote >= 0 && split < remaining)
        {
            if (split > 0)
            {
                renderAudioSlice (buffer, startSample + offset, split);
                ppqCursor += static_cast<double> (split) * ppqPerSample;
                offset += split;
                remaining -= split;
            }

            const int noteToStart = pendingNote;
            cancelPending();
            startStutter (noteToStart);
            stutterActive.store (stutterIsOn, std::memory_order_relaxed);
            continue;
        }

        renderAudioSlice (buffer, startSample + offset, remaining);
        ppqCursor += static_cast<double> (remaining) * ppqPerSample;
        remaining = 0;
        break;
    }
}

void StutterCloneAudioProcessor::renderAudioSlice (juce::AudioBuffer<float>& buffer,
                                                   int startSample,
                                                   int numSamples) noexcept
{
    if (numSamples <= 0)
        return;

    writeToRingBuffer (buffer, startSample, numSamples);

    if (stutterIsOn && playingNote >= 0)
        playingAction = rtActionForNote (playingNote);

    if (stutterIsOn || fadeOutRemaining > 0)
    {
        const int numChannels = juce::jmin (buffer.getNumChannels(), ringBuffer.getNumChannels());
        const int fadeNorm = juce::jmax (1, crossfadeSamples);
        const double beatsPerSample = std::isfinite (ppqPerSample) ? juce::jmax (0.0, ppqPerSample) : 0.0;

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

                gestureBeat = stutter::wrapLinearGestureBeat (gestureBeat + beatsPerSample);
                const double readBeat = stutter::measureBeatForAction (playingAction, gestureBeat);

                if (playingAction.loopUnfreeze != 0)
                {
                    const auto periodIndex = static_cast<size_t> (juce::jlimit (0, stutter::numLoopPeriods - 1, playingAction.loopPeriod));
                    const double period = stutter::loopPeriodBeats[periodIndex];
                    beatsSinceCapture += beatsPerSample;

                    if (beatsSinceCapture >= period)
                    {
                        beatsSinceCapture -= period;
                        recaptureLoop();
                    }
                }

                const auto step = stutter::evaluateAction (playingAction, readBeat);
                const int stepIndex = stutter::stepIndexForBeat (readBeat, playingAction.gridResolution);

                if (stepIndex != lastStepIndex)
                {
                    applyEvaluatedStep (step);
                    lastStepIndex = stepIndex;
                }

                reversePlayback = step.reverse;
                activeStep.store (stepIndex, std::memory_order_relaxed);
                gestureBeatAtomic.store (static_cast<float> (readBeat), std::memory_order_relaxed);
                activeDivisionIndex.store (step.divisionIndex, std::memory_order_relaxed);
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

            const auto step = stutter::evaluateAction (playingAction,
                                                       stutter::measureBeatForAction (playingAction, gestureBeat));
            float targetL = 1.0f;
            float targetR = 1.0f;

            if (step.altPan && numChannels > 1)
            {
                const bool leftCycle = (loopCycleCount & 1) == 0;
                targetL = leftCycle ? 1.0f : 0.0f;
                targetR = leftCycle ? 0.0f : 1.0f;
            }

            // Slew the alternating pan over the crossfade time: switching it on a sample boundary
            // gates the channel and clicks on its own, regardless of the loop seam.
            const float panStep = 1.0f / static_cast<float> (fadeNorm);
            panGainL += juce::jlimit (-panStep, panStep, targetL - panGainL);
            panGainR += juce::jlimit (-panStep, panStep, targetR - panGainR);

            const int sampleIndex = startSample + i;

            for (int channel = 0; channel < numChannels; ++channel)
            {
                const float live = buffer.getSample (channel, sampleIndex);
                const float stutterSample = readLoopedSample (channel);
                const float pan = channel == 0 ? panGainL : (channel == 1 ? panGainR : 1.0f);
                buffer.setSample (channel, sampleIndex, live * gainLive + stutterSample * gainStutter * pan);
            }

            advanceLoopReadHead();
        }
    }

    processFxSlice (buffer, startSample, numSamples);
}

void StutterCloneAudioProcessor::applyEvaluatedStep (const stutter::EvaluatedStep& step) noexcept
{
    const int previousLength = loopLengthSamples;
    activeDivisionIndex.store (step.divisionIndex, std::memory_order_relaxed);
    loopLengthSamples = clampLoopLength (computeLoopLengthSamples());

    if (previousLength > 0 && stutterReadOffset >= loopLengthSamples)
    {
        // A shorter division drops the read head outside the loop. Keep the existing modulo
        // placement, but splice from where the head was heading so the jump is cross-faded.
        const int lastBefore = juce::jmax (0, previousLength - 1);
        const int playBefore = reversePlayback ? (lastBefore - stutterReadOffset) : stutterReadOffset;

        stutterReadOffset %= loopLengthSamples;
        beginSeam (playBefore + (reversePlayback ? -1 : 1), reversePlayback);
    }
}

void StutterCloneAudioProcessor::processFxSlice (juce::AudioBuffer<float>& buffer,
                                                 int startSample,
                                                 int numSamples) noexcept
{
    const auto step = stutter::evaluateAction (playingAction,
                                               stutter::measureBeatForAction (playingAction, gestureBeat));
    const int delayDiv = juce::jlimit (0, 3, playingAction.delayDivision);

    GestureDspChain::Settings settings;
    settings.feedEffects = stutterIsOn || fadeOutRemaining > 0;
    settings.granularOn = step.granularOn;
    settings.granularMidiNote = step.granularMidiNote;
    settings.granularMix = juce::jlimit (0.0f, 1.0f, playingAction.granularMix);
    settings.granularEngine = juce::jlimit (0, 1, playingAction.granularEngine);
    settings.fuzzGain = step.fuzzGain;
    settings.filterOn = step.filterOn;
    settings.filterType = juce::jlimit (0, 2, playingAction.filterType);
    settings.cutoffHz = step.cutoffHz;
    settings.resonance = step.resonance;
    settings.loFiOn = step.loFiOn;
    settings.bitDepth = step.bitDepth;
    settings.downsample = step.downsample;
    settings.delayOn = step.delayOn;
    settings.delayMix = step.delayMix;
    settings.delayFeedback = step.delayFeedback;
    settings.delaySamples = static_cast<float> ((stutter::beatsPerDivision[static_cast<size_t> (delayDiv)] * 60.0 / clampedBpm) * currentSampleRate);
    settings.reverbOn = step.reverbOn;
    settings.reverbMix = step.reverbMix;
    settings.reverbSize = step.reverbSize;
    settings.reverbDamping = step.reverbDamping;

    dspChain.process (buffer, startSample, numSamples, settings);
}

void StutterCloneAudioProcessor::writeToRingBuffer (const juce::AudioBuffer<float>& buffer,
                                                    int startSample,
                                                    int numSamples) noexcept
{
    if (ringBufferSize <= 0 || numSamples <= 0)
        return;

    const int bufferSamples = buffer.getNumSamples();

    if (startSample < 0 || startSample >= bufferSamples)
        return;

    numSamples = juce::jmin (numSamples, bufferSamples - startSample);

    const int numChannels = juce::jmin (buffer.getNumChannels(), ringBuffer.getNumChannels());

    if (numChannels <= 0)
        return;

    if (writePosition < 0 || writePosition >= ringBufferSize)
        writePosition = 0;

    int remaining = numSamples;
    int inputPos = startSample;

    while (remaining > 0)
    {
        const int chunk = juce::jmin (remaining, ringBufferSize - writePosition);

        if (chunk <= 0)
        {
            if (writePosition == 0)
                break;

            writePosition = 0;
            continue;
        }

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
    if (! stutter::isGestureNote (midiNote, currentFirstGestureNote()))
        return;

    playingAction = rtActionForNote (midiNote);
    playingNote = midiNote;
    gestureBeat = 0.0;
    beatsSinceCapture = 0.0;
    lastStepIndex = -1;

    // A new gesture must not inherit the previous loop, or applyEvaluatedStep would splice a
    // seam from a read position that belongs to a capture that is already gone.
    loopLengthSamples = 0;
    stutterReadOffset = 0;
    seamRemaining = 0;

    const auto step = stutter::evaluateAction (playingAction, 0.0);
    applyEvaluatedStep (step);
    reversePlayback = step.reverse;

    if (! captureLoopRegion())
        return;

    panGainL = 1.0f;
    panGainR = 1.0f;
    fadeInRemaining = juce::jmin (crossfadeSamples, loopLengthSamples);
    fadeOutRemaining = 0;
    loopCycleCount = 0;
    stutterIsOn = true;
    dspChain.beginGesture();
    gestureNote.store (midiNote, std::memory_order_relaxed);
    activeDivisionIndex.store (step.divisionIndex, std::memory_order_relaxed);
    activeStep.store (0, std::memory_order_relaxed);
    gestureBeatAtomic.store (0.0f, std::memory_order_relaxed);
}

void StutterCloneAudioProcessor::stopStutter() noexcept
{
    if (! stutterIsOn)
        return;

    stutterIsOn = false;
    fadeInRemaining = 0;
    fadeOutRemaining = juce::jmin (crossfadeSamples, juce::jmax (1, loopLengthSamples));

    if (playingAction.delayCut != 0)
        dspChain.resetDelay();

    if (playingAction.reverbCut != 0)
        dspChain.resetReverb();
}

void StutterCloneAudioProcessor::recaptureLoop() noexcept
{
    // Absolute ring position the outgoing capture was heading for, resolved before the new one
    // moves loopStartInRing under our feet.
    const int previousLast = juce::jmax (0, loopLengthSamples - 1);
    const int previousPlay = reversePlayback ? (previousLast - stutterReadOffset) : stutterReadOffset;
    const int continuation = loopStartInRing + previousPlay + (reversePlayback ? -1 : 1);

    if (! captureLoopRegion())
        return;

    beginSeam (continuation - loopStartInRing, reversePlayback);
}

bool StutterCloneAudioProcessor::captureLoopRegion() noexcept
{
    if (validSamplesInRing < 2)
        return false;

    // A quarter of the valid audio keeps the margin small enough to never starve the loop itself
    // when the ring has only just started filling.
    seamMargin = juce::jmin (crossfadeSamples, juce::jmax (0, (validSamplesInRing - 2) / 4));

    loopLengthSamples = clampLoopLength (computeLoopLengthSamples());
    loopStartInRing = wrapRingIndex (writePosition - loopLengthSamples - seamMargin, ringBufferSize);
    stutterReadOffset = 0;

    return true;
}

void StutterCloneAudioProcessor::beginSeam (int continuationOffset, bool reverse) noexcept
{
    const int length = juce::jmin (seamMargin, loopLengthSamples);

    if (length <= 0)
    {
        seamRemaining = 0;
        return;
    }

    // wrapRingIndex folds a single ring length, so normalise the splice point now: the seam then
    // walks at most seamMargin samples away from an index that already sits inside the buffer.
    const int normalised = wrapRingIndex (loopStartInRing + continuationOffset,
                                          juce::jmax (1, ringBufferSize));

    seamOffset = normalised - loopStartInRing;
    seamReverse = reverse;
    seamLength = length;
    seamRemaining = length;
}

void StutterCloneAudioProcessor::resetHeldNotes() noexcept
{
    midiHeldGestureMask.store (0, std::memory_order_relaxed);
    uiHeldGestureMask.store (0, std::memory_order_relaxed);
    heldGestureMask.store (0, std::memory_order_relaxed);
    lastCombinedHeldMask = 0;
}

int StutterCloneAudioProcessor::findHighestHeldGestureNote() const noexcept
{
    const uint16_t mask = heldGestureMask.load (std::memory_order_relaxed);

    for (int i = stutter::numGestureNotes - 1; i >= 0; --i)
        if ((mask & static_cast<uint16_t> (1u << i)) != 0)
            return stutter::noteForGestureIndex (i, currentFirstGestureNote());

    return -1;
}

int StutterCloneAudioProcessor::computeLoopLengthSamples() const noexcept
{
    const auto step = stutter::evaluateAction (playingAction,
                                               stutter::measureBeatForAction (playingAction, gestureBeat));
    const double beats = stutter::beatsPerDivision[static_cast<size_t> (step.divisionIndex)];
    const int samples = juce::roundToInt ((beats * 60.0 / clampedBpm) * currentSampleRate);
    return juce::jlimit (2, juce::jmax (2, ringBufferSize), samples);
}

int StutterCloneAudioProcessor::clampLoopLength (int samples) const noexcept
{
    // Both seam margins have to stay inside the valid audio, otherwise the seam would read the
    // stale end of the ring rather than the continuation of the loop.
    const int usable = juce::jmax (2, validSamplesInRing - 2 * seamMargin);
    return juce::jlimit (2, usable, samples);
}

float StutterCloneAudioProcessor::readRingAtLoopOffset (int channel, int offset) const noexcept
{
    const int size = juce::jmin (ringBufferSize, ringBuffer.getNumSamples());
    const int numChannels = ringBuffer.getNumChannels();

    if (size <= 0 || numChannels <= 0)
        return 0.0f;

    const int safeChannel = juce::jlimit (0, numChannels - 1, channel);
    const int index = wrapRingIndex (loopStartInRing + offset, size);
    return ringBuffer.getSample (safeChannel, index);
}

float StutterCloneAudioProcessor::readLoopedSample (int channel) const noexcept
{
    const int last = juce::jmax (0, loopLengthSamples - 1);
    const int playOffset = reversePlayback ? (last - stutterReadOffset) : stutterReadOffset;
    float sample = readRingAtLoopOffset (channel, playOffset);

    if (seamRemaining > 0)
    {
        // The seam starts fully on the continuation, so the first sample after a splice follows
        // the previous one exactly and the waveform never steps.
        const float mix = static_cast<float> (seamLength - seamRemaining)
                        / static_cast<float> (seamLength);
        const float continued = readRingAtLoopOffset (channel, seamOffset);
        sample = sample * mix + continued * (1.0f - mix);
    }

    return sample;
}

void StutterCloneAudioProcessor::advanceLoopReadHead() noexcept
{
    if (loopLengthSamples < 2)
        return;

    if (seamRemaining > 0)
    {
        seamOffset += seamReverse ? -1 : 1;
        --seamRemaining;
    }

    ++stutterReadOffset;

    if (stutterReadOffset >= loopLengthSamples)
    {
        // Splice from the material that runs past the end of the pass that just finished, which
        // is what the seam margin was reserved for.
        beginSeam (reversePlayback ? -1 : loopLengthSamples, reversePlayback);

        stutterReadOffset = 0;
        ++loopCycleCount;
        loopLengthSamples = clampLoopLength (computeLoopLengthSamples());
    }
}

void StutterCloneAudioProcessor::publishWorkingPreset()
{
    const int dest = 1 - rtActionIndex.load (std::memory_order_relaxed);
    copyActions (rtActions[static_cast<size_t> (dest)], workingPreset.actions);
    rtActionIndex.store (dest, std::memory_order_release);
}

void StutterCloneAudioProcessor::replaceWorkingPreset (Preset preset, bool setQuantizeParam)
{
    workingPreset = std::move (preset);
    workingPreset.quantizeIndex = juce::jlimit (0, stutter::numQuantizeChoices - 1, workingPreset.quantizeIndex);

    if (setQuantizeParam)
        if (auto* param = apvts.getParameter (quantizeParamId))
            param->setValueNotifyingHost (param->convertTo0to1 (static_cast<float> (workingPreset.quantizeIndex)));

    publishWorkingPreset();
    sendChangeMessage();
}

void StutterCloneAudioProcessor::loadNamedPreset (const juce::String& name)
{
    replaceWorkingPreset (presetBank.loadPreset (name), true);
}

bool StutterCloneAudioProcessor::saveWorkingPreset()
{
    if (workingPreset.isFactory || workingPreset.name.isEmpty())
        return false;

    workingPreset.quantizeIndex = currentQuantizeIndex();
    const bool ok = presetBank.saveUserPreset (workingPreset);
    sendChangeMessage();
    return ok;
}

bool StutterCloneAudioProcessor::saveWorkingPresetAs (const juce::String& name)
{
    const auto trimmed = name.trim();

    if (trimmed.isEmpty() || presetBank.isFactoryName (trimmed))
        return false;

    workingPreset.name = trimmed;
    workingPreset.isFactory = false;
    workingPreset.quantizeIndex = currentQuantizeIndex();

    if (! presetBank.saveUserPreset (workingPreset))
        return false;

    sendChangeMessage();
    return true;
}

bool StutterCloneAudioProcessor::deleteNamedPreset (const juce::String& name)
{
    if (! presetBank.deleteUserPreset (name))
        return false;

    if (workingPreset.name == name)
        replaceWorkingPreset (makeClassicPreset(), true);

    sendChangeMessage();
    return true;
}

void StutterCloneAudioProcessor::updateAction (int gestureIndex, const stutter::Action& action)
{
    if (gestureIndex < 0 || gestureIndex >= stutter::numGestureNotes)
        return;

    workingPreset.actions[static_cast<size_t> (gestureIndex)] = action;
    workingPreset.isFactory = false;
    publishWorkingPreset();
}

void StutterCloneAudioProcessor::setWorkingQuantizeIndex (int index)
{
    workingPreset.quantizeIndex = juce::jlimit (0, stutter::numQuantizeChoices - 1, index);
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
    // A torn copy needs two publishes, so an unchanged sequence proves the writer never touched this slot.
    for (int attempt = 0; attempt < 4; ++attempt)
    {
        const uint32_t sequence = waveformSequence.load (std::memory_order_acquire);
        const int index = waveformPublished.load (std::memory_order_acquire);
        dest = waveformSnapshots[static_cast<size_t> (juce::jlimit (0, 1, index))];
        std::atomic_thread_fence (std::memory_order_acquire);

        if (waveformSequence.load (std::memory_order_relaxed) == sequence)
            return;
    }
}

void StutterCloneAudioProcessor::publishWaveformSnapshot() noexcept
{
    if (! editorOpen.load (std::memory_order_relaxed))
        return;

    const int size = juce::jmin (ringBufferSize, ringBuffer.getNumSamples());
    const int channels = juce::jmin (2, ringBuffer.getNumChannels());

    if (size <= 1 || channels <= 0)
        return;

    const int dest = 1 - waveformPublished.load (std::memory_order_relaxed);
    auto& snap = waveformSnapshots[static_cast<size_t> (dest)];

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
    const int playIndex = wrapRingIndex (loopStartInRing + stutterReadOffset, size);

    snap.writePos = static_cast<float> (wrapRingIndex (writePosition, size)) * inv;
    snap.loopStart = static_cast<float> (wrapRingIndex (loopStartInRing, size)) * inv;
    snap.loopLength = static_cast<float> (juce::jmax (0, loopLengthSamples)) * inv;
    snap.playPos = static_cast<float> (playIndex) * inv;
    snap.loopActive = stutterIsOn;
    snap.valid = validSamplesInRing > 1;

    waveformPublished.store (dest, std::memory_order_release);
    waveformSequence.store (waveformSequence.load (std::memory_order_relaxed) + 1u, std::memory_order_release);
}

void StutterCloneAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    workingPreset.quantizeIndex = currentQuantizeIndex();

    juce::ValueTree root ("STUTTERCLONE");
    root.setProperty ("firstGestureNote", getFirstGestureNote(), nullptr);
    root.appendChild (apvts.copyState(), nullptr);
    root.appendChild (PresetBank::presetToValueTree (workingPreset), nullptr);

    if (auto xml = root.createXml())
        copyXmlToBinary (*xml, destData);
}

void StutterCloneAudioProcessor::setFirstGestureNote (int midiNote)
{
    const int clamped = stutter::clampFirstGestureNote (midiNote);

    if (clamped == getFirstGestureNote())
        return;

    firstGestureNote.store (clamped, std::memory_order_relaxed);
    sendChangeMessage();
}

void StutterCloneAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary (data, sizeInBytes);

    if (xml == nullptr)
        return;

    if (xml->hasTagName ("STUTTERCLONE"))
    {
        auto root = juce::ValueTree::fromXml (*xml);
        auto params = root.getChildWithName (apvts.state.getType());

        if (params.isValid())
            apvts.replaceState (params);

        if (root.hasProperty ("firstGestureNote"))
            firstGestureNote.store (stutter::clampFirstGestureNote (static_cast<int> (root.getProperty ("firstGestureNote"))),
                                    std::memory_order_relaxed);

        auto presetTree = root.getChildWithName ("PRESET");

        if (presetTree.isValid())
            replaceWorkingPreset (PresetBank::presetFromValueTree (presetTree), false);
        else
            replaceWorkingPreset (makeClassicPreset(), false);

        return;
    }

    if (xml->hasTagName (apvts.state.getType()))
    {
        // Legacy session: APVTS-only. Drop old FX params and load Classic.
        replaceWorkingPreset (makeClassicPreset(), true);
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new StutterCloneAudioProcessor();
}
