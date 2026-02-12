/*
  ==============================================================================

    SubGraphEditorComponent.cpp

    Implementation of the SubGraph editor, mirroring PluginField's architecture
    but with a different color hue for visual differentiation.

  ==============================================================================
*/

#include "SubGraphEditorComponent.h"

#include "AudioSingletons.h"
#include "ColourScheme.h"
#include "FontManager.h"
#include "InternalFilters.h"
#include "PluginComponent.h"
#include "SettingsManager.h"
#include "SubGraphProcessor.h"

#include <map>
#include <spdlog/spdlog.h>

using namespace juce;

//==============================================================================
// SubGraphCanvas - mirrors PluginField but for subgraphs
//==============================================================================

SubGraphCanvas::SubGraphCanvas(SubGraphProcessor& processor, KnownPluginList* list)
    : subGraph(processor), pluginList(list)
{
    spdlog::debug("[SubGraphCanvas] Constructor starting");
    setSize(2000, 1500);
    setWantsKeyboardFocus(true);

    // Build initial components from any existing nodes
    spdlog::debug("[SubGraphCanvas] About to call rebuildGraph()");
    rebuildGraph();
    spdlog::debug("[SubGraphCanvas] Constructor complete");
}

SubGraphCanvas::~SubGraphCanvas()
{
    // OwnedArray members (filterComponents, connectionComponents) will
    // automatically delete their contents when this object is destroyed.
    // Do NOT call deleteAllChildren() as it would double-delete.
}

void SubGraphCanvas::paint(Graphics& g)
{
    auto& colours = ColourScheme::getInstance().colours;
    auto bounds = getLocalBounds().toFloat();

    // === SubGraph uses cyan/teal hue instead of purple ===
    Colour bgBase = colours["Field Background"];
    // Shift hue towards cyan (approximately 180 degrees from purple)
    Colour bgCol = bgBase.withHue(0.5f); // Cyan hue

    ColourGradient bgGrad(bgCol.brighter(0.08f), 0.0f, 0.0f, bgCol.darker(0.15f), 0.0f, bounds.getHeight(), false);
    g.setGradientFill(bgGrad);
    g.fillRect(bounds);

    // === Grid pattern ===
    float gridSize = 30.0f;
    Colour gridCol = Colour(0xFF00AAAA).withAlpha(0.15f); // Cyan grid
    g.setColour(gridCol);

    for (float x = 0.0f; x < bounds.getWidth(); x += gridSize)
        g.drawVerticalLine((int)x, 0.0f, bounds.getHeight());
    for (float y = 0.0f; y < bounds.getHeight(); y += gridSize)
        g.drawHorizontalLine((int)y, 0.0f, bounds.getWidth());

    if (displayDoubleClickMessage)
    {
        // Draw hint at center of visible viewport area (not canvas center)
        float centerX, centerY;
        if (auto* viewport = findParentComponentOfClass<Viewport>())
        {
            auto viewArea = viewport->getViewArea();
            centerX = viewArea.getCentreX();
            centerY = viewArea.getCentreY();
        }
        else
        {
            centerX = bounds.getCentreX();
            centerY = bounds.getCentreY();
        }

        g.setFont(FontManager::getInstance().getUIFont(18.0f));
        g.setColour(Colour(0xFF00CCCC).withAlpha(0.6f)); // Cyan text

        String hintText = "Double-click to add a plugin";
        auto textWidth = g.getCurrentFont().getStringWidth(hintText);
        g.drawText(hintText, (int)(centerX - textWidth / 2), (int)(centerY - 10), textWidth + 20, 30,
                   Justification::centred, false);

        g.setFont(FontManager::getInstance().getUIFont(13.0f));
        g.setColour(Colour(0xFF00AAAA).withAlpha(0.35f));

        String subHint = "This is an Effect Rack sub-graph";
        auto subWidth = g.getCurrentFont().getStringWidth(subHint);
        g.drawText(subHint, (int)(centerX - subWidth / 2), (int)(centerY + 18), subWidth + 20, 24,
                   Justification::centred, false);
    }
}

void SubGraphCanvas::resized()
{
    // Components position themselves
}

void SubGraphCanvas::mouseDown(const MouseEvent& e)
{
    if (e.getNumberOfClicks() == 2)
    {
        // Double-click: show plugin menu (mirrors PluginField exactly)
        PopupMenu menu;

        const int SEARCH_ITEM_ID = 100000;
        const int MANAGE_FAVORITES_BASE = 200000;

        auto& settings = SettingsManager::getInstance();
        StringArray favorites = settings.getStringArray("PluginFavorites");
        StringArray recentPlugins = settings.getStringArray("RecentPlugins");

        // Get all plugin types
        auto types = pluginList->getTypes();

        // Add Effect Rack for nested racks
        InternalPluginFormat internalFormat;
        types.add(*internalFormat.getDescriptionFor(InternalPluginFormat::subGraphProcFilter));

        // Build lookup map
        std::map<String, int> identifierToIndex;
        for (int i = 0; i < types.size(); ++i)
            identifierToIndex[types.getReference(i).createIdentifierString()] = i;

        // Favorites section
        PopupMenu favoritesMenu;
        for (const auto& favId : favorites)
        {
            auto it = identifierToIndex.find(favId);
            if (it != identifierToIndex.end())
            {
                int idx = it->second;
                favoritesMenu.addItem(idx + 1, types.getReference(idx).name);
            }
        }
        if (favoritesMenu.getNumItems() > 0)
            menu.addSubMenu(CharPointer_UTF8("\xe2\x98\x85 Favorites"), favoritesMenu);

        // Recent section
        PopupMenu recentMenu;
        for (const auto& recentId : recentPlugins)
        {
            auto it = identifierToIndex.find(recentId);
            if (it != identifierToIndex.end())
            {
                int idx = it->second;
                recentMenu.addItem(idx + 1, types.getReference(idx).name);
            }
        }
        if (recentMenu.getNumItems() > 0)
            menu.addSubMenu("Recent", recentMenu);

        // Search option
        menu.addItem(SEARCH_ITEM_ID, CharPointer_UTF8("\xf0\x9f\x94\x8d Search..."));

        // Edit Favorites
        PopupMenu editFavoritesMenu;
        for (int i = 0; i < types.size(); ++i)
        {
            const auto& type = types.getReference(i);
            bool isFavorite = favorites.contains(type.createIdentifierString());
            editFavoritesMenu.addItem(MANAGE_FAVORITES_BASE + i + 1, type.name, true, isFavorite);
        }
        menu.addSubMenu(CharPointer_UTF8("\xe2\x98\x85 Edit Favorites..."), editFavoritesMenu);

        if (favoritesMenu.getNumItems() > 0 || recentMenu.getNumItems() > 0)
            menu.addSeparator();

        // Category menus
        PopupMenu builtInMenu;
        PopupMenu allPluginsMenu;
        std::map<String, PopupMenu> categoryMenus;

        for (int i = 0; i < types.size(); ++i)
        {
            const auto& type = types.getReference(i);

            if (type.pluginFormatName == "Internal" || type.category == "Built-in")
                builtInMenu.addItem(i + 1, type.name);
            else
            {
                String category = type.category.isNotEmpty() ? type.category : "Uncategorized";
                categoryMenus[category].addItem(i + 1, type.name);
                allPluginsMenu.addItem(i + 1, type.name);
            }
        }

        if (builtInMenu.getNumItems() > 0)
        {
            menu.addSubMenu("Pedalboard", builtInMenu);
            menu.addSeparator();
        }

        for (auto& [category, categoryMenu] : categoryMenus)
            menu.addSubMenu(category, categoryMenu);

        menu.addSeparator();
        menu.addSubMenu("All Plugins", allPluginsMenu);

        int result = menu.show();

        // Handle search
        if (result == SEARCH_ITEM_ID)
        {
            AlertWindow searchDialog("Search Plugins", "Type to filter:", AlertWindow::NoIcon);
            searchDialog.addTextEditor("search", "", "Plugin name:");
            searchDialog.addButton("Cancel", 0);
            searchDialog.addButton("OK", 1);

            if (searchDialog.runModalLoop() == 1)
            {
                String searchText = searchDialog.getTextEditor("search")->getText().toLowerCase();
                if (searchText.isNotEmpty())
                {
                    PopupMenu searchResults;
                    for (int i = 0; i < types.size(); ++i)
                    {
                        const auto& type = types.getReference(i);
                        if (type.name.toLowerCase().contains(searchText))
                            searchResults.addItem(i + 1, type.name);
                    }

                    if (searchResults.getNumItems() > 0)
                        result = searchResults.show();
                    else
                    {
                        AlertWindow::showMessageBox(AlertWindow::InfoIcon, "No Results",
                                                    "No plugins found matching \"" + searchText + "\"");
                        result = 0;
                    }
                }
            }
            else
                result = 0;
        }

        // Handle favorite toggle
        if (result >= MANAGE_FAVORITES_BASE)
        {
            int typeIndex = result - MANAGE_FAVORITES_BASE - 1;
            if (typeIndex >= 0 && typeIndex < types.size())
            {
                String pluginId = types.getReference(typeIndex).createIdentifierString();
                if (favorites.contains(pluginId))
                    favorites.removeString(pluginId);
                else
                    favorites.add(pluginId);
                settings.setStringArray("PluginFavorites", favorites);
            }
            return;
        }

        // Handle plugin selection
        if (result > 0 && result < SEARCH_ITEM_ID)
        {
            int typeIndex = result - 1;
            if (typeIndex >= 0 && typeIndex < types.size())
            {
                PluginDescription pluginType = types.getReference(typeIndex);
                spdlog::info("[SubGraphCanvas] Loading plugin: {}", pluginType.name.toStdString());

                // Add to the subgraph's internal graph
                String errorMessage;
                auto instance = AudioPluginFormatManagerSingleton::getInstance().createPluginInstance(
                    pluginType, 44100.0, 512, errorMessage);

                if (instance)
                {
                    auto& graph = subGraph.getInternalGraph();
                    auto node = graph.addNode(std::move(instance));
                    if (node)
                    {
                        node->properties.set("x", (double)e.x);
                        node->properties.set("y", (double)e.y);
                        addFilter(graph.getNumNodes() - 1);
                        sendChangeMessage();
                        clearDoubleClickMessage();

                        // Update recent plugins
                        String pluginId = pluginType.createIdentifierString();
                        recentPlugins.removeString(pluginId);
                        recentPlugins.insert(0, pluginId);
                        while (recentPlugins.size() > 8)
                            recentPlugins.remove(recentPlugins.size() - 1);
                        settings.setStringArray("RecentPlugins", recentPlugins);
                    }
                }
                else
                {
                    spdlog::error("[SubGraphCanvas] Failed to load plugin: {}", errorMessage.toStdString());
                }
            }
        }
    }
    else
    {
        // Single click: begin panning
        if (auto* viewport = findParentComponentOfClass<Viewport>())
        {
            isPanning = true;
            panStartMouse = e.getScreenPosition();
            panStartScroll = viewport->getViewPosition();
            setMouseCursor(MouseCursor::DraggingHandCursor);
        }
    }
}

void SubGraphCanvas::mouseDrag(const MouseEvent& e)
{
    if (isPanning)
    {
        if (auto* viewport = findParentComponentOfClass<Viewport>())
        {
            Point<int> delta = panStartMouse - e.getScreenPosition();
            Point<int> targetPosition = panStartScroll + delta;

            // Expand canvas if needed
            int currentWidth = getWidth();
            int currentHeight = getHeight();
            int viewWidth = viewport->getViewWidth();
            int viewHeight = viewport->getViewHeight();

            int neededWidth = targetPosition.x + viewWidth;
            int neededHeight = targetPosition.y + viewHeight;

            bool sizeChanged = false;
            if (neededWidth > currentWidth)
            {
                currentWidth = neededWidth + 200;
                sizeChanged = true;
            }
            if (neededHeight > currentHeight)
            {
                currentHeight = neededHeight + 200;
                sizeChanged = true;
            }

            if (sizeChanged)
                setSize(currentWidth, currentHeight);

            // Handle negative scroll (expand canvas leftward/upward)
            if (targetPosition.x < 0)
            {
                int expandBy = -targetPosition.x + 100;
                setSize(getWidth() + expandBy, getHeight());
                for (int i = 0; i < getNumChildComponents(); ++i)
                    getChildComponent(i)->setTopLeftPosition(getChildComponent(i)->getX() + expandBy,
                                                             getChildComponent(i)->getY());
                panStartScroll.setX(panStartScroll.x + expandBy);
                targetPosition.setX(100);
            }
            if (targetPosition.y < 0)
            {
                int expandBy = -targetPosition.y + 100;
                setSize(getWidth(), getHeight() + expandBy);
                for (int i = 0; i < getNumChildComponents(); ++i)
                    getChildComponent(i)->setTopLeftPosition(getChildComponent(i)->getX(),
                                                             getChildComponent(i)->getY() + expandBy);
                panStartScroll.setY(panStartScroll.y + expandBy);
                targetPosition.setY(100);
            }

            viewport->setViewPosition(targetPosition);
        }
    }
}

void SubGraphCanvas::mouseUp(const MouseEvent& e)
{
    if (isPanning)
    {
        isPanning = false;
        setMouseCursor(MouseCursor::NormalCursor);
    }
}

void SubGraphCanvas::mouseWheelMove(const MouseEvent& e, const MouseWheelDetails& wheel)
{
    float zoomDelta = wheel.deltaY * 0.1f;
    float newZoom = jlimit(minZoom, maxZoom, zoomLevel + zoomDelta);

    if (newZoom != zoomLevel)
    {
        auto mousePos = e.getPosition();
        float scaleRatio = newZoom / zoomLevel;

        zoomLevel = newZoom;
        setTransform(AffineTransform::scale(zoomLevel));

        if (auto* viewport = findParentComponentOfClass<Viewport>())
        {
            auto currentPos = viewport->getViewPosition();
            int newX = static_cast<int>((currentPos.x + mousePos.x) * scaleRatio - mousePos.x);
            int newY = static_cast<int>((currentPos.y + mousePos.y) * scaleRatio - mousePos.y);
            viewport->setViewPosition(jmax(0, newX), jmax(0, newY));
        }

        repaint();
    }
}

void SubGraphCanvas::changeListenerCallback(ChangeBroadcaster* source)
{
    if (auto* pluginComp = dynamic_cast<PluginComponent*>(source))
    {
        Point<int> fieldSize(getWidth(), getHeight());
        Point<int> pluginPos = pluginComp->getPosition();
        Point<int> pluginSize(pluginComp->getWidth(), pluginComp->getHeight());

        if ((pluginPos.getX() + pluginSize.getX()) > fieldSize.getX())
            fieldSize.setX((pluginPos.getX() + pluginSize.getX()));
        if ((pluginPos.getY() + pluginSize.getY()) > fieldSize.getY())
            fieldSize.setY((pluginPos.getY() + pluginSize.getY()));

        setSize(fieldSize.getX(), fieldSize.getY());
        repaint();
    }
}

void SubGraphCanvas::addFilter(int filterIndex)
{
    auto& graph = subGraph.getInternalGraph();
    spdlog::debug("[SubGraphCanvas::addFilter] filterIndex={}, numNodes={}", filterIndex, graph.getNumNodes());
    if (filterIndex >= 0 && filterIndex < graph.getNumNodes())
    {
        auto node = graph.getNode(filterIndex);
        if (node)
        {
            spdlog::debug("[SubGraphCanvas::addFilter] Creating PluginComponent for node: {}",
                          node->getProcessor()->getName().toStdString());
            auto* comp = new PluginComponent(node.get());
            spdlog::debug("[SubGraphCanvas::addFilter] PluginComponent created, adding change listener");

            // Position from node properties (mirroring PluginField)
            int x = node->properties.getWithDefault("x", 50);
            int y = node->properties.getWithDefault("y", 50 + filterIndex * 110);
            comp->setTopLeftPosition(x, y);

            comp->addChangeListener(this);
            addAndMakeVisible(comp);
            filterComponents.add(comp);
            spdlog::debug("[SubGraphCanvas::addFilter] Complete for index {} at ({}, {})", filterIndex, x, y);
        }
    }
}

void SubGraphCanvas::deleteFilter(AudioProcessorGraph::Node* node)
{
    // Find and remove the component
    for (int i = filterComponents.size() - 1; i >= 0; --i)
    {
        if (filterComponents[i]->getNode() == node)
        {
            filterComponents[i]->removeChangeListener(this);
            removeChildComponent(filterComponents[i]);
            filterComponents.remove(i);
            break;
        }
    }

    // Remove from the graph
    subGraph.getInternalGraph().removeNode(node->nodeID);
    sendChangeMessage();
}

void SubGraphCanvas::addConnection(AudioProcessorGraph::NodeID srcId, int srcChannel,
                                   AudioProcessorGraph::NodeID destId, int destChannel)
{
    subGraph.getInternalGraph().addConnection({{srcId, srcChannel}, {destId, destChannel}});
    rebuildGraph(); // Refresh all connections
    sendChangeMessage();
}

void SubGraphCanvas::deleteConnection(PluginConnection* connection)
{
    if (!connection)
        return;

    const PluginPinComponent* source = connection->getSource();
    const PluginPinComponent* dest = connection->getDestination();

    if (source && dest)
    {
        spdlog::debug("[SubGraphCanvas::deleteConnection] Removing connection from {} to {}", source->getUid(),
                      dest->getUid());

        // Remove from the internal graph
        subGraph.getInternalGraph().removeConnection(
            {{AudioProcessorGraph::NodeID(source->getUid()), source->getChannel()},
             {AudioProcessorGraph::NodeID(dest->getUid()), dest->getChannel()}});
    }

    // Remove and delete the visual component
    removeChildComponent(connection);

    // Find and remove from connectionComponents array
    for (int i = connectionComponents.size() - 1; i >= 0; --i)
    {
        if (connectionComponents[i] == connection)
        {
            connectionComponents.remove(i);
            break;
        }
    }

    sendChangeMessage();
}

bool SubGraphCanvas::keyPressed(const KeyPress& key)
{
    // Delete selected connections on Delete or Backspace
    if (key == KeyPress::deleteKey || key == KeyPress::backspaceKey)
    {
        spdlog::debug("[SubGraphCanvas::keyPressed] Delete key pressed");

        // Iterate in reverse since we're deleting
        for (int i = connectionComponents.size() - 1; i >= 0; --i)
        {
            if (connectionComponents[i]->getSelected())
            {
                spdlog::debug("[SubGraphCanvas::keyPressed] Deleting selected connection {}", i);
                deleteConnection(connectionComponents[i]);
            }
        }
        return true;
    }
    return false;
}

void SubGraphCanvas::rebuildGraph()
{
    spdlog::debug("[SubGraphCanvas::rebuildGraph] Starting");

    // Clear existing connections
    for (int i = getNumChildComponents() - 1; i >= 0; --i)
    {
        if (auto* conn = dynamic_cast<PluginConnection*>(getChildComponent(i)))
        {
            removeChildComponent(conn);
            delete conn;
        }
    }
    connectionComponents.clear();

    // Clear and rebuild filter components
    for (auto* comp : filterComponents)
    {
        comp->removeChangeListener(this);
        removeChildComponent(comp);
    }
    filterComponents.clear();

    // Rebuild from current graph state
    auto& graph = subGraph.getInternalGraph();
    spdlog::debug("[SubGraphCanvas::rebuildGraph] Graph has {} nodes", graph.getNumNodes());
    for (int i = 0; i < graph.getNumNodes(); ++i)
    {
        spdlog::debug("[SubGraphCanvas::rebuildGraph] Adding filter {}", i);
        addFilter(i);
    }

    // Rebuild connections from graph state
    // Copy connections to avoid iterator invalidation issues with JUCE's ReferenceCountedArray
    std::vector<AudioProcessorGraph::Connection> connectionsCopy;
    for (const auto& conn : graph.getConnections())
    {
        connectionsCopy.push_back(conn);
    }

    spdlog::debug("[SubGraphCanvas::rebuildGraph] Rebuilding {} connections", (int)connectionsCopy.size());

    for (const auto& conn : connectionsCopy)
    {
        // Find source and destination pins
        PluginPinComponent* sourcePin = nullptr;
        PluginPinComponent* destPin = nullptr;

        for (auto* comp : filterComponents)
        {
            // Check if this component is the source node
            if (comp->getNode() && comp->getNode()->nodeID == conn.source.nodeID)
            {
                // Bounds check before accessing pin
                if (conn.source.channelIndex >= 0 && conn.source.channelIndex < comp->getNumOutputPins())
                {
                    sourcePin = comp->getOutputPin(conn.source.channelIndex);
                }
                else
                {
                    spdlog::warn("[SubGraphCanvas::rebuildGraph] Source channel {} out of range (0-{})",
                                 conn.source.channelIndex, comp->getNumOutputPins() - 1);
                }
            }
            // Check if this component is the destination node
            if (comp->getNode() && comp->getNode()->nodeID == conn.destination.nodeID)
            {
                // Bounds check before accessing pin
                if (conn.destination.channelIndex >= 0 && conn.destination.channelIndex < comp->getNumInputPins())
                {
                    destPin = comp->getInputPin(conn.destination.channelIndex);
                }
                else
                {
                    spdlog::warn("[SubGraphCanvas::rebuildGraph] Dest channel {} out of range (0-{})",
                                 conn.destination.channelIndex, comp->getNumInputPins() - 1);
                }
            }
        }

        if (sourcePin && destPin)
        {
            auto* connection = new PluginConnection(sourcePin, destPin, false);
            addAndMakeVisible(connection);
            connectionComponents.add(connection);
            spdlog::debug("[SubGraphCanvas::rebuildGraph] Restored connection {}:{} -> {}:{}", conn.source.nodeID.uid,
                          conn.source.channelIndex, conn.destination.nodeID.uid, conn.destination.channelIndex);
        }
        else
        {
            spdlog::warn("[SubGraphCanvas::rebuildGraph] Could not find pins for connection {}:{} -> {}:{}",
                         conn.source.nodeID.uid, conn.source.channelIndex, conn.destination.nodeID.uid,
                         conn.destination.channelIndex);
        }
    }

    spdlog::debug("[SubGraphCanvas::rebuildGraph] Complete");
    repaint();
}

Component* SubGraphCanvas::getPinAt(int x, int y)
{
    for (auto* comp : filterComponents)
    {
        // Check input pins
        for (int i = 0; i < comp->getNumInputPins(); ++i)
        {
            auto* pin = comp->getInputPin(i);
            if (pin && pin->getBounds().contains(x - comp->getX(), y - comp->getY()))
                return pin;
        }
        // Check output pins
        for (int i = 0; i < comp->getNumOutputPins(); ++i)
        {
            auto* pin = comp->getOutputPin(i);
            if (pin && pin->getBounds().contains(x - comp->getX(), y - comp->getY()))
                return pin;
        }
    }
    return nullptr;
}

//------------------------------------------------------------------------------
void SubGraphCanvas::addConnection(PluginPinComponent* source, bool connectAll)
{
    if (source)
    {
        PluginConnection* connection = new PluginConnection(source, 0, connectAll);

        connection->setSize(10, 12);
        addAndMakeVisible(connection);
        connection->toFront(false);                         // Bring dragging connection to front
        connection->setInterceptsMouseClicks(false, false); // Don't intercept mouse while dragging
        draggingConnection = connection;
    }
}

//------------------------------------------------------------------------------
void SubGraphCanvas::dragConnection(int x, int y)
{
    if (draggingConnection)
    {
        Component* c = getPinAt(x + 5, y);
        PluginPinComponent* p = dynamic_cast<PluginPinComponent*>(c);

        if (p)
        {
            const PluginPinComponent* s = draggingConnection->getSource();

            // Snap to pin if: same type (audio/param) AND opposite direction
            if (p->getParameterPin() == draggingConnection->getParameterConnection() &&
                p->getDirection() != s->getDirection())
            {
                Point<int> tempPoint(p->getX() + 7, p->getY() + 8);

                tempPoint = getLocalPoint(p->getParentComponent(), tempPoint);
                draggingConnection->drag(tempPoint.getX(), tempPoint.getY());
            }
            else
                draggingConnection->drag(x, y);
        }
        else
            draggingConnection->drag(x, y);
    }
}

//------------------------------------------------------------------------------
void SubGraphCanvas::releaseConnection(int x, int y)
{
    if (draggingConnection)
    {
        Component* c = getPinAt(x, y);
        PluginPinComponent* p = dynamic_cast<PluginPinComponent*>(c);

        repaint();

        if (p)
        {
            const PluginPinComponent* s = draggingConnection->getSource();

            // Accept connection if source and destination have opposite directions
            if (p->getDirection() != s->getDirection())
            {
                // Check that both pins are same type (audio or parameter)
                if ((s->getParameterPin() && p->getParameterPin()) || (!s->getParameterPin() && !p->getParameterPin()))
                {
                    // Determine which pin is output and which is input
                    const PluginPinComponent* outputPin = s->getDirection() ? s : p;
                    const PluginPinComponent* inputPin = s->getDirection() ? p : s;

                    // Always connect output -> input
                    auto& graph = subGraph.getInternalGraph();
                    graph.addConnection({{AudioProcessorGraph::NodeID(outputPin->getUid()), outputPin->getChannel()},
                                         {AudioProcessorGraph::NodeID(inputPin->getUid()), inputPin->getChannel()}});

                    draggingConnection->setDestination(p);
                    draggingConnection->setInterceptsMouseClicks(true, true); // Re-enable mouse clicks
                    connectionComponents.add(draggingConnection);

                    spdlog::debug("[SubGraphCanvas] Connection made: {}:{} -> {}:{}", outputPin->getUid(),
                                  outputPin->getChannel(), inputPin->getUid(), inputPin->getChannel());
                }
                else
                {
                    // Type mismatch (audio vs parameter)
                    removeChildComponent(draggingConnection);
                    delete draggingConnection;
                }
            }
            else
            {
                // Same direction (input-to-input or output-to-output) - reject
                removeChildComponent(draggingConnection);
                delete draggingConnection;
            }
        }
        else
        {
            removeChildComponent(draggingConnection);
            delete draggingConnection;
        }
        draggingConnection = nullptr;
    }
}

//==============================================================================
// SubGraphEditorComponent - the editor window
//==============================================================================

SubGraphEditorComponent::SubGraphEditorComponent(SubGraphProcessor& processor)
    : AudioProcessorEditor(processor), subGraphProcessor(processor)
{
    spdlog::debug("[SubGraphEditorComponent] Constructor starting for: {}", processor.getName().toStdString());
    setSize(700, 500);

    // Title label
    titleLabel = std::make_unique<Label>("title", "Effect Rack: " + processor.getName());
    titleLabel->setFont(FontManager::getInstance().getUIFont(16.0f));
    titleLabel->setColour(Label::textColourId, Colour(0xFF00DDDD)); // Cyan title
    addAndMakeVisible(titleLabel.get());

    // Canvas in viewport
    canvas = std::make_unique<SubGraphCanvas>(processor, KnownPluginListSingleton::getInstance());
    viewport = std::make_unique<Viewport>();
    viewport->setViewedComponent(canvas.get(), false);
    viewport->setScrollBarsShown(true, true);
    addAndMakeVisible(viewport.get());

    // Ensure layout is done after all children are created
    resized();

    spdlog::info("[SubGraphEditor] Created editor for: {}", processor.getName().toStdString());
}

SubGraphEditorComponent::~SubGraphEditorComponent() {}

void SubGraphEditorComponent::paint(Graphics& g)
{
    // Toolbar background with cyan accent
    auto toolbar = getLocalBounds().removeFromTop(TOOLBAR_HEIGHT);
    g.setColour(Colour(0xFF1A2A2A)); // Dark teal
    g.fillRect(toolbar);

    // Subtle bottom border
    g.setColour(Colour(0xFF00AAAA).withAlpha(0.3f));
    g.drawLine(0, TOOLBAR_HEIGHT - 1, getWidth(), TOOLBAR_HEIGHT - 1, 1.0f);
}

void SubGraphEditorComponent::resized()
{
    spdlog::debug("[SubGraphEditorComponent::resized] Called, size={}x{}", getWidth(), getHeight());
    auto bounds = getLocalBounds();
    auto toolbar = bounds.removeFromTop(TOOLBAR_HEIGHT);

    toolbar = toolbar.reduced(8, 4);
    if (titleLabel)
        titleLabel->setBounds(toolbar);

    if (viewport)
    {
        viewport->setBounds(bounds);
        spdlog::debug("[SubGraphEditorComponent::resized] Viewport bounds={}x{} at ({},{})", viewport->getWidth(),
                      viewport->getHeight(), viewport->getX(), viewport->getY());
    }

    if (canvas)
    {
        spdlog::debug("[SubGraphEditorComponent::resized] Canvas size={}x{}, numChildren={}", canvas->getWidth(),
                      canvas->getHeight(), canvas->getNumChildComponents());
    }
}
