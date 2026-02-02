/*
  ==============================================================================

    SubGraphEditorComponent.h

    Editor component for editing sub-graph/rack contents.
    Displays the internal AudioProcessorGraph of a SubGraphProcessor,
    showing nodes and connections with a Reason-style front/back toggle.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class SubGraphProcessor;

//==============================================================================
/**
    Simple node component for displaying a processor in the sub-graph editor.
*/
class RackNodeComponent : public juce::Component, public juce::ChangeBroadcaster
{
  public:
    RackNodeComponent(juce::AudioProcessorGraph::Node::Ptr node, bool isIONode = false);
    ~RackNodeComponent() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;

    juce::AudioProcessorGraph::NodeID getNodeId() const { return nodePtr->nodeID; }
    juce::AudioProcessorGraph::Node::Ptr getNode() const { return nodePtr; }

    // Pin positions for connection drawing
    juce::Point<int> getInputPinPosition(int channel) const;
    juce::Point<int> getOutputPinPosition(int channel) const;
    int getNumInputPins() const;
    int getNumOutputPins() const;

    // Context menu action flags
    bool isDeleteRequested() const { return deleteRequested; }
    bool isDisconnectRequested() const { return disconnectRequested; }
    void clearActionFlags()
    {
        deleteRequested = false;
        disconnectRequested = false;
    }
    bool isIO() const { return isIONode; }

  private:
    juce::AudioProcessorGraph::Node::Ptr nodePtr;
    bool isIONode;
    juce::Point<int> dragStart;
    bool deleteRequested = false;
    bool disconnectRequested = false;

    static constexpr int PIN_SIZE = 10;
    static constexpr int PIN_SPACING = 18;
    static constexpr int HEADER_HEIGHT = 24;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RackNodeComponent)
};

//==============================================================================
/**
    Main canvas for displaying and editing the rack's internal graph.
    Supports front (controls) and back (cable routing) views.
*/
class SubGraphCanvas : public juce::Component, public juce::ChangeListener
{
  public:
    enum class ViewMode
    {
        Front, // Plugin/control view
        Back   // Cable routing view (Reason-style)
    };

    SubGraphCanvas(SubGraphProcessor& processor);
    ~SubGraphCanvas() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;

    void changeListenerCallback(juce::ChangeBroadcaster* source) override;

    void setViewMode(ViewMode mode);
    ViewMode getViewMode() const { return viewMode; }

    void rebuildNodes();

  private:
    void drawConnections(juce::Graphics& g);
    void drawBackPanelConnections(juce::Graphics& g);
    void drawDraggingConnection(juce::Graphics& g);

    // Pin hit detection
    RackNodeComponent* findNodeAt(juce::Point<int> pos);
    bool hitTestInputPin(RackNodeComponent* node, juce::Point<int> pos, int& pinIndex);
    bool hitTestOutputPin(RackNodeComponent* node, juce::Point<int> pos, int& pinIndex);

    SubGraphProcessor& subGraph;
    juce::OwnedArray<RackNodeComponent> nodeComponents;
    ViewMode viewMode = ViewMode::Front;

    // Connection dragging state
    bool isDraggingConnection = false;
    bool draggingFromOutput = true; // true = from output, false = from input
    juce::AudioProcessorGraph::NodeID dragSourceNodeId;
    int dragSourceChannel = -1;
    juce::Point<int> dragEndPos;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SubGraphCanvas)
};

//==============================================================================
/**
    Editor window/component for the SubGraphProcessor.
    Shows toolbar with view mode toggle and the rack canvas.
*/
class SubGraphEditorComponent : public juce::AudioProcessorEditor, public juce::Button::Listener
{
  public:
    SubGraphEditorComponent(SubGraphProcessor& processor);
    ~SubGraphEditorComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void buttonClicked(juce::Button* button) override;

  private:
    SubGraphProcessor& subGraphProcessor;

    std::unique_ptr<juce::Viewport> viewport;
    std::unique_ptr<SubGraphCanvas> canvas;
    std::unique_ptr<juce::TextButton> frontBackButton;
    std::unique_ptr<juce::TextButton> addPluginButton;
    std::unique_ptr<juce::Label> titleLabel;

    static constexpr int TOOLBAR_HEIGHT = 40;

    void showPluginMenu();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SubGraphEditorComponent)
};
