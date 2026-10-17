#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "ui/Palette.h"
#include "ui/Fonts.h"
#include "BuildInfo.h"

using namespace juce;

/*
    PluginEditor -- the LevelAnchor editor window: fixed 530x380, not resizable.
    Siblings: ui/PixelScreen.h (the LCD centrepiece), ui/TargetStrip.h,
    ui/MatrixReadout.h, ui/AboutPanel.h, ui/ModePanel.h, ui/Palette.h, ui/Fonts.h.

    Design idea: a live stream should be readable at a glance and never demand
    attention -- one target to set, continuous visual feedback instead of dense
    numerics, and the algorithm's own behaviour as the thing on screen. The
    pixel matrix carries that, with the LevelAPI mark rasterized into it as a
    watermark that doubles as the status lamp.

    The editor pulls, and is never pushed to: a 30 Hz timer polls the
    processor's UIDisplayData snapshot and hands each child the one value it
    draws. Nothing here touches the audio thread.
*/

namespace
{
    // Row/strip geometry shared between resized() (child component layout)
    // and paint() (wordmark alignment) -- see resized() for the row layout
    // this geometry describes.
    constexpr int kMargin    = 16;
    constexpr int kStripW    = 44;
    // Independent visual nudges (px, relative to the un-shifted kMargin
    // position) for the strip and the screen -- two constants rather than one
    // shared shift, so the two can be moved apart.
    constexpr int kStripShift  = -4;
    constexpr int kScreenShift = 0;
    constexpr int kScreenTop = 62;  // bottom of row one / top of row two

    constexpr int kStripLedW = 20;  // must match TargetStrip's private kLedW

    // Wordmark left edge: flush with the strip's visual LED column below it,
    // not the strip's (wider) outer component bounds -- TargetStrip centres
    // its LEDs within its own bounds, so the two don't coincide.
    constexpr int kWordmarkLeft = kMargin + kStripShift + (kStripW - kStripLedW) / 2;

    // Nudged 3px below dead-centre of row one -- pure visual offset.
    constexpr int kWordmarkYOffset = 3;

    // Version tag: top of its 10px line, just under the wordmark's glyphs and
    // clear of the screen at kScreenTop. Pure visual placement, tune by eye.
    constexpr int kVersionTop = 50;

    // Peak LED. It lights (dim) from kPeakLightDb of peak-stage reduction and
    // goes full from kPeakHeavyDb; kPeakHoldMs keeps a short peak visible.
    // kPeakLightDb also decides which gain-trace columns turn amber.
    constexpr float  kPeakLightDb = 0.5f;
    constexpr float  kPeakHeavyDb = 3.0f;
    constexpr uint32 kPeakHoldMs  = 300;
    constexpr int    kPeakLedSize = 14;
}

// =============================================================
// Constructor
// =============================================================
LevelAnchorAudioProcessorEditor::LevelAnchorAudioProcessorEditor(LevelAnchorAudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    // 1. Window setup -- fixed, not resizable.
    // NOTE: setSize() is called at the END of this constructor -- it triggers
    // resized(), which must only run once all child components exist.
    setResizable(false, false);

    // 2. Pixel screen (LCD centerpiece)
    addAndMakeVisible(pixelScreen);

    // 3. Reset button -- triggers a parameter write to clear the loudness measurement.
    // Styled to sit quietly next to the timer window: dark fill, thin border,
    // white text. It's a deliberate action, not something to draw the eye.
    resetButton.setButtonText("RESET");
    resetButton.setColour(TextButton::buttonColourId,   Colour(0xff141814));
    resetButton.setColour(TextButton::buttonOnColourId, Colour(0xff1e261e));
    resetButton.setColour(TextButton::textColourOffId,  Colours::white);
    resetButton.setColour(TextButton::textColourOnId,   Colours::white);
    resetButton.setColour(ComboBox::outlineColourId,    Colour(0xff2e3a2e));
    resetButton.onClick = [this]()
    {
        if (auto* r = dynamic_cast<AudioParameterBool*>(
                processorRef.getValueTreeState().getParameter("reset")))
            *r = true;
    };
    addAndMakeVisible(resetButton);

    // 3a. Delay button under RESET; opens the mode panel (added in step 7)
    delayButton.onClick = [this] { modePanel.setVisible(true); };
    addAndMakeVisible(delayButton);

    // 3b. Peak LED -- top bar, centred on the logo's axis (see resized())
    addAndMakeVisible(peakLed);

    // 4. Target strip -- 32-LED touch strip bound to the "targetLUFS" parameter
    auto& vts = processorRef.getValueTreeState();
    if (auto* targetParam = vts.getParameter("targetLUFS"))
    {
        targetStrip = std::make_unique<TargetStrip>(*targetParam);
        addAndMakeVisible(*targetStrip);
    }
    targetParamValue = vts.getRawParameterValue("targetLUFS");

    // 5. Labels -- fixed font sizes (the window size is fixed)
    auto setupLabel = [this](Label& l, float fontSize, Justification just)
    {
        l.setJustificationType(just);
        l.setFont(Font(FontOptions(fontSize)));
        addAndMakeVisible(l);
    };
    setupLabel(devLabel,      12.f, Justification::centredLeft);
    setupLabel(timerCaption,   9.f, Justification::centred);
    setupLabel(peakCaption,    9.f, Justification::centred);
    setupLabel(targetCaption,  9.f, Justification::centred);
    setupLabel(outputCaption,  9.f, Justification::centred);

    // Captions are chrome, not data -- plain sans, dim, so the lit readouts
    // above them stay the thing your eye lands on.
    for (Label* l : { &timerCaption, &peakCaption, &targetCaption, &outputCaption })
    {
        l->setFont(Font(FontOptions(9.f)).withExtraKerningFactor(0.20f));
        l->setColour(Label::textColourId, Colours::grey);
    }
    timerCaption.setText("TIME",         dontSendNotification);
    peakCaption.setText("PEAK",          dontSendNotification);
    targetCaption.setText("TARGET",       dontSendNotification);
    outputCaption.setText("OUTPUT LEVEL", dontSendNotification);

    // 5b. Small LCD windows. Sized from the widest string each will ever show
    // so the boxes never reflow as values change.
    addAndMakeVisible(timerReadout);
    addAndMakeVisible(targetReadout);
    addAndMakeVisible(outputReadout);

    if (targetParamValue != nullptr) // initial value; timer keeps it current
        targetReadout.setText(String::formatted("%.1f LUFS", targetParamValue->load()));

    // 6. Diagnostics: default OFF for release; still reachable for tuning.
    // Ctrl+click on the editor background toggles it either way (see mouseDown).
    constexpr bool kShowDiagnosticsByDefault = false;
    devLabel.setVisible(kShowDiagnosticsByDefault);

    // 7. Powered-by badge + About overlay (added last so the overlay is topmost)
    poweredBy.onClick = [this] { aboutPanel.setVisible(true); };
    addAndMakeVisible(poweredBy);
    addChildComponent(aboutPanel); // starts hidden

    // 7b. Mode panel overlay. Every mode is listed; the ones this build does
    // not offer show locked. The processor rebuilds the engine on a pick.
    static_assert(ModePanel::kNumModes == LevelAnchorAudioProcessor::kNumModes,
                  "ModePanel lists every engine mode");
    for (int m = 0; m < ModePanel::kNumModes; ++m)
        modePanel.setOffered(m, LevelAnchorAudioProcessor::isModeOffered(m));
    modePanel.setSelectedMode(processorRef.getRequestedMode());
    modePanel.onSelect = [this](int mode) { processorRef.requestMode(mode); };
    addChildComponent(modePanel); // starts hidden

    // 8. UI refresh timer -- PixelScreen consumes one tick per callback
    startTimerHz(30);

    // 9. Size last: triggers resized() now that every child component exists
    setSize(530, 380);

    // 10. One-time About-panel auto-open: plugin state
    // (getStateInformation) is per-project/session, so a per-user PropertiesFile
    // is used instead -- shared across VST3/Standalone, so "first launch ever"
    // means first launch of either, not one auto-open per format.
    // NOTE: we only decide *whether* to auto-open here, not do it -- the editor
    // isn't guaranteed to be on-screen yet (the host may construct it before
    // showing its window), and AboutPanel::setVisible(true) grabs keyboard
    // focus, which asserts if the component isn't showing. The actual open
    // happens in timerCallback() once isShowing() is true.
    juce::PropertiesFile::Options propsOptions;
    propsOptions.applicationName    = "LevelAnchor";
    propsOptions.filenameSuffix     = "settings";
    propsOptions.folderName         = "LevelAnchor";
    propsOptions.osxLibrarySubFolder = "Application Support";
    juce::PropertiesFile props(propsOptions);
    DBG("Settings file: " + props.getFile().getFullPathName());
    pendingAboutAutoOpen = ! props.getBoolValue("hasShownAboutPanel", false);
}

LevelAnchorAudioProcessorEditor::~LevelAnchorAudioProcessorEditor()
{
}

// =============================================================
// Drawing
// =============================================================

void LevelAnchorAudioProcessorEditor::drawBackground(Graphics& g)
{
    // Near-black gradient -- professional, low-distraction background
    auto bounds = getLocalBounds();
    ColourGradient grad(Colour(0xff191919), 0.f, 0.f,
                        Colour(0xff121212), 0.f, (float)bounds.getBottom(), false);
    g.setGradientFill(grad);
    g.fillRect(bounds);
}

// =============================================================
// Main Paint
// =============================================================
void LevelAnchorAudioProcessorEditor::paint(Graphics& g)
{
    drawBackground(g);

    // Wordmark (static chrome; the editor rarely repaints, so drawing it here
    // costs nothing per-frame). Two runs: LEVEL white, ANCHOR brand green.
    // Left edge lines up with the target strip's LED column below it
    // (kWordmarkLeft); vertically centred in row one (y 0..kScreenTop) by
    // drawing over the full row height with Justification::centredLeft.
    // Also clickable -- opens the About panel, same as the powered-by badge
    // (see mouseDown()/mouseMove()); wordmarkLeftW/RightW computed in resized().
    g.setFont(Fonts::wordmark(42.f).withExtraKerningFactor(0.03f));
    const String left("LEVEL");
    const String right("ANCHOR");
    const juce::Rectangle<int> row(kWordmarkLeft, kWordmarkYOffset, wordmarkLeftW, kScreenTop);
    g.setColour(Colours::white);
    g.drawText(left, row, Justification::centredLeft);
    g.setColour(Palette::kBrand);
    g.drawText(right, row.withX(kWordmarkLeft + wordmarkLeftW).withWidth(wordmarkRightW), Justification::centredLeft);

    // Version tag, small and dim under the wordmark's right end, so a release
    // can be told apart at a glance. From BuildInfo.h: a build made without the
    // release script shows "vdev".
    g.setColour(Colours::grey);
    g.setFont(Font(FontOptions(9.f)).withExtraKerningFactor(0.20f));
    g.drawText("v" LEVELANCHOR_VERSION,
               juce::Rectangle<int>(kWordmarkLeft, kVersionTop, wordmarkLeftW + wordmarkRightW, 10),
               Justification::centredRight);
}

// Combined "LEVEL"+"ANCHOR" bounding rect, used for click/hover hit-testing.
juce::Rectangle<int> LevelAnchorAudioProcessorEditor::wordmarkBounds() const
{
    return { kWordmarkLeft, kWordmarkYOffset, wordmarkLeftW + wordmarkRightW, kScreenTop };
}

// =============================================================
// Resize
// =============================================================
void LevelAnchorAudioProcessorEditor::resized()
{
    // Window is fixed 530x380, laid out as three bands of equal top/bottom
    // weight: chrome 0-62, screen 62-318, readouts 318-380. The wordmark
    // top-left is drawn in paint(); everything else is a child component.
    // kMargin, kStripW, kScreenTop, kStripShift, kScreenShift are file-scope
    // (shared with paint()'s wordmark alignment) -- see the top of this file.
    constexpr int kGap         = 0;   // strip-to-screen gap; 0 = flush
    constexpr int kScreenH     = 256;
    constexpr int kReadoutTop  = 330;

    // Wordmark run widths -- recomputed here (not just once in the constructor)
    // since resized() is where paint()'s other layout constants live; cheap,
    // and font metrics don't change since the window is fixed-size anyway.
    const Font wordmarkFont(Fonts::wordmark(42.f).withExtraKerningFactor(0.03f));
    wordmarkLeftW  = (int)std::ceil(GlyphArrangement::getStringWidth(wordmarkFont, "LEVEL"));
    wordmarkRightW = (int)std::ceil(GlyphArrangement::getStringWidth(wordmarkFont, "ANCHOR"));

    // Both fold in kStripShift: the strip's real right edge is
    // kMargin + kStripShift + kStripW, so measuring the gap from
    // kMargin + kStripW instead would leave a hidden extra -kStripShift px of
    // whitespace no matter what kGap is set to. screenW mirrors screenX so the
    // screen's right edge (distance from the window's right margin) is
    // unaffected.
    const int screenX = kMargin + kStripShift + kStripW + kGap + kScreenShift;
    const int screenW = getWidth() - kMargin - (kMargin + kStripShift + kStripW + kGap);

    if (targetStrip != nullptr)
        targetStrip->setBounds(kMargin + kStripShift, kScreenTop, kStripW, kScreenH);

    pixelScreen.setBounds(screenX, kScreenTop, screenW, kScreenH);

    // The screen sits right of the strip, so its own centre is off the
    // window's centre axis. Anchor its watermark + centred status message to
    // the window centre instead, so they line up with the badge below.
    // Computed from screenX, so it self-corrects whenever kScreenShift
    // changes. (The gain readout is right-aligned and unaffected.)
    pixelScreen.setContentCentreX(getWidth() / 2 - screenX);

    // Top chrome: reset button then timer window, hard right
    const auto timerSize = MatrixReadout::preferredSize("00:00:00");
    timerReadout.setBounds(getWidth() - kMargin - timerSize.x, 18,
                           timerSize.x, timerSize.y);
    resetButton.setBounds(timerReadout.getX() - 8 - 64, 18, 64, timerSize.y);
    timerCaption.setBounds(timerReadout.getX(), timerReadout.getBottom() + 3,
                           timerSize.x, 12);

    // Delay button on the caption line under RESET, as wide as RESET.
    delayButton.setBounds(resetButton.getX(), timerCaption.getY(), resetButton.getWidth(), 14);

    // Peak LED on the window's centre axis, the same axis as the logo in the
    // screen and the badge below, level with the reset button; its caption
    // sits on the TIME caption's line.
    peakLed.setBounds(getWidth() / 2 - kPeakLedSize / 2,
                      resetButton.getBounds().getCentreY() - kPeakLedSize / 2,
                      kPeakLedSize, kPeakLedSize);
    peakCaption.setBounds(getWidth() / 2 - 30, timerCaption.getY(), 60, 12);

    // Bottom band: target readout left, output readout right, badge centered.
    // Both windows are sized for the widest value so neither reflows.
    const auto valueSize = MatrixReadout::preferredSize("-99.9 LUFS");
    targetReadout.setBounds(kMargin, kReadoutTop, valueSize.x, valueSize.y);
    outputReadout.setBounds(getWidth() - kMargin - valueSize.x, kReadoutTop,
                            valueSize.x, valueSize.y);

    targetCaption.setBounds(targetReadout.getX(), targetReadout.getBottom() + 3,
                            valueSize.x, 12);
    outputCaption.setBounds(outputReadout.getX(), outputReadout.getBottom() + 3,
                            valueSize.x, 12);

    poweredBy.setBounds(getWidth() / 2 - 90, kReadoutTop + 2, 180, 34);

    // Diagnostics overlays the bottom of the screen -- dead space below the
    // trace, so toggling it shifts nothing in the layout above.
    devLabel.setBounds(screenX + 4, pixelScreen.getBottom() - 20, screenW - 8, 16);

    aboutPanel.setBounds(getLocalBounds());    // overlay covers everything
    modePanel.setBounds(getLocalBounds());     // likewise
}

// =============================================================
// Timer callback -- runs at 30 Hz
// =============================================================
void LevelAnchorAudioProcessorEditor::timerCallback()
{
    // One-time About-panel auto-open, deferred from the constructor until the
    // editor is actually on-screen (see constructor step 10 for why).
    if (pendingAboutAutoOpen && isShowing())
    {
        pendingAboutAutoOpen = false;
        aboutPanel.setVisible(true);

        juce::PropertiesFile::Options propsOptions;
        propsOptions.applicationName    = "LevelAnchor";
        propsOptions.filenameSuffix     = "settings";
        propsOptions.folderName         = "LevelAnchor";
        propsOptions.osxLibrarySubFolder = "Application Support";
        juce::PropertiesFile props(propsOptions);
        props.setValue("hasShownAboutPanel", true);
        props.saveIfNeeded();
    }

    auto data = processorRef.getLatestUIDisplayData();

    // Before the early return below, so the LED's hold still runs out (and the
    // LED goes dark) when audio stops and the snapshot stops changing.
    updatePeakLed(data.index != prevIndex ? data.gainReductionDb : 0.f);

    // Delay and mode in force. Both setters no-op when nothing changed. With no
    // engine there is no delay to show, and the panel shows the stored choice.
    if (data.engineReady && data.samplerate > 0)
        delayButton.setDelayText(String(roundToInt(1000.0 * data.latencySamples / data.samplerate)) + " ms");
    else
        delayButton.setDelayText("--");
    modePanel.setSelectedMode(data.engineReady ? data.mode : processorRef.getRequestedMode());

    if (data.index == prevIndex)
        return; // nothing new from the processor

    prevIndex = data.index;

    // Warmup is reported as seconds remaining, so 0 covers both "finished" and
    // "this preset has none" -- the default presets correct from the first block
    // and never show the countdown at all.
    const float warmupLeft  = data.warmupSecondsLeft;
    const float currentLUFS = data.LUFS;
    const bool  warmedUp    = warmupLeft <= 0.f;
    const float target      = targetParamValue != nullptr ? targetParamValue->load()
                                                          : TARGET_DEFAULT;

    // Feed the pixel screen: integrated-LUFS deviation from target (upper
    // band) and currently applied gain (lower band).
    // Below -48 LUFS the integrated value is treated as "no signal yet".
    const float devLU = (warmedUp && currentLUFS > -48.f)
        ? (currentLUFS - target)
        : 0.f;
    pixelScreen.pushTick(devLU, data.appliedGainDb, warmedUp, data.gainReductionDb >= kPeakLightDb);

    // Target value readout under the strip (setText no-ops if unchanged)
    targetReadout.setText(String::formatted("%.1f LUFS", target));

    // Both value readouts are rendered by the pixel screen itself, each beside
    // the thing it annotates (small 4x6 LED font): gain against the zero-gain
    // line, deviation against the range block. During warmup both are empty and
    // the big centred status slot shows the countdown instead (see below).

    // Deviation readout (integrated LUFS vs target, since reset; all in LU).
    // RMS is the headline "how tightly it holds" figure; spread is the wobble
    // (bias removed) and bias is the lean -- together they say whether remaining
    // deviation is fixable by calibration (bias) or needs response tuning (spread).
    // max is the worst excursion. Blank until warmed up, since the processor only
    // accumulates post-warmup (devRms stays 0 until then).
    if (warmedUp && data.devRms > 0.f)
        devLabel.setText(String::formatted("dev %.2f LU  (spread %.2f  bias %+.2f  max %.2f)",
                                           data.devRms, data.devSpread, data.devBias, data.devMax),
                         dontSendNotification);
    else
        devLabel.setText("dev --", dontSendNotification);

    // Status / LUFS readouts
    if (! warmedUp)
    {
        // Still warming up: no elapsed time and no integrated value yet, so the
        // small windows show placeholders and dim. The countdown itself goes in
        // the big screen, which has the room for words.
        timerReadout.setText("--:--:--");
        outputReadout.setText("--.- LUFS");
        timerReadout.setDimmed(true);
        outputReadout.setDimmed(true);
        pixelScreen.setStatusText(String::formatted("warmup %d", (int)std::ceil(warmupLeft)));
        pixelScreen.setGainText({}); // nothing to label yet -- neither trace is
        pixelScreen.setDevText({});  // being written during warmup
    }
    else
    {
        timerReadout.setDimmed(false);
        outputReadout.setDimmed(false);
        pixelScreen.setStatusText({});
        // Fixed-width numbers (%+5.1f pads to "+32.0"'s width) so the labels'
        // left edges don't twitch as a value crosses +-10.
        pixelScreen.setGainText(String::formatted("gain %+5.1f dB", data.appliedGainDb));
        pixelScreen.setDevText(String::formatted("diff %+5.1f LU", devLU));

        // Show elapsed time and current LUFS. Integer division rather than
        // float: data.index is a 64-bit sample count, and float's 24-bit
        // mantissa starts dropping whole seconds long before it wraps. index is
        // never negative and the guard keeps samplerate from being 0, so the
        // quotient needs no separate clamp.
        // The warmup, if the preset has one, is subtracted so the clock reads
        // zero at the moment correction reaches full strength. Clamped at zero
        // because a preset carrying a longer warmup can be selected after a
        // shorter one has already finished, which would otherwise subtract more
        // than has elapsed and print a negative field.
        const int64_t sr = data.samplerate > 0 ? (int64_t)data.samplerate : 48000;
        const int totalSec = jmax(0, static_cast<int>(data.index / sr)
                                       - static_cast<int>(data.warmupSecondsMax));
        const String elapsed = String::formatted("%02d:%02d:%02d",
            totalSec / 3600, (totalSec % 3600) / 60, totalSec % 60);
        timerReadout.setText(elapsed);
        outputReadout.setText(String::formatted("%.1f LUFS", currentLUFS));

        if (aboutPanel.isVisible())
            aboutPanel.setStatusLine(
                String::formatted("anchored at %.1f LUFS for ", target) + elapsed);
    }

    // No editor-wide repaint: labels repaint themselves on setText, and the
    // PixelScreen repaints itself only on column advance / status change.
}

// A rise shows at once and restarts the hold; a fall waits for the hold to run
// out. gainReductionDb is already the deepest value since the previous snapshot
// (the processor keeps a max-hold), so no peak is missed between ticks.
void LevelAnchorAudioProcessorEditor::updatePeakLed(float gainReductionDb)
{
    const int level = gainReductionDb >= kPeakHeavyDb ? PeakLed::Heavy
                    : gainReductionDb >= kPeakLightDb ? PeakLed::Light
                                                      : PeakLed::Off;
    const uint32 now = Time::getMillisecondCounter();

    if (level >= peakShown)
    {
        peakShown = level;
        if (level != PeakLed::Off)
            peakHoldUntilMs = now + kPeakHoldMs;
    }
    else if ((int32)(now - peakHoldUntilMs) >= 0) // wrap-safe "hold has run out"
    {
        peakShown = level;
    }

    peakLed.setLevel(peakShown);
}

// Ctrl+click anywhere on the editor background toggles the diagnostics readout;
// a plain click on the wordmark opens the About panel (same as the badge below).
void LevelAnchorAudioProcessorEditor::mouseDown(const MouseEvent& e)
{
    if (e.mods.isCtrlDown())
    {
        devLabel.setVisible(!devLabel.isVisible());
        return;
    }

    if (wordmarkBounds().contains(e.getPosition()))
        aboutPanel.setVisible(true);
}

// Pointing-hand cursor while over the (otherwise-plain-painted) wordmark, so
// it reads as clickable the same way the powered-by badge does.
void LevelAnchorAudioProcessorEditor::mouseMove(const MouseEvent& e)
{
    setMouseCursor(wordmarkBounds().contains(e.getPosition())
                       ? MouseCursor::PointingHandCursor
                       : MouseCursor::NormalCursor);
}
