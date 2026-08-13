#pragma once

#include <JuceHeader.h>
#include "Palette.h"
#include "Fonts.h"
#include "../BuildInfo.h"

/*
    AboutPanel + PoweredByBadge -- the funnel surface of LevelAnchor.
    Header-only. Siblings: ui/PixelScreen.h, ui/TargetStrip.h, ui/LedFont5x7.h.

    PoweredByBadge: two-line centered badge -- "POWERED BY" over
    "LEVELAPI.DEV", sitting bottom-center of the editor on the same axis as the
    screen watermark. Click opens the AboutPanel (wired by the editor via
    onClick).

    AboutPanel: full-editor overlay. Pitch text, live "anchored at X LUFS for
    HH:MM:SS" line, visit/copy-link buttons for levelapi.dev (with
    ?ref=levelanchor so plugin-driven traffic is attributable), and a small
    tlemon.com creator credit using the tree logo. Closes on X, click outside
    the panel, or Escape.
*/

// ===========================================================================
class PoweredByBadge : public juce::Component
{
public:
    std::function<void()> onClick;

    PoweredByBadge()
    {
        setMouseCursor(juce::MouseCursor::PointingHandCursor);
    }

    void paint(juce::Graphics& g) override
    {
        // Two centered runs. No logo here: the mark already appears as the
        // screen watermark directly above, on this same center axis.
        auto area = getLocalBounds();
        const auto topHalf    = area.removeFromTop((int)(area.getHeight() * 0.45f));
        const auto bottomHalf = area;

        g.setColour(juce::Colours::grey.withAlpha(hovered ? 1.0f : 0.8f));
        g.setFont(juce::Font(juce::FontOptions(9.0f)).withExtraKerningFactor(0.30f));
        g.drawText("POWERED BY", topHalf, juce::Justification::centredBottom);

        g.setColour(juce::Colours::white.withAlpha(hovered ? 1.0f : 0.85f));
        g.setFont(Fonts::wordmark(20.8f).withExtraKerningFactor(0.10f));
        g.drawText("LEVELAPI.DEV", bottomHalf.translated(0, 2), juce::Justification::centredTop);
    }

    void mouseEnter(const juce::MouseEvent&) override { hovered = true;  repaint(); }
    void mouseExit (const juce::MouseEvent&) override { hovered = false; repaint(); }
    void mouseUp(const juce::MouseEvent& e) override
    {
        if (getLocalBounds().contains(e.getPosition()) && onClick)
            onClick();
    }

private:
    bool hovered = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PoweredByBadge)
};

// ===========================================================================
class AboutPanel : public juce::Component
{
public:
    AboutPanel()
    {
        logo = juce::Drawable::createFromImageData(
            BinaryData::levelapi2logo_svg, BinaryData::levelapi2logo_svgSize);
        treeLogo = juce::Drawable::createFromImageData(
            BinaryData::logotree_svg, BinaryData::logotree_svgSize);

        // Styled to match the main editor's RESET button (PluginEditor.cpp)
        // so the panel reads as the same piece of hardware: dark fill, thin
        // muted-green border, white text -- nothing here should look like a
        // stock OS button.
        for (auto* b : { &visitButton, &copyButton, &closeButton })
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

        copyButton.setButtonText("copy link");
        copyButton.onClick = [this]
        {
            juce::SystemClipboard::copyTextToClipboard(kSiteUrl);
            copyButton.setButtonText("copied!");
            // Named local instead of init-capturing SafePointer(this) directly:
            // MSVC mis-resolves `this` inside a nested lambda's init-capture.
            const juce::Component::SafePointer<AboutPanel> safe(this);
            juce::Timer::callAfterDelay(1200, [safe]
            {
                if (safe != nullptr)
                    safe->copyButton.setButtonText("copy link");
            });
        };
        addAndMakeVisible(copyButton);

        closeButton.setButtonText("x");
        closeButton.onClick = [this] { setVisible(false); };
        addAndMakeVisible(closeButton);

        // Secondary credit: the main creator's mark
        creditLink.setButtonText("created by tlemon.com");
        creditLink.setURL(juce::URL("https://tlemon.com/?ref=levelanchor"));
        creditLink.setFont(juce::Font(juce::FontOptions(12.0f)), false,
                           juce::Justification::centredLeft);
        addAndMakeVisible(creditLink);

        setWantsKeyboardFocus(true);
    }

    /** Live line, e.g. "anchored at -14.0 LUFS for 01:23:45". Cheap to call
        every tick; repaints only when the text changes (once per second). */
    void setStatusLine(const juce::String& s)
    {
        if (s == statusLine)
            return;
        statusLine = s;
        repaint(panelRect());
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

        // Header: logo + name + version
        auto header = content.removeFromTop(40);
        if (logo != nullptr)
            logo->drawWithin(g, header.removeFromLeft(40).toFloat().reduced(2.0f),
                             juce::RectanglePlacement::centred, 1.0f);
        header.removeFromLeft(10);
        g.setColour(juce::Colours::white);
        g.setFont(Fonts::wordmark(20.0f).withExtraKerningFactor(0.06f));
        g.drawText("LEVELANCHOR", header.removeFromTop(24), juce::Justification::bottomLeft);
        g.setColour(juce::Colours::grey);
        g.setFont(juce::Font(juce::FontOptions(11.0f)));
        // From BuildInfo.h, not JucePlugin_VersionString: this carries the
        // per-build ID, so a support question can name an exact build.
        g.drawText(LEVELANCHOR_VERSION_LABEL,
                   header, juce::Justification::topLeft);

        content.removeFromTop(12);

        // Pitch: leads with the engine (not the plugin) and states the
        // license requirement plainly. The EUR500k org-revenue figure is a
        // hard threshold, not an approximation -- keep the wording exact.
        g.setColour(juce::Colours::lightgrey);
        g.setFont(juce::Font(juce::FontOptions(15.0f)));
        g.drawFittedText(
            "Real-time R128 loudness anchoring\n" 
            "Powered by the LevelAPI engine.\n"
            "The compiled plugin with DSP, FREE for individuals and\n"
            "organizations < EUR500k annual revenue;\n"
            "a LevelAPI license is required above that threshold.",
            content.removeFromTop(76), juce::Justification::topLeft, 4);

        content.removeFromTop(6);

        // Dev/business callout: the reference-integration funnel -- distinct
        // from the streamer pitch above.
        g.setColour(Palette::kBrand.withAlpha(0.85f));
        g.setFont(juce::Font(juce::FontOptions(13.5f)));
        g.drawFittedText(
            "Need real-time loudness measurement & adjustment in your own\n"
            "product? License the engine directly, below.",
            content.removeFromTop(32), juce::Justification::topLeft, 2);

        content.removeFromTop(6);

        // Live status line
        g.setColour(Palette::kBrand);
        g.setFont(juce::Font(juce::FontOptions(13.0f, juce::Font::bold)));
        g.drawText(statusLine, content.removeFromTop(20), juce::Justification::centredLeft);

        // Small tree logo next to the credit link (link itself is a child)
        if (treeLogo != nullptr)
            treeLogo->drawWithin(g, creditLogoRect().toFloat(),
                                 juce::RectanglePlacement::centred, 0.9f);
    }

    void resized() override
    {
        const auto panel  = panelRect();
        auto bottomRow    = panel.reduced(20).removeFromBottom(28);

        visitButton.setBounds(bottomRow.removeFromLeft(150));
        bottomRow.removeFromLeft(10);
        copyButton.setBounds(bottomRow.removeFromLeft(90));

        creditLink.setBounds(creditLogoRect().getRight() + 4,
                             creditLogoRect().getY() - 2,
                             160, 24);

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
    }

private:
    static constexpr const char* kSiteUrl = "https://levelapi.dev/levelanchor/?ref=levelanchor";

    juce::Rectangle<int> panelRect() const
    {
        return getLocalBounds().withSizeKeepingCentre(
            juce::jmin(420, getWidth() - 40), juce::jmin(320, getHeight() - 40));
    }

    juce::Rectangle<int> creditLogoRect() const
    {
        const auto panel = panelRect().reduced(20);
        return { panel.getX(), panel.getBottom() - 28 - 8 - 20, 20, 20 };
    }

    std::unique_ptr<juce::Drawable> logo, treeLogo;
    juce::TextButton visitButton, copyButton, closeButton;
    juce::HyperlinkButton creditLink;
    // Replaced by setStatusLine on the first UI tick that carries audio; this is
    // only what shows before any has arrived.
    juce::String statusLine { "waiting for audio..." };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AboutPanel)
};
