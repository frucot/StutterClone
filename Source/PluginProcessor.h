#pragma once

#include "Action.h"
#include "DspChain.h"
#include "PresetBank.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <array>
#include <atomic>

class StutterCloneAudioProcessor final : public juce::AudioProcessor,
                                         public juce::ChangeBroadcaster
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
    PresetBank& getPresetBank() noexcept { return presetBank; }
    const Preset& getWorkingPreset() const noexcept { return workingPreset; }
    Preset& getWorkingPreset() noexcept { return workingPreset; }

    void publishWorkingPreset();
    void replaceWorkingPreset (Preset preset, bool setQuantizeParam);
    void loadNamedPreset (const juce::String& name);
    bool saveWorkingPreset();
    bool saveWorkingPresetAs (const juce::String& name);
    bool deleteNamedPreset (const juce::String& name);
    void updateAction (int gestureIndex, const stutter::Action& action);
    void setWorkingQuantizeIndex (int index);

    float getCurrentBpm() const noexcept { return currentBpm.load (std::memory_order_relaxed); }
    double getPpqPosition() const noexcept { return ppqPosition.load (std::memory_order_relaxed); }
    bool isStutterActive() const noexcept { return stutterActive.load (std::memory_order_relaxed); }
    bool isGesturePending() const noexcept { return gesturePending.load (std::memory_order_relaxed); }
    int getGestureNote() const noexcept { return gestureNote.load (std::memory_order_relaxed); }
    int getPendingNote() const noexcept { return pendingNoteAtomic.load (std::memory_order_relaxed); }
    int getActiveDivisionIndex() const noexcept { return activeDivisionIndex.load (std::memory_order_relaxed); }
    int getActiveStep() const noexcept { return activeStep.load (std::memory_order_relaxed); }
    float getGestureBeat() const noexcept { return gestureBeatAtomic.load (std::memory_order_relaxed); }
    uint16_t getHeldGestureMask() const noexcept { return heldGestureMask.load (std::memory_order_relaxed); }

    static constexpr int waveformBins = 256;
    static constexpr const char* quantizeParamId = "quantize";

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
    void armPending (int midiNote) noexcept;
    void cancelPending() noexcept;
    void startStutter (int midiNote) noexcept;
    void stopStutter() noexcept;
    void resetHeldNotes() noexcept;
    int findHighestHeldGestureNote() const noexcept;
    int computeLoopLengthSamples() const noexcept;
    void applyEvaluatedStep (const stutter::EvaluatedStep& step) noexcept;
    float readRingAtLoopOffset (int channel, int offset) const noexcept;
    float readLoopedSample (int channel) const noexcept;
    void advanceLoopReadHead() noexcept;
    void publishWaveformSnapshot() noexcept;
    const stutter::Action& rtActionForNote (int midiNote) const noexcept;
    int currentQuantizeIndex() const noexcept;

    juce::AudioProcessorValueTreeState apvts;
    std::atomic<float>* quantizeParam = nullptr;
    PresetBank presetBank;
    Preset workingPreset;
    std::array<std::array<stutter::Action, stutter::numGestureNotes>, 2> rtActions {};
    std::atomic<int> rtActionIndex { 0 };

    GestureDspChain dspChain;
    juce::AudioBuffer<float> ringBuffer;
    int ringBufferSize = 0;
    int writePosition = 0;
    int validSamplesInRing = 0;
    int crossfadeSamples = 0;
    double currentSampleRate = 44100.0;
    double ppqCursor = 0.0;
    double ppqPerSample = 0.0;
    double pendingGridPpq = 0.0;

    bool stutterIsOn = false;
    bool hasWrapped = false;
    bool reversePlayback = false;
    bool pendingArmed = false;
    int heldNoteCount = 0;
    int stutterReadOffset = 0;
    int loopLengthSamples = 0;
    int loopStartInRing = 0;
    int fadeInRemaining = 0;
    int fadeOutRemaining = 0;
    int loopCycleCount = 0;
    int pendingNote = -1;
    int playingNote = -1;
    int lastStepIndex = -1;
    double gestureBeat = 0.0;
    stutter::Action playingAction {};
    std::array<uint8_t, 128> notesHeld {};

    std::atomic<float> currentBpm { 120.0f };
    std::atomic<double> ppqPosition { 0.0 };
    std::atomic<bool> stutterActive { false };
    std::atomic<bool> gesturePending { false };
    std::atomic<int> gestureNote { -1 };
    std::atomic<int> pendingNoteAtomic { -1 };
    std::atomic<int> activeDivisionIndex { 2 };
    std::atomic<int> activeStep { 0 };
    std::atomic<float> gestureBeatAtomic { 0.0f };
    std::atomic<uint16_t> heldGestureMask { 0 };
    std::atomic<bool> editorOpen { false };
    std::atomic<int> waveformPublished { 0 };
    std::array<WaveformSnapshot, 2> waveformSnapshots {};
    int samplesUntilWaveformUpdate = 0;
    int waveformUpdateInterval = 1024;

    static constexpr double ringBufferSeconds = 4.0;
    static constexpr double crossfadeSeconds = 0.003;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StutterCloneAudioProcessor)
};
