#pragma once

#include <JuceHeader.h>

/*
    Palette -- the single source of truth for every green in LevelAnchor's UI,
    plus the status and peak accents. Header-only. Used by: ui/TargetStrip.h,
    ui/AboutPanel.h, ui/PixelScreen.h, ui/MatrixReadout.h, ui/PeakLed.h,
    PluginEditor.cpp.

    Two coordinated families, each a tonal ramp off ONE base colour, so
    retuning is a one-line edit here rather than hunting hex values across
    files:

      - Brand family (kBrand) -- the mark itself: logo, target strip,
        wordmark, trace line. Hue ~130 (emerald).
      - Phosphor family (kPhosphor) -- the LCD instrument: PixelScreen and
        MatrixReadout backgrounds/readouts/status lamp. Deliberately a
        different, more yellow-green hue for its own "LCD" character, but
        derived from kBrand by a fixed offset rather than hand-picked
        independently, so it stays a controlled ~15 degrees off brand. Widen
        that gap to ~35-40 degrees and the small LCD windows stop reading as
        the same green as the logo/slider.

    brighter()/darker() preserve hue exactly, so every tone below shares its
    family's hue and differs only in brightness. Tune a family at its base
    colour or hue offset; tune one tone at its own amount.
*/
namespace Palette
{
    // --- Brand family: the mark -- logo, slider, wordmark, trace line -------
    inline const juce::Colour kBrand { 0xff3bc452 };

    // Target-strip LED bands.
    inline const juce::Colour kHot   = kBrand.brighter(0.55f); // current-value LED
    inline const juce::Colour kLit   = kBrand.darker(0.45f);   // filled below the value
    inline const juce::Colour kUnlit = kBrand.darker(0.92f).withSaturation(0.30f);

    // --- Phosphor family: the LCD instrument (PixelScreen, MatrixReadout) ---
    inline const juce::Colour kPhosphor = kBrand.withHue(kBrand.getHue() - (15.0f / 360.0f));

    inline const juce::Colour kLcdBackground = kPhosphor.darker(14.0f);  // screen/window fill
    inline const juce::Colour kLcdIdle       = kPhosphor.darker(6.0f);   // unlit idle-pixel texture
    inline const juce::Colour kLcdBevel      = kPhosphor.darker(3.0f);   // MatrixReadout window border
    inline const juce::Colour kLcdReadout    = kPhosphor.brighter(0.55f); // gain/value readout text
    inline const juce::Colour kLcdStatusGood = kPhosphor.brighter(0.15f); // status-lamp green endpoint

    // Status-lamp ramp (green -> orange -> red). Orange/red aren't part of
    // either green family, so they stay independent, but live here too so the
    // whole ramp is one thing to tune.
    inline const juce::Colour kStatusOrange { 0xffffa040 };
    inline const juce::Colour kStatusRed    { 0xffff4040 };

    // --- Peak accent: the engine's peak stage is holding the level down ------
    // The one colour that means "overshoot guard / compressor / limiter
    // acting": the top-bar PeakLed and the gain-trace columns where it acted.
    // Kept yellower than kStatusOrange on purpose, because orange belongs to
    // the deviation lamp and the two must not read as the same signal.
    inline const juce::Colour kPeak    { 0xffffc23a };
    inline const juce::Colour kPeakDim = kPeak.darker(1.2f);                        // LED, light action
    inline const juce::Colour kPeakOff = kPeak.darker(5.0f).withSaturation(0.35f); // LED, unlit

    // Deviation trace (PixelScreen's upper band). Deliberately outside both
    // green families -- the plugin's palette is green/white/black (plus the
    // status and peak accents), and white
    // is what distinguishes the deviation history line from the brand-green
    // gain trace sharing the same screen.
    //
    // CRITICAL: the white/black below are written as literals, NOT as
    // juce::Colours::white / ::black. Those are namespace-scope const Colour
    // objects defined in juce_Colours.h, so each translation unit gets its own
    // dynamically-initialized copy -- while these Palette entries are inline
    // variables whose single initializer the linker picks from one TU's COMDAT.
    // Which of the two runs first depends on link layout, and when Palette lost
    // that race the colours read as zero-initialized = fully TRANSPARENT, so
    // the deviation trace, the zero-gain row and the centre line silently
    // vanished from the LCD -- reliably after an incremental build, fixed by a
    // full rebuild. Colour(uint32) is a plain function call and has no such
    // ordering dependency. Don't "tidy" these back.
    inline const juce::Colour kDevTrace     { 0xffffffff };
    inline const juce::Colour kGainZeroLine = kDevTrace.darker(0.85f); // dim white, like kLcdCenterLine below

    // Range-band centre line (PixelScreen's +/-1 LU deviation range). Plain
    // black rather than derived from a green family -- it needs to read as a
    // hard cut against the range band's tinted fill on either side, not blend
    // with it.
    inline const juce::Colour kLcdCenterLine { 0xff000000 };
}
