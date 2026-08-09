#pragma once

#include <JuceHeader.h>


// Two permanent linking paths, chosen by the build configuration: only the
// *-static configurations in LevelAnchor.jucer define __STATIC_LIB__=1, and an
// undefined identifier evaluates to 0 in #if -- so the DYNAMIC path is what you
// get with no configuration at all. That is deliberate: it is the path the
// open-source audience builds, and this repo doubles as the reference
// integration of LevelAPI2.
//
// DynamicRealtimeAdjuster mirrors RealtimeAdjuster method-for-method, so this
// alias is enough to make the whole processor path-agnostic; the only other
// #if in this class is the single construction line in prepareToPlay.
//
// CRITICAL: the #includes must stay INSIDE the guard. Each configuration's
// header search path reaches only its own library headers, so an include placed
// outside breaks the other configuration with a file-not-found.
#if __STATIC_LIB__ == 0
    #include "LevelApiDynamic.h"
    using AdjusterType = DynamicRealtimeAdjuster;
#else
    #include "RealtimeAdjuster.h"
    using AdjusterType = RealtimeAdjuster;
#endif

#define TARGET_MIN -24.f
#define TARGET_MAX -8.f
#define TARGET_DEFAULT -16.f // single source of truth for the shipping default

/**
    LevelAnchorAudioProcessor
    ----------------------------------
    AudioProcessor wrapper for RealtimeAdjuster.

    - Supports mono & stereo.
    - Exposes Target LUFS (-24..-8). The adjuster preset is fixed at Medium
      (Slow/Fast still exist in the RealtimeAdjuster core for internal testing).
    - Periodically (~40 ms) prepares UI data snapshot.
*/
class LevelAnchorAudioProcessor : public juce::AudioProcessor
{
public:
    LevelAnchorAudioProcessor();
    ~LevelAnchorAudioProcessor() override;

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    //==============================================================================
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    //==============================================================================
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    //==============================================================================
    struct UIDisplayData
    {
        // Samples elapsed since the last reset. int64 because it also drives
        // the elapsed-time readout: an int overflows after ~12.4 h at 48 kHz,
        // which is a reachable live-stream session length.
        int64_t index = 0;
        int samplerate = 44100;
        float appliedGainDb = 0.0f;
        float warmupProgress = 0.0f;
        float calibWarmup = 0.0f; // calibration-meter warmup 0..1 (post main warmup); deviation stats gate on this reaching 1
        float LUFS = 0.0f;
        float PSR = 0.0f;
        float LRA = 0.0f;
        float warmup_sec = 3; // WARMUP_SEC_FULL
        float currentMValue = 0.0f; // realtime fused loudness indicator for meter
        // False when no adjuster exists (dynamic build with the LevelAPI2 DLL
        // missing/refused), i.e. every field above is meaningless and audio is
        // passing through untouched. The editor ignores this for now; it is
        // here so it CAN surface the state without touching the audio thread.
        bool engineReady = false;
        // Deviation of integrated LUFS from target, accumulated since last reset
        // (post-warmup only). All in LU. rms is the headline "how tightly it holds"
        // figure; bias = mean signed error (sitting high/low); maxDev = worst |error|.
        // spread = std-dev of error (the wobble, with bias removed): rms^2 = bias^2 + spread^2.
        float devRms    = 0.0f;
        float devBias   = 0.0f;
        float devSpread = 0.0f;
        float devMax    = 0.0f;
    };

    /** Called from UI thread periodically (40 ms). Returns latest snapshot (thread-safe). */
    UIDisplayData getLatestUIDisplayData() const noexcept;

    juce::AudioProcessorValueTreeState& getValueTreeState() noexcept { return parameters_; }

private:
    // The DSP engine, of whichever kind this configuration links (see the alias
    // at the top of this file). Null means no engine -- audio then passes
    // through unmodified rather than being muted, which for a live-stream tool
    // is the only acceptable failure mode.
    std::unique_ptr<AdjusterType> adjuster_;

    // Parameters
    juce::AudioProcessorValueTreeState parameters_;
    std::atomic<float> targetLufsParamValue_{ TARGET_DEFAULT };

    // Cached in prepareToPlay -- resolving either by key inside processBlock
    // means a keyed lookup (plus RTTI for the reset one) on the audio thread,
    // every block. Both owned by parameters_; these are non-owning
    // observers. targetLufsRaw_ is the APVTS's live atomic; targetLufsParamValue_
    // above is our own mirror of it, used only for change detection.
    juce::AudioParameterBool* resetParam_     = nullptr;
    std::atomic<float>*       targetLufsRaw_  = nullptr;

    // UI update rate (~40 ms)
    double sampleRate_ = 48000.0;
    int64_t samplesCounter_ = 0; // see UIDisplayData::index for why this is 64-bit
    int samplesSinceLastUIDispatch_ = 0;
    int uiDispatchIntervalSamples_ = 1920; // 40ms @48k
    mutable std::atomic<UIDisplayData> latestUIDisplayData_;

    // --- Deviation accumulators (integrated LUFS vs target, since last reset) ---
    // Running sums let us derive bias/RMS/max in O(1) per UI dispatch with no
    // history buffer. Sampled at the UI dispatch rate, post-warmup only, so the
    // cold-start intro is excluded (consistent with the calibration meter/timer).
    // Cleared by ResetDeviationStats() on the reset one-shot and on prepareToPlay.
    uint64_t devCount_    = 0;   // number of samples accumulated
    double   devSumErr_   = 0.0; // sum of (LUFS - target)   -> bias
    double   devSumErrSq_ = 0.0; // sum of (LUFS - target)^2 -> rms
    double   devMaxAbs_   = 0.0; // max |LUFS - target|      -> worst excursion
    void ResetDeviationStats() noexcept { devCount_ = 0; devSumErr_ = 0.0; devSumErrSq_ = 0.0; devMaxAbs_ = 0.0; }

    // helpers
    void updateAdjusterParamsFromHost();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LevelAnchorAudioProcessor)
};
