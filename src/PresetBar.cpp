/*
  ==============================================================================

  This is an automatically generated file created by the Jucer!

  Creation date:  26 Jan 2012 10:24:15am

  Be careful when adding custom code to these files, as only the code within
  the "//[xyz]" and "//[/xyz]" sections will be retained when the file is loaded
  and re-saved.

  Jucer version: 1.12

  ------------------------------------------------------------------------------

  The Jucer is part of the JUCE library - "Jules' Utility Class Extensions"
  Copyright 2004-6 by Raw Material Software ltd.

  ==============================================================================
*/

//[Headers] You can add your own extra header files here...

#include "ColourScheme.h"
#include "FontManager.h"
#include "JuceHelperStuff.h"
#include "PluginComponent.h"
#include "PresetManager.h"
#include "Vectors.h"

//[/Headers]

#include "PresetBar.h"

//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

//==============================================================================
PresetBar::PresetBar(PluginComponent* comp) : presetsComboBox(0), presetsLabel(0), importButton(0), saveButton(0)
{
    addAndMakeVisible(presetsComboBox = new ComboBox("presetsComboBox"));
    presetsComboBox->setEditableText(true);
    presetsComboBox->setJustificationType(Justification::centredLeft);
    presetsComboBox->setTextWhenNothingSelected(String());
    presetsComboBox->setTextWhenNoChoicesAvailable("(no choices)");
    presetsComboBox->addListener(this);

    addAndMakeVisible(presetsLabel = new Label("presetsLabel", "Presets:"));
    presetsLabel->setFont(FontManager::getInstance().getBodyFont());
    presetsLabel->setJustificationType(Justification::centredLeft);
    presetsLabel->setEditable(false, false, false);
    presetsLabel->setColour(TextEditor::backgroundColourId, Colour(0x0));

    addAndMakeVisible(importButton = new DrawableButton("importButton", DrawableButton::ImageOnButtonBackground));
    importButton->setName("importButton");

    addAndMakeVisible(saveButton = new DrawableButton("saveButton", DrawableButton::ImageOnButtonBackground));
    saveButton->setName("saveButton");

    //[UserPreSize]

    AudioProcessor* proc;

    Colour tempCol = ColourScheme::getInstance().colours["Button Colour"];
    std::unique_ptr<Drawable> saveImage(
        JuceHelperStuff::loadSVGFromMemory(Vectors::savebutton_svg, Vectors::savebutton_svgSize));
    std::unique_ptr<Drawable> openImage(
        JuceHelperStuff::loadSVGFromMemory(Vectors::openbutton_svg, Vectors::openbutton_svgSize));

    component = comp;
    proc = component->getNode()->getProcessor();

    saveButton->setImages(saveImage.get());
    saveButton->setColour(DrawableButton::backgroundColourId, tempCol);
    saveButton->setColour(DrawableButton::backgroundOnColourId, tempCol);
    saveButton->setTooltip("Save current preset");
    saveButton->addListener(this);
    importButton->setImages(openImage.get());
    importButton->setColour(DrawableButton::backgroundColourId, tempCol);
    importButton->setColour(DrawableButton::backgroundOnColourId, tempCol);
    importButton->setTooltip("Import preset from .fxp file");
    importButton->addListener(this);

    fillOutComboBox();
    presetsComboBox->setSelectedId(proc->getCurrentProgram() + 1, dontSendNotification);
    lastComboBox = presetsComboBox->getSelectedId();

    //[/UserPreSize]

    setSize(396, 32);

    //[Constructor] You can add your own custom stuff here..
    //[/Constructor]
}

PresetBar::~PresetBar()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    //[/Destructor_pre]

    delete presetsComboBox;
    presetsComboBox = nullptr;
    delete presetsLabel;
    presetsLabel = nullptr;
    delete importButton;
    importButton = nullptr;
    delete saveButton;
    saveButton = nullptr;

    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void PresetBar::paint(Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    //[/UserPrePaint]

    g.fillAll(Colour(0xffeeece1));

    g.setColour(Colour(0xff2aa545));
    g.fillPath(internalPath1);
    g.setColour(Colour(0x20000000));
    g.strokePath(internalPath1, PathStrokeType(1.0000f));

    //[UserPaint] Add your own custom painting code here..
    //[/UserPaint]
}

void PresetBar::resized()
{
    presetsComboBox->setBounds(64, 4, 272, 24);
    presetsLabel->setBounds(0, 4, 64, 24);
    importButton->setBounds(340, 4, 24, 24);
    saveButton->setBounds(368, 4, 24, 24);
    internalPath1.clear();
    internalPath1.startNewSubPath(0.0f, 32.0f);
    internalPath1.lineTo((float)(getWidth()), 32.0f);
    internalPath1.closeSubPath();

    //[UserResized] Add your own custom resize handling here..
    //[/UserResized]
}

void PresetBar::comboBoxChanged(ComboBox* comboBoxThatHasChanged)
{
    //[UsercomboBoxChanged_Pre]
    //[/UsercomboBoxChanged_Pre]

    if (comboBoxThatHasChanged == presetsComboBox)
    {
        //[UserComboBoxCode_presetsComboBox] -- add your combo box handling code
        // here..

        // Null safety: component may be invalidated before window closes
        if (!component || !component->getNode() || !component->getNode()->getProcessor())
            return;

        int index = presetsComboBox->getSelectedItemIndex();
        AudioProcessor* proc = component->getNode()->getProcessor();

        // Plugin presets.
        if ((index > -1) && (index < proc->getNumPrograms()))
        {
            MemoryBlock cachedPreset;

            component->getCachedPreset(index, cachedPreset);

            proc->setCurrentProgram(index);

            // If we have a cached preset for this program, load it.
            if (cachedPreset.getSize() > 0)
            {
                proc->setCurrentProgramStateInformation(cachedPreset.getData(), cachedPreset.getSize());
            }

            lastComboBox = index + 1;
        }
        // User-saved presets.
        else if (index >= proc->getNumPrograms())
        {
            PresetManager manager;

            // Only cache plugin presets, not user ones.
            if (component && (lastComboBox - 1) < proc->getNumPrograms())
                component->cacheCurrentPreset();

            manager.importPreset(presetsComboBox->getText(), proc);

            lastComboBox = index + 1;
        }
        // Changing a preset's name.
        else
        {
            proc->changeProgramName(proc->getCurrentProgram(), presetsComboBox->getText());
            presetsComboBox->changeItemText(lastComboBox, presetsComboBox->getText());
        }

        //[/UserComboBoxCode_presetsComboBox]
    }

    //[UsercomboBoxChanged_Post]
    //[/UsercomboBoxChanged_Post]
}

//[MiscUserCode] You can add your own definitions of your custom methods or any
// other code here...

//------------------------------------------------------------------------------
void PresetBar::buttonClicked(Button* button)
{
    if (button == importButton)
    {
        // Null safety: component may be invalidated before window closes
        if (!component || !component->getNode() || !component->getNode()->getProcessor())
            return;

        FileChooser dlg("Select an .fxp file to import...", File(), "*.fxp");

        if (dlg.browseForFileToOpen())
        {
            PresetManager manager;
            AudioProcessor* proc = component->getNode()->getProcessor();

            manager.importPreset(dlg.getResult(), proc);
            presetsComboBox->setText(proc->getProgramName(proc->getCurrentProgram()), true);
        }
    }
    else if (button == saveButton)
    {
        // Null safety: component may be invalidated before window closes
        if (!component || !component->getNode() || !component->getNode()->getProcessor())
            return;

        int currentId;
        MemoryBlock memBlock;
        PresetManager manager;
        AudioProcessor* proc = component->getNode()->getProcessor();

        proc->getCurrentProgramStateInformation(memBlock);
        manager.savePreset(memBlock, presetsComboBox->getText(), proc->getName());

        currentId = presetsComboBox->getSelectedId();
        fillOutComboBox();
        presetsComboBox->setSelectedId(currentId, true);
    }
}

//------------------------------------------------------------------------------
void PresetBar::fillOutComboBox()
{
    // Null safety: component may be invalidated before window closes
    if (!component || !component->getNode() || !component->getNode()->getProcessor())
        return;

    int i, j;
    StringArray userPresets;
    AudioProcessor* proc = component->getNode()->getProcessor();

    presetsComboBox->clear(true);

    // Add plugin presets to the combobox.
    j = 1;
    for (i = 0; i < proc->getNumPrograms(); ++i, ++j)
    {
        String tempstr = proc->getProgramName(i);
        if (tempstr == String())
            tempstr = " ";
        presetsComboBox->addItem(tempstr, j);
    }

    // Add user-saved presets to the combobox.
    PresetManager::getListOfUserPresets(proc->getName(), userPresets);

    for (i = 0; i < userPresets.size(); ++i, ++j)
        presetsComboBox->addItem(userPresets[i], j);
}

//[/MiscUserCode]

//==============================================================================
#if 0
/*  -- Jucer information section --

    This is where the Jucer puts all of its metadata, so don't change anything in here!

BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="PresetBar" componentName=""
                 parentClasses="public Component, public Button::Listener" constructorParams="PluginComponent *comp"
                 variableInitialisers="" snapPixels="8" snapActive="1" snapShown="1"
                 overlayOpacity="0.330000013" fixedSize="0" initialWidth="396"
                 initialHeight="32">
  <BACKGROUND backgroundColour="ffeeece1">
    <PATH pos="0 0 100 100" fill="solid: ff2aa545" hasStroke="1" stroke="1, mitered, butt"
          strokeColour="solid: 20000000" nonZeroWinding="1">s 0 32 l 0R 32 x</PATH>
  </BACKGROUND>
  <COMBOBOX name="presetsComboBox" id="c6c86dda7c641998" memberName="presetsComboBox"
            virtualName="" explicitFocusOrder="0" pos="64 4 272 24" editable="1"
            layout="33" items="" textWhenNonSelected="" textWhenNoItems="(no choices)"/>
  <LABEL name="presetsLabe" id="1313b0aafdbc5e32" memberName="presetsLabe"
         virtualName="" explicitFocusOrder="0" pos="0 4 64 24" edTextCol="ff000000"
         edBkgCol="0" labelText="Presets:" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="0" fontname="Default font" fontsize="15"
         bold="0" italic="0" justification="33"/>
  <GENERICCOMPONENT name="importButton" id="22f5888739c5b78b" memberName="importButton"
                    virtualName="" explicitFocusOrder="0" pos="340 4 24 24" class="DrawableButton"
                    params="L&quot;importButton&quot;, DrawableButton::ImageOnButtonBackground"/>
  <GENERICCOMPONENT name="saveButton" id="480bd37fd9ab42d3" memberName="saveButton"
                    virtualName="" explicitFocusOrder="0" pos="368 4 24 24" class="DrawableButton"
                    params="L&quot;saveButton&quot;, DrawableButton::ImageOnButtonBackground"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif
