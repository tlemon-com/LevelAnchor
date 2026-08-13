#pragma once

#include <JuceHeader.h>
#include "LedFont5x7.h"
#include "LedFont4x6.h"
#include "Palette.h"

/*
    PixelScreen -- the LCD-style pixel-matrix centerpiece of the LevelAnchor UI.
    Header-only. Siblings: ui/LedFont5x7.h (big centred status messages),
    ui/LedFont4x6.h (the smaller in-band gain readout), ui/TargetStrip.h,
    ui/AboutPanel.h.

    One cell grid renders everything: idle texture, the LevelAPI watermark
    (which doubles as the status lamp), both history traces, every readout.
    It splits into two horizontal bands so the histories don't fight each
    other -- loudness deviation above, applied gain below, each with its own
    zero row, vertical scale and hard row clamp. See resized() for the split.

    Kept cheap, because the plugin runs live and usually hidden: the static
    chrome is baked into a cached backplate rebuilt only on resize or a
    status-colour step, paint() blits it and adds at most one dot per column
    per trace, and the component repaints itself only on column advance or
    status change -- the editor never force-repaints it.
*/
class PixelScreen : public juce::Component
{
public:
    PixelScreen()
    {
        logoDrawable = juce::Drawable::createFromImageData(
            BinaryData::levelapi2logo_svg, BinaryData::levelapi2logo_svgSize);
        jassert(logoDrawable != nullptr);

        setOpaque(true);
        setInterceptsMouseClicks(false, false);
    }

    /** Feed one UI tick (30 Hz). devLU = integrated LUFS minus target,
        gainDb = currently applied gain. Not warmed up -> no trace dot is
        written for either trace and the lamp stays neutral. */
    void pushTick(float devLU, float gainDb, bool warmedUp)
    {
        // --- Status lamp: green within +-1 LU, blend to red at +-3 LU ---
        const float t = warmedUp
            ? juce::jlimit(0.0f, 1.0f, (std::abs(devLU) - 1.0f) / 2.0f)
            : 0.0f;
        const int step = (int)std::round(t * (float)kStatusSteps);
        if (step != statusStep)
        {
            statusStep   = step;
            statusColour = statusColourFor((float)step / (float)kStatusSteps);
            rebuildBackplate();
            repaint();
        }

        // --- Whole-column scroll: one quantized row per column, written once ---
        if (++tickCount >= kTicksPerColumn)
        {
            tickCount = 0;
            if (cols > 0)
            {
                head = (head + 1) % cols;
                colRows[(size_t)head]     = warmedUp ? quantizeRow(devLU) : kNoDot;
                gainColRows[(size_t)head] = warmedUp ? quantizeGainRow(gainDb) : kNoDot;
                updateReadoutSides();
                repaint();
            }
        }
    }

    /** Gain readout, rendered into the gain band next to the zero-gain line
        (e.g. "gain +2.5 dB"). Cheap to call every tick: no-op unless the text
        actually changed. Empty string hides it. */
    void setGainText(const juce::String& text)
    {
        if (text == gainText)
            return;
        gainText = text;
        repaint(readoutRepaintArea(gainSlot()));
    }

    /** Deviation readout, rendered into the deviation band just clear of the
        range block (e.g. "dev -0.4 LUFS"). Same contract as setGainText. */
    void setDevText(const juce::String& text)
    {
        if (text == devText)
            return;
        devText = text;
        repaint(readoutRepaintArea(devSlot()));
    }

    /** Big centred status message under the watermark (the warmup countdown, on
        presets that have one). Separate slot from the gain readout: 5x7 font,
        screen centre, where there's room for words. Empty string hides it. */
    void setStatusText(const juce::String& text)
    {
        if (text == statusText)
            return;
        statusText = text;
        repaint(statusRepaintArea());
    }

    /** Horizontal anchor for the centered content (watermark + status message),
        in component-local pixels. The screen sits right of the target strip, so
        its own centre is off the window's centre axis; the editor passes the
        window centre here to line the logo and message up with the chrome below.
        Negative (the default) means "use the screen's own centre". The gain
        readout is right-aligned instead and doesn't use this. */
    void setContentCentreX(int localX)
    {
        if (localX == contentCentreX)
            return;
        contentCentreX = localX;
        rasterizeLogo();   // logoX derives from the anchor
        rebuildBackplate();
        repaint();
    }

    void resized() override
    {
        cols = getWidth()  / kCell;
        rows = getHeight() / kCell;

        // Split into an upper deviation band and a lower gain band (see the
        // class comment). Each band gets its own zero-row and never sees the
        // other's dots -- quantizeRow()/quantizeGainRow() clamp to their band.
        gainRows    = juce::jmax(1, (int)std::round((float)rows * kGainBandFrac));
        devRows     = juce::jmax(1, rows - gainRows);
        targetRow   = devRows / 2;
        gainZeroRow = devRows + gainRows / 2;
        gainRowsPerDb = ((float)gainRows * 0.5f) / kGainRangeDb;

        colRows.assign((size_t)juce::jmax(1, cols), kNoDot);
        gainColRows.assign((size_t)juce::jmax(1, cols), kNoDot);
        head      = 0;
        tickCount = 0;

        rasterizeLogo();
        rebuildBackplate();
    }

    void paint(juce::Graphics& g) override
    {
        if (backplate.isValid())
            g.drawImageAt(backplate, 0, 0);
        else
            g.fillAll(kBackground);

        // Deviation trace: brightest layer in the upper band, one dot per
        // column, rightmost = newest
        g.setColour(kTrace);
        for (int x = 0; x < cols; ++x)
        {
            const int age = cols - 1 - x;
            const int idx = (head - age + cols) % cols;
            const int r   = colRows[(size_t)idx];
            if (r >= 0)
                g.fillRect(x * kCell, r * kCell, 1, 1);
        }

        // Gain trace: same scheme, lower band, brand green
        g.setColour(kGainTrace);
        for (int x = 0; x < cols; ++x)
        {
            const int age = cols - 1 - x;
            const int idx = (head - age + cols) % cols;
            const int r   = gainColRows[(size_t)idx];
            if (r >= 0)
                g.fillRect(x * kCell, r * kCell, 1, 1);
        }

        // Topmost layer
        drawStatusText(g);
        drawReadout(g, devText,  devSlot(),  devAbove);
        drawReadout(g, gainText, gainSlot(), gainAbove);
    }

private:
    // --- Tunables -----------------------------------------------------------
    static constexpr int   kCell           = 2;    // px per cell, 1 lit dot per cell
    static constexpr int   kIdleRowStep    = 4;    // idle texture: every 4th row
    static constexpr float kRowsPerLU      = 8.0f; // vertical scale of the trace
    static constexpr int   kTicksPerColumn = 15;   // @30 Hz -> 2 columns/sec
    static constexpr float kLogoHeightFrac = 0.38f;   // watermark height vs screen rows
    static constexpr float kLogoBarFrac    = 0.5825f; // bar center within mark height (measured from SVG)
    static constexpr float kLogoDim        = 0.40f;   // watermark max alpha (it's a watermark)
    static constexpr int   kLogoMarginCells = 1;      // halo (in cells) kept clear of the range band
                                                      // around the logo's rasterized coverage, 2px at kCell=2
    static constexpr float kLogoMaskThreshold = 1.0f; // coverage cutoff for the halo mask ONLY -- much
                                                      // stricter than the watermark's own 0.05 render cutoff,
                                                      // else faint antialiased dust along diagonal edges gets
                                                      // treated as solid logo and dilates into a jagged,
                                                      // non-straight halo boundary
    static constexpr float kRangeLU        = 1.0f;    // range band half-width, +/-1 LUFS around target
    static constexpr float kRangeDarken    = 0.35f;   // darken amount off statusColour (vs. the logo's own tint)
    static constexpr float kRangeDim       = 0.30f;   // range band alpha, dimmer than kLogoDim so it reads behind it
    static constexpr int   kStatusSteps    = 20;      // status color quantization (limits rebuilds)
    static constexpr int   kReadoutMargin  = 47;      // rows between the centred status message's baseline and
                                                      // the screen bottom (tuned so it sits just under the
                                                      // watermark and clear of the gain band, not adrift near
                                                      // the bottom edge)
    static constexpr int   kReadoutGap    = 2;        // cells of clear space between an in-band readout's glyph
                                                      // box and the line/block it parks against, either side
    static constexpr int   kReadoutRightMargin = 4;   // cells between an in-band readout and the screen's right edge
    static constexpr float  kGainBandFrac  = 0.30f;   // fraction of rows given to the lower gain band
    static constexpr float  kGainRangeDb   = 32.0f;   // +/- range the gain band shows before clipping;
                                                      // starts equal to RealtimeAdjuster's GAIN_LIMIT_DB but
                                                      // is a separate, UI-only constant -- shrink this once
                                                      // real content shows how wide gain actually swings.
    static constexpr int   kNoDot          = -1;

    // --- Palette ------------------------------------------------------------
    // The LCD draws from Palette's phosphor family, not the brand family
    // directly -- deriving the whole screen from the brighter brand green
    // flattens its brightness bands. The one exception is the gain
    // trace, which uses the brand green deliberately so the moving element
    // ties to the chrome; the deviation trace above it is white instead, so
    // the two bands' moving lines don't read as the same line split in two.
    // See ui/Palette.h for the two-family rationale.
    //
    // Note the crossover in the names: this class's kTrace is the DEVIATION
    // trace (white, upper band) and kGainTrace is the gain trace (green, lower
    // band) -- Palette's kDevTrace is the white one.
    const juce::Colour kBackground { Palette::kLcdBackground };
    const juce::Colour kIdle       { Palette::kLcdIdle };
    const juce::Colour kCenterLine { Palette::kLcdCenterLine };
    const juce::Colour kTrace      { Palette::kDevTrace };
    const juce::Colour kReadout    { Palette::kLcdReadout };
    const juce::Colour kBevel      { Palette::kLcdBevel }; // border, matches MatrixReadout's window edge
    const juce::Colour kGainTrace     { Palette::kBrand };
    const juce::Colour kGainZeroLine  { Palette::kGainZeroLine };

    // --- Helpers ------------------------------------------------------------
    int quantizeRow(float devLU) const
    {
        // Louder than target -> above the line (smaller row index).
        // Clamped to the upper (deviation) band only -- never crosses into
        // the gain band below.
        const int r = targetRow - (int)std::round(devLU * kRowsPerLU);
        return juce::jlimit(0, devRows - 1, r);
    }

    int quantizeGainRow(float gainDb) const
    {
        // More gain (boost) -> above the zero-gain line, same up=more
        // convention as quantizeRow(). Clamped to the lower (gain) band only.
        const int r = gainZeroRow - (int)std::round(gainDb * gainRowsPerDb);
        return juce::jlimit(devRows, rows - 1, r);
    }

    static juce::Colour statusColourFor(float t)
    {
        return t < 0.5f ? Palette::kLcdStatusGood.interpolatedWith(Palette::kStatusOrange, t * 2.0f)
                        : Palette::kStatusOrange.interpolatedWith(Palette::kStatusRed, (t - 0.5f) * 2.0f);
    }

    // --- Centred status message (5x7 LED font, always full brightness) -------
    int statusTopRow() const { return rows - LedFont5x7::kHeight - kReadoutMargin; }

    /** Cell column that centered content is centred on. Single source of truth
        for the watermark and the readout so the two can never drift apart. */
    int contentCentreCol() const
    {
        // Upper bound is cols - 1 (a column index of cols is off-screen); the
        // jmax keeps jlimit's range valid if this is ever hit before resized().
        return contentCentreX < 0 ? cols / 2
                                  : juce::jlimit(0, juce::jmax(0, cols - 1), contentCentreX / kCell);
    }

    // Fixed repaint region wide enough for any status string; using a constant
    // area avoids tracking old-vs-new text extents when the width changes.
    juce::Rectangle<int> statusRepaintArea() const
    {
        const int wCells = 16 * LedFont5x7::kAdvance; // generous: 16 glyphs
        const int x      = (contentCentreCol() - wCells / 2) * kCell;
        const int y      = (statusTopRow() - 1) * kCell;
        return { x, y, wCells * kCell, (LedFont5x7::kHeight + 2) * kCell };
    }

    void drawStatusText(juce::Graphics& g) const
    {
        if (statusText.isEmpty() || rows <= LedFont5x7::kHeight)
            return;

        const auto utf8   = statusText.toRawUTF8();
        const int  wCells = LedFont5x7::textWidthCells(utf8);
        int col           = contentCentreCol() - wCells / 2;
        const int topRow  = statusTopRow();

        g.setColour(kReadout); // readout keeps the original hot LCD green
        for (const char* p = utf8; *p != 0; ++p, col += LedFont5x7::kAdvance)
        {
            const auto* glyph = LedFont5x7::getGlyph(*p);
            if (glyph == nullptr)
                continue;
            for (int y = 0; y < LedFont5x7::kHeight; ++y)
                for (int x = 0; x < LedFont5x7::kWidth; ++x)
                    if ((glyph->row[y] >> (LedFont5x7::kWidth - 1 - x)) & 1)
                        g.fillRect((col + x) * kCell, (topRow + y) * kCell, 1, 1);
        }
    }

    // --- In-band readouts (4x6 LED font) ------------------------------------
    // Both bands carry the same kind of label -- "gain +1.2 dB" against the
    // zero-gain line, "dev -0.4 LUFS" against the deviation range block. One
    // implementation, two instances (gainReadout / devReadout), parameterised
    // by a Slot describing what it parks against.
    //
    // Right-aligned rather than centred: the newest data is at the right edge,
    // so a label sits at the live end of the line it names. Callers pad the
    // number to a fixed width, so the left edge doesn't twitch as digits come
    // and go.
    struct Slot
    {
        int edgeAbove;      // row the label's bottom must clear when sitting above
        int edgeBelow;      // row the label's top must clear when sitting below
        int rowMin, rowMax; // band the label must stay inside (inclusive)
    };

    // Half-height (in rows) of the +/-kRangeLU range block around targetRow.
    // Single source of truth for rebuildBackplate() and the deviation readout,
    // so the label can't drift away from the block's actual edge.
    int rangeHalfRows() const { return juce::jmax(1, (int)std::round(kRangeLU * kRowsPerLU)); }

    Slot gainSlot() const { return { gainZeroRow, gainZeroRow, devRows, rows - 1 }; }
    Slot devSlot()  const
    {
        const int h = rangeHalfRows();
        return { targetRow - h, targetRow + h, 0, devRows - 1 };
    }

    int readoutLeftCol(const juce::String& text) const
    {
        return juce::jmax(0, cols - kReadoutRightMargin
                                 - LedFont4x6::textWidthCells(text.toRawUTF8()));
    }

    /** Top row of the glyph box for either side of the slot's reference.
        Clamped into the slot's band so a label can never bleed into the
        neighbouring band or off a screen edge. */
    static int readoutTopRow(const Slot& s, bool above)
    {
        const int r = above ? s.edgeAbove - kReadoutGap - LedFont4x6::kHeight
                            : s.edgeBelow + kReadoutGap + 1;
        const int hi = s.rowMax - LedFont4x6::kHeight + 1;
        return juce::jlimit(s.rowMin, juce::jmax(s.rowMin, hi), r);
    }

    // Covers BOTH sides of the reference plus a cell of slack, so a side flip
    // and a text change are the same repaint region -- no need to remember
    // where the label was last frame.
    juce::Rectangle<int> readoutRepaintArea(const Slot& s) const
    {
        const int wCells = 16 * LedFont4x6::kAdvance; // generous: 16 glyphs
        const int top    = readoutTopRow(s, true) - 1;
        const int bot    = readoutTopRow(s, false) + LedFont4x6::kHeight + 1;
        return { juce::jmax(0, cols - kReadoutRightMargin - wCells) * kCell, top * kCell,
                 wCells * kCell, (bot - top) * kCell };
    }

    /** True if any dot of `ring` in columns [c0, c1) falls in rows [r0, r1]. */
    bool traceHits(const std::vector<int>& ring, int c0, int c1, int r0, int r1) const
    {
        if (cols <= 0 || (int)ring.size() < cols)
            return false;
        for (int x = juce::jmax(0, c0); x < juce::jmin(cols, c1); ++x)
        {
            // Same column -> ring mapping paint() uses: the newest sample
            // (head) is the rightmost column, x = cols - 1.
            const int r = ring[(size_t)((head + x + 1) % cols)];
            if (r >= r0 && r <= r1)
                return true;
        }
        return false;
    }

    /** Keep a readout off its trace: flip sides only when the side it's on is
        fouled AND the other one is clear. That asymmetry is the hysteresis --
        a trace sitting right on the reference covers both sides, so the label
        stays put instead of flickering between them every column. */
    void updateReadoutSide(const juce::String& text, const Slot& s,
                           const std::vector<int>& ring, bool& above) const
    {
        if (text.isEmpty() || cols <= 0)
            return;

        const int c0 = readoutLeftCol(text) - 1;
        const int c1 = cols - kReadoutRightMargin + 1;
        const int tAbove = readoutTopRow(s, true);
        const int tBelow = readoutTopRow(s, false);
        const bool fouledAbove = traceHits(ring, c0, c1, tAbove - 1, tAbove + LedFont4x6::kHeight);
        const bool fouledBelow = traceHits(ring, c0, c1, tBelow - 1, tBelow + LedFont4x6::kHeight);

        if (above && fouledAbove && !fouledBelow)
            above = false;
        else if (!above && fouledBelow && !fouledAbove)
            above = true;
        // both clear or both fouled -> stay where it is
    }

    void updateReadoutSides()
    {
        updateReadoutSide(gainText, gainSlot(), gainColRows, gainAbove);
        updateReadoutSide(devText,  devSlot(),  colRows,     devAbove);
    }

    void drawReadout(juce::Graphics& g, const juce::String& text,
                     const Slot& s, bool above) const
    {
        if (text.isEmpty() || rows <= LedFont4x6::kHeight)
            return;

        const auto utf8  = text.toRawUTF8();
        int col          = readoutLeftCol(text);
        const int topRow = readoutTopRow(s, above);

        g.setColour(kReadout);
        for (const char* p = utf8; *p != 0; ++p, col += LedFont4x6::kAdvance)
        {
            const auto* glyph = LedFont4x6::getGlyph(*p);
            if (glyph == nullptr)
                continue;
            for (int y = 0; y < LedFont4x6::kHeight; ++y)
                for (int x = 0; x < LedFont4x6::kWidth; ++x)
                    if ((glyph->row[y] >> (LedFont4x6::kWidth - 1 - x)) & 1)
                        g.fillRect((col + x) * kCell, (topRow + y) * kCell, 1, 1);
        }
    }

    // Rasterize the logo SVG into per-cell coverage (0..1), sized so the mark's
    // bar row lands exactly on targetRow by construction.
    void rasterizeLogo()
    {
        logoCov.clear();
        if (logoDrawable == nullptr || rows <= 0 || cols <= 0)
            return;

        const auto svgBounds = logoDrawable->getDrawableBounds();
        if (svgBounds.isEmpty())
            return;

        // Sized against the full screen height (not just the deviation band)
        // -- at the current 70/30 band split it still fits comfortably inside
        // the upper band, bar-lock and all.
        logoRows = juce::jmax(1, (int)(rows * kLogoHeightFrac));
        logoCols = juce::jmax(1, (int)std::round(logoRows * svgBounds.getWidth() / svgBounds.getHeight()));
        if (logoCols > cols - 4) // keep a small side margin; preserve aspect
        {
            // jmax: on a very narrow screen cols - 4 goes <= 0, and the
            // logoCols * logoRows below would then resize() a negative int as
            // size_t. Unreachable at the fixed window size, cheap to rule out.
            logoCols = juce::jmax(1, cols - 4);
            logoRows = juce::jmax(1, (int)std::round(logoCols * svgBounds.getHeight() / svgBounds.getWidth()));
        }
        logoX = juce::jlimit(0, juce::jmax(0, cols - logoCols),
                             contentCentreCol() - logoCols / 2);
        // Bar lock: the mark's bar center sits at kLogoBarFrac of its height,
        // so anchoring that to targetRow keeps logo bar == target line.
        logoY = juce::jmax(0, targetRow - (int)std::round(kLogoBarFrac * (float)logoRows));

        juce::Image tmp(juce::Image::ARGB, logoCols, logoRows, true);
        {
            juce::Graphics g(tmp);
            logoDrawable->drawWithin(g,
                juce::Rectangle<float>(0.0f, 0.0f, (float)logoCols, (float)logoRows),
                juce::RectanglePlacement::stretchToFit, 1.0f);
        }

        logoCov.resize((size_t)(logoCols * logoRows));
        for (int y = 0; y < logoRows; ++y)
            for (int x = 0; x < logoCols; ++x)
                logoCov[(size_t)(y * logoCols + x)] = tmp.getPixelAt(x, y).getFloatAlpha();

        buildLogoMask();
    }

    // Dilate logoCov by kLogoMarginCells into a boolean "keep-clear" mask, so
    // the range band (drawn in rebuildBackplate) can be punched out around the
    // logo with a small halo instead of butting right up against its edge.
    // Uses kLogoMaskThreshold (not the watermark's own softer render cutoff)
    // so the shape being dilated is the glyph's solid core, not its
    // antialiased fringe -- dilating the fringe gave a jagged, non-straight
    // halo boundary along diagonal edges. Rebuilt only
    // alongside the logo raster (resize/recentre), not on every backplate
    // rebuild (e.g. status colour steps).
    void buildLogoMask()
    {
        maskX    = logoX - kLogoMarginCells;
        maskY    = logoY - kLogoMarginCells;
        maskCols = logoCols + 2 * kLogoMarginCells;
        maskRows = logoRows + 2 * kLogoMarginCells;
        logoMask.assign((size_t)(maskCols * maskRows), false);

        for (int ly = 0; ly < logoRows; ++ly)
        {
            for (int lx = 0; lx < logoCols; ++lx)
            {
                if (logoCov[(size_t)(ly * logoCols + lx)] < kLogoMaskThreshold)
                    continue;
                // Stamp a (2*margin+1) square centred on this covered cell's
                // position in mask space (mask space = logo space + margin).
                for (int dy = 0; dy <= 2 * kLogoMarginCells; ++dy)
                {
                    const int my = ly + dy;
                    for (int dx = 0; dx <= 2 * kLogoMarginCells; ++dx)
                        logoMask[(size_t)(my * maskCols + (lx + dx))] = true;
                }
            }
        }
    }

    // True if (col, row) -- in the same screen-cell coordinates as logoX/logoY
    // -- falls within the logo's rasterized coverage or its margin halo.
    bool nearLogo(int col, int row) const
    {
        const int mx = col - maskX;
        const int my = row - maskY;
        if (mx < 0 || mx >= maskCols || my < 0 || my >= maskRows)
            return false;
        return logoMask[(size_t)(my * maskCols + mx)];
    }

    // Bake background + idle texture + range band + centre line + tinted watermark.
    void rebuildBackplate()
    {
        if (getWidth() <= 0 || getHeight() <= 0)
            return;

        backplate = juce::Image(juce::Image::ARGB, getWidth(), getHeight(), false);
        juce::Graphics g(backplate);
        g.fillAll(kBackground);

        // Idle-pixel texture: every 4th row, full width
        g.setColour(kIdle);
        for (int r = 0; r < rows; r += kIdleRowStep)
            for (int c = 0; c < cols; ++c)
                g.fillRect(c * kCell, r * kCell, 1, 1);

        // Range band: fixed +/-1 LUFS zone around target, tinted like the logo
        // but darker/dimmer so the two read as related but distinct. Masked
        // out near the logo (coverage + halo margin) so the watermark is
        // never interrupted -- drawn on top of this, unobstructed, below.
        {
            const int rangeRows = rangeHalfRows(); // shared with the deviation readout's parking position
            const int rTop = juce::jmax(0, targetRow - rangeRows);
            const int rBot = juce::jmin(devRows - 1, targetRow + rangeRows);
            g.setColour(statusColour.darker(kRangeDarken).withAlpha(kRangeDim));
            for (int r = rTop; r <= rBot; ++r)
                for (int c = 0; c < cols; ++c)
                    if (!nearLogo(c, r))
                        g.fillRect(c * kCell, r * kCell, 1, 1);
        }

        // Centre line: exact target row, near-black so it reads clearly
        // against the range band's tint on either side. Same logo masking as
        // the range band, so the watermark stays uninterrupted.
        g.setColour(kCenterLine);
        for (int c = 0; c < cols; ++c)
            if (!nearLogo(c, targetRow))
                g.fillRect(c * kCell, targetRow * kCell, 1, 1);

        // Logo watermark, tinted by the status lamp color -- drawn last so it
        // is always the topmost thing in its own footprint.
        if (!logoCov.empty())
        {
            for (int y = 0; y < logoRows; ++y)
            {
                const int r = logoY + y;
                if (r < 0 || r >= rows)
                    continue;
                for (int x = 0; x < logoCols; ++x)
                {
                    const float cov = logoCov[(size_t)(y * logoCols + x)];
                    if (cov < 0.05f)
                        continue;
                    g.setColour(statusColour.withAlpha(cov * kLogoDim));
                    g.fillRect((logoX + x) * kCell, r * kCell, 1, 1);
                }
            }
        }

        // Zero-gain row: fixed dim-white line in the lower band; the gain
        // trace is drawn relative to it, same idea as the target row above.
        g.setColour(kGainZeroLine);
        for (int c = 0; c < cols; ++c)
            g.fillRect(c * kCell, gainZeroRow * kCell, 1, 1);

        // Outline: the small MatrixReadout windows draw a bevel border around
        // their edge; the big screen was missing it. Vertical gradient (lighter
        // top -> darker bottom) instead of MatrixReadout's flat colour so the
        // larger edge reads as a subtle depth bevel rather than a flat line.
        const auto edge = juce::Rectangle<float>(0.0f, 0.0f, (float)getWidth(), (float)getHeight()).reduced(0.5f);
        juce::ColourGradient bevelGrad(kBevel.brighter(0.3f), 0.0f, edge.getY(),
                                       kBevel.darker(0.3f),   0.0f, edge.getBottom(), false);
        g.setGradientFill(bevelGrad);
        g.drawRect(edge, 1.0f);
    }

    // --- State --------------------------------------------------------------
    std::unique_ptr<juce::Drawable> logoDrawable;

    int cols = 0, rows = 0, targetRow = 0;

    // Band split (see class comment): rows 0..devRows-1 are the deviation
    // band, devRows..rows-1 are the gain band. gainRowsPerDb is derived each
    // resize so +/-kGainRangeDb exactly spans the gain band's half-height.
    int   devRows = 0, gainRows = 0, gainZeroRow = 0;
    float gainRowsPerDb = 1.0f;

    std::vector<int> colRows;      // ring: quantized deviation row per column, kNoDot = empty
    std::vector<int> gainColRows;  // ring: quantized gain row per column, kNoDot = empty
    int head      = 0;          // index of newest column (shared by both rings)
    int tickCount = 0;

    std::vector<float> logoCov; // per-cell watermark coverage
    int logoCols = 0, logoRows = 0, logoX = 0, logoY = 0;

    // Dilated logoCov (see buildLogoMask()): true wherever the range band and
    // centre line must stay clear so the logo (drawn on top regardless) gets
    // a clean halo instead of a hard seam.
    std::vector<bool> logoMask;
    int maskX = 0, maskY = 0, maskCols = 0, maskRows = 0;

    juce::Colour statusColour { Palette::kLcdStatusGood };
    int statusStep = 0;

    juce::String gainText;      // gain readout, gain band right side (e.g. "gain +2.5 dB")
    juce::String devText;       // deviation readout, deviation band right side (e.g. "dev -0.4 LUFS")
    juce::String statusText;    // big centred message (e.g. "warmup 8"); empty once running

    // Which side of its reference each in-band readout currently sits on --
    // flipped by updateReadoutSide() when the trace would run through it.
    bool gainAbove = false;
    bool devAbove  = false;
    int contentCentreX = -1;    // <0 = centre on the screen itself (see setContentCentreX)

    juce::Image backplate;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PixelScreen)
};
