#pragma once

#include <JuceHeader.h>

class MainPanel;

class ScratchPanel final : public juce::Component,
                           private juce::Button::Listener,
                           private juce::Timer
{
public:
    explicit ScratchPanel(MainPanel& ownerToUse);
    ~ScratchPanel() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void buttonClicked(juce::Button* button) override;
    void timerCallback() override;
    void refresh();

    juce::Component::SafePointer<MainPanel> owner;
    juce::TextButton recordButton{"recordButton"};
    juce::TextButton revealButton{"revealButton"};
    juce::Label statusLabel{"statusLabel", "Ready"};
    juce::Label elapsedLabel{"elapsedLabel", "00:00"};
    juce::TextEditor recentTakesBox{"recentTakesBox"};
};
