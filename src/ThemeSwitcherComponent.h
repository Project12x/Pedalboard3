#pragma once

#include "JuceHeader.h"

#include <array>
#include <functional>

class ThemeSwitcherComponent final : public Component, public SettableTooltipClient
{
  public:
    using ThemeChangedCallback = std::function<void(const String&)>;

    explicit ThemeSwitcherComponent(ThemeChangedCallback callback = {});

    void setThemeChangedCallback(ThemeChangedCallback callback);
    int getPreferredWidth() const;

    void paint(Graphics& g) override;
    void resized() override;
    void mouseMove(const MouseEvent& event) override;
    void mouseExit(const MouseEvent& event) override;
    void mouseDown(const MouseEvent& event) override;

  private:
    struct ThemeSwatch
    {
        String name;
        Colour accent;
        Colour background;
    };

    static const std::array<ThemeSwatch, 5>& getThemeSwatches();

    int getHitIndex(Point<float> position) const;
    bool isThemeActive(const ThemeSwatch& swatch) const;
    void rebuildHitboxes();

    ThemeChangedCallback onThemeChanged;
    std::array<Rectangle<float>, 5> swatchBounds{};
    int hoveredIndex = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ThemeSwitcherComponent)
};
