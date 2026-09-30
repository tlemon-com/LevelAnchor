#pragma once

#include <JuceHeader.h>
#include "Palette.h"

/*
    PeakLed -- the amber LED in the top bar that lights while the engine's peak
    stage (overshoot guard, compressor, true-peak limiter) is holding the
    programme down. Header-only. Siblings: ui/PixelScreen.h (marks the same
    moments on its gain trace), ui/Palette.h.

    A light, not a number: it answers "is the peak stage acting?", and the
    screen already carries the figures. Three states, and the editor picks
    which (thresholds and hold time live in PluginEditor.cpp). Repaints only
    when the state changes.

    Round, so never setOpaque(true): the corners outside the bezel must show
    the editor background (see the note in ui/MatrixReadout.h).
*/
class PeakLed : public juce::Component
{
public:
    enum Level { Off = 0, Light = 1, Heavy = 2 };

    PeakLed() { setInterceptsMouseClicks(false, false); }

    void setLevel(int newLevel)
    {
        newLevel = juce::jlimit<int>(Off, Heavy, newLevel);
        if (newLevel == level)
            return;
        level = newLevel;
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        const auto b     = getLocalBounds().toFloat();
        const auto bezel = b.withSizeKeepingCentre(juce::jmin(b.getWidth(), b.getHeight()),
                                                   juce::jmin(b.getWidth(), b.getHeight()));
        const auto lens  = bezel.reduced(kBezelPx);

        // Bezel: the same recessed fill and edge as the LCD windows, so the LED
        // reads as part of the instrument rather than a loose dot.
        g.setColour(Palette::kLcdBackground);
        g.fillEllipse(bezel);
        g.setColour(Palette::kLcdBevel);
        g.drawEllipse(bezel.reduced(0.5f), 1.0f);

        g.setColour(level == Heavy ? Palette::kPeak
                  : level == Light ? Palette::kPeakDim
                                   : Palette::kPeakOff);
        g.fillEllipse(lens);

        // Small glint, lit states only, so an unlit LED stays flat and quiet.
        if (level != Off)
        {
            const float s = lens.getWidth() * 0.35f;
            g.setColour(juce::Colour(0x59ffffff));
            g.fillEllipse(lens.getX() + lens.getWidth() * 0.22f,
                          lens.getY() + lens.getHeight() * 0.18f, s, s);
        }
    }

private:
    static constexpr float kBezelPx = 2.0f; // ring of recessed fill around the lens

    int level = Off;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PeakLed)
};
