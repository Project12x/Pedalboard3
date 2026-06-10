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
#include "BypassableInstance.h"
#include "ColourSchemeEditor.h"
#include "CrashProtection.h"
#include "DawMixerProcessor.h"
#include "DawSplitterProcessor.h"
#include "FontManager.h"
#include "IRLoaderProcessor.h"
#include "Images.h"
#include "JuceHelperStuff.h"
#include "LabelProcessor.h"
#include "LogDisplay.h"
#include "LogFile.h"
#include "MainTransport.h"
#include "Mapping.h"
#include "MasterGainState.h"
#include "MidiFilePlayer.h"
#include "MidiUtilityProcessors.h"
#include "NAMControl.h"
#include "NAMModelBrowser.h"
#include "NAMProcessor.h"
#include "NotesProcessor.h"
#include "OscilloscopeProcessor.h"
#include "PatchOrganiser.h"
#include "PedalboardProcessors.h"
#include "PluginField.h"
#include "PluginPoolManager.h"
#include "PreferencesDialog.h"
#include "RoutingProcessors.h"
#include "SafePluginScanner.h"
#include "SettingsManager.h"
#include "ScratchPanel.h"
#include "ScratchPanelPresentation.h"
#include "StageView.h"
#include "SubGraphEditorComponent.h"
#include "TapTempoBox.h"
#include "ThemeSwitcherComponent.h"
#include "ToastOverlay.h"
#include "ToneGeneratorProcessor.h"
#include "TunerProcessor.h"
#include "UiScale.h"
#include "UserPresetWindow.h"
#include "Vectors.h"
#include "VirtualMidiInputProcessor.h"

#include <iostream>
#include <sstream>

using namespace std;
//[/Headers]

#include "MainPanel.h"

//[MiscUserDefs] You can add your own user definitions and misc code here...

namespace
{
constexpr const char* kScratchRootSettingsKey = "scratchRootDirectory";
constexpr const char* kVirtualKeyboardCollapsedSettingsKey = "VirtualKeyboardCollapsed";
constexpr const char* kGraphGridStyleSettingsKey = "GraphGridStyle";
constexpr int kGraphGridDotsMenuId = 910001;
constexpr int kGraphGridLinesMenuId = 910002;
constexpr int kGraphGridOffMenuId = 910003;

String normaliseGraphGridStyle(String style)
{
    style = style.toLowerCase();
    if (style != "dots" && style != "lines" && style != "off")
        return "lines";
    return style;
}

class KeyboardDeckButtonLookAndFeel : public LookAndFeel_V4
{
  public:
    void drawButtonBackground(Graphics& g, Button& button, const Colour& /*backgroundColour*/, bool isMouseOverButton,
                              bool isButtonDown) override
    {
        auto& palette = ::ColourScheme::getInstance().colours;
        auto bounds = button.getLocalBounds().toFloat().reduced(1.0f, 1.0f);
        const auto active = button.getToggleState();
        auto accent = palette["Accent Colour"];
        auto base = active ? accent.withAlpha(0.44f) : palette["Button Colour"].withAlpha(0.88f);

        if (isButtonDown)
            bounds = bounds.translated(0.0f, 0.8f);
        if (isMouseOverButton)
            base = base.brighter(active ? 0.12f : 0.08f);

        g.setColour(palette["Window Background"].darker(0.55f).withAlpha(isButtonDown ? 0.12f : 0.26f));
        g.fillRoundedRectangle(bounds.translated(0.0f, isButtonDown ? 0.6f : 1.8f), 7.0f);

        ColourGradient fill(base.brighter(0.16f), bounds.getX(), bounds.getY(), base.darker(0.18f), bounds.getX(),
                            bounds.getBottom(), false);
        g.setGradientFill(fill);
        g.fillRoundedRectangle(bounds, 7.0f);

        g.setColour(palette["Text Colour"].withAlpha(active ? 0.18f : 0.08f));
        g.drawLine(bounds.getX() + 4.0f, bounds.getY() + 1.5f, bounds.getRight() - 4.0f, bounds.getY() + 1.5f, 1.0f);
        g.setColour((active ? accent : palette["Text Colour"]).withAlpha(active ? 0.78f : 0.22f));
        g.drawRoundedRectangle(bounds.reduced(0.5f), 7.0f, active ? 1.2f : 1.0f);
    }

    void drawButtonText(Graphics& g, TextButton& button, bool /*isMouseOverButton*/, bool /*isButtonDown*/) override
    {
        auto& palette = ::ColourScheme::getInstance().colours;
        g.setFont(::FontManager::getInstance().getBadgeFont());
        g.setColour(palette["Text Colour"].withAlpha(button.isEnabled() ? 0.96f : 0.35f));
        g.drawFittedText(button.getButtonText(), button.getLocalBounds().reduced(4, 1), Justification::centred, 1);
    }
};

class PedalboardMidiKeyboardComponent : public MidiKeyboardComponent
{
  public:
    PedalboardMidiKeyboardComponent(MidiKeyboardState& state, Orientation orientation)
        : MidiKeyboardComponent(state, orientation)
    {
        setBlackNoteLengthProportion(0.68f);
    }

    void drawWhiteNote(int midiNoteNumber, Graphics& g, Rectangle<float> area, bool isDown, bool isOver,
                       Colour /*lineColour*/, Colour /*textColour*/) override
    {
        auto& palette = ::ColourScheme::getInstance().colours;
        auto key = area.reduced(0.5f, 0.0f);
        auto top = palette["Dialog Inner Background"].brighter(0.74f);
        auto bottom = palette["Dialog Inner Background"].brighter(0.38f);

        if (isDown)
        {
            top = palette["Accent Colour"].withAlpha(0.82f);
            bottom = palette["Accent Colour"].darker(0.18f);
        }
        else if (isOver)
        {
            top = top.overlaidWith(palette["Accent Colour"].withAlpha(0.12f));
            bottom = bottom.overlaidWith(palette["Accent Colour"].withAlpha(0.08f));
        }

        ColourGradient fill(top, key.getX(), key.getY(), bottom, key.getX(), key.getBottom(), false);
        g.setGradientFill(fill);
        g.fillRoundedRectangle(key.withTrimmedTop(1.0f), 2.5f);

        g.setColour(palette["Window Background"].darker(0.55f).withAlpha(0.18f));
        g.drawLine(key.getX(), key.getY() + 1.0f, key.getX(), key.getBottom(), 1.0f);
        g.setColour(palette["Text Colour"].withAlpha(isDown ? 0.18f : 0.28f));
        g.drawLine(key.getX() + 1.5f, key.getY() + 2.0f, key.getRight() - 1.5f, key.getY() + 2.0f, 1.0f);

        if (midiNoteNumber % 12 == 0)
        {
            const auto octave = midiNoteNumber / 12 - 1;
            g.setColour((isDown ? palette["Text Colour"] : palette["Window Background"]).withAlpha(isDown ? 0.74f : 0.62f));
            g.setFont(::FontManager::getInstance().getCaptionFont());
            g.drawText("C" + String(octave), key.reduced(1.0f, 2.0f), Justification::centredBottom, true);
        }
    }

    void drawBlackNote(int /*midiNoteNumber*/, Graphics& g, Rectangle<float> area, bool isDown, bool isOver,
                       Colour /*noteFillColour*/) override
    {
        auto& palette = ::ColourScheme::getInstance().colours;
        auto key = area.reduced(1.0f, 0.0f).withTrimmedBottom(3.0f);
        auto top = palette["Window Background"].brighter(0.2f);
        auto bottom = palette["Window Background"].darker(0.28f);

        if (isDown)
        {
            top = palette["Accent Colour"].darker(0.04f);
            bottom = palette["Accent Colour"].darker(0.46f);
            key = key.translated(0.0f, 1.0f);
        }
        else if (isOver)
        {
            top = top.overlaidWith(palette["Accent Colour"].withAlpha(0.18f));
            bottom = bottom.overlaidWith(palette["Accent Colour"].withAlpha(0.1f));
        }

        g.setColour(palette["Window Background"].darker(0.7f).withAlpha(0.34f));
        g.fillRoundedRectangle(key.translated(0.0f, 1.8f), 3.0f);

        ColourGradient fill(top, key.getX(), key.getY(), bottom, key.getX(), key.getBottom(), false);
        g.setGradientFill(fill);
        g.fillRoundedRectangle(key, 3.0f);

        g.setColour(palette["Text Colour"].withAlpha(isDown ? 0.16f : 0.08f));
        g.drawLine(key.getX() + 2.0f, key.getY() + 1.5f, key.getRight() - 2.0f, key.getY() + 1.5f, 1.0f);
        g.setColour((isDown ? palette["Accent Colour"] : palette["Text Colour"]).withAlpha(isDown ? 0.72f : 0.18f));
        g.drawRoundedRectangle(key.reduced(0.5f), 3.0f, 1.0f);
    }
};

KeyboardDeckButtonLookAndFeel keyboardDeckButtonLookAndFeel;
}

//------------------------------------------------------------------------------
File MainPanel::lastDocument = File();

//------------------------------------------------------------------------------
class PluginListWindow : public DocumentWindow
{
  public:
    PluginListWindow(KnownPluginList& knownPluginList, MainPanel* p, bool useSafeScanner = true)
        : DocumentWindow("Available Plugins", ColourScheme::getInstance().colours["Dialog Background"],
                         DocumentWindow::minimiseButton | DocumentWindow::closeButton),
          panel(p)
    {
        const File deadMansPedalFile(
            SettingsManager::getInstance().getUserDataDirectory().getChildFile("RecentlyCrashedPluginsList"));

        if (useSafeScanner)
        {
            // Use our safe scanner with out-of-process support
            setContentOwned(new SafePluginListComponent(AudioPluginFormatManagerSingleton::getInstance(),
                                                        knownPluginList, deadMansPedalFile, nullptr),
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
        centreWithSize(500, 500); // Slightly larger for better UX
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
    patchLabel->setFont(FontManager::getInstance().getSubheadingFont());
    patchLabel->setJustificationType(Justification::centredLeft);
    patchLabel->setEditable(false, false, false);
    patchLabel->setColour(TextEditor::textColourId, ColourScheme::getInstance().colours["Text Colour"]);
    patchLabel->setColour(TextEditor::backgroundColourId,
                          ColourScheme::getInstance().colours["Text Editor Colour"].withAlpha(0.0f));

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

    addAndMakeVisible(cpuLabel = new Label("cpuLabel", "CPU"));
    cpuLabel->setFont(FontManager::getInstance().getSubheadingFont());
    cpuLabel->setJustificationType(Justification::centredLeft);
    cpuLabel->setEditable(false, false, false);
    cpuLabel->setColour(TextEditor::textColourId, ColourScheme::getInstance().colours["Text Colour"]);
    cpuLabel->setColour(TextEditor::backgroundColourId,
                        ColourScheme::getInstance().colours["Text Editor Colour"].withAlpha(0.0f));

    addAndMakeVisible(playButton = new DrawableButton("playButton", DrawableButton::ImageOnButtonBackground));
    playButton->setName("playButton");

    addAndMakeVisible(rtzButton = new DrawableButton("rtzButton", DrawableButton::ImageOnButtonBackground));
    rtzButton->setName("rtzButton");

    addAndMakeVisible(tempoLabel = new Label("tempoLabel", "Tempo:"));
    tempoLabel->setFont(FontManager::getInstance().getSubheadingFont());
    tempoLabel->setJustificationType(Justification::centredLeft);
    tempoLabel->setEditable(false, false, false);
    tempoLabel->setColour(TextEditor::textColourId, ColourScheme::getInstance().colours["Text Colour"]);
    tempoLabel->setColour(TextEditor::backgroundColourId,
                          ColourScheme::getInstance().colours["Text Editor Colour"].withAlpha(0.0f));

    addAndMakeVisible(tempoEditor = new TextEditor("tempoEditor"));
    tempoEditor->setMultiLine(false);
    tempoEditor->setReturnKeyStartsNewLine(false);
    tempoEditor->setReadOnly(false);
    tempoEditor->setScrollbarsShown(true);
    tempoEditor->setCaretVisible(true);
    tempoEditor->setPopupMenuEnabled(true);
    tempoEditor->setText("120.00");

    addAndMakeVisible(tapTempoButton =
                          new ArrowButton("tapTempoButton", 0.0,
                                          ColourScheme::getInstance().colours["Button Highlight"].withAlpha(0.25f)));
    tapTempoButton->setName("tapTempoButton");

    addAndMakeVisible(organiseButton = new TextButton("organiseButton"));
    organiseButton->setButtonText("Manage");
    organiseButton->addListener(this);

    addAndMakeVisible(fitButton = new TextButton("fitButton"));
    fitButton->setButtonText("Fit");
    fitButton->setTooltip("Fit all nodes to screen");
    fitButton->addListener(this);

    addAndMakeVisible(inputGainLabel = new Label("inputGainLabel", "IN"));
    inputGainLabel->setFont(FontManager::getInstance().getLabelFont());
    inputGainLabel->setJustificationType(Justification::centredRight);

    addAndMakeVisible(inputGainSlider = new Slider("inputGainSlider"));
    inputGainSlider->setSliderStyle(Slider::LinearBar);
    inputGainSlider->setRange(-60.0, 12.0, 0.1);
    inputGainSlider->setTextValueSuffix(" dB");
    inputGainSlider->setDoubleClickReturnValue(true, 0.0);
    inputGainSlider->setTooltip("Master Input Gain");
    inputGainSlider->textFromValueFunction = [](double v) { return "IN " + String(v, 1) + " dB"; };
    inputGainSlider->addListener(this);

    addAndMakeVisible(outputGainLabel = new Label("outputGainLabel", "OUT"));
    outputGainLabel->setFont(FontManager::getInstance().getLabelFont());
    outputGainLabel->setJustificationType(Justification::centredRight);

    addAndMakeVisible(outputGainSlider = new Slider("outputGainSlider"));
    outputGainSlider->setSliderStyle(Slider::LinearBar);
    outputGainSlider->setRange(-60.0, 12.0, 0.1);
    outputGainSlider->setTextValueSuffix(" dB");
    outputGainSlider->setDoubleClickReturnValue(true, 0.0);
    outputGainSlider->setTooltip("Master Output Gain");
    outputGainSlider->textFromValueFunction = [](double v) { return "OUT " + String(v, 1) + " dB"; };
    outputGainSlider->addListener(this);

    addAndMakeVisible(masterInsertButton = new TextButton("masterInsertButton"));
    masterInsertButton->setButtonText("FX");
    masterInsertButton->setTooltip("Master Bus Insert Rack");
    masterInsertButton->addListener(this);

    addAndMakeVisible(uiScaleFooterLabel = new Label("uiScaleFooterLabel", "UI Scale"));
    uiScaleFooterLabel->setFont(FontManager::getInstance().getLabelFont());
    uiScaleFooterLabel->setJustificationType(Justification::centredRight);
    uiScaleFooterLabel->setTooltip("Pedalboard UI Scale");

    addAndMakeVisible(uiScaleFooterComboBox = new ComboBox("uiScaleFooterComboBox"));
    uiScaleFooterComboBox->setJustificationType(Justification::centredLeft);
    uiScaleFooterComboBox->setTextWhenNothingSelected("100%");
    uiScaleFooterComboBox->setTextWhenNoChoicesAvailable("(no scale choices)");
    uiScaleFooterComboBox->setTooltip("Pedalboard UI Scale");
    for (auto percent : UiScale::supportedPercents())
        uiScaleFooterComboBox->addItem(String(percent) + "%", percent);
    uiScaleFooterComboBox->addListener(this);

    addAndMakeVisible(scratchRecordButton = new TextButton("scratchRecordButton"));
    scratchRecordButton->setButtonText("REC");
    scratchRecordButton->setTooltip("Record scratch idea");
    scratchRecordButton->addListener(this);

    addAndMakeVisible(scratchStatusLabel = new Label("scratchStatusLabel", "Ready"));
    scratchStatusLabel->setFont(FontManager::getInstance().getLabelFont());
    scratchStatusLabel->setJustificationType(Justification::centredLeft);
    scratchStatusLabel->setTooltip("Scratch capture status");

    addAndMakeVisible(scratchPanelButton = new TextButton("scratchPanelButton"));
    scratchPanelButton->setButtonText("Takes");
    scratchPanelButton->setTooltip("Open scratch takes");
    scratchPanelButton->addListener(this);

    themeSwitcher = std::make_unique<ThemeSwitcherComponent>(
        [this](const String& presetName) { applyColourSchemePreset(presetName); });
    addAndMakeVisible(themeSwitcher.get());

    const auto savedScratchRoot = SettingsManager::getInstance().getString(kScratchRootSettingsKey);
    if (savedScratchRoot.isNotEmpty())
        scratchRecorder.setScratchRoot(File(savedScratchRoot));

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

        DawMixerProcessor dawMixer;
        dawMixer.fillInPluginDescription(desc);
        pluginList.addType(desc);

        DawSplitterProcessor dawSplitter;
        dawSplitter.fillInPluginDescription(desc);
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
    graphPlayer.setScratchRecorder(&scratchRecorder);
    deviceManager.addAudioCallback(&graphPlayer);

    // Device meter tap for I/O node VU meters (can be disabled for debugging)
    if (SettingsManager::getInstance().getBool("EnableDeviceMeterTap", true))
    {
        deviceManager.addAudioCallback(&deviceMeterTap);
        DeviceMeterTap::setInstance(&deviceMeterTap);
    }
    deviceManager.addChangeListener(this);

    // Setup midi: global callback receives from ALL enabled MIDI inputs.
    // In JUCE 8, per-device callbacks with specific identifiers don't fire;
    // empty identifier is required to receive from all enabled devices.
    deviceManager.addMidiInputCallback({}, &graphPlayer);

    // On first launch (no saved audio state), auto-enable all MIDI devices.
    if (!savedAudioState)
    {
        for (const auto& device : MidiInput::getAvailableDevices())
            deviceManager.setMidiInputDeviceEnabled(device.identifier, true);
    }

    // Setup virtual MIDI keyboard
    virtualKeyboardCollapsed = SettingsManager::getInstance().getBool(kVirtualKeyboardCollapsedSettingsKey, false);
    virtualKeyboard =
        std::make_unique<PedalboardMidiKeyboardComponent>(keyboardState, MidiKeyboardComponent::horizontalKeyboard);
    virtualKeyboard->setKeyWidth(40.0f);
    virtualKeyboard->setAvailableRange(36, 96); // C2 to C7
    addAndMakeVisible(virtualKeyboard.get());
    keyboardState.addListener(this);

    // Keyboard control strip
    keyboardToggleButton = std::make_unique<TextButton>("KEYS");
    keyboardToggleButton->setTooltip("Show or hide the virtual MIDI keyboard");
    keyboardToggleButton->setClickingTogglesState(true);
    keyboardToggleButton->setToggleState(!virtualKeyboardCollapsed, dontSendNotification);
    keyboardToggleButton->setLookAndFeel(&keyboardDeckButtonLookAndFeel);
    keyboardToggleButton->addListener(this);
    addAndMakeVisible(keyboardToggleButton.get());

    octaveDownButton = std::make_unique<TextButton>("-");
    octaveDownButton->setLookAndFeel(&keyboardDeckButtonLookAndFeel);
    octaveDownButton->addListener(this);
    addAndMakeVisible(octaveDownButton.get());

    octaveUpButton = std::make_unique<TextButton>("+");
    octaveUpButton->setLookAndFeel(&keyboardDeckButtonLookAndFeel);
    octaveUpButton->addListener(this);
    addAndMakeVisible(octaveUpButton.get());

    octaveLabel = std::make_unique<Label>("octave", "Oct: 0");
    octaveLabel->setJustificationType(Justification::centred);
    octaveLabel->setFont(FontManager::getInstance().getLabelFont());
    octaveLabel->setColour(Label::textColourId, ColourScheme::getInstance().colours["Text Colour"].withAlpha(0.8f));
    addAndMakeVisible(octaveLabel.get());

    velocitySlider = std::make_unique<Slider>(Slider::LinearHorizontal, Slider::TextBoxRight);
    velocitySlider->setRange(1.0, 127.0, 1.0);
    velocitySlider->setValue(100.0);
    velocitySlider->addListener(this);
    velocitySlider->setTextBoxStyle(Slider::TextBoxRight, false, 32, 18);
    addAndMakeVisible(velocitySlider.get());

    velocityLabel = std::make_unique<Label>("vel", "Vel:");
    velocityLabel->setJustificationType(Justification::centredRight);
    velocityLabel->setFont(FontManager::getInstance().getLabelFont());
    velocityLabel->setColour(Label::textColourId, ColourScheme::getInstance().colours["Text Colour"].withAlpha(0.8f));
    addAndMakeVisible(velocityLabel.get());

    sustainButton = std::make_unique<TextButton>("Sus");
    sustainButton->setClickingTogglesState(true);
    sustainButton->setLookAndFeel(&keyboardDeckButtonLookAndFeel);
    sustainButton->addListener(this);
    addAndMakeVisible(sustainButton.get());

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

    // Wire the lock-free FIFO so MIDI/OSC mapping parameter changes are
    // deferred from the audio thread to this timer on the message thread.
    Mapping::setParamFifo(&midiAppFifo);

    // Load master gain state from settings and sync footer sliders
    MasterGainState::getInstance().loadFromSettings();
    {
        auto& gs = MasterGainState::getInstance();
        inputGainSlider->setValue(gs.masterInputGainDb.load(std::memory_order_relaxed), dontSendNotification);
        outputGainSlider->setValue(gs.masterOutputGainDb.load(std::memory_order_relaxed), dontSendNotification);
    }
    syncUiScaleComboBoxFromSettings();

    // Start timers.
    startTimer(CpuTimer, 100);
    startTimer(MidiAppTimer, 5);

    // To load the default patch.
    {
        File defaultPatch = JuceHelperStuff::getAppDataFolder().getChildFile("default.pdl");

        if (defaultPatch.existsAsFile())
            commandManager->invokeDirectly(FileNew, true);
    }

    // Defer fitToScreen until after the message loop processes all pending
    // resize/layout events so the viewport has its final dimensions.
    MessageManager::callAsync(
        [this]()
        {
            if (auto* pluginField = dynamic_cast<PluginField*>(viewport->getViewedComponent()))
                pluginField->fitToScreen();
        });

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

    // Save gain state before shutdown
    MasterGainState::getInstance().saveToSettings();

    // Remove keyboard listener before destruction
    keyboardState.removeListener(this);
    if (keyboardToggleButton)
        keyboardToggleButton->setLookAndFeel(nullptr);
    if (octaveDownButton)
        octaveDownButton->setLookAndFeel(nullptr);
    if (octaveUpButton)
        octaveUpButton->setLookAndFeel(nullptr);
    if (sustainButton)
        sustainButton->setLookAndFeel(nullptr);

    int i;

    // Logger::setCurrentLogger(0);

    signalThreadShouldExit();
    stopThread(2000);

    // deviceManager.setAudioCallback(0);
    if (DeviceMeterTap::getInstance() != nullptr)
    {
        deviceManager.removeAudioCallback(&deviceMeterTap);
        DeviceMeterTap::setInstance(nullptr);
    }
    scratchRecorder.requestStop();
    graphPlayer.setScratchRecorder(nullptr);
    deviceManager.removeAudioCallback(&graphPlayer);
    deviceManager.removeMidiInputCallback({}, &graphPlayer);
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
    delete inputGainSlider;
    inputGainSlider = nullptr;
    delete outputGainSlider;
    outputGainSlider = nullptr;
    delete inputGainLabel;
    inputGainLabel = nullptr;
    delete outputGainLabel;
    outputGainLabel = nullptr;
    delete masterInsertButton;
    masterInsertButton = nullptr;
    delete uiScaleFooterLabel;
    uiScaleFooterLabel = nullptr;
    delete uiScaleFooterComboBox;
    uiScaleFooterComboBox = nullptr;
    delete scratchRecordButton;
    scratchRecordButton = nullptr;
    delete scratchPanelButton;
    scratchPanelButton = nullptr;
    delete scratchStatusLabel;
    scratchStatusLabel = nullptr;

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

    //[UserPaint] Add your own custom painting code here..

    g.fillAll(ColourScheme::getInstance().colours["Window Background"]);

    //[/UserPaint]

    paintVirtualKeyboardDeck(g);
}

void MainPanel::resized()
{
    const auto uiScalePercent = getUiScalePercent();
    const int toolbarHeight = UiScale::footerHeight(getWidth(), uiScalePercent);
    const int keyboardDeckHeight = getVirtualKeyboardDeckHeight();
    const int viewportHeight = jmax(0, getHeight() - toolbarHeight - keyboardDeckHeight);
    const int footerTop = getHeight() - toolbarHeight;
    const int footerLayoutW = UiScale::footerLayoutWidth(getWidth(), uiScalePercent);
    const int footerW = getWidth();
    const bool singleRowFooter = UiScale::shouldUseSingleRowFooter(getWidth(), uiScalePercent);
    const bool stackedFooter = UiScale::shouldUseStackedFooter(getWidth(), uiScalePercent);

    viewport->setBounds(0, 0, getWidth(), viewportHeight);

    // Virtual MIDI keyboard control strip + keys
    if (virtualKeyboard != nullptr)
    {
        const int controlStripHeight = 26;
        const int keysHeight = keyboardDeckHeight - controlStripHeight;
        const int stripY = viewportHeight;
        const int buttonY = stripY + 3;
        const int buttonH = 20;

        keyboardToggleButton->setVisible(true);
        keyboardToggleButton->setToggleState(!virtualKeyboardCollapsed, dontSendNotification);
        keyboardToggleButton->setBounds(8, buttonY, 54, buttonH);

        const bool showKeyboardControls = !virtualKeyboardCollapsed;
        octaveDownButton->setVisible(showKeyboardControls);
        octaveUpButton->setVisible(showKeyboardControls);
        octaveLabel->setVisible(showKeyboardControls);
        velocitySlider->setVisible(showKeyboardControls);
        velocityLabel->setVisible(showKeyboardControls);
        sustainButton->setVisible(showKeyboardControls);
        virtualKeyboard->setVisible(showKeyboardControls);

        if (showKeyboardControls)
        {
            // Control strip: [Keys] [Oct- Oct:0 Oct+] [Vel: ====slider====] [Sus]
            int cx = 70;
            octaveDownButton->setBounds(cx, buttonY, 24, buttonH);
            cx += 28;
            octaveLabel->setBounds(cx, buttonY, 48, buttonH);
            cx += 52;
            octaveUpButton->setBounds(cx, buttonY, 24, buttonH);
            cx += 36;
            velocityLabel->setBounds(cx, buttonY, 30, buttonH);
            cx += 32;
            velocitySlider->setBounds(cx, stripY + 2, jmin(170, jmax(92, getWidth() / 9)), controlStripHeight - 4);
            sustainButton->setBounds(getWidth() - 50, buttonY, 42, buttonH);

            virtualKeyboard->setBounds(0, stripY + controlStripHeight, getWidth(), keysHeight);
        }
    }

    //[UserResized] Add your own custom resize handling here..

    int x, y;
    Component* field = viewport->getViewedComponent();

    // Canvas must always be larger than viewport to enable scrolling/panning.
    // Add generous padding so scrollbars appear and drag-pan works in all directions.
    const int canvasPadding = 1000;
    x = field->getWidth();
    y = field->getHeight();
    if (x < getWidth() + canvasPadding)
        x = getWidth() + canvasPadding;
    if (y < viewportHeight + canvasPadding)
        y = viewportHeight + canvasPadding;
    field->setSize(x, y);

    // Keep StageView covering the entire panel
    if (stageView != nullptr)
        stageView->setBounds(getLocalBounds());

    if (themeSwitcher != nullptr)
    {
        constexpr int switcherW = 136;
        themeSwitcher->setBounds(jmax(8, getWidth() - switcherW - 12), 10, switcherW, 32);
        themeSwitcher->setVisible(stageView == nullptr && getWidth() >= 760 && viewportHeight >= 120);
        themeSwitcher->toFront(false);
    }

    const int controlH = 24;
    const int gap = 4;
    const int rightMargin = 6;

    patchLabel->setVisible(true);
    prevPatch->setVisible(true);
    nextPatch->setVisible(true);
    patchComboBox->setVisible(true);
    tempoLabel->setVisible(true);
    tempoEditor->setVisible(true);
    tapTempoButton->setVisible(true);
    playButton->setVisible(true);
    rtzButton->setVisible(true);
    fitButton->setVisible(true);
    organiseButton->setVisible(true);
    cpuLabel->setVisible(true);
    cpuSlider->setVisible(true);
    scratchRecordButton->setVisible(true);
    scratchStatusLabel->setVisible(true);
    scratchPanelButton->setVisible(true);

    auto layoutPatchControls = [this, gap, controlH](int left, int controlY, int right)
    {
        const int available = jmax(0, right - left);
        const bool showPatchLabel = available >= 172;
        int x = left;

        patchLabel->setVisible(showPatchLabel);
        if (showPatchLabel)
        {
            patchLabel->setBounds(x, controlY, 44, controlH);
            x += 44 + gap;
        }

        const int navW = 48;
        const int comboW = jmax(48, jmin(220, right - x - gap - navW));
        patchComboBox->setBounds(x, controlY, comboW, controlH);
        x += comboW + gap;

        const int navX = jmin(x, right - navW);
        prevPatch->setBounds(navX, controlY, 24, controlH);
        nextPatch->setBounds(navX + 24, controlY, 24, controlH);
    };

    auto layoutUtilityControls = [this, gap, controlH](int right, int controlY, bool showScaleLabel)
    {
        uiScaleFooterComboBox->setVisible(true);
        uiScaleFooterComboBox->setBounds(right - 78, controlY, 78, controlH);
        right -= 78 + gap;

        uiScaleFooterLabel->setVisible(showScaleLabel);
        if (showScaleLabel)
        {
            uiScaleFooterLabel->setBounds(right - 58, controlY, 58, controlH);
            right -= 58 + gap;
        }

        fitButton->setBounds(right - 38, controlY, 38, controlH);
        right -= 38 + gap;
        organiseButton->setBounds(right - 64, controlY, 64, controlH);
        right -= 64 + gap;

        return right - gap;
    };

    auto layoutTransportControls = [this, gap, controlH](int left, int controlY)
    {
        int x = left;
        tempoLabel->setBounds(x, controlY, 52, controlH);
        x += 52 + gap;
        tempoEditor->setBounds(x, controlY, 50, controlH);
        x += 50 + gap;
        tapTempoButton->setBounds(x, controlY + 4, 10, 16);
        x += 10 + 8;
        playButton->setBounds(x, controlY - 6, 36, 36);
        x += 36 + gap;
        rtzButton->setBounds(x, controlY, 24, controlH);
        return x + 24;
    };

    auto layoutScratchControls = [this, gap, controlH](int left, int controlY, int areaW)
    {
        const int recW = 52;
        const int panelW = 56;
        const int minStatusW = 64;

        if (areaW >= recW + gap + minStatusW + gap + panelW)
        {
            int x = left;
            scratchRecordButton->setVisible(true);
            scratchRecordButton->setBounds(x, controlY, recW, controlH);
            x += recW + gap;
            scratchStatusLabel->setVisible(true);
            scratchStatusLabel->setBounds(x, controlY, areaW - recW - panelW - gap * 2, controlH);
            scratchPanelButton->setVisible(true);
            scratchPanelButton->setBounds(left + areaW - panelW, controlY, panelW, controlH);
        }
        else if (areaW >= recW + gap + panelW)
        {
            scratchRecordButton->setVisible(true);
            scratchRecordButton->setBounds(left, controlY, recW, controlH);
            scratchStatusLabel->setVisible(false);
            scratchPanelButton->setVisible(true);
            scratchPanelButton->setBounds(left + areaW - panelW, controlY, panelW, controlH);
        }
        else
        {
            scratchRecordButton->setVisible(areaW >= recW);
            scratchRecordButton->setBounds(left, controlY, recW, controlH);
            scratchStatusLabel->setVisible(false);
            scratchPanelButton->setVisible(false);
        }
    };

    auto layoutCpuControls = [this, rightMargin, controlH](int footerWidth, int controlY)
    {
        int cpuRight = footerWidth - rightMargin;
        const int cpuSliderW = jlimit(72, 120, footerWidth / 5);
        cpuSlider->setBounds(cpuRight - cpuSliderW, controlY, cpuSliderW, controlH);
        cpuRight -= cpuSliderW + 2;
        cpuLabel->setBounds(cpuRight - 36, controlY, 36, controlH);
        return cpuRight - 36;
    };

    auto layoutGainControls = [this, gap, controlH](int left, int controlY, int areaW, int minSliderW)
    {
        const int labelW = 34;
        const int fxBtnW = 28;
        const int fullW = labelW * 2 + gap * 3 + minSliderW * 2 + fxBtnW;
        const int compactW = gap * 2 + minSliderW * 2 + fxBtnW;
        const int smallestSliderW = 28;
        const int smallestCompactW = gap * 2 + smallestSliderW * 2 + fxBtnW;

        if (areaW >= fullW)
        {
            const int sliderW = (areaW - labelW * 2 - gap * 3 - fxBtnW) / 2;
            int x = left;

            inputGainLabel->setVisible(true);
            inputGainLabel->setBounds(x, controlY, labelW, controlH);
            x += labelW;
            inputGainSlider->setVisible(true);
            inputGainSlider->setBounds(x, controlY, sliderW, controlH);
            x += sliderW + gap;
            masterInsertButton->setVisible(true);
            masterInsertButton->setBounds(x, controlY, fxBtnW, controlH);
            x += fxBtnW + gap;
            outputGainLabel->setVisible(true);
            outputGainLabel->setBounds(x, controlY, labelW, controlH);
            x += labelW;
            outputGainSlider->setVisible(true);
            outputGainSlider->setBounds(x, controlY, sliderW, controlH);
        }
        else if (areaW >= compactW || areaW >= smallestCompactW)
        {
            const int sliderW = jmax(smallestSliderW, (areaW - gap * 2 - fxBtnW) / 2);
            int x = left;

            inputGainLabel->setVisible(false);
            outputGainLabel->setVisible(false);
            inputGainSlider->setVisible(true);
            inputGainSlider->setBounds(x, controlY, sliderW, controlH);
            x += sliderW + gap;
            masterInsertButton->setVisible(true);
            masterInsertButton->setBounds(x, controlY, fxBtnW, controlH);
            x += fxBtnW + gap;
            outputGainSlider->setVisible(true);
            outputGainSlider->setBounds(x, controlY, sliderW, controlH);
        }
        else if (areaW >= fxBtnW)
        {
            inputGainLabel->setVisible(false);
            outputGainLabel->setVisible(false);
            inputGainSlider->setVisible(false);
            outputGainSlider->setVisible(false);
            masterInsertButton->setVisible(true);
            masterInsertButton->setBounds(left + (areaW - fxBtnW) / 2, controlY, fxBtnW, controlH);
        }
        else
        {
            inputGainLabel->setVisible(false);
            outputGainLabel->setVisible(false);
            inputGainSlider->setVisible(false);
            outputGainSlider->setVisible(false);
            masterInsertButton->setVisible(false);
        }
    };

    if (singleRowFooter)
    {
        const int footerY = footerTop + 7;
        const int centerX = footerW / 2;

        layoutScratchControls(8, footerY, 112);
        layoutPatchControls(128, footerY, 312);
        playButton->setBounds(centerX - 18, footerTop + 2, 36, 36);
        rtzButton->setBounds(centerX + 20, footerY, 24, controlH);
        tempoLabel->setBounds(centerX - 169, footerY, 64, controlH);
        tempoEditor->setBounds(centerX - 105, footerY, 52, controlH);
        tapTempoButton->setBounds(centerX - 49, footerY + 6, 10, 16);

        int rxEnd = footerW - rightMargin;
        cpuSlider->setBounds(rxEnd - 144, footerY, 144, controlH);
        rxEnd -= 144 + 2;
        cpuLabel->setBounds(rxEnd - 42, footerY, 42, controlH);
        rxEnd -= 42 + gap;
        rxEnd = layoutUtilityControls(rxEnd, footerY, true);

        const int transportEndX = centerX + 54;
        layoutGainControls(transportEndX + gap, footerY, rxEnd - transportEndX - gap, 50);
    }
    else
    {
        const int row1Y = footerTop + 6;
        const int row2Y = footerTop + 40;
        const bool showScaleLabel = footerLayoutW >= 640;
        const int patchRight = layoutUtilityControls(footerW - rightMargin, row1Y, showScaleLabel);
        layoutPatchControls(8, row1Y, patchRight);

        if (stackedFooter)
        {
            const int row3Y = footerTop + 72;
            layoutTransportControls(8, row2Y);
            layoutCpuControls(footerW, row2Y);
            layoutScratchControls(8, row3Y, 180);
            layoutGainControls(196, row3Y, footerW - 204, 64);
        }
        else
        {
            const int scratchW = footerLayoutW >= 780 ? 180 : 112;
            layoutScratchControls(8, row2Y, scratchW);
            const int transportEndX = layoutTransportControls(8 + scratchW + 8, row2Y);
            const int cpuLeft = layoutCpuControls(footerW, row2Y);
            const int gainLeft = transportEndX + 8;
            const int gainRight = cpuLeft - 8;
            layoutGainControls(gainLeft, row2Y, gainRight - gainLeft, 44);
        }
    }

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
    else if (buttonThatWasClicked == masterInsertButton)
    {
        auto& gainState = MasterGainState::getInstance();
        auto& masterBus = gainState.getMasterBus();
        auto* editor = new SubGraphEditorComponent(*masterBus.getRack());
        editor->setSize(600, 400);

        DialogWindow::LaunchOptions opts;
        opts.content.setOwned(editor);
        opts.dialogTitle = "Master Bus Insert Rack";
        opts.dialogBackgroundColour = ColourScheme::getInstance().colours["Dialog Background"];
        opts.escapeKeyTriggersCloseButton = true;
        opts.useNativeTitleBar = true;
        opts.resizable = true;
        opts.launchAsync();
    }
    else if (buttonThatWasClicked == scratchRecordButton)
        commandManager->invokeDirectly(ScratchCaptureToggle, true);
    else if (buttonThatWasClicked == scratchPanelButton)
        commandManager->invokeDirectly(ScratchPanelOpen, true);
    else if (buttonThatWasClicked == keyboardToggleButton.get())
    {
        virtualKeyboardCollapsed = !keyboardToggleButton->getToggleState();
        SettingsManager::getInstance().setValue(kVirtualKeyboardCollapsedSettingsKey, virtualKeyboardCollapsed);
        resized();
        repaint();
    }
    else if (buttonThatWasClicked == octaveDownButton.get())
    {
        if (auto* proc = VirtualMidiInputProcessor::getInstance())
        {
            proc->setOctaveShift(proc->getOctaveShift() - 1);
            int shift = proc->getOctaveShift();
            int baseC2 = 36; // C2 = MIDI 36
            int low = baseC2 + shift * 12;
            int high = low + 60; // 5 octaves
            virtualKeyboard->setAvailableRange(jlimit(0, 127, low), jlimit(0, 127, high));
            updateOctaveDisplay();
        }
    }
    else if (buttonThatWasClicked == octaveUpButton.get())
    {
        if (auto* proc = VirtualMidiInputProcessor::getInstance())
        {
            proc->setOctaveShift(proc->getOctaveShift() + 1);
            int shift = proc->getOctaveShift();
            int baseC2 = 36;
            int low = baseC2 + shift * 12;
            int high = low + 60;
            virtualKeyboard->setAvailableRange(jlimit(0, 127, low), jlimit(0, 127, high));
            updateOctaveDisplay();
        }
    }
    else if (buttonThatWasClicked == sustainButton.get())
    {
        if (auto* proc = VirtualMidiInputProcessor::getInstance())
        {
            bool held = sustainButton->getToggleState();
            proc->setSustainHeld(held);
            sustainButton->setColour(TextButton::buttonColourId,
                                     held ? ColourScheme::getInstance().colours["Accent Colour"]
                                          : getLookAndFeel().findColour(TextButton::buttonColourId));
        }
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
    else if (comboBoxThatHasChanged == uiScaleFooterComboBox)
    {
        setUiScalePercent(uiScaleFooterComboBox->getSelectedId());
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
    else if (sliderThatWasMoved == inputGainSlider)
    {
        auto& state = MasterGainState::getInstance();
        state.masterInputGainDb.store((float)inputGainSlider->getValue(), std::memory_order_relaxed);
        state.saveToSettings();
    }
    else if (sliderThatWasMoved == outputGainSlider)
    {
        auto& state = MasterGainState::getInstance();
        state.masterOutputGainDb.store((float)outputGainSlider->getValue(), std::memory_order_relaxed);
        state.saveToSettings();
    }
    else if (sliderThatWasMoved == velocitySlider.get())
    {
        if (auto* proc = VirtualMidiInputProcessor::getInstance())
            proc->setFixedVelocity(static_cast<int>(velocitySlider->getValue()));
    }
    //[/UsersliderValueChanged_Post]
}

//[MiscUserCode] You can add your own definitions of your custom methods or any
// other code here...

//------------------------------------------------------------------------------
void MainPanel::handleNoteOn(MidiKeyboardState* source, int midiChannel, int midiNoteNumber, float velocity)
{
    ignoreUnused(source);
    if (auto* processor = VirtualMidiInputProcessor::getInstance())
    {
        auto msg = MidiMessage::noteOn(midiChannel, midiNoteNumber, velocity);
        msg.setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
        processor->addMidiMessage(msg);
    }
}

//------------------------------------------------------------------------------
void MainPanel::handleNoteOff(MidiKeyboardState* source, int midiChannel, int midiNoteNumber, float velocity)
{
    ignoreUnused(source);
    if (auto* processor = VirtualMidiInputProcessor::getInstance())
    {
        auto msg = MidiMessage::noteOff(midiChannel, midiNoteNumber, velocity);
        msg.setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
        processor->addMidiMessage(msg);
    }
}

//------------------------------------------------------------------------------
int MainPanel::getVirtualKeyboardDeckHeight() const
{
    return virtualKeyboardCollapsed ? keyboardCollapsedHeight : keyboardHeight;
}

//------------------------------------------------------------------------------
void MainPanel::paintVirtualKeyboardDeck(Graphics& g)
{
    const auto uiScalePercent = getUiScalePercent();
    const int toolbarHeight = UiScale::footerHeight(getWidth(), uiScalePercent);
    const int deckHeight = getVirtualKeyboardDeckHeight();
    const int deckTop = getHeight() - toolbarHeight - deckHeight;
    if (deckTop < 0 || deckHeight <= 0)
        return;

    auto& colours = ColourScheme::getInstance().colours;
    auto deck = Rectangle<float>(0.0f, (float)deckTop, (float)getWidth(), (float)deckHeight);

    ColourGradient back(colours["Window Background"].brighter(0.05f), deck.getX(), deck.getY(),
                        colours["Window Background"].darker(0.12f), deck.getX(), deck.getBottom(), false);
    g.setGradientFill(back);
    g.fillRect(deck);

    g.setColour(colours["Plugin Border"].withAlpha(0.42f));
    g.drawHorizontalLine(deckTop, 0.0f, (float)getWidth());
    g.setColour(colours["Text Colour"].withAlpha(0.05f));
    g.drawHorizontalLine(deckTop + 1, 0.0f, (float)getWidth());

    if (virtualKeyboardCollapsed)
    {
        auto labelArea = deck.reduced(70.0f, 0.0f);
        g.setFont(FontManager::getInstance().getCaptionFont());
        g.setColour(colours["Text Colour"].withAlpha(0.52f));
        g.drawText("VIRTUAL PIANO  HIDDEN", labelArea.withWidth(220.0f), Justification::centredLeft, true);

        const auto miniKeyW = 9.0f;
        auto mini = Rectangle<float>(deck.getRight() - 170.0f, deck.getCentreY() - 5.0f, 148.0f, 10.0f);
        for (int i = 0; i < 14; ++i)
        {
            auto key = Rectangle<float>(mini.getX() + (float)i * miniKeyW, mini.getY(), miniKeyW - 1.0f,
                                        mini.getHeight());
            g.setColour(colours["Text Colour"].withAlpha(i % 7 == 1 || i % 7 == 3 || i % 7 == 5 ? 0.22f : 0.42f));
            g.fillRoundedRectangle(key, 1.5f);
        }
    }
    else
    {
        auto strip = deck.removeFromTop(26.0f);
        g.setColour(colours["Dialog Inner Background"].withAlpha(0.62f));
        g.fillRect(strip);
        g.setColour(colours["Accent Colour"].withAlpha(0.36f));
        g.drawLine(8.0f, strip.getBottom() - 1.0f, jmin(360.0f, (float)getWidth() - 8.0f), strip.getBottom() - 1.0f,
                   1.0f);
    }
}

//------------------------------------------------------------------------------
void MainPanel::updateOctaveDisplay()
{
    if (auto* proc = VirtualMidiInputProcessor::getInstance())
    {
        int shift = proc->getOctaveShift();
        String text = "Oct: ";
        if (shift > 0)
            text += "+";
        text += String(shift);
        octaveLabel->setText(text, dontSendNotification);
    }
}

//------------------------------------------------------------------------------
void MainPanel::showToast(const String& message)
{
    // Use custom ToastOverlay with Melatonin Blur for premium shadows
    ToastOverlay::getInstance().show(message, 1500);
}

//------------------------------------------------------------------------------
int MainPanel::getUiScalePercent() const
{
    return UiScale::normalisePercent(
        SettingsManager::getInstance().getInt(UiScale::settingsKey, UiScale::defaultPercent));
}

//------------------------------------------------------------------------------
void MainPanel::syncUiScaleComboBoxFromSettings()
{
    if (uiScaleFooterComboBox != nullptr)
        uiScaleFooterComboBox->setSelectedId(getUiScalePercent(), dontSendNotification);
}

//------------------------------------------------------------------------------
void MainPanel::applyColourSchemePreset(const String& presetName, bool showToastMessage)
{
    if (!ColourScheme::getBuiltInPresets().contains(presetName))
        return;

    ColourScheme::getInstance().loadPreset(presetName);
    SettingsManager::getInstance().setValue("colourScheme", presetName);
    refreshThemeDependentSurfaces();

    if (showToastMessage)
        showToast("Theme: " + presetName);
}

//------------------------------------------------------------------------------
void MainPanel::refreshThemeDependentSurfaces()
{
    if (auto* laf = dynamic_cast<BranchesLAF*>(&LookAndFeel::getDefaultLookAndFeel()))
        laf->refreshColours();

    std::function<void(Component*)> refreshNAMTree;
    refreshNAMTree = [&refreshNAMTree](Component* comp)
    {
        if (!comp)
            return;
        if (auto* namCtrl = dynamic_cast<NAMControl*>(comp))
            namCtrl->refreshColours();
        if (auto* namBrowser = dynamic_cast<NAMModelBrowserComponent*>(comp))
            namBrowser->refreshColours();
        for (int i = 0; i < comp->getNumChildComponents(); ++i)
            refreshNAMTree(comp->getChildComponent(i));
    };
    for (int i = Desktop::getInstance().getNumComponents(); --i >= 0;)
        refreshNAMTree(Desktop::getInstance().getComponent(i));

    menuItemsChanged();
    resized();

    if (auto* topLevel = getTopLevelComponent())
        topLevel->repaint();
    else
        repaint();

    for (int i = Desktop::getInstance().getNumComponents(); --i >= 0;)
        if (auto* comp = Desktop::getInstance().getComponent(i))
            comp->repaint();
}

//------------------------------------------------------------------------------
ScratchTakeContext MainPanel::createScratchTakeContext() const
{
    ScratchTakeContext context;
    context.rootDirectory = scratchRecorder.getScratchRoot();
    context.patchIndex = currentPatch;
    context.patchName = getCurrentPatchName();
    context.documentPath = getFile().getFullPathName();

    if (auto* device = deviceManager.getCurrentAudioDevice())
    {
        context.deviceName = device->getName();
        context.sampleRate = device->getCurrentSampleRate();
        context.rawChannelCount = device->getActiveInputChannels().countNumberOfSetBits();
        context.wetChannelCount = device->getActiveOutputChannels().countNumberOfSetBits();
    }

    auto& gainState = MasterGainState::getInstance();
    context.masterInputGainDb = gainState.masterInputGainDb.load(std::memory_order_relaxed);
    context.masterOutputGainDb = gainState.masterOutputGainDb.load(std::memory_order_relaxed);
    return context;
}

//------------------------------------------------------------------------------
void MainPanel::toggleScratchCapture()
{
    if (scratchRecorder.isRecording())
    {
        scratchRecorder.requestStop();
        showToast("Saving scratch take");
    }
    else if (scratchRecorder.start(createScratchTakeContext()))
    {
        showToast("Scratch recording");
    }
    else
    {
        showToast(scratchRecorder.getStatus().message);
    }

    refreshScratchControls();
}

//------------------------------------------------------------------------------
void MainPanel::openScratchPanel()
{
    auto* panel = new ScratchPanel(*this);
    panel->setSize(600, 560);
    JuceHelperStuff::showNonModalDialog("Scratch Capture", panel, this,
                                        ColourScheme::getInstance().colours["Window Background"], true, true);
}

//------------------------------------------------------------------------------
void MainPanel::revealScratchFolder()
{
    const auto root = scratchRecorder.getScratchRoot();
    root.createDirectory();
    root.revealToUser();
}

//------------------------------------------------------------------------------
void MainPanel::chooseScratchFolder()
{
    auto currentRoot = scratchRecorder.getScratchRoot();
    if (currentRoot == File())
        currentRoot = ScratchRecorder::getDefaultScratchRoot();

    scratchFolderChooser = std::make_unique<FileChooser>("Choose Scratch Ideas Folder", currentRoot, "", true);
    const auto chooserFlags = FileBrowserComponent::openMode | FileBrowserComponent::canSelectDirectories;
    Component::SafePointer<MainPanel> safeThis(this);

    scratchFolderChooser->launchAsync(chooserFlags, [safeThis](const FileChooser& chooser)
    {
        auto* panel = safeThis.getComponent();
        if (panel == nullptr)
            return;

        const auto selected = chooser.getResult();
        if (selected == File())
            return;

        selected.createDirectory();
        panel->scratchRecorder.setScratchRoot(selected);
        SettingsManager::getInstance().setValue(kScratchRootSettingsKey, selected.getFullPathName());
        panel->showToast("Scratch destination set");
        panel->refreshScratchControls();
        panel->repaint();
    });
}

//------------------------------------------------------------------------------
void MainPanel::resetScratchFolderToDefault()
{
    if (scratchRecorder.isRecording())
    {
        showToast("Stop recording before changing scratch destination");
        return;
    }

    scratchRecorder.resetScratchRootToDefault();
    SettingsManager::getInstance().setValue(kScratchRootSettingsKey, String{});
    showToast("Scratch destination reset");
    refreshScratchControls();
    repaint();
}

//------------------------------------------------------------------------------
void MainPanel::previewScratchWetTake(const ScratchTake& take)
{
    if (!take.canPlayWetPreview())
    {
        showToast("Wet take unavailable");
        return;
    }

    showToast(take.wetFile.startAsProcess() ? "Opening wet take" : "Could not open wet take");
}

//------------------------------------------------------------------------------
void MainPanel::reampScratchRawTake(const ScratchTake& take)
{
    if (!take.canReampRawCapture())
    {
        showToast("Raw take unavailable");
        return;
    }

    auto* field = viewport != nullptr ? dynamic_cast<PluginField*>(viewport->getViewedComponent()) : nullptr;
    if (field == nullptr || field->getFilterGraph() == nullptr)
    {
        showToast("Patch graph unavailable");
        return;
    }

    auto* graph = field->getFilterGraph();
    const auto previousLastIndex = graph->getNumFilters() - 1;
    const auto x = static_cast<double>(viewport->getViewPositionX() + viewport->getWidth() / 2);
    const auto y = static_cast<double>(viewport->getViewPositionY() + viewport->getHeight() / 2);

    graph->addFilter(new FilePlayerProcessor(take.rawFile), x, y);

    if ((graph->getNumFilters() - 1) > previousLastIndex)
    {
        field->addFilter(graph->getNumFilters() - 1);
        field->clearDoubleClickMessage();
        field->sendChangeMessage();
        showToast("Raw take added for reamp");
    }
    else
    {
        showToast("Could not add raw take");
    }
}

//------------------------------------------------------------------------------
void MainPanel::revealScratchTake(const ScratchTake& take)
{
    if (!take.canReveal())
    {
        showToast("Take folder unavailable");
        return;
    }

    take.takeDirectory.revealToUser();
}

//------------------------------------------------------------------------------
void MainPanel::refreshScratchControls()
{
    const auto status = getScratchRecorderStatus();
    auto& colours = ColourScheme::getInstance().colours;
    const auto text = colours["Text Colour"];
    const auto accent = colours["Accent Colour"];
    const auto border = colours["Plugin Border"];
    const auto stateColour = status.state == ScratchRecorderState::Recording
                                 ? colours["Danger Colour"]
                             : status.state == ScratchRecorderState::Saving
                                 ? colours["Warning Colour"]
                             : status.state == ScratchRecorderState::Failed
                                 ? colours["Danger Colour"]
                                 : accent;

    if (scratchRecordButton != nullptr)
    {
        if (status.state == ScratchRecorderState::Recording)
            scratchRecordButton->setButtonText("STOP");
        else if (status.state == ScratchRecorderState::Saving)
            scratchRecordButton->setButtonText("SAVE");
        else
            scratchRecordButton->setButtonText("REC");

        scratchRecordButton->setEnabled(status.state != ScratchRecorderState::Saving);
        scratchRecordButton->setTooltip(status.state == ScratchRecorderState::Recording
                                            ? "Stop scratch capture and save raw plus wet takes"
                                            : "Record a scratch idea as raw input plus wet output");
        scratchRecordButton->setColour(TextButton::buttonColourId, stateColour.withAlpha(0.28f));
        scratchRecordButton->setColour(TextButton::buttonOnColourId, stateColour.withAlpha(0.38f));
        scratchRecordButton->setColour(TextButton::textColourOffId, text.withAlpha(0.95f));
        scratchRecordButton->setColour(TextButton::textColourOnId, text);
    }

    if (scratchPanelButton != nullptr)
    {
        scratchPanelButton->setColour(TextButton::buttonColourId, border.withAlpha(0.46f));
        scratchPanelButton->setColour(TextButton::buttonOnColourId, accent.withAlpha(0.18f));
        scratchPanelButton->setColour(TextButton::textColourOffId, text.withAlpha(0.86f));
        scratchPanelButton->setColour(TextButton::textColourOnId, text);
    }

    if (scratchStatusLabel != nullptr)
    {
        const auto statusText = ScratchPanelPresentation::formatFooterStatusLine(status);
        scratchStatusLabel->setText(statusText, dontSendNotification);
        scratchStatusLabel->setTooltip(statusText);
        scratchStatusLabel->setColour(Label::textColourId,
                                      (status.state == ScratchRecorderState::Ready ? text.withAlpha(0.82f)
                                                                                   : stateColour.withAlpha(0.94f)));
    }
}

//------------------------------------------------------------------------------
ScratchRecorderStatus MainPanel::getScratchRecorderStatus() const
{
    auto status = scratchRecorder.getStatus();
    status.armedContext = createScratchTakeContext();
    return status;
}

//------------------------------------------------------------------------------
void MainPanel::setUiScalePercent(int percent)
{
    const auto normalisedPercent = UiScale::normalisePercent(percent);
    SettingsManager::getInstance().setValue(UiScale::settingsKey, normalisedPercent);
    Desktop::getInstance().setGlobalScaleFactor(UiScale::toScaleFactor(normalisedPercent));

    if (uiScaleFooterComboBox != nullptr && uiScaleFooterComboBox->getSelectedId() != normalisedPercent)
        uiScaleFooterComboBox->setSelectedId(normalisedPercent, dontSendNotification);

    menuItemsChanged();
    resized();
    repaint();
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
        retval.addCommandItem(commandManager, ScratchCaptureToggle);
        retval.addCommandItem(commandManager, ScratchPanelOpen);
        retval.addCommandItem(commandManager, ScratchRevealFolder);
        retval.addCommandItem(commandManager, ScratchChooseFolder);
        retval.addCommandItem(commandManager, ScratchResetFolderDefault);
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
        PopupMenu uiScaleMenu;
        const auto currentUiScalePercent = getUiScalePercent();
        uiScaleMenu.addItem(OptionsUiScale75, "75%", true, currentUiScalePercent == 75);
        uiScaleMenu.addItem(OptionsUiScale100, "100%", true, currentUiScalePercent == 100);
        uiScaleMenu.addItem(OptionsUiScale125, "125%", true, currentUiScalePercent == 125);
        uiScaleMenu.addItem(OptionsUiScale150, "150%", true, currentUiScalePercent == 150);
        uiScaleMenu.addItem(OptionsUiScale175, "175%", true, currentUiScalePercent == 175);
        uiScaleMenu.addItem(OptionsUiScale200, "200%", true, currentUiScalePercent == 200);
        uiScaleMenu.addSeparator();
        uiScaleMenu.addItem(OptionsUiScaleResetDefault, "Reset to Default (100%)",
                            currentUiScalePercent != UiScale::defaultPercent, false);
        retval.addSubMenu("UI Scale", uiScaleMenu);
        retval.addCommandItem(commandManager, OptionsColourSchemes);
        retval.addSeparator();
        retval.addCommandItem(commandManager, OptionsSnapToGrid);
        PopupMenu graphGridMenu;
        const auto currentGridStyle =
            normaliseGraphGridStyle(SettingsManager::getInstance().getString(kGraphGridStyleSettingsKey, "Lines"));
        graphGridMenu.addItem(kGraphGridDotsMenuId, "Dots", true, currentGridStyle == "dots");
        graphGridMenu.addItem(kGraphGridLinesMenuId, "Lines", true, currentGridStyle == "lines");
        graphGridMenu.addItem(kGraphGridOffMenuId, "Off", true, currentGridStyle == "off");
        retval.addSubMenu("Graph Grid", graphGridMenu);
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
void MainPanel::menuItemSelected(int menuItemID, int topLevelMenuIndex)
{
    ignoreUnused(topLevelMenuIndex);

    switch (menuItemID)
    {
    case OptionsUiScale75:
        setUiScalePercent(75);
        showToast("UI Scale 75%");
        break;
    case OptionsUiScale100:
        setUiScalePercent(100);
        showToast("UI Scale 100%");
        break;
    case OptionsUiScale125:
        setUiScalePercent(125);
        showToast("UI Scale 125%");
        break;
    case OptionsUiScale150:
        setUiScalePercent(150);
        showToast("UI Scale 150%");
        break;
    case OptionsUiScale175:
        setUiScalePercent(175);
        showToast("UI Scale 175%");
        break;
    case OptionsUiScale200:
        setUiScalePercent(200);
        showToast("UI Scale 200%");
        break;
    case OptionsUiScaleResetDefault:
        setUiScalePercent(UiScale::defaultPercent);
        showToast("UI Scale reset");
        break;
    case kGraphGridDotsMenuId:
    case kGraphGridLinesMenuId:
    case kGraphGridOffMenuId:
    {
        const auto style = menuItemID == kGraphGridDotsMenuId ? String("Dots")
                         : menuItemID == kGraphGridLinesMenuId
                             ? String("Lines")
                             : String("Off");
        SettingsManager::getInstance().setValue(kGraphGridStyleSettingsKey, style);
        if (auto* field = dynamic_cast<PluginField*>(viewport->getViewedComponent()))
            field->repaint();
        showToast("Graph grid: " + style);
        menuItemsChanged();
    }
    break;
    default:
        break;
    }
}

//------------------------------------------------------------------------------
ApplicationCommandTarget* MainPanel::getNextCommandTarget()
{
    return findFirstTargetParentComponent();
}

//------------------------------------------------------------------------------
void MainPanel::showPluginSearchWindowForVisualQa()
{
    if (auto* field = dynamic_cast<PluginField*>(viewport->getViewedComponent()))
        field->showPluginSearchWindowForVisualQa();
}

//------------------------------------------------------------------------------
void MainPanel::setStageViewModeForVisualQa(int modeIndex)
{
    if (stageView != nullptr)
        stageView->setViewModeForVisualQa(modeIndex);
}

//------------------------------------------------------------------------------
void MainPanel::setVirtualKeyboardCollapsedForVisualQa(bool shouldCollapse)
{
    virtualKeyboardCollapsed = shouldCollapse;
    if (keyboardToggleButton != nullptr)
        keyboardToggleButton->setToggleState(!virtualKeyboardCollapsed, dontSendNotification);
    resized();
    repaint();
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
                             ScratchCaptureToggle,
                             ScratchPanelOpen,
                             ScratchRevealFolder,
                             ScratchChooseFolder,
                             ScratchResetFolderDefault,
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
                             OptionsPluginBlacklist,
                             OptionsSnapToGrid};
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
    case ScratchCaptureToggle:
        result.setInfo("Start/Stop Scratch Capture", "Records synchronized raw and wet scratch WAV files.",
                       fileCategory, 0);
        result.addDefaultKeypress(L'r', ModifierKeys::commandModifier | ModifierKeys::shiftModifier);
        break;
    case ScratchPanelOpen:
        result.setInfo("Open Scratch Panel", "Shows recent scratch takes and capture status.", fileCategory, 0);
        break;
    case ScratchRevealFolder:
        result.setInfo("Reveal Scratch Ideas Folder", "Opens the scratch ideas folder.", fileCategory, 0);
        break;
    case ScratchChooseFolder:
        result.setInfo("Choose Scratch Ideas Folder", "Sets where new raw and wet scratch takes are saved.",
                       fileCategory, 0);
        result.setActive(!scratchRecorder.isRecording());
        break;
    case ScratchResetFolderDefault:
        result.setInfo("Reset Scratch Ideas Folder", "Restores scratch ideas to the default app data folder.",
                       fileCategory, 0);
        result.setActive(!scratchRecorder.isRecording());
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
    case OptionsSnapToGrid:
        result.setInfo("Snap to Grid", "Snap plugin nodes to a 20px grid when dragging.", optionsCategory, 0);
        result.setTicked(SettingsManager::getInstance().getBool("SnapToGrid", false));
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
    case ScratchCaptureToggle:
        toggleScratchCapture();
        break;
    case ScratchPanelOpen:
        openScratchPanel();
        break;
    case ScratchRevealFolder:
        revealScratchFolder();
        break;
    case ScratchChooseFolder:
        chooseScratchFolder();
        break;
    case ScratchResetFolderDefault:
        resetScratchFolderToDefault();
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

        dlg.setSize(400, 340);

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
    case OptionsSnapToGrid:
    {
        bool current = SettingsManager::getInstance().getBool("SnapToGrid", false);
        SettingsManager::getInstance().setValue("SnapToGrid", !current);
        showToast(!current ? "Snap to Grid enabled" : "Snap to Grid disabled");
    }
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

    const bool willLoadPatch = ((newPatch != currentPatch) && !reloadPatch) || !savePrev;

    if (willLoadPatch)
    {
        scratchRecorder.stopForPatchChange();
        refreshScratchControls();
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

        // Sync master gain sliders from MasterGainState (when not being dragged)
        {
            auto& gs = MasterGainState::getInstance();
            if (!inputGainSlider->isMouseButtonDown())
            {
                float inDb = gs.masterInputGainDb.load(std::memory_order_relaxed);
                if (std::abs((float)inputGainSlider->getValue() - inDb) > 0.01f)
                    inputGainSlider->setValue(inDb, dontSendNotification);
            }
            if (!outputGainSlider->isMouseButtonDown())
            {
                float outDb = gs.masterOutputGainDb.load(std::memory_order_relaxed);
                if (std::abs((float)outputGainSlider->getValue() - outDb) > 0.01f)
                    outputGainSlider->setValue(outDb, dontSendNotification);
            }
        }

        refreshScratchControls();
        break;
    case MidiAppTimer:
        CrashProtection::getInstance().pingWatchdog();
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
        // Drain deferred parameter changes from MIDI/OSC mapping (audio thread).
        {
            MidiAppFifo::PendingParamChange pc;
            while (midiAppFifo.readParamChange(pc))
            {
                if (pc.graph != &signalPath)
                    continue;

                auto node = pc.graph->getNodeForId(juce::AudioProcessorGraph::NodeID(pc.pluginId));
                if (node)
                {
                    if (pc.paramIndex == -1)
                    {
                        auto* bypassable = dynamic_cast<BypassableInstance*>(node->getProcessor());
                        if (bypassable)
                            bypassable->setBypass(pc.value > 0.5f);
                    }
                    else
                    {
                        auto* processor = node->getProcessor();
                        const int numParams = processor->getNumParameters();
                        if (pc.paramIndex >= 0 && pc.paramIndex < numParams)
                            processor->setParameter(pc.paramIndex, pc.value);
                    }
                }
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
        scratchRecorder.stopForDeviceChange();
        refreshScratchControls();

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
        refreshThemeDependentSurfaces();
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
    {
        signalThreadShouldExit();
        stopThread(2000);
    }
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
void MainPanel::enableVirtualMidiInput(bool val)
{
    PluginField* field = dynamic_cast<PluginField*>(viewport->getViewedComponent());

    field->enableVirtualMidiInput(val);

    SettingsManager::getInstance().setValue("VirtualMidiInput", val);
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
        String previousName = "";
        String nextName = "";
        StringArray patchNames;

        // Safety check for patching index
        if (currentPatch >= 0 && currentPatch < patches.size())
        {
            for (int i = 0; i < patches.size(); ++i)
                patchNames.add(patches[i] != nullptr ? patches[i]->getStringAttribute("name") : String());

            // Get current name from array to be sure
            currentName = patches[currentPatch]->getStringAttribute("name");

            if (currentPatch > 0)
                previousName = patches[currentPatch - 1]->getStringAttribute("name");

            // Get next patch if available
            if (currentPatch + 1 < patches.size())
            {
                nextName = patches[currentPatch + 1]->getStringAttribute("name");
            }
        }

        stageView->updatePatchInfo(currentName, previousName, nextName, currentPatch, patches.size(), patchNames);
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
         edBkgCol="0" labelText="CPU" editableSingleClick="0" editableDoubleClick="0"
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
                    class="ArrowButton" params="L&quot;tapTempoButton&quot;, 0.0, ColourScheme::getInstance().colours[&quot;Button Highlight&quot;].withAlpha(0.25f)"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif
