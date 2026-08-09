#pragma once

#include <JuceHeader.h>
#include "Palette.h"

/*
    TargetStrip -- the 32-LED touch strip that sets the LUFS target.
    Header-only. Siblings: ui/PixelScreen.h, ui/LedFont5x7.h.

    Vertical strip, top LED = loudest target (range end, -8 LUFS), bottom =
    quietest (range start, -24 LUFS). Click jumps to the clicked position, drag
    adjusts continuously, mouse wheel steps 0.5 dB. All values quantize to
    half-dB steps.

    Bound to the host parameter via juce::ParameterAttachment, so host
    automation / preset recall light the strip correctly and drags are wrapped
    in proper begin/end gestures. The parameter's own range is used -- no
    duplicated min/max constants here.
*/
class TargetStrip : public juce::Component
{
public:
    explicit TargetStrip(juce::RangedAudioParameter& p)
        : param(p),
          attachment(p, [this](float v) { value = v; repaint(); }, nullptr)
    {
        setMouseCursor(juce::MouseCursor::PointingHandCursor);
        attachment.sendInitialUpdate();
    }

    void paint(juce::Graphics& g) override
    {
        const auto  area  = ledArea().toFloat();
        const float ledH  = area.getHeight() / (float)kNumLeds;
        const auto& range = param.getNormalisableRange();
        const float span  = range.end - range.start;

        // Recessed LCD box behind the LEDs -- same fill/bevel/corner recipe
        // as MatrixReadout (ui/MatrixReadout.h) so the strip reads as the
        // same instrument as the small windows rather than bare LEDs. Full
        // component height (already matches PixelScreen's top/bottom), but
        // only as wide as the LED column plus kBgPad -- that padding is what
        // lines its left edge up with the target readout below.
        const juce::Rectangle<float> bg(area.getX() - kBgPad, 0.0f,
                                         area.getWidth() + 2.0f * kBgPad,
                                         (float)getHeight());
        g.setColour(kBackground);
        g.fillRoundedRectangle(bg, kCorner);
        g.setColour(kBevel);
        g.drawRoundedRectangle(bg.reduced(0.5f), kCorner, 1.0f);

        // Nearest LED to the current value (the "cursor" LED, drawn brightest)
        const int currentLed = juce::jlimit(0, kNumLeds - 1,
            (int)std::round((range.end - value) / span * (float)(kNumLeds - 1)));

        for (int i = 0; i < kNumLeds; ++i)
        {
            // i = 0 is the TOP LED = loudest target
            const float ledValue = range.end - span * (float)i / (float)(kNumLeds - 1);

            juce::Rectangle<float> r(area.getX(),
                                     area.getY() + (float)i * ledH,
                                     area.getWidth(),
                                     juce::jmax(1.0f, ledH - kGapPx));

            if (i == currentLed)              g.setColour(kCursor); // value LED
            else if (ledValue <= value)       g.setColour(kLit);    // filled up to target
            else                              g.setColour(kUnlit);
            g.fillRect(r);
        }
    }

    // Click jumps (mouseDown applies immediately), drag adjusts continuously
    void mouseDown(const juce::MouseEvent& e) override
    {
        attachment.beginGesture();
        applyDrag(e);
    }
    void mouseDrag(const juce::MouseEvent& e) override { applyDrag(e); }
    void mouseUp(const juce::MouseEvent&) override     { attachment.endGesture(); }

    void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails& wheel) override
    {
        if (wheel.deltaY == 0.0f)
            return;
        const auto& range = param.getNormalisableRange();
        const float step  = wheel.deltaY > 0.0f ? kStepDb : -kStepDb;
        attachment.setValueAsCompleteGesture(
            juce::jlimit(range.start, range.end, quantize(value + step)));
    }

private:
    static constexpr int   kNumLeds = 32;
    static constexpr float kStepDb  = 0.5f;  // wheel step + quantization grid
    static constexpr float kGapPx   = 2.0f;  // vertical gap between LEDs
    static constexpr int   kLedW    = 20;    // LED width (strip is centered in bounds)
    static constexpr float kBgPad   = 8.0f;  // horizontal LCD box padding -- matches MatrixReadout::kPadX
    static constexpr float kCorner  = 3.0f;  // matches MatrixReadout::kCorner

    // Greens derive from Palette::kBrand -- see ui/Palette.h
    const juce::Colour kCursor { Palette::kHot };   // current-value LED, brightest
    const juce::Colour kLit    { Palette::kLit };   // filled portion below the value
    const juce::Colour kUnlit  { Palette::kUnlit }; // empty LEDs, faint LCD-idle green

    // Phosphor family -- same LCD box recipe as MatrixReadout (ui/MatrixReadout.h)
    const juce::Colour kBackground { Palette::kLcdBackground };
    const juce::Colour kBevel      { Palette::kLcdBevel };

    juce::Rectangle<int> ledArea() const
    {
        auto b = getLocalBounds().reduced(0, 2);
        return b.withSizeKeepingCentre(juce::jmin(kLedW, b.getWidth()), b.getHeight());
    }

    static float quantize(float v) { return std::round(v / kStepDb) * kStepDb; }

    void applyDrag(const juce::MouseEvent& e)
    {
        const auto  area  = ledArea().toFloat();
        const auto& range = param.getNormalisableRange();
        const float v     = juce::jmap((float)e.position.y,
                                       area.getY(), area.getBottom(),
                                       range.end, range.start);
        attachment.setValueAsPartOfGesture(
            juce::jlimit(range.start, range.end, quantize(v)));
    }

    juce::RangedAudioParameter& param;
    juce::ParameterAttachment   attachment;
    float value = 0.0f; // kept in sync by the attachment callback

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TargetStrip)
};
