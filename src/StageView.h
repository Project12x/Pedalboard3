/*
  ==============================================================================

    StageView.h
    Performance/Stage Mode - Fullscreen overlay for live use
    Large fonts, quick patch navigation, integrated tuner

  ==============================================================================
*/

#pragma once

#include "JuceHeader.h"

class MainPanel;
class TunerProcessor;
class TunerControl;

//==============================================================================
/**
    Fullscreen performance view optimized for live use.

    Features:
    - Large patch name display (72pt+)
    - Quick prev/next patch navigation
    - Integrated tuner (large display)
    - Panic button
    - Keyboard/foot controller friendly
*/
class StageView : public Component, public Button::Listener, public Timer
{
  public:
    StageView(MainPanel* panel);
    ~StageView();

    //==========================================================================
    // Update methods (called by MainPanel)
    void updatePatchInfo(const String& patchName, const String& nextPatchName, int currentIndex, int totalPatches);
    void setTunerProcessor(TunerProcessor* tuner);

    //==========================================================================
    // Component overrides
    void paint(Graphics& g) override;
    void resized() override;
    void buttonClicked(Button* button) override;
    bool keyPressed(const KeyPress& key) override;

    //==========================================================================
    // Timer for tuner updates
    void timerCallback() override;

  private:
    MainPanel* mainPanel = nullptr;
    TunerProcessor* tunerProcessor = nullptr;

    // Current state
    // Current state
    String currentPatchName = "No Patch";
    String nextPatchName = "";
    int currentPatchIndex = 0;
    int totalPatchCount = 0;

    // Tuner state
    float displayedCents = 0.0f;
    float needleAngle = 0.0f;
    int detectedNote = -1;
    bool showTuner = true;

    // UI Components
    std::unique_ptr<TextButton> prevButton;
    std::unique_ptr<TextButton> nextButton;
    std::unique_ptr<TextButton> panicButton;
    std::unique_ptr<TextButton> exitButton;
    std::unique_ptr<TextButton> tunerToggleButton;

    // Drawing helpers
    void drawPatchDisplay(Graphics& g, Rectangle<float> bounds);
    void drawTunerDisplay(Graphics& g, Rectangle<float> bounds);
    void drawStatusBar(Graphics& g, Rectangle<float> bounds);
    String getNoteName(int midiNote) const;
    Colour getTuningColour(float cents) const;
    void updateAfterPatchChange();

    // Smoothing constants
    static constexpr float NEEDLE_SMOOTHING = 0.15f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StageView)
};
