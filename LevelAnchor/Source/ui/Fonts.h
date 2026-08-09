#pragma once

#include <JuceHeader.h>

/*
    Fonts -- shared custom-typeface access for LevelAnchor's wordmark.
    Header-only. Used by: PluginEditor.cpp, ui/AboutPanel.h.

    Bebas Neue (SIL OFL, embedded as BinaryData::BebasNeueRegular_ttf) is the
    "LEVEL"/"ANCHOR" wordmark typeface. Parsed once and cached in a static --
    the wordmark is drawn from more than one paint() call and there's no
    reason to re-parse the TTF each time.
*/
namespace Fonts
{
    inline juce::Font wordmark(float height)
    {
        static const juce::Typeface::Ptr typeface = juce::Typeface::createSystemTypefaceFor(
            BinaryData::BebasNeueRegular_ttf, BinaryData::BebasNeueRegular_ttfSize);

        return juce::Font(juce::FontOptions(typeface)).withHeight(height);
    }
}
