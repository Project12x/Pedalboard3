//	PluginField.cpp - Field representing the signal path through the app.
//	----------------------------------------------------------------------------
//	This file is part of Pedalboard3, an audio plugin host.
//	Copyright (c) 2009 Niall Moody.
//
//	This program is free software: you can redistribute it and/or modify
//	it under the terms of the GNU General Public License as published by
//	the Free Software Foundation, either version 3 of the License, or
//	(at your option) any later version.
//
//	This program is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//	GNU General Public License for more details.
//
//	You should have received a copy of the GNU General Public License
//	along with this program.  If not, see <http://www.gnu.org/licenses/>.
//	----------------------------------------------------------------------------

#include "PluginField.h"

#include "AudioSingletons.h"
#include "BypassableInstance.h"
#include "ColourScheme.h"
#include "FilterGraph.h"
#include "FontManager.h"
#include "InternalFilters.h"
#include "LogFile.h"
#include "MainTransport.h"
#include "Mapping.h"
#include "NiallsOSCLib/OSCBundle.h"
#include "NiallsOSCLib/OSCMessage.h"
#include "PedalboardProcessors.h"
#include "PluginComponent.h"
#include "SettingsManager.h"

#include <set>
#include <tuple>

using namespace std;

//------------------------------------------------------------------------------
PluginField::PluginField(FilterGraph* filterGraph, KnownPluginList* list, ApplicationCommandManager* appManager)
    : signalPath(filterGraph), pluginList(list), midiManager(appManager), oscManager(appManager), draggingConnection(0),
      tempo(120.0), displayDoubleClickMessage(true)
{
    int i;

    audioInputEnabled = SettingsManager::getInstance().getBool("AudioInput", true);
    midiInputEnabled = SettingsManager::getInstance().getBool("MidiInput", true);
    oscInputEnabled = SettingsManager::getInstance().getBool("OscInput", true);

    autoMappingsWindow = SettingsManager::getInstance().getBool("AutoMappingsWindow", true);

    // Inform the signal path about our AudioPlayHead.
    signalPath->getGraph().setPlayHead(this);

    // Add OSC input.
    if (oscInputEnabled)
    {
        OscInput p;
        PluginDescription desc;

        p.fillInPluginDescription(desc);

        // Use Raw method to avoid adding to undo history
        signalPath->addFilterRaw(&desc, 50, 400);
    }

    // Setup gui.
    for (i = 0; i < signalPath->getNumFilters(); ++i)
        addFilter(i);

    // Add MidiInterceptor.
    if (midiInputEnabled)
    {
        MidiInterceptor p;
        PluginDescription desc;

        p.fillInPluginDescription(desc);

        // Use Raw method to avoid adding to undo history
        signalPath->addFilterRaw(&desc, 50, 350);

        // And connect it up to the midi input.
        {
            AudioProcessorGraph::NodeID midiInput;
            AudioProcessorGraph::NodeID midiInterceptor;

            for (i = 0; i < signalPath->getNumFilters(); ++i)
            {
                if (signalPath->getNode(i)->getProcessor()->getName() == "Midi Input")
                    midiInput = signalPath->getNode(i)->nodeID; // JUCE 8: NodeID struct
                else if (signalPath->getNode(i)->getProcessor()->getName() == "Midi Interceptor")
                {
                    midiInterceptor = signalPath->getNode(i)->nodeID; // JUCE 8: NodeID struct
                    dynamic_cast<MidiInterceptor*>(signalPath->getNode(i)->getProcessor())->setManager(&midiManager);
                }
            }
            // Use Raw method to avoid adding to undo history
            signalPath->addConnectionRaw(midiInput, AudioProcessorGraph::midiChannelIndex, midiInterceptor,
                                         AudioProcessorGraph::midiChannelIndex);
        }
    }

    setWantsKeyboardFocus(true);

    startTimer(50);
}

//------------------------------------------------------------------------------
PluginField::~PluginField()
{
    int i;
    multimap<uint32, Mapping*>::iterator it;

    // If we don't do this, the connections will try to contact their pins, which
    // may have already been deleted.
    for (i = (getNumChildComponents() - 1); i >= 0; --i)
    {
        PluginConnection* connection = dynamic_cast<PluginConnection*>(getChildComponent(i));

        if (connection)
        {
            removeChildComponent(connection);
            delete connection;
        }
    }

    for (it = mappings.begin(); it != mappings.end(); ++it)
        delete it->second;

    deleteAllChildren();
}

//------------------------------------------------------------------------------
void PluginField::paint(Graphics& g)
{
    auto& colours = ColourScheme::getInstance().colours;
    auto bounds = getLocalBounds().toFloat();

    // === Gradient background ===
    Colour bgCol = colours["Field Background"];
    ColourGradient bgGrad(bgCol.brighter(0.08f), 0.0f, 0.0f, bgCol.darker(0.15f), 0.0f, bounds.getHeight(), false);
    g.setGradientFill(bgGrad);
    g.fillRect(bounds);

    // === Grid pattern ===
    float gridSize = 30.0f;
    Colour gridCol = colours["Plugin Border"].withAlpha(0.15f);
    g.setColour(gridCol);

    // Vertical lines
    for (float x = 0.0f; x < bounds.getWidth(); x += gridSize)
    {
        g.drawVerticalLine((int)x, 0.0f, bounds.getHeight());
    }

    // Horizontal lines
    for (float y = 0.0f; y < bounds.getHeight(); y += gridSize)
    {
        g.drawHorizontalLine((int)y, 0.0f, bounds.getWidth());
    }

    if (displayDoubleClickMessage)
    {
        // Draw a centered, polished empty state hint
        auto centerX = bounds.getCentreX();
        auto centerY = bounds.getCentreY();

        // Primary instruction text
        g.setFont(FontManager::getInstance().getUIFont(18.0f));
        g.setColour(ColourScheme::getInstance().colours["Text Colour"].withAlpha(0.6f));

        String hintText = "Double-click to add a plugin";
        auto textWidth = g.getCurrentFont().getStringWidth(hintText);
        g.drawText(hintText, (int)(centerX - textWidth / 2), (int)(centerY - 10), textWidth + 20, 30,
                   Justification::centred, false);

        // Secondary hint
        g.setFont(FontManager::getInstance().getUIFont(13.0f));
        g.setColour(ColourScheme::getInstance().colours["Text Colour"].withAlpha(0.35f));

        String subHint = "or drag & drop VST/preset files";
        auto subWidth = g.getCurrentFont().getStringWidth(subHint);
        g.drawText(subHint, (int)(centerX - subWidth / 2), (int)(centerY + 18), subWidth + 20, 24,
                   Justification::centred, false);
    }
}

//------------------------------------------------------------------------------
void PluginField::mouseDown(const MouseEvent& e)
{
    if (e.getNumberOfClicks() == 2)
    {
        int result = 0;
        PopupMenu menu;

        // Special menu item IDs for actions (above plugin range)
        const int SEARCH_ITEM_ID = 100000;
        const int MANAGE_FAVORITES_BASE = 200000; // IDs 200000+ are for toggling favorites

        // Load favorites and recent from settings
        auto& settings = SettingsManager::getInstance();
        StringArray favorites = settings.getStringArray("PluginFavorites");
        StringArray recentPlugins = settings.getStringArray("RecentPlugins");

        // Collect all plugin types
        auto types = pluginList->getTypes();

        // Build lookup map: pluginIdentifier -> index
        std::map<String, int> identifierToIndex;
        for (int i = 0; i < types.size(); ++i)
        {
            identifierToIndex[types.getReference(i).createIdentifierString()] = i;
        }

        // ★ Favorites section
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
        {
            menu.addSubMenu(CharPointer_UTF8("\xe2\x98\x85 Favorites"), favoritesMenu);
        }

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
        {
            menu.addSubMenu("Recent", recentMenu);
        }

        // Search option
        menu.addItem(SEARCH_ITEM_ID, CharPointer_UTF8("\xf0\x9f\x94\x8d Search..."));

        // Edit Favorites submenu - shows all plugins with checkmarks
        PopupMenu editFavoritesMenu;
        for (int i = 0; i < types.size(); ++i)
        {
            const auto& type = types.getReference(i);
            bool isFavorite = favorites.contains(type.createIdentifierString());
            editFavoritesMenu.addItem(MANAGE_FAVORITES_BASE + i + 1, type.name, true, isFavorite);
        }
        menu.addSubMenu(CharPointer_UTF8("\xe2\x98\x85 Edit Favorites..."), editFavoritesMenu);

        if (favoritesMenu.getNumItems() > 0 || recentMenu.getNumItems() > 0)
        {
            menu.addSeparator();
        }

        // Build category menus
        PopupMenu builtInMenu;
        PopupMenu allPluginsMenu;
        std::map<String, PopupMenu> categoryMenus;

        for (int i = 0; i < types.size(); ++i)
        {
            const auto& type = types.getReference(i);

            if (type.pluginFormatName == "Internal" || type.category == "Built-in")
            {
                builtInMenu.addItem(i + 1, type.name);
            }
            else
            {
                String category = type.category.isNotEmpty() ? type.category : "Uncategorized";
                categoryMenus[category].addItem(i + 1, type.name);
                allPluginsMenu.addItem(i + 1, type.name);
            }
        }

        // Add Pedalboard submenu
        if (builtInMenu.getNumItems() > 0)
        {
            menu.addSubMenu("Pedalboard", builtInMenu);
            menu.addSeparator();
        }

        // Add category submenus
        for (auto& [category, categoryMenu] : categoryMenus)
        {
            menu.addSubMenu(category, categoryMenu);
        }

        // Add All Plugins submenu
        menu.addSeparator();
        menu.addSubMenu("All Plugins", allPluginsMenu);

        result = menu.show();

        // Handle search action
        if (result == SEARCH_ITEM_ID)
        {
            // Show search dialog
            AlertWindow searchDialog("Search Plugins", "Type to filter:", AlertWindow::NoIcon);
            searchDialog.addTextEditor("search", "", "Plugin name:");
            searchDialog.addButton("Cancel", 0);
            searchDialog.addButton("OK", 1);

            if (searchDialog.runModalLoop() == 1)
            {
                String searchText = searchDialog.getTextEditor("search")->getText().toLowerCase();
                if (searchText.isNotEmpty())
                {
                    // Build filtered menu
                    PopupMenu searchResults;
                    for (int i = 0; i < types.size(); ++i)
                    {
                        const auto& type = types.getReference(i);
                        if (type.name.toLowerCase().contains(searchText))
                        {
                            searchResults.addItem(i + 1, type.name);
                        }
                    }

                    if (searchResults.getNumItems() > 0)
                    {
                        result = searchResults.show();
                    }
                    else
                    {
                        AlertWindow::showMessageBox(AlertWindow::InfoIcon, "No Results",
                                                    "No plugins found matching \"" + searchText + "\"");
                        result = 0;
                    }
                }
            }
            else
            {
                result = 0;
            }
        }

        // Handle "Edit Favorites" toggle
        if (result >= MANAGE_FAVORITES_BASE)
        {
            int typeIndex = result - MANAGE_FAVORITES_BASE - 1;
            if (typeIndex >= 0 && typeIndex < types.size())
            {
                String pluginId = types.getReference(typeIndex).createIdentifierString();
                if (favorites.contains(pluginId))
                {
                    favorites.removeString(pluginId);
                }
                else
                {
                    favorites.add(pluginId);
                }
                settings.setStringArray("PluginFavorites", favorites);
            }
            return; // Don't load a plugin, just updated favorites
        }

        if (result > 0 && result < SEARCH_ITEM_ID)
        {
            int pluginIndex = signalPath->getNumFilters() - 1;

            // Get the plugin index from menu result
            int typeIndex = result - 1;
            if (typeIndex >= 0 && typeIndex < types.size())
            {
                // Copy the plugin description (don't take reference to temporary from getTypes())
                PluginDescription pluginType = types.getReference(typeIndex);
                signalPath->addFilter(&pluginType, (double)e.x, (double)e.y);

                // Make sure the plugin got created before we add a component for it.
                if ((signalPath->getNumFilters() - 1) > pluginIndex)
                {
                    pluginIndex = signalPath->getNumFilters() - 1;

                    addFilter(pluginIndex);

                    sendChangeMessage();

                    clearDoubleClickMessage();

                    // Update recent plugins list
                    String pluginId = pluginType.createIdentifierString();
                    recentPlugins.removeString(pluginId); // Remove if already exists
                    recentPlugins.insert(0, pluginId);    // Add to front
                    while (recentPlugins.size() > 8)      // Keep only 8 recent
                    {
                        recentPlugins.remove(recentPlugins.size() - 1);
                    }
                    settings.setStringArray("RecentPlugins", recentPlugins);
                }
            }
        }
    }
    else
    {
        // Single click on empty canvas - begin panning
        if (auto* viewport = findParentComponentOfClass<Viewport>())
        {
            isPanning = true;
            panStartMouse = e.getScreenPosition();
            panStartScroll = viewport->getViewPosition();
            setMouseCursor(MouseCursor::DraggingHandCursor);
        }
    }
}

//------------------------------------------------------------------------------
void PluginField::mouseDrag(const MouseEvent& e)
{
    if (isPanning)
    {
        if (auto* viewport = findParentComponentOfClass<Viewport>())
        {
            Point<int> delta = panStartMouse - e.getScreenPosition();
            Point<int> targetPosition = panStartScroll + delta;

            // Expand canvas if we're trying to pan beyond current bounds
            int currentWidth = getWidth();
            int currentHeight = getHeight();
            int viewWidth = viewport->getViewWidth();
            int viewHeight = viewport->getViewHeight();

            // Calculate how much we need to expand
            int neededWidth = targetPosition.x + viewWidth;
            int neededHeight = targetPosition.y + viewHeight;

            bool sizeChanged = false;
            if (neededWidth > currentWidth)
            {
                currentWidth = neededWidth + 200; // Add some buffer
                sizeChanged = true;
            }
            if (neededHeight > currentHeight)
            {
                currentHeight = neededHeight + 200; // Add some buffer
                sizeChanged = true;
            }

            if (sizeChanged)
            {
                setSize(currentWidth, currentHeight);
            }

            // Expand canvas upward/leftward if trying to pan past origin
            if (targetPosition.x < 0)
            {
                int expandBy = -targetPosition.x + 100;
                setSize(getWidth() + expandBy, getHeight());
                // Move all child components right
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
                // Move all child components down
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

//------------------------------------------------------------------------------
void PluginField::mouseUp(const MouseEvent& e)
{
    if (isPanning)
    {
        isPanning = false;
        setMouseCursor(MouseCursor::NormalCursor);
    }
}

//------------------------------------------------------------------------------
void PluginField::mouseWheelMove(const MouseEvent& e, const MouseWheelDetails& wheel)
{
    // Zoom with scroll wheel
    float zoomDelta = wheel.deltaY * 0.1f;
    float newZoom = jlimit(minZoom, maxZoom, zoomLevel + zoomDelta);

    if (newZoom != zoomLevel)
    {
        // Get mouse position relative to this component for zoom centering
        auto mousePos = e.getPosition();

        // Calculate the point we're zooming towards in unscaled coordinates
        float scaleRatio = newZoom / zoomLevel;

        zoomLevel = newZoom;
        setTransform(AffineTransform::scale(zoomLevel));

        // Adjust viewport to zoom towards mouse position
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

//------------------------------------------------------------------------------
void PluginField::fitToScreen()
{
    if (auto* viewport = findParentComponentOfClass<Viewport>())
    {
        // Find bounding box of all visible nodes
        Rectangle<int> bounds;
        bool first = true;

        for (int i = 0; i < getNumChildComponents(); ++i)
        {
            if (auto* comp = getChildComponent(i))
            {
                if (first)
                {
                    bounds = comp->getBounds();
                    first = false;
                }
                else
                {
                    bounds = bounds.getUnion(comp->getBounds());
                }
            }
        }

        if (!first && !bounds.isEmpty())
        {
            // Add padding
            bounds = bounds.expanded(50);

            // Calculate zoom to fit
            float viewWidth = static_cast<float>(viewport->getViewWidth());
            float viewHeight = static_cast<float>(viewport->getViewHeight());
            float boundsWidth = static_cast<float>(bounds.getWidth());
            float boundsHeight = static_cast<float>(bounds.getHeight());

            float zoomToFit = jmin(viewWidth / boundsWidth, viewHeight / boundsHeight);
            zoomLevel = jlimit(minZoom, maxZoom, zoomToFit);

            setTransform(AffineTransform::scale(zoomLevel));

            // Center the view on the nodes
            int centeredX = static_cast<int>(bounds.getCentreX() * zoomLevel - viewWidth / 2);
            int centeredY = static_cast<int>(bounds.getCentreY() * zoomLevel - viewHeight / 2);
            viewport->setViewPosition(jmax(0, centeredX), jmax(0, centeredY));

            repaint();
        }
    }
}

//------------------------------------------------------------------------------
void PluginField::changeListenerCallback(ChangeBroadcaster* source)
{
    PluginComponent* pluginComp = dynamic_cast<PluginComponent*>(source);

    if (pluginComp)
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

//------------------------------------------------------------------------------
void PluginField::timerCallback()
{
    int i;

    for (i = 0; i < getNumChildComponents(); ++i)
    {
        PluginComponent* comp = dynamic_cast<PluginComponent*>(getChildComponent(i));

        if (comp)
            comp->timerUpdate();
    }
}

//------------------------------------------------------------------------------
bool PluginField::isInterestedInFileDrag(const StringArray& files)
{
    int i;
    bool retval = false;

    for (i = 0; i < files.size(); ++i)
    {
        // If they're plugins.
#ifdef WIN32
        if (files[i].endsWith(".dl"))
        {
            retval = true;
            break;
        }
#elif defined(__APPLE__)
        if (files[i].endsWith(".vst") || files[i].endsWith(".component"))
        {
            retval = true;
            break;
        }
#endif
        // If they're sound files.
        else if (files[i].endsWith(".wav") || files[i].endsWith(".aif") || files[i].endsWith(".aiff") ||
                 files[i].endsWith(".ogg") || files[i].endsWith(".flac") || files[i].endsWith(".wma"))
        {
            retval = true;
            break;
        }
    }

    return retval;
}

//------------------------------------------------------------------------------
void PluginField::filesDropped(const StringArray& files, int x, int y)
{
    int i;
    bool soundsInArray = false;
    bool pluginsInArray = false;
    OwnedArray<PluginDescription> foundPlugins;

    for (i = 0; i < files.size(); ++i)
    {
        // If they're plugins.
#ifdef WIN32
        if (files[i].endsWith(".dl"))
            pluginsInArray = true;
#elif defined(__APPLE__)
        if (files[i].endsWith(".vst") || files[i].endsWith(".component"))
            pluginsInArray = true;
#endif
        // If they're sound files.
        else if (files[i].endsWith(".wav") || files[i].endsWith(".aif") || files[i].endsWith(".aiff") ||
                 files[i].endsWith(".ogg") || files[i].endsWith(".flac") || files[i].endsWith(".wma"))
        {
            soundsInArray = true;
        }
    }

    if (pluginsInArray)
    {
        pluginList->scanAndAddDragAndDroppedFiles(AudioPluginFormatManagerSingleton::getInstance(), files,
                                                  foundPlugins);

        for (i = 0; i < foundPlugins.size(); ++i)
        {
            int pluginIndex = signalPath->getNumFilters() - 1;

            signalPath->addFilter(foundPlugins[i], (double)x, (double)y);

            // Make sure the plugin got created before we add a component for it.
            if ((signalPath->getNumFilters() - 1) > pluginIndex)
            {
                pluginIndex = signalPath->getNumFilters() - 1;

                addFilter(pluginIndex);

                sendChangeMessage();
            }

            x += 100;
            y += 100;
        }
    }

    if (soundsInArray)
    {
        for (i = 0; i < files.size(); ++i)
        {
            int pluginIndex = signalPath->getNumFilters() - 1;

            signalPath->addFilter(new FilePlayerProcessor(File(files[i])), (double)x, (double)y);

            // Make sure the plugin got created before we add a component for it.
            if ((signalPath->getNumFilters() - 1) > pluginIndex)
            {
                pluginIndex = signalPath->getNumFilters() - 1;

                addFilter(pluginIndex);

                sendChangeMessage();

                clearDoubleClickMessage();
            }
        }
    }
}

//------------------------------------------------------------------------------
bool PluginField::getCurrentPosition(CurrentPositionInfo& result)
{
    result.bpm = tempo;
    result.timeSigNumerator = 4;
    result.timeSigDenominator = 4;
    result.timeInSeconds = 0.0;
    result.editOriginTime = 0.0;
    result.ppqPosition = 0.0;
    result.ppqPositionOfLastBarStart = 0.0;
    result.frameRate = AudioPlayHead::fpsUnknown;
    result.isPlaying = MainTransport::getInstance()->getState();
    result.isRecording = false;

    return true;
}

//------------------------------------------------------------------------------
// JUCE 8: New getPosition() method that returns Optional<PositionInfo>
Optional<AudioPlayHead::PositionInfo> PluginField::getPosition() const
{
    PositionInfo result;
    result.setBpm(tempo);
    result.setTimeSignature(TimeSignature{4, 4});
    result.setTimeInSeconds(0.0);
    result.setEditOriginTime(0.0);
    result.setPpqPosition(0.0);
    result.setPpqPositionOfLastBarStart(0.0);
    result.setFrameRate(FrameRate());
    result.setIsPlaying(MainTransport::getInstance()->getState());
    result.setIsRecording(false);

    return result;
}

//------------------------------------------------------------------------------
void PluginField::enableAudioInput(bool val)
{
    int i;

    audioInputEnabled = val;

    if (!val)
    {
        // Delete the filter(s) in the signal path.
        for (i = (signalPath->getNumFilters() - 1); i >= 0; --i)
        {
            if (signalPath->getNode(i)->getProcessor()->getName() == "Audio Input")
                deleteFilter(signalPath->getNode(i).get()); // JUCE 8: Node::Ptr
        }

        // Delete the associated "Audio Input" PluginComponent(s).
        for (i = (getNumChildComponents() - 1); i >= 0; --i)
        {
            PluginComponent* comp = dynamic_cast<PluginComponent*>(getChildComponent(i));

            if (comp)
            {
                if (comp->getNode()->getProcessor()->getName() == "Audio Input")
                    delete removeChildComponent(i);
            }
        }
    }
    else
    {
        InternalPluginFormat internalFormat;

        // Add the filter to the signal path.
        signalPath->addFilter(internalFormat.getDescriptionFor(InternalPluginFormat::audioInputFilter), 10.0f, 10.0f);

        // Add the associated PluginComponent.
        addFilter(signalPath->getNumFilters() - 1);
    }
}

//------------------------------------------------------------------------------
void PluginField::enableMidiInput(bool val)
{
    int i;
    AudioProcessorGraph::Node* tempNode = 0;
    multimap<uint32, Mapping*>::iterator it;

    midiInputEnabled = val;

    if (!val)
    {
        // Delete mappings.
        for (it = mappings.begin(); it != mappings.end();)
        {
            MidiMapping* midiMapping = dynamic_cast<MidiMapping*>(it->second);

            if (midiMapping)
            {
                delete it->second;
                mappings.erase(it++);
            }
            else
                ++it;
        }

        // Midi Input filter.
        {
            // Delete filter.
            for (i = (signalPath->getNumFilters() - 1); i >= 0; --i)
            {
                if (signalPath->getNode(i)->getProcessor()->getName() == "Midi Input")
                {
                    tempNode = signalPath->getNode(i).get(); // JUCE 8: Node::Ptr
                    deleteFilter(tempNode);
                }
            }

            // Delete PluginComponent.
            for (i = (getNumChildComponents() - 1); i >= 0; --i)
            {
                PluginComponent* comp = dynamic_cast<PluginComponent*>(getChildComponent(i));

                if (comp)
                {
                    if (comp->getNode() == tempNode)
                        delete removeChildComponent(i);
                }
            }
        }
        // Midi Interceptor filter.
        {
            // Delete filter.
            for (i = (signalPath->getNumFilters() - 1); i >= 0; --i)
            {
                if (signalPath->getNode(i)->getProcessor()->getName() == "Midi Interceptor")
                    deleteFilter(signalPath->getNode(i).get()); // JUCE 8: Node::Ptr
            }
        }
    }
    else
    {
        InternalPluginFormat internalFormat;

        // Add the filter to the signal path.
        signalPath->addFilter(internalFormat.getDescriptionFor(InternalPluginFormat::midiInputFilter), 10.0f, 120.0f);

        // Add the associated PluginComponent.
        addFilter(signalPath->getNumFilters() - 1);

        // Add the Midi Interceptor too.
        {
            MidiInterceptor p;
            PluginDescription desc;

            p.fillInPluginDescription(desc);

            signalPath->addFilter(&desc, 100, 100);

            // And connect it up to the midi input.
            {
                AudioProcessorGraph::NodeID midiInput;
                AudioProcessorGraph::NodeID midiInterceptor;

                for (i = 0; i < signalPath->getNumFilters(); ++i)
                {
                    if (signalPath->getNode(i)->getProcessor()->getName() == "Midi Input")
                        midiInput = signalPath->getNode(i)->nodeID; // JUCE 8: NodeID struct
                    else if (signalPath->getNode(i)->getProcessor()->getName() == "Midi Interceptor")
                    {
                        midiInterceptor = signalPath->getNode(i)->nodeID; // JUCE 8: NodeID struct
                        dynamic_cast<MidiInterceptor*>(signalPath->getNode(i)->getProcessor())
                            ->setManager(&midiManager);
                    }
                }
                signalPath->addConnection(midiInput, AudioProcessorGraph::midiChannelIndex, midiInterceptor,
                                          AudioProcessorGraph::midiChannelIndex);
            }
        }
    }
}

//------------------------------------------------------------------------------
void PluginField::enableOscInput(bool val)
{
    int i;
    multimap<uint32, Mapping*>::iterator it;
    AudioProcessorGraph::Node* tempNode = 0;

    oscInputEnabled = val;

    if (!val)
    {
        // Delete mappings.
        for (it = mappings.begin(); it != mappings.end();)
        {
            OscMapping* oscMapping = dynamic_cast<OscMapping*>(it->second);

            if (oscMapping)
            {
                delete it->second;
                mappings.erase(it++);
            }
            else
                ++it;
        }

        // Delete filter.
        for (i = (signalPath->getNumFilters() - 1); i >= 0; --i)
        {
            if (signalPath->getNode(i)->getProcessor()->getName() == "OSC Input")
            {
                tempNode = signalPath->getNode(i).get(); // JUCE 8: Node::Ptr
                deleteFilter(tempNode);
            }
        }

        // Delete PluginComponent.
        for (i = (getNumChildComponents() - 1); i >= 0; --i)
        {
            PluginComponent* comp = dynamic_cast<PluginComponent*>(getChildComponent(i));

            if (comp)
            {
                if (comp->getNode() == tempNode)
                    delete removeChildComponent(i);
            }
        }
    }
    else
    {
        OscInput p;
        PluginDescription desc;

        p.fillInPluginDescription(desc);

        signalPath->addFilter(&desc, 10, 215);

        addFilter(signalPath->getNumFilters() - 1);
    }
}

//------------------------------------------------------------------------------
void PluginField::setAutoMappingsWindow(bool val)
{
    autoMappingsWindow = val;
}

//------------------------------------------------------------------------------
void PluginField::setTempo(double val)
{
    tempo = val;
}

//------------------------------------------------------------------------------
void PluginField::addFilter(int index, bool broadcastChangeMessage)
{
    int x, y;
    PluginComponent* plugin;
    AudioProcessorGraph::Node* node;

    if (index < signalPath->getNumFilters())
    {
        node = signalPath->getNode(index).get(); // JUCE 8: Node::Ptr

        // Skip creating UI for internal/hidden nodes
        auto processorName = node->getProcessor()->getName();
        if (processorName != "Midi Interceptor" && processorName != "SafetyLimiter")
        {
            // Make sure the plugin knows about the AudioPlayHead.
            node->getProcessor()->setPlayHead(this);

            plugin = new PluginComponent(node);
            x = signalPath->getNode(index)->properties.getWithDefault("x", 0);
            y = signalPath->getNode(index)->properties.getWithDefault("y", 0);
            plugin->setTopLeftPosition(x, y);
            plugin->addChangeListener(this);
            addAndMakeVisible(plugin);

            if (LogFile::getInstance().getIsLogging())
            {
                String tempstr;

                tempstr << "Added plugin to signal path: " << node->getProcessor()->getName();
                LogFile::getInstance().logEvent("Pedalboard", tempstr);
            }

            // To make sure the plugin field bounds are correct.
            changeListenerCallback(plugin);

            if (broadcastChangeMessage)
                sendChangeMessage();
        }
    }
}

//------------------------------------------------------------------------------
void PluginField::deleteFilter(AudioProcessorGraph::Node* node)
{
    int i;
    PluginConnection* connection;
    const uint32 uid = node->nodeID.uid; // JUCE 8: NodeID struct
    multimap<uint32, Mapping*>::iterator it;
    String pluginName = node->getProcessor()->getName();

    // Disconnect any PluginConnections.
    for (i = getNumChildComponents(); i >= 0; --i)
    {
        connection = dynamic_cast<PluginConnection*>(getChildComponent(i));

        if (connection)
        {
            const PluginPinComponent* src = connection->getSource();
            const PluginPinComponent* dest = connection->getDestination();
            uint32 srcId, destId;

            // Had a crash here once, where dest was 0. Not exactly sure why that
            // happened, but the following will at least prevent it happening
            // again...
            if (src)
                srcId = src->getUid();
            else
                srcId = 4294967295u;
            if (dest)
                destId = dest->getUid();
            else
                destId = 4294967295u;

            if ((uid == srcId) || (uid == destId))
            {
                removeChildComponent(connection);
                delete connection;
            }
        }
    }

    // Delete any associated mappings.
    for (it = mappings.lower_bound(uid); it != mappings.upper_bound(uid); ++it)
    {
        delete it->second;
    }
    mappings.erase(uid);

    // Unregister the filter from wanting MIDI over OSC.
    {
        BypassableInstance* proc = dynamic_cast<BypassableInstance*>(node->getProcessor());

        if (proc)
            oscManager.unregisterMIDIProcessor(proc);
    }

    signalPath->disconnectFilter(AudioProcessorGraph::NodeID(uid));
    signalPath->removeFilter(AudioProcessorGraph::NodeID(uid));

    if (LogFile::getInstance().getIsLogging())
    {
        String tempstr;

        tempstr << "Deleted plugin from signal path: " << pluginName;
        LogFile::getInstance().logEvent("Pedalboard", tempstr);
    }

    sendChangeMessage();
}

//------------------------------------------------------------------------------
void PluginField::updateProcessorName(uint32 id, const String& val)
{
    userNames[id] = val;
}

//------------------------------------------------------------------------------
void PluginField::addConnection(PluginPinComponent* source, bool connectAll)
{
    if (source)
    {
        PluginConnection* connection = new PluginConnection(source, 0, connectAll);

        connection->setSize(10, 12);
        addAndMakeVisible(connection);
        connection->toFront(false); // Bring dragging connection to front
        connection->setInterceptsMouseClicks(false, false); // Don't intercept mouse while dragging
        draggingConnection = connection;

        sendChangeMessage();
    }
}

//------------------------------------------------------------------------------
void PluginField::dragConnection(int x, int y)
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
void PluginField::releaseConnection(int x, int y)
{
    if (draggingConnection)
    {
        // PluginPinComponent *p = dynamic_cast<PluginPinComponent
        // *>(getComponentAt(x, y));
        Component* c = getPinAt(x, y);
        PluginPinComponent* p = dynamic_cast<PluginPinComponent*>(c);

        /*testX = x;
        testY = y;*/
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
                    signalPath->addConnection(AudioProcessorGraph::NodeID(outputPin->getUid()), outputPin->getChannel(),
                                              AudioProcessorGraph::NodeID(inputPin->getUid()), inputPin->getChannel());
                    draggingConnection->setDestination(p);
                    draggingConnection->setInterceptsMouseClicks(true, true); // Re-enable mouse clicks for finalized connection

                    // If we should be connecting all the outputs and inputs of the two
                    // plugins (user holding down shift).
                    if (draggingConnection->getRepresentsAllOutputs())
                    {
                        connectAll(draggingConnection);
                        draggingConnection->setRepresentsAllOutputs(false);
                    }

                    if (p->getParameterPin())
                    {
                        PluginComponent* pComp = dynamic_cast<PluginComponent*>(p->getParentComponent());

                        if (pComp && autoMappingsWindow)
                            pComp->openMappingsWindow();
                    }
                    moveConnectionsBehind();

                    sendChangeMessage();
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
        draggingConnection = 0;
    }
}

//------------------------------------------------------------------------------
void PluginField::deleteConnection()
{
    int i;

    for (i = (getNumChildComponents() - 1); i >= 0; --i)
    {
        PluginConnection* c = dynamic_cast<PluginConnection*>(getChildComponent(i));

        if (c)
        {
            if (c->getSelected())
            {
                const PluginPinComponent* s = c->getSource();
                const PluginPinComponent* d = c->getDestination();

                signalPath->removeConnection(AudioProcessorGraph::NodeID(s->getUid()), s->getChannel(),
                                             AudioProcessorGraph::NodeID(d->getUid()), d->getChannel());
                removeChildComponent(c);
                delete c;

                // If it's a param connection, delete any MIDI or OSC mappings.
                if (c->getParameterConnection())
                {
                    uint32 sourceId = s->getUid();
                    uint32 destId = d->getUid();
                    String tempstr =
                        signalPath->getNodeForId(AudioProcessorGraph::NodeID(sourceId))->getProcessor()->getName();

                    // It's a Midi connection, so delete any associated Midi
                    // mappings for the destination plugin.
                    if (tempstr == "Midi Input")
                    {
                        multimap<uint32, Mapping*>::iterator it;

                        for (it = mappings.lower_bound(destId); it != mappings.upper_bound(destId);)
                        {
                            MidiMapping* mapping = dynamic_cast<MidiMapping*>(it->second);

                            if (mapping)
                            {
                                delete mapping;
                                mappings.erase(it++);
                            }
                            else
                                ++it;
                        }
                    }
                    else if (tempstr == "OSC Input")
                    {
                        multimap<uint32, Mapping*>::iterator it;

                        for (it = mappings.lower_bound(destId); it != mappings.upper_bound(destId);)
                        {
                            OscMapping* mapping = dynamic_cast<OscMapping*>(it->second);

                            if (mapping)
                            {
                                delete it->second;
                                mappings.erase(it++);
                            }
                            else
                                ++it;
                        }
                    }
                }
                sendChangeMessage();

                repaint();
            }
        }
    }
}

//------------------------------------------------------------------------------
void PluginField::enableMidiForNode(AudioProcessorGraph::Node* node, bool val)
{
    int i;
    bool connection = false;
    AudioProcessorGraph::Node* midiInput = 0;

    // Find the Midi Input node.
    for (i = 0; i < signalPath->getNumFilters(); ++i)
    {
        auto midiInputNode = signalPath->getNode(i);

        if (midiInputNode->getProcessor()->getName() == "Midi Input")
        {
            midiInput = midiInputNode.get();
            break;
        }
    }
    // Just in case.
    if (midiInput)
    {
        if (midiInput->getProcessor()->getName() != "Midi Input")
            return;
    }
    else
        return;

    // Check if there's a connection.
    connection = (signalPath->getConnectionBetween(midiInput->nodeID, AudioProcessorGraph::midiChannelIndex,
                                                   node->nodeID, AudioProcessorGraph::midiChannelIndex) != 0);
    if (val)
    {
        // If there's a connection, remove it.
        signalPath->removeConnection(midiInput->nodeID, AudioProcessorGraph::midiChannelIndex, node->nodeID,
                                     AudioProcessorGraph::midiChannelIndex);
    }
    else
    {
        // If there's not a connection, add it.
        signalPath->addConnection(midiInput->nodeID, AudioProcessorGraph::midiChannelIndex, node->nodeID,
                                  AudioProcessorGraph::midiChannelIndex);
    }
}

//------------------------------------------------------------------------------
bool PluginField::getMidiEnabledForNode(AudioProcessorGraph::Node* node) const
{
    int i;
    AudioProcessorGraph::Node* midiInput = 0;

    // Find the Midi Input node.
    for (i = 0; i < signalPath->getNumFilters(); ++i)
    {
        auto midiInputNode = signalPath->getNode(i);

        if (midiInputNode->getProcessor()->getName() == "Midi Input")
        {
            midiInput = midiInputNode.get();
            break;
        }
        else
            midiInput = nullptr;
    }

    if (!midiInput)
        return false;
    else
        return signalPath->getConnectionBetween(midiInput->nodeID, AudioProcessorGraph::midiChannelIndex, node->nodeID,
                                                AudioProcessorGraph::midiChannelIndex) != 0;
}

//------------------------------------------------------------------------------
void PluginField::addMapping(Mapping* mapping)
{
    mappings.insert(make_pair(mapping->getPluginId(), mapping));
    sendChangeMessage();
}

//------------------------------------------------------------------------------
void PluginField::removeMapping(Mapping* mapping)
{
    multimap<uint32, Mapping*>::iterator it;

    for (it = mappings.begin(); it != mappings.end(); ++it)
    {
        if (it->second == mapping)
        {
            delete it->second;
            mappings.erase(it);
            break;
        }
    }
    sendChangeMessage();
}

//------------------------------------------------------------------------------
Array<Mapping*> PluginField::getMappingsForPlugin(uint32 id)
{
    Array<Mapping*> retval;
    multimap<uint32, Mapping*>::iterator it;

    for (it = mappings.lower_bound(id); it != mappings.upper_bound(id); ++it)
    {
        retval.add(it->second);
    }

    return retval;
}

//------------------------------------------------------------------------------
void PluginField::socketDataArrived(char* data, int32 dataSize)
{
    if (OSC::Bundle::isBundle(data, dataSize))
    {
        OSC::Bundle bundle(data, dataSize);

        handleOscBundle(&bundle);
    }
    else if (OSC::Message::isMessage(data, dataSize))
    {
        OSC::Message message(data, dataSize);

        oscManager.messageReceived(&message);
    }
}

//------------------------------------------------------------------------------
XmlElement* PluginField::getXml() const
{
    int i;
    map<uint32, String>::const_iterator it2;
    multimap<uint32, Mapping*>::const_iterator it;
    XmlElement* retval = new XmlElement("Patch");
    XmlElement* mappingsXml = new XmlElement("Mappings");
    XmlElement* userNamesXml = new XmlElement("UserNames");

    // Update saved window positions.
    for (i = 0; i < getNumChildComponents(); ++i)
    {
        PluginComponent* plugin = dynamic_cast<PluginComponent*>(getChildComponent(i));

        if (plugin)
            plugin->saveWindowState();
    }

    // Set the patch tempo.
    retval->setAttribute("tempo", tempo);

    // Add FilterGraph.
    retval->addChildElement(signalPath->createXml(oscManager));

    // Add Mappings.
    for (it = mappings.begin(); it != mappings.end(); ++it)
        mappingsXml->addChildElement(it->second->getXml());
    retval->addChildElement(mappingsXml);

    // Add user names.
    for (it2 = userNames.begin(); it2 != userNames.end(); ++it2)
    {
        XmlElement* nameXml = new XmlElement("Name");

        nameXml->setAttribute("id", (int)it2->first);
        nameXml->setAttribute("va", it2->second);

        userNamesXml->addChildElement(nameXml);
    }
    retval->addChildElement(userNamesXml);

    return retval;
}

#ifdef __APPLE__
struct NodeAndId
{
    AudioProcessorGraph::Node* node;
    uint32 id;
};
#endif

//------------------------------------------------------------------------------
void PluginField::loadFromXml(XmlElement* patch)
{
    int i, j;
    Array<uint32> paramConnections;

    // Delete all the filter and connection components.
    {
        // If we don't do this, the connections will try to contact their pins,
        // which may have already been deleted.
        for (i = (getNumChildComponents() - 1); i >= 0; --i)
        {
            PluginConnection* connection = dynamic_cast<PluginConnection*>(getChildComponent(i));

            if (connection)
            {
                removeChildComponent(connection);
                delete connection;
            }
        }
    }
    deleteAllChildren();
    repaint();

    // Wipe userNames.
    userNames.clear();

    // Clear and possibly load the signal path.
    clearMappings();
    if (patch)
    {
        tempo = patch->getDoubleAttribute("tempo", 120.0);

        signalPath->clear(false, false, false);
        signalPath->restoreFromXml(*(patch->getChildByName("FILTERGRAPH")), oscManager);
    }
    else
        signalPath->clear(audioInputEnabled, midiInputEnabled);

    // Add the filter components.
    for (i = 0; i < signalPath->getNumFilters(); ++i)
        addFilter(i, false);

    // Update any plugin names.
    {
        XmlElement* userNamesXml = patch->getChildByName("UserNames");

        if (userNamesXml)
        {
            forEachXmlChildElement(*userNamesXml, e)
            {
                if (e->hasTagName("Name"))
                {
                    uint32 id = e->getIntAttribute("id");
                    String name = e->getStringAttribute("va");

                    for (i = 0; i < getNumChildComponents(); ++i)
                    {
                        PluginComponent* pluginComp = dynamic_cast<PluginComponent*>(getChildComponent(i));

                        if (pluginComp)
                        {
                            if (pluginComp->getNode()->nodeID.uid == id)
                            {
                                pluginComp->setUserName(name);
                                userNames[id] = name;
                                break;
                            }
                        }
                    }
                }
            }
        }
    }

    // Add the audio/midi connections.
    {
#ifndef __APPLE__ // Stupid gcc...
        struct NodeAndId
        {
            AudioProcessorGraph::Node::Ptr node; // JUCE 8: Use smart pointer
            uint32 id;
        };
#endif
        Array<NodeAndId> tempNodes;

        // Stick all the nodes and their ids in an array.
        for (i = 0; i < signalPath->getNumFilters(); ++i)
        {
            NodeAndId n;

            n.node = signalPath->getNode(i);
            n.id = n.node->nodeID.uid; // JUCE 8: NodeID struct
            tempNodes.add(n);
        }

        // Add the audio/midi connections.
        for (const auto& connection : signalPath->getConnections())
        {
            AudioProcessorGraph::Node* sourceNode = nullptr;
            AudioProcessorGraph::Node* destNode = nullptr;
            PluginComponent* sourceComp = 0;
            PluginComponent* destComp = 0;
            PluginPinComponent* sourcePin;
            PluginPinComponent* destPin;

            // Fill out sourceNode and destNode.
            for (j = 0; j < tempNodes.size(); ++j)
            {
                if (tempNodes[j].id == connection.source.nodeID.uid)
                    sourceNode = tempNodes[j].node.get();
                else if (tempNodes[j].id == connection.destination.nodeID.uid)
                    destNode = tempNodes[j].node.get();
            }

            if (!sourceNode || !destNode)
            {
                jassertfalse;
                continue;
            }
            else if (destNode->getProcessor()->getName() == "Midi Interceptor")
                continue;

            // Now get the source and destination components.
            for (j = 0; j < getNumChildComponents(); ++j)
            {
                PluginComponent* pluginComp = dynamic_cast<PluginComponent*>(getChildComponent(j));

                if (pluginComp)
                {
                    if (pluginComp->getNode() == sourceNode)
                        sourceComp = pluginComp;
                    else if (pluginComp->getNode() == destNode)
                        destComp = pluginComp;
                }
            }

            if (!sourceComp || !destComp)
            {
                jassertfalse;
                continue;
            }

            if ((connection.source.channelIndex == connection.destination.channelIndex) &&
                (connection.source.channelIndex == AudioProcessorGraph::midiChannelIndex))
            {
                sourcePin = sourceComp->getParamPin(0);
                destPin = destComp->getParamPin(0);

                paramConnections.add(connection.destination.nodeID.uid);
            }
            else
            {
                sourcePin = sourceComp->getOutputPin(connection.source.channelIndex);
                destPin = destComp->getInputPin(connection.destination.channelIndex);
            }

            if (!sourcePin || !destPin)
            {
                jassertfalse;
                continue;
            }

            addAndMakeVisible(new PluginConnection(sourcePin, destPin));
        }
    }

    // Add the mappings.
    {
        XmlElement* mappingsXml = patch->getChildByName("Mappings");

        if (mappingsXml)
        {
            forEachXmlChildElement(*mappingsXml, e)
            {
                if (e->hasTagName("MidiMapping"))
                {
                    MidiMapping* mapping = new MidiMapping(&midiManager, signalPath, e);
                    midiManager.registerMapping(mapping->getCc(), mapping);

                    mappings.insert(make_pair(mapping->getPluginId(), mapping));
                }
                else if (e->hasTagName("OscMapping"))
                {
                    OscMapping* mapping = new OscMapping(&oscManager, signalPath, e);
                    oscManager.registerMapping(mapping->getAddress(), mapping);

                    mappings.insert(make_pair(mapping->getPluginId(), mapping));
                }
            }
        }
    }

    // Connect the Midi Interceptor to the MidiMappingManager.
    if (midiInputEnabled)
    {
        MidiInterceptor* interceptor = 0;

        for (i = 0; i < signalPath->getNumFilters(); ++i)
        {
            interceptor = dynamic_cast<MidiInterceptor*>(signalPath->getNode(i)->getProcessor());

            if (interceptor)
            {
                interceptor->setManager(&midiManager);
                break;
            }
        }
    }

    // Add in any parameter mapping connections.
    {
        multimap<uint32, Mapping*>::iterator it;
        PluginPinComponent* midiInput = 0;
        PluginPinComponent* oscInput = 0;

        // Get the Midi Input and OSC Input pins.
        for (i = 0; i < getNumChildComponents(); ++i)
        {
            PluginComponent* comp = dynamic_cast<PluginComponent*>(getChildComponent(i));

            if (comp)
            {
                String tempstr = comp->getNode()->getProcessor()->getName();

                if (tempstr == "Midi Input")
                    midiInput = comp->getParamPin(0);
                else if (tempstr == "OSC Input")
                    oscInput = comp->getParamPin(0);
            }
        }

        /*jassert(midiInput);
        jassert(oscInput);*/

        if (midiInputEnabled && midiInput)
        {
            // Add a connection for each Midi mapping, checking that we don't already
            // have an identical one.
            for (it = mappings.begin(); it != mappings.end(); ++it)
            {
                MidiMapping* midiMapping = dynamic_cast<MidiMapping*>(it->second);

                if (midiMapping)
                {
                    uint32 uid = midiMapping->getPluginId();

                    if (!paramConnections.contains(uid))
                    {
                        // Find the PluginComponent matching this uid.
                        for (i = 0; i < getNumChildComponents(); ++i)
                        {
                            PluginComponent* comp = dynamic_cast<PluginComponent*>(getChildComponent(i));

                            if (comp)
                            {
                                if (comp->getNode()->nodeID.uid == uid)
                                {
                                    PluginPinComponent* paramInput = 0;

                                    for (j = 0; j < comp->getNumParamPins(); ++j)
                                    {
                                        if (!comp->getParamPin(j)->getDirection())
                                        {
                                            paramInput = comp->getParamPin(j);
                                            break;
                                        }
                                    }

                                    jassert(paramInput);

                                    addAndMakeVisible(new PluginConnection(midiInput, paramInput));
                                    paramConnections.add(uid);

                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }

        if (oscInputEnabled && oscInput)
        {
            // Ditto for the osc mappings.
            paramConnections.clear();
            for (it = mappings.begin(); it != mappings.end(); ++it)
            {
                OscMapping* oscMapping = dynamic_cast<OscMapping*>(it->second);

                if (oscMapping)
                {
                    uint32 uid = oscMapping->getPluginId();

                    if (!paramConnections.contains(uid))
                    {
                        // Find the PluginComponent matching this uid.
                        for (i = 0; i < getNumChildComponents(); ++i)
                        {
                            PluginComponent* comp = dynamic_cast<PluginComponent*>(getChildComponent(i));

                            if (comp)
                            {
                                if (comp->getNode()->nodeID.uid == uid)
                                {
                                    PluginPinComponent* paramInput = 0;

                                    for (j = 0; j < comp->getNumParamPins(); ++j)
                                    {
                                        if (!comp->getParamPin(j)->getDirection())
                                        {
                                            paramInput = comp->getParamPin(j);
                                            break;
                                        }
                                    }

                                    jassert(paramInput);

                                    addAndMakeVisible(new PluginConnection(oscInput, paramInput));
                                    paramConnections.add(uid);

                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // Make sure any disabled inputs/outputs don't get accidentally re-enabled.
    if (!audioInputEnabled)
        enableAudioInput(audioInputEnabled);
    if (!midiInputEnabled)
        enableMidiInput(midiInputEnabled);
    if (!oscInputEnabled)
        enableOscInput(oscInputEnabled);

    moveConnectionsBehind();
    repaint();
}

//------------------------------------------------------------------------------
void PluginField::clear()
{
    int i;

    // Delete all the filter and connection components.
    {
        // If we don't do this, the connections will try to contact their pins,
        // which may have already been deleted.
        for (i = (getNumChildComponents() - 1); i >= 0; --i)
        {
            PluginConnection* connection = dynamic_cast<PluginConnection*>(getChildComponent(i));

            if (connection)
            {
                removeChildComponent(connection);
                delete connection;
            }
        }
    }
    deleteAllChildren();
    repaint();

    // Wipe userNames.
    userNames.clear();

    // Clear any mappings.
    clearMappings();

    // Clear the signal path.
    signalPath->clear(audioInputEnabled, midiInputEnabled);

    // Add OSC input.
    if (oscInputEnabled)
    {
        OscInput p;
        PluginDescription desc;

        p.fillInPluginDescription(desc);

        signalPath->addFilter(&desc, 10, 215);
    }

    // Setup gui.
    for (i = 0; i < signalPath->getNumFilters(); ++i)
        addFilter(i);

    // Add MidiInterceptor.
    if (midiInputEnabled)
    {
        MidiInterceptor p;
        PluginDescription desc;

        p.fillInPluginDescription(desc);

        signalPath->addFilter(&desc, 100, 100);

        // And connect it up to the midi input.
        {
            AudioProcessorGraph::NodeID midiInput;
            AudioProcessorGraph::NodeID midiInterceptor;

            for (i = 0; i < signalPath->getNumFilters(); ++i)
            {
                if (signalPath->getNode(i)->getProcessor()->getName() == "Midi Input")
                    midiInput = signalPath->getNode(i)->nodeID; // JUCE 8: NodeID struct
                else if (signalPath->getNode(i)->getProcessor()->getName() == "Midi Interceptor")
                {
                    midiInterceptor = signalPath->getNode(i)->nodeID; // JUCE 8: NodeID struct
                    dynamic_cast<MidiInterceptor*>(signalPath->getNode(i)->getProcessor())->setManager(&midiManager);
                }
            }
            signalPath->addConnection(midiInput, AudioProcessorGraph::midiChannelIndex, midiInterceptor,
                                      AudioProcessorGraph::midiChannelIndex);
        }
    }

    repaint();
}

//------------------------------------------------------------------------------
void PluginField::clearDoubleClickMessage()
{
    displayDoubleClickMessage = false;
    repaint();
}

//------------------------------------------------------------------------------
void PluginField::syncWithGraph()
{
    // Build a set of all NodeIDs in the graph
    std::set<uint32> graphNodeIds;
    for (int i = 0; i < signalPath->getNumFilters(); ++i)
    {
        auto node = signalPath->getNode(i);
        if (node != nullptr)
            graphNodeIds.insert(node->nodeID.uid);
    }

    // Helper lambda to safely get uid from PluginComponent
    // Uses the pins which store uid as a member (not a pointer that can dangle)
    auto getComponentUid = [](PluginComponent* comp) -> uint32
    {
        if (comp->getNumInputPins() > 0)
            return comp->getInputPin(0)->getUid();
        if (comp->getNumOutputPins() > 0)
            return comp->getOutputPin(0)->getUid();
        if (comp->getNumParamPins() > 0)
            return comp->getParamPin(0)->getUid();
        return 0;
    };

    // Find PluginComponents that no longer have a corresponding graph node
    std::vector<PluginComponent*> toRemove;
    for (int i = 0; i < getNumChildComponents(); ++i)
    {
        PluginComponent* comp = dynamic_cast<PluginComponent*>(getChildComponent(i));
        if (comp != nullptr)
        {
            uint32 uid = getComponentUid(comp);
            if (uid != 0 && graphNodeIds.find(uid) == graphNodeIds.end())
            {
                toRemove.push_back(comp);
            }
        }
    }

    // Remove orphan PluginComponents
    for (auto* comp : toRemove)
    {
        comp->removeChangeListener(this);
        removeChildComponent(comp);
        delete comp;
    }

    // Find graph nodes that don't have a PluginComponent
    std::set<uint32> uiNodeIds;
    for (int i = 0; i < getNumChildComponents(); ++i)
    {
        PluginComponent* comp = dynamic_cast<PluginComponent*>(getChildComponent(i));
        if (comp != nullptr)
        {
            uint32 uid = getComponentUid(comp);
            if (uid != 0)
                uiNodeIds.insert(uid);
        }
    }

    for (int i = 0; i < signalPath->getNumFilters(); ++i)
    {
        auto node = signalPath->getNode(i);
        if (node != nullptr && uiNodeIds.find(node->nodeID.uid) == uiNodeIds.end())
        {
            // Add missing PluginComponent
            addFilter(i, false);
        }
    }

    // Sync connections: remove UI connections not in graph, add graph connections not in UI
    auto graphConnections = signalPath->getConnections();

    // Build set of graph connections for fast lookup
    std::set<std::tuple<uint32, int, uint32, int>> graphConnSet;
    for (const auto& conn : graphConnections)
    {
        graphConnSet.insert(std::make_tuple(conn.source.nodeID.uid, conn.source.channelIndex,
                                            conn.destination.nodeID.uid, conn.destination.channelIndex));
    }

    // Build set of UI connections
    std::set<std::tuple<uint32, int, uint32, int>> uiConnSet;
    std::vector<PluginConnection*> connectionsToRemove;
    for (int i = 0; i < getNumChildComponents(); ++i)
    {
        PluginConnection* conn = dynamic_cast<PluginConnection*>(getChildComponent(i));
        if (conn != nullptr)
        {
            auto* src = conn->getSource();
            auto* dest = conn->getDestination();

            if (src != nullptr && dest != nullptr)
            {
                auto key = std::make_tuple(src->getUid(), src->getChannel(), dest->getUid(), dest->getChannel());
                uiConnSet.insert(key);

                // Check if this UI connection exists in graph
                if (graphConnSet.find(key) == graphConnSet.end())
                {
                    connectionsToRemove.push_back(conn);
                }
            }
            else
            {
                // Invalid connection, remove it
                connectionsToRemove.push_back(conn);
            }
        }
    }

    // Remove UI connections that aren't in graph
    for (auto* conn : connectionsToRemove)
    {
        removeChildComponent(conn);
        delete conn;
    }

    // Add graph connections that aren't in UI
    for (const auto& conn : graphConnections)
    {
        auto key = std::make_tuple(conn.source.nodeID.uid, conn.source.channelIndex, conn.destination.nodeID.uid,
                                   conn.destination.channelIndex);

        if (uiConnSet.find(key) == uiConnSet.end())
        {
            // Find the source and destination PluginComponents and their pins
            PluginComponent* sourceComp = nullptr;
            PluginComponent* destComp = nullptr;

            for (int i = 0; i < getNumChildComponents(); ++i)
            {
                PluginComponent* pc = dynamic_cast<PluginComponent*>(getChildComponent(i));
                if (pc != nullptr)
                {
                    if (pc->getNumOutputPins() > 0 && pc->getOutputPin(0)->getUid() == conn.source.nodeID.uid)
                        sourceComp = pc;
                    if (pc->getNumInputPins() > 0 && pc->getInputPin(0)->getUid() == conn.destination.nodeID.uid)
                        destComp = pc;
                }
            }

            if (sourceComp != nullptr && destComp != nullptr)
            {
                PluginPinComponent* sourcePin = nullptr;
                PluginPinComponent* destPin = nullptr;

                // Find the source pin by channel
                bool isMidiChannel = (conn.source.channelIndex == AudioProcessorGraph::midiChannelIndex);
                if (isMidiChannel)
                {
                    sourcePin = sourceComp->getParamPin(0);
                    destPin = destComp->getParamPin(0);
                }
                else
                {
                    if (conn.source.channelIndex < sourceComp->getNumOutputPins())
                        sourcePin = sourceComp->getOutputPin(conn.source.channelIndex);
                    if (conn.destination.channelIndex < destComp->getNumInputPins())
                        destPin = destComp->getInputPin(conn.destination.channelIndex);
                }

                if (sourcePin != nullptr && destPin != nullptr)
                {
                    addAndMakeVisible(new PluginConnection(sourcePin, destPin));
                }
            }
        }
    }

    repaint();
}

//------------------------------------------------------------------------------
void PluginField::clearMappings()
{
    multimap<uint32, Mapping*>::iterator it;

    for (it = mappings.begin(); it != mappings.end(); ++it)
        delete it->second;

    mappings.clear();
}

//------------------------------------------------------------------------------
void PluginField::handleOscBundle(OSC::Bundle* bundle)
{
    int i;

    for (i = 0; i < bundle->getNumBundles(); ++i)
        handleOscBundle(bundle->getBundle(i));

    for (i = 0; i < bundle->getNumMessages(); ++i)
        oscManager.messageReceived(bundle->getMessage(i));
    // handleOscMessage(bundle->getMessage(i));
}

//------------------------------------------------------------------------------
/*void PluginField::handleOscMessage(OSC::Message *message)
{
        oscManager.messageReceived(message);
}*/

//------------------------------------------------------------------------------
void PluginField::moveConnectionsBehind()
{
    int i;

    for (i = (getNumChildComponents() - 1); i >= 0; --i)
    {
        PluginConnection* connection = dynamic_cast<PluginConnection*>(getChildComponent(i));

        if (connection)
            connection->toBack();
        else
            getChildComponent(i)->toFront(false);
    }
}

//------------------------------------------------------------------------------
Component* PluginField::getPinAt(const int x, const int y)
{
    Point<int> pos(x, y);

    if (isVisible() && ((unsigned int)x) < (unsigned int)getWidth() && ((unsigned int)y) < (unsigned int)getHeight() &&
        hitTest(x, y))
    {
        for (int i = getNumChildComponents(); --i >= 0;)
        {
            Component* const child = getChildComponent(i);

            if (!dynamic_cast<PluginConnection*>(child))
            {
                Rectangle<int> updatedRect(child->getX() - 16, child->getY() - 16, child->getWidth() + 32,
                                           child->getHeight() + 32);

                if (updatedRect.contains(pos))
                {
                    if (pos.getX() < child->getX())
                        pos.addXY((child->getX() - pos.getX()), 0);
                    if (pos.getY() < child->getY())
                        pos.addXY(0, (child->getY() - pos.getY()));

                    if (pos.getX() > (child->getX() + child->getWidth()))
                        pos.addXY(-(pos.getX() - (child->getX() + child->getWidth())), 0);
                    if (pos.getY() > (child->getY() + child->getHeight()))
                        pos.addXY(0, -(pos.getY() - (child->getY() + child->getHeight())));

                    Component* const c = child->getComponentAt(pos.getX() - child->getX(), pos.getY() - child->getY());

                    if (c != 0)
                        return c;
                }
            }
        }

        return this;
    }

    return 0;
}

//------------------------------------------------------------------------------
void PluginField::connectAll(PluginConnection* connection)
{
    PluginComponent* source = dynamic_cast<PluginComponent*>(connection->getSource()->getParentComponent());
    PluginComponent* dest = dynamic_cast<PluginComponent*>(connection->getDestination()->getParentComponent());

    if (source && dest)
    {
        int left, right;
        int numSources = source->getNumOutputPins();
        int numDests = dest->getNumInputPins();
        PluginPinComponent* sourcePin;
        PluginPinComponent* destPin;

        for (left = 0; left < numSources; ++left)
        {
            if (source->getOutputPin(left) == connection->getSource())
            {
                ++left;
                break;
            }
        }
        for (right = 0; right < numDests; ++right)
        {
            if (dest->getInputPin(right) == connection->getDestination())
            {
                ++right;
                break;
            }
        }

        for (; (left < numSources) && (right < numDests); ++left, ++right)
        {
            sourcePin = source->getOutputPin(left);
            destPin = dest->getInputPin(right);
            signalPath->addConnection(AudioProcessorGraph::NodeID(sourcePin->getUid()), sourcePin->getChannel(),
                                      AudioProcessorGraph::NodeID(destPin->getUid()), destPin->getChannel());
            addAndMakeVisible(new PluginConnection(sourcePin, destPin));
        }
    }
}
