#pragma once

#include "DspChain.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <array>
#include <atomic>

class StutterCloneAudioProcessor final : public juce::AudioProcessor
{
public:
    StutterCloneAudioProcessor();
    ~StutterCloneAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getAPVTS() noexcept { return apvts; }

    float getCurrentBpm() const noexcept { return currentBpm.load (std::memory_order_relaxed); }
    double getPpqPosition() const noexcept { return ppqPosition.load (std::memory_order_relaxed); }
    bool isStutterActive() const noexcept { return stutterActive.load (std::memory_order_relaxed); }
    int getGestureNote() const noexcept { return gestureNote.load (std::memory_order_relaxed); }
    int getActiveDivisionIndex() const noexcept { return activeDivisionIndex.load (std::memory_order_relaxed); }

    static constexpr int waveformBins = 256;
    static constexpr int numDivisions = 5;
    static constexpr const char* loopDivisionParamId = "loopDivision";
    static constexpr const char* sweepParamId = "sweep";
    static constexpr const char* reverseParamId = "reverse";
    static constexpr const char* alternatePanParamId = "alternatePan";

    static constexpr const char* filterOnParamId = "filterOn";
    static constexpr const char* filterTypeParamId = "filterType";
    static constexpr const char* filterCutoffStartParamId = "filterCutoffStart";
    static constexpr const char* filterCutoffEndParamId = "filterCutoffEnd";
    static constexpr const char* filterResonanceParamId = "filterResonance";

    static constexpr const char* loFiOnParamId = "loFiOn";
    static constexpr const char* loFiBitsParamId = "loFiBits";
    static constexpr const char* loFiDownsampleParamId = "loFiDownsample";

    static constexpr const char* delayOnParamId = "delayOn";
    static constexpr const char* delayMixParamId = "delayMix";
    static constexpr const char* delayDivisionParamId = "delayDivision";
    static constexpr const char* delayFeedbackParamId = "delayFeedback";
    static constexpr const char* delayCutParamId = "delayCut";

    static constexpr const char* reverbOnParamId = "reverbOn";
    static constexpr const char* reverbMixParamId = "reverbMix";
    static constexpr const char* reverbSizeParamId = "reverbSize";
    static constexpr const char* reverbDampingParamId = "reverbDamping";
    static constexpr const char* reverbCutParamId = "reverbCut";

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    struct WaveformSnapshot
    {
        std::array<float, waveformBins> mins {};
        std::array<float, waveformBins> maxs {};
        float writePos = 0.0f;
        float loopStart = 0.0f;
        float loopLength = 0.0f;
        float playPos = 0.0f;
        bool loopActive = false;
        bool valid = false;
    };

    void setEditorOpen (bool shouldBeOpen) noexcept;
    void copyWaveformSnapshot (WaveformSnapshot& dest) const noexcept;

private:
    void capturePlayHead() noexcept;
    void handleMidiEvent (const juce::uint8* data, int numBytes) noexcept;
    void processAudioSlice (juce::AudioBuffer<float>& buffer, int startSample, int numSamples) noexcept;
    void processFxSlice (juce::AudioBuffer<float>& buffer, int startSample, int numSamples) noexcept;
    void writeToRingBuffer (const juce::AudioBuffer<float>& buffer, int startSample, int numSamples) noexcept;
    void startStutter (int midiNote) noexcept;
    void stopStutter() noexcept;
    void resetHeldNotes() noexcept;
    int findHighestHeldNote() const noexcept;
    int currentDivisionIndex() const noexcept;
    int computeLoopLengthSamples() const noexcept;
    float readRingAtLoopOffset (int channel, int offset) const noexcept;
    float readLoopedSample (int channel) const noexcept;
    void advanceLoopReadHead() noexcept;
    void publishWaveformSnapshot() noexcept;

    juce::AudioProcessorValueTreeState apvts;
    std::atomic<float>* loopDivisionParam = nullptr;
    std::atomic<float>* sweepParam = nullptr;
    std::atomic<float>* reverseParam = nullptr;
    std::atomic<float>* alternatePanParam = nullptr;
    std::atomic<float>* filterOnParam = nullptr;
    std::atomic<float>* filterTypeParam = nullptr;
    std::atomic<float>* filterCutoffStartParam = nullptr;
    std::atomic<float>* filterCutoffEndParam = nullptr;
    std::atomic<float>* filterResonanceParam = nullptr;
    std::atomic<float>* loFiOnParam = nullptr;
    std::atomic<float>* loFiBitsParam = nullptr;
    std::atomic<float>* loFiDownsampleParam = nullptr;
    std::atomic<float>* delayOnParam = nullptr;
    std::atomic<float>* delayMixParam = nullptr;
    std::atomic<float>* delayDivisionParam = nullptr;
    std::atomic<float>* delayFeedbackParam = nullptr;
    std::atomic<float>* delayCutParam = nullptr;
    std::atomic<float>* reverbOnParam = nullptr;
    std::atomic<float>* reverbMixParam = nullptr;
    std::atomic<float>* reverbSizeParam = nullptr;
    std::atomic<float>* reverbDampingParam = nullptr;
    std::atomic<float>* reverbCutParam = nullptr;

    GestureDspChain dspChain;
    juce::AudioBuffer<float> ringBuffer;
    int ringBufferSize = 0;
    int writePosition = 0;
    int validSamplesInRing = 0;
    int crossfadeSamples = 0;
    double currentSampleRate = 44100.0;

    bool stutterIsOn = false;
    bool hasWrapped = false;
    bool reversePlayback = false;
    int heldNoteCount = 0;
    int stutterReadOffset = 0;
    int loopLengthSamples = 0;
    int loopStartInRing = 0;
    int fadeInRemaining = 0;
    int fadeOutRemaining = 0;
    int midiDivisionOverride = -1;
    int sweepSamplesElapsed = 0;
    int sweepLengthSamples = 1;
    int loopCycleCount = 0;
    double sweepStartBeats = 0.25;
    std::array<uint8_t, 128> notesHeld {};

    std::atomic<float> currentBpm { 120.0f };
    std::atomic<double> ppqPosition { 0.0 };
    std::atomic<bool> stutterActive { false };
    std::atomic<int> gestureNote { -1 };
    std::atomic<int> activeDivisionIndex { 2 };
    std::atomic<bool> editorOpen { false };
    std::atomic<int> waveformPublished { 0 };
    std::array<WaveformSnapshot, 2> waveformSnapshots {};
    int samplesUntilWaveformUpdate = 0;
    int waveformUpdateInterval = 1024;

    static constexpr double ringBufferSeconds = 4.0;
    static constexpr double crossfadeSeconds = 0.003;
    static constexpr double sweepDurationBeats = 4.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StutterCloneAudioProcessor)
};
