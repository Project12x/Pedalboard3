//	ToastOverlay.h - Simple toast notification overlay for the Pedalboard app.
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

#ifndef TOASTOVERLAY_H_
#define TOASTOVERLAY_H_

#include "JuceHeader.h"

#include <melatonin_blur/melatonin_blur.h>
#include <queue>

/**
 * A simple toast notification overlay that displays temporary messages.
 * Use ToastOverlay::getInstance().show("Message") from anywhere.
 */
class ToastOverlay : public Component, public Timer
{
  public:
    static ToastOverlay& getInstance();

    /** Show a toast notification with the given message */
    void show(const String& message, int durationMs = 2500);

    /** Paint override */
    void paint(Graphics& g) override;

    /** Called when parent is resized - repositions the toast */
    void parentSizeChanged() override;

  private:
    ToastOverlay();
    ~ToastOverlay() override;

    void timerCallback() override;
    void fadeIn();
    void fadeOut();
    void showNextToast();

    String currentMessage;
    float alpha = 0.0f;
    bool isFadingOut = false;
    int displayDurationMs = 2500;
    int fadeStepMs = 16; // ~60fps

    std::queue<std::pair<String, int>> pendingToasts;

    // Melatonin Blur drop shadow for premium UI
    melatonin::DropShadow dropShadow{Colours::black, 15, {0, 4}, 0};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ToastOverlay)
};

#endif // TOASTOVERLAY_H_
