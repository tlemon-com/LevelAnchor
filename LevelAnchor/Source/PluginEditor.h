#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "ui/PixelScreen.h"
#include "ui/MatrixReadout.h"
#include "ui/TargetStrip.h"
#include "ui/AboutPanel.h"
#include "ui/PeakLed.h"

// ===============================================================
// Editor Class
// ===============================================================
class LevelAnchorAudioProcessorEditor
    : public juce::AudioProcessorEditor,
    private juce::Timer
{
public:
    explicit LevelAnchorAudioProcessorEditor(LevelAnchorAudioProcessor&);
    ~LevelAnchorAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    // ===============================================================
    // Processor Reference
    // ===============================================================
    LevelAnchorAudioProcessor& processorRef;

    // Raw "targetLUFS" parameter value (owned by the APVTS; read each tick)
    std::atomic<float>* targetParamValue = nullptr;

    // ===============================================================
    // UI Elements
    // ===============================================================
    PixelScreen pixelScreen;        // the LCD centerpiece: trace, target row, logo watermark/status lamp

    // Created in the constructor once the "targetLUFS" parameter is looked up
    std::unique_ptr<TargetStrip> targetStrip; // 32-LED touch strip, left side

    juce::TextButton resetButton;

    PeakLed peakLed;                // top bar, on the logo's axis: peak stage acting

    PoweredByBadge poweredBy;       // bottom-center "powered by" badge; opens the About panel
    AboutPanel     aboutPanel;      // full-editor overlay (hidden by default)

    // Small LCD windows -- same 5x7 font and cell pitch as the big screen
    MatrixReadout timerReadout;     // top-right: elapsed time / warmup countdown
    MatrixReadout targetReadout;    // bottom-left: target value + unit
    MatrixReadout outputReadout;    // bottom-right: integrated LUFS + unit

    // Labels
    juce::Label devLabel;           // Deviation diagnostics (Ctrl+click background toggles)
    juce::Label timerCaption;       // "TIME" caption under the timer readout
    juce::Label peakCaption;        // "PEAK" caption under the peak LED
    juce::Label targetCaption;      // "TARGET" caption under its readout
    juce::Label outputCaption;      // "OUTPUT LEVEL" caption under its readout

    // ===============================================================
    // Drawing
    // ===============================================================
    void drawBackground(juce::Graphics& g);

    // ===============================================================
    // Internal
    // ===============================================================
    void timerCallback() override;
    void mouseDown(const juce::MouseEvent& e) override; // Ctrl+click: toggle diagnostics; also opens About if on the wordmark
    void mouseMove(const juce::MouseEvent& e) override;  // pointing-hand cursor over the wordmark
    int64_t prevIndex = -1; // matches UIDisplayData::index (64-bit sample count)

    // Peak LED state: the level shown and when its hold runs out, so a short
    // peak stays visible for a moment. See updatePeakLed().
    void updatePeakLed(float gainReductionDb);
    int          peakShown       = PeakLed::Off;
    juce::uint32 peakHoldUntilMs = 0;

    // Wordmark hit area -- widths computed once in resized() (font metrics are
    // fixed since the window doesn't resize) and reused by paint() (two-tone
    // "LEVEL"/"ANCHOR" draw) and mouseDown()/mouseMove() (click-to-open-About),
    // so the clickable area always matches what's actually drawn.
    juce::Rectangle<int> wordmarkBounds() const;
    int wordmarkLeftW  = 0;
    int wordmarkRightW = 0;

    // Set in the constructor from a per-user PropertiesFile check; consumed in
    // timerCallback() once the editor isShowing() (see PluginEditor.cpp for why).
    bool pendingAboutAutoOpen = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LevelAnchorAudioProcessorEditor)
};
