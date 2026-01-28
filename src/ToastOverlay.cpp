//	ToastOverlay.cpp - Simple toast notification overlay for the Pedalboard app.
//	----------------------------------------------------------------------------
//	This file is part of Pedalboard3, an audio plugin host.
//	Copyright (c) 2024 Niall Moody.
//
//	This program is free software: you can redistribute it and/or modify
//	it under the terms of the GNU General Public License as published by
//	the Free Software Foundation, either version 3 of the License, or
//	(at your option) any later version.
//
//	This program is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//	GNU General Public License for more details.
//
//	You should have received a copy of the GNU General Public License
//	along with this program.  If not, see <http://www.gnu.org/licenses/>.
//	----------------------------------------------------------------------------

#include "ToastOverlay.h"

#include "ColourScheme.h"
#include "FontManager.h"

//------------------------------------------------------------------------------
ToastOverlay& ToastOverlay::getInstance()
{
    static ToastOverlay instance;
    return instance;
}

//------------------------------------------------------------------------------
ToastOverlay::ToastOverlay()
{
    setInterceptsMouseClicks(false, false);
    setAlwaysOnTop(true);
}

//------------------------------------------------------------------------------
ToastOverlay::~ToastOverlay()
{
    stopTimer();
}

//------------------------------------------------------------------------------
void ToastOverlay::show(const String& message, int durationMs)
{
    // If currently showing, queue this toast
    if (alpha > 0.0f || isTimerRunning())
    {
        pendingToasts.push({message, durationMs});
        return;
    }

    currentMessage = message;
    displayDurationMs = durationMs;
    alpha = 0.0f;
    isFadingOut = false;

    // Calculate size based on message
    auto font = FontManager::getInstance().getUIFont(14.0f);
    int textWidth = font.getStringWidth(currentMessage);
    int width = jmax(200, textWidth + 48);
    int height = 40;

    setSize(width, height);

    // Position at bottom center of parent
    if (auto* parent = getParentComponent())
    {
        int x = (parent->getWidth() - width) / 2;
        int y = parent->getHeight() - height - 60;
        setBounds(x, y, width, height);
        DBG("ToastOverlay: Showing at " + String(x) + ", " + String(y) + " in parent " + String(parent->getWidth()) +
            "x" + String(parent->getHeight()));
    }
    else
    {
        // Fallback: use desktop bounds
        auto desktopBounds = Desktop::getInstance().getDisplays().getPrimaryDisplay()->userArea;
        int x = (desktopBounds.getWidth() - width) / 2;
        int y = desktopBounds.getHeight() - height - 100;
        setBounds(x, y, width, height);
        DBG("ToastOverlay: No parent, using desktop position " + String(x) + ", " + String(y));
    }

    setVisible(true);
    toFront(false);
    fadeIn();
}

//------------------------------------------------------------------------------
void ToastOverlay::paint(Graphics& g)
{
    if (alpha <= 0.0f)
        return;

    auto bounds = getLocalBounds().toFloat();

    // Background with rounded corners and shadow effect
    auto bgColour = ColourScheme::getInstance().colours["Button Colour"].darker(0.3f).withAlpha(0.95f * alpha);

    // Subtle shadow
    g.setColour(Colours::black.withAlpha(0.3f * alpha));
    g.fillRoundedRectangle(bounds.translated(2, 2), 8.0f);

    // Main background
    g.setColour(bgColour);
    g.fillRoundedRectangle(bounds, 8.0f);

    // Border
    g.setColour(ColourScheme::getInstance().colours["Text Colour"].withAlpha(0.2f * alpha));
    g.drawRoundedRectangle(bounds.reduced(0.5f), 8.0f, 1.0f);

    // Text
    g.setColour(ColourScheme::getInstance().colours["Text Colour"].withAlpha(alpha));
    g.setFont(FontManager::getInstance().getUIFont(14.0f));
    g.drawText(currentMessage, bounds, Justification::centred, false);
}

//------------------------------------------------------------------------------
void ToastOverlay::parentSizeChanged()
{
    if (auto* parent = getParentComponent())
    {
        int x = (parent->getWidth() - getWidth()) / 2;
        int y = parent->getHeight() - getHeight() - 60;
        setTopLeftPosition(x, y);
    }
}

//------------------------------------------------------------------------------
void ToastOverlay::timerCallback()
{
    if (isFadingOut)
    {
        alpha -= 0.08f;
        if (alpha <= 0.0f)
        {
            alpha = 0.0f;
            stopTimer();
            setVisible(false);
            showNextToast();
        }
    }
    else
    {
        alpha += 0.15f;
        if (alpha >= 1.0f)
        {
            alpha = 1.0f;
            stopTimer();
            // Wait for display duration, then fade out
            Timer::callAfterDelay(displayDurationMs, [this]() { fadeOut(); });
        }
    }
    repaint();
}

//------------------------------------------------------------------------------
void ToastOverlay::fadeIn()
{
    isFadingOut = false;
    startTimerHz(60);
}

//------------------------------------------------------------------------------
void ToastOverlay::fadeOut()
{
    isFadingOut = true;
    startTimerHz(60);
}

//------------------------------------------------------------------------------
void ToastOverlay::showNextToast()
{
    if (!pendingToasts.empty())
    {
        auto [msg, duration] = pendingToasts.front();
        pendingToasts.pop();
        show(msg, duration);
    }
}
