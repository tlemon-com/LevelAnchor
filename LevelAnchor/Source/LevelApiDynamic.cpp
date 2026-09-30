/*
    LevelApiDynamic.cpp
    -------------------
    Implementation of the runtime-loaded (dynamic) LevelAPI2 path.
    See LevelApiDynamic.h for what this is and why it is commented so heavily.

    This is the only translation unit that includes LevelAPI2_dynlib.h. That
    header brings <windows.h> with it and defines its loader functions in place
    (InitFunctions, LevelAPI2_LoadDLL, LevelAPI2_UnloadDLL), so keeping it here
    keeps both out of every other file.
*/

#include "LevelApiDynamic.h"

#if __STATIC_LIB__ == 0

// CRITICAL include order. LevelAPI2_dynlib.h pulls in <windows.h> and, under
// UNICODE (which the Projucer's VS exporter defines), its ConvertToTString
// helper calls strlen/malloc/free without including the headers that declare
// them. So: <JuceHeader.h> first -- it arrives via LevelApiDynamic.h above, so
// JUCE's own guarded <windows.h> wins and the header's include is a no-op --
// then the two C headers, then the library header itself.
#include <string.h>
#include <stdlib.h>
#include "LevelAPI2_dynlib.h"

namespace
{
    /** The DLL filename for this build. Matches the naming used by the
        #pragma comment(lib, ...) block in PluginProcessor.cpp -- JUCE_DEBUG is
        JUCE's portable spelling of the same condition. */
    const char* getDllName() noexcept
    {
       #if JUCE_DEBUG
        return "LevelAPI2-x64_d.dll";
       #else
        return "LevelAPI2-x64.dll";
       #endif
    }

    /** The folder the plugin binary itself lives in.
        NOT the working directory (which the library's own main.c example uses):
        inside a host like OBS that would be the host's directory, not ours.
        currentExecutableFile resolves to this DLL, not to the host .exe. */
    juce::File getPluginDirectory()
    {
        return juce::File::getSpecialLocation (juce::File::currentExecutableFile)
                   .getParentDirectory();
    }

    /** Where to look for the DLL, in order.

        1. LEVELAPI2_DLL -- a development override, either the DLL itself or the
           folder holding it. Nothing in a shipping build depends on it; it is
           here so you can point at a LevelAPI2 build output directly instead of
           copying the DLL around while integrating.
        2. Next to the plugin binary. This is the deployment answer: ship the
           DLL alongside the plugin and it is found with no configuration.
        3. The bare filename, which hands the search to Windows -- that covers
           the host application's own folder and anything on PATH. */
    juce::StringArray getDllCandidates (const juce::File& pluginDir, const char* dllName)
    {
        juce::StringArray candidates;

        const juce::String fromEnv = juce::SystemStats::getEnvironmentVariable ("LEVELAPI2_DLL", {})
                                         .unquoted().trim();

        if (fromEnv.isNotEmpty())
        {
            if (juce::File::isAbsolutePath (fromEnv))
            {
                const juce::File f (fromEnv);

                candidates.add (f.isDirectory() ? f.getChildFile (dllName).getFullPathName()
                                                : f.getFullPathName());
            }
            else
            {
                candidates.add (fromEnv); // relative: let LoadLibrary resolve it
            }
        }

        candidates.add (pluginDir.getChildFile (dllName).getFullPathName());
        candidates.add (juce::String (dllName));

        return candidates;
    }

    /** Reads the library's own explanation of the last failure, if it has one. */
    juce::String fetchErrorString (const LevelAPI2_FunctionHandles& fn)
    {
        if (fn.GetErrorString == nullptr)
            return {};

        char buffer[1024] = {};

        if (! fn.GetErrorString (buffer, sizeof (buffer)))
            return {};

        return juce::String::fromUTF8 (buffer).trim();
    }

    /** Returns the name of the first function the plugin needs that the loaded
        DLL did not export, or nullptr when everything resolved.

        Worth doing explicitly: LevelAPI2_LoadDLL only null-checks
        Create_LevelAPI2, so a DLL predating the RealtimeAdjuster_* exports
        "loads successfully" and then crashes on the first call.

        Only what the plugin actually calls is listed. RealtimeAdjuster_Adjust
        (planar) is required; its interleaved sibling is not, because nothing
        here calls it -- refusing a DLL over an unused export would be wrong. */
    const char* findMissingFunction (const LevelAPI2_FunctionHandles& fn)
    {
        if (fn.Create_RealtimeAdjuster == nullptr)                return "Create_RealtimeAdjuster";
        if (fn.Destroy_RealtimeAdjuster == nullptr)               return "Destroy_RealtimeAdjuster";
        if (fn.RealtimeAdjuster_Adjust == nullptr)                return "RealtimeAdjuster_Adjust";
        if (fn.RealtimeAdjuster_Reset == nullptr)                 return "RealtimeAdjuster_Reset";
        if (fn.RealtimeAdjuster_SetPreset == nullptr)             return "RealtimeAdjuster_SetPreset";
        if (fn.RealtimeAdjuster_SetTargetLUFS == nullptr)         return "RealtimeAdjuster_SetTargetLUFS";
        if (fn.RealtimeAdjuster_GetLatencySamples == nullptr)     return "RealtimeAdjuster_GetLatencySamples";
        if (fn.RealtimeAdjuster_GetLastAppliedGainDb == nullptr)  return "RealtimeAdjuster_GetLastAppliedGainDb";
        if (fn.RealtimeAdjuster_GetGainReductionDb == nullptr)    return "RealtimeAdjuster_GetGainReductionDb";
        if (fn.RealtimeAdjuster_GetWarmupSecondsLeft == nullptr)  return "RealtimeAdjuster_GetWarmupSecondsLeft";
        if (fn.RealtimeAdjuster_GetWarmupSecondsMax == nullptr)   return "RealtimeAdjuster_GetWarmupSecondsMax";
        if (fn.RealtimeAdjuster_GetCalibrationSecondsLeft == nullptr)  return "RealtimeAdjuster_GetCalibrationSecondsLeft";
        if (fn.RealtimeAdjuster_GetCalibrationSecondsMax == nullptr)   return "RealtimeAdjuster_GetCalibrationSecondsMax";
        if (fn.RealtimeAdjuster_GetLUFS == nullptr)               return "RealtimeAdjuster_GetLUFS";
        if (fn.RealtimeAdjuster_GetPSR == nullptr)                return "RealtimeAdjuster_GetPSR";
        if (fn.RealtimeAdjuster_GetLRA == nullptr)                return "RealtimeAdjuster_GetLRA";
        if (fn.RealtimeAdjuster_GetBS1771 == nullptr)             return "RealtimeAdjuster_GetBS1771";

        return nullptr;
    }

    /** Certificate handling, three tiers in order:
          1. LEVELANCHOR_CERTIFICATE_STR defined at build time -> use it.
          2. certificate.txt sitting next to the plugin binary.
          3. neither -> don't call SetCertificate at all and let the library
             decide. Against a DLL that requires one, Create_RealtimeAdjuster
             then fails and says so; against one that doesn't, this is the
             normal path. Making no compile-time assumption either way is what
             lets a single binary cover both.

        SetCertificate is a global (not per-object) call, hence doing it here in
        the loader, once per process. A rejection is recorded but not treated as
        fatal -- Create_RealtimeAdjuster gets the final say. */
    void applyCertificate (const LevelAPI2_FunctionHandles& fn,
                           const juce::File& pluginDir,
                           juce::String& errorOut)
    {
        if (fn.SetCertificate == nullptr)
            return;

       #ifdef LEVELANCHOR_CERTIFICATE_STR
        // ---- Tier 1: certificate compiled in -------------------------------
        // Licensees: this is the slot to paste yours into. Obscuring it in your
        // own source, rather than leaving it in plain text, is advised.
        const char* const certificate = LEVELANCHOR_CERTIFICATE_STR;

        if (! fn.SetCertificate (certificate, strlen (certificate)))
            errorOut = "LevelAPI2 rejected the built-in certificate. "
                       + fetchErrorString (fn);
       #else
        // ---- Tier 2: certificate.txt beside the plugin ----------------------
        const juce::File certFile = pluginDir.getChildFile ("certificate.txt");

        if (certFile.existsAsFile())
        {
            const juce::String certificate = certFile.loadFileAsString().trim();

            if (certificate.isNotEmpty()
                 && ! fn.SetCertificate (certificate.toRawUTF8(),
                                         (size_t) certificate.getNumBytesAsUTF8()))
            {
                errorOut = "LevelAPI2 rejected " + certFile.getFullPathName()
                           + ". " + fetchErrorString (fn);
            }
        }
        // ---- Tier 3: nothing to do ------------------------------------------
       #endif
    }
}

//==============================================================================
struct LevelApiLibrary::Impl
{
    void* dllHandle = nullptr;
    LevelAPI2_FunctionHandles fn {};
};

const LevelApiLibrary& LevelApiLibrary::getInstance()
{
    // Allocated once, never deleted -- see the CRITICAL note on the declaration.
    // A function-local static gives thread-safe one-time construction, and using
    // a pointer means the object is never destroyed at exit, so the DLL is never
    // unloaded while some other thread might still be inside it.
    static const LevelApiLibrary* instance = new LevelApiLibrary();
    return *instance;
}

LevelApiLibrary::LevelApiLibrary()
    : impl_ (std::make_unique<Impl>())
{
    const juce::File pluginDir = getPluginDirectory();
    const char* const dllName = getDllName();

    const juce::StringArray candidates = getDllCandidates (pluginDir, dllName);

    for (const auto& path : candidates)
    {
        impl_->dllHandle = LevelAPI2_LoadDLL (path.toRawUTF8(), &impl_->fn);

        DBG ("LevelAnchor: " + juce::String (impl_->dllHandle != nullptr ? "loaded " : "no luck with ") + path);

        if (impl_->dllHandle != nullptr)
            break;
    }

    if (impl_->dllHandle == nullptr)
    {
        // Deliberately vague, because LevelAPI2_LoadDLL is: it returns the same
        // nothing whether the file is absent, one of ITS dependencies is
        // missing, or it loaded but exports no Create_LevelAPI2.
        lastError_ = juce::String (dllName) + " could not be loaded from any of: "
                     + candidates.joinIntoString (", ")
                     + ". The file may be absent, a dependency of it may be missing, or it may not "
                       "export Create_LevelAPI2 -- the library's loader reports all three alike.";
        return;
    }

    const int revision = (impl_->fn.GetRevision != nullptr ? impl_->fn.GetRevision() : -1);

    if (const char* missing = findMissingFunction (impl_->fn))
    {
        // Distinguish a genuinely old library from an export problem. If the core
        // LevelAPI2 functions resolved but NONE of the RealtimeAdjuster ones did,
        // the likelier cause is that they aren't exported under those plain names
        // -- GetProcAddress matches the literal string, so a C++-mangled export
        // (no extern "C" / no .def entry) is invisible to it.
        const bool noAdjusterExports = impl_->fn.Create_RealtimeAdjuster == nullptr
                                    && impl_->fn.Destroy_RealtimeAdjuster == nullptr
                                    && impl_->fn.RealtimeAdjuster_Reset == nullptr
                                    && impl_->fn.RealtimeAdjuster_AdjustInterleaved == nullptr;

        lastError_ = juce::String (dllName) + " loaded (revision " + juce::String (revision) + ") but "
                     + (noAdjusterExports
                            ? juce::String ("exports none of the RealtimeAdjuster_* functions. Either it "
                                            "predates that C API, or they are not exported with C linkage "
                                            "(extern \"C\" + __declspec(dllexport), or a .def entry).")
                            : juce::String ("is missing ") + missing + ", so it is older than this plugin expects.");

        LevelAPI2_UnloadDLL (impl_->dllHandle);
        impl_->dllHandle = nullptr;
        return;
    }

    DBG ("LevelAnchor: LevelAPI2 revision " + juce::String (revision));

    // May leave a non-fatal note in lastError_ even though we go on to report
    // isLoaded() == true; see applyCertificate.
    applyCertificate (impl_->fn, pluginDir, lastError_);

    loaded_ = true;
}

LevelApiLibrary::~LevelApiLibrary()
{
    // Never actually runs: getInstance() leaks the single instance deliberately.
    // Defined because unique_ptr<Impl> needs a complete type here, and left
    // empty because unloading the DLL is precisely what we are avoiding. The
    // only FreeLibrary in this file is the controlled one in the constructor's
    // failure path, before anything has used the library.
}

//==============================================================================
DynamicRealtimeAdjuster::DynamicRealtimeAdjuster (double sampleRate, int numChannels,
                                                  unsigned int preset, float targetLufs,
                                                  unsigned int bitdepth)
    : numChannels_ (juce::jlimit (1, (int) max_channels, numChannels))
{
    const LevelApiLibrary& lib = LevelApiLibrary::getInstance();

    if (! lib.isLoaded())
    {
        lastError_ = lib.getLastError();
        return;
    }

    // A host can call prepareToPlay before it knows the real rate -- Reaper does
    // during its plugin scan. Creating the engine with a nonsense rate would
    // configure every internal meter wrongly, so refuse and let the next
    // prepareToPlay (with real values) build it instead.
    const int roundedRate = juce::roundToInt (sampleRate);

    if (roundedRate <= 0)
    {
        lastError_ = "Refusing to create the adjuster at sample rate "
                     + juce::String (sampleRate) + ".";
        return;
    }

    api_ = lib.getImpl();

    const auto& fn = api_->fn;

    // Logged because this crosses the C boundary, where the argument order is
    // channels-first -- the reverse of the C++ class. If the engine ever comes
    // up misconfigured, this line says exactly what went in.
    DBG ("LevelAnchor: Create_RealtimeAdjuster(channels=" + juce::String (numChannels_)
          + ", samplerate=" + juce::String (roundedRate)
          + ", bitdepth=" + juce::String ((int) bitdepth) + ")");

    object_ = fn.Create_RealtimeAdjuster ((unsigned int) numChannels_,
                                          (unsigned int) roundedRate,
                                          bitdepth);

    if (object_ == nullptr)
    {
        // Most likely a certificate the library wouldn't accept; it can say so.
        lastError_ = "LevelAPI2 refused to create the adjuster. " + fetchErrorString (fn);
        return;
    }

    // The C constructor takes neither the preset nor the target (unlike the C++
    // class), so both are applied straight afterwards.
    fn.RealtimeAdjuster_SetPreset (object_, preset);
    fn.RealtimeAdjuster_SetTargetLUFS (object_, targetLufs);

    // Then Reset, which the static path also does immediately after construction.
    // Redundant in principle -- the constructor already configured everything --
    // but mirroring the working path exactly removes one difference between the
    // two, and Reset re-Inits every internal meter from the values below.
    Reset (numChannels_, sampleRate, bitdepth);
}

DynamicRealtimeAdjuster::~DynamicRealtimeAdjuster()
{
    if (object_ != nullptr)
        api_->fn.Destroy_RealtimeAdjuster (object_);
}

void DynamicRealtimeAdjuster::Reset (int numChannels, double sampleRate, unsigned int bitdepth)
{
    if (object_ == nullptr)
        return;

    numChannels_ = juce::jlimit (1, (int) max_channels, numChannels);

    api_->fn.RealtimeAdjuster_Reset (object_,
                                                (unsigned int) numChannels_,
                                                (unsigned int) juce::roundToInt (sampleRate),
                                                bitdepth);
}

void DynamicRealtimeAdjuster::SetTargetLUFS (float targetLufs)
{
    if (object_ != nullptr)
        api_->fn.RealtimeAdjuster_SetTargetLUFS (object_, targetLufs);
}

void DynamicRealtimeAdjuster::ProcessBlock (juce::AudioBuffer<float>& buffer)
{
    if (object_ == nullptr)
        return; // no engine -> audio passes through untouched

    const auto& fn = api_->fn;

    const int numFrames = buffer.getNumSamples();
    const int numCh = juce::jmin (buffer.getNumChannels(), numChannels_);

    if (numFrames <= 0 || numCh <= 0)
        return;

    // JUCE buffers are already planar, so this hands the library our channel
    // pointers directly -- no copy at all. RealtimeAdjuster_Adjust is known to
    // be non-null: findMissingFunction refuses to load a DLL without it.
    // const_cast is safe here: JUCE returns float* const* (the array of
    // pointers is const, the samples are not) and the C API asks for float**;
    // neither side writes to the pointer array itself.
    float** channels = const_cast<float**> (buffer.getArrayOfWritePointers());

    // The C API accepts at most max_frames per call. Live hosts never come
    // close, but an offline render can, so chunk rather than trust it.
    for (int start = 0; start < numFrames; start += (int) max_frames)
    {
        const int chunk = juce::jmin<int> ((int) max_frames, numFrames - start);

        float* offsetChannels[max_channels];

        for (int ch = 0; ch < numCh; ++ch)
            offsetChannels[ch] = channels[ch] + start;

        fn.RealtimeAdjuster_Adjust (object_, offsetChannels, (unsigned int) chunk);
    }
}

int DynamicRealtimeAdjuster::GetLatencySamples() const
{
    // No engine means audio passes straight through, which adds no delay.
    return object_ != nullptr ? (int) api_->fn.RealtimeAdjuster_GetLatencySamples (object_) : 0;
}

float DynamicRealtimeAdjuster::GetLastAppliedGainDb() const
{
    return object_ != nullptr ? api_->fn.RealtimeAdjuster_GetLastAppliedGainDb (object_) : 0.0f;
}

float DynamicRealtimeAdjuster::GetGainReductionDb() const
{
    return object_ != nullptr ? api_->fn.RealtimeAdjuster_GetGainReductionDb (object_) : 0.0f;
}

// With no engine these report 0, i.e. "nothing left to wait for" -- the same
// answer the caller gets from a preset with no warmup, and the honest one when
// audio is passing through untouched.
float DynamicRealtimeAdjuster::GetWarmupSecondsLeft() const
{
    return object_ != nullptr ? api_->fn.RealtimeAdjuster_GetWarmupSecondsLeft (object_) : 0.0f;
}

float DynamicRealtimeAdjuster::GetWarmupSecondsMax() const
{
    return object_ != nullptr ? api_->fn.RealtimeAdjuster_GetWarmupSecondsMax (object_) : 0.0f;
}

float DynamicRealtimeAdjuster::GetCalibrationSecondsLeft() const
{
    return object_ != nullptr ? api_->fn.RealtimeAdjuster_GetCalibrationSecondsLeft (object_) : 0.0f;
}

float DynamicRealtimeAdjuster::GetCalibrationSecondsMax() const
{
    return object_ != nullptr ? api_->fn.RealtimeAdjuster_GetCalibrationSecondsMax (object_) : 0.0f;
}

float DynamicRealtimeAdjuster::GetLUFS() const
{
    return object_ != nullptr ? api_->fn.RealtimeAdjuster_GetLUFS (object_) : 0.0f;
}

float DynamicRealtimeAdjuster::GetPSR() const
{
    return object_ != nullptr ? api_->fn.RealtimeAdjuster_GetPSR (object_) : 0.0f;
}

float DynamicRealtimeAdjuster::GetLRA() const
{
    return object_ != nullptr ? api_->fn.RealtimeAdjuster_GetLRA (object_) : 0.0f;
}

float DynamicRealtimeAdjuster::GetBS1771() const
{
    return object_ != nullptr ? api_->fn.RealtimeAdjuster_GetBS1771 (object_) : 0.0f;
}

#endif // __STATIC_LIB__ == 0
