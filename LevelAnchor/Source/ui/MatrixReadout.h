#pragma once

#include <JuceHeader.h>
#include "LedFont5x7.h"
#include "Palette.h"

/*
    MatrixReadout -- small LCD window for a single value (timer, target,
    output level). Header-only. Siblings: ui/LedFont5x7.h (glyphs),
    ui/PixelScreen.h (the large screen this deliberately mirrors).

    Same 5x7 font and same 2px cell pitch as PixelScreen, so the small windows
    read as the same instrument as the big one rather than as separate widgets.
    Draws its own recessed bezel (dark inset fill + border) -- the editor just
    positions it.

    Sizing: pass the widest string the readout will ever show to
    preferredSize(), e.g. "-99.9 LUFS". The box is then fixed at that width
    and shorter strings stay centred inside it, so the layout never reflows
    as values change.
    LedFont5x7's alphabet is partial -- add a glyph there before using a new
    character here, or it renders blank.
*/
class MatrixReadout : public juce::Component
{
public:
    MatrixReadout()
    {
        // NOT opaque: paint() fills a rounded rect, so the four corner pixels
        // are never drawn. Declaring the component opaque tells JUCE to skip
        // clearing underneath it, which leaves stale framebuffer content in
        // those corners -- they flick between rounded and sharp as the value
        // updates and only this component repaints.
        setInterceptsMouseClicks(false, false);
    }

    /** Preferred component size for the widest string this readout will show.
        Call once at construction, then size the component to the result. */
    static juce::Point<int> preferredSize(const char* widestText)
    {
        const int wCells = LedFont5x7::textWidthCells(widestText);
        return { wCells * kCell + 2 * kPadX,
                 LedFont5x7::kHeight * kCell + 2 * kPadY };
    }

    /** Text to display. No-op unless it actually changed. */
    void setText(const juce::String& text)
    {
        if (text == current)
            return;
        current = text;
        repaint();
    }

    const juce::String& getText() const noexcept { return current; }

    /** Dim the lit pixels (e.g. while the meter is still warming up). */
    void setDimmed(bool shouldDim)
    {
        if (shouldDim == dimmed)
            return;
        dimmed = shouldDim;
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        const auto bounds = getLocalBounds().toFloat();

        // Recessed window: dark fill, slightly darker top edge, lighter border.
        g.setColour(kBackground);
        g.fillRoundedRectangle(bounds, kCorner);

        g.setColour(kBevel);
        g.drawRoundedRectangle(bounds.reduced(0.5f), kCorner, 1.0f);

        drawText(g);
    }

private:
    // --- Tunables -----------------------------------------------------------
    static constexpr int   kCell   = 2;  // px per cell -- must match PixelScreen::kCell
    static constexpr int   kPadX   = 8;  // inner margin left/right
    static constexpr int   kPadY   = 5;  // inner margin top/bottom
    static constexpr float kCorner = 3.0f;

    // --- Palette ------------------------------------------------------------
    // Phosphor family (ui/Palette.h) -- shares PixelScreen's greens by
    // construction, not by duplicated hex, so the small windows and the
    // big screen can't drift apart.
    const juce::Colour kBackground { Palette::kLcdBackground };
    const juce::Colour kBevel      { Palette::kLcdBevel };
    const juce::Colour kText       { Palette::kLcdReadout };

    // --- Rendering ----------------------------------------------------------
    void drawText(juce::Graphics& g) const
    {
        if (current.isEmpty())
            return;

        const auto utf8   = current.toRawUTF8();
        const int  wCells = LedFont5x7::textWidthCells(utf8);

        // Centre within the padded interior; clamp so an over-long string
        // starts at the left margin instead of running off both edges.
        int col = juce::jmax(kPadX, (getWidth() - wCells * kCell) / 2) / kCell;
        const int topRow = ((getHeight() - LedFont5x7::kHeight * kCell) / 2) / kCell;

        g.setColour(dimmed ? kText.withAlpha(0.35f) : kText);
        for (const char* p = utf8; *p != 0; ++p, col += LedFont5x7::kAdvance)
        {
            const auto* glyph = LedFont5x7::getGlyph(*p);
            if (glyph == nullptr)
                continue;
            for (int y = 0; y < LedFont5x7::kHeight; ++y)
                for (int x = 0; x < LedFont5x7::kWidth; ++x)
                    if ((glyph->row[y] >> (LedFont5x7::kWidth - 1 - x)) & 1)
                        g.fillRect((col + x) * kCell, (topRow + y) * kCell, kCell, kCell);
        }
    }

    // --- State --------------------------------------------------------------
    juce::String current;
    bool dimmed = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MatrixReadout)
};
