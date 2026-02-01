/*
  ==============================================================================

    SubGraphEditorComponent.cpp

    Implementation of the rack editor component.

  ==============================================================================
*/

#include "SubGraphEditorComponent.h"

#include "SubGraphProcessor.h"

#include <spdlog/spdlog.h>

using namespace juce;

//==============================================================================
// RackNodeComponent
//==============================================================================

RackNodeComponent::RackNodeComponent(juce::AudioProcessorGraph::Node::Ptr node, bool isIO)
    : nodePtr(node), isIONode(isIO)
{
    setSize(140, 80);
}

void RackNodeComponent::paint(Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Safety check
    if (!nodePtr || !nodePtr->getProcessor())
    {
        g.setColour(Colour(0xFF4A4A4A));
        g.fillRoundedRectangle(bounds, 4.0f);
        g.setColour(Colours::red);
        g.drawText("Invalid Node", bounds, Justification::centred, true);
        return;
    }

    // Background
    if (isIONode)
    {
        // I/O nodes have a different look
        g.setColour(Colour(0xFF2A3A4A));
    }
    else
    {
        g.setColour(Colour(0xFF3A3A4A));
    }
    g.fillRoundedRectangle(bounds, 4.0f);

    // Border
    g.setColour(Colour(0xFF5A6A7A));
    g.drawRoundedRectangle(bounds.reduced(0.5f), 4.0f, 1.0f);

    // Header bar
    auto headerBounds = bounds.removeFromTop(HEADER_HEIGHT);
    g.setColour(Colour(0xFF4A5A6A));
    g.fillRoundedRectangle(headerBounds.reduced(1.0f), 3.0f);

    // Title
    g.setColour(Colours::white);
    g.setFont(Font(FontOptions(13.0f).withStyle("Bold")));
    String name = nodePtr->getProcessor()->getName();
    g.drawText(name, headerBounds.reduced(4.0f, 0.0f), Justification::centredLeft, true);

    // Draw input pins (left side)
    int numInputs = getNumInputPins();
    for (int i = 0; i < numInputs; ++i)
    {
        auto pinPos = getInputPinPosition(i);
        g.setColour(i >= nodePtr->getProcessor()->getTotalNumInputChannels() ? Colour(0xFFAA6688) // MIDI pin
                                                                             : Colour(0xFF6688AA));
        g.fillEllipse(pinPos.x - PIN_SIZE / 2, pinPos.y - PIN_SIZE / 2, PIN_SIZE, PIN_SIZE);
    }

    // Draw output pins (right side)
    int numOutputs = getNumOutputPins();
    for (int i = 0; i < numOutputs; ++i)
    {
        auto pinPos = getOutputPinPosition(i);
        g.setColour(i >= nodePtr->getProcessor()->getTotalNumOutputChannels() ? Colour(0xFFAA6688) // MIDI pin
                                                                              : Colour(0xFF88AA66));
        g.fillEllipse(pinPos.x - PIN_SIZE / 2, pinPos.y - PIN_SIZE / 2, PIN_SIZE, PIN_SIZE);
    }
}

void RackNodeComponent::resized()
{
    // Nothing to resize internally
}

void RackNodeComponent::mouseDown(const MouseEvent& e)
{
    dragStart = e.getPosition();
    toFront(true);
}

void RackNodeComponent::mouseDrag(const MouseEvent& e)
{
    if (!nodePtr)
        return;

    auto parentPos = e.getEventRelativeTo(getParentComponent()).getPosition();
    auto newPos = parentPos - dragStart;

    // Clamp to canvas bounds
    newPos.x = std::max(0, newPos.x);
    newPos.y = std::max(0, newPos.y);

    setTopLeftPosition(newPos);

    // Update node properties for persistence
    nodePtr->properties.set("x", (double)newPos.x);
    nodePtr->properties.set("y", (double)newPos.y);

    sendChangeMessage();
}

Point<int> RackNodeComponent::getInputPinPosition(int channel) const
{
    int yOffset = HEADER_HEIGHT + 10 + channel * PIN_SPACING;
    return Point<int>(0, yOffset);
}

Point<int> RackNodeComponent::getOutputPinPosition(int channel) const
{
    int yOffset = HEADER_HEIGHT + 10 + channel * PIN_SPACING;
    return Point<int>(getWidth(), yOffset);
}

int RackNodeComponent::getNumInputPins() const
{
    if (!nodePtr || !nodePtr->getProcessor())
        return 0;
    auto* proc = nodePtr->getProcessor();
    return proc->getTotalNumInputChannels() + (proc->acceptsMidi() ? 1 : 0);
}

int RackNodeComponent::getNumOutputPins() const
{
    if (!nodePtr || !nodePtr->getProcessor())
        return 0;
    auto* proc = nodePtr->getProcessor();
    return proc->getTotalNumOutputChannels() + (proc->producesMidi() ? 1 : 0);
}

//==============================================================================
// SubGraphCanvas
//==============================================================================

SubGraphCanvas::SubGraphCanvas(SubGraphProcessor& processor) : subGraph(processor)
{
    spdlog::debug("[SubGraphCanvas] Constructor called");
    setSize(800, 600);

    // Register as listener to graph changes (Element-style / Standard JUCE)
    subGraph.getInternalGraph().addChangeListener(this);

    rebuildNodes();
    spdlog::debug("[SubGraphCanvas] Constructor finished");
}

SubGraphCanvas::~SubGraphCanvas()
{
    // CRITICAL: Remove ourselves from the graph's listener list to prevent use-after-free
    subGraph.getInternalGraph().removeChangeListener(this);

    for (auto* node : nodeComponents)
        node->removeChangeListener(this);
}

void SubGraphCanvas::paint(Graphics& g)
{
    // Background with grid
    if (viewMode == ViewMode::Front)
    {
        g.fillAll(Colour(0xFF282830));

        // Subtle grid
        g.setColour(Colour(0xFF333340));
        for (int x = 0; x < getWidth(); x += 20)
            g.drawVerticalLine(x, 0.0f, (float)getHeight());
        for (int y = 0; y < getHeight(); y += 20)
            g.drawHorizontalLine(y, 0.0f, (float)getWidth());
    }
    else
    {
        // Back view - Reason-style darker background
        g.fillAll(Colour(0xFF1A1A20));

        // More prominent grid for cable routing
        g.setColour(Colour(0xFF2A2A30));
        for (int x = 0; x < getWidth(); x += 40)
            g.drawVerticalLine(x, 0.0f, (float)getHeight());
        for (int y = 0; y < getHeight(); y += 40)
            g.drawHorizontalLine(y, 0.0f, (float)getWidth());
    }

    // Draw connections
    if (viewMode == ViewMode::Back)
        drawBackPanelConnections(g);
    else
        drawConnections(g);
}

void SubGraphCanvas::resized()
{
    // Nothing to do - nodes position themselves
}

void SubGraphCanvas::mouseDown(const MouseEvent& e)
{
    if (e.mods.isRightButtonDown())
    {
        // TODO: Show add plugin menu
        PopupMenu menu;
        menu.addItem(1, "Add Plugin...");
        menu.addSeparator();
        menu.addItem(2, "Front View", true, viewMode == ViewMode::Front);
        menu.addItem(3, "Back View (Cables)", true, viewMode == ViewMode::Back);

        int result = menu.show();
        if (result == 2)
            setViewMode(ViewMode::Front);
        else if (result == 3)
            setViewMode(ViewMode::Back);
    }
}

void SubGraphCanvas::changeListenerCallback(ChangeBroadcaster* source)
{
    repaint(); // Redraw connections when nodes move
}

void SubGraphCanvas::setViewMode(ViewMode mode)
{
    viewMode = mode;
    repaint();
}

// Replace rebuildNodes with corrected version that logs and uses correct constructor
void SubGraphCanvas::rebuildNodes()
{
    spdlog::debug("[SubGraphCanvas] rebuildNodes started");

    // Remove existing listeners
    for (auto* node : nodeComponents)
        node->removeChangeListener(this);

    // Clear the OwnedArray first (deletes components)
    // This automatically removes them from parent as well via Component destructor
    nodeComponents.clear();

    // Ensure UI is clear (should be empty already, but safe to call)
    removeAllChildren();

    auto& graph = subGraph.getInternalGraph();
    // CRITICAL: Protect against concurrent access during audio processing
    const juce::ScopedLock sl(graph.getCallbackLock());

    int numNodes = graph.getNumNodes();
    spdlog::debug("[SubGraphCanvas] Internal graph has {} nodes", numNodes);

    for (int i = 0; i < numNodes; ++i)
    {
        auto node = graph.getNode(i);
        if (!node)
        {
            spdlog::warn("[SubGraphCanvas] Null node at index {}", i);
            continue;
        }

        spdlog::debug("[SubGraphCanvas] Processing node uid={} type={}", node->nodeID.uid,
                      node->getProcessor() ? node->getProcessor()->getName().toStdString() : "NULL");

        if (node->getProcessor() == nullptr)
        {
            spdlog::error("[SubGraphCanvas] Node uid={} has null processor!", node->nodeID.uid);
            continue;
        }

        // Check if this is an I/O node
        bool isIO =
            (node->nodeID == subGraph.getRackAudioInputNodeId() ||
             node->nodeID == subGraph.getRackAudioOutputNodeId() || node->nodeID == subGraph.getRackMidiInputNodeId());

        auto* comp = new RackNodeComponent(node, isIO);

        // Position from node properties
        double x = node->properties.getWithDefault("x", 100.0);
        double y = node->properties.getWithDefault("y", 100.0);

        spdlog::debug("[SubGraphCanvas] Placing node at {}, {}", x, y);
        comp->setTopLeftPosition((int)x, (int)y);

        comp->addChangeListener(this);
        addAndMakeVisible(comp);
        nodeComponents.add(comp);
    }
    spdlog::debug("[SubGraphCanvas] rebuildNodes finished, created {} nodes", nodeComponents.size());
}

void SubGraphCanvas::drawConnections(Graphics& g)
{
    auto& graph = subGraph.getInternalGraph();

    g.setColour(Colour(0xFF5588BB));

    for (const auto& conn : graph.getConnections())
    {
        // Find source and dest components
        RackNodeComponent* srcComp = nullptr;
        RackNodeComponent* dstComp = nullptr;

        for (auto* node : nodeComponents)
        {
            if (node->getNodeId() == conn.source.nodeID)
                srcComp = node;
            if (node->getNodeId() == conn.destination.nodeID)
                dstComp = node;
        }

        if (srcComp && dstComp)
        {
            auto srcPos = srcComp->getOutputPinPosition(conn.source.channelIndex);
            auto dstPos = dstComp->getInputPinPosition(conn.destination.channelIndex);

            // Convert to canvas coordinates
            srcPos = srcComp->getPosition() + srcPos;
            dstPos = dstComp->getPosition() + dstPos;

            // MIDI connections in a different color
            if (conn.source.channelIndex == juce::AudioProcessorGraph::midiChannelIndex)
                g.setColour(Colour(0xFFAA6688));
            else
                g.setColour(Colour(0xFF5588BB));

            // Draw bezier curve
            Path cable;
            cable.startNewSubPath(srcPos.toFloat());
            float ctrlOffset = std::abs(dstPos.x - srcPos.x) * 0.5f;
            cable.cubicTo(srcPos.x + ctrlOffset, srcPos.y, dstPos.x - ctrlOffset, dstPos.y, dstPos.x, dstPos.y);

            g.strokePath(cable, PathStrokeType(2.0f));
        }
    }
}

void SubGraphCanvas::drawBackPanelConnections(Graphics& g)
{
    // Back view: thicker, more colorful cables like Reason
    auto& graph = subGraph.getInternalGraph();

    for (const auto& conn : graph.getConnections())
    {
        RackNodeComponent* srcComp = nullptr;
        RackNodeComponent* dstComp = nullptr;

        for (auto* node : nodeComponents)
        {
            if (node->getNodeId() == conn.source.nodeID)
                srcComp = node;
            if (node->getNodeId() == conn.destination.nodeID)
                dstComp = node;
        }

        if (srcComp && dstComp)
        {
            auto srcPos = srcComp->getOutputPinPosition(conn.source.channelIndex);
            auto dstPos = dstComp->getInputPinPosition(conn.destination.channelIndex);

            srcPos = srcComp->getPosition() + srcPos;
            dstPos = dstComp->getPosition() + dstPos;

            // Color coding by channel
            Colour cableColour;
            if (conn.source.channelIndex == juce::AudioProcessorGraph::midiChannelIndex)
                cableColour = Colour(0xFFDD6699); // Pink for MIDI
            else if (conn.source.channelIndex == 0)
                cableColour = Colour(0xFFDD8844); // Orange for left
            else if (conn.source.channelIndex == 1)
                cableColour = Colour(0xFF44AADD); // Blue for right
            else
                cableColour = Colour(0xFF88BB44); // Green for others

            // Draw shadow
            g.setColour(Colours::black.withAlpha(0.3f));
            Path shadow;
            shadow.startNewSubPath(srcPos.x + 2, srcPos.y + 3);
            float ctrlOffset = std::abs(dstPos.x - srcPos.x) * 0.4f;
            shadow.cubicTo(srcPos.x + ctrlOffset + 2, srcPos.y + 3, dstPos.x - ctrlOffset + 2, dstPos.y + 3,
                           dstPos.x + 2, dstPos.y + 3);
            g.strokePath(shadow, PathStrokeType(5.0f, PathStrokeType::curved, PathStrokeType::rounded));

            // Draw cable
            g.setColour(cableColour);
            Path cable;
            cable.startNewSubPath(srcPos.toFloat());
            cable.cubicTo(srcPos.x + ctrlOffset, srcPos.y, dstPos.x - ctrlOffset, dstPos.y, dstPos.x, dstPos.y);
            g.strokePath(cable, PathStrokeType(4.0f, PathStrokeType::curved, PathStrokeType::rounded));

            // Highlight
            g.setColour(cableColour.brighter(0.3f));
            g.strokePath(cable, PathStrokeType(1.5f, PathStrokeType::curved, PathStrokeType::rounded));
        }
    }
}

//==============================================================================
// SubGraphEditorComponent
//==============================================================================

SubGraphEditorComponent::SubGraphEditorComponent(SubGraphProcessor& processor)
    : AudioProcessorEditor(processor), subGraphProcessor(processor)
{
    spdlog::debug("[SubGraphEditor] Creating editor for rack: {}", processor.getRackName().toStdString());

    // Title label
    titleLabel = std::make_unique<Label>("title", processor.getRackName());
    titleLabel->setFont(Font(FontOptions(18.0f).withStyle("Bold")));
    titleLabel->setColour(Label::textColourId, Colours::white);
    titleLabel->setEditable(true);
    titleLabel->onTextChange = [this]() { subGraphProcessor.setRackName(titleLabel->getText()); };
    addAndMakeVisible(*titleLabel);

    // Front/Back toggle button
    frontBackButton = std::make_unique<TextButton>("Front");
    frontBackButton->addListener(this);
    addAndMakeVisible(*frontBackButton);

    // Add plugin button
    addPluginButton = std::make_unique<TextButton>("+ Add");
    addPluginButton->addListener(this);
    addAndMakeVisible(*addPluginButton);

    // Canvas in viewport
    canvas = std::make_unique<SubGraphCanvas>(processor);
    viewport = std::make_unique<Viewport>();
    viewport->setViewedComponent(canvas.get(), false);
    viewport->setScrollBarsShown(true, true);
    addAndMakeVisible(*viewport);

    // Set size LAST - this triggers resized() which needs all components to exist
    setSize(800, 600);
}

SubGraphEditorComponent::~SubGraphEditorComponent()
{
    spdlog::debug("[SubGraphEditor] Destroying editor");
    if (viewport)
        viewport->setViewedComponent(nullptr, false);
}

void SubGraphEditorComponent::paint(Graphics& g)
{
    // Toolbar background
    g.setColour(Colour(0xFF3A3A45));
    g.fillRect(0, 0, getWidth(), TOOLBAR_HEIGHT);

    // Toolbar bottom border
    g.setColour(Colour(0xFF5A5A65));
    g.drawHorizontalLine(TOOLBAR_HEIGHT - 1, 0.0f, (float)getWidth());
}

void SubGraphEditorComponent::resized()
{
    auto bounds = getLocalBounds();
    auto toolbar = bounds.removeFromTop(TOOLBAR_HEIGHT);

    // Toolbar layout
    toolbar = toolbar.reduced(8, 4);
    titleLabel->setBounds(toolbar.removeFromLeft(200));
    toolbar.removeFromLeft(10);
    frontBackButton->setBounds(toolbar.removeFromLeft(80));
    toolbar.removeFromLeft(10);
    addPluginButton->setBounds(toolbar.removeFromLeft(80));

    // Canvas viewport
    viewport->setBounds(bounds);
}

void SubGraphEditorComponent::buttonClicked(Button* button)
{
    if (button == frontBackButton.get())
    {
        auto currentMode = canvas->getViewMode();
        if (currentMode == SubGraphCanvas::ViewMode::Front)
        {
            canvas->setViewMode(SubGraphCanvas::ViewMode::Back);
            frontBackButton->setButtonText("Back");
        }
        else
        {
            canvas->setViewMode(SubGraphCanvas::ViewMode::Front);
            frontBackButton->setButtonText("Front");
        }
    }
    else if (button == addPluginButton.get())
    {
        // TODO: Show plugin selection menu
        spdlog::debug("[SubGraphEditor] Add plugin button clicked");
    }
}
