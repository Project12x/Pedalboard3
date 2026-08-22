#pragma once

#include <JuceHeader.h>

class MainPanel;

class LinkAudioSettingsDialog : public juce::Component,
                                private juce::Button::Listener,
                                private juce::TextEditor::Listener,
                                private juce::Timer
{
  public:
    explicit LinkAudioSettingsDialog(MainPanel* panel);
    ~LinkAudioSettingsDialog() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

  private:
    void buttonClicked(juce::Button* button) override;
    void textEditorReturnKeyPressed(juce::TextEditor& editor) override;
    void textEditorFocusLost(juce::TextEditor& editor) override;
    void timerCallback() override;
    void applyPeerName();
    void refreshStatus();

    MainPanel* mainPanel;
    juce::Label linkSectionLabel;
    juce::Label linkDescriptionLabel;
    juce::ToggleButton enableAudioButton;
    juce::Label peerNameLabel;
    juce::TextEditor peerNameEditor;
    juce::Label publishedChannelLabel;
    juce::Label publishedChannelValue;
    juce::Label limitationsLabel;
    juce::Label peersSectionLabel;
    juce::Label peersHeaderLabel;
    juce::TextEditor peersList;
    juce::Label peerStatusLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LinkAudioSettingsDialog)
};
