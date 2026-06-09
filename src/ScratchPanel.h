#pragma once

#include "ScratchRecorder.h"

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

    class RecentTakesList final : public juce::Component
    {
    public:
        explicit RecentTakesList(MainPanel& ownerToUse);

        void setTakes(std::vector<ScratchTake> takesToShow);
        int getPreferredHeight() const;
        void paint(juce::Graphics& g) override;
        void mouseDown(const juce::MouseEvent& event) override;

    private:
        enum class Action
        {
            None,
            Play,
            Reamp,
            Reveal
        };

        struct Hit
        {
            int takeIndex = -1;
            Action action = Action::None;
        };

        juce::Rectangle<int> getRowBounds(int index) const;
        juce::Rectangle<int> getActionBounds(int index, Action action) const;
        Hit hitTest(juce::Point<int> point) const;

        juce::Component::SafePointer<MainPanel> owner;
        std::vector<ScratchTake> takes;
    };

    struct Layout
    {
        juce::Rectangle<int> header;
        juce::Rectangle<int> hero;
        juce::Rectangle<int> record;
        juce::Rectangle<int> timer;
        juce::Rectangle<int> status;
        juce::Rectangle<int> scope;
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
    Layout calculateLayout() const;

    juce::Component::SafePointer<MainPanel> owner;
    RecordButton recordButton;
    juce::TextButton chooseButton{"chooseButton"};
    juce::TextButton resetButton{"resetButton"};
    juce::TextButton revealButton{"revealButton"};
    juce::Label statusLabel{"statusLabel", "Ready"};
    juce::Label elapsedLabel{"elapsedLabel", "00:00"};
    juce::Viewport recentTakesViewport{"recentTakesViewport"};
    RecentTakesList recentTakesList;
    ScratchRecorderStatus lastStatus;
};
