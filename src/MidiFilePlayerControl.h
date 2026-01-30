//  MidiFilePlayerControl.h - UI control for MIDI file player.
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

#ifndef MIDIFILEPLAYERCONTROL_H_
#define MIDIFILEPLAYERCONTROL_H_

#include <JuceHeader.h>

class MidiFilePlayerProcessor;

//------------------------------------------------------------------------------
/// UI control for the MIDI file player processor.
class MidiFilePlayerControl : public Component,
                              public Button::Listener,
                              public Slider::Listener,
                              public FilenameComponentListener,
                              public ChangeListener,
                              public Timer
{
  public:
    /// Constructor.
    MidiFilePlayerControl(MidiFilePlayerProcessor* processor);
    /// Destructor.
    ~MidiFilePlayerControl();

    //--------------------------------------------------------------------------
    // Component overrides
    void paint(Graphics& g) override;
    void resized() override;

    //--------------------------------------------------------------------------
    // Listeners
    void buttonClicked(Button* button) override;
    void sliderValueChanged(Slider* slider) override;
    void filenameComponentChanged(FilenameComponent* fileChooser) override;
    void changeListenerCallback(ChangeBroadcaster* source) override;
    void timerCallback() override;

  private:
    /// The processor we're controlling.
    MidiFilePlayerProcessor* processor;

    /// File chooser component.
    std::unique_ptr<FilenameComponent> fileChooser;

    /// Transport buttons.
    std::unique_ptr<DrawableButton> playButton;
    std::unique_ptr<DrawableButton> stopButton;

    /// Loop toggle.
    std::unique_ptr<ToggleButton> loopButton;

    /// BPM slider.
    std::unique_ptr<Slider> bpmSlider;
    std::unique_ptr<Label> bpmLabel;

    /// Position slider (scrubber).
    std::unique_ptr<Slider> positionSlider;

    /// Track info label.
    std::unique_ptr<Label> trackInfoLabel;

    /// Current play state for button icon update.
    bool isPlaying = false;

    /// Drawable images for play/pause.
    std::unique_ptr<Drawable> playImage;
    std::unique_ptr<Drawable> pauseImage;
    std::unique_ptr<Drawable> stopImage;

    /// Update UI from processor state.
    void updateUI();

    /// Last directory used for file loading.
    static File lastDirectory;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiFilePlayerControl)
};

#endif // MIDIFILEPLAYERCONTROL_H_
