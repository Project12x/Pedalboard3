/*
  ==============================================================================

  This is an automatically generated file created by the Jucer!

  Creation date:  20 Oct 2012 7:09:35pm

  Be careful when adding custom code to these files, as only the code within
  the "//[xyz]" and "//[/xyz]" sections will be retained when the file is loaded
  and re-saved.

  Jucer version: 1.12

  ------------------------------------------------------------------------------

  The Jucer is part of the JUCE library - "Jules' Utility Class Extensions"
  Copyright 2004-6 by Raw Material Software ltd.

  ==============================================================================
*/

#ifndef __JUCER_HEADER_LOOPEREDITOR_LOOPEREDITOR_89E8DC78__
#define __JUCER_HEADER_LOOPEREDITOR_LOOPEREDITOR_89E8DC78__

//[Headers]     -- You can add your own extra header files here --
#include "WaveformDisplay.h"

#include <JuceHeader.h>


class LooperProcessor;
//[/Headers]

//==============================================================================
/**
                                                                    //[Comments]
    An auto-generated component, created by the Jucer.

    Describe your class and how it works here!
                                                                    //[/Comments]
*/
class LooperEditor : public AudioProcessorEditor,
                     public FilenameComponentListener,
                     public Timer,
                     public ChangeListener,
                     public Button::Listener,
                     public Label::Listener,
                     public Slider::Listener
{
  public:
    //==============================================================================
    LooperEditor(LooperProcessor* proc, AudioThumbnail* thumbnail);
    ~LooperEditor();

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.

    ///	Used to load a sound file.
    void filenameComponentChanged(FilenameComponent* filenameComp);
    ///	Used to update the read position graphic.
    void timerCallback();
    ///	Used to change the player's read position when the user clicks on the WaveformDisplay.
    void changeListenerCallback(ChangeBroadcaster* source);

    ///	Changes the colour of the WaveformDisplay background.
    void setWaveformBackground(const Colour& col);
    ///	Clears the waveform display.
    void clearDisplay();

    //[/UserMethods]

    void paint(Graphics& g);
    void resized();
    void buttonClicked(Button* buttonThatWasClicked);
    void labelTextChanged(Label* labelThatHasChanged);
    void sliderValueChanged(Slider* sliderThatWasMoved);

    //==============================================================================


      private :
      //[UserVariables]   -- You can add your own custom variables in this section.

      ///	Our copy of the AudioProcessor pointer.
      LooperProcessor* processor;

    ///	The two drawables we use for the playButton.
    std::unique_ptr<Drawable> playImage;
    std::unique_ptr<Drawable> pauseImage;
    ///	Whether the playPauseButton is currently displaying the play icon.
    bool playing;

    ///	The two drawables we use for the recordButton.
    std::unique_ptr<Drawable> recordImage;
    std::unique_ptr<Drawable> stopImage;
    ///	Whether the recordButton is currently displaying the record icon.
    bool recording;

    //[/UserVariables]

    //==============================================================================
    WaveformDisplay* fileDisplay;
    FilenameComponent* filename;
    ToggleButton* syncButton;
    ToggleButton* stopAfterBarButton;
    DrawableButton* playPauseButton;
    DrawableButton* rtzButton;
    DrawableButton* recordButton;
    ToggleButton* autoPlayButton;
    Label* barLengthLabel;
    Label* separatorLabel;
    Label* numeratorLabel;
    Label* denominatorLabel;
    Label* loopLevelLabel;
    Slider* loopLevelSlider;
    Label* inputLevelLabel;
    Slider* inputLevelSlider;

    //==============================================================================
    // (prevent copy constructor and operator= being generated..)
    LooperEditor(const LooperEditor&);
    const LooperEditor& operator=(const LooperEditor&);
};

#endif // __JUCER_HEADER_LOOPEREDITOR_LOOPEREDITOR_89E8DC78__
