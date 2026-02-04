//	OutputToggleEditors.cpp - Output Toggle control and editor implementations.
//	----------------------------------------------------------------------------
//	This file is part of Pedalboard3, an audio plugin host.
//	Copyright (c) 2011 Niall Moody.
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

#include "ColourScheme.h"
#include "JuceHelperStuff.h"
#include "PedalboardProcessorEditors.h"
#include "PedalboardProcessors.h"
#include "Vectors.h"


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
OutputToggleControl::OutputToggleControl(OutputToggleProcessor* proc) : processor(proc)
{
    std::unique_ptr<Drawable> im1(
        JuceHelperStuff::loadSVGFromMemory(Vectors::outputtoggle1_svg, Vectors::outputtoggle1_svgSize));
    std::unique_ptr<Drawable> im2(
        JuceHelperStuff::loadSVGFromMemory(Vectors::outputtoggle2_svg, Vectors::outputtoggle2_svgSize));

    toggleButton = new DrawableButton("toggleButton", DrawableButton::ImageFitted);
    toggleButton->setImages(im1.get(), 0, 0, 0, im2.get());
    toggleButton->setColour(DrawableButton::backgroundColourId, Colours::transparentBlack);
    toggleButton->setColour(DrawableButton::backgroundOnColourId, Colours::transparentBlack);
    toggleButton->setClickingTogglesState(true);
    toggleButton->setTopLeftPosition(0, 0);
    toggleButton->setSize(48, 48);
    toggleButton->addListener(this);
    addAndMakeVisible(toggleButton);

    startTimer(60);

    setSize(48, 48);
}

//------------------------------------------------------------------------------
OutputToggleControl::~OutputToggleControl()
{
    deleteAllChildren();
}

//------------------------------------------------------------------------------
void OutputToggleControl::timerCallback()
{
    toggleButton->setToggleState(processor->getParameter(0) > 0.5f, false);
}

//------------------------------------------------------------------------------
void OutputToggleControl::buttonClicked(Button* button)
{
    processor->setParameter(0, toggleButton->getToggleState() ? 1.0f : 0.0f);
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
OutputToggleEditor::OutputToggleEditor(AudioProcessor* processor, const Rectangle<int>& windowBounds)
    : AudioProcessorEditor(processor), parentBounds(windowBounds), setPos(false)
{
    std::unique_ptr<Drawable> im1(
        JuceHelperStuff::loadSVGFromMemory(Vectors::outputtoggle1_svg, Vectors::outputtoggle1_svgSize));
    std::unique_ptr<Drawable> im2(
        JuceHelperStuff::loadSVGFromMemory(Vectors::outputtoggle2_svg, Vectors::outputtoggle2_svgSize));

    toggleButton = new DrawableButton("toggleButton", DrawableButton::ImageFitted);
    toggleButton->setImages(im1.get(), 0, 0, 0, im2.get());
    toggleButton->setColour(DrawableButton::backgroundColourId, Colours::transparentBlack);
    toggleButton->setColour(DrawableButton::backgroundOnColourId, Colours::transparentBlack);
    toggleButton->setClickingTogglesState(true);
    toggleButton->setTopLeftPosition(0, 0);
    toggleButton->setSize(48, 48);
    toggleButton->addListener(this);
    addAndMakeVisible(toggleButton);

    startTimer(60);

    setSize(192, 192);
}

//------------------------------------------------------------------------------
OutputToggleEditor::~OutputToggleEditor()
{
    LevelProcessor* proc = dynamic_cast<LevelProcessor*>(getAudioProcessor());

    if (proc && getParentComponent())
    {
        parentBounds = getTopLevelComponent()->getBounds();
        proc->updateEditorBounds(parentBounds);
    }

    deleteAllChildren();
    getAudioProcessor()->editorBeingDeleted(this);
}

//------------------------------------------------------------------------------
void OutputToggleEditor::resized()
{
    // Resize the button.
    toggleButton->setSize(getWidth(), getHeight());
}

//------------------------------------------------------------------------------
void OutputToggleEditor::paint(Graphics& g)
{
    g.fillAll(ColourScheme::getInstance().colours["Window Background"]);
}

//------------------------------------------------------------------------------
void OutputToggleEditor::timerCallback()
{
    toggleButton->setToggleState(getAudioProcessor()->getParameter(0) > 0.5f, false);

    if (!setPos)
    {
        ComponentPeer* peer = getPeer();

        if (parentBounds.isEmpty())
            setPos = true;
        else if (peer)
        {
            // JUCE 8: setBounds takes Rectangle<int> instead of x,y,w,h
            peer->setBounds(parentBounds, false);
            setPos = true;
        }
    }
}

//------------------------------------------------------------------------------
void OutputToggleEditor::buttonClicked(Button* button)
{
    getAudioProcessor()->setParameter(0, toggleButton->getToggleState() ? 1.0f : 0.0f);
}
