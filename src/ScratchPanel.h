#pragma once

#include "ScratchRecorder.h"
#include "ScratchTakeLibrary.h"
#include "ScratchWaveformView.h"
#include "ScratchPlaybackTypes.h"

#include <JuceHeader.h>

class MainPanel;

class ScratchPanel final : public juce::Component,
                           private juce::Button::Listener,
                           private juce::Timer
{
public:
    explicit ScratchPanel(MainPanel& ownerToUse);
    ~ScratchPanel() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    class RecordButton final : public juce::Button
    {
    public:
        RecordButton();

        void setRecordingState(bool shouldShowRecording, bool shouldShowSaving);
        void paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted,
                         bool shouldDrawButtonAsDown) override;

    private:
        bool recording = false;
        bool saving = false;
    };

    class RecentTakesList final : public juce::Component, private juce::ListBoxModel
    {
    public:
        RecentTakesList(MainPanel&, ScratchWaveformCache&);
        ~RecentTakesList() override { list.setModel(nullptr); }
        void setSnapshot(std::shared_ptr<const ScratchLibrarySnapshot>, const juce::File& selection);
        void resized() override { list.setBounds(getLocalBounds()); }
        void paint(juce::Graphics&) override;
        std::function<void(const juce::File&)> onSelection;
    private:
        class Row;
        int getNumRows() override;
        void paintListBoxItem(int, juce::Graphics&, int, int, bool) override {}
        juce::Component* refreshComponentForRow(int, bool, juce::Component*) override;
        void selectedRowsChanged(int) override;
        juce::Component::SafePointer<MainPanel> owner;
        ScratchWaveformCache& cache;
        std::shared_ptr<const ScratchLibrarySnapshot> snapshot;
        bool updating = false;
        juce::ListBox list{"Scratch takes", this};
    };

    struct Layout
    {
        juce::Rectangle<int> header;
        juce::Rectangle<int> hero;
        juce::Rectangle<int> record;
        juce::Rectangle<int> timer;
        juce::Rectangle<int> status;
        juce::Rectangle<int> scope;
        juce::Rectangle<int> transport;
        juce::Rectangle<int> context;
        juce::Rectangle<int> destination;
        juce::Rectangle<int> destinationText;
        juce::Rectangle<int> choose;
        juce::Rectangle<int> reset;
        juce::Rectangle<int> reveal;
        juce::Rectangle<int> recentHeader;
        juce::Rectangle<int> recent;
    };

    void buttonClicked(juce::Button* button) override;
    void timerCallback() override;
    void refresh();
    void updateRecentTakesList();
    void playSelected(ScratchPlaybackMode);
    Layout calculateLayout() const;

    juce::Component::SafePointer<MainPanel> owner;
    RecordButton recordButton;
    juce::TextButton chooseButton{"chooseButton"};
    juce::TextButton resetButton{"resetButton"};
    juce::TextButton revealButton{"revealButton"};
    juce::TextButton refreshButton{"Refresh"};
    juce::TextButton wetButton{"Original WET"}, rawButton{"Current rig"}, pauseButton{"Pause"}, stopButton{"Stop"};
    juce::ToggleButton loopButton{"Loop"};
    juce::Slider previewVolume;
    juce::Label playbackLabel;
    juce::Label statusLabel{"statusLabel", "Ready"};
    juce::Label elapsedLabel{"elapsedLabel", "00:00"};
    ScratchWaveformCache waveformCache;
    ScratchWaveformView selectedWaveform{waveformCache};
    juce::Viewport waveformViewport{"Selected take audio"};
    RecentTakesList recentTakesList;
    juce::File selectedTakeDirectory;
    ScratchRecorderStatus lastStatus;
    std::shared_ptr<const ScratchLibrarySnapshot> librarySnapshot;
    std::vector<ScratchTake> libraryTakes;
};
