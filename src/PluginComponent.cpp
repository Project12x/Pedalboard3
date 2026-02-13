//	PluginComponent.cpp - Component representing a plugin/filter in the
//						  PluginField.
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

#include "PluginComponent.h"

#include "BypassableInstance.h"
#include "ColourScheme.h"
#include "IconManager.h"
#include "CrashProtection.h"
#include "DeviceMeterTap.h"
#include "FilterGraph.h"
#include "MasterGainState.h"
#include "SafetyLimiter.h"
#include "Images.h"
#include "JuceHelperStuff.h"
#include "MappingsDialog.h"
#include "PedalboardProcessors.h"
#include "PluginField.h"
#include "PresetBar.h"
#include "SettingsManager.h"
#include "SubGraphEditorComponent.h"
#include "Vectors.h"

#include <melatonin_blur/melatonin_blur.h>
#include <spdlog/spdlog.h>

using namespace std;

//------------------------------------------------------------------------------
// Helper functions to get channel counts and names.
// For BypassableInstance-wrapped plugins, uses cached data populated at
// construction time (before audio starts) to avoid racing the audio thread.
// For unwrapped processors (internal PedalboardProcessors), queries directly.
//------------------------------------------------------------------------------
namespace
{
int countInputChannelsFromBuses(AudioProcessor* proc)
{
    if (auto* bypassable = dynamic_cast<BypassableInstance*>(proc))
        return bypassable->getCachedInputChannelCount();

    // Internal processors - no BypassableInstance wrapper, safe to query directly
    int totalChannels = 0;
    for (int busIdx = 0; busIdx < proc->getBusCount(true); ++busIdx)
    {
        if (auto* bus = proc->getBus(true, busIdx))
            totalChannels += bus->getNumberOfChannels();
    }
    if (totalChannels == 0)
        totalChannels = proc->getTotalNumInputChannels();
    return totalChannels;
}

int countOutputChannelsFromBuses(AudioProcessor* proc)
{
    if (auto* bypassable = dynamic_cast<BypassableInstance*>(proc))
        return bypassable->getCachedOutputChannelCount();

    // Internal processors - no BypassableInstance wrapper, safe to query directly
    int totalChannels = 0;
    for (int busIdx = 0; busIdx < proc->getBusCount(false); ++busIdx)
    {
        if (auto* bus = proc->getBus(false, busIdx))
            totalChannels += bus->getNumberOfChannels();
    }
    if (totalChannels == 0)
        totalChannels = proc->getTotalNumOutputChannels();
    return totalChannels;
}

String getInputChannelNameSafe(AudioProcessor* proc, int index)
{
    if (auto* bypassable = dynamic_cast<BypassableInstance*>(proc))
        return bypassable->getCachedInputChannelName(index);
    return proc->getInputChannelName(index);
}

String getOutputChannelNameSafe(AudioProcessor* proc, int index)
{
    if (auto* bypassable = dynamic_cast<BypassableInstance*>(proc))
        return bypassable->getCachedOutputChannelName(index);
    return proc->getOutputChannelName(index);
}

bool acceptsMidiSafe(AudioProcessor* proc)
{
    if (auto* bypassable = dynamic_cast<BypassableInstance*>(proc))
        return bypassable->getCachedAcceptsMidi();
    return proc->acceptsMidi();
}

bool producesMidiSafe(AudioProcessor* proc)
{
    if (auto* bypassable = dynamic_cast<BypassableInstance*>(proc))
        return bypassable->getCachedProducesMidi();
    return proc->producesMidi();
}
} // namespace

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
class NiallsGenericEditor : public GenericAudioProcessorEditor
{
  public:
    ///	Constructor.
    NiallsGenericEditor(AudioProcessor* const owner)
        : GenericAudioProcessorEditor(*owner) {

          };

    ///	Fill the background the correct colour.
    void paint(Graphics& g) { g.fillAll(ColourScheme::getInstance().colours["Window Background"]); };
};

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
PluginComponent::PluginComponent(AudioProcessorGraph::Node* n)
    : Component(),
      // plugin(p),
      editButton(0), mappingsButton(0), bypassButton(0), deleteButton(0), node(n), pluginWindow(0), beingDragged(false),
      dragX(0), dragY(0)
{
    BypassableInstance* bypassable = dynamic_cast<BypassableInstance*>(node->getProcessor());
    PedalboardProcessor* proc = nullptr;

    // Try to get PedalboardProcessor from BypassableInstance wrapper (main canvas)
    if (bypassable)
        proc = dynamic_cast<PedalboardProcessor*>(bypassable->getPlugin());
    // Fall back to direct cast (SubGraph canvas, no wrapper)
    if (!proc)
        proc = dynamic_cast<PedalboardProcessor*>(node->getProcessor());

    pluginName = node->getProcessor()->getName();
    spdlog::debug("[PluginComponent] creating '{}'", pluginName.toStdString());

    determineSize();

    titleLabel = new Label("titleLabe", pluginName);
    titleLabel->setBounds(5, 0, getWidth() - 10, 20);
    titleLabel->setInterceptsMouseClicks(false, false);
    titleLabel->setFont(Font(14.0f, Font::bold));
    titleLabel->addListener(this);
    addAndMakeVisible(titleLabel);

    // Shift title label to make room for icon on Audio I/O nodes
    if ((pluginName == "Audio Input") || (pluginName == "Audio Output"))
        titleLabel->setBounds(18, 0, getWidth() - 23, 20);

    if ((pluginName != "Audio Input") && (pluginName != "Midi Input") && (pluginName != "Audio Output") &&
        (pluginName != "OSC Input"))
    {
        std::unique_ptr<Drawable> closeUp(
            JuceHelperStuff::loadSVGFromMemory(Vectors::closefilterbutton_svg, Vectors::closefilterbutton_svgSize));
        std::unique_ptr<Drawable> closeOver(JuceHelperStuff::loadSVGFromMemory(Vectors::closefilterbuttonover_svg,
                                                                               Vectors::closefilterbuttonover_svgSize));
        std::unique_ptr<Drawable> closeDown(JuceHelperStuff::loadSVGFromMemory(Vectors::closefilterbuttondown_svg,
                                                                               Vectors::closefilterbuttondown_svgSize));
        std::unique_ptr<Drawable> bypassOff(
            JuceHelperStuff::loadSVGFromMemory(Vectors::bypassbuttonoff_svg, Vectors::bypassbuttonoff_svgSize));
        std::unique_ptr<Drawable> bypassOn(
            JuceHelperStuff::loadSVGFromMemory(Vectors::bypassbuttonon_svg, Vectors::bypassbuttonon_svgSize));

        // So the audio I/O etc. don't get their titles squeezed by the
        // non-existent close button.
        titleLabel->setBounds(5, 0, getWidth() - 17, 20);

        // Skip edit/mappings buttons for Tuner (no external editor, no mappable params)
        if (pluginName != "Tuner")
        {
            editButton = new TextButton("e", "Open plugin editor (right-click for options)");
            editButton->setBounds(10, getHeight() - 30, 20, 20);
            editButton->addListener(this);
            // Add mouse listener for right-click context menu
            editButton->addMouseListener(this, false);
            addAndMakeVisible(editButton);

            mappingsButton = new TextButton("m", "Open mappings editor");
            mappingsButton->setBounds(32, getHeight() - 30, 24, 20);
            mappingsButton->addListener(this);
            addAndMakeVisible(mappingsButton);
        }

        bypassButton = new DrawableButton("BypassFilterButton", DrawableButton::ImageOnButtonBackground);
        bypassButton->setImages(bypassOff.get(), nullptr, nullptr, nullptr, bypassOn.get());
        bypassButton->setClickingTogglesState(true);
        bypassButton->setBounds(getWidth() - 30, getHeight() - 30, 20, 20);
        bypassButton->addListener(this);
        addAndMakeVisible(bypassButton);

        deleteButton = new DrawableButton("DeleteFilterButton", DrawableButton::ImageRaw);
        deleteButton->setImages(closeUp.get(), closeOver.get(), closeDown.get());
        deleteButton->setEdgeIndent(0);
        deleteButton->setBounds(getWidth() - 17, 5, 12, 12);
        deleteButton->addListener(this);
        addAndMakeVisible(deleteButton);
    }

    if (proc)
    {
        int tempint;
        Component* comp = proc->getControls();
        Point<int> compSize = proc->getSize();

        spdlog::debug("[PluginComponent] proc valid, getControls()={}, getSize()={}x{}", comp != nullptr,
                      compSize.getX(), compSize.getY());

        tempint = (getWidth() / 2) - (compSize.getX() / 2);
        comp->setTopLeftPosition(tempint, 24);
        comp->setSize(compSize.getX(), compSize.getY()); // Ensure size is set explicitly

        spdlog::debug("[PluginComponent] Control positioned: x={}, y=24, PluginComponent size={}x{}", tempint,
                      getWidth(), getHeight());

        addAndMakeVisible(comp);
    }

    createPins();

    // Create per-channel gain sliders for Audio I/O nodes (inline with pins)
    if (isAudioIONode())
    {
        AudioProcessor* plugin = node->getProcessor();
        bool isInput = (pluginName == "Audio Input");
        int numCh = isInput ? countOutputChannelsFromBuses(plugin)
                            : countInputChannelsFromBuses(plugin);
        auto& state = MasterGainState::getInstance();

        const float meterStartY = 32.0f;
        const float pinSpacing = 36.0f;
        const int sliderHeight = 18;
        const int pinMargin = 22;
        const int edgeMargin = 8;
        // Slider width matches VU meter width
        int sliderW = getWidth() - pinMargin - edgeMargin;

        for (int ch = 0; ch < numCh && ch < MasterGainState::MaxChannels; ++ch)
        {
            auto* slider = new Slider("channelGain_" + String(ch));
            slider->setSliderStyle(Slider::LinearBar);
            slider->setRange(-60.0, 12.0, 0.1);
            slider->setTextValueSuffix(" dB");
            slider->setDoubleClickReturnValue(true, 0.0);
            slider->setTooltip(String(isInput ? "Input" : "Output") + " Ch " + String(ch + 1) + " Gain");
            slider->addListener(this);

            // Position slider inline with its pin (below VU meter for this channel)
            int sliderY = (int)(meterStartY + ch * pinSpacing + 10.0f);
            int sliderX = isInput ? edgeMargin : pinMargin;
            slider->setBounds(sliderX, sliderY, sliderW, sliderHeight);

            // Sync initial value from MasterGainState per-channel
            float initDb = isInput
                ? state.inputChannelGainDb[ch].load(std::memory_order_relaxed)
                : state.outputChannelGainDb[ch].load(std::memory_order_relaxed);
            slider->setValue(initDb, dontSendNotification);

            addAndMakeVisible(slider);
            channelGainSliders.add(slider);
        }
    }

    if (node->properties.getWithDefault("windowOpen", false))
        buttonClicked(editButton);
}

//------------------------------------------------------------------------------
PluginComponent::~PluginComponent()
{
    channelGainSliders.clear(false); // Release without deleting - deleteAllChildren() handles it
    deleteAllChildren();
    if (pluginWindow)
        delete pluginWindow;
}

//------------------------------------------------------------------------------
void PluginComponent::paint(Graphics& g)
{
    int i;
    auto& colours = ColourScheme::getInstance().colours;
    float w = (float)getWidth();
    float h = (float)getHeight();
    const float cornerRadius = 8.0f;

    // === MAIN FILL (gradient for premium feel) ===
    Colour bgTop = colours["Plugin Background"].brighter(0.08f);
    Colour bgBottom = colours["Plugin Background"].darker(0.08f);
    g.setGradientFill(ColourGradient(bgTop, 0, 0, bgBottom, 0, h, false));
    g.fillRoundedRectangle(2.0f, 2.0f, w - 4.0f, h - 4.0f, cornerRadius);

    // === BORDER (thicker, more defined) ===
    g.setColour(colours["Plugin Border"]);
    g.drawRoundedRectangle(2.0f, 2.0f, w - 4.0f, h - 4.0f, cornerRadius, 2.0f);

    // === HEADER BAR (title area) ===
    // Use tinted header for Audio I/O nodes
    if (isAudioIONode())
    {
        Colour accentColour = colours["Audio Connection"].withAlpha(0.6f);
        g.setColour(accentColour.interpolatedWith(colours["Plugin Border"], 0.3f));
    }
    else
    {
        g.setColour(colours["Plugin Border"]);
    }
    g.fillRoundedRectangle(2.0f, 2.0f, w - 4.0f, 18.0f, cornerRadius);
    g.fillRect(2.0f, 14.0f, w - 4.0f, 6.0f); // Square off bottom of header

    // === ICON for Audio I/O nodes ===
    if (isAudioIONode())
    {
        const float iconSize = 12.0f;
        const float iconX = 5.0f;
        const float iconY = 3.0f;

        auto& iconManager = IconManager::getInstance();
        std::unique_ptr<Drawable> icon;

        if (pluginName == "Audio Input")
            icon = iconManager.getMicIcon(colours["Text Colour"]);
        else
            icon = iconManager.getSpeakerIcon(colours["Text Colour"]);

        if (icon)
            icon->drawWithin(g, Rectangle<float>(iconX, iconY, iconSize, iconSize),
                             RectanglePlacement::centred, 1.0f);

        // Draw device name subtitle
        if (auto* tap = DeviceMeterTap::getInstance())
        {
            String deviceName = tap->getDeviceName();
            if (deviceName.isNotEmpty())
            {
                g.setColour(colours["Text Colour"].withAlpha(0.6f));
                g.setFont(Font(9.0f));
                g.drawText(deviceName, 4.0f, 18.0f, w - 8.0f, 12.0f, Justification::centred, true);
            }
        }
    }

    // Draw the plugin name.
    g.setColour(colours["Text Colour"]);

    // Draw the input channels.
    for (i = 0; i < inputText.size(); ++i)
        inputText[i]->draw(g);

    // Draw the output channels.
    for (i = 0; i < outputText.size(); ++i)
        outputText[i]->draw(g);

    // Draw horizontal VU meters for Audio I/O nodes (full width)
    if (isAudioIONode() && cachedMeterChannelCount > 0)
    {
        const float pinMargin = 22.0f;    // Space for pin on one side
        const float edgeMargin = 8.0f;    // Space on the other side
        const float meterWidth = w - pinMargin - edgeMargin;  // Full width minus margins
        const float meterHeight = 6.0f;   // Compact height per channel
        const float meterStartY = 32.0f;  // Below device name subtitle
        const float pinSpacing = 36.0f;   // Match Audio I/O pin spacing

        for (int ch = 0; ch < cachedMeterChannelCount && ch < 16; ++ch)
        {
            float level = cachedMeterLevels[ch];

            // Convert linear to normalized dB scale (-60 to 0 dB)
            float levelDb = (level > 0.001f) ? 20.0f * std::log10(level) : -60.0f;
            float normalizedLevel = jlimit(0.0f, 1.0f, (levelDb + 60.0f) / 60.0f);

            // Position meter - full width horizontal bars
            float x, y;
            if (pluginName == "Audio Input")
            {
                // Output pins are on the right, meter starts from left edge
                x = edgeMargin;
                y = meterStartY + ch * pinSpacing;
            }
            else // Audio Output
            {
                // Input pins are on the left, meter starts after pin
                x = pinMargin;
                y = meterStartY + ch * pinSpacing;
            }

            // Meter background (dark track)
            g.setColour(colours["Plugin Background"].darker(0.5f));
            g.fillRoundedRectangle(x, y, meterWidth, meterHeight, 2.0f);

            // Meter bar with gradient color based on level (horizontal fill)
            if (normalizedLevel > 0.0f)
            {
                float barWidth = meterWidth * normalizedLevel;
                Colour barColour;
                if (level >= 1.0f)
                    barColour = colours["VU Meter Over Colour"];
                else if (normalizedLevel > 0.75f)
                    barColour = colours["VU Meter Upper Colour"];
                else
                    barColour = colours["VU Meter Lower Colour"];

                g.setColour(barColour);
                g.fillRoundedRectangle(x, y, barWidth, meterHeight, 2.0f);
            }

            // Subtle border for definition
            g.setColour(colours["Plugin Border"].withAlpha(0.3f));
            g.drawRoundedRectangle(x, y, meterWidth, meterHeight, 2.0f, 0.5f);
        }
    }
}

//------------------------------------------------------------------------------
void PluginComponent::moved()
{
    sendChangeMessage();
}

//------------------------------------------------------------------------------
void PluginComponent::timerUpdate()
{
    BypassableInstance* bypassable = dynamic_cast<BypassableInstance*>(node->getProcessor());

    if (bypassable)
        bypassButton->setToggleState(bypassable->getBypass(), false);

    // Update meter levels for Audio I/O nodes
    if (isAudioIONode())
    {
        bool needsRepaint = false;
        int numChannels = 0;

        if (auto* limiter = SafetyLimiterProcessor::getInstance())
        {
            numChannels = 2;
            for (int ch = 0; ch < numChannels; ++ch)
            {
                float level = (pluginName == "Audio Input")
                    ? limiter->getInputLevel(ch)
                    : limiter->getOutputLevel(ch);
                if (std::abs(level - cachedMeterLevels[ch]) > 0.001f)
                {
                    cachedMeterLevels[ch] = level;
                    needsRepaint = true;
                }
            }
        }

        cachedMeterChannelCount = numChannels;
        if (needsRepaint)
            repaint();

        // Sync per-channel gain sliders from MasterGainState (when not being dragged)
        if (channelGainSliders.size() > 0)
        {
            bool isInput = (pluginName == "Audio Input");
            auto& state = MasterGainState::getInstance();
            for (int ch = 0; ch < channelGainSliders.size(); ++ch)
            {
                auto* slider = channelGainSliders[ch];
                if (slider != nullptr && !slider->isMouseButtonDown())
                {
                    float currentDb = isInput
                        ? state.inputChannelGainDb[ch].load(std::memory_order_relaxed)
                        : state.outputChannelGainDb[ch].load(std::memory_order_relaxed);

                    if (std::abs((float)slider->getValue() - currentDb) > 0.01f)
                        slider->setValue(currentDb, dontSendNotification);
                }
            }
        }
    }
}

//------------------------------------------------------------------------------
void PluginComponent::mouseDown(const MouseEvent& e)
{
    // Handle right-click on the edit button to show context menu
    if (e.originalComponent == editButton && e.mods.isPopupMenu() && !pluginWindow)
    {
        PopupMenu menu;
        menu.addItem(1, "Open Custom Editor", node->getProcessor()->hasEditor());
        menu.addItem(2, "Open Generic Editor");

        menu.showMenuAsync(PopupMenu::Options().withTargetComponent(editButton),
                           [this](int result)
                           {
                               if (result == 1)
                                   openPluginEditor(false); // Custom editor
                               else if (result == 2)
                                   openPluginEditor(true); // Generic editor
                           });
        return;
    }

    // Ignore all other events from the edit button - let the Button::Listener handle left-clicks
    if (e.originalComponent == editButton)
        return;

    // Title bar drag logic (only for events on PluginComponent itself)
    if (e.y < 21)
    {
        if (e.getNumberOfClicks() == 2)
            titleLabel->showEditor();
        else
        {
            beginDragAutoRepeat(30);
            beingDragged = true;
            dragX = e.getPosition().getX();
            dragY = e.getPosition().getY();
            toFront(true);
        }
    }
}

//------------------------------------------------------------------------------
void PluginComponent::mouseDrag(const MouseEvent& e)
{
    if (beingDragged)
    {
        MouseEvent eField = e.getEventRelativeTo(getParentComponent());

        // parent = PluginField => parent = Viewport's contentHolder => parent =
        // Viewport.
        Viewport* viewport = dynamic_cast<Viewport*>(getParentComponent()->getParentComponent()->getParentComponent());

        if (viewport)
        {
            MouseEvent tempEv = e.getEventRelativeTo(viewport);

            viewport->autoScroll(tempEv.x, tempEv.y, 20, 4);
        }

        int newX = eField.x - dragX;
        int newY = eField.y - dragY;

        // Snap to grid if enabled
        if (SettingsManager::getInstance().getBool("SnapToGrid", false))
        {
            constexpr int gridSize = 20;
            newX = (newX / gridSize) * gridSize;
            newY = (newY / gridSize) * gridSize;
        }

        setTopLeftPosition(newX, newY);
        if (getX() < 0)
            setTopLeftPosition(0, getY());
        if (getY() < 0)
            setTopLeftPosition(getX(), 0);
        node->properties.set("x", getX());
        node->properties.set("y", getY());
        sendChangeMessage();
    }
}

//------------------------------------------------------------------------------
void PluginComponent::mouseUp(const MouseEvent& e)
{
    beingDragged = false;

    // Final snap on mouse up (in case drag didn't snap perfectly)
    if (SettingsManager::getInstance().getBool("SnapToGrid", false))
    {
        constexpr int gridSize = 20;
        int snappedX = (getX() / gridSize) * gridSize;
        int snappedY = (getY() / gridSize) * gridSize;
        setTopLeftPosition(snappedX, snappedY);
        node->properties.set("x", snappedX);
        node->properties.set("y", snappedY);
    }

    if (pluginWindow)
        node->properties.set("windowOpen", false);
}

//------------------------------------------------------------------------------
void PluginComponent::openPluginEditor(bool forceGeneric)
{
    if (pluginWindow)
        return; // Already open

    if (!node || !node->getProcessor())
    {
        spdlog::error("[PluginComponent::openPluginEditor] node or processor is null");
        return;
    }

    AudioProcessorEditor* editor = nullptr;
    juce::String pluginName = node->getProcessor()->getName();

    spdlog::debug("[PluginComponent::openPluginEditor] Opening editor for: {}, forceGeneric={}",
                  pluginName.toStdString(), forceGeneric);

    // Try custom editor unless user explicitly requested generic
    if (!forceGeneric && node->getProcessor()->hasEditor())
    {
        // Wrap in crash protection to catch SEH exceptions from misbehaving plugins
        bool editorCreated = CrashProtection::getInstance().executeWithProtection(
            [&]() { editor = node->getProcessor()->createEditor(); }, "createEditor", pluginName);

        if (!editorCreated)
        {
            spdlog::error("[PluginComponent::openPluginEditor] createEditor() failed with exception for: {}",
                          pluginName.toStdString());
            return;
        }
    }

    // Use generic editor if: forced, custom failed, or plugin has no editor
    if (!editor)
    {
        spdlog::debug("[PluginComponent::openPluginEditor] Creating NiallsGenericEditor");
        editor = new NiallsGenericEditor(node->getProcessor());
    }

    if (editor)
    {
        spdlog::debug("[PluginComponent::openPluginEditor] Creating PluginEditorWindow");
        editor->setName(pluginName);
        pluginWindow = new PluginEditorWindow(editor, this);
        node->properties.set("windowOpen", true);
        spdlog::debug("[PluginComponent::openPluginEditor] Editor window created");
    }
}

//------------------------------------------------------------------------------
void PluginComponent::buttonClicked(Button* button)
{
    std::cerr << "[buttonClicked] Enter for button\n" << std::flush;

    // Safety: Verify button pointers are valid before comparing
    if (!button)
    {
        std::cerr << "[buttonClicked] ERROR: button is null!\n" << std::flush;
        return;
    }
    if (!node || !node->getProcessor())
    {
        std::cerr << "[buttonClicked] ERROR: node or processor is null!\n" << std::flush;
        return;
    }

    std::cerr << "[buttonClicked] Button addr=" << button << ", edit=" << editButton << ", delete=" << deleteButton
              << ", bypass=" << bypassButton << ", mappings=" << mappingsButton << ", pluginWindow=" << pluginWindow
              << "\n"
              << std::flush;

    if ((button == editButton) && !pluginWindow)
    {
        std::cerr << "[buttonClicked] Edit button - opening custom editor\n" << std::flush;
        openPluginEditor(false); // Default to custom editor on left-click
        std::cerr << "[buttonClicked] Complete\n" << std::flush;
    }

    else if (button == mappingsButton)
    {
        std::cerr << "[buttonClicked] MAPPINGS button clicked\n" << std::flush;
        openMappingsWindow();
    }
    else if (button == bypassButton)
    {
        std::cerr << "[buttonClicked] BYPASS button clicked\n" << std::flush;
        BypassableInstance* bypassable = dynamic_cast<BypassableInstance*>(node->getProcessor());
        std::cerr << "[buttonClicked] Bypassable cast=" << bypassable
                  << ", toggleState=" << bypassButton->getToggleState() << "\n"
                  << std::flush;

        if (bypassable)
        {
            bypassable->setBypass(bypassButton->getToggleState());
            std::cerr << "[buttonClicked] Bypass set to " << bypassButton->getToggleState() << "\n" << std::flush;
        }
    }
    else if (button == deleteButton)
    {
        std::cerr << "[buttonClicked] DELETE button clicked\n" << std::flush;
        PluginField* parent = dynamic_cast<PluginField*>(getParentComponent());
        std::cerr << "[buttonClicked] parent PluginField=" << parent << "\n" << std::flush;

        if (pluginWindow)
        {
            std::cerr << "[buttonClicked] Closing pluginWindow\n" << std::flush;
            pluginWindow->closeButtonPressed();
            std::cerr << "[buttonClicked] pluginWindow closed\n" << std::flush;
        }

        if (parent)
        {
            std::cerr << "[buttonClicked] Calling parent->deleteFilter()\n" << std::flush;
            parent->deleteFilter(node);
            std::cerr << "[buttonClicked] parent->deleteFilter() done\n" << std::flush;
            // PluginField doesn't own us via OwnedArray, so we need to delete ourselves
            std::cerr << "[buttonClicked] About to delete this (PluginComponent)\n" << std::flush;
            delete this;
        }
        else if (auto* canvas = dynamic_cast<SubGraphCanvas*>(getParentComponent()))
        {
            std::cerr << "[buttonClicked] SubGraphCanvas found, calling deleteFilter()\n" << std::flush;
            canvas->deleteFilter(node);
            // SubGraphCanvas::deleteFilter() already deleted 'this' via OwnedArray.remove()
            // DO NOT call delete this here - it would be a double-delete!
            std::cerr << "[buttonClicked] SubGraphCanvas::deleteFilter() done, returning (already deleted)\n"
                      << std::flush;
            return; // 'this' is already deleted, return immediately
        }
        else
        {
            std::cerr << "[buttonClicked] ERROR: No parent found to delete from!\n" << std::flush;
        }
    }
}

//------------------------------------------------------------------------------
void PluginComponent::labelTextChanged(Label* label)
{
    int i, y;
    PluginField* parent = dynamic_cast<PluginField*>(getParentComponent());

    pluginName = label->getText();

    // Update processor name in main canvas (SubGraphCanvas doesn't track names)
    if (parent)
        parent->updateProcessorName(node->nodeID.uid, pluginName);

    // Reset the Component's size/layout.
    determineSize(true);
    titleLabel->setBounds(5, 0, getWidth() - 17, 20);
    if (deleteButton)
        deleteButton->setBounds(getWidth() - 17, 5, 12, 12);
    if (bypassButton)
        bypassButton->setBounds(getWidth() - 30, getHeight() - 30, 20, 20);

    y = 25;
    for (i = 0; i < outputPins.size(); ++i)
    {
        Point<int> pinPos;

        pinPos.setXY(getWidth() - 6, y);
        outputPins[i]->setTopLeftPosition(pinPos.getX(), pinPos.getY());

        y += 18;
    }

    for (i = 0; i < paramPins.size(); ++i)
    {
        Point<int> pinPos;

        pinPos.setXY(getWidth() - 6, y);
        if (paramPins[i]->getX() > 0)
        {
            paramPins[i]->setTopLeftPosition(pinPos.getX(), pinPos.getY());

            y += 18;
        }
    }
}

//------------------------------------------------------------------------------
void PluginComponent::sliderValueChanged(Slider* slider)
{
    // Find which channel this slider controls
    int chIndex = channelGainSliders.indexOf(slider);
    if (chIndex >= 0)
    {
        auto& state = MasterGainState::getInstance();
        float val = (float)slider->getValue();

        if (pluginName == "Audio Input")
            state.inputChannelGainDb[chIndex].store(val, std::memory_order_relaxed);
        else
            state.outputChannelGainDb[chIndex].store(val, std::memory_order_relaxed);
    }
}

//------------------------------------------------------------------------------
void PluginComponent::setUserName(const String& val)
{
    titleLabel->setText(val, sendNotification);
}

//------------------------------------------------------------------------------
void PluginComponent::setWindow(PluginEditorWindow* val)
{
    pluginWindow = val;
    if (node)
    {
        if (pluginWindow)
            node->properties.set("windowOpen", true);
        else
            node->properties.set("windowOpen", false);
    }
}

//------------------------------------------------------------------------------
void PluginComponent::saveWindowState()
{
    if (pluginWindow)
    {
        node->properties.set("uiLastX", pluginWindow->getX());
        node->properties.set("uiLastY", pluginWindow->getY());
        node->properties.set("windowOpen", true);
    }
    else
        node->properties.set("windowOpen", false);
}

//------------------------------------------------------------------------------
void PluginComponent::openMappingsWindow()
{
    PluginField* parent = dynamic_cast<PluginField*>(getParentComponent());

    // Mappings are only supported in main PluginField canvas
    if (!parent)
        return;

    String tempstr;
    MappingsDialog dlg(parent->getMidiManager(), parent->getOscManager(), node,
                       /// @note JUCE 8: NodeID is now a struct, use .uid for integer value
                       parent->getMappingsForPlugin(node->nodeID.uid), parent);

    tempstr << node->getProcessor()->getName() << " Mappings";
    JuceHelperStuff::showModalDialog(tempstr, &dlg, getParentComponent(), Colour(0xFFEEECE1), false, true);
}

//------------------------------------------------------------------------------
void PluginComponent::cacheCurrentPreset()
{
    MemoryBlock* preset = new MemoryBlock();

    node->getProcessor()->getCurrentProgramStateInformation(*preset);

    cachedPresets.insert(make_pair(node->getProcessor()->getCurrentProgram(), shared_ptr<MemoryBlock>(preset)));
}

//------------------------------------------------------------------------------
void PluginComponent::getCachedPreset(int index, MemoryBlock& memBlock)
{
    map<int, shared_ptr<MemoryBlock>>::iterator it;

    it = cachedPresets.find(index);

    // Make sure the cached preset actually exists.
    if (it != cachedPresets.end())
    {
        it->second->swapWith(memBlock);
        cachedPresets.erase(it);
    }
}

//------------------------------------------------------------------------------
void PluginComponent::determineSize(bool onlyUpdateWidth)
{
    int i;
    Rectangle<float> bounds;
    float nameWidth;
    float inputWidth = 0.0f;
    float outputWidth = 0.0f;
    int w = 150;
    int h = 100;
    float x;
    float y = 15.0f;
    int numInputPins = 0;
    int numOutputPins = 0;
    PedalboardProcessor* proc = nullptr;
    Font tempFont(14.0f, Font::bold);
    AudioProcessor* plugin = node->getProcessor();
    BypassableInstance* bypassable = dynamic_cast<BypassableInstance*>(plugin);
    bool ignorePinNames = SettingsManager::getInstance().getBool("IgnorePinNames", false);

    // Try to get PedalboardProcessor from BypassableInstance wrapper (main canvas)
    if (bypassable)
        proc = dynamic_cast<PedalboardProcessor*>(bypassable->getPlugin());
    // Fall back to direct cast (SubGraph canvas, no wrapper)
    if (!proc)
        proc = dynamic_cast<PedalboardProcessor*>(plugin);

    nameText.clear();

    // Determine plugin name bounds.
    nameText.addLineOfText(tempFont, pluginName, 10.0f, y);
    // nameText.getBoundingBox(0, -1, l, t, r, b, true);
    bounds = nameText.getBoundingBox(0, -1, true);
    nameWidth = bounds.getWidth();

    // Add on space for the close button if necessary.
    if ((pluginName != "Audio Input") && (pluginName != "Midi Input") && (pluginName != "Audio Output") &&
        (pluginName != "OSC Input"))
    {
        nameWidth += 20.0f;
    }
    else
        nameWidth += 4.0f;

    inputText.clear();
    outputText.clear();

    bool showLabels = (!proc) || (pluginName == "Splitter") || (pluginName == "Mixer");

    // Use larger spacing for Audio I/O nodes (36px for VU + slider per channel)
    const float pinSpacing = isAudioIONode() ? 36.0f : 18.0f;

    if (showLabels)
    {
        int numIn = countInputChannelsFromBuses(plugin);
        int numOut = countOutputChannelsFromBuses(plugin);
        // Determine plugin input channel name bounds.
        y = 35.0f;
        tempFont.setHeight(12.0f);
        tempFont.setStyleFlags(Font::plain);
        for (i = 0; i < numIn; ++i)
        {
            // Use numbered names for Audio Output (its inputs are device output channels)
            bool useNumberedNames = ignorePinNames || (pluginName == "Audio Output");

            if (!useNumberedNames)
            {
                GlyphArrangement* g = new GlyphArrangement;

                g->addLineOfText(tempFont, getInputChannelNameSafe(plugin, i), 10.0f, y);
                bounds = g->getBoundingBox(0, -1, true);

                inputText.add(g);

                if (bounds.getWidth() > inputWidth)
                    inputWidth = bounds.getWidth();
            }
            else
            {
                String tempstr;
                GlyphArrangement* g = new GlyphArrangement;

                // For Audio Output, just show channel number
                if (pluginName == "Audio Output")
                    tempstr << i + 1;
                else
                    tempstr << "Input " << i + 1;
                g->addLineOfText(tempFont, tempstr, 10.0f, y);
                bounds = g->getBoundingBox(0, -1, true);

                inputText.add(g);

                if (bounds.getWidth() > inputWidth)
                    inputWidth = bounds.getWidth();
            }

            y += pinSpacing;
            ++numInputPins;
        }

        // Add input parameter/midi name.
        if ((acceptsMidiSafe(plugin) || (numIn > 0) ||
             (numOut > 0)) &&
            ((pluginName != "Audio Input") && (pluginName != "Audio Output")))
        {
            // if(!ignorePinNames)
            {
                GlyphArrangement* g = new GlyphArrangement;

                g->addLineOfText(tempFont, "param", 10.0f, y);
                bounds = g->getBoundingBox(0, -1, true);

                inputText.add(g);

                if (bounds.getWidth() > inputWidth)
                    inputWidth = bounds.getWidth();
            }

            y += 18.0f;
            ++numInputPins;
        }

        // Determine plugin output channel name bounds.
        y = 35.0f;
        for (i = 0; i < numOut; ++i)
        {
            // Use numbered names for Audio Input (its outputs are device input channels)
            bool useNumberedNames = ignorePinNames || (pluginName == "Audio Input");

            if (!useNumberedNames)
            {
                GlyphArrangement* g = new GlyphArrangement;

                g->addLineOfText(tempFont, getOutputChannelNameSafe(plugin, i),
                                 0.0f, //(inputWidth + 20.0f),
                                 y);
                bounds = g->getBoundingBox(0, -1, true);

                outputText.add(g);

                if (bounds.getWidth() > outputWidth)
                    outputWidth = bounds.getWidth();
            }
            else
            {
                String tempstr;
                GlyphArrangement* g = new GlyphArrangement;

                // For Audio Input, just show channel number
                if (pluginName == "Audio Input")
                    tempstr << i + 1;
                else
                    tempstr << "Output " << i + 1;
                g->addLineOfText(tempFont, tempstr,
                                 0.0f, //(inputWidth + 20.0f),
                                 y);
                bounds = g->getBoundingBox(0, -1, true);

                outputText.add(g);

                if (bounds.getWidth() > outputWidth)
                    outputWidth = bounds.getWidth();
            }

            y += pinSpacing;
            ++numOutputPins;
        }

        // Add output parameter/midi name.
        if (producesMidiSafe(plugin) || (plugin->getName() == "OSC Input"))
        {
            // if(!ignorePinNames)
            {
                GlyphArrangement* g = new GlyphArrangement;

                g->addLineOfText(tempFont, "param",
                                 0.0f, //(inputWidth + 20.0f),
                                 y);
                bounds = g->getBoundingBox(0, -1, true);

                outputText.add(g);

                if (bounds.getWidth() > outputWidth)
                    outputWidth = bounds.getWidth();
            }

            y += 18.0f;
            ++numOutputPins;
        }

        float contentW = inputWidth + outputWidth + 30.0f;
        float procW = 0.0f;
        float procH = 0.0f;

        if (proc)
        {
            Point<int> compSize = proc->getSize();
            // Ensure inputs and outputs fit on sides of the control
            procW = inputWidth + compSize.getX() + outputWidth + 20.0f;
            procH = (float)compSize.getY();

            // Minimal check
            if (procW < compSize.getX() + 24.0f)
                procW = compSize.getX() + 24.0f;
        }

        if (nameWidth > jmax(contentW, procW))
            w = (int)(nameWidth + 12.0f);
        else
            w = (int)jmax(contentW, procW);

        // Enforce consistent minimum width for Audio I/O nodes (VU meters + gain sliders)
        if (isAudioIONode())
            w = jmax(w, 140);

        // Shift output texts to where they should be.
        {
            x = (w - outputWidth - 10.0f);

            for (i = 0; i < outputText.size(); ++i)
                outputText[i]->moveRangeOfGlyphs(0, -1, x, 0.0f);
        }

        h = jmax(numInputPins, numOutputPins);
        h *= (int)pinSpacing;

        float minH = (float)h + 60.0f;
        if (proc && minH < procH + 52.0f)
            minH = procH + 52.0f;

        if ((pluginName != "Audio Input") && (pluginName != "Midi Input") && (pluginName != "Audio Output") &&
            (pluginName != "OSC Input"))
        {
            h = (int)minH;
        }
        else if (proc) // Wait, Audio Input doesn't have proc usually?
            h = (int)minH;

        // Original logic for Input/Output nodes used + 60 in the block, so that's preserved.
        // Wait, original: if (pluginName != ...) h += 60; else h += 34; (lines 596-602)
        // I should preserve that.

        if ((pluginName != "Audio Input") && (pluginName != "Midi Input") && (pluginName != "Audio Output") &&
            (pluginName != "OSC Input"))
        {
            // h is pin height * 13
            h = jmax((int)minH, h + 60);
        }
        else
        {
            h = jmax((int)minH, h + 34);
        }
    }
    else
    {
        Point<int> compSize = proc->getSize();

        if (nameWidth > (compSize.getX() + 24.0f))
            w = (int)(nameWidth + 20.0f);
        else
            w = (int)(compSize.getX() + 24.0f);

        h = compSize.getY() + 52;
    }

    if (onlyUpdateWidth)
        setSize(w, getHeight());
    else
        setSize(w, h);
}

//------------------------------------------------------------------------------
bool PluginComponent::isAudioIONode() const
{
    return (pluginName == "Audio Input") || (pluginName == "Audio Output");
}

//------------------------------------------------------------------------------
void PluginComponent::refreshPins()
{
    // Remove all existing pins
    for (auto* pin : inputPins)
        removeChildComponent(pin);
    for (auto* pin : outputPins)
        removeChildComponent(pin);
    for (auto* pin : paramPins)
        removeChildComponent(pin);

    inputPins.clear();
    outputPins.clear();
    paramPins.clear();

    // Recalculate size and recreate pins
    determineSize();
    createPins();
    repaint();
}

//------------------------------------------------------------------------------
void PluginComponent::createPins()
{
    int i;
    int y;
    PluginPinComponent* pin;
    AudioProcessor* plugin = node->getProcessor();
    /// @note JUCE 8: NodeID is now a struct, use .uid for integer value
    const uint32 uid = node->nodeID.uid;

    // Use larger pins and spacing for Audio I/O nodes
    const bool largePin = isAudioIONode();
    const int pinSpacing = largePin ? 36 : 18;
    const int pinXOffset = largePin ? -10 : -8;
    const int pinXOffsetRight = largePin ? (getWidth() - 8) : (getWidth() - 6);

    y = 25;
    for (i = 0; i < countInputChannelsFromBuses(plugin); ++i)
    {
        Point<int> pinPos;

        pin = new PluginPinComponent(false, uid, i, false, largePin);
        pinPos.setXY(pinXOffset, y);
        pin->setTopLeftPosition(pinPos.getX(), pinPos.getY());
        addAndMakeVisible(pin);

        inputPins.add(pin);

        y += pinSpacing;
    }

    if ((acceptsMidiSafe(plugin) || (countInputChannelsFromBuses(plugin) > 0) ||
         (countOutputChannelsFromBuses(plugin) > 0)) &&
        ((pluginName != "Audio Input") && (pluginName != "Audio Output")))
    {
        Point<int> pinPos;

        pin = new PluginPinComponent(false, uid, AudioProcessorGraph::midiChannelIndex, true);
        pinPos.setXY(-8, y);
        pin->setTopLeftPosition(pinPos.getX(), pinPos.getY());
        addAndMakeVisible(pin);

        paramPins.add(pin);

        y += 18;
    }

    y = 25;
    for (i = 0; i < countOutputChannelsFromBuses(plugin); ++i)
    {
        Point<int> pinPos;

        pin = new PluginPinComponent(true, uid, i, false, largePin);
        pinPos.setXY(pinXOffsetRight, y);
        pin->setTopLeftPosition(pinPos.getX(), pinPos.getY());
        addAndMakeVisible(pin);

        outputPins.add(pin);

        y += pinSpacing;
    }

    if (producesMidiSafe(plugin) || (plugin->getName() == "OSC Input"))
    {
        Point<int> pinPos;

        pin = new PluginPinComponent(true, uid, AudioProcessorGraph::midiChannelIndex, true);
        pinPos.setXY(getWidth() - 6, y);
        pin->setTopLeftPosition(pinPos.getX(), pinPos.getY());
        addAndMakeVisible(pin);

        paramPins.add(pin);

        y += 18;
    }
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
PluginPinComponent::PluginPinComponent(bool dir, uint32 id, int chan, bool param, bool large)
    : Component(), direction(dir), uid(id), channel(chan), parameterPin(param), largePin(large)
{
    if (largePin)
        setSize(18, 20); // Larger pins for Audio I/O nodes
    else
        setSize(14, 16);
}

//------------------------------------------------------------------------------
PluginPinComponent::~PluginPinComponent() {}

//------------------------------------------------------------------------------
void PluginPinComponent::paint(Graphics& g)
{
    const float w = (float)getWidth() - 2;
    const float h = (float)getHeight() - 2;
    const float cx = 1.0f + w * 0.5f;
    const float cy = 1.0f + h * 0.5f;
    const float radius = jmin(w, h) * 0.5f;

    // Get base color
    Colour baseColour = parameterPin ? ColourScheme::getInstance().colours["Parameter Connection"]
                                     : ColourScheme::getInstance().colours["Audio Connection"];

    // === 3D Gradient sphere effect ===
    ColourGradient sphereGrad(baseColour.brighter(0.4f), cx - radius * 0.3f, cy - radius * 0.3f,
                              baseColour.darker(0.3f), cx + radius * 0.5f, cy + radius * 0.5f, true);
    g.setGradientFill(sphereGrad);
    g.fillEllipse(1, 1, w, h);

    // === Highlight for gloss effect ===
    g.setColour(Colours::white.withAlpha(0.25f));
    g.fillEllipse(cx - radius * 0.5f, cy - radius * 0.6f, radius * 0.6f, radius * 0.4f);

    // === Border ===
    g.setColour(baseColour.darker(0.5f));
    g.drawEllipse(1, 1, w, h, 1.5f);

    // === Direction indicator (chevron) ===
    Path chevron;
    const float chevronSize = radius * 0.5f;
    g.setColour(Colours::white.withAlpha(0.8f));

    if (direction) // Output pin - chevron points right (→)
    {
        chevron.startNewSubPath(cx - chevronSize * 0.3f, cy - chevronSize * 0.6f);
        chevron.lineTo(cx + chevronSize * 0.5f, cy);
        chevron.lineTo(cx - chevronSize * 0.3f, cy + chevronSize * 0.6f);
    }
    else // Input pin - chevron points left (←)
    {
        chevron.startNewSubPath(cx + chevronSize * 0.3f, cy - chevronSize * 0.6f);
        chevron.lineTo(cx - chevronSize * 0.5f, cy);
        chevron.lineTo(cx + chevronSize * 0.3f, cy + chevronSize * 0.6f);
    }

    g.strokePath(chevron, PathStrokeType(1.5f, PathStrokeType::mitered, PathStrokeType::rounded));
}

//------------------------------------------------------------------------------
void PluginPinComponent::mouseDown(const MouseEvent& e)
{
    // Allow dragging from both input and output pins (bidirectional)
    PluginField* field = findParentComponentOfClass<PluginField>();

    if (field)
    {
        field->addConnection(this, (e.mods.isShiftDown() && !parameterPin));
    }
    else if (auto* canvas = findParentComponentOfClass<SubGraphCanvas>())
    {
        canvas->addConnection(this, (e.mods.isShiftDown() && !parameterPin));
    }
}

//------------------------------------------------------------------------------
void PluginPinComponent::mouseDrag(const MouseEvent& e)
{
    PluginField* field = findParentComponentOfClass<PluginField>();

    if (field)
    {
        MouseEvent e2 = e.getEventRelativeTo(field);
        field->dragConnection(e2.x - 5, e2.y);
    }
    else if (auto* canvas = findParentComponentOfClass<SubGraphCanvas>())
    {
        MouseEvent e2 = e.getEventRelativeTo(canvas);
        canvas->dragConnection(e2.x - 5, e2.y);
    }
}

//------------------------------------------------------------------------------
void PluginPinComponent::mouseUp(const MouseEvent& e)
{
    if (e.mods.testFlags(ModifierKeys::leftButtonModifier))
    {
        PluginField* field = findParentComponentOfClass<PluginField>();

        if (field)
        {
            MouseEvent e2 = e.getEventRelativeTo(field);
            field->releaseConnection(e2.x, e2.y);
        }
        else if (auto* canvas = findParentComponentOfClass<SubGraphCanvas>())
        {
            MouseEvent e2 = e.getEventRelativeTo(canvas);
            canvas->releaseConnection(e2.x, e2.y);
        }
    }
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
PluginEditorWindow::PluginEditorWindow(AudioProcessorEditor* editor, PluginComponent* c)
    : DocumentWindow(c->getUserName(), ColourScheme::getInstance().colours["Window Background"],
                     DocumentWindow::minimiseButton | DocumentWindow::maximiseButton | DocumentWindow::closeButton),
      component(c)
{
    int x, y;

    centreWithSize(400, 300);

    setResizeLimits(396, 32, 10000, 10000);
    setUsingNativeTitleBar(true);
    setContentOwned(new EditorWrapper(editor, c), true);
    setAlwaysOnTop(SettingsManager::getInstance().getBool("WindowsOnTop", false));
    // setDropShadowEnabled(false);
    // Fix for my favourite synth being unable to handle being resizable :(
    if ((c->getNode()->getProcessor()->getName() != "VAZPlusVSTi") &&
        !SettingsManager::getInstance().getBool("fixedSizeWindows", true))
    {
        /*#ifdef __APPLE__
                        //Most OSX AudioUnits/VSTs do not like being put in
        resizable windows, so
                        //we only put our internal processors in them.
                        PedalboardProcessor *p = dynamic_cast<PedalboardProcessor
        *>(editor->getAudioProcessor()); if(p) #endif*/
        setResizable(true, false);
    }

    x = component->getNode()->properties.getWithDefault("uiLastX", getX());
    if (x < 10)
        x = 10;
    y = component->getNode()->properties.getWithDefault("uiLastY", getY());
    if (y < 10)
        y = 10;
    setTopLeftPosition(x, y);

    setVisible(true);
    getPeer()->setIcon(ImageCache::getFromMemory(Images::icon512_png, Images::icon512_pngSize));
}

//------------------------------------------------------------------------------
PluginEditorWindow::~PluginEditorWindow()
{
    std::cerr << "[~PluginEditorWindow] START, component=" << component << "\n" << std::flush;
    if (component && component->getNode())
    {
        component->getNode()->properties.set("uiLastX", getX());
        component->getNode()->properties.set("uiLastY", getY());
        // Clear the pluginWindow reference so the edit button works again
        std::cerr << "[~PluginEditorWindow] Calling setWindow(0)\n" << std::flush;
        component->setWindow(0);
    }
    std::cerr << "[~PluginEditorWindow] DONE\n" << std::flush;
}

//------------------------------------------------------------------------------
void PluginEditorWindow::closeButtonPressed()
{
    std::cerr << "[closeButtonPressed] START, component=" << component << "\n" << std::flush;
    if (component)
    {
        std::cerr << "[closeButtonPressed] Calling setWindow(0)\n" << std::flush;
        component->setWindow(0);
        std::cerr << "[closeButtonPressed] setWindow(0) done\n" << std::flush;
    }
    std::cerr << "[closeButtonPressed] About to delete this\n" << std::flush;
    delete this;
    // Note: No code after delete this - object is destroyed
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
PluginEditorWindow::EditorWrapper::EditorWrapper(AudioProcessorEditor* ed, PluginComponent* comp)
    : editor(ed), component(comp)
{
    presetBar = new PresetBar(component);

    presetBar->setBounds(0, 0, 396, 32);
    addAndMakeVisible(presetBar);

    editor->setTopLeftPosition(0, 32);
    addAndMakeVisible(editor);

    if (editor->getWidth() < 396)
        setSize(396, 32 + editor->getHeight());
    else
        setSize(editor->getWidth(), 32 + editor->getHeight());
}

//------------------------------------------------------------------------------
PluginEditorWindow::EditorWrapper::~EditorWrapper()
{
    std::cerr << "[~EditorWrapper] START, editor=" << editor << ", presetBar=" << presetBar << "\n" << std::flush;
    // Since we use createEditor() (not createEditorIfNeeded()), the caller owns
    // the editor and must delete it. The old comment was incorrect - we MUST delete
    // the editor here, otherwise the plugin won't be able to create a new one.
    if (editor)
    {
        std::cerr << "[~EditorWrapper] Removing and deleting editor\n" << std::flush;
        removeChildComponent(editor);
        delete editor;
        editor = nullptr;
        std::cerr << "[~EditorWrapper] Editor deleted\n" << std::flush;
    }
    std::cerr << "[~EditorWrapper] Deleting presetBar\n" << std::flush;
    delete presetBar;
    presetBar = nullptr;
    std::cerr << "[~EditorWrapper] DONE\n" << std::flush;
}

//------------------------------------------------------------------------------
void PluginEditorWindow::EditorWrapper::resized()
{
    presetBar->setSize(getWidth(), 32);
    editor->setSize(getWidth(), getHeight() - 32);
}

//------------------------------------------------------------------------------
void PluginEditorWindow::EditorWrapper::childBoundsChanged(Component* child)
{
    if (child == editor)
    {
        if (editor->getWidth() < 396)
            setSize(396, 32 + editor->getHeight());
        else
            setSize(editor->getWidth(), 32 + editor->getHeight());
    }
}
