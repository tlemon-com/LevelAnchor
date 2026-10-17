#pragma once

#include <JuceHeader.h>
#include "Palette.h"
#include "Fonts.h"

/*
    DelayButton + ModePanel -- choosing the engine mode.
    Header-only. Siblings: ui/AboutPanel.h (same overlay style), ui/Palette.h.

    DelayButton: small button under RESET showing the delay the plugin adds
    right now ("DELAY 50 ms"). Click opens the ModePanel (wired by the editor).

    ModePanel: full-editor overlay listing every engine mode with its delay.
    Modes this build does not offer are still listed, dimmed and marked as part
    of the licensed engine, so the panel also shows what LevelAPI can do; a
    note under the list says how to get them, and clicking a locked row lights
    it up. The selected row is the mode in force. Picking another one calls onSelect;
    the processor does the rest. Closes on X, click outside, or Escape.
*/

// ===========================================================================
class DelayButton : public juce::Component
{
public:
    std::function<void()> onClick;

    DelayButton()
    {
        setMouseCursor(juce::MouseCursor::PointingHandCursor);
    }

    /** e.g. "50 ms". Repaints only when it changes. */
    void setDelayText(const juce::String& s)
    {
        if (s == delayText)
            return;
        delayText = s;
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        // The RESET button's colours, so the two read as one control group.
        const auto r = getLocalBounds().toFloat().reduced(0.5f);
        g.setColour(juce::Colour(hovered ? 0xff1e261e : 0xff141814));
        g.fillRoundedRectangle(r, 3.0f);
        g.setColour(juce::Colour(0xff2e3a2e));
        g.drawRoundedRectangle(r, 3.0f, 1.0f);

        g.setColour(hovered ? juce::Colours::white : juce::Colours::grey);
        g.setFont(juce::Font(juce::FontOptions(9.0f)).withExtraKerningFactor(0.10f));
        g.drawText("DELAY " + delayText, getLocalBounds(), juce::Justification::centred);
    }

    void mouseEnter(const juce::MouseEvent&) override { hovered = true;  repaint(); }
    void mouseExit (const juce::MouseEvent&) override { hovered = false; repaint(); }
    void mouseUp(const juce::MouseEvent& e) override
    {
        if (getLocalBounds().contains(e.getPosition()) && onClick)
            onClick();
    }

private:
    juce::String delayText { "--" };
    bool hovered = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DelayButton)
};

// ===========================================================================
class ModePanel : public juce::Component
{
public:
    static constexpr int kNumModes = 3; // must match LevelAnchorAudioProcessor::kNumModes

    /** Called with the mode the user picked; only ever an offered one. */
    std::function<void(int)> onSelect;

    ModePanel()
    {
        logo = juce::Drawable::createFromImageData(
            BinaryData::levelapi2logo_svg, BinaryData::levelapi2logo_svgSize);

        for (int m = 0; m < kNumModes; ++m)
        {
            rows[m].mode = m;
            rows[m].onClick = [this](int mode)
            {
                if (onSelect)
                    onSelect(mode);
            };
            // A locked mode cannot be picked; clicking it lights up the
            // license note under the list instead.
            rows[m].onLockedClick = [this](int)
            {
                lockedNoteLit = true;
                repaint(pitchRect());
            };
            addAndMakeVisible(rows[m]);
        }

        // Styled like the About panel's buttons (and RESET).
        for (auto* b : { &visitButton, &closeButton })
        {
            b->setColour(juce::TextButton::buttonColourId,   juce::Colour(0xff141814));
            b->setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff1e261e));
            b->setColour(juce::TextButton::textColourOffId,  juce::Colours::white);
            b->setColour(juce::TextButton::textColourOnId,   juce::Colours::white);
            b->setColour(juce::ComboBox::outlineColourId,    juce::Colour(0xff2e3a2e));
        }

        visitButton.setButtonText("visit levelapi.dev");
        visitButton.onClick = [] { juce::URL(kSiteUrl).launchInDefaultBrowser(); };
        addAndMakeVisible(visitButton);

        closeButton.setButtonText("x");
        closeButton.onClick = [this] { setVisible(false); };
        addAndMakeVisible(closeButton);

        setWantsKeyboardFocus(true);
    }

    /** Whether this build lets the user select the mode; others show locked. */
    void setOffered(int mode, bool offered)
    {
        if (mode >= 0 && mode < kNumModes)
            rows[mode].setOffered(offered);

        anyLocked = false;
        for (const auto& row : rows)
            anyLocked = anyLocked || !row.isOffered();
        repaint();
    }

    /** The mode in force, shown as the selected row. Cheap to call every tick. */
    void setSelectedMode(int mode)
    {
        for (int m = 0; m < kNumModes; ++m)
            rows[m].setSelected(m == mode);
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colours::black.withAlpha(0.75f)); // dim the editor behind

        const auto panel = panelRect().toFloat();
        g.setColour(juce::Colour(0xff10140f));
        g.fillRoundedRectangle(panel, 8.0f);
        g.setColour(Palette::kBrand.withAlpha(0.6f));
        g.drawRoundedRectangle(panel, 8.0f, 1.0f);

        auto content = panelRect().reduced(20);

        // Header: logo, title, and what a switch costs
        auto header = content.removeFromTop(kHeaderH);
        if (logo != nullptr)
            logo->drawWithin(g, header.removeFromLeft(40).toFloat().reduced(2.0f),
                             juce::RectanglePlacement::centred, 1.0f);
        header.removeFromLeft(10);
        g.setColour(juce::Colours::white);
        g.setFont(Fonts::wordmark(20.0f).withExtraKerningFactor(0.06f));
        g.drawText("ENGINE MODE", header.removeFromTop(24), juce::Justification::bottomLeft);
        g.setColour(juce::Colours::grey);
        g.setFont(juce::Font(juce::FontOptions(11.0f)));
        g.drawText("Switching restarts the measurement, with a short gap in the audio.",
                   header, juce::Justification::topLeft);

        // Below the rows: how to get the locked modes, lit up when one is
        // clicked. With nothing locked, a general line about the engine.
        if (anyLocked)
        {
            g.setColour(lockedNoteLit ? juce::Colours::white : Palette::kBrand.withAlpha(0.85f));
            g.setFont(juce::Font(juce::FontOptions(13.0f, lockedNoteLit ? juce::Font::bold
                                                                        : juce::Font::plain)));
            g.drawFittedText(kLockedNote, pitchRect(), juce::Justification::topLeft, 2);
        }
        else
        {
            g.setColour(Palette::kBrand.withAlpha(0.85f));
            g.setFont(juce::Font(juce::FontOptions(13.0f)));
            g.drawFittedText(kEngineNote, pitchRect(), juce::Justification::topLeft, 2);
        }
    }

    void resized() override
    {
        const auto panel = panelRect();
        auto content = panel.reduced(20);
        content.removeFromTop(kHeaderH + 12);

        for (int m = 0; m < kNumModes; ++m)
        {
            rows[m].setBounds(content.removeFromTop(kRowH));
            content.removeFromTop(kRowGap);
        }

        auto bottomRow = panel.reduced(20).removeFromBottom(28);
        visitButton.setBounds(bottomRow.removeFromLeft(150));

        closeButton.setBounds(panel.getRight() - 30, panel.getY() + 6, 24, 22);
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        if (!panelRect().contains(e.getPosition()))
            setVisible(false); // click outside the panel closes it
    }

    bool keyPressed(const juce::KeyPress& k) override
    {
        if (k == juce::KeyPress::escapeKey) { setVisible(false); return true; }
        return false;
    }

    void visibilityChanged() override
    {
        if (isVisible())
            grabKeyboardFocus(); // so Escape works immediately
        else
            lockedNoteLit = false; // the next opening starts calm
    }

private:
    // The note under the list. kLockedNote shows whenever a mode is locked.
    static constexpr const char* kLockedNote =
        "The 2 ms low-latency modes come with a full LevelAPI\n"
        "license. Ask about it at levelapi.dev.";
    static constexpr const char* kEngineNote =
        "Every mode is part of the LevelAPI engine.\n"
        "License it for your own product.";

    // Its own ref, so the site can tell visits from this panel apart from the
    // About panel's.
    static constexpr const char* kSiteUrl = "https://levelapi.dev/levelanchor/?ref=levelanchor-modes";

    static constexpr int kHeaderH = 40;
    static constexpr int kRowH    = 50;
    static constexpr int kRowGap  = 5;

    // Index = mode. The delays are the nominal figures; the DelayButton shows
    // the exact one at the current rate.
    struct ModeInfo { const char* name; const char* delay; const char* description; };
    static constexpr ModeInfo kModes[kNumModes] = {
        { "Standard",              "50 ms", "Look-ahead peak control. The default, for any source." },
        { "Low latency",           "2 ms",  "The same loudness ride with 2 ms of delay. For any material." },
        { "Low latency, dialogue", "2 ms",  "Holds the dialogue at the target instead of the full mix. For speech." },
    };

    // ---------------------------------------------------------------------
    class ModeRow : public juce::Component
    {
    public:
        int mode = 0;
        std::function<void(int)> onClick;       // an offered mode was picked
        std::function<void(int)> onLockedClick; // a locked mode was clicked

        ModeRow() { setMouseCursor(juce::MouseCursor::PointingHandCursor); }

        bool isOffered() const noexcept { return offered; }

        void setOffered(bool o)
        {
            if (o == offered)
                return;
            offered = o;
            repaint();
        }

        void setSelected(bool s)
        {
            if (s == selected)
                return;
            selected = s;
            repaint();
        }

        void paint(juce::Graphics& g) override
        {
            const auto& info = kModes[mode];
            const auto r = getLocalBounds().toFloat().reduced(0.5f);

            g.setColour(juce::Colour(selected ? 0xff1e261e
                                   : (hovered && offered) ? 0xff181f18 : 0xff141814));
            g.fillRoundedRectangle(r, 5.0f);
            g.setColour(selected ? Palette::kBrand.withAlpha(0.7f) : juce::Colour(0xff2e3a2e));
            g.drawRoundedRectangle(r, 5.0f, 1.0f);

            // Radio dot
            const juce::Rectangle<float> dot(10.0f, 12.0f, 10.0f, 10.0f);
            if (selected)
            {
                g.setColour(Palette::kBrand);
                g.fillEllipse(dot);
            }
            else
            {
                g.setColour(offered ? juce::Colours::grey : juce::Colour(0xff3a3a3a));
                g.drawEllipse(dot, 1.0f);
            }

            auto area = getLocalBounds().withTrimmedLeft(30).withTrimmedRight(12);
            auto top  = area.removeFromTop(28).withTrimmedTop(6);

            // Delay, then the LICENSE tag left of it for a locked mode
            g.setColour(offered ? Palette::kBrand : juce::Colours::grey);
            g.setFont(juce::Font(juce::FontOptions(14.0f, juce::Font::bold)));
            g.drawText(info.delay, top.removeFromRight(50), juce::Justification::centredRight);

            if (!offered)
            {
                g.setColour(Palette::kBrand.withAlpha(0.8f));
                g.setFont(juce::Font(juce::FontOptions(9.0f)).withExtraKerningFactor(0.20f));
                g.drawText("LICENSE", top.removeFromRight(70), juce::Justification::centredRight);
            }

            g.setColour(offered ? juce::Colours::white : juce::Colours::grey);
            g.setFont(juce::Font(juce::FontOptions(14.0f)));
            g.drawText(info.name, top, juce::Justification::centredLeft);

            g.setColour(offered ? juce::Colours::lightgrey : juce::Colour(0xff6a6a6a));
            g.setFont(juce::Font(juce::FontOptions(11.5f)));
            g.drawFittedText(info.description, area.withTrimmedBottom(4),
                             juce::Justification::centredLeft, 1, 0.85f);
        }

        void mouseEnter(const juce::MouseEvent&) override { hovered = true;  repaint(); }
        void mouseExit (const juce::MouseEvent&) override { hovered = false; repaint(); }
        void mouseUp(const juce::MouseEvent& e) override
        {
            if (!getLocalBounds().contains(e.getPosition()))
                return;

            if (!offered)
            {
                if (onLockedClick)
                    onLockedClick(mode);
            }
            else if (!selected && onClick)
            {
                onClick(mode);
            }
        }

    private:
        bool offered  = true;
        bool selected = false;
        bool hovered  = false;
    };
    // ---------------------------------------------------------------------

    juce::Rectangle<int> panelRect() const
    {
        return getLocalBounds().withSizeKeepingCentre(
            juce::jmin(420, getWidth() - 40), juce::jmin(340, getHeight() - 40));
    }

    // Between the last row and the bottom button row.
    juce::Rectangle<int> pitchRect() const
    {
        const int top = panelRect().reduced(20).getY() + kHeaderH + 12
                      + kNumModes * (kRowH + kRowGap) + 5;
        return { panelRect().reduced(20).getX(), top, panelRect().reduced(20).getWidth(), 36 };
    }

    std::unique_ptr<juce::Drawable> logo;
    ModeRow rows[kNumModes];
    juce::TextButton visitButton, closeButton;
    bool anyLocked     = false; // some mode is not offered: show kLockedNote
    bool lockedNoteLit = false; // a locked row was clicked since the panel opened

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModePanel)
};
