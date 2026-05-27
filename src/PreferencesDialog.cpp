/*
  ==============================================================================

  This is an automatically generated file created by the Jucer!

  Creation date:  19 Oct 2012 11:10:47am

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

#include "App.h"
#include "ColourScheme.h"
#include "FontManager.h"
#include "MainPanel.h"
#include "SettingsManager.h"
#include "Tone3000DownloadManager.h"
#include "UiScale.h"

//[/Headers]

#include "PreferencesDialog.h"

//[MiscUserDefs] You can add your own user definitions and misc code here...
namespace
{
constexpr int preferencesContentMinWidth = 560;
constexpr int preferencesContentMinHeight = 704;
constexpr int ioOptionsPanelY = 204;
constexpr int ioOptionsPanelHeight = 112;

class PreferencesContentComponent : public Component
{
  public:
    void paint(Graphics& g) override
    {
        g.fillAll(ColourScheme::getInstance().colours["Window Background"]);

        g.setColour(ColourScheme::getInstance().colours["Dialog Inner Background"]);
        g.fillRect(12, ioOptionsPanelY, getWidth() - 24, ioOptionsPanelHeight);

        g.setColour(ColourScheme::getInstance().colours["Plugin Border"].withAlpha(0.25f));
        g.drawRect(12, ioOptionsPanelY, getWidth() - 24, ioOptionsPanelHeight, 1);
    }
};
} // namespace
//[/MiscUserDefs]

//==============================================================================
PreferencesDialog::PreferencesDialog(MainPanel* panel, const String& port, const String& multicastAddress)
    : viewport(0), contentComponent(0), oscPortLabel(0), oscPortEditor(0), oscLabel(0), oscMulticastLabel(0),
      oscMulticastEditor(0), multicastHintLabel(0), ioOptionsLabel(0), audioInputButton(0), midiInputButton(0),
      oscInputButton(0), virtualMidiInputButton(0), otherLabel(0), mappingsWindowButton(0), loopPatchesButton(0),
      windowsOnTopButton(0), ignorePinNamesButton(0), midiLabel(0), midiProgramChangeButton(0),
      mmcTransportButton(0), useTrayIconButton(0), startInTrayButton(0), fixedSizeButton(0),
      pdlAudioSettingsButton(0), interfaceLabel(0), uiScaleLabel(0), uiScaleComboBox(0), namLabel(0), namDirLabel(0),
      namDirValue(0), namDirBrowseButton(0)
{
    addAndMakeVisible(viewport = new Viewport("preferencesViewport"));
    contentComponent = new PreferencesContentComponent();
    viewport->setViewedComponent(contentComponent, true);
    viewport->setScrollBarsShown(true, false);

    contentComponent->addAndMakeVisible(oscPortLabel = new Label("oscPortLabel", "OSC Port:"));
    oscPortLabel->setFont(FontManager::getInstance().getBodyFont());
    oscPortLabel->setJustificationType(Justification::centredLeft);
    oscPortLabel->setEditable(false, false, false);
    oscPortLabel->setColour(TextEditor::backgroundColourId, Colour(0x0));

    contentComponent->addAndMakeVisible(oscPortEditor = new TextEditor("oscPortEditor"));
    oscPortEditor->setMultiLine(false);
    oscPortEditor->setReturnKeyStartsNewLine(false);
    oscPortEditor->setReadOnly(false);
    oscPortEditor->setScrollbarsShown(true);
    oscPortEditor->setCaretVisible(true);
    oscPortEditor->setPopupMenuEnabled(true);
    oscPortEditor->setText("5678");

    contentComponent->addAndMakeVisible(oscLabel = new Label("oscLabel", "Open Sound Control Options"));
    oscLabel->setFont(FontManager::getInstance().getSubheadingFont());
    oscLabel->setJustificationType(Justification::centredLeft);
    oscLabel->setEditable(false, false, false);
    oscLabel->setColour(TextEditor::backgroundColourId, Colour(0x0));

    contentComponent->addAndMakeVisible(oscMulticastLabel = new Label("oscMulticastLabel", "OSC Multicast Address:"));
    oscMulticastLabel->setFont(FontManager::getInstance().getBodyFont());
    oscMulticastLabel->setJustificationType(Justification::centredLeft);
    oscMulticastLabel->setEditable(false, false, false);
    oscMulticastLabel->setColour(TextEditor::backgroundColourId, Colour(0x0));

    contentComponent->addAndMakeVisible(oscMulticastEditor = new TextEditor("oscMulticastEditor"));
    oscMulticastEditor->setMultiLine(false);
    oscMulticastEditor->setReturnKeyStartsNewLine(false);
    oscMulticastEditor->setReadOnly(false);
    oscMulticastEditor->setScrollbarsShown(true);
    oscMulticastEditor->setCaretVisible(true);
    oscMulticastEditor->setPopupMenuEnabled(true);
    oscMulticastEditor->setText(String());

    contentComponent->addAndMakeVisible(multicastHintLabel = new Label("multicastHintLabel", "optional"));
    multicastHintLabel->setFont(FontManager::getInstance().getBodyFont());
    multicastHintLabel->setJustificationType(Justification::centredLeft);
    multicastHintLabel->setEditable(false, false, false);
    multicastHintLabel->setColour(Label::textColourId,
                                  ColourScheme::getInstance().colours["Text Colour"].withAlpha(0.5f));
    multicastHintLabel->setColour(TextEditor::backgroundColourId, Colour(0x0));

    contentComponent->addAndMakeVisible(ioOptionsLabel = new Label("ioOptionsLabel", "Visible I/O Nodes"));
    ioOptionsLabel->setFont(FontManager::getInstance().getSubheadingFont());
    ioOptionsLabel->setJustificationType(Justification::centredLeft);
    ioOptionsLabel->setEditable(false, false, false);
    ioOptionsLabel->setColour(TextEditor::backgroundColourId, Colour(0x0));

    contentComponent->addAndMakeVisible(audioInputButton = new ToggleButton("audioInputButton"));
    audioInputButton->setButtonText("Audio Input");
    audioInputButton->addListener(this);
    audioInputButton->setToggleState(true, false);

    contentComponent->addAndMakeVisible(midiInputButton = new ToggleButton("midiInputButton"));
    midiInputButton->setButtonText("Midi Input");
    midiInputButton->addListener(this);
    midiInputButton->setToggleState(true, false);

    contentComponent->addAndMakeVisible(oscInputButton = new ToggleButton("oscInputButton"));
    oscInputButton->setButtonText("OSC Input");
    oscInputButton->addListener(this);
    oscInputButton->setToggleState(true, false);

    contentComponent->addAndMakeVisible(virtualMidiInputButton = new ToggleButton("virtualMidiInputButton"));
    virtualMidiInputButton->setButtonText("Virtual MIDI Input");
    virtualMidiInputButton->addListener(this);
    virtualMidiInputButton->setToggleState(true, false);

    contentComponent->addAndMakeVisible(otherLabel = new Label("otherLabel", "Other Options"));
    otherLabel->setFont(FontManager::getInstance().getSubheadingFont());
    otherLabel->setJustificationType(Justification::centredLeft);
    otherLabel->setEditable(false, false, false);
    otherLabel->setColour(TextEditor::backgroundColourId, Colour(0x0));

    contentComponent->addAndMakeVisible(mappingsWindowButton = new ToggleButton("mappingsWindowButton"));
    mappingsWindowButton->setButtonText("Open mappings window on successful param connection");
    mappingsWindowButton->addListener(this);
    mappingsWindowButton->setToggleState(true, false);

    contentComponent->addAndMakeVisible(loopPatchesButton = new ToggleButton("loopPatchesButton"));
    loopPatchesButton->setButtonText("Loop next/prev patch action");
    loopPatchesButton->addListener(this);
    loopPatchesButton->setToggleState(true, false);

    contentComponent->addAndMakeVisible(windowsOnTopButton = new ToggleButton("windowsOnTopButton"));
    windowsOnTopButton->setButtonText("Set plugin windows Always On Top");
    windowsOnTopButton->addListener(this);

    contentComponent->addAndMakeVisible(ignorePinNamesButton = new ToggleButton("ignorePinNamesButton"));
    ignorePinNamesButton->setButtonText("Ignore plugin pin names");
    ignorePinNamesButton->addListener(this);

    contentComponent->addAndMakeVisible(midiLabel = new Label("midiLabel", "Midi Options"));
    midiLabel->setFont(FontManager::getInstance().getSubheadingFont());
    midiLabel->setJustificationType(Justification::centredLeft);
    midiLabel->setEditable(false, false, false);
    midiLabel->setColour(TextEditor::backgroundColourId, Colour(0x0));

    contentComponent->addAndMakeVisible(midiProgramChangeButton = new ToggleButton("midiProgramChangeButton"));
    midiProgramChangeButton->setButtonText("Program Change messages switch patches");
    midiProgramChangeButton->addListener(this);

    contentComponent->addAndMakeVisible(mmcTransportButton = new ToggleButton("mmcTransportButton"));
    mmcTransportButton->setButtonText("Main transport responds to MMC");
    mmcTransportButton->addListener(this);

    contentComponent->addAndMakeVisible(useTrayIconButton = new ToggleButton("useTrayIconButton"));
    useTrayIconButton->setButtonText("Display tray icon (not OSX)");
    useTrayIconButton->addListener(this);

    contentComponent->addAndMakeVisible(startInTrayButton = new ToggleButton("startInTrayButton"));
    startInTrayButton->setButtonText("Start in tray (not OSX)");
    startInTrayButton->addListener(this);

    contentComponent->addAndMakeVisible(fixedSizeButton = new ToggleButton("fixedSizeButton"));
    fixedSizeButton->setButtonText("Force fixed-size plugin windows");
    fixedSizeButton->addListener(this);
    fixedSizeButton->setToggleState(true, false);

    contentComponent->addAndMakeVisible(pdlAudioSettingsButton = new ToggleButton("pdlAudioSettingsButton"));
    pdlAudioSettingsButton->setButtonText("Save audio settings in .pdl files");
    pdlAudioSettingsButton->addListener(this);

    contentComponent->addAndMakeVisible(interfaceLabel = new Label("interfaceLabel", "Interface"));
    interfaceLabel->setFont(FontManager::getInstance().getSubheadingFont());
    interfaceLabel->setJustificationType(Justification::centredLeft);
    interfaceLabel->setEditable(false, false, false);
    interfaceLabel->setColour(TextEditor::backgroundColourId, Colour(0x0));

    contentComponent->addAndMakeVisible(uiScaleLabel = new Label("uiScaleLabel", "UI Scale:"));
    uiScaleLabel->setFont(FontManager::getInstance().getBodyFont());
    uiScaleLabel->setJustificationType(Justification::centredLeft);
    uiScaleLabel->setEditable(false, false, false);
    uiScaleLabel->setColour(TextEditor::backgroundColourId, Colour(0x0));

    contentComponent->addAndMakeVisible(uiScaleComboBox = new ComboBox("uiScaleComboBox"));
    uiScaleComboBox->setJustificationType(Justification::centredLeft);
    uiScaleComboBox->setTextWhenNothingSelected("100%");
    uiScaleComboBox->setTextWhenNoChoicesAvailable("(no scale choices)");
    for (auto percent : UiScale::supportedPercents())
        uiScaleComboBox->addItem(String(percent) + "%", percent);
    uiScaleComboBox->addListener(this);

    // NAM Options
    contentComponent->addAndMakeVisible(namLabel = new Label("namLabel", "NAM Options"));
    namLabel->setFont(FontManager::getInstance().getSubheadingFont());
    namLabel->setJustificationType(Justification::centredLeft);
    namLabel->setEditable(false, false, false);

    contentComponent->addAndMakeVisible(namDirLabel = new Label("namDirLabel", "Download Directory:"));
    namDirLabel->setFont(FontManager::getInstance().getBodyFont());
    namDirLabel->setJustificationType(Justification::centredLeft);
    namDirLabel->setEditable(false, false, false);

    contentComponent->addAndMakeVisible(namDirValue = new Label("namDirValue", ""));
    namDirValue->setFont(FontManager::getInstance().getBodyFont());
    namDirValue->setJustificationType(Justification::centredLeft);
    namDirValue->setEditable(false, false, false);
    namDirValue->setColour(Label::backgroundColourId, ColourScheme::getInstance().colours["Text Editor Colour"]);
    namDirValue->setColour(Label::outlineColourId,
                           ColourScheme::getInstance().colours["Plugin Border"].withAlpha(0.25f));

    contentComponent->addAndMakeVisible(namDirBrowseButton = new TextButton("namDirBrowseButton"));
    namDirBrowseButton->setButtonText("Browse...");
    namDirBrowseButton->addListener(this);

    //[UserPreSize]

    bool useTrayIcon;

    mainPanel = panel;
    currentPort = port;
    currentMulticast = multicastAddress;

    oscPortEditor->setText(currentPort);
    oscMulticastEditor->setText(currentMulticast);

    oscPortEditor->addListener(this);
    oscMulticastEditor->addListener(this);

    audioInputButton->setToggleState(SettingsManager::getInstance().getBool("AudioInput", true), false);
    midiInputButton->setToggleState(SettingsManager::getInstance().getBool("MidiInput", true), false);
    oscInputButton->setToggleState(SettingsManager::getInstance().getBool("OscInput", true), false);
    virtualMidiInputButton->setToggleState(SettingsManager::getInstance().getBool("VirtualMidiInput", true), false);

    midiProgramChangeButton->setToggleState(SettingsManager::getInstance().getBool("midiProgramChange", false), false);
    mmcTransportButton->setToggleState(SettingsManager::getInstance().getBool("mmcTransport", false), false);

    mappingsWindowButton->setToggleState(SettingsManager::getInstance().getBool("AutoMappingsWindow", true), false);
    loopPatchesButton->setToggleState(SettingsManager::getInstance().getBool("LoopPatches", true), false);
    windowsOnTopButton->setToggleState(SettingsManager::getInstance().getBool("WindowsOnTop", false), false);
    ignorePinNamesButton->setToggleState(SettingsManager::getInstance().getBool("IgnorePinNames", false), false);

    fixedSizeButton->setToggleState(SettingsManager::getInstance().getBool("fixedSizeWindows", true), false);

    pdlAudioSettingsButton->setToggleState(SettingsManager::getInstance().getBool("pdlAudioSettings", false), false);
    uiScaleComboBox->setSelectedId(
        UiScale::normalisePercent(SettingsManager::getInstance().getInt(UiScale::settingsKey, UiScale::defaultPercent)),
        dontSendNotification);

    // Set NAM directory value from download manager
    namDirValue->setText(Tone3000DownloadManager::getInstance().getCacheDirectory().getFullPathName(),
                         dontSendNotification);

#ifndef __APPLE__
    useTrayIcon = SettingsManager::getInstance().getBool("useTrayIcon", false);
    useTrayIconButton->setToggleState(useTrayIcon, false);
    if (useTrayIcon)
        startInTrayButton->setToggleState(SettingsManager::getInstance().getBool("startInTray", false), false);
    else
    {
        startInTrayButton->setToggleState(false, false);
        startInTrayButton->setEnabled(false);
    }
#else
    useTrayIconButton->setEnabled(false);
    startInTrayButton->setEnabled(false);
#endif

    //[/UserPreSize]

    setSize(560, 680);

    //[Constructor] You can add your own custom stuff here..
    //[/Constructor]
}

PreferencesDialog::~PreferencesDialog()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    //[/Destructor_pre]

    delete oscPortLabel;
    oscPortLabel = nullptr;
    delete oscPortEditor;
    oscPortEditor = nullptr;
    delete oscLabel;
    oscLabel = nullptr;
    delete oscMulticastLabel;
    oscMulticastLabel = nullptr;
    delete oscMulticastEditor;
    oscMulticastEditor = nullptr;
    delete multicastHintLabel;
    multicastHintLabel = nullptr;
    delete ioOptionsLabel;
    ioOptionsLabel = nullptr;
    delete audioInputButton;
    audioInputButton = nullptr;
    delete midiInputButton;
    midiInputButton = nullptr;
    delete oscInputButton;
    oscInputButton = nullptr;
    delete virtualMidiInputButton;
    virtualMidiInputButton = nullptr;
    delete otherLabel;
    otherLabel = nullptr;
    delete mappingsWindowButton;
    mappingsWindowButton = nullptr;
    delete loopPatchesButton;
    loopPatchesButton = nullptr;
    delete windowsOnTopButton;
    windowsOnTopButton = nullptr;
    delete ignorePinNamesButton;
    ignorePinNamesButton = nullptr;
    delete midiLabel;
    midiLabel = nullptr;
    delete midiProgramChangeButton;
    midiProgramChangeButton = nullptr;
    delete mmcTransportButton;
    mmcTransportButton = nullptr;
    delete useTrayIconButton;
    useTrayIconButton = nullptr;
    delete startInTrayButton;
    startInTrayButton = nullptr;
    delete fixedSizeButton;
    fixedSizeButton = nullptr;
    delete pdlAudioSettingsButton;
    pdlAudioSettingsButton = nullptr;
    delete interfaceLabel;
    interfaceLabel = nullptr;
    delete uiScaleLabel;
    uiScaleLabel = nullptr;
    delete uiScaleComboBox;
    uiScaleComboBox = nullptr;
    delete namLabel;
    namLabel = nullptr;
    delete namDirLabel;
    namDirLabel = nullptr;
    delete namDirValue;
    namDirValue = nullptr;
    delete namDirBrowseButton;
    namDirBrowseButton = nullptr;
    delete viewport;
    viewport = nullptr;
    contentComponent = nullptr;

    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void PreferencesDialog::paint(Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    //[/UserPrePaint]

    //[UserPaint] Add your own custom painting code here..

    g.fillAll(ColourScheme::getInstance().colours["Window Background"]);

    //[/UserPaint]
}

void PreferencesDialog::resized()
{
    if (viewport != nullptr && contentComponent != nullptr)
    {
        viewport->setBounds(getLocalBounds());
        const auto contentWidth = jmax(preferencesContentMinWidth, getWidth());
        contentComponent->setSize(contentWidth, preferencesContentMinHeight);
    }

    const auto contentWidth = contentComponent != nullptr ? contentComponent->getWidth() : getWidth();

    oscPortLabel->setBounds(8, 40, 72, 24);
    oscPortEditor->setBounds(80, 40, 64, 24);
    oscLabel->setBounds(0, 8, 260, 24);
    oscMulticastLabel->setBounds(8, 72, 160, 24);
    oscMulticastEditor->setBounds(168, 72, 112, 24);
    multicastHintLabel->setBounds(288, 72, jmax(120, contentWidth - 304), 24);
    interfaceLabel->setBounds(0, 112, 150, 24);
    uiScaleLabel->setBounds(16, 136, 88, 24);
    uiScaleComboBox->setBounds(104, 136, 104, 24);
    ioOptionsLabel->setBounds(0, 176, 160, 24);
    audioInputButton->setBounds(16, 208, 128, 24);
    midiInputButton->setBounds(16, 232, 120, 24);
    oscInputButton->setBounds(16, 256, 120, 24);
    virtualMidiInputButton->setBounds(16, 280, 190, 24);
    midiLabel->setBounds(0, 332, 104, 24);
    midiProgramChangeButton->setBounds(16, 356, 360, 24);
    mmcTransportButton->setBounds(16, 380, 304, 24);
    otherLabel->setBounds(0, 420, 150, 24);
    mappingsWindowButton->setBounds(16, 444, 440, 24);
    loopPatchesButton->setBounds(16, 468, 260, 24);
    windowsOnTopButton->setBounds(16, 492, 320, 24);
    ignorePinNamesButton->setBounds(16, 516, 220, 24);
    useTrayIconButton->setBounds(16, 540, 240, 24);
    startInTrayButton->setBounds(16, 564, 220, 24);
    fixedSizeButton->setBounds(16, 588, 280, 24);
    pdlAudioSettingsButton->setBounds(16, 612, 280, 24);

    // NAM Options
    namLabel->setBounds(0, 644, 150, 24);
    namDirLabel->setBounds(16, 668, 150, 24);
    namDirBrowseButton->setBounds(contentWidth - 88, 668, 78, 24);
    namDirValue->setBounds(166, 668, jmax(80, contentWidth - 264), 24);
    //[UserResized] Add your own custom resize handling here..
    //[/UserResized]
}

void PreferencesDialog::buttonClicked(Button* buttonThatWasClicked)
{
    //[UserbuttonClicked_Pre]
    //[/UserbuttonClicked_Pre]

    if (buttonThatWasClicked == audioInputButton)
    {
        //[UserButtonCode_audioInputButton] -- add your button handler code here..

        mainPanel->enableAudioInput(audioInputButton->getToggleState());

        //[/UserButtonCode_audioInputButton]
    }
    else if (buttonThatWasClicked == midiInputButton)
    {
        //[UserButtonCode_midiInputButton] -- add your button handler code here..

        mainPanel->enableMidiInput(midiInputButton->getToggleState());

        //[/UserButtonCode_midiInputButton]
    }
    else if (buttonThatWasClicked == oscInputButton)
    {
        //[UserButtonCode_oscInputButton] -- add your button handler code here..

        mainPanel->enableOscInput(oscInputButton->getToggleState());

        //[/UserButtonCode_oscInputButton]
    }
    else if (buttonThatWasClicked == virtualMidiInputButton)
    {
        mainPanel->enableVirtualMidiInput(virtualMidiInputButton->getToggleState());
    }
    else if (buttonThatWasClicked == mappingsWindowButton)
    {
        //[UserButtonCode_mappingsWindowButton] -- add your button handler code here..

        mainPanel->setAutoMappingsWindow(mappingsWindowButton->getToggleState());

        //[/UserButtonCode_mappingsWindowButton]
    }
    else if (buttonThatWasClicked == loopPatchesButton)
    {
        //[UserButtonCode_loopPatchesButton] -- add your button handler code here..

        SettingsManager::getInstance().setValue("LoopPatches", loopPatchesButton->getToggleState());

        //[/UserButtonCode_loopPatchesButton]
    }
    else if (buttonThatWasClicked == windowsOnTopButton)
    {
        //[UserButtonCode_windowsOnTopButton] -- add your button handler code here..

        SettingsManager::getInstance().setValue("WindowsOnTop", windowsOnTopButton->getToggleState());

        //[/UserButtonCode_windowsOnTopButton]
    }
    else if (buttonThatWasClicked == ignorePinNamesButton)
    {
        //[UserButtonCode_ignorePinNamesButton] -- add your button handler code here..

        SettingsManager::getInstance().setValue("IgnorePinNames", ignorePinNamesButton->getToggleState());

        //[/UserButtonCode_ignorePinNamesButton]
    }
    else if (buttonThatWasClicked == midiProgramChangeButton)
    {
        //[UserButtonCode_midiProgramChangeButton] -- add your button handler code here..

        SettingsManager::getInstance().setValue("midiProgramChange", midiProgramChangeButton->getToggleState());

        //[/UserButtonCode_midiProgramChangeButton]
    }
    else if (buttonThatWasClicked == mmcTransportButton)
    {
        //[UserButtonCode_mmcTransportButton] -- add your button handler code here..

        SettingsManager::getInstance().setValue("mmcTransport", mmcTransportButton->getToggleState());

        //[/UserButtonCode_mmcTransportButton]
    }
    else if (buttonThatWasClicked == useTrayIconButton)
    {
        //[UserButtonCode_useTrayIconButton] -- add your button handler code here..

        ((App*)JUCEApplication::getInstance())->showTrayIcon(useTrayIconButton->getToggleState());
        if (useTrayIconButton->getToggleState())
            startInTrayButton->setEnabled(true);
        else
        {
            startInTrayButton->setToggleState(false, false);
            startInTrayButton->setEnabled(false);
        }

        SettingsManager::getInstance().setValue("useTrayIcon", useTrayIconButton->getToggleState());

        //[/UserButtonCode_useTrayIconButton]
    }
    else if (buttonThatWasClicked == startInTrayButton)
    {
        //[UserButtonCode_startInTrayButton] -- add your button handler code here..

        SettingsManager::getInstance().setValue("startInTray", startInTrayButton->getToggleState());

        //[/UserButtonCode_startInTrayButton]
    }
    else if (buttonThatWasClicked == fixedSizeButton)
    {
        //[UserButtonCode_fixedSizeButton] -- add your button handler code here..

        SettingsManager::getInstance().setValue("fixedSizeWindows", fixedSizeButton->getToggleState());

        //[/UserButtonCode_fixedSizeButton]
    }
    else if (buttonThatWasClicked == pdlAudioSettingsButton)
    {
        //[UserButtonCode_pdlAudioSettingsButton] -- add your button handler code here..

        SettingsManager::getInstance().setValue("pdlAudioSettings", pdlAudioSettingsButton->getToggleState());

        //[/UserButtonCode_pdlAudioSettingsButton]
    }
    else if (buttonThatWasClicked == namDirBrowseButton)
    {
        FileChooser chooser("Select NAM Download Directory", Tone3000DownloadManager::getInstance().getCacheDirectory(),
                            "", true);

        if (chooser.browseForDirectory())
        {
            File selectedDir = chooser.getResult();
            Tone3000DownloadManager::getInstance().setCacheDirectory(selectedDir);
            namDirValue->setText(selectedDir.getFullPathName(), dontSendNotification);
        }
    }

    //[UserbuttonClicked_Post]
    //[/UserbuttonClicked_Post]
}

void PreferencesDialog::comboBoxChanged(ComboBox* comboBoxThatHasChanged)
{
    if (comboBoxThatHasChanged == uiScaleComboBox)
    {
        const auto percent = UiScale::normalisePercent(uiScaleComboBox->getSelectedId());
        mainPanel->setUiScalePercent(percent);
        repaint();
    }
}

//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...

//------------------------------------------------------------------------------
void PreferencesDialog::textEditorReturnKeyPressed(TextEditor& editor)
{
    if (editor.getName() == "oscPortEditor")
    {
        currentPort = editor.getText();
        mainPanel->setSocketPort(currentPort);
    }
    else if (editor.getName() == "oscMulticastEditor")
    {
        currentMulticast = editor.getText();
        mainPanel->setSocketMulticast(currentMulticast);
    }
}

//------------------------------------------------------------------------------
void PreferencesDialog::textEditorEscapeKeyPressed(TextEditor& editor)
{
    if (editor.getName() == "oscPortEditor")
        editor.setText(currentPort, false);
    else if (editor.getName() == "oscMulticastEditor")
        editor.setText(currentMulticast, false);
}

//------------------------------------------------------------------------------
void PreferencesDialog::textEditorFocusLost(TextEditor& editor)
{
    if (editor.getName() == "oscPortEditor")
    {
        currentPort = editor.getText();
        mainPanel->setSocketPort(currentPort);
    }
    else if (editor.getName() == "oscMulticastEditor")
    {
        currentMulticast = editor.getText();
        mainPanel->setSocketMulticast(currentMulticast);
    }
}

//[/MiscUserCode]

//==============================================================================
#if 0
/*  -- Jucer information section --

    This is where the Jucer puts all of its metadata, so don't change anything in here!

BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="PreferencesDialog" componentName=""
                 parentClasses="public Component, public TextEditor::Listener"
                 constructorParams="MainPanel *panel, const String&amp; port, const String&amp; multicastAddress"
                 variableInitialisers="" snapPixels="8" snapActive="1" snapShown="1"
                 overlayOpacity="0.330000013" fixedSize="0" initialWidth="560"
                 initialHeight="530">
  <BACKGROUND backgroundColour="ffeeece1">
    <RECT pos="12 132 24M 82" fill="solid: ffffffff" hasStroke="1" stroke="1, mitered, butt"
          strokeColour="solid: 40000000"/>
  </BACKGROUND>
  <LABEL name="oscPortLabe" id="25a7e3b1bb323992" memberName="oscPortLabe"
         virtualName="" explicitFocusOrder="0" pos="8 40 72 24" edTextCol="ff000000"
         edBkgCol="0" labelText="OSC Port:" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="0" fontname="Default font" fontsize="15"
         bold="0" italic="0" justification="33"/>
  <TEXTEDITOR name="oscPortEditor" id="134f9867ca7a8638" memberName="oscPortEditor"
              virtualName="" explicitFocusOrder="0" pos="80 40 64 24" initialText="5678"
              multiline="0" retKeyStartsLine="0" readonly="0" scrollbars="1"
              caret="1" popupmenu="1"/>
  <LABEL name="oscLabe" id="b47f1edc63447709" memberName="oscLabe" virtualName=""
         explicitFocusOrder="0" pos="0 8 208 24" edTextCol="ff000000"
         edBkgCol="0" labelText="Open Sound Control Options" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="15" bold="1" italic="0" justification="33"/>
  <LABEL name="oscMulticastLabe" id="591f82ca7e2ff8d" memberName="oscMulticastLabe"
         virtualName="" explicitFocusOrder="0" pos="8 72 160 24" edTextCol="ff000000"
         edBkgCol="0" labelText="OSC Multicast Address:" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="15" bold="0" italic="0" justification="33"/>
  <TEXTEDITOR name="oscMulticastEditor" id="1f35983ab3efaf2b" memberName="oscMulticastEditor"
              virtualName="" explicitFocusOrder="0" pos="168 72 112 24" initialText=""
              multiline="0" retKeyStartsLine="0" readonly="0" scrollbars="1"
              caret="1" popupmenu="1"/>
  <LABEL name="multicastHintLabe" id="b799cdf5320811b0" memberName="multicastHintLabe"
         virtualName="" explicitFocusOrder="0" pos="280 72 272 24" textCol="80000000"
         edTextCol="ff000000" edBkgCol="0" labelText="(leave blank for a one-to-one connection)"
         editableSingleClick="0" editableDoubleClick="0" focusDiscardsChanges="0"
         fontname="Default font" fontsize="15" bold="0" italic="0" justification="33"/>
  <LABEL name="ioOptionsLabe" id="2926986b1304b84f" memberName="ioOptionsLabe"
         virtualName="" explicitFocusOrder="0" pos="0 104 136 24" edTextCol="ff000000"
         edBkgCol="0" labelText="Visible I/O Nodes" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="15" bold="1" italic="0" justification="33"/>
  <TOGGLEBUTTON name="audioInputButton" id="aa35f82f890421a3" memberName="audioInputButton"
                virtualName="" explicitFocusOrder="0" pos="16 136 96 24" buttonText="Audio Input"
                connectedEdges="0" needsCallback="1" radioGroupId="0" state="1"/>
  <TOGGLEBUTTON name="midiInputButton" id="8018f8e307323ec8" memberName="midiInputButton"
                virtualName="" explicitFocusOrder="0" pos="16 160 88 24" buttonText="Midi Input"
                connectedEdges="0" needsCallback="1" radioGroupId="0" state="1"/>
  <TOGGLEBUTTON name="oscInputButton" id="55b04d19b16b9942" memberName="oscInputButton"
                virtualName="" explicitFocusOrder="0" pos="16 184 88 24" buttonText="OSC Input"
                connectedEdges="0" needsCallback="1" radioGroupId="0" state="1"/>
  <LABEL name="otherLabe" id="202db8c3e9a34bc7" memberName="otherLabe"
         virtualName="" explicitFocusOrder="0" pos="0 304 150 24" edTextCol="ff000000"
         edBkgCol="0" labelText="Other Options" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="15" bold="1" italic="0" justification="33"/>
  <TOGGLEBUTTON name="mappingsWindowButton" id="e72a3b9c5addc0c7" memberName="mappingsWindowButton"
                virtualName="" explicitFocusOrder="0" pos="16 328 376 24" buttonText="Open mappings window on successful param connection"
                connectedEdges="0" needsCallback="1" radioGroupId="0" state="1"/>
  <TOGGLEBUTTON name="loopPatchesButton" id="aef1fb50754f52b7" memberName="loopPatchesButton"
                virtualName="" explicitFocusOrder="0" pos="16 352 208 24" buttonText="Loop next/prev patch action"
                connectedEdges="0" needsCallback="1" radioGroupId="0" state="1"/>
  <TOGGLEBUTTON name="windowsOnTopButton" id="9ecea969a24ce2a2" memberName="windowsOnTopButton"
                virtualName="" explicitFocusOrder="0" pos="16 376 256 24" buttonText="Set plugin windows Always On Top"
                connectedEdges="0" needsCallback="1" radioGroupId="0" state="0"/>
  <TOGGLEBUTTON name="ignorePinNamesButton" id="431bd0e29682e3d8" memberName="ignorePinNamesButton"
                virtualName="" explicitFocusOrder="0" pos="16 400 176 24" buttonText="Ignore plugin pin names"
                connectedEdges="0" needsCallback="1" radioGroupId="0" state="0"/>
  <LABEL name="midiLabe" id="f7b44c527444165d" memberName="midiLabe"
         virtualName="" explicitFocusOrder="0" pos="0 224 104 24" edTextCol="ff000000"
         edBkgCol="0" labelText="Midi Options" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="15" bold="1" italic="0" justification="33"/>
  <TOGGLEBUTTON name="midiProgramChangeButton" id="42be78a2253f0618" memberName="midiProgramChangeButton"
                virtualName="" explicitFocusOrder="0" pos="16 248 288 24" buttonText="Program Change messages switch patches"
                connectedEdges="0" needsCallback="1" radioGroupId="0" state="0"/>
  <TOGGLEBUTTON name="mmcTransportButton" id="75d98d256ceb308a" memberName="mmcTransportButton"
                virtualName="" explicitFocusOrder="0" pos="16 272 232 24" buttonText="Main transport responds to MMC"
                connectedEdges="0" needsCallback="1" radioGroupId="0" state="0"/>
  <TOGGLEBUTTON name="useTrayIconButton" id="84259dceb70b80e0" memberName="useTrayIconButton"
                virtualName="" explicitFocusOrder="0" pos="16 424 200 24" buttonText="Display tray icon (not OSX)"
                connectedEdges="0" needsCallback="1" radioGroupId="0" state="0"/>
  <TOGGLEBUTTON name="startInTrayButton" id="94a617ed323eb91" memberName="startInTrayButton"
                virtualName="" explicitFocusOrder="0" pos="16 448 168 24" buttonText="Start in tray (not OSX)"
                connectedEdges="0" needsCallback="1" radioGroupId="0" state="0"/>
  <TOGGLEBUTTON name="fixedSizeButton" id="fe165d18d64b3d8" memberName="fixedSizeButton"
                virtualName="" explicitFocusOrder="0" pos="16 472 224 24" buttonText="Force fixed-size plugin windows"
                connectedEdges="0" needsCallback="1" radioGroupId="0" state="1"/>
  <TOGGLEBUTTON name="pdlAudioSettingsButton" id="42de854235eeaedc" memberName="pdlAudioSettingsButton"
                virtualName="" explicitFocusOrder="0" pos="16 496 224 24" buttonText="Save audio settings in .pdl files"
                connectedEdges="0" needsCallback="1" radioGroupId="0" state="0"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif
