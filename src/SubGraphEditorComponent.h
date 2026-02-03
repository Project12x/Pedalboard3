/*
  ==============================================================================

    SubGraphEditorComponent.h

    Editor component for editing sub-graph/rack contents.
    This is a simplified version of PluginField that reuses the same
    PluginComponent, PluginPinComponent, and PluginConnection classes.
    Differentiated by a cyan/teal color hue instead of the main purple theme.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class SubGraphProcessor;
class PluginComponent;
class PluginConnection;
class PluginPinComponent;

//==============================================================================
/**
    Canvas for displaying and editing the rack's internal graph.
    Mirrors PluginField's architecture but uses a different color scheme.
*/
class SubGraphCanvas : public juce::Component, public juce::ChangeListener, public juce::ChangeBroadcaster
{
  public:
    SubGraphCanvas(SubGraphProcessor& processor, juce::KnownPluginList* pluginList);
    ~SubGraphCanvas() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;
    bool keyPressed(const juce::KeyPress& key) override;

    void changeListenerCallback(juce::ChangeBroadcaster* source) override;

    // Node/connection management (mirrors PluginField)
    void addFilter(int filterIndex);
    void deleteFilter(juce::AudioProcessorGraph::Node* node);
    void addConnection(juce::AudioProcessorGraph::NodeID srcId, int srcChannel,
                       juce::AudioProcessorGraph::NodeID destId, int destChannel);
    void deleteConnection(PluginConnection* connection);
    void rebuildGraph();

    // Pin-based connection wiring (mirrors PluginField)
    void addConnection(PluginPinComponent* source, bool connectAll);
    void dragConnection(int x, int y);
    void releaseConnection(int x, int y);

    // Pin/connection helpers
    juce::Component* getPinAt(int x, int y);
    void clearDoubleClickMessage()
    {
        displayDoubleClickMessage = false;
        repaint();
    }

  private:
    SubGraphProcessor& subGraph;
    juce::KnownPluginList* pluginList;

    juce::OwnedArray<PluginComponent> filterComponents;
    juce::OwnedArray<PluginConnection> connectionComponents;

    // Graph expansion/panning
    bool isPanning = false;
    juce::Point<int> panStartMouse;
    juce::Point<int> panStartScroll;

    // Zoom
    float zoomLevel = 1.0f;
    static constexpr float minZoom = 0.25f;
    static constexpr float maxZoom = 2.0f;

    // Dragging connection
    PluginConnection* draggingConnection = nullptr;

    bool displayDoubleClickMessage = true;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SubGraphCanvas)
};

//==============================================================================
/**
    Editor window/component for the SubGraphProcessor.
    Shows toolbar and the canvas.
*/
class SubGraphEditorComponent : public juce::AudioProcessorEditor
{
  public:
    SubGraphEditorComponent(SubGraphProcessor& processor);
    ~SubGraphEditorComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

  private:
    SubGraphProcessor& subGraphProcessor;

    std::unique_ptr<juce::Viewport> viewport;
    std::unique_ptr<SubGraphCanvas> canvas;
    std::unique_ptr<juce::Label> titleLabel;

    static constexpr int TOOLBAR_HEIGHT = 32;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SubGraphEditorComponent)
};
