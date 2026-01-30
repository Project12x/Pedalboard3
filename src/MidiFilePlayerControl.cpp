//  MidiFilePlayerControl.cpp - UI control implementation for MIDI file player.
//  ----------------------------------------------------------------------------
//  This file is part of Pedalboard3, an audio plugin host.
//  Copyright (c) 2024-2026 Antigravity.
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//  ----------------------------------------------------------------------------

#include "MidiFilePlayerControl.h"

#include "MidiFilePlayer.h"


File MidiFilePlayerControl::lastDirectory = File::getSpecialLocation(File::userHomeDirectory);

//------------------------------------------------------------------------------
MidiFilePlayerControl::MidiFilePlayerControl(MidiFilePlayerProcessor* proc) : processor(proc)
{
    // File chooser
    fileChooser = std::make_unique<FilenameComponent>("midiFile", processor->getFile(), false, false, false,
                                                      "*.mid;*.midi", String(), "Select a MIDI file...");
    fileChooser->addListener(this);
    fileChooser->setBrowseButtonText("...");
    addAndMakeVisible(fileChooser.get());

    // Play button (SVG path for play triangle)
    playImage = Drawable::createFromSVG(
        *XmlDocument::parse("<svg viewBox='0 0 24 24'><polygon points='6,4 20,12 6,20' fill='currentColor'/></svg>"));
    pauseImage = Drawable::createFromSVG(
        *XmlDocument::parse("<svg viewBox='0 0 24 24'><rect x='5' y='4' width='4' height='16' fill='currentColor'/>"
                            "<rect x='15' y='4' width='4' height='16' fill='currentColor'/></svg>"));
    stopImage = Drawable::createFromSVG(*XmlDocument::parse(
        "<svg viewBox='0 0 24 24'><rect x='5' y='5' width='14' height='14' fill='currentColor'/></svg>"));

    playButton = std::make_unique<DrawableButton>("play", DrawableButton::ImageFitted);
    playButton->setImages(playImage.get());
    playButton->addListener(this);
    playButton->setTooltip("Play/Pause");
    addAndMakeVisible(playButton.get());

    stopButton = std::make_unique<DrawableButton>("stop", DrawableButton::ImageFitted);
    stopButton->setImages(stopImage.get());
    stopButton->addListener(this);
    stopButton->setTooltip("Stop and rewind");
    addAndMakeVisible(stopButton.get());

    // Loop toggle
    loopButton = std::make_unique<ToggleButton>("Loop");
    loopButton->setToggleState(processor->isLooping(), dontSendNotification);
    loopButton->addListener(this);
    addAndMakeVisible(loopButton.get());

    // BPM slider
    bpmSlider = std::make_unique<Slider>(Slider::LinearHorizontal, Slider::TextBoxRight);
    bpmSlider->setRange(20.0, 300.0, 0.1);
    bpmSlider->setValue(processor->getBPM(), dontSendNotification);
    bpmSlider->addListener(this);
    bpmSlider->setTextBoxStyle(Slider::TextBoxRight, false, 50, 20);
    addAndMakeVisible(bpmSlider.get());

    bpmLabel = std::make_unique<Label>("bpmLabel", "BPM:");
    bpmLabel->setJustificationType(Justification::centredRight);
    addAndMakeVisible(bpmLabel.get());

    // Position slider
    positionSlider = std::make_unique<Slider>(Slider::LinearHorizontal, Slider::NoTextBox);
    positionSlider->setRange(0.0, 1.0, 0.001);
    positionSlider->setValue(0.0, dontSendNotification);
    positionSlider->addListener(this);
    positionSlider->setTooltip("Playback position");
    addAndMakeVisible(positionSlider.get());

    // Track info label
    trackInfoLabel = std::make_unique<Label>("trackInfo", "No file loaded");
    trackInfoLabel->setJustificationType(Justification::centredLeft);
    trackInfoLabel->setColour(Label::textColourId, Colours::grey);
    addAndMakeVisible(trackInfoLabel.get());

    // Listen for processor changes
    processor->addChangeListener(this);

    // Start timer for position updates
    startTimerHz(30);

    // Initial UI update
    updateUI();
}

//------------------------------------------------------------------------------
MidiFilePlayerControl::~MidiFilePlayerControl()
{
    processor->removeChangeListener(this);
    stopTimer();
}

//------------------------------------------------------------------------------
void MidiFilePlayerControl::paint(Graphics& g)
{
    // Background
    g.setColour(findColour(ResizableWindow::backgroundColourId).darker(0.1f));
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 4.0f);

    // Border
    g.setColour(Colours::grey.withAlpha(0.3f));
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 4.0f, 1.0f);
}

//------------------------------------------------------------------------------
void MidiFilePlayerControl::resized()
{
    auto bounds = getLocalBounds().reduced(8);

    // Row 1: File chooser
    auto row1 = bounds.removeFromTop(24);
    fileChooser->setBounds(row1);

    bounds.removeFromTop(6);

    // Row 2: Transport buttons + loop + BPM
    auto row2 = bounds.removeFromTop(28);
    playButton->setBounds(row2.removeFromLeft(28));
    row2.removeFromLeft(4);
    stopButton->setBounds(row2.removeFromLeft(28));
    row2.removeFromLeft(8);
    loopButton->setBounds(row2.removeFromLeft(60));
    row2.removeFromLeft(8);

    bpmLabel->setBounds(row2.removeFromLeft(35));
    bpmSlider->setBounds(row2);

    bounds.removeFromTop(6);

    // Row 3: Position slider
    auto row3 = bounds.removeFromTop(20);
    positionSlider->setBounds(row3);

    bounds.removeFromTop(4);

    // Row 4: Track info
    trackInfoLabel->setBounds(bounds.removeFromTop(20));
}

//------------------------------------------------------------------------------
void MidiFilePlayerControl::buttonClicked(Button* button)
{
    if (button == playButton.get())
    {
        if (processor->isPlaying())
            processor->pause();
        else
            processor->play();
    }
    else if (button == stopButton.get())
    {
        processor->stop();
    }
    else if (button == loopButton.get())
    {
        processor->setLooping(loopButton->getToggleState());
    }

    updateUI();
}

//------------------------------------------------------------------------------
void MidiFilePlayerControl::sliderValueChanged(Slider* slider)
{
    if (slider == bpmSlider.get())
    {
        processor->setBPM(bpmSlider->getValue());
    }
    else if (slider == positionSlider.get())
    {
        // Only seek if user is actively dragging (not from timer updates)
        if (slider->isMouseButtonDown())
        {
            processor->seekToPosition(positionSlider->getValue());
        }
    }
}

//------------------------------------------------------------------------------
void MidiFilePlayerControl::filenameComponentChanged(FilenameComponent* fileChooser)
{
    File file = fileChooser->getCurrentFile();
    if (file.existsAsFile())
    {
        lastDirectory = file.getParentDirectory();
        processor->setFile(file);
        updateUI();
    }
}

//------------------------------------------------------------------------------
void MidiFilePlayerControl::changeListenerCallback(ChangeBroadcaster* source)
{
    updateUI();
}

//------------------------------------------------------------------------------
void MidiFilePlayerControl::timerCallback()
{
    // Update position slider (only if not being dragged)
    if (!positionSlider->isMouseButtonDown())
    {
        double pos = processor->getPlaybackPosition();
        positionSlider->setValue(pos, dontSendNotification);
    }

    // Update play button icon if state changed
    bool currentlyPlaying = processor->isPlaying();
    if (currentlyPlaying != isPlaying)
    {
        isPlaying = currentlyPlaying;
        if (isPlaying)
            playButton->setImages(pauseImage.get());
        else
            playButton->setImages(playImage.get());
    }
}

//------------------------------------------------------------------------------
void MidiFilePlayerControl::updateUI()
{
    // Update loop button
    loopButton->setToggleState(processor->isLooping(), dontSendNotification);

    // Update BPM slider
    bpmSlider->setValue(processor->getBPM(), dontSendNotification);

    // Update file chooser
    if (processor->getFile().existsAsFile())
        fileChooser->setCurrentFile(processor->getFile(), false, dontSendNotification);

    // Update track info
    if (processor->getFile().existsAsFile())
    {
        String info = processor->getFile().getFileNameWithoutExtension();
        info += " - " + String(processor->getNumTracks()) + " tracks";
        info += ", " + String(processor->getLengthInSeconds(), 1) + "s";
        trackInfoLabel->setText(info, dontSendNotification);
        trackInfoLabel->setColour(Label::textColourId, Colours::white.withAlpha(0.8f));
    }
    else
    {
        trackInfoLabel->setText("No file loaded", dontSendNotification);
        trackInfoLabel->setColour(Label::textColourId, Colours::grey);
    }

    // Update play button icon
    isPlaying = processor->isPlaying();
    if (isPlaying)
        playButton->setImages(pauseImage.get());
    else
        playButton->setImages(playImage.get());
}
