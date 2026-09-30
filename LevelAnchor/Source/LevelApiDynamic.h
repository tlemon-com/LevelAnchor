#pragma once

/*
    LevelApiDynamic.h
    -----------------
    The DYNAMIC linking path: LevelAPI2 loaded from its DLL at runtime.

    This is the default build (see the four build configurations in
    LevelAnchor.jucer -- only the *-static ones define __STATIC_LIB__=1), and it
    is deliberately the one the open-source audience gets out of the box: this
    file plus its .cpp are the worked example of integrating LevelAPI2 into a
    real plugin. It is commented more heavily than the rest of the codebase for
    that reason.

    Two types live here:
      LevelApiLibrary         -- process-wide loader. Loaded on first use and
                                 then kept for the lifetime of the process; it
                                 is deliberately never unloaded (see
                                 getInstance() and the .cpp for why).
      DynamicRealtimeAdjuster -- one per plugin instance. Mirrors the static
                                 RealtimeAdjuster class method-for-method
                                 (same names, same argument order) so that
                                 PluginProcessor can share all of its code
                                 between the two linking paths.

    Nothing here includes <windows.h> or LevelAPI2_dynlib.h -- both are confined
    to LevelApiDynamic.cpp. The library header pulls in <windows.h> and defines
    its loader functions in place (static, so each includer gets its own copy),
    which is why it stays behind this one translation unit.

    Sibling file: LevelApiDynamic.cpp.
*/

// The whole file compiles to nothing in the static configurations, which is why
// no per-configuration file exclusions are needed in the Projucer project.
// CRITICAL: the include below must stay INSIDE this guard -- the static
// configurations do not have the dynlib header path, and the dynamic ones do
// not have the RealtimeAdjuster path, so a misplaced #include breaks the other
// configuration with a file-not-found.
#if __STATIC_LIB__ == 0

#include <JuceHeader.h>
#include <memory>

//==============================================================================
/** Loads LevelAPI2's DLL and resolves its C API once per process.

    Failure is never fatal: if the DLL is missing, too old, or refuses the
    certificate, isLoaded() returns false, getLastError() explains why, and the
    plugin simply passes audio through. For a live-streaming tool, doing nothing
    is much better than killing the stream.
*/
class LevelApiLibrary
{
public:
    /** The one loader, created on first use.

        CRITICAL -- it is never destroyed and the DLL is never unloaded, on
        purpose. Tying the library's lifetime to plugin instances meant every
        stop/start (and every host plugin-scan cycle, Reaper loads, unloads and
        reloads plugins during scanning) unloaded and reloaded the DLL, re-ran
        its static initialisation and re-applied the certificate. Unloading a
        DSP library underneath a host that may still call into it is a far worse
        outcome than the handful of bytes and one module reference that leak at
        exit, which the OS reclaims anyway. Freeing it at plugin-unload time is
        not an option either: FreeLibrary must never be called from DllMain, so
        there is no safe hook for it. */
    static const LevelApiLibrary& getInstance();

    /** True when the DLL loaded AND every function the plugin calls resolved. */
    bool isLoaded() const noexcept { return loaded_; }

    /** Human-readable reason for the most recent failure; empty on success. */
    const juce::String& getLastError() const noexcept { return lastError_; }

    /** Holds the resolved LevelAPI2_FunctionHandles and the module handle.
        Opaque here so this header stays free of <windows.h> and the C API. */
    struct Impl;
    const Impl* getImpl() const noexcept { return impl_.get(); }

private:
    LevelApiLibrary();
    ~LevelApiLibrary();

    std::unique_ptr<Impl> impl_;
    juce::String lastError_;
    bool loaded_ = false;

    JUCE_DECLARE_NON_COPYABLE (LevelApiLibrary)
};

//==============================================================================
/** One realtime adjuster instance, obtained from the runtime-loaded library.

    Deliberately mirrors the static RealtimeAdjuster class method-for-method,
    so that every #if __STATIC_LIB__ block in PluginProcessor stays down to a
    single construction line. That is what stops the two linking paths drifting
    apart over time.

    All methods are safe to call when construction failed; they no-op or return
    zero, and ProcessBlock() leaves the audio untouched.
*/
class DynamicRealtimeAdjuster
{
public:
    /** The internal preset, hard-coded to Medium plugin-side. The C API takes it
        as a plain unsigned (the C++ enum isn't visible across the DLL boundary);
        Medium == 1. */
    static constexpr unsigned int PresetMedium = 1;

    /** Argument order matches the static RealtimeAdjuster constructor. The C
        API's Create takes neither preset nor target, so both are applied
        immediately afterwards.

        bitdepth is the word length the HOST quantises the output to, not the
        depth of the incoming audio: it drives the engine's dither. 0 means a
        float host, so no dither. */
    DynamicRealtimeAdjuster (double sampleRate, int numChannels, unsigned int preset, float targetLufs,
                             unsigned int bitdepth);
    ~DynamicRealtimeAdjuster();

    /** False when the library or the object could not be created -- the caller
        should drop this instance and fall back to pass-through. */
    bool isValid() const noexcept { return object_ != nullptr; }

    /** Why isValid() is false -- either the library's load-time error or the
        library's own message from a refused Create. Empty when all is well. */
    const juce::String& getLastError() const noexcept { return lastError_; }

    //==========================================================================
    // Mirrors RealtimeAdjuster. Same names, same argument order.
    // Reset allocates inside the library: call it off the audio thread.
    void Reset (int numChannels, double sampleRate, unsigned int bitdepth);
    void SetTargetLUFS (float targetLufs);
    void ProcessBlock (juce::AudioBuffer<float>& buffer);

    /** The engine's delay from input to output, in samples. Constant from
        construction or Reset until the next Reset; report it to the host. */
    int GetLatencySamples() const;

    float GetLastAppliedGainDb() const;

    /** How far the engine's peak control held the programme down during the
        last block, in dB, 0 or more. */
    float GetGainReductionDb() const;

    /** Warmup reporting, all in seconds. The *Left pair counts down to 0 and
        stays there, so "<= 0" is the done test; the *Max pair reports the
        configured length, which is 0 when the preset has no warmup at all. */
    float GetWarmupSecondsLeft() const;
    float GetWarmupSecondsMax() const;
    float GetCalibrationSecondsLeft() const;
    float GetCalibrationSecondsMax() const;
    float GetLUFS() const;
    float GetPSR() const;
    float GetLRA() const;
    float GetBS1771() const;

private:
    // Resolved function table, cached at construction. Non-owning, and safe to
    // hold indefinitely: the library outlives every plugin instance by design
    // (see LevelApiLibrary::getInstance). Null unless construction succeeded,
    // and every use below is guarded by object_ != nullptr, which implies it.
    const LevelApiLibrary::Impl* api_ = nullptr;
    void* object_ = nullptr;     // LevelAPI2_RealtimeAdjusterPtr, opaque here
    int numChannels_ = 2;
    juce::String lastError_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DynamicRealtimeAdjuster)
};

#endif // __STATIC_LIB__ == 0
