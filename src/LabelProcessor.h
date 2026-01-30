//  LabelProcessor.h - A simple text label processor for canvas annotations.
//  ----------------------------------------------------------------------------
//  This file is part of Pedalboard3, an audio plugin host.
//  Copyright (c) 2024 Pedalboard3 Project.
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//  ----------------------------------------------------------------------------

#pragma once

#include "PedalboardProcessors.h"

#include <JuceHeader.h>

class LabelControl;

/**
    A simple text label processor for quick canvas annotations.
    No audio I/O - purely visual.
*/
class LabelProcessor : public PedalboardProcessor
{
  public:
    LabelProcessor();
    ~LabelProcessor();

    // AudioProcessor overrides (no-op for visual-only processor)
    void prepareToPlay(double, int) override {}
    void releaseResources() override {}
    void processBlock(AudioBuffer<float>&, MidiBuffer&) override {}

    // PedalboardProcessor overrides
    const String getName() const override { return "Label"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const String getProgramName(int) override { return {}; }
    void changeProgramName(int, const String&) override {}

    // I/O - Visual only, no channels
    const String getInputChannelName(int) const override { return ""; }
    const String getOutputChannelName(int) const override { return ""; }
    bool isInputChannelStereoPair(int) const override { return false; }
    bool isOutputChannelStereoPair(int) const override { return false; }
    bool silenceInProducesSilenceOut() const override { return true; }

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override
    {
        return layouts.inputBuses.isEmpty() && layouts.outputBuses.isEmpty();
    }

    bool hasEditor() const override { return true; }
    AudioProcessorEditor* createEditor() override;
    Component* getControls() override;
    Point<int> getSize() override { return Point<int>(120, 32); }

    void getStateInformation(MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;
    void fillInPluginDescription(PluginDescription& description) const override;

    // Text accessors
    const String& getText() const { return labelText; }
    void setText(const String& newText);

    void registerControl(LabelControl* ctrl);
    void unregisterControl(LabelControl* ctrl);

  private:
    String labelText;
    Component::SafePointer<LabelControl> activeControl;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LabelProcessor)
};
