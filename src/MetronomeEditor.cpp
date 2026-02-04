//	MetronomeEditor.cpp - Metronome editor implementation.
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
#include "MetronomeControl.h"
#include "PedalboardProcessorEditors.h"
#include "PedalboardProcessors.h"


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
MetronomeEditor::MetronomeEditor(MetronomeProcessor* processor, const Rectangle<int>& windowBounds)
    : AudioProcessorEditor(processor), parentBounds(windowBounds), setPos(false)
{
    controls = new MetronomeControl(processor, true);
    controls->setTopLeftPosition(4, 4);
    controls->setSize(getWidth() - 8, getHeight() - 8);
    addAndMakeVisible(controls);

    setSize(400, 200);

    startTimer(60);
}

//------------------------------------------------------------------------------
MetronomeEditor::~MetronomeEditor()
{
    MetronomeProcessor* proc = dynamic_cast<MetronomeProcessor*>(getAudioProcessor());

    if (proc && getParentComponent())
    {
        parentBounds = getTopLevelComponent()->getBounds();
        proc->updateEditorBounds(parentBounds);
    }

    deleteAllChildren();
    getAudioProcessor()->editorBeingDeleted(this);
}

//------------------------------------------------------------------------------
void MetronomeEditor::resized()
{
    // Resize the controls.
    controls->setSize(getWidth() - 8, getHeight() - 8);
}

//------------------------------------------------------------------------------
void MetronomeEditor::paint(Graphics& g)
{
    g.fillAll(ColourScheme::getInstance().colours["Window Background"]);
}

//------------------------------------------------------------------------------
void MetronomeEditor::timerCallback()
{
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
            stopTimer();
        }
    }
}
