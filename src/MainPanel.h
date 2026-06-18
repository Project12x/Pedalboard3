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

#ifndef __JUCER_HEADER_MAINPANEL_MAINPANEL_89D8C0B__
#define __JUCER_HEADER_MAINPANEL_MAINPANEL_89D8C0B__

//[Headers]     -- You can add your own extra header files here --
#include "ColourScheme.h"
#include "DeviceMeterTap.h"
#include "FilterGraph.h"
#include "FontManager.h"
#include "MasterBusProcessor.h"
#include "MasterGainState.h"
#include "MidiAppFifo.h"
#include "NiallsSocketLib/UDPSocket.h"
#include "PluginField.h"
#include "ScratchRecorder.h"

#include <JuceHeader.h>
#include <array>
#include <atomic>

class PluginListWindow;
class StageView;
class TunerProcessor;

//[/Headers]

//==============================================================================
/// AudioProcessorPlayer subclass that applies master gain and taps device
/// buffers for VU metering. All operations are RT-safe: pre-allocated buffers,
/// atomic reads, no allocations or locks in the audio callback.
class MeteringProcessorPlayer : public AudioProcessorPlayer
{
  public:
    static constexpr int MaxChannels = 16;
    static constexpr int MaxCallbackBlockSize = 8192;

    int getCallbackBoundsRejectCount() const noexcept
    {
        return callbackBoundsRejectCount.load(std::memory_order_relaxed);
    }

    void setScratchRecorder(ScratchRecorder* recorderToUse) noexcept
    {
        scratchRecorder.store(recorderToUse, std::memory_order_release);
    }

    void audioDeviceAboutToStart(AudioIODevice* device) override
    {
        AudioProcessorPlayer::audioDeviceAboutToStart(device);
        if (device != nullptr)
        {
            int maxCh = jmax(device->getActiveInputChannels().countNumberOfSetBits(),
                             device->getActiveOutputChannels().countNumberOfSetBits());
            inputGainBuffer.setSize(jmax(maxCh, 2), device->getCurrentBufferSizeSamples() * 2);
            masterBusBuffer.setSize(jmax(maxCh, 2), device->getCurrentBufferSizeSamples() * 2);

            // Prepare master bus insert rack
            auto& gainState = MasterGainState::getInstance();
            gainState.getMasterBus().prepare(device->getCurrentSampleRate(), device->getCurrentBufferSizeSamples());

            // Initialize gain smoothing at device sample rate
            gainState.prepareSmoothing(device->getCurrentSampleRate());
        }
    }

    void audioDeviceIOCallbackWithContext(const float* const* inputChannelData, int numInputChannels,
                                          float* const* outputChannelData, int numOutputChannels, int numSamples,
                                          const AudioIODeviceCallbackContext& context) override
    {
        auto& gainState = MasterGainState::getInstance();
        auto* recorder = scratchRecorder.load(std::memory_order_acquire);

        if (numSamples <= 0)
            return;

        if (numSamples > MaxCallbackBlockSize)
        {
            callbackBoundsRejectCount.fetch_add(1, std::memory_order_relaxed);
            for (int ch = 0; ch < numOutputChannels; ++ch)
                if (outputChannelData[ch] != nullptr)
                    FloatVectorOperations::clear(outputChannelData[ch], numSamples);
            return;
        }

        if (recorder != nullptr)
            recorder->writeRawBlock(inputChannelData, numInputChannels, numSamples);

        // Update smoothed gain targets from atomic dB values (once per block)
        gainState.updateSmoothedTargets();

        // Pre-compute smoothed master input gain ramp (one value per sample).
        // This ensures the ramp advances at the correct rate regardless of
        // how many channels reference it.
        for (int i = 0; i < numSamples; ++i)
            smoothedInputRamp[i] = gainState.smoothedInputGain.getNextValue();

        // Apply per-channel input gain. inputChannelData is const, so we copy
        // to a pre-allocated buffer and multiply by smoothed master * channel gain.
        const float* const* actualInput = inputChannelData;
        int actualInputChannels = numInputChannels;
        {
            bool anyInputGain = false;
            int chCount = jmin(numInputChannels, (int)MaxChannels);
            for (int ch = 0; ch < chCount; ++ch)
            {
                float channelGain = gainState.getInputChannelGainLinear(ch);
                bool needsGain = gainState.smoothedInputGain.isSmoothing() ||
                                 std::abs(channelGain * smoothedInputRamp[numSamples - 1] - 1.0f) > 0.0001f;

                if (inputChannelData[ch] != nullptr && needsGain && numSamples <= inputGainBuffer.getNumSamples())
                {
                    float* dest = inputGainBuffer.getWritePointer(ch);
                    const float* src = inputChannelData[ch];
                    for (int i = 0; i < numSamples; ++i)
                        dest[i] = src[i] * smoothedInputRamp[i] * channelGain;
                    gainedInputPtrs[ch] = dest;
                    anyInputGain = true;
                }
                else
                {
                    gainedInputPtrs[ch] = inputChannelData[ch];
                }
            }
            if (anyInputGain)
            {
                actualInput = gainedInputPtrs;
                actualInputChannels = chCount;
                if (numInputChannels > MaxChannels)
                    callbackBoundsRejectCount.fetch_add(1, std::memory_order_relaxed);
            }
        }

        // Process graph with (possibly gained) input
        AudioProcessorPlayer::audioDeviceIOCallbackWithContext(actualInput, actualInputChannels, outputChannelData,
                                                               numOutputChannels, numSamples, context);

        // Process master bus insert rack (between graph output and output gain)
        // Only when user has inserted plugins - empty rack is pure passthrough
        auto& masterBus = gainState.getMasterBus();
        if (masterBus.hasPluginsCached() && !masterBus.isBypassed())
        {
            int chCount = jmin(numOutputChannels, masterBusBuffer.getNumChannels());
            if (chCount > 0 && numSamples <= masterBusBuffer.getNumSamples())
            {
                // Copy output to pre-allocated buffer for processBlock
                for (int ch = 0; ch < chCount; ++ch)
                {
                    if (outputChannelData[ch] != nullptr)
                        masterBusBuffer.copyFrom(ch, 0, outputChannelData[ch], numSamples);
                    else
                        masterBusBuffer.clear(ch, 0, numSamples);
                }

                MidiBuffer emptyMidi;
                masterBus.processBlock(masterBusBuffer, emptyMidi);

                // Copy processed data back to output
                for (int ch = 0; ch < chCount; ++ch)
                    if (outputChannelData[ch] != nullptr)
                        FloatVectorOperations::copy(outputChannelData[ch], masterBusBuffer.getReadPointer(ch),
                                                    numSamples);
            }
        }

        // Pre-compute smoothed master output gain ramp (same pattern as input)
        for (int i = 0; i < numSamples; ++i)
            smoothedOutputRamp[i] = gainState.smoothedOutputGain.getNextValue();

        // Apply per-channel output gain with smoothing. Output buffers are writable.
        const int outputGainChannels = jmin(numOutputChannels, (int)MaxChannels);
        for (int ch = 0; ch < outputGainChannels; ++ch)
        {
            float channelGain = gainState.getOutputChannelGainLinear(ch);
            bool needsGain = gainState.smoothedOutputGain.isSmoothing() ||
                             std::abs(channelGain * smoothedOutputRamp[numSamples - 1] - 1.0f) > 0.0001f;

            if (outputChannelData[ch] != nullptr && needsGain)
            {
                float* data = outputChannelData[ch];
                for (int i = 0; i < numSamples; ++i)
                    data[i] *= smoothedOutputRamp[i] * channelGain;
            }
        }

        if (recorder != nullptr)
            recorder->writeWetBlock(outputChannelData, numOutputChannels, numSamples);

        // Tap levels for VU metering (post-gain)
        if (auto* limiter = SafetyLimiterProcessor::getInstance())
        {
            limiter->updateInputLevelsFromDevice(actualInput, actualInputChannels, numSamples);
            limiter->updateOutputLevelsFromDevice(outputChannelData, numOutputChannels, numSamples);
        }
    }

  private:
    AudioBuffer<float> inputGainBuffer; // Pre-allocated in audioDeviceAboutToStart
    AudioBuffer<float> masterBusBuffer; // Pre-allocated for master insert rack
    std::array<float, MaxCallbackBlockSize> smoothedInputRamp{};
    std::array<float, MaxCallbackBlockSize> smoothedOutputRamp{};
    const float* gainedInputPtrs[MaxChannels] = {};
    std::atomic<int> callbackBoundsRejectCount{0};
    std::atomic<ScratchRecorder*> scratchRecorder{nullptr};
};

//==============================================================================
/**
                                                                    //[Comments]
    An auto-generated component, created by the Jucer.

    Describe your class and how it works here!
                                                                    //[/Comments]
*/
class MainPanel : public Component,
                  public MenuBarModel,
                  public ApplicationCommandTarget,
                  public MultiTimer,
                  public ChangeListener,
                  public FileBasedDocument,
                  public Thread,
                  public FileDragAndDropTarget,
                  public TextEditor::Listener,
                  public Button::Listener,
                  public ComboBox::Listener,
                  public Slider::Listener,
                  public MidiKeyboardState::Listener
{
  public:
    //==============================================================================
    MainPanel(ApplicationCommandManager* appManager);
    ~MainPanel();

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.

    ///	For the menu bar.
    StringArray getMenuBarNames();
    ///	For the menu bar.
    PopupMenu getMenuForIndex(int topLevelMenuIndex, const String& menuName);
    ///	For the menu bar.
    void menuItemSelected(int menuItemID, int topLevelMenuIndex);

    ///	For keyboard shortcuts etc.
    ApplicationCommandTarget* getNextCommandTarget();
    ///	For keyboard shortcuts etc.
    void getAllCommands(Array<CommandID>& commands);
    ///	For keyboard shortcuts etc.
    void getCommandInfo(CommandID commandID, ApplicationCommandInfo& result);
    ///	For keyboard shortcuts etc.
    bool perform(const InvocationInfo& info);

    ///	So we know what the commandManager is.
    void setCommandManager(ApplicationCommandManager* manager);

    ///	Used to add an ApplicationCommand to the queue so it'll get called in
    /// the message thread.
    void invokeCommandFromOtherThread(CommandID commandID);
    ///	Used to update the tempo from a non-message thread.
    void updateTempoFromOtherThread(double tempo);

    ///	Used to update the CPU usage slider.
    void timerCallback(int timerId);
    ///	Used to save the plugin list.
    void changeListenerCallback(ChangeBroadcaster* changedObject);
    ///	Used to update the tempo.
    void textEditorTextChanged(TextEditor& editor);
    ///	Used to update the tempo.
    void textEditorReturnKeyPressed(TextEditor& editor);
    bool keyPressed(const KeyPress& key) override;

    ///	Used to accept dragged .pdl files.
    bool isInterestedInFileDrag(const StringArray& files);
    ///	Used to accept dragged .pdl files.
    void filesDropped(const StringArray& files, int x, int y);

    ///	Used to update the socket's port.
    void setSocketPort(const String& port);
    ///	Used to update the socket's multicast address input.
    void setSocketMulticast(const String& address);

    ///	Enables/disables the audio input.
    void enableAudioInput(bool val);
    ///	Enables/disables the MIDI input.
    void enableMidiInput(bool val);
    ///	Enables/disables the OSC input.
    void enableOscInput(bool val);
    ///	Enables/disables the Virtual MIDI input.
    void enableVirtualMidiInput(bool val);
    ///	Sets whether to automatically open the mappings window or not.
    void setAutoMappingsWindow(bool val);

    ///	Where the app listens for OSC messages.
    void run();

    ///	Returns the title of the current set of patches.
    String getDocumentTitle();
    ///	Loads a set of patches from the passed-in file.
    Result loadDocument(const File& file);
    ///	Tries to save the current patches to the passed-in file.
    Result saveDocument(const File& file);
    ///	Returns the last set of patches opened.
    File getLastDocumentOpened();
    ///	Saves the last set of patches opened.
    void setLastDocumentOpened(const File& file);

    ///	Used to update patches from PatchOrganiser.
    ComboBox* getPatchComboBox() { return patchComboBox; };

    ///	Used to clear the listWindow variable.
    void setListWindow(PluginListWindow* win) { listWindow = win; };

    ///	Adds a patch from its XmlElement.
    void addPatch(XmlElement* patch);
    ///	Saves the current patch to our patches array.
    void savePatch();
    ///	Duplicates the indexed patch.
    void duplicatePatch(int index);
    ///	Called from PatchOrganiser when it's moving patches up/down.
    void nextSwitchDoNotSavePrev();
    ///	Triggers a patch change from a Midi Program Change message.
    void switchPatchFromProgramChange(int newPatch);
    ///	Returns the index of the current patch.
    int getCurrentPatch() const { return patchComboBox->getSelectedId() - 1; };
    ///	Returns the name of the current patch.
    String getCurrentPatchName() const;
    ///	Returns the total number of patches.
    int getPatchCount() const;

    /// Helper method to update Stage View state
    void updateStageView();
    /// Refreshes plugin pool definitions to match the current patch list.
    void refreshPluginPoolDefinitions();
    /// Updates a single patch definition in the plugin pool.
    void updatePluginPoolDefinition(int patchIndex, const XmlElement* patch);
    /// Returns the persisted app-owned Pedalboard UI scale.
    int getUiScalePercent() const;
    /// Applies and persists the app-owned Pedalboard UI scale.
    void setUiScalePercent(int percent);
    /// Syncs the footer UI scale selector from persisted settings.
    void syncUiScaleComboBoxFromSettings();
    /// Applies a built-in colour scheme immediately from lightweight theme swatches.
    void applyColourSchemePreset(const String& presetName, bool showToastMessage = true);
    /// Refreshes theme-dependent surfaces after the active colour map changes.
    void refreshThemeDependentSurfaces();
    ScratchTakeContext createScratchTakeContext() const;
    void toggleScratchCapture();
    void openScratchPanel();
    void revealScratchFolder();
    void chooseScratchFolder();
    void resetScratchFolderToDefault();
    void previewScratchWetTake(const ScratchTake& take);
    void reampScratchRawTake(const ScratchTake& take);
    void revealScratchTake(const ScratchTake& take);
    void refreshScratchControls();
    ScratchRecorderStatus getScratchRecorderStatus() const;
    /// Refreshes MIDI mapping settings cached for callback-safe dispatch.
    void refreshMidiMappingRealtimeSettings();

    ///	Toggles Stage Mode (fullscreen performance view).
    void toggleStageMode();
    ///	Returns true if Stage Mode is active.
    bool isStageMode() const { return stageView != nullptr; };
    ///	Returns the application command manager for invoking commands.
    ApplicationCommandManager* getApplicationCommandManager() const { return commandManager; };
    /// Shows the plugin search window for deterministic visual QA captures.
    void showPluginSearchWindowForVisualQa();
    /// Sets Stage Mode subview for deterministic visual QA captures.
    void setStageViewModeForVisualQa(int modeIndex);
    /// Sets the virtual keyboard deck state for deterministic visual QA captures.
    void setVirtualKeyboardCollapsedForVisualQa(bool shouldCollapse);
    /// Writes isolated graph-node PNGs for visual comparison against mockups.
    void writeNodeSnapshotsForVisualQa(const File& outputDir);

    ///	Returns the PluginField's MidiMappingManager.
    MidiMappingManager* getMidiMappingManager()
    {
        return dynamic_cast<PluginField*>(viewport->getViewedComponent())->getMidiManager();
    };
    ///	Returns the PluginField's AppMappingManager.
    OscMappingManager* getOscMappingManager()
    {
        return dynamic_cast<PluginField*>(viewport->getViewedComponent())->getOscManager();
    };

    ///	Constants for the various menu options.
    /*!
            NOTE: Add new constants TO THE END OF THE LIST, or the user's saved
            mappings will get screwed up!
     */
    enum
    {
        FileNew = 1,
        FileOpen,
        FileSave,
        FileSaveAs,
        FileSaveAsDefault,
        FileResetDefault,
        FileExit,
        EditDeleteConnection,
        EditOrganisePatches,
        OptionsAudio,
        OptionsPluginList,
        OptionsPreferences,
        OptionsColourSchemes,
        OptionsKeyMappings,
        HelpAbout,
        PatchNextPatch,
        PatchPrevPatch,
        TransportPlay,
        TransportRtz,
        TransportTapTempo,
        EditUserPresetManagement,
        HelpDocumentation,
        HelpLog,
        EditUndo,
        EditRedo,
        EditPanic,
        ToggleStageMode,
        OptionsPluginBlacklist,
        OptionsSnapToGrid,
        OptionsUiScale75,
        OptionsUiScale100,
        OptionsUiScale125,
        OptionsUiScale150,
        OptionsUiScale175,
        OptionsUiScale200,
        OptionsUiScaleResetDefault,
        ScratchCaptureToggle,
        ScratchPanelOpen,
        ScratchRevealFolder,
        ScratchChooseFolder,
        ScratchResetFolderDefault
    };

    //[/UserMethods]

    void paint(Graphics& g);
    void resized();
    void buttonClicked(Button* buttonThatWasClicked);
    void comboBoxChanged(ComboBox* comboBoxThatHasChanged);
    void sliderValueChanged(Slider* sliderThatWasMoved);

    //==============================================================================

  private:
    //[UserVariables]   -- You can add your own custom variables in this
    // section.

    ///	Helper method. Switches patches.
    /*!
            \param newPatch Index of the new patch to load.
            \param savePrev Saves the current patch in the process.
            \param reloadPatch Reloads the current patch.
     */
    void switchPatch(int newPatch, bool savePrev = true, bool reloadPatch = false);

    ///	The IDs of the three timers.
    enum
    {
        CpuTimer = 0,
        MidiAppTimer,
        ProgramChangeTimer
    };

    ///	The last opened document.
    static File lastDocument;

    ///	Our copy of the commandManager.
    ApplicationCommandManager* commandManager;
    ///	Used to display tooltips.
    TooltipWindow tooltips;

    ///	The sound card settings.
    AudioDeviceManager deviceManager;
    ///	The graph representing the audio signal path.
    FilterGraph signalPath;
    ThreadedWavSinkFactory scratchSinkFactory;
    ScratchRecorder scratchRecorder{scratchSinkFactory};
    ///	Object used to 'play' the signalPath object (with output metering).
    MeteringProcessorPlayer graphPlayer;
    ///	The list of plugins the user can load.
    KnownPluginList pluginList;
    ///	The socket we listen for OSC messages on.
    UDPSocket sock;

    ///	Used to protect sock when we change port/multicast address.
    CriticalSection sockCritSec;

    ///	Window to display/edit the list of possible plugins.
    PluginListWindow* listWindow;

    ///	Stage Mode overlay (fullscreen performance view).
    std::unique_ptr<StageView> stageView;
    ///	Pointer to active TunerProcessor for Stage Mode integration.
    TunerProcessor* activeTuner = nullptr;

    /// Dedicated player for the global tuner (bypasses signal chain)
    AudioProcessorPlayer tunerPlayer;
    /// The global tuner processor
    std::unique_ptr<TunerProcessor> globalTuner;

    /// Device-level audio metering for I/O nodes
    DeviceMeterTap deviceMeterTap;

    ///	The currently-loaded patches.
    Array<XmlElement*> patches;
    ///	The index of the current patch.
    int currentPatch;
    ///	Used to switch patches via Midi Program Changes.
    int programChangePatch;

    ///	The two drawables we use for the playButton.
    std::unique_ptr<Drawable> playImage;
    std::unique_ptr<Drawable> pauseImage;
    ///	Whether the playPauseButton is currently displaying the play icon.
    bool playing;

    ///	Used to compensate for the stupid way the combo box handles the user
    /// editing its text.
    int lastCombo;

    ///	Used to ensure patches don't get overwritten when they're being
    /// re-ordered in PatchOrganiser.
    bool doNotSaveNextPatch;

    ///	Used for tap tempo when the user does it via keyboard.
    int64 lastTempoTicks;

    ///	Used to pass messages from the audio thread to the message thread.
    MidiAppFifo midiAppFifo;

    ///	Simple Component to pass to warningBox.
    class ProgramChangeWarning : public Component
    {
      public:
        ///	Constructor.
        ProgramChangeWarning() : index(0) { setSize(250, 150); };
        ///	Destructor.
        ~ProgramChangeWarning() {};

        ///	Draws the warning.
        void paint(Graphics& g)
        {
            String tempstr;
            Font smallFont = FontManager::getInstance().getDisplayFont(24.0f);
            Font bigFont = FontManager::getInstance().getDisplayFont(48.0f);

            g.setColour(ColourScheme::getInstance().colours["Text Colour"]);

            g.setFont(smallFont);
            g.drawText("Out of bounds", 0, 0, 250, 50, Justification(Justification::centred), false);
            g.drawText("MIDI Program Change", 0, 24, 250, 50, Justification(Justification::centred), false);
            g.drawText("received:", 0, 48, 250, 50, Justification(Justification::centred), false);

            tempstr << index;
            g.setFont(bigFont);
            g.drawText(tempstr, 0, 88, 250, 50, Justification(Justification::centred), false);
        };
        ///	Sets the offending index.
        void setIndex(int i) { index = i; };

      private:
        ///	The offending index.
        int index;
    };
    ///	Used to inform the user when the user does a MIDI program change outside
    /// the limits of the patch list.
    ProgramChangeWarning warningText;
    ///	Used to inform the user when the user does a MIDI program change outside
    /// the limits of the patch list.
    std::unique_ptr<CallOutBox> warningBox;

    /// Toast notification using JUCE's BubbleMessageComponent
    std::unique_ptr<BubbleMessageComponent> toastBubble;
    void showToast(const String& message);
    std::unique_ptr<FileChooser> scratchFolderChooser;

    /// Virtual MIDI keyboard state and component
    MidiKeyboardState keyboardState;
    std::unique_ptr<MidiKeyboardComponent> virtualKeyboard;
    static constexpr int keyboardHeight = 86;          // control strip + keys
    static constexpr int keyboardCollapsedHeight = 28; // slim restore strip
    bool virtualKeyboardCollapsed = false;

    // Keyboard control strip
    std::unique_ptr<TextButton> keyboardToggleButton;
    std::unique_ptr<TextButton> octaveDownButton;
    std::unique_ptr<TextButton> octaveUpButton;
    std::unique_ptr<Label> octaveLabel;
    std::unique_ptr<Slider> velocitySlider;
    std::unique_ptr<Label> velocityLabel;
    std::unique_ptr<TextButton> sustainButton;
    std::unique_ptr<Component> themeSwitcher;
    int getVirtualKeyboardDeckHeight() const;
    void paintVirtualKeyboardDeck(Graphics& g);
    void updateOctaveDisplay();

    /// MidiKeyboardState::Listener callbacks
    void handleNoteOn(MidiKeyboardState* source, int midiChannel, int midiNoteNumber, float velocity) override;
    void handleNoteOff(MidiKeyboardState* source, int midiChannel, int midiNoteNumber, float velocity) override;

    //[/UserVariables]

    //==============================================================================
    Label* patchLabel;
    TextButton* prevPatch;
    TextButton* nextPatch;
    ComboBox* patchComboBox;
    Viewport* viewport;
    Slider* cpuSlider;
    Label* cpuLabel;
    DrawableButton* playButton;
    DrawableButton* rtzButton;
    Label* tempoLabel;
    TextEditor* tempoEditor;
    ArrowButton* tapTempoButton;
    TextButton* organiseButton;
    TextButton* fitButton;
    Slider* inputGainSlider;
    Slider* outputGainSlider;
    Label* inputGainLabel;
    Label* outputGainLabel;
    TextButton* masterInsertButton;
    Label* uiScaleFooterLabel;
    ComboBox* uiScaleFooterComboBox;
    TextButton* scratchRecordButton = nullptr;
    TextButton* scratchPanelButton = nullptr;
    Label* scratchStatusLabel = nullptr;

    //==============================================================================
    // (prevent copy constructor and operator= being generated..)
    MainPanel(const MainPanel&);
    const MainPanel& operator=(const MainPanel&);
};

#endif // __JUCER_HEADER_MAINPANEL_MAINPANEL_89D8C0B__
