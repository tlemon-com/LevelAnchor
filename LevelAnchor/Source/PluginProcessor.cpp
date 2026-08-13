#include "PluginProcessor.h"
#include "PluginEditor.h"

#if __STATIC_LIB__ == 1
#if _DEBUG
    #pragma comment(lib, "LevelAPI2-x64_d.lib")
#else
    #pragma comment(lib, "LevelAPI2-x64.lib")
#endif
#endif

using namespace juce;

//==============================================================================
LevelAnchorAudioProcessor::LevelAnchorAudioProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", AudioChannelSet::stereo(), true)
        .withOutput("Output", AudioChannelSet::stereo(), true)),
    parameters_(*this, nullptr, "PARAMETERS",
        {
            std::make_unique<AudioParameterFloat>("targetLUFS", "Target LUFS", NormalisableRange<float>(TARGET_MIN, TARGET_MAX, 0.1f), TARGET_DEFAULT),
            std::make_unique<AudioParameterBool>("reset", "Reset", false)
        })
{
}

LevelAnchorAudioProcessor::~LevelAnchorAudioProcessor() = default;

//==============================================================================
const String LevelAnchorAudioProcessor::getName() const { return "LevelAnchorAdjuster"; }

bool LevelAnchorAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    // allow mono or stereo only
    auto mainIn = layouts.getMainInputChannelSet();
    auto mainOut = layouts.getMainOutputChannelSet();
    if (mainIn.isDisabled() || mainOut.isDisabled())
        return false;

    if (mainIn != mainOut)
        return false;

    return (mainIn == AudioChannelSet::mono() || mainIn == AudioChannelSet::stereo());
}

void LevelAnchorAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Unused on both linking paths: the adjuster allocates nothing per block and
    // sizes everything from the sample rate.
    ignoreUnused(samplesPerBlock);

    sampleRate_ = sampleRate;
    int numCh = jmin(getTotalNumInputChannels(), 2); // use up to stereo

    // Cache both parameter handles here rather than resolving them by key on
    // every processBlock call.
    targetLufsRaw_ = parameters_.getRawParameterValue("targetLUFS");
    resetParam_    = dynamic_cast<AudioParameterBool*>(parameters_.getParameter("reset"));

    auto targetLUFS = targetLufsRaw_->load();

    // Sync the cached mirror so first-block change detection in
    // updateAdjusterParamsFromHost doesn't fire on stale defaults.
    targetLufsParamValue_ = targetLUFS;

    // Preset is fixed at Medium (the tuned reference); Slow/Fast remain in the
    // RealtimeAdjuster core for internal testing but are not user-facing.
    // The only path-specific code in this class. Both adjusters take the same
    // arguments in the same order; only the preset differs in type, being a C++
    // enum on one side and a plain unsigned across the DLL boundary on the other.
#if __STATIC_LIB__ == 0
    adjuster_ = std::make_unique<AdjusterType>(sampleRate, numCh, DynamicRealtimeAdjuster::PresetMedium, targetLUFS);

    // DLL missing, too old, or certificate refused: drop the engine so audio
    // passes through untouched. Reported via DBG only -- the editor doesn't
    // surface this yet; UIDisplayData::engineReady carries it for when it does.
    if (!adjuster_->isValid())
    {
        DBG("LevelAnchor: " + adjuster_->getLastError());
        adjuster_.reset();
    }
#else
    adjuster_ = std::make_unique<AdjusterType>(sampleRate, numCh, Preset::Medium, targetLUFS);
    adjuster_->Reset(numCh, sampleRate);
#endif

    samplesSinceLastUIDispatch_ = 0;
    samplesCounter_ = 0;
    uiDispatchIntervalSamples_ = static_cast<int>(0.04 * sampleRate_); // ~40ms

    ResetDeviationStats();
}

void LevelAnchorAudioProcessor::releaseResources()
{
    adjuster_.reset();
}

void LevelAnchorAudioProcessor::updateAdjusterParamsFromHost()
{
    if (!adjuster_ || targetLufsRaw_ == nullptr) return;

    float newTarget = targetLufsRaw_->load();

    if (std::fabs(newTarget - targetLufsParamValue_.load()) > 0.01f)
    {
        targetLufsParamValue_ = newTarget;
        adjuster_->SetTargetLUFS(newTarget);
    }
}

void LevelAnchorAudioProcessor::processBlock(AudioBuffer<float>& buffer, MidiBuffer& /*midiMessages*/)
{
    ScopedNoDenormals noDenormals;

    const int numIn = getTotalNumInputChannels();
    const int numOut = getTotalNumOutputChannels();

    for (int ch = numIn; ch < numOut; ++ch)
        buffer.clear(ch, 0, buffer.getNumSamples());


    // No engine (dynamic build without a usable LevelAPI2 DLL): the audio above
    // is already passing through untouched. Keep the UI snapshot ticking so the
    // editor sees engineReady == false instead of a frozen display.
    if (!adjuster_)
    {
        samplesSinceLastUIDispatch_ += buffer.getNumSamples();

        if (samplesSinceLastUIDispatch_ >= uiDispatchIntervalSamples_)
        {
            UIDisplayData data; // engineReady defaults to false
            data.samplerate = static_cast<int>(getSampleRate());
            samplesSinceLastUIDispatch_ = 0;
            publishUIDisplayData(data);
        }

        return;
    }

    // Update params if changed
    updateAdjusterParamsFromHost();

    // Check reset flag (resetParam_ cached in prepareToPlay)
    if (resetParam_ != nullptr && resetParam_->get())
    {
        samplesCounter_ = 0;
        adjuster_->Reset(getTotalNumInputChannels(), getSampleRate());

        ResetDeviationStats(); // deviation is measured "since reset", so clear it too
        resetParam_->operator=(false); // clear flag so it's one-shot
    }

    // Process audio
    adjuster_->ProcessBlock(buffer);

    // Prepare UI snapshot every ~40 ms
    samplesSinceLastUIDispatch_ += buffer.getNumSamples();
    samplesCounter_ += buffer.getNumSamples();
    if (samplesSinceLastUIDispatch_ >= uiDispatchIntervalSamples_)
    {
        UIDisplayData data;
        data.engineReady = true;
        data.appliedGainDb = adjuster_->GetLastAppliedGainDb();
        data.warmupSecondsLeft = adjuster_->GetWarmupSecondsLeft();
        data.warmupSecondsMax  = adjuster_->GetWarmupSecondsMax();
        data.calibSecondsLeft  = adjuster_->GetCalibrationSecondsLeft();
        data.LUFS = adjuster_->GetLUFS();
        data.PSR = adjuster_->GetPSR();
        data.LRA = adjuster_->GetLRA();
        data.currentMValue = jmax<float>(adjuster_->GetBS1771(), -70.f);
        data.index = samplesCounter_;
        data.samplerate = (int)getSampleRate();

        // --- Deviation of integrated LUFS from target, accumulated since reset ---
        // Gate on the CALIBRATION ramp, not the main warmup. GetLUFS reads the
        // calibration meter, which starts empty when the main warmup ends and ramps
        // its own strength in from there -- so for that window GetLUFS is genuinely
        // far off target (correction is weakest exactly when we'd start measuring).
        // Accumulating then poisons the since-reset stats, and because the sums are
        // cumulative that early error only decays as 1/N. The calibration countdown
        // covers the whole of that window and hits 0 only once GetLUFS is settled,
        // so it is the honest gate.
        // Hard gate (don't count until settled) rather than a fade: a weighted fade
        // would still admit the bad early samples into the sums. One sample per UI
        // dispatch (~40 ms).
        if (data.calibSecondsLeft <= 0.0f)
        {
            const double err = (double)data.LUFS - (double)targetLufsParamValue_.load();
            ++devCount_;
            devSumErr_   += err;
            devSumErrSq_ += err * err;
            devMaxAbs_ = std::max <double> (devMaxAbs_, std::abs(err));
        }
        if (devCount_ > 0)
        {
            const double mean = devSumErr_ / (double)devCount_;
            data.devBias = (float)mean;
            data.devRms  = (float)std::sqrt(devSumErrSq_ / (double)devCount_);
            data.devMax  = (float)devMaxAbs_;
            // spread = std-dev of error = sqrt(meanSq - mean^2). Clamp the radicand
            // at 0 to guard tiny negative values from float rounding.
            const double variance = std::max<double>(0.0, (devSumErrSq_ / (double)devCount_) - (mean * mean));
            data.devSpread = (float)std::sqrt(variance);
        }

        samplesSinceLastUIDispatch_ = 0;
        publishUIDisplayData(data);
    }
}

// Audio thread. Writer half of the seqlock described in PluginProcessor.h: two
// relaxed stores and a struct copy, with release fences so a reader that sees
// the closing count also sees the whole snapshot behind it.
void LevelAnchorAudioProcessor::publishUIDisplayData(const UIDisplayData& data) noexcept
{
    const uint32_t seq = uiSeq_.load(std::memory_order_relaxed); // audio thread is the only writer

    uiSeq_.store(seq + 1, std::memory_order_relaxed); // odd: write in progress
    std::atomic_thread_fence(std::memory_order_release);

    uiSnapshot_ = data;

    std::atomic_thread_fence(std::memory_order_release);
    uiSeq_.store(seq + 2, std::memory_order_relaxed); // even: snapshot complete
}

// Message thread. Reader half: copy, then check the count did not move. A retry
// costs a ~64-byte copy and is already rare (the writer holds the odd count for
// no longer than that same copy), so the attempt limit is only there so a
// writer descheduled mid-write cannot spin the UI thread. Falling back to the
// previous clean snapshot for one frame is invisible at 30 Hz.
LevelAnchorAudioProcessor::UIDisplayData LevelAnchorAudioProcessor::getLatestUIDisplayData() const noexcept
{
    constexpr int kMaxAttempts = 8;

    for (int attempt = 0; attempt < kMaxAttempts; ++attempt)
    {
        const uint32_t before = uiSeq_.load(std::memory_order_acquire);

        if ((before & 1u) != 0u) // write in progress
            continue;

        UIDisplayData data = uiSnapshot_;

        std::atomic_thread_fence(std::memory_order_acquire);

        if (uiSeq_.load(std::memory_order_relaxed) == before)
        {
            uiSnapshotFallback_ = data; // message thread only, no synchronisation needed
            return data;
        }
    }

    return uiSnapshotFallback_;
}

//==============================================================================
bool LevelAnchorAudioProcessor::hasEditor() const { return true; }
AudioProcessorEditor* LevelAnchorAudioProcessor::createEditor()
{
    return new LevelAnchorAudioProcessorEditor(*this);
}

//==============================================================================
void LevelAnchorAudioProcessor::getStateInformation(MemoryBlock& destData)
{
    MemoryOutputStream stream(destData, true);
    parameters_.state.writeToStream(stream);
}

void LevelAnchorAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    ValueTree tree = ValueTree::readFromData(data, static_cast<size_t>(sizeInBytes));
    if (tree.isValid())
        parameters_.replaceState(tree);
}
//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new LevelAnchorAudioProcessor();
}
