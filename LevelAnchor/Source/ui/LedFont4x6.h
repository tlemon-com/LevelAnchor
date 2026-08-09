#pragma once

#include <cstdint>

/*
    LedFont4x6 -- the small dot-matrix glyph table for PixelScreen's in-band
    labels. Header-only, static data. Sibling of ui/LedFont5x7.h (same layout,
    same consumer: ui/PixelScreen.h).

    Why a second table rather than a scale factor: these are bitmaps drawn one
    lit pixel per screen cell, so "smaller" can only mean fewer cells -- there
    is no fractional size between 5x7 and this. Used for the two in-band
    readouts (gain, deviation), which annotate the lines they sit next to and
    must not shout over them; 5x7 stays the voice of the centred status
    messages.

    Each glyph is 6 rows of 4-bit masks, bit 3 = leftmost pixel. Digits and
    ascenders fill rows 0..5; lowercase x-height is rows 2..5; 'g' shifts up a
    row to make room for its descender. Alphabet is deliberately partial (only
    what the two readouts need: digits, "+-. ", the units "dB"/"LUFS", and the
    captions "gain"/"diff") -- getGlyph() returns nullptr for anything else and
    PixelScreen renders that as a blank cell, so add glyphs here before using a
    new character.
*/
namespace LedFont4x6
{
    constexpr int kWidth   = 4;
    constexpr int kHeight  = 6;
    constexpr int kAdvance = 5; // glyph width + 1 column spacing

    struct Glyph
    {
        char    ch;
        uint8_t row[6];
    };

    inline const Glyph* getGlyph(char c)
    {
        static const Glyph glyphs[] =
        {
            { '0', { 0x6, 0x9, 0x9, 0x9, 0x9, 0x6 } },
            { '1', { 0x4, 0xC, 0x4, 0x4, 0x4, 0xE } },
            { '2', { 0x6, 0x9, 0x1, 0x2, 0x4, 0xF } },
            { '3', { 0xE, 0x1, 0x6, 0x1, 0x1, 0xE } },
            { '4', { 0x9, 0x9, 0x9, 0xF, 0x1, 0x1 } },
            { '5', { 0xF, 0x8, 0xE, 0x1, 0x9, 0x6 } },
            { '6', { 0x6, 0x8, 0xE, 0x9, 0x9, 0x6 } },
            { '7', { 0xF, 0x1, 0x2, 0x2, 0x4, 0x4 } },
            { '8', { 0x6, 0x9, 0x6, 0x9, 0x9, 0x6 } },
            { '9', { 0x6, 0x9, 0x9, 0x7, 0x1, 0x6 } },
            // Sign glyphs are 3 cells wide, not 4: a 4-wide bar has no centre
            // column for the '+' stem to sit on, so it would lean.
            { '+', { 0x0, 0x4, 0xE, 0x4, 0x0, 0x0 } },
            { '-', { 0x0, 0x0, 0xE, 0x0, 0x0, 0x0 } },
            { '.', { 0x0, 0x0, 0x0, 0x0, 0x6, 0x6 } },
            { ' ', { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 } },
            // Unit suffixes "dB" and "LUFS".
            { 'd', { 0x1, 0x1, 0x7, 0x9, 0x9, 0x7 } },
            { 'B', { 0xE, 0x9, 0xE, 0x9, 0x9, 0xE } },
            { 'L', { 0x8, 0x8, 0x8, 0x8, 0x8, 0xF } },
            { 'U', { 0x9, 0x9, 0x9, 0x9, 0x9, 0x6 } },
            { 'F', { 0xF, 0x8, 0x8, 0xE, 0x8, 0x8 } },
            { 'S', { 0x7, 0x8, 0x6, 0x1, 0x1, 0xE } },
            // The "gain" and "diff" captions themselves ('d' and 'i' come from
            // the unit letters above; 'f' is an ascender, crossbar at x-height).
            { 'g', { 0x0, 0x7, 0x9, 0x7, 0x1, 0x6 } },
            { 'a', { 0x0, 0x0, 0x6, 0x9, 0x9, 0x7 } },
            { 'i', { 0x4, 0x0, 0x4, 0x4, 0x4, 0x4 } },
            { 'n', { 0x0, 0x0, 0xE, 0x9, 0x9, 0x9 } },
            { 'e', { 0x0, 0x0, 0x6, 0xF, 0x8, 0x7 } },
            { 'v', { 0x0, 0x0, 0x9, 0x9, 0x6, 0x6 } },
            { 'f', { 0x6, 0x4, 0xE, 0x4, 0x4, 0x4 } },
        };

        for (const auto& g : glyphs)
            if (g.ch == c)
                return &g;
        return nullptr;
    }

    /** Pixel-cell width of a rendered string (no trailing spacing column). */
    inline int textWidthCells(const char* text)
    {
        int n = 0;
        for (const char* p = text; *p != 0; ++p)
            ++n;
        return n > 0 ? n * kAdvance - 1 : 0;
    }
}
