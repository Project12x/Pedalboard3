#include "ScratchPanel.h"

#include "ColourScheme.h"
#include "FontManager.h"
#include "MainPanel.h"

ScratchPanel::ScratchPanel(MainPanel& ownerToUse) : owner(&ownerToUse)
{
    addAndMakeVisible(recordButton);
    recordButton.setButtonText("REC");
    recordButton.addListener(this);

    addAndMakeVisible(revealButton);
    revealButton.setButtonText("Reveal");
    revealButton.addListener(this);

    for (auto* label : {&statusLabel, &elapsedLabel})
    {
        addAndMakeVisible(label);
        label->setFont(FontManager::getInstance().getLabelFont());
        label->setJustificationType(juce::Justification::centredLeft);
    }

    addAndMakeVisible(recentTakesBox);
    recentTakesBox.setMultiLine(true);
    recentTakesBox.setReadOnly(true);
    recentTakesBox.setScrollbarsShown(true);
    recentTakesBox.setText("No scratch takes yet", juce::dontSendNotification);

    startTimerHz(10);
    refresh();
}

ScratchPanel::~ScratchPanel()
{
    stopTimer();
    recordButton.removeListener(this);
    revealButton.removeListener(this);
}

void ScratchPanel::paint(juce::Graphics& g)
{
    g.fillAll(ColourScheme::getInstance().colours["Window Background"]);
}

void ScratchPanel::resized()
{
    auto bounds = getLocalBounds().reduced(12);
    auto top = bounds.removeFromTop(48);
    recordButton.setBounds(top.removeFromLeft(96));
    top.removeFromLeft(8);
    statusLabel.setBounds(top.removeFromLeft(160));
    revealButton.setBounds(top.removeFromRight(96));

    bounds.removeFromTop(10);
    elapsedLabel.setBounds(bounds.removeFromTop(28));
    recentTakesBox.setBounds(bounds);
}

void ScratchPanel::buttonClicked(juce::Button* button)
{
    if (button == &recordButton)
    {
        if (auto* mainPanel = owner.getComponent())
            mainPanel->toggleScratchCapture();
    }
    else if (button == &revealButton)
    {
        if (auto* mainPanel = owner.getComponent())
            mainPanel->revealScratchFolder();
    }

    refresh();
}

void ScratchPanel::timerCallback()
{
    refresh();
}

void ScratchPanel::refresh()
{
    auto* mainPanel = owner.getComponent();
    if (mainPanel == nullptr)
    {
        stopTimer();
        recordButton.setEnabled(false);
        revealButton.setEnabled(false);
        statusLabel.setText("Closed", juce::dontSendNotification);
        return;
    }

    const auto status = mainPanel->getScratchRecorderStatus();
    const bool isRecording = status.state == ScratchRecorderState::Recording;
    recordButton.setButtonText(isRecording ? "STOP" : "REC");
    recordButton.setEnabled(status.state != ScratchRecorderState::Saving);
    statusLabel.setText(status.message, juce::dontSendNotification);

    int seconds = 0;
    if (status.lastTake.has_value() && status.lastTake->sampleRate > 0.0)
        seconds = static_cast<int>(status.elapsedSamples / status.lastTake->sampleRate);
    elapsedLabel.setText(juce::String::formatted("%02d:%02d", seconds / 60, seconds % 60),
                         juce::dontSendNotification);

    if (!status.recentTakes.empty())
    {
        juce::String listText;
        for (const auto& take : status.recentTakes)
        {
            listText << take.startTime.formatted("%H:%M:%S") << "  "
                     << (take.patchName.isNotEmpty() ? take.patchName : "<untitled>") << "  raw/wet "
                     << (take.complete ? "saved" : "incomplete") << juce::newLine;
        }
        recentTakesBox.setText(listText.trimEnd(), juce::dontSendNotification);
    }
    else
    {
        recentTakesBox.setText("No scratch takes yet", juce::dontSendNotification);
    }
}
