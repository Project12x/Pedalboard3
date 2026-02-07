/*
  ==============================================================================

  This is an automatically generated file created by the Jucer!

  Creation date:  26 Nov 2011 3:32:09pm

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
#include "AboutPage.h"
#include "App.h"
#include "ApplicationMappingsEditor.h"
#include "AudioSingletons.h"
#include "BlacklistWindow.h"
#include "BranchesLAF.h"
#include "ColourSchemeEditor.h"
#include "CrashProtection.h"
#include "Images.h"
#include "JuceHelperStuff.h"
#include "SafePluginScanner.h"
#include "IRLoaderProcessor.h"
#include "LabelProcessor.h"
#include "LogDisplay.h"
#include "LogFile.h"
#include "MainTransport.h"
#include "MidiFilePlayer.h"
#include "MidiUtilityProcessors.h"
#include "NAMProcessor.h"
#include "NotesProcessor.h"
#include "OscilloscopeProcessor.h"
#include "PatchOrganiser.h"
#include "PedalboardProcessors.h"
#include "PluginField.h"
#include "PluginPoolManager.h"
#include "PreferencesDialog.h"
#include "RoutingProcessors.h"
#include "SettingsManager.h"
#include "StageView.h"
#include "TapTempoBox.h"
#include "ToastOverlay.h"
#include "ToneGeneratorProcessor.h"
#include "TunerProcessor.h"
#include "UserPresetWindow.h"
#include "Vectors.h"

#include <iostream>
#include <sstream>

using namespace std;
//[/Headers]

#include "MainPanel.h"

//[MiscUserDefs] You can add your own user definitions and misc code here...

//------------------------------------------------------------------------------
File MainPanel::lastDocument = File();

//------------------------------------------------------------------------------
class PluginListWindow : public DocumentWindow
{
  public:
    PluginListWindow(KnownPluginList& knownPluginList, MainPanel* p, bool useSafeScanner = true)
        : DocumentWindow("Available Plugins", Colour(0xffeeece1),
                         DocumentWindow::minimiseButton | DocumentWindow::closeButton),
          panel(p)
    {
        const File deadMansPedalFile(
            SettingsManager::getInstance().getUserDataDirectory().getChildFile("RecentlyCrashedPluginsList"));

        if (useSafeScanner)
        {
            // Use our safe scanner with out-of-process support
            setContentOwned(new SafePluginListComponent(AudioPluginFormatManagerSingleton::getInstance(), knownPluginList,
                                                        deadMansPedalFile, nullptr),
                            true);
        }
        else
        {
            // Fall back to JUCE's built-in scanner
            setContentOwned(new PluginListComponent(AudioPluginFormatManagerSingleton::getInstance(), knownPluginList,
                                                    deadMansPedalFile, nullptr),
                            true);
        }

        setResizable(true, false);
        centreWithSize(500, 500);  // Slightly larger for better UX
        setUsingNativeTitleBar(true);
        getPeer()->setIcon(ImageCache::getFromMemory(Images::icon512_png, Images::icon512_pngSize));

        restoreWindowStateFromString(SettingsManager::getInstance().getString("listWindowPos"));
        setVisible(true);
    }

    ~PluginListWindow()
    {
        panel->setListWindow(0);

        SettingsManager::getInstance().setValue("listWindowPos", getWindowStateAsString());
    }

    void closeButtonPressed() { delete this; }

  private:
    ///	The 'parent' main panel.
    MainPanel* panel;
};
//[/MiscUserDefs]

//==============================================================================
MainPanel::MainPanel(ApplicationCommandManager* appManager)
    : FileBasedDocument(".pd", "*.pd", "Choose a set of patches to open...", "Choose a set of patches to save as..."),
      Thread("OSC Thread"), commandManager(appManager), currentPatch(0), patchLabel(0), prevPatch(0), nextPatch(0),
      patchComboBox(0), viewport(0), cpuSlider(0), cpuLabel(0), playButton(0), rtzButton(0), tempoLabel(0),
      tempoEditor(0), tapTempoButton(0)
{
    addAndMakeVisible(patchLabel = new Label("patchLabel", "Patch:"));
    patchLabel->setFont(Font(15.0000f, Font::plain));
    patchLabel->setJustificationType(Justification::centredLeft);
    patchLabel->setEditable(false, false, false);
    patchLabel->setColour(TextEditor::textColourId, Colours::black);
    patchLabel->setColour(TextEditor::backgroundColourId, Colour(0x0));

    addAndMakeVisible(prevPatch = new TextButton("prevPatch"));
    prevPatch->setButtonText("-");
    prevPatch->setConnectedEdges(Button::ConnectedOnRight);
    prevPatch->addListener(this);

    addAndMakeVisible(nextPatch = new TextButton("nextPatch"));
    nextPatch->setButtonText("+");
    nextPatch->setConnectedEdges(Button::ConnectedOnLeft);
    nextPatch->addListener(this);

    addAndMakeVisible(patchComboBox = new ComboBox("patchComboBox"));
    patchComboBox->setEditableText(true);
    patchComboBox->setJustificationType(Justification::centredLeft);
    patchComboBox->setTextWhenNothingSelected(String());
    patchComboBox->setTextWhenNoChoicesAvailable("(no choices)");
    patchComboBox->addItem("1 - <untitled>", 1);
    patchComboBox->addItem("<new patch>", 2);
    patchComboBox->addListener(this);

    addAndMakeVisible(viewport = new Viewport("new viewport"));

    addAndMakeVisible(cpuSlider = new Slider("cpuSlider"));
    cpuSlider->setRange(0, 1, 0);
    cpuSlider->setSliderStyle(Slider::LinearBar);
    cpuSlider->setTextBoxStyle(Slider::NoTextBox, true, 80, 20);
    cpuSlider->addListener(this);

    addAndMakeVisible(cpuLabel = new Label("cpuLabel", "CPU Usage:"));
    cpuLabel->setFont(Font(15.0000f, Font::plain));
    cpuLabel->setJustificationType(Justification::centredLeft);
    cpuLabel->setEditable(false, false, false);
    cpuLabel->setColour(TextEditor::textColourId, Colours::black);
    cpuLabel->setColour(TextEditor::backgroundColourId, Colour(0x0));

    addAndMakeVisible(playButton = new DrawableButton("playButton", DrawableButton::ImageOnButtonBackground));
    playButton->setName("playButton");

    addAndMakeVisible(rtzButton = new DrawableButton("rtzButton", DrawableButton::ImageOnButtonBackground));
    rtzButton->setName("rtzButton");

    addAndMakeVisible(tempoLabel = new Label("tempoLabel", "Tempo:"));
    tempoLabel->setFont(Font(15.0000f, Font::plain));
    tempoLabel->setJustificationType(Justification::centredLeft);
    tempoLabel->setEditable(false, false, false);
    tempoLabel->setColour(TextEditor::textColourId, Colours::black);
    tempoLabel->setColour(TextEditor::backgroundColourId, Colour(0x0));

    addAndMakeVisible(tempoEditor = new TextEditor("tempoEditor"));
    tempoEditor->setMultiLine(false);
    tempoEditor->setReturnKeyStartsNewLine(false);
    tempoEditor->setReadOnly(false);
    tempoEditor->setScrollbarsShown(true);
    tempoEditor->setCaretVisible(true);
    tempoEditor->setPopupMenuEnabled(true);
    tempoEditor->setText("120.00");

    addAndMakeVisible(tapTempoButton = new ArrowButton("tapTempoButton", 0.0, Colour(0x40000000)));
    tapTempoButton->setName("tapTempoButton");

    addAndMakeVisible(organiseButton = new TextButton("organiseButton"));
    organiseButton->setButtonText("Manage");
    organiseButton->addListener(this);

    addAndMakeVisible(fitButton = new TextButton("fitButton"));
    fitButton->setButtonText("Fit");
    fitButton->setTooltip("Fit all nodes to screen");
    fitButton->addListener(this);

    //[UserPreSize]

    // Logger::setCurrentLogger(this);

    Colour buttonCol = ColourScheme::getInstance().colours["Button Colour"];

    patches.add(0);

    programChangePatch = currentPatch;
    lastCombo = 1;

    listWindow = 0;

    doNotSaveNextPatch = false;

    lastTempoTicks = 0;

    prevPatch->setTooltip("Previous patch");
    nextPatch->setTooltip("Next patch");
    playButton->setTooltip("Play (main transport)");
    rtzButton->setTooltip("Return to zero (main transport)");
    tapTempoButton->setTooltip("Tap tempo");
    organiseButton->setTooltip("Manage Setlist (Reorder/Rename Patches)");

    // So the user can't drag the cpu meter.
    cpuSlider->setInterceptsMouseClicks(false, true);
    cpuSlider->setColour(Slider::thumbColourId, ColourScheme::getInstance().colours["CPU Meter Colour"]);

    // Setup the DrawableButton images.
    // Setup the DrawableButton images.
    playImage.reset(JuceHelperStuff::loadSVGFromMemory(Vectors::playbutton_svg, Vectors::playbutton_svgSize));
    pauseImage.reset(JuceHelperStuff::loadSVGFromMemory(Vectors::pausebutton_svg, Vectors::pausebutton_svgSize));
    playButton->setImages(playImage.get());
    playButton->setColour(DrawableButton::backgroundColourId, buttonCol);
    playButton->setColour(DrawableButton::backgroundOnColourId, buttonCol);
    playButton->addListener(this);

    std::unique_ptr<Drawable> rtzImage(
        JuceHelperStuff::loadSVGFromMemory(Vectors::rtzbutton_svg, Vectors::rtzbutton_svgSize));
    rtzButton->setImages(rtzImage.get());
    rtzButton->setColour(DrawableButton::backgroundColourId, buttonCol);
    rtzButton->setColour(DrawableButton::backgroundOnColourId, buttonCol);
    rtzButton->addListener(this);

    MainTransport::getInstance()->registerTransport(this);

    tempoEditor->addListener(this);
    tempoEditor->setInputRestrictions(0, "0123456789.");

    tapTempoButton->addListener(this);

    // Setup the soundcard.
    String tempstr;
    auto savedAudioState = // JUCE 8: returns unique_ptr
        SettingsManager::getInstance().getXmlValue("audioDeviceState");
    // Support up to 16 input/output channels for multi-channel interfaces
    tempstr = deviceManager.initialise(16, 16, savedAudioState.get(), true);
    if (savedAudioState)
    {
        // JUCE 8: unique_ptr auto-deleted
    }

    // Load the plugin list.
    auto savedPluginList = // JUCE 8: returns unique_ptr
        SettingsManager::getInstance().getXmlValue("pluginList");
    if (savedPluginList != 0)
    {
        pluginList.recreateFromXml(*savedPluginList);
        // JUCE 8: unique_ptr auto-deleted
    }
    {
        LevelProcessor lev;
        FilePlayerProcessor fPlay;
        OutputToggleProcessor toggle;
        VuMeterProcessor vuMeter;
        RecorderProcessor recorder;
        MetronomeProcessor metronome;
        LooperProcessor looper;
        PluginDescription desc;

        lev.fillInPluginDescription(desc);
        pluginList.addType(desc);

        fPlay.fillInPluginDescription(desc);
        pluginList.addType(desc);

        toggle.fillInPluginDescription(desc);
        pluginList.addType(desc);

        vuMeter.fillInPluginDescription(desc);
        pluginList.addType(desc);

        recorder.fillInPluginDescription(desc);
        pluginList.addType(desc);

        metronome.fillInPluginDescription(desc);
        pluginList.addType(desc);

        looper.fillInPluginDescription(desc);
        pluginList.addType(desc);

        TunerProcessor tuner;
        tuner.fillInPluginDescription(desc);
        pluginList.addType(desc);

        ToneGeneratorProcessor toneGen;
        toneGen.fillInPluginDescription(desc);
        pluginList.addType(desc);

        SplitterProcessor splitter;
        splitter.fillInPluginDescription(desc);
        pluginList.addType(desc);

        MixerProcessor mixer;
        mixer.fillInPluginDescription(desc);
        pluginList.addType(desc);

        NotesProcessor notes;
        notes.fillInPluginDescription(desc);
        pluginList.addType(desc);

        LabelProcessor label;
        label.fillInPluginDescription(desc);
        pluginList.addType(desc);

        MidiFilePlayerProcessor midiFilePlayer;
        midiFilePlayer.fillInPluginDescription(desc);
        pluginList.addType(desc);

        IRLoaderProcessor irLoader;
        irLoader.fillInPluginDescription(desc);
        pluginList.addType(desc);

        NAMProcessor nam;
        nam.fillInPluginDescription(desc);
        pluginList.addType(desc);

        OscilloscopeProcessor oscilloscope;
        oscilloscope.fillInPluginDescription(desc);
        pluginList.addType(desc);

        MidiTransposeProcessor midiTranspose;
        midiTranspose.fillInPluginDescription(desc);
        pluginList.addType(desc);

        MidiRechannelizeProcessor midiRechannelize;
        midiRechannelize.fillInPluginDescription(desc);
        pluginList.addType(desc);

        KeyboardSplitProcessor keyboardSplit;
        keyboardSplit.fillInPluginDescription(desc);
        pluginList.addType(desc);
    }
    pluginList.addChangeListener(this);

    pluginList.sort(KnownPluginList::sortAlphabetically,
                    true); // JUCE 8: sort takes 2 args

    // Register plugin list singleton for SubGraph editors to access
    KnownPluginListSingleton::setInstance(&pluginList);

    // Configure graph bus layout to match device channels
    if (auto* device = deviceManager.getCurrentAudioDevice())
    {
        auto activeInputs = device->getActiveInputChannels();
        auto activeOutputs = device->getActiveOutputChannels();
        int numInputs = activeInputs.countNumberOfSetBits();
        int numOutputs = activeOutputs.countNumberOfSetBits();
        signalPath.setDeviceChannelCounts(numInputs, numOutputs);
    }

    // Setup the signal path to connect it to the soundcard.
    graphPlayer.setProcessor(&signalPath.getGraph());
    deviceManager.addAudioCallback(&graphPlayer);

    // Device meter tap for I/O node VU meters (can be disabled for debugging)
    if (SettingsManager::getInstance().getBool("EnableDeviceMeterTap", true))
    {
        deviceManager.addAudioCallback(&deviceMeterTap);
        DeviceMeterTap::setInstance(&deviceMeterTap);
    }
    deviceManager.addChangeListener(this);

    // Setup midi.
    {
        auto midiDevices = MidiInput::getAvailableDevices(); // JUCE 8: Array<MidiDeviceInfo>
        for (const auto& device : midiDevices)
            deviceManager.addMidiInputCallback(device.identifier, &graphPlayer);
    }

    // Setup the PluginField.
    PluginField* field = new PluginField(&signalPath, &pluginList, commandManager);
    field->addChangeListener(this);
    viewport->setViewedComponent(field);
    viewport->setWantsKeyboardFocus(false);

    patchComboBox->setSelectedId(1);

    // Setup the socket.
    {
        String port, address;

        port = SettingsManager::getInstance().getString("OSCPort", "5678");
        if (port == "")
            port = "5678";
        address = SettingsManager::getInstance().getString("OSCMulticastAddress");

        if (SettingsManager::getInstance().getBool("OscInput", true))
        {
            sock.setPort((int16_t)port.getIntValue());
            sock.setMulticastGroup(std::string(address.toUTF8()));
            sock.bindSocket();
            startThread();
        }
    }

    savePatch();

    // Necessary?
    Process::setPriority(Process::HighPriority);

    // Used to ensure we get MidiAppMapping events even when the window's
    // not focused.
    appManager->setFirstCommandTarget(this);

    //[/UserPreSize]

    setSize(1024, 570);

    //[Constructor] You can add your own custom stuff here..
    setWantsKeyboardFocus(true);

    // Setup the program change warning.
    // Setup the program change warning.
    warningBox.reset(new CallOutBox(warningText, patchComboBox->getBounds(), this));
    warningBox->setVisible(false);

    // Add ToastOverlay for premium notifications
    addChildComponent(&ToastOverlay::getInstance());

    // Start timers.
    startTimer(CpuTimer, 100);
    startTimer(MidiAppTimer, 30);

    // To load the default patch.
    {
        File defaultPatch = JuceHelperStuff::getAppDataFolder().getChildFile("default.pdl");

        if (defaultPatch.existsAsFile())
            commandManager->invokeDirectly(FileNew, true);
    }

    // Set up crash protection auto-save callback
    CrashProtection::getInstance().setAutoSaveCallback(
        [this]()
        {
            // Save current patch state before risky plugin operations
            if (hasChangedSinceSaved())
            {
                savePatch();
                spdlog::debug("[MainPanel] Auto-save triggered by crash protection");
            }
        });

    spdlog::info("[MainPanel] Crash protection auto-save callback registered");
    //[/Constructor]
}

MainPanel::~MainPanel()
{
    //[Destructor_pre]. You can add your own custom destruction code here..

    int i;
    auto midiDevices = MidiInput::getAvailableDevices(); // JUCE 8: Array<MidiDeviceInfo>

    // Logger::setCurrentLogger(0);

    signalThreadShouldExit();

    // deviceManager.setAudioCallback(0);
    if (DeviceMeterTap::getInstance() != nullptr)
    {
        deviceManager.removeAudioCallback(&deviceMeterTap);
        DeviceMeterTap::setInstance(nullptr);
    }
    deviceManager.removeAudioCallback(&graphPlayer);
    for (const auto& device : midiDevices)
        deviceManager.removeMidiInputCallback(device.identifier, &graphPlayer);
    graphPlayer.setProcessor(0);
    signalPath.clear(false, false, false);

    if (listWindow)
        delete listWindow;

    for (i = 0; i < patches.size(); ++i)
        delete patches[i];
    //[/Destructor_pre]

    delete patchLabel;
    patchLabel = nullptr;
    delete prevPatch;
    prevPatch = nullptr;
    delete nextPatch;
    nextPatch = nullptr;
    delete patchComboBox;
    patchComboBox = nullptr;
    delete viewport;
    viewport = nullptr;
    delete cpuSlider;
    cpuSlider = nullptr;
    delete cpuLabel;
    cpuLabel = nullptr;
    delete playButton;
    playButton = nullptr;
    delete rtzButton;
    rtzButton = nullptr;
    delete tempoLabel;
    tempoLabel = nullptr;
    delete tempoEditor;
    tempoEditor = nullptr;
    delete tapTempoButton;
    tapTempoButton = nullptr;
    delete organiseButton;
    organiseButton = nullptr;

    //[Destructor]. You can add your own custom destruction code here..

    MainTransport::getInstance()->unregisterTransport(this);
    if (LogFile::getInstance().getIsLogging())
        LogFile::getInstance().stop();

    //[/Destructor]
}

//==============================================================================
void MainPanel::paint(Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..

    Colour tempCol = ColourScheme::getInstance().colours["Button Colour"];

    playButton->setColour(DrawableButton::backgroundColourId, tempCol);
    playButton->setColour(DrawableButton::backgroundOnColourId, tempCol);
    rtzButton->setColour(DrawableButton::backgroundColourId, tempCol);
    rtzButton->setColour(DrawableButton::backgroundOnColourId, tempCol);

    //[/UserPrePaint]

    g.fillAll(Colour(0xffeeece1));

    //[UserPaint] Add your own custom painting code here..

    g.fillAll(ColourScheme::getInstance().colours["Window Background"]);

    //[/UserPaint]
}

void MainPanel::resized()
{
    patchLabel->setBounds(8, getHeight() - 33, 48, 24);
    prevPatch->setBounds(264, getHeight() - 33, 24, 24);
    nextPatch->setBounds(288, getHeight() - 33, 24, 24);
    patchComboBox->setBounds(56, getHeight() - 33, 200, 24);
    viewport->setBounds(0, 0, getWidth() - 0, getHeight() - 40);
    cpuSlider->setBounds(getWidth() - 156, getHeight() - 33, 150, 24);
    cpuLabel->setBounds(getWidth() - 236, getHeight() - 33, 78, 24);
    playButton->setBounds(proportionOfWidth(0.5000f) - ((36) / 2), getHeight() - 38, 36, 36);
    rtzButton->setBounds((proportionOfWidth(0.5000f) - ((36) / 2)) + 38, getHeight() - 32, 24, 24);
    tempoLabel->setBounds((proportionOfWidth(0.5000f) - ((36) / 2)) + -151, getHeight() - 33, 64, 24);
    tempoEditor->setBounds((proportionOfWidth(0.5000f) - ((36) / 2)) + -87, getHeight() - 33, 52, 24);
    tapTempoButton->setBounds((proportionOfWidth(0.5000f) - ((36) / 2)) + -31, getHeight() - 27, 10, 16);
    //[UserResized] Add your own custom resize handling here..

    int x, y;
    Component* field = viewport->getViewedComponent();

    x = field->getWidth();
    y = field->getHeight();
    if (field->getWidth() < getWidth())
        x = getWidth();
    if (field->getHeight() < (getHeight() - 40))
        y = getHeight() - 40;
    field->setSize(x, y);

    // Keep StageView covering the entire panel
    if (stageView != nullptr)
        stageView->setBounds(getLocalBounds());

    // Place Organise button in the gap between Transport (Center) and CPU (Right)
    // Center group ends approx width/2 + 50. CPU starts width - 236.
    organiseButton->setBounds(getWidth() - 320, getHeight() - 33, 70, 24);

    // FIT button next to Organise
    fitButton->setBounds(getWidth() - 390, getHeight() - 33, 50, 24);

    //[/UserResized]
}

void MainPanel::buttonClicked(Button* buttonThatWasClicked)
{
    //[UserbuttonClicked_Pre]
    //[/UserbuttonClicked_Pre]

    if (buttonThatWasClicked == prevPatch)
    {
        //[UserButtonCode_prevPatch] -- add your button handler code here..
        commandManager->invokeDirectly(PatchPrevPatch, true);
        //[/UserButtonCode_prevPatch]
    }
    else if (buttonThatWasClicked == nextPatch)
    {
        //[UserButtonCode_nextPatch] -- add your button handler code here..
        /*if(patchComboBox->getSelectedItemIndex() <
           (patchComboBox->getNumItems()-2))
                patchComboBox->setSelectedItemIndex(patchComboBox->getSelectedItemIndex()+1);*/
        commandManager->invokeDirectly(PatchNextPatch, true);
        //[/UserButtonCode_nextPatch]
    }

    //[UserbuttonClicked_Post]

    else if (buttonThatWasClicked == playButton)
        commandManager->invokeDirectly(TransportPlay, true);
    else if (buttonThatWasClicked == rtzButton)
        commandManager->invokeDirectly(TransportRtz, true);
    else if (buttonThatWasClicked == tapTempoButton)
    {
        PluginField* pluginField = dynamic_cast<PluginField*>(viewport->getViewedComponent());
        TapTempoBox tempoBox(pluginField, tempoEditor);

        CallOutBox callout(tempoBox, tapTempoButton->getBounds(), this);
        callout.runModalLoop();
    }
    else if (buttonThatWasClicked == fitButton)
    {
        if (auto* pluginField = dynamic_cast<PluginField*>(viewport->getViewedComponent()))
            pluginField->fitToScreen();
    }

    //[/UserbuttonClicked_Post]
}

void MainPanel::comboBoxChanged(ComboBox* comboBoxThatHasChanged)
{
    //[UsercomboBoxChanged_Pre]
    //[/UsercomboBoxChanged_Pre]

    if (comboBoxThatHasChanged == patchComboBox)
    {
        //[UserComboBoxCode_patchComboBox] -- add your combo box handling code
        // here..

        // Add a new patch.
        if (patchComboBox->getSelectedItemIndex() == (patchComboBox->getNumItems() - 1))
        {
            String tempstr;

            // Save current patch.
            savePatch();

            // Setup the new ComboBox stuff.
            tempstr << patchComboBox->getNumItems() << " - <untitled>";
            patchComboBox->changeItemText(patchComboBox->getNumItems(), tempstr);
            patchComboBox->addItem("<new patch>", patchComboBox->getNumItems() + 1);
            patches.add(0);

            // Make the new patch the current patch, clear it to the default
            // state.
            patchComboBox->setSelectedId(patchComboBox->getNumItems() - 1, false);
            switchPatch(patchComboBox->getNumItems() - 2);
            savePatch();

            changed();
        }
        // Update the patch text if the user's changed it.
        else if (patchComboBox->getSelectedItemIndex() == -1)
        {
            patchComboBox->changeItemText(lastCombo, patchComboBox->getText());
            patches[currentPatch]->setAttribute("name", patchComboBox->getText());

            changed();
        }
        // Switch to the new patch.
        else
        {
            switchPatch(patchComboBox->getSelectedItemIndex());
        }

        lastCombo = patchComboBox->getSelectedId();

        //[/UserComboBoxCode_patchComboBox]
    }

    //[UsercomboBoxChanged_Post]
    //[/UsercomboBoxChanged_Post]
}

void MainPanel::sliderValueChanged(Slider* sliderThatWasMoved)
{
    //[UsersliderValueChanged_Pre]
    //[/UsersliderValueChanged_Pre]

    if (sliderThatWasMoved == cpuSlider)
    {
        //[UserSliderCode_cpuSlider] -- add your slider handling code here..
        //[/UserSliderCode_cpuSlider]
    }

    //[UsersliderValueChanged_Post]
    //[/UsersliderValueChanged_Post]
}

//[MiscUserCode] You can add your own definitions of your custom methods or any
// other code here...

//------------------------------------------------------------------------------
void MainPanel::showToast(const String& message)
{
    // Use custom ToastOverlay with Melatonin Blur for premium shadows
    ToastOverlay::getInstance().show(message, 1500);
}

//------------------------------------------------------------------------------
void MainPanel::refreshPluginPoolDefinitions()
{
    auto& pool = PluginPoolManager::getInstance();
    pool.clear();

    for (int i = 0; i < patches.size(); ++i)
    {
        if (patches[i] != nullptr)
            pool.addPatchDefinition(i, std::make_unique<XmlElement>(*patches[i]));
    }
}

//------------------------------------------------------------------------------
void MainPanel::updatePluginPoolDefinition(int patchIndex, const XmlElement* patch)
{
    if (patch == nullptr || patchIndex < 0)
        return;

    PluginPoolManager::getInstance().addPatchDefinition(patchIndex, std::make_unique<XmlElement>(*patch));
}

//------------------------------------------------------------------------------
StringArray MainPanel::getMenuBarNames()
{
    StringArray retval;

    retval.add("File");
    retval.add("Edit");
    retval.add("Options");
    retval.add("Help");

    return retval;
}

//------------------------------------------------------------------------------
PopupMenu MainPanel::getMenuForIndex(int topLevelMenuIndex, const String& menuName)
{
    PopupMenu retval;

    if (menuName == "File")
    {
        retval.addCommandItem(commandManager, FileNew);
        retval.addCommandItem(commandManager, FileOpen);
        retval.addSeparator();
        retval.addCommandItem(commandManager, FileSave);
        retval.addCommandItem(commandManager, FileSaveAs);
        retval.addSeparator();
        retval.addCommandItem(commandManager, FileSaveAsDefault);
        retval.addCommandItem(commandManager, FileResetDefault);
        retval.addSeparator();
        retval.addCommandItem(commandManager, FileExit);
    }
    else if (menuName == "Edit")
    {
        retval.addCommandItem(commandManager, EditUndo);
        retval.addCommandItem(commandManager, EditRedo);
        retval.addSeparator();
        retval.addCommandItem(commandManager, EditDeleteConnection);
        retval.addSeparator();
        retval.addCommandItem(commandManager, EditOrganisePatches);
        retval.addCommandItem(commandManager, EditUserPresetManagement);
        retval.addSeparator();
        retval.addCommandItem(commandManager, EditPanic);
    }
    else if (menuName == "Options")
    {
        retval.addCommandItem(commandManager, OptionsAudio);
        retval.addCommandItem(commandManager, OptionsPluginList);
        retval.addCommandItem(commandManager, OptionsPluginBlacklist);
        retval.addCommandItem(commandManager, OptionsPreferences);
        retval.addCommandItem(commandManager, OptionsColourSchemes);
        retval.addSeparator();
        retval.addCommandItem(commandManager, OptionsKeyMappings);
        retval.addSeparator();
        retval.addCommandItem(commandManager, ToggleStageMode);
    }
    else if (menuName == "Help")
    {
        retval.addCommandItem(commandManager, HelpDocumentation);
        retval.addCommandItem(commandManager, HelpLog);
        retval.addSeparator();
        retval.addCommandItem(commandManager, HelpAbout);
    }

    return retval;
}

//------------------------------------------------------------------------------
void MainPanel::menuItemSelected(int menuItemID, int topLevelMenuIndex) {}

//------------------------------------------------------------------------------
ApplicationCommandTarget* MainPanel::getNextCommandTarget()
{
    return findFirstTargetParentComponent();
}

//------------------------------------------------------------------------------
void MainPanel::getAllCommands(Array<CommandID>& commands)
{
    const CommandID ids[] = {FileNew,
                             FileOpen,
                             FileSave,
                             FileSaveAs,
                             FileSaveAsDefault,
                             FileResetDefault,
                             FileExit,
                             EditDeleteConnection,
                             EditOrganisePatches,
                             EditUserPresetManagement,
                             EditUndo,
                             EditRedo,
                             EditPanic,
                             OptionsPreferences,
                             OptionsAudio,
                             OptionsPluginList,
                             OptionsColourSchemes,
                             OptionsKeyMappings,
                             HelpAbout,
                             HelpDocumentation,
                             HelpLog,
                             PatchNextPatch,
                             PatchPrevPatch,
                             TransportPlay,
                             TransportRtz,
                             TransportTapTempo,
                             ToggleStageMode,
                             OptionsPluginBlacklist};
    commands.addArray(ids, numElementsInArray(ids));
}

//------------------------------------------------------------------------------
void MainPanel::getCommandInfo(const CommandID commandID, ApplicationCommandInfo& result)
{
    const String fileCategory("File");
    const String editCategory("Edit");
    const String optionsCategory("Options");
    const String helpCategory("Help");
    const String patchCategory("Patch");
    const String transportCategory("Main Transport");

    switch (commandID)
    {
    case FileNew:
        result.setInfo("New", "Creates a new pedalboard file to work from.", fileCategory, 0);
        result.addDefaultKeypress(L'n', ModifierKeys::commandModifier);
        break;
    case FileOpen:
        result.setInfo("Open...", "Opens an existing pedalboard file from disk.", fileCategory, 0);
        result.addDefaultKeypress(L'o', ModifierKeys::commandModifier);
        break;
    case FileSave:
        result.setInfo("Save", "Saves the current pedalboard file to disk.", fileCategory, 0);
        result.addDefaultKeypress(L's', ModifierKeys::commandModifier);
        break;
    case FileSaveAs:
        result.setInfo("Save As...", "Saves the current pedalboard file to a new file on disk.", fileCategory, 0);
        result.addDefaultKeypress(L's', ModifierKeys::commandModifier | ModifierKeys::shiftModifier);
        break;
    case FileSaveAsDefault:
        result.setInfo("Save As Default", "Saves the current pedalboard file as the default file to load.",
                       fileCategory, 0);
        break;
    case FileResetDefault:
        result.setInfo("Reset Default", "Resets the default pedalboard file to its original state.", fileCategory, 0);
        break;
    case FileExit:
        result.setInfo("Exit", "Quits the program.", fileCategory, 0);
        break;
    case EditDeleteConnection:
        result.setInfo("Delete selected connection(s)", "Deletes the selected connection(s).", editCategory, 0);
        result.addDefaultKeypress(KeyPress::deleteKey, ModifierKeys());
        result.addDefaultKeypress(KeyPress::backspaceKey, ModifierKeys());
        break;
    case EditOrganisePatches:
        result.setInfo("Organise patches", "Opens the patch organiser.", editCategory, 0);
        break;
    case EditUserPresetManagement:
        result.setInfo("User Preset Management", "Opens the user preset managemet window.", editCategory, 0);
        break;
    case EditUndo:
        result.setInfo("Undo", "Undoes the last action.", editCategory, 0);
        result.addDefaultKeypress(L'z', ModifierKeys::commandModifier);
        break;
    case EditRedo:
        result.setInfo("Redo", "Redoes the previously undone action.", editCategory, 0);
        result.addDefaultKeypress(L'y', ModifierKeys::commandModifier);
        result.addDefaultKeypress(L'z', ModifierKeys::commandModifier | ModifierKeys::shiftModifier);
        break;
    case EditPanic:
        result.setInfo("Panic (All Notes Off)", "Sends All Notes Off on all MIDI channels.", editCategory, 0);
        break;
    case OptionsPreferences:
        result.setInfo("Misc Settings", "Displays miscellaneous settings.", optionsCategory, 0);
        break;
    case OptionsAudio:
        result.setInfo("Audio Settings", "Displays soundcard settings.", optionsCategory, 0);
        break;
    case OptionsPluginList:
        result.setInfo("Plugin List", "Options to scan and remove plugins.", optionsCategory, 0);
        break;
    case OptionsColourSchemes:
        result.setInfo("Colour Schemes", "Load and edit alternate colour schemes.", optionsCategory, 0);
        break;
    case OptionsKeyMappings:
        result.setInfo("Application Mappings", "Change the application mappings.", optionsCategory, 0);
        break;
    case HelpDocumentation:
        result.setInfo("Documentation", "Loads the documentation in your default browser.", helpCategory, 0);
        result.addDefaultKeypress(KeyPress::F1Key, ModifierKeys());
        break;
    case HelpLog:
        result.setInfo("Event Log", "Displays an event log for the program.", helpCategory, 0);
        break;
    case HelpAbout:
        result.setInfo("About", "Shows some details about the program.", helpCategory, 0);
        break;
    case PatchNextPatch:
        result.setInfo("Next Patch", "Switches to the next patch.", patchCategory, 0);
        break;
    case PatchPrevPatch:
        result.setInfo("Previous Patch", "Switches to the previous patch.", patchCategory, 0);
        break;
    case TransportPlay:
        result.setInfo("Play/Pause", "Plays/pauses the main transport.", transportCategory, 0);
        result.addDefaultKeypress(KeyPress::spaceKey, ModifierKeys());
        break;
    case TransportRtz:
        result.setInfo("Return to Zero", "Returns the main transport to the zero position.", transportCategory, 0);
        break;
    case TransportTapTempo:
        result.setInfo("Tap Tempo", "Used to set the tempo by 'tapping'.", transportCategory, 0);
        break;
    case ToggleStageMode:
        result.setInfo("Toggle Stage Mode", "Fullscreen performance view with large fonts.", optionsCategory, 0);
        result.addDefaultKeypress(KeyPress::F11Key, ModifierKeys());
        break;
    case OptionsPluginBlacklist:
        result.setInfo("Plugin Blacklist", "Manage blacklisted plugins that will not be loaded.", optionsCategory, 0);
        break;
    }
}

//------------------------------------------------------------------------------
bool MainPanel::perform(const InvocationInfo& info)
{
    int i;
    PluginField* field = (PluginField*)viewport->getViewedComponent();

    switch (info.commandID)
    {
    case FileNew:
    {
        File defaultFile = JuceHelperStuff::getAppDataFolder().getChildFile("default.pdl");

        // Delete all the patches.
        for (i = 0; i < patches.size(); ++i)
            delete patches[i];
        patches.clear();

        // Clear the PluginField.
        if (defaultFile.existsAsFile())
            loadDocument(defaultFile);
        else
        {
            field->clear();

            // Load the default patch into patches.
            patches.add(field->getXml());

            patchComboBox->clear(true);
            patchComboBox->addItem("1 - <untitled>", 1);
            patchComboBox->addItem("<new patch>", 2);
            patchComboBox->setSelectedId(1, true);
            currentPatch = 0;

            refreshPluginPoolDefinitions();
            PluginPoolManager::getInstance().setCurrentPosition(currentPatch);

            changed();

            int temp;

            temp = patches.size();

            field->clearDoubleClickMessage();
        }
    }
    break;
    case FileOpen:
        loadFromUserSpecifiedFile(true);
        field->clearDoubleClickMessage();
        showToast("Loaded");
        break;
    case FileSave:
        save(true, true);
        showToast("Saved");
        break;
    case FileSaveAs:
        saveAsInteractive(true);
        showToast("Saved");
        break;
    case FileSaveAsDefault:
    {
        File defaultFile = JuceHelperStuff::getAppDataFolder().getChildFile("default.pdl");

        saveDocument(defaultFile);
        showToast("Default saved");
    }
    break;
    case FileResetDefault:
    {
        File defaultFile = JuceHelperStuff::getAppDataFolder().getChildFile("default.pdl");

        if (defaultFile.existsAsFile())
            defaultFile.deleteFile();
    }
    break;
    case FileExit:
        ((App*)JUCEApplication::getInstance())->getWindow()->closeButtonPressed();
        break;
    case EditDeleteConnection:
        field->deleteConnection();
        changed();
        break;
    case EditOrganisePatches:
        // Save the current patch.
        {
            XmlElement* patch = field->getXml();

            patch->setAttribute("name", patchComboBox->getItemText(lastCombo - 1));

            delete patches[currentPatch];
            patches.set(currentPatch, patch);
        }
        // Open the organiser.
        {
            PatchOrganiser patchOrganiser(this, patches);

            patchOrganiser.setSize(400, 300);

            JuceHelperStuff::showModalDialog("Patch Organiser", &patchOrganiser, 0,
                                             ColourScheme::getInstance().colours["Window Background"], true, true);
        }
        refreshPluginPoolDefinitions();
        PluginPoolManager::getInstance().setCurrentPosition(currentPatch);
        break;
    case EditUserPresetManagement:
        // Open the preset window.
        {
            UserPresetWindow win(&pluginList);

            win.setSize(400, 300);

            JuceHelperStuff::showModalDialog("User Preset Management", &win, 0,
                                             ColourScheme::getInstance().colours["Window Background"], true, true);
        }
        break;
    case OptionsPreferences:
    {
        String tempstr;
        tempstr << sock.getPort();
        PreferencesDialog dlg(this, tempstr, sock.getMulticastGroup().c_str());

        JuceHelperStuff::showModalDialog("Misc Settings", &dlg, 0,
                                         ColourScheme::getInstance().colours["Window Background"], true, true);
    }
    break;
    case OptionsAudio:
    {
        savePatch();

        {
            AudioDeviceSelectorComponent win(deviceManager, 1, 16, 1, 16, true, false, false, false);
            win.setSize(380, 400);
            JuceHelperStuff::showModalDialog("Audio Settings", &win, 0,
                                             ColourScheme::getInstance().colours["Window Background"], true, true);
        }

        // NOTE: We intentionally do NOT call switchPatch here - the patch is already
        // loaded and reloading causes crashes with some plugins (e.g., Frohmager).

        std::unique_ptr<XmlElement> audioState = deviceManager.createStateXml();
        if (audioState)
        {
            SettingsManager::getInstance().setValue("audioDeviceState", audioState->toString());
            SettingsManager::getInstance().save();
        }
    }
    break;
    case OptionsPluginList:
        if (!listWindow)
        {
            listWindow = new PluginListWindow(pluginList, this);
            listWindow->toFront(true);
        }
        break;
    case OptionsColourSchemes:
    {
        ColourSchemeEditor* dlg = new ColourSchemeEditor();

        dlg->setSize(500, 375);
        dlg->addChangeListener(this);

        JuceHelperStuff::showNonModalDialog("Colour Schemes", dlg, 0,
                                            ColourScheme::getInstance().colours["Window Background"], true, true);
    }
    break;
    case OptionsKeyMappings:
    {
        StupidWindow* win = (StupidWindow*)getParentComponent();
        ApplicationMappingsEditor editor(win->getAppManager(), field->getMidiManager(), field->getOscManager());

        editor.setSize(414, 524);
        JuceHelperStuff::showModalDialog("Application Mappings", &editor, this,
                                         ColourScheme::getInstance().colours["Window Background"], false, true);
    }
    break;
    case HelpAbout:
    {
        AboutPage dlg(String(sock.getIpAddress().c_str()));

        dlg.setSize(400, 250);

        JuceHelperStuff::showModalDialog("About", &dlg, 0, ColourScheme::getInstance().colours["Window Background"],
                                         true, true);
    }
    break;
    case HelpDocumentation:
    {
#ifdef WIN32
        File docDir(
            File::getSpecialLocation(File::currentApplicationFile).getParentDirectory().getChildFile("documentation"));
#elif defined(LINUX)

#elif defined(__APPLE__)
        File docDir(File::getSpecialLocation(File::currentApplicationFile)
                        .getChildFile("Contents")
                        .getChildFile("Resources")
                        .getChildFile("documentation"));
#endif
        File docIndex(docDir.getChildFile("index.htm"));

        if (docIndex.existsAsFile())
        {
            URL docUrl(docDir.getChildFile("index.htm").getFullPathName());

            docUrl.launchInDefaultBrowser();
        }
        else
        {
            AlertWindow::showMessageBox(AlertWindow::WarningIcon, "Documentation Missing",
                                        "Could not find documentation/index.htm");
        }
    }
    break;
    case HelpLog:
    {
        LogDisplay* dlg = new LogDisplay();

        dlg->setSize(600, 400);

        JuceHelperStuff::showNonModalDialog(
            L"Event Log", dlg, 0, ColourScheme::getInstance().colours["Window Background"], true, true, false, true);
    }
    break;
    case PatchNextPatch:
        if (patchComboBox->getSelectedItemIndex() < (patchComboBox->getNumItems() - 2))
            patchComboBox->setSelectedItemIndex(patchComboBox->getSelectedItemIndex() + 1);
        else if (SettingsManager::getInstance().getBool("LoopPatches", true))
            patchComboBox->setSelectedItemIndex(0);
        field->clearDoubleClickMessage();
        break;
    case PatchPrevPatch:
        if (patchComboBox->getSelectedItemIndex() > 0)
            patchComboBox->setSelectedItemIndex(patchComboBox->getSelectedItemIndex() - 1);
        else if (SettingsManager::getInstance().getBool("LoopPatches", true))
            patchComboBox->setSelectedItemIndex(patchComboBox->getNumItems() - 2);
        field->clearDoubleClickMessage();
        break;
    case TransportPlay:
        MainTransport::getInstance()->toggleState();
        break;
    case TransportRtz:
        MainTransport::getInstance()->setReturnToZero();
        break;
    case TransportTapTempo:
    {
        int64 delta;
        double tempo;
        double seconds;
        std::wstringstream converterString;
        int64 ticks = Time::getHighResolutionTicks();

        if (lastTempoTicks > 0)
        {
            delta = ticks - lastTempoTicks;

            seconds = Time::highResolutionTicksToSeconds(delta);
            if (seconds > 0.0)
            {
                tempo = (1.0f / seconds) * 60.0;
                field->setTempo(tempo);

                converterString.precision(2);
                converterString.fill(L'0');
                converterString << std::fixed << tempo;
                tempoEditor->setText(converterString.str().c_str(), false);
            }
        }
        lastTempoTicks = ticks;
    }
    break;
    case EditUndo:
        signalPath.getUndoManager().undo();
        field->syncWithGraph();
        showToast("Undone");
        break;
    case EditRedo:
        signalPath.getUndoManager().redo();
        field->syncWithGraph();
        showToast("Redone");
        break;
    case EditPanic:
    {
        // Send All Notes Off (CC 123) and All Sound Off (CC 120) on all channels
        MidiMessageCollector& midiCollector = graphPlayer.getMidiMessageCollector();
        for (int channel = 1; channel <= 16; ++channel)
        {
            midiCollector.addMessageToQueue(MidiMessage::allNotesOff(channel));
            midiCollector.addMessageToQueue(MidiMessage::allSoundOff(channel));
        }

        // Unmute the safety limiter if it was auto-muted
        if (auto* limiter = signalPath.getSafetyLimiter())
            limiter->unmute();

        showToast("Panic sent");
    }
    break;
    case ToggleStageMode:
        toggleStageMode();
        break;
    case OptionsPluginBlacklist:
        BlacklistWindow::showWindow();
        break;
    }
    return true;
}

//------------------------------------------------------------------------------
void MainPanel::setCommandManager(ApplicationCommandManager* manager)
{
    commandManager = manager;
}

//------------------------------------------------------------------------------
void MainPanel::invokeCommandFromOtherThread(CommandID commandID)
{
    midiAppFifo.writeID(commandID);
}

//------------------------------------------------------------------------------
void MainPanel::updateTempoFromOtherThread(double tempo)
{
    midiAppFifo.writeTempo(tempo);
}

//------------------------------------------------------------------------------
void MainPanel::switchPatch(int newPatch, bool savePrev, bool reloadPatch)
{
    if (doNotSaveNextPatch)
    {
        savePrev = false;
        doNotSaveNextPatch = false;
    }

    if (((newPatch != currentPatch) && !reloadPatch) || !savePrev)
    {
        PluginField* field = ((PluginField*)viewport->getViewedComponent());
        XmlElement* patch = 0;

        if (savePrev)
        {
            patch = field->getXml();

            patch->setAttribute("name", patchComboBox->getItemText(lastCombo - 1));
        }

        if ((newPatch > -1) && (newPatch < patches.size()))
        {
            // Save current patch.
            if (patch)
            {
                delete patches[currentPatch];
                patches.set(currentPatch, patch);
                updatePluginPoolDefinition(currentPatch, patch);
            }

            // Load new patch if it exists.
            currentPatch = newPatch;
            programChangePatch = currentPatch;
            patch = patches[currentPatch];
            if (patch)
            {
                // patchComboBox->setText(patch->getStringAttribute("name"), true);
                // field->loadFromXml(patch->getChildByName("FILTERGRAPH"));
                field->loadFromXml(patch);
                field->clearDoubleClickMessage();

                tempoEditor->setText(String(field->getTempo(), 2), false);
            }
            else
            {
                String tempstr;

                field->clear();
                patch = field->getXml();

                tempstr << (currentPatch + 1) << " - <untitled>";
                patch->setAttribute("name", tempstr);

                patches.set(currentPatch, patch);

                tempoEditor->setText("120.00");
            }
            lastTempoTicks = 0;
        }

        // Update Stage View
        updateStageView();
    }

    PluginPoolManager::getInstance().setCurrentPosition(currentPatch);
}

//------------------------------------------------------------------------------
void MainPanel::timerCallback(int timerId)
{
    switch (timerId)
    {
    case CpuTimer:
        cpuSlider->setColour(Slider::thumbColourId, ColourScheme::getInstance().colours["CPU Meter Colour"]);
        cpuSlider->setValue(deviceManager.getCpuUsage());

        // Check for safety limiter mute condition
        if (auto* limiter = signalPath.getSafetyLimiter())
        {
            if (limiter->checkAndClearMuteTriggered())
            {
                showToast("OUTPUT MUTED - Use Panic to unmute");
            }
        }

        /*if(programChangePatch != currentPatch)
        {
                if((programChangePatch < patches.size()) && (programChangePatch >
        -1)) patchComboBox->setSelectedId(programChangePatch+1); programChangePatch
        = currentPatch;
        }*/
        break;
    case MidiAppTimer:
        if (midiAppFifo.getNumWaitingID() > 0)
            commandManager->invokeDirectly(midiAppFifo.readID(), true);
        if (midiAppFifo.getNumWaitingTempo() > 0)
        {
            stringstream converterString;
            double tempo = midiAppFifo.readTempo();
            PluginField* field = (PluginField*)viewport->getViewedComponent();

            Logger::writeToLog(String(tempo));

            field->setTempo(tempo);

            converterString.precision(2);
            converterString.fill(L'0');
            converterString << std::fixed << tempo;
            tempoEditor->setText(converterString.str().c_str(), false);
        }
        if (midiAppFifo.getNumWaitingPatchChange() > 0)
        {
            int index = midiAppFifo.readPatchChange();

            if ((index > -1) && (index < patches.size()))
            {
                patchComboBox->setSelectedItemIndex(index);

                if (warningBox->isVisible())
                    warningBox->setVisible(false);
            }
            else
            {
                warningText.setIndex(index);
                if (!warningBox->isVisible())
                    warningBox->setVisible(true);
                else
                    warningBox->repaint();
                startTimer(ProgramChangeTimer, 5 * 1000); // 5 seconds.
            }
        }
        break;
    case ProgramChangeTimer:
        warningBox->setVisible(false);
        stopTimer(ProgramChangeTimer);
        break;
    }
}

//------------------------------------------------------------------------------
void MainPanel::changeListenerCallback(ChangeBroadcaster* changedObject)
{
    ColourSchemeEditor* ed = dynamic_cast<ColourSchemeEditor*>(changedObject);

    if (changedObject == &deviceManager)
    {
        // Audio device changed - update graph channel counts
        if (auto* device = deviceManager.getCurrentAudioDevice())
        {
            auto activeInputs = device->getActiveInputChannels();
            auto activeOutputs = device->getActiveOutputChannels();
            int numInputs = activeInputs.countNumberOfSetBits();
            int numOutputs = activeOutputs.countNumberOfSetBits();
            signalPath.setDeviceChannelCounts(numInputs, numOutputs);

            // Refresh the UI to show updated channel pins
            if (auto* field = dynamic_cast<PluginField*>(viewport->getViewedComponent()))
                field->refreshAudioIOPins();
        }
    }
    else if (changedObject == MainTransport::getInstance())
    {
        if (MainTransport::getInstance()->getState())
            playButton->setImages(pauseImage.get());
        else
            playButton->setImages(playImage.get());

        // To decrement the counter.
        MainTransport::getInstance()->getReturnToZero();
    }
    else if (changedObject == dynamic_cast<PluginField*>(viewport->getViewedComponent()))
        changed();
    else if (ed) // The colour scheme editor's updated our colour scheme.
    {
        // Refresh LookAndFeel colors
        if (auto* laf = dynamic_cast<BranchesLAF*>(&LookAndFeel::getDefaultLookAndFeel()))
            laf->refreshColours();

        // Repaint the entire component tree
        if (auto* topLevel = getTopLevelComponent())
            topLevel->repaint();
        else
            repaint();

        // Also update any visible windows (plugin editors, dialogs, etc.)
        for (int i = Desktop::getInstance().getNumComponents(); --i >= 0;)
            if (auto* comp = Desktop::getInstance().getComponent(i))
                comp->repaint();
    }
    else
    {
        // Save the plugin list every time it gets changed, so that if we're
        // scanning and it crashes, we've still saved the previous ones
        auto savedPluginList = // JUCE 8: returns unique_ptr
            pluginList.createXml();

        if (savedPluginList != nullptr)
        {
            try
            {
                SettingsManager::getInstance().setValue("pluginList", savedPluginList->toString());
                // Note: setValue already calls save() internally
            }
            catch (const std::exception& e)
            {
                juce::Logger::writeToLog("Error saving plugin list: " + juce::String(e.what()));
            }
        }
    }
}

//------------------------------------------------------------------------------
void MainPanel::textEditorTextChanged(TextEditor& editor)
{
    if (&editor == tempoEditor)
    {
        PluginField* field = dynamic_cast<PluginField*>(viewport->getViewedComponent());

        if (field)
            field->setTempo(tempoEditor->getText().getDoubleValue());
    }
}

//------------------------------------------------------------------------------
void MainPanel::textEditorReturnKeyPressed(TextEditor& editor)
{
    if (&editor == tempoEditor)
    {
        PluginField* field = dynamic_cast<PluginField*>(viewport->getViewedComponent());

        if (field)
            field->setTempo(tempoEditor->getText().getDoubleValue());
    }
    playButton->grabKeyboardFocus();
}

//------------------------------------------------------------------------------
bool MainPanel::isInterestedInFileDrag(const StringArray& files)
{
    int i;
    bool retval = false;

    for (i = 0; i < files.size(); ++i)
    {
        if (files[i].endsWith(".pdl"))
        {
            retval = true;
            break;
        }
    }

    return retval;
}

//------------------------------------------------------------------------------
void MainPanel::filesDropped(const StringArray& files, int x, int y)
{
    int i;

    for (i = 0; i < files.size(); ++i)
    {
        if (files[i].endsWith(".pdl"))
        {
            File phil(files[i]);

            if (phil.existsAsFile())
                loadDocument(phil);
            else
            {
                String tempstr;

                tempstr << "Could not locate file: " << files[i];
                AlertWindow::showMessageBox(AlertWindow::WarningIcon, "File error", tempstr);
            }
        }
    }
}

//------------------------------------------------------------------------------
void MainPanel::setSocketPort(const String& port)
{
    int16_t tempVal;
    ScopedLock lock(sockCritSec);

    tempVal = (int16_t)port.getIntValue();
    sock.setPort(tempVal);
    sock.bindSocket();

    SettingsManager::getInstance().setValue("OSCPort", port);
}

//------------------------------------------------------------------------------
void MainPanel::setSocketMulticast(const String& address)
{
    ScopedLock lock(sockCritSec);

    sock.setMulticastGroup(std::string(address.toUTF8()));
    sock.bindSocket();

    SettingsManager::getInstance().setValue("OSCMulticastAddress", address);
}

//------------------------------------------------------------------------------
void MainPanel::enableAudioInput(bool val)
{
    PluginField* field = dynamic_cast<PluginField*>(viewport->getViewedComponent());

    field->enableAudioInput(val);

    SettingsManager::getInstance().setValue("AudioInput", val);
}

//------------------------------------------------------------------------------
void MainPanel::enableMidiInput(bool val)
{
    PluginField* field = dynamic_cast<PluginField*>(viewport->getViewedComponent());

    field->enableMidiInput(val);

    SettingsManager::getInstance().setValue("MidiInput", val);
}

//------------------------------------------------------------------------------
void MainPanel::enableOscInput(bool val)
{
    PluginField* field = dynamic_cast<PluginField*>(viewport->getViewedComponent());

    field->enableOscInput(val);

    // If there's no OSC input, we don't need to run the OSC thread.
    if (!val && isThreadRunning())
        signalThreadShouldExit();
    else if (val && !isThreadRunning())
    {
        String port, address;

        port = SettingsManager::getInstance().getString("OSCPort");
        if (port == "")
            port = "5678";
        address = SettingsManager::getInstance().getString("OSCMulticastAddress");

        sock.setPort((int16_t)port.getIntValue());
        sock.setMulticastGroup(std::string(address.toUTF8()));
        sock.bindSocket();
        startThread();
    }

    SettingsManager::getInstance().setValue("OscInput", val);
}

//------------------------------------------------------------------------------
void MainPanel::setAutoMappingsWindow(bool val)
{
    PluginField* field = dynamic_cast<PluginField*>(viewport->getViewedComponent());

    field->setAutoMappingsWindow(val);

    SettingsManager::getInstance().setValue("AutoMappingsWindow", val);
}

//------------------------------------------------------------------------------
void MainPanel::run()
{
    char* data;
    int32_t dataSize;
    PluginField* field = dynamic_cast<PluginField*>(viewport->getViewedComponent());

    while (!threadShouldExit())
    {
        {
            ScopedLock lock(sockCritSec);

            data = sock.getData(dataSize);
        }

        if (field && (dataSize > 0))
            field->socketDataArrived(data, dataSize);
    }
}

//------------------------------------------------------------------------------
String MainPanel::getDocumentTitle()
{
    return "Pedalboard3 Patch File";
}

//------------------------------------------------------------------------------
Result MainPanel::loadDocument(const File& file)
{
    int i;
    XmlDocument doc(file);
    std::unique_ptr<XmlElement> root = doc.getDocumentElement(); // JUCE 8: unique_ptr
    // XmlElement *patch;

    if (root)
    {
        if (root->hasTagName("Pedalboard3PatchFile"))
        {
            // Clear existing patches.
            for (i = 0; i < patches.size(); ++i)
                delete patches[i];
            patches.clear();

            // Clear patchComboBox.
            patchComboBox->clear(); // JUCE 8: clear() takes no args (notification handled
                                    // elsewhere usually) or explicit notificationType
            /*patchComboBox->addItem("1 - <untitled>", 1);
            patchComboBox->addItem("<new patch>", 2);
            patchComboBox->setSelectedId(1);*/

            // If there are audio settings saved in this file and
            // pdlAudioSettings is set, load them.
            if (SettingsManager::getInstance().getBool("pdlAudioSettings"))
            {
                XmlElement* deviceXml = root->getChildByName("DEVICESETUP");

                if (deviceXml)
                {
                    String err;

                    // Support up to 16 input/output channels for multi-channel interfaces
                    err = deviceManager.initialise(16, 16, deviceXml, true);

                    if (err != "")
                    {
                        AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon, "Audio Device Error",
                                                         "Could not initialise audio settings loaded from .pdl file");
                        showToast("Audio error!");
                    }
                    else
                    {
                        // Update graph bus layout to match device channels
                        if (auto* device = deviceManager.getCurrentAudioDevice())
                        {
                            auto activeInputs = device->getActiveInputChannels();
                            auto activeOutputs = device->getActiveOutputChannels();
                            int numInputs = activeInputs.countNumberOfSetBits();
                            int numOutputs = activeOutputs.countNumberOfSetBits();
                            signalPath.setDeviceChannelCounts(numInputs, numOutputs);
                        }
                    }
                }
            }

            // Load any xml patches into patches.
            forEachXmlChildElement(*root, patch)
            {
                if (patch->getTagName() == "Patch")
                    patches.add(patch);
            }

            // Remove xml patches from root, so they don't get deleted.
            // JUCE 8: root is unique_ptr, so we must be careful.
            // patches.add(patch) usually takes ownership if patches is OwnedArray?
            // Let's check patches declaration. Assuming it is valid to take pointers.
            // If we remove them from root, root won't delete them.
            for (i = (root->getNumChildElements() - 1); i >= 0; --i)
            {
                if (root->getChildElement(i)->getTagName() == "Patch")
                    root->removeChildElement(root->getChildElement(i), false);
            }

            // Delete root.
            // delete root; // JUCE 8: unique_ptr auto-deleted

            refreshPluginPoolDefinitions();

            // Load the current patch.
            switchPatch(0, false);

            // Fill out patchComboBox.
            for (i = 0; i < patches.size(); ++i)
                patchComboBox->addItem(patches[i]->getStringAttribute("name"), i + 1);
            patchComboBox->addItem("<new patch>", patches.size() + 1);
            patchComboBox->setSelectedId(1, true); // deprecated but works

            if (auto* win = dynamic_cast<StupidWindow*>(getParentComponent()))
                win->updateWindowTitle(file.getFileName());
        }

        // Update Stage View if active
        updateStageView();
    }

    return Result::ok();
}

//------------------------------------------------------------------------------
Result MainPanel::saveDocument(const File& file)
{
    int i;
    PluginField* field = ((PluginField*)viewport->getViewedComponent());
    XmlElement* main = new XmlElement("Pedalboard3PatchFile");
    XmlElement* patch = field->getXml();

    // Save the current patch.
    {
        patch->setAttribute("name", patchComboBox->getText());

        delete patches[currentPatch];
        patches.set(currentPatch, patch);
        updatePluginPoolDefinition(currentPatch, patch);
    }

    for (i = 0; i < patches.size(); ++i)
        main->addChildElement(patches[i]);

    if (SettingsManager::getInstance().getBool("pdlAudioSettings"))
        main->addChildElement(deviceManager.createStateXml().release());

    main->writeToFile(file, "");

    // Remove the child "Patch" elements so they don't get deleted.
    for ((i = main->getNumChildElements() - 1); i >= 0; --i)
        main->removeChildElement(main->getChildElement(i), false);

    delete main;

    return Result::ok();
}

//------------------------------------------------------------------------------
File MainPanel::getLastDocumentOpened()
{
    return lastDocument;
}

//------------------------------------------------------------------------------
void MainPanel::setLastDocumentOpened(const File& file)
{
    lastDocument = file;
}

//------------------------------------------------------------------------------
void MainPanel::addPatch(XmlElement* patch)
{
    patches.add(patch);

    updatePluginPoolDefinition(patches.size() - 1, patch);

    patchComboBox->changeItemText(patchComboBox->getNumItems(), patch->getStringAttribute("name"));
    patchComboBox->addItem("<new patch>", patchComboBox->getNumItems() + 1);

    changed();
}

//------------------------------------------------------------------------------
void MainPanel::savePatch()
{
    XmlElement* patch = 0;
    PluginField* field = ((PluginField*)viewport->getViewedComponent());

    // Save current patch.
    patch = field->getXml();
    patch = field->getXml();
    patch->setAttribute("name", patchComboBox->getItemText(lastCombo - 1));

    // Update Stage View if open
    updateStageView();

    delete patches[currentPatch];
    patches.set(currentPatch, patch);

    updatePluginPoolDefinition(currentPatch, patch);
}

//------------------------------------------------------------------------------
void MainPanel::updateStageView()
{
    if (stageView != nullptr)
    {
        String currentName = getCurrentPatchName();
        String nextName = "";

        // Safety check for patching index
        if (currentPatch >= 0 && currentPatch < patches.size())
        {
            // Get current name from array to be sure
            currentName = patches[currentPatch]->getStringAttribute("name");

            // Get next patch if available
            if (currentPatch + 1 < patches.size())
            {
                nextName = patches[currentPatch + 1]->getStringAttribute("name");
            }
        }

        stageView->updatePatchInfo(currentName, nextName, currentPatch, patches.size());
    }
}

//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
void MainPanel::duplicatePatch(int index)
{
    String tempstr;
    XmlElement* patch;
    // PluginField *field = ((PluginField *)viewport->getViewedComponent());

    jassert((index > -1) && (index < patches.size()));

    // Save current patch.
    savePatch();

    // Setup the new ComboBox stuff.
    tempstr << patches[index]->getStringAttribute("name") << " (copy)";
    patchComboBox->changeItemText(patchComboBox->getNumItems(), tempstr);
    patchComboBox->addItem("<new patch>", patchComboBox->getNumItems() + 1);

    // Copy the current patch to the new one.
    patch = new XmlElement(*patches[index]);
    patch->setAttribute("name", tempstr);
    patches.set(patches.size(), patch);

    updatePluginPoolDefinition(patches.size() - 1, patch);

    changed();
}

//------------------------------------------------------------------------------
void MainPanel::nextSwitchDoNotSavePrev()
{
    doNotSaveNextPatch = true;
}

//------------------------------------------------------------------------------
void MainPanel::switchPatchFromProgramChange(int newPatch)
{
    // programChangePatch = newPatch;
    /*if(newPatch > currentPatch)
            midiAppFifo.writeID(PatchNextPatch);
    else if(newPatch < currentPatch)
            midiAppFifo.writeID(PatchPrevPatch);*/
    midiAppFifo.writePatchChange(newPatch);
}

//------------------------------------------------------------------------------
/*void MainPanel::logMessage(const String &message)
{
        CharPointer_UTF8 temp = message.toUTF8();
        String endline = "\n";

        outFile.write(temp.getAddress(), temp.length());
        outFile.write(endline.toUTF8().getAddress(), endline.toUTF8().length());
}*/

//==============================================================================
// Stage Mode methods
//==============================================================================

void MainPanel::toggleStageMode()
{
    if (stageView != nullptr)
    {
        // Exit Stage Mode
        removeChildComponent(stageView.get());
        stageView.reset();

        // Disable global tuner
        deviceManager.removeAudioCallback(&tunerPlayer);
        tunerPlayer.setProcessor(nullptr);

        activeTuner = nullptr; // Clear reference
        grabKeyboardFocus();   // Ensure MainPanel gets focus back
        DBG("Stage Mode disabled");
    }
    else
    {
        // Enter Stage Mode

        // Ensure global tuner exists
        if (globalTuner == nullptr)
            globalTuner = std::make_unique<TunerProcessor>();

        // Configure global tuner for silent monitoring
        globalTuner->setMuteOutput(true);
        tunerPlayer.setProcessor(globalTuner.get());

        // Add to device manager to receive input audio independent of graph
        deviceManager.addAudioCallback(&tunerPlayer);

        activeTuner = globalTuner.get();
        DBG("Global Tuner activated (parallel monitoring)");

        stageView = std::make_unique<StageView>(this);
        addAndMakeVisible(stageView.get());
        stageView->setBounds(getLocalBounds());
        stageView->setTunerProcessor(activeTuner);
        updateStageView();
        stageView->toFront(true); // Bring to front and grab keyboard focus
        DBG("Stage Mode enabled");
    }
}

bool MainPanel::keyPressed(const KeyPress& key)
{
    // Manually handle F11 if the command manager misses it
    if (key == KeyPress::F11Key)
    {
        toggleStageMode();
        return true;
    }
    return false;
}

String MainPanel::getCurrentPatchName() const
{
    if (patchComboBox != nullptr)
        return patchComboBox->getText();
    return "No Patch";
}

int MainPanel::getPatchCount() const
{
    return patches.size();
}

//[/MiscUserCode]

//==============================================================================
#if 0
/*  -- Jucer information section --

    This is where the Jucer puts all of its metadata, so don't change anything in here!

BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="MainPane" componentName=""
                 parentClasses="public Component, public MenuBarModel, public ApplicationCommandTarget, public MultiTimer, public ChangeListener, public FileBasedDocument, public Thread, public FileDragAndDropTarget, public TextEditor::Listener"
                 constructorParams="ApplicationCommandManager *appManager" variableInitialisers="FileBasedDocument(&quot;.pdl&quot;, &quot;*.pdl&quot;, &quot;Choose a set of patches to open...&quot;, &quot;Choose a set of patches to save as...&quot;),&#10;Thread(&quot;OSC Thread&quot;), commandManager(appManager), currentPatch(0)&#10;"
                 snapPixels="8" snapActive="1" snapShown="1" overlayOpacity="0.330000013"
                 fixedSize="0" initialWidth="1024" initialHeight="570">
  <BACKGROUND backgroundColour="ffeeece1"/>
  <LABEL name="patchLabe" id="7487f2115f5ed988" memberName="patchLabe"
         virtualName="" explicitFocusOrder="0" pos="8 33R 48 24" edTextCol="ff000000"
         edBkgCol="0" labelText="Patch:" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="0" fontname="Default font" fontsize="15"
         bold="0" italic="0" justification="33"/>
  <TEXTBUTTON name="prevPatch" id="342ff0e9a494b29" memberName="prevPatch"
              virtualName="" explicitFocusOrder="0" pos="264 33R 24 24" buttonText="-"
              connectedEdges="2" needsCallback="1" radioGroupId="0"/>
  <TEXTBUTTON name="nextPatch" id="6247f6fccfbcb165" memberName="nextPatch"
              virtualName="" explicitFocusOrder="0" pos="288 33R 24 24" buttonText="+"
              connectedEdges="1" needsCallback="1" radioGroupId="0"/>
  <COMBOBOX name="patchComboBox" id="20bec1c831d2831c" memberName="patchComboBox"
            virtualName="" explicitFocusOrder="0" pos="56 33R 200 24" editable="1"
            layout="33" items="1 - &lt;untitled&gt;&#10;&lt;new patch&gt;"
            textWhenNonSelected="" textWhenNoItems="(no choices)"/>
  <VIEWPORT name="new viewport" id="17841313120b1834" memberName="viewport"
            virtualName="" explicitFocusOrder="0" pos="0 0 0M 40M" vscroll="1"
            hscroll="1" scrollbarThickness="18" contentType="0" jucerFile=""
            contentClass="" constructorParams=""/>
  <SLIDER name="cpuSlider" id="49855b028c510925" memberName="cpuSlider"
          virtualName="" explicitFocusOrder="0" pos="156R 33R 150 24" min="0"
          max="1" int="0" style="LinearBar" textBoxPos="NoTextBox" textBoxEditable="0"
          textBoxWidth="80" textBoxHeight="20" skewFactor="1"/>
  <LABEL name="cpuLabe" id="896921bd35cf3005" memberName="cpuLabe" virtualName=""
         explicitFocusOrder="0" pos="236R 33R 78 24" edTextCol="ff000000"
         edBkgCol="0" labelText="CPU Usage:" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="0" fontname="Default font" fontsize="15"
         bold="0" italic="0" justification="33"/>
  <GENERICCOMPONENT name="playButton" id="382190f9abb24dc2" memberName="playButton"
                    virtualName="" explicitFocusOrder="0" pos="50%c 38R 36 36" class="DrawableButton"
                    params="&quot;playButton&quot;, DrawableButton::ImageOnButtonBackground"/>
  <GENERICCOMPONENT name="rtzButton" id="22f2164788c3f1be" memberName="rtzButton"
                    virtualName="" explicitFocusOrder="0" pos="38 32R 24 24" posRelativeX="382190f9abb24dc2"
                    class="DrawableButton" params="&quot;rtzButton&quot;, DrawableButton::ImageOnButtonBackground"/>
  <LABEL name="tempoLabe" id="b6ca8b83aba988fd" memberName="tempoLabe"
         virtualName="" explicitFocusOrder="0" pos="-151 33R 64 24" posRelativeX="382190f9abb24dc2"
         edTextCol="ff000000" edBkgCol="0" labelText="Tempo:" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="15" bold="0" italic="0" justification="33"/>
  <TEXTEDITOR name="tempoEditor" id="17103462ab2d58d1" memberName="tempoEditor"
              virtualName="" explicitFocusOrder="0" pos="-87 33R 52 24" posRelativeX="382190f9abb24dc2"
              initialText="120.00" multiline="0" retKeyStartsLine="0" readonly="0"
              scrollbars="1" caret="1" popupmenu="1"/>
  <GENERICCOMPONENT name="tapTempoButton" id="2fec60b9d3555246" memberName="tapTempoButton"
                    virtualName="" explicitFocusOrder="0" pos="-31 27R 10 16" posRelativeX="382190f9abb24dc2"
                    class="ArrowButton" params="L&quot;tapTempoButton&quot;, 0.0, Colour(0x40000000)"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif
