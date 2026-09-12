#include "DspChain.h"

#include <cmath>

void GestureDspChain::prepare (double sampleRate, int samplesPerBlock, int numChannels)
{
    currentSampleRate = sampleRate;
    maxChannels = juce::jlimit (1, 8, numChannels);
    maxDelaySamples = juce::jmax (1, juce::roundToInt (sampleRate * 2.0));

    const juce::dsp::ProcessSpec spec
    {
        sampleRate,
        static_cast<juce::uint32> (juce::jmax (1, samplesPerBlock)),
        static_cast<juce::uint32> (maxChannels)
    };

    granular.prepare (spec);
    resonator.prepare (spec);
    fuzz.prepare (spec);
    filter.prepare (spec);
    filter.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
    filter.setResonance (0.707f);
    filter.setCutoffFrequency (1000.0f);
    filter.reset();

    delayLine.prepare (spec);
    delayLine.setMaximumDelayInSamples (maxDelaySamples);
    delayLine.reset();

    reverb.prepare (spec);
    reverb.reset();

    scratch.setSize (maxChannels, juce::jmax (samplesPerBlock * 4, 4096), false, false, true);
    scratch.clear();

    cutoffSmoothed.reset (sampleRate, 0.02);
    cutoffSmoothed.setCurrentAndTargetValue (1000.0f);
    delayMixSmoothed.reset (sampleRate, 0.03);
    reverbMixSmoothed.reset (sampleRate, 0.03);

    downsampleHold = 0;
    heldSample.fill (0.0f);
    lastFilterType = 0;
    lastPitchEngine = -1;
}

void GestureDspChain::reset() noexcept
{
    granular.reset();
    resonator.reset();
    fuzz.reset();
    filter.reset();
    delayLine.reset();
    reverb.reset();
    downsampleHold = 0;
    heldSample.fill (0.0f);
    scratch.clear();
    lastPitchEngine = -1;
}

void GestureDspChain::beginGesture() noexcept
{
    granular.beginGesture();
    resonator.beginGesture();
    fuzz.reset();
    filter.reset();
    downsampleHold = 0;
}

void GestureDspChain::resetDelay() noexcept
{
    delayLine.reset();
}

void GestureDspChain::resetReverb() noexcept
{
    reverb.reset();
}

void GestureDspChain::process (juce::AudioBuffer<float>& buffer,
                               int startSample,
                               int numSamples,
                               const Settings& settings) noexcept
{
    if (numSamples <= 0)
        return;

    const int numChannels = juce::jmin (buffer.getNumChannels(), scratch.getNumChannels(), maxChannels);

    if (scratch.getNumSamples() < numSamples)
        return; // block larger than prepared size: skip FX rather than allocate

    for (int channel = 0; channel < numChannels; ++channel)
        scratch.copyFrom (channel, 0, buffer, channel, startSample, numSamples);

    if (! settings.feedEffects)
        scratch.clear (0, numSamples);

    const int type = juce::jlimit (0, 2, settings.filterType);
    const juce::dsp::StateVariableTPTFilterType svfType[]
    {
        juce::dsp::StateVariableTPTFilterType::lowpass,
        juce::dsp::StateVariableTPTFilterType::highpass,
        juce::dsp::StateVariableTPTFilterType::bandpass
    };

    if (type != lastFilterType)
    {
        filter.setType (svfType[type]);
        lastFilterType = type;
    }

    filter.setResonance (juce::jlimit (0.1f, 8.0f, settings.resonance));
    cutoffSmoothed.setTargetValue (juce::jlimit (20.0f, 20000.0f, settings.cutoffHz));
    delayMixSmoothed.setTargetValue (settings.delayOn ? juce::jlimit (0.0f, 1.0f, settings.delayMix) : 0.0f);
    reverbMixSmoothed.setTargetValue (settings.reverbOn ? juce::jlimit (0.0f, 1.0f, settings.reverbMix) : 0.0f);

    const float delaySamples = juce::jlimit (1.0f, static_cast<float> (maxDelaySamples - 1), settings.delaySamples);
    delayLine.setDelay (delaySamples);

    std::array<float, 8> frame {};

    for (int i = 0; i < numSamples; ++i)
    {
        for (int channel = 0; channel < numChannels; ++channel)
            frame[static_cast<size_t> (channel)] = scratch.getSample (channel, i);

        processScratchSample (numChannels, frame.data(), settings);

        for (int channel = 0; channel < numChannels; ++channel)
            scratch.setSample (channel, i, frame[static_cast<size_t> (channel)]);
    }

    if (settings.reverbOn || reverbMixSmoothed.getCurrentValue() > 0.0001f)
        processReverb (numChannels, numSamples, settings);

    if (settings.feedEffects)
    {
        for (int channel = 0; channel < numChannels; ++channel)
            buffer.copyFrom (channel, startSample, scratch, channel, 0, numSamples);
    }
    else
    {
        for (int channel = 0; channel < numChannels; ++channel)
            buffer.addFrom (channel, startSample, scratch, channel, 0, numSamples);
    }
}

void GestureDspChain::processScratchSample (int numChannels, float* frame, const Settings& settings) noexcept
{
    processGranular (numChannels, frame, settings);
    processFuzz (numChannels, frame, settings);
    processFilter (numChannels, frame, settings);
    processLoFi (numChannels, frame, settings);
    processDelay (numChannels, frame, settings);
    reverbMixSmoothed.skip (1);
}

void GestureDspChain::processGranular (int numChannels, float* frame, const Settings& settings) noexcept
{
    const int engine = juce::jlimit (0, 1, settings.granularEngine);

    if (engine != lastPitchEngine)
    {
        granular.beginGesture();
        resonator.beginGesture();
        lastPitchEngine = engine;
    }

    if (engine == 1)
    {
        resonator.processFrame (numChannels, frame,
                                settings.feedEffects && settings.granularOn,
                                settings.granularMidiNote,
                                settings.granularMix);
        return;
    }

    if (! settings.feedEffects)
        return;

    granular.processFrame (numChannels, frame, settings.granularOn, settings.granularMidiNote, settings.granularMix);
}

void GestureDspChain::processFuzz (int numChannels, float* frame, const Settings& settings) noexcept
{
    fuzz.processFrame (numChannels, frame, settings.fuzzGain);
}

void GestureDspChain::processFilter (int numChannels, float* frame, const Settings& settings) noexcept
{
    const float cutoff = cutoffSmoothed.getNextValue();
    filter.setCutoffFrequency (cutoff);

    if (! settings.filterOn)
        return;

    for (int channel = 0; channel < numChannels; ++channel)
        frame[channel] = filter.processSample (channel, frame[channel]);
}

void GestureDspChain::processLoFi (int numChannels, float* frame, const Settings& settings) noexcept
{
    if (! settings.loFiOn)
        return;

    const int factor = juce::jmax (1, settings.downsample);

    if (downsampleHold == 0)
    {
        for (int channel = 0; channel < numChannels; ++channel)
            heldSample[static_cast<size_t> (channel)] = frame[channel];
    }

    ++downsampleHold;

    if (downsampleHold >= factor)
        downsampleHold = 0;

    const float bits = juce::jlimit (1.0f, 16.0f, settings.bitDepth);
    const float scale = std::exp2 (bits - 1.0f);

    for (int channel = 0; channel < numChannels; ++channel)
    {
        const float held = heldSample[static_cast<size_t> (channel)];
        frame[channel] = std::round (held * scale) / scale;
    }
}

void GestureDspChain::processDelay (int numChannels, float* frame, const Settings& settings) noexcept
{
    const float delayMix = delayMixSmoothed.getNextValue();
    const float feedback = juce::jlimit (0.0f, 0.95f, settings.delayFeedback);

    for (int channel = 0; channel < numChannels; ++channel)
    {
        const float input = frame[channel];
        const float delayed = delayLine.popSample (channel);
        delayLine.pushSample (channel, input + delayed * feedback);
        frame[channel] = input * (1.0f - delayMix) + delayed * delayMix;
    }
}

void GestureDspChain::processReverb (int numChannels, int numSamples, const Settings& settings) noexcept
{
    juce::dsp::Reverb::Parameters reverbParams;
    const float mix = reverbMixSmoothed.getCurrentValue();
    reverbParams.roomSize = juce::jlimit (0.0f, 1.0f, settings.reverbSize);
    reverbParams.damping = juce::jlimit (0.0f, 1.0f, settings.reverbDamping);
    reverbParams.width = 1.0f;
    reverbParams.freezeMode = 0.0f;
    reverbParams.dryLevel = 1.0f - mix;
    reverbParams.wetLevel = mix;
    reverb.setParameters (reverbParams);
    reverb.setEnabled (true);

    auto block = juce::dsp::AudioBlock<float> (scratch)
                     .getSubsetChannelBlock (0, static_cast<size_t> (numChannels))
                     .getSubBlock (0, static_cast<size_t> (numSamples));
    juce::dsp::ProcessContextReplacing<float> context (block);
    reverb.process (context);
}
