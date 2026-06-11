#include "ThemeSwitcherComponent.h"

#include "ColourScheme.h"

#include <utility>

namespace
{
Colour swatchColour(const char* argb)
{
    return Colour::fromString(String(argb));
}
} // namespace

ThemeSwitcherComponent::ThemeSwitcherComponent(ThemeChangedCallback callback)
    : onThemeChanged(std::move(callback))
{
    setName("Theme Switcher");
    setTooltip("Switch colour theme");
    setMouseCursor(MouseCursor::PointingHandCursor);
}

void ThemeSwitcherComponent::setThemeChangedCallback(ThemeChangedCallback callback)
{
    onThemeChanged = std::move(callback);
}

int ThemeSwitcherComponent::getPreferredWidth() const
{
    return 136;
}

const std::array<ThemeSwitcherComponent::ThemeSwatch, 5>& ThemeSwitcherComponent::getThemeSwatches()
{
    static const std::array<ThemeSwatch, 5> themes{{
        {"Midnight", swatchColour("ff00d9ff"), swatchColour("ff1a1a2e")},
        {"Synthwave", swatchColour("ffff2bff"), swatchColour("ff0d0221")},
        {"Deep Ocean", swatchColour("ff00c8ff"), swatchColour("ff0a1628")},
        {"Forest", swatchColour("ff7edb7a"), swatchColour("ff172514")},
        {"Daylight", swatchColour("ff0077cc"), swatchColour("ffe8ebef")},
    }};

    return themes;
}

void ThemeSwitcherComponent::paint(Graphics& g)
{
    auto& palette = ColourScheme::getInstance().colours;
    const auto bounds = getLocalBounds().toFloat().reduced(1.0f);

    g.setColour(palette["Window Background"].darker(0.55f).withAlpha(0.22f));
    g.fillRoundedRectangle(bounds.translated(0.0f, 1.4f), 12.0f);

    ColourGradient panelFill(palette["Dialog Inner Background"].withAlpha(0.72f), bounds.getX(), bounds.getY(),
                             palette["Field Background"].withAlpha(0.78f), bounds.getX(), bounds.getBottom(), false);
    g.setGradientFill(panelFill);
    g.fillRoundedRectangle(bounds, 12.0f);

    g.setColour(palette["Plugin Border"].withAlpha(0.62f));
    g.drawRoundedRectangle(bounds, 12.0f, 1.0f);

    const auto& themes = getThemeSwatches();
    for (size_t i = 0; i < themes.size(); ++i)
    {
        const auto dot = swatchBounds[i];
        if (dot.isEmpty())
            continue;

        const auto active = isThemeActive(themes[i]);
        const auto hovered = hoveredIndex == static_cast<int>(i);
        auto centre = dot.getCentre();
        auto radius = dot.getWidth() * 0.5f;

        if (active)
        {
            g.setColour(themes[i].accent.withAlpha(0.20f));
            g.fillEllipse(Rectangle<float>(radius * 2.8f, radius * 2.8f).withCentre(centre));
            g.setColour(themes[i].accent.withAlpha(0.92f));
            g.drawEllipse(dot.expanded(3.0f), 2.0f);
        }

        if (hovered && !active)
        {
            g.setColour(themes[i].accent.withAlpha(0.15f));
            g.fillEllipse(dot.expanded(3.0f));
        }

        g.setColour(palette["Window Background"].darker(0.55f).withAlpha(0.46f));
        g.fillEllipse(dot.translated(0.0f, 1.2f));

        g.setColour(themes[i].background);
        g.fillEllipse(dot);

        auto inner = dot.reduced(3.0f);
        ColourGradient accentFill(themes[i].accent.brighter(0.24f), inner.getX(), inner.getY(),
                                  themes[i].accent.darker(0.22f), inner.getX(), inner.getBottom(), false);
        g.setGradientFill(accentFill);
        g.fillEllipse(inner);

        g.setColour(Colours::white.withAlpha(active || hovered ? 0.38f : 0.18f));
        g.fillEllipse(inner.withSizeKeepingCentre(inner.getWidth() * 0.42f, inner.getHeight() * 0.42f)
                          .translated(-inner.getWidth() * 0.13f, -inner.getHeight() * 0.13f));

        g.setColour(palette["Text Colour"].withAlpha(active ? 0.52f : 0.18f));
        g.drawEllipse(dot.reduced(0.5f), 1.0f);
    }
}

void ThemeSwitcherComponent::resized()
{
    rebuildHitboxes();
}

void ThemeSwitcherComponent::mouseMove(const MouseEvent& event)
{
    const auto index = getHitIndex(event.position);
    if (hoveredIndex != index)
    {
        hoveredIndex = index;
        repaint();
    }
}

void ThemeSwitcherComponent::mouseExit(const MouseEvent&)
{
    if (hoveredIndex != -1)
    {
        hoveredIndex = -1;
        repaint();
    }
}

void ThemeSwitcherComponent::mouseDown(const MouseEvent& event)
{
    const auto index = getHitIndex(event.position);
    const auto& themes = getThemeSwatches();
    if (index < 0 || index >= static_cast<int>(themes.size()) || !onThemeChanged)
        return;

    onThemeChanged(themes[(size_t)index].name);
}

int ThemeSwitcherComponent::getHitIndex(Point<float> position) const
{
    for (size_t i = 0; i < swatchBounds.size(); ++i)
        if (swatchBounds[i].expanded(3.0f).contains(position))
            return static_cast<int>(i);

    return -1;
}

bool ThemeSwitcherComponent::isThemeActive(const ThemeSwatch& swatch) const
{
    auto& scheme = ColourScheme::getInstance();
    if (scheme.presetName == swatch.name)
        return true;

    const auto accent = scheme.colours.find("Accent Colour");
    const auto stageTop = scheme.colours.find("Stage Background Top");
    return accent != scheme.colours.end() && stageTop != scheme.colours.end() && accent->second == swatch.accent &&
           stageTop->second == swatch.background;
}

void ThemeSwitcherComponent::rebuildHitboxes()
{
    for (auto& bounds : swatchBounds)
        bounds = {};

    auto area = getLocalBounds().toFloat().reduced(7.0f, 5.0f);
    if (area.getWidth() <= 0.0f || area.getHeight() <= 0.0f)
        return;

    const auto dotSize = jlimit(14.0f, 20.0f, area.getHeight());
    const auto gap = jlimit(4.0f, 7.0f, (area.getWidth() - dotSize * 5.0f) / 4.0f);
    const auto totalWidth = dotSize * 5.0f + gap * 4.0f;
    auto x = area.getRight() - totalWidth;
    const auto y = area.getCentreY() - dotSize * 0.5f;

    for (auto& dot : swatchBounds)
    {
        dot = {x, y, dotSize, dotSize};
        x += dotSize + gap;
    }
}
