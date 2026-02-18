/*
  ==============================================================================

    OscilloscopeControl.h
    Real-time waveform display control for OscilloscopeProcessor

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <array>

class OscilloscopeProcessor;

//==============================================================================
/**
    Real-time waveform display component.
    Uses timer-driven updates to poll processor for audio data.
*/
class OscilloscopeControl : public Component, private Timer
{
  public:
    OscilloscopeControl(OscilloscopeProcessor* processor);
    ~OscilloscopeControl() override;

    void paint(Graphics& g) override;
    void resized() override;

  private:
    void timerCallback() override;

    OscilloscopeProcessor* oscilloscopeProcessor;

    static constexpr int DISPLAY_SAMPLES = 512;
    std::array<float, DISPLAY_SAMPLES> displayBuffer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OscilloscopeControl)
};
