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
#include "CrashProtection.h"
#include "DeviceMeterTap.h"
#include "FilterGraph.h"
#include "FontManager.h"
#include "IconManager.h"
#include "Images.h"
#include "IRLoaderProcessor.h"
#include "JuceHelperStuff.h"
#include "MappingsDialog.h"
#include "MasterGainState.h"
#include "NAMProcessor.h"
#include "NotesControl.h"
#include "PedalboardProcessors.h"
#include "PluginField.h"
#include "PresetBar.h"
#include "SafetyLimiter.h"
#include "SettingsManager.h"
#include "SubGraphEditorComponent.h"
#include "SubGraphProcessor.h"
#include "TunerControl.h"
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
    {
        // PedalboardProcessor subclasses (Mixer, Splitter) dynamically change
        // channel count via setPlayConfigDetails. The cached count from construction
        // time is stale. Query the inner plugin directly for current count.
        if (auto* inner = dynamic_cast<PedalboardProcessor*>(bypassable->getPlugin()))
            return inner->getTotalNumInputChannels();
        return bypassable->getCachedInputChannelCount();
    }

    return proc->getTotalNumInputChannels();
}

int countOutputChannelsFromBuses(AudioProcessor* proc)
{
    if (auto* bypassable = dynamic_cast<BypassableInstance*>(proc))
    {
        if (auto* inner = dynamic_cast<PedalboardProcessor*>(bypassable->getPlugin()))
            return inner->getTotalNumOutputChannels();
        return bypassable->getCachedOutputChannelCount();
    }

    return proc->getTotalNumOutputChannels();
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

String getRackBoundaryRole(AudioProcessorGraph::Node* node)
{
    if (node == nullptr)
        return {};

    return node->properties.getWithDefault("rackPortRole", var()).toString();
}

String getRackBoundaryDisplayName(AudioProcessorGraph::Node* node, const String& fallback)
{
    const auto role = getRackBoundaryRole(node);
    if (role == "audio-in")
        return "Rack Input";
    if (role == "audio-out")
        return "Rack Output";
    if (role == "midi-in")
        return "Rack MIDI In";

    return fallback;
}

bool isRackAudioBoundaryNode(AudioProcessorGraph::Node* node)
{
    const auto role = getRackBoundaryRole(node);
    return role == "audio-in" || role == "audio-out";
}

bool isRackBoundaryNode(AudioProcessorGraph::Node* node)
{
    const auto role = getRackBoundaryRole(node);
    return role == "audio-in" || role == "audio-out" || role == "midi-in";
}

struct NodeVisualStyle
{
    String category;
    Colour accent;
};

constexpr const char* kShowNodeParameterControlsSettingsKey = "ShowNodeParameterControls";
constexpr const char* kRackNodeWidthProperty = "nodeWidth";
constexpr const char* kRackNodeHeightProperty = "nodeHeight";
constexpr int kMaxNodeParameterControls = 3;
constexpr int kNodeParameterControlHeight = 24;
constexpr int kNodeParameterControlGap = 4;
constexpr int kNodeParameterControlVerticalPadding = 8;

std::unique_ptr<Drawable> createNoteCloseDrawable(Colour colour, float alpha)
{
    auto drawable = std::make_unique<DrawablePath>();
    Path path;
    path.startNewSubPath(3.0f, 3.0f);
    path.lineTo(9.0f, 9.0f);
    path.startNewSubPath(9.0f, 3.0f);
    path.lineTo(3.0f, 9.0f);
    drawable->setPath(path);
    drawable->setStrokeType(PathStrokeType(2.1f, PathStrokeType::curved, PathStrokeType::rounded));
    drawable->setStrokeFill(colour.withAlpha(alpha));
    return drawable;
}

bool containsAnyToken(const String& text, std::initializer_list<const char*> tokens)
{
    for (auto* token : tokens)
    {
        if (text.containsIgnoreCase(token))
            return true;
    }

    return false;
}

bool containsTokenWord(const String& text, StringRef token)
{
    const auto lowerText = text.toLowerCase();
    const auto lowerToken = String(token).toLowerCase();
    int start = lowerText.indexOf(lowerToken);

    while (start >= 0)
    {
        const int before = start - 1;
        const int after = start + lowerToken.length();
        const auto beforeChar = before >= 0 ? lowerText[before] : juce_wchar();
        const auto afterChar = after < lowerText.length() ? lowerText[after] : juce_wchar();
        const bool startsClean = before < 0 || (!CharacterFunctions::isLetterOrDigit(beforeChar) && beforeChar != '_');
        const bool endsClean =
            after >= lowerText.length() || (!CharacterFunctions::isLetterOrDigit(afterChar) && afterChar != '_');

        if (startsClean && endsClean)
            return true;

        start = lowerText.indexOf(start + 1, lowerToken);
    }

    return false;
}

bool isLabelNodeName(const String& name)
{
    return name.equalsIgnoreCase("Label") || name.equalsIgnoreCase("Label Node");
}

bool isStickyNoteNodeName(const String& name)
{
    return name.equalsIgnoreCase("Notes") || name.equalsIgnoreCase("Note");
}

Colour graphCategoryColour(const String& role)
{
    auto& colours = ColourScheme::getInstance().colours;
    const auto found = colours.find(role);
    if (found != colours.end())
        return found->second;

    return colours["Accent Colour"];
}

NodeVisualStyle getNodeVisualStyle(AudioProcessor* processor, const String& displayName)
{
    PluginDescription desc;
    if (auto* pluginInstance = dynamic_cast<AudioPluginInstance*>(processor))
        pluginInstance->fillInPluginDescription(desc);

    const String text = displayName + " " + desc.name + " " + desc.category + " " + desc.manufacturerName + " " +
                        desc.pluginFormatName;

    // Palette copied from the Pedalboard 3 mockup M2_CAT category identity map.
    if (displayName.equalsIgnoreCase("Audio Output") || containsAnyToken(text, {"master output", "audio output"}))
        return {"out", graphCategoryColour("Graph Category Output")};

    if (displayName.equalsIgnoreCase("Audio Input") || displayName.equalsIgnoreCase("MIDI Input") ||
        displayName.equalsIgnoreCase("OSC Input") || displayName.equalsIgnoreCase("Virtual MIDI Input") ||
        containsAnyToken(text, {"file player", "tone generator", "source"}))
        return {"source", graphCategoryColour("Graph Category Source")};

    if (containsAnyToken(text, {"tuner", "vu meter", "meter", "analyser", "analyzer", "scope"}))
        return {"meter", graphCategoryColour("Graph Category Meter")};

    if (containsAnyToken(text, {"compressor", "limiter", "gate", "expander", "dynamics"}))
        return {"dyn", graphCategoryColour("Graph Category Dynamics")};

    if (containsAnyToken(text, {"distortion", "overdrive", "fuzz", "saturat", "clipper", "drive"}))
        return {"drive", graphCategoryColour("Graph Category Drive")};

    if (containsAnyToken(text, {"nam", "neural amp", "amp model", "amplifier", "cabinet", "cab sim", "impulse"}))
        return {"amp", graphCategoryColour("Graph Category Amp")};

    if (containsAnyToken(text, {"chorus", "flanger", "phaser", "tremolo", "vibrato", "rotary", "modulation"}))
        return {"mod", graphCategoryColour("Graph Category Modulation")};

    if (containsAnyToken(text, {"delay", "echo"}))
        return {"delay", graphCategoryColour("Graph Category Delay")};

    if (containsAnyToken(text, {"reverb", "room", "plate", "hall", "shimmer"}))
        return {"reverb", graphCategoryColour("Graph Category Reverb")};

    if (containsAnyToken(text, {"effect rack", "subgraph"}) || containsTokenWord(text, "rack"))
        return {"rack", graphCategoryColour("Graph Category Modulation")};

    if (desc.isInstrument)
        return {"source", graphCategoryColour("Graph Category Source")};

    return {"module", graphCategoryColour("Graph Category Delay")};
}

void drawPortLabelBackplates(Graphics& g, const OwnedArray<GlyphArrangement>& labels, Colour accent, float nodeWidth,
                             bool rightSide)
{
    if (labels.isEmpty())
        return;

    auto& colours = ColourScheme::getInstance().colours;
    const auto plateFill = colours["Window Background"].interpolatedWith(accent, 0.10f).withAlpha(0.46f);
    const auto plateBorder = accent.withAlpha(0.24f);
    const auto rail = accent.withAlpha(0.48f);

    for (auto* label : labels)
    {
        if (label == nullptr)
            continue;

        auto bounds = label->getBoundingBox(0, -1, true);
        if (bounds.isEmpty())
            continue;

        auto plate = bounds.expanded(4.5f, 2.5f);
        plate.setHeight(jmax(14.0f, plate.getHeight()));
        plate.setY(bounds.getCentreY() - plate.getHeight() * 0.5f);
        const auto maxPlateX = jmax(7.0f, nodeWidth - plate.getWidth() - 7.0f);
        plate.setX(jlimit(7.0f, maxPlateX, plate.getX()));

        g.setColour(plateFill);
        g.fillRoundedRectangle(plate, 4.0f);
        g.setColour(plateBorder);
        g.drawRoundedRectangle(plate.reduced(0.5f), 4.0f, 0.8f);

        auto railBounds = rightSide ? Rectangle<float>(plate.getRight() - 2.0f, plate.getY() + 3.0f, 1.5f,
                                                       plate.getHeight() - 6.0f)
                                    : Rectangle<float>(plate.getX() + 0.5f, plate.getY() + 3.0f, 1.5f,
                                                       plate.getHeight() - 6.0f);
        g.setColour(rail);
        g.fillRoundedRectangle(railBounds, 0.75f);
    }
}

bool areNodeParameterControlsEnabled()
{
    return SettingsManager::getInstance().getBool(kShowNodeParameterControlsSettingsKey, true);
}

bool isHeroChassisNodeName(const String& pluginName)
{
    return pluginName == "NAM Loader" || pluginName == "IR Loader";
}

bool isDirectPaintedEmbeddedNodeName(const String& pluginName)
{
    return pluginName == "Tuner" || pluginName == "Oscilloscope" || pluginName == "Tone Generator" ||
           isStickyNoteNodeName(pluginName);
}

bool usesEmbeddedParameterSurface(const String& pluginName)
{
    return pluginName == "ReverbSC";
}

bool suppressesHostParamPinForUtilityNode(const String& pluginName)
{
    return pluginName == "Oscilloscope" || pluginName == "Tone Generator";
}

bool usesCompactHostPinLabels(const String& pluginName)
{
    return pluginName == "Splitter" || pluginName == "Mixer";
}

bool shouldDrawHostPinText(const String& pluginName)
{
    return !usesCompactHostPinLabels(pluginName) && !isDirectPaintedEmbeddedNodeName(pluginName);
}

bool shouldShowHostTitleLabel(const String& pluginName)
{
    return !isHeroChassisNodeName(pluginName) && !isDirectPaintedEmbeddedNodeName(pluginName);
}

bool shouldCreateHostMidiOrParamPin(AudioProcessor* plugin, const String& pluginName, int numInputs, int numOutputs)
{
    if (isDirectPaintedEmbeddedNodeName(pluginName) || usesEmbeddedParameterSurface(pluginName) ||
        suppressesHostParamPinForUtilityNode(pluginName))
        return false;

    if ((pluginName == "Audio Input") || (pluginName == "Audio Output"))
        return false;

    return acceptsMidiSafe(plugin) || (numInputs > 0) || (numOutputs > 0);
}

String getMidiOrParameterPinLabel(AudioProcessor* plugin, const String& pluginName, bool outputPin)
{
    if (pluginName == "Virtual MIDI Input" || pluginName == "MIDI Input")
        return "MIDI";

    if (pluginName == "OSC Input")
        return "OSC";

    if (outputPin ? producesMidiSafe(plugin) : acceptsMidiSafe(plugin))
        return "MIDI";

    return "param";
}

int getEmbeddedNodeControlTopOffset(const String& pluginName)
{
    if (isDirectPaintedEmbeddedNodeName(pluginName))
        return 0;
    if (pluginName == "NAM Loader")
        return 70;
    if (pluginName == "IR Loader")
        return 78;

    return 26;
}

int getEmbeddedNodeControlHeightPadding(const String& pluginName)
{
    if (isDirectPaintedEmbeddedNodeName(pluginName))
        return 0;
    if (pluginName == "NAM Loader")
        return 84;
    if (pluginName == "IR Loader")
        return 112;
    if (pluginName == "ReverbSC")
        return 60;

    return 64;
}

Point<int> getDefaultRackNodeSize()
{
    return {324, 212};
}

AudioProcessor* unwrapVisualProcessor(AudioProcessor* processor)
{
    if (auto* bypassable = dynamic_cast<BypassableInstance*>(processor))
        return bypassable->getPlugin();

    return processor;
}

Point<int> getEmbeddedNodeControlSize(PedalboardProcessor* proc, const String& pluginName)
{
    if (pluginName == "NAM Loader")
    {
        if (auto* nam = dynamic_cast<NAMProcessor*>(proc))
            return {430, nam->isEmbeddedCabinetIrCollapsed() ? 535 : 640};
        return {430, 640};
    }
    if (pluginName == "IR Loader")
        return {344, 470};

    return proc->getSize();
}

int getEmbeddedNodeControlLeftOffset(int hostWidth, Point<int> compSize, const String& pluginName)
{
    if (isDirectPaintedEmbeddedNodeName(pluginName))
        return 0;

    return (hostWidth / 2) - (compSize.getX() / 2);
}

struct HeroChassisPalette
{
    Colour top;
    Colour bottom;
    Colour face;
    Colour face2;
    Colour inset;
    Colour edge;
    Colour edgeHi;
    Colour accent;
    Colour accent2;
    Colour led;
    Colour text;
    Colour textDim;
};

HeroChassisPalette makeHeroChassisPalette()
{
    auto& colours = ColourScheme::getInstance().colours;
    const auto accent = graphCategoryColour("Graph Category Amp");
    const auto accent2 = graphCategoryColour("Audio Connection");
    const auto window = colours["Window Background"];
    const auto plugin = colours["Plugin Background"];
    const auto field = colours["Field Background"];
    const auto edge = colours["Plugin Border"].interpolatedWith(accent, 0.16f);
    const auto face = plugin.interpolatedWith(accent, 0.15f).interpolatedWith(field, 0.18f);

    return {window.interpolatedWith(accent, 0.10f).darker(0.22f),
            window.interpolatedWith(accent, 0.06f).darker(0.44f),
            face.darker(0.10f),
            face.brighter(0.08f),
            field.interpolatedWith(plugin, 0.52f).darker(0.22f),
            edge,
            edge.brighter(0.22f),
            accent,
            accent2,
            colours["Success Colour"].brighter(0.18f),
            colours["Text Colour"],
            colours["Text Colour"].withAlpha(0.62f)};
}

void drawHeroChassisNodeChrome(Graphics& g, Rectangle<float> bounds, const String& pluginName, AudioProcessor* processor, bool highlighted, bool bypassed)
{
    auto& colours = ColourScheme::getInstance().colours;
    const auto palette = makeHeroChassisPalette();
    auto* inner = unwrapVisualProcessor(processor);
    const bool namNode = pluginName == "NAM Loader";
    const bool irNode = pluginName == "IR Loader";

    bool loaded = false;
    String title = namNode ? "No Model Loaded" : "No IR Loaded";
    if (namNode)
    {
        if (auto* nam = dynamic_cast<NAMProcessor*>(inner))
        {
            loaded = nam->isModelLoaded();
            if (loaded)
                title = nam->getModelName();
        }
    }
    else if (irNode)
    {
        if (auto* ir = dynamic_cast<IRLoaderProcessor*>(inner))
        {
            loaded = ir->isIRLoaded() || ir->isIR2Loaded();
            if (ir->isIRLoaded())
                title = ir->getIRName();
            else if (ir->isIR2Loaded())
                title = ir->getIR2Name();
        }
    }

    auto outer = bounds.reduced(1.0f);
    const float radius = 9.0f;
    g.setColour(palette.bottom.darker(0.46f).withAlpha(0.42f));
    g.fillRoundedRectangle(outer.translated(0.0f, 2.0f), radius);

    ColourGradient shellGradient(palette.top.brighter(0.05f), outer.getX(), outer.getY(), palette.bottom, outer.getX(),
                                 outer.getBottom(), false);
    shellGradient.addColour(0.44, palette.face);
    g.setGradientFill(shellGradient);
    g.fillRoundedRectangle(outer, radius);
    g.setColour(palette.edge.withAlpha(highlighted ? 0.92f : 0.62f));
    const float chassisBorderWidth = highlighted ? 1.6f : 1.1f;
    g.drawRoundedRectangle(outer.reduced(0.5f), radius, chassisBorderWidth);
    g.setColour(palette.edgeHi.withAlpha(0.42f));
    g.drawRoundedRectangle(outer.reduced(2.0f), radius - 2.0f, 0.7f);

    auto faceplate = Rectangle<float>(outer.getX() + 8.0f, outer.getY() + 8.0f, outer.getWidth() - 16.0f, 48.0f);
    ColourGradient faceGradient(palette.face2.brighter(0.10f), faceplate.getX(), faceplate.getY(),
                                palette.face.darker(0.14f), faceplate.getX(), faceplate.getBottom(), false);
    g.setGradientFill(faceGradient);
    g.fillRoundedRectangle(faceplate, 6.0f);
    g.setColour(palette.edge.withAlpha(0.62f));
    g.drawRoundedRectangle(faceplate.reduced(0.5f), 6.0f, 1.0f);

    g.setColour(palette.text.withAlpha(0.035f));
    for (float yLine = faceplate.getY() + 5.0f; yLine < faceplate.getBottom() - 4.0f; yLine += 4.0f)
        g.drawLine(faceplate.getX() + 8.0f, yLine, faceplate.getRight() - 8.0f, yLine, 0.8f);

    auto faceplateContent = faceplate.reduced(10.0f, 7.0f);
    auto glyphTile = faceplateContent.removeFromLeft(34.0f).reduced(0.0f, 1.0f);
    IconManager::getInstance().drawDomainGlyphTile(g, glyphTile,
                                                   namNode ? IconManager::DomainGlyph::Amp
                                                           : IconManager::DomainGlyph::Cabinet,
                                                   namNode ? palette.accent : palette.accent2, loaded && !bypassed,
                                                   5.0f);
    faceplateContent.removeFromLeft(10.0f);
    auto textArea = faceplateContent.withTrimmedRight(92.0f);
    g.setFont(FontManager::getInstance().getBadgeFont().withHeight(namNode ? 12.0f : 10.0f));
    g.setColour(palette.accent.withAlpha(0.88f));
    g.drawText(namNode ? "NAM LOADER" : "IR LOADER", textArea.removeFromTop(13.0f), Justification::centredLeft,
               true);
    g.setFont(FontManager::getInstance().getSubheadingFont().withHeight(namNode ? 18.0f : 16.0f));
    g.setColour(loaded ? palette.text : palette.textDim);
    g.drawText(title, textArea, Justification::centredLeft, true);

    auto statusPill = faceplate.reduced(10.0f, 12.0f).removeFromRight(82.0f);
    g.setColour(palette.inset.withAlpha(0.80f));
    g.fillRoundedRectangle(statusPill, 9.0f);
    g.setColour((loaded ? palette.led : palette.textDim).withAlpha(0.34f));
    g.drawRoundedRectangle(statusPill.reduced(0.5f), 9.0f, 1.0f);
    auto led = Rectangle<float>(8.0f, 8.0f).withCentre({statusPill.getX() + 13.0f, statusPill.getCentreY()});
    if (loaded && !bypassed)
    {
        g.setColour(palette.led.withAlpha(0.24f));
        g.fillEllipse(led.expanded(5.0f));
    }
    g.setColour((loaded && !bypassed ? palette.led : palette.textDim).withAlpha(loaded && !bypassed ? 0.92f : 0.46f));
    g.fillEllipse(led);
    g.setColour(palette.text.withAlpha(0.20f));
    g.drawEllipse(led, 0.7f);
    g.setFont(FontManager::getInstance().getBadgeFont().withHeight(namNode ? 11.0f : 9.0f));
    g.setColour((loaded && !bypassed ? palette.text : palette.textDim).withAlpha(0.82f));
    g.drawText(bypassed ? "BYPASS" : (loaded ? "ACTIVE" : "EMPTY"), statusPill.withTrimmedLeft(24.0f),
               Justification::centredLeft, true);

    g.setColour(palette.accent.withAlpha(0.70f));
    g.fillRoundedRectangle(faceplate.getX() + 14.0f, faceplate.getBottom() - 3.0f, faceplate.getWidth() - 28.0f, 2.0f,
                           1.0f);

    const float contentTop = getEmbeddedNodeControlTopOffset(pluginName) - 4.0f;
    ignoreUnused(contentTop);

    auto footer = Rectangle<float>(outer.getX() + 8.0f, outer.getBottom() - 34.0f, outer.getWidth() - 16.0f, 25.0f);
    g.setColour(palette.edge.withAlpha(0.32f));
    g.drawHorizontalLine(roundToInt(footer.getY()), footer.getX() + 4.0f, footer.getRight() - 4.0f);
    g.setColour(palette.accent.withAlpha(0.24f));
    g.fillRoundedRectangle(footer.getX() + 6.0f, footer.getBottom() - 5.0f, footer.getWidth() - 12.0f, 2.0f, 1.0f);

    if (bypassed)
    {
        g.setColour(colours["Warning Colour"].withAlpha(0.16f));
        g.fillRoundedRectangle(outer.reduced(5.0f), radius - 2.0f);
        g.setColour(colours["Warning Colour"].withAlpha(0.40f));
        g.drawRoundedRectangle(outer.reduced(4.0f), radius - 2.0f, 1.2f);
    }
}

void drawEffectRackSubgraphPreview(Graphics& g, Rectangle<float> rackPreview, Colour accentColour)
{
    auto& colours = ColourScheme::getInstance().colours;
    const auto base = colours["Field Background"].interpolatedWith(accentColour, 0.11f);
    ColourGradient rackGrad(base.brighter(0.06f), rackPreview.getX(), rackPreview.getY(), base.darker(0.12f),
                            rackPreview.getX(), rackPreview.getBottom(), false);
    rackGrad.addColour(0.42, base);
    rackGrad.addColour(0.80, colours["Field Background"]);
    g.setGradientFill(rackGrad);
    g.fillRoundedRectangle(rackPreview, 9.0f);

    g.setColour(accentColour.withAlpha(0.30f));
    g.drawRoundedRectangle(rackPreview.reduced(0.5f), 9.0f, 1.0f);
    g.setColour(colours["Window Background"].darker(0.55f).withAlpha(0.18f));
    g.drawRoundedRectangle(rackPreview.reduced(2.0f).translated(0.0f, 1.0f), 7.0f, 1.0f);

    auto badge = Rectangle<float>(rackPreview.getX() + 8.0f, rackPreview.getY() + 6.0f, 72.0f, 14.0f);
    g.setColour(accentColour.withAlpha(0.16f));
    g.fillRoundedRectangle(badge, 4.0f);
    g.setColour(accentColour.withAlpha(0.38f));
    g.drawRoundedRectangle(badge.reduced(0.5f), 4.0f, 0.8f);
    g.setFont(FontManager::getInstance().getMonoDisplayFont(7.8f));
    g.setColour(accentColour.brighter(0.10f).withAlpha(0.84f));
    g.drawText("SUB-GRAPH", badge.toNearestInt(), Justification::centred, true);

    auto gridArea = rackPreview.reduced(9.0f, 8.0f);
    const float gridStep = 13.0f;
    const float dotSize = 1.1f;
    g.setColour(accentColour.withAlpha(0.22f));
    for (float xDot = gridArea.getX() - 2.0f; xDot < gridArea.getRight(); xDot += gridStep)
    {
        for (float yDot = gridArea.getY() - 2.0f; yDot < gridArea.getBottom(); yDot += gridStep)
            g.fillEllipse(xDot - dotSize * 0.5f, yDot - dotSize * 0.5f, dotSize, dotSize);
    }

    auto graphContent = gridArea.reduced(3.0f, 6.0f);
    const float laneY = graphContent.getCentreY() - 1.0f;
    auto inDot = Rectangle<float>(10.0f, 10.0f).withCentre({graphContent.getX() + 11.0f, laneY});
    auto outDot = Rectangle<float>(10.0f, 10.0f).withCentre({graphContent.getRight() - 11.0f, laneY});

    Array<Rectangle<float>> processorNodes;
    constexpr int visibleProcessors = 3;
    const float nodeW = jmin(32.0f, jmax(24.0f, (graphContent.getWidth() - 72.0f) / 3.0f));
    const float nodeH = 28.0f;
    const float nodeGap =
        jmax(8.0f, (graphContent.getWidth() - 34.0f - nodeW * visibleProcessors) /
                       static_cast<float>(visibleProcessors + 1));
    float nodeX = inDot.getRight() + nodeGap;
    for (int i = 0; i < visibleProcessors; ++i)
    {
        auto nodeRect = Rectangle<float>(nodeX, laneY - nodeH * 0.5f, nodeW, nodeH);
        processorNodes.add(nodeRect);
        nodeX += nodeW + nodeGap;
    }

    Path route;
    auto start = inDot.getCentre();
    route.startNewSubPath(start);
    for (auto nodeRect : processorNodes)
    {
        const auto target = Point<float>{nodeRect.getX(), nodeRect.getCentreY()};
        route.cubicTo(start.x + 16.0f, start.y, target.x - 16.0f, target.y, target.x, target.y);
        start = Point<float>{nodeRect.getRight(), nodeRect.getCentreY()};
    }
    route.cubicTo(start.x + 16.0f, start.y, outDot.getCentreX() - 16.0f, outDot.getCentreY(),
                  outDot.getCentreX(), outDot.getCentreY());
    g.setColour(accentColour.withAlpha(0.16f));
    g.strokePath(route, PathStrokeType(4.0f, PathStrokeType::curved, PathStrokeType::rounded));
    g.setColour(accentColour.withAlpha(0.62f));
    g.strokePath(route, PathStrokeType(1.7f, PathStrokeType::curved, PathStrokeType::rounded));

    g.setColour(accentColour.withAlpha(0.28f));
    g.fillEllipse(inDot.expanded(4.0f));
    g.fillEllipse(outDot.expanded(4.0f));
    g.setColour(colours["Window Background"].interpolatedWith(accentColour, 0.18f));
    g.fillEllipse(inDot.expanded(1.0f));
    g.fillEllipse(outDot.expanded(1.0f));
    g.setColour(accentColour.withAlpha(0.90f));
    g.drawEllipse(inDot.expanded(1.0f), 1.4f);
    g.drawEllipse(outDot.expanded(1.0f), 1.4f);

    for (int i = 0; i < processorNodes.size(); ++i)
    {
        const auto nodeRect = processorNodes.getReference(i);
        Colour nodeAccent = accentColour.interpolatedWith(colours["Graph Category Delay"], i == 1 ? 0.24f : 0.08f);
        if (i == 2)
            nodeAccent = accentColour.interpolatedWith(colours["Graph Category Reverb"], 0.28f);

        ColourGradient nodeFill(colours["Plugin Background"].interpolatedWith(nodeAccent, 0.26f).brighter(0.04f),
                                nodeRect.getX(), nodeRect.getY(),
                                colours["Window Background"].interpolatedWith(nodeAccent, 0.16f),
                                nodeRect.getX(), nodeRect.getBottom(), false);
        nodeFill.addColour(0.62, colours["Plugin Background"].interpolatedWith(nodeAccent, 0.20f));
        g.setGradientFill(nodeFill);
        g.fillRoundedRectangle(nodeRect, 5.0f);
        g.setColour(nodeAccent.withAlpha(0.58f));
        g.drawRoundedRectangle(nodeRect.reduced(0.5f), 5.0f, 0.95f);
        g.setColour(nodeAccent.withAlpha(0.86f));
        g.fillRoundedRectangle(nodeRect.getX() + 1.5f, nodeRect.getY() + 3.0f, nodeRect.getWidth() - 3.0f, 3.0f, 1.4f);
    }
}

int countEffectRackNestedProcessors(AudioProcessor* processor)
{
    auto* unwrapped = unwrapVisualProcessor(processor);
    auto* rack = dynamic_cast<SubGraphProcessor*>(unwrapped);
    if (rack == nullptr)
        return 0;

    int count = 0;
    const auto& graph = rack->getInternalGraph();
    for (auto* node : graph.getNodes())
    {
        if (node == nullptr)
            continue;

        const auto id = node->nodeID;
        if (id == rack->getRackAudioInputNodeId() || id == rack->getRackAudioOutputNodeId() ||
            id == rack->getRackMidiInputNodeId())
            continue;

        ++count;
    }

    return count;
}

void drawEffectRackFooterSummary(Graphics& g, Rectangle<float> bounds, AudioProcessor* processor, Colour accentColour)
{
    auto& colours = ColourScheme::getInstance().colours;
    const int count = countEffectRackNestedProcessors(processor);
    const String suffix = count == 1 ? " processor nested" : " processors nested";

    auto countArea = bounds.removeFromLeft(18.0f);
    g.setFont(FontManager::getInstance().getMonoDisplayFont(11.0f));
    g.setColour(accentColour.brighter(0.08f).withAlpha(0.86f));
    g.drawText(String(count), countArea.toNearestInt(), Justification::centredLeft, true);

    g.setFont(FontManager::getInstance().getLabelFont().withHeight(10.5f));
    g.setColour(colours["Text Colour"].withAlpha(0.58f));
    g.drawText(suffix.trimStart(), bounds.toNearestInt(), Justification::centredLeft, true);
}

AudioPluginInstance* getPreviewParameterProcessor(AudioProcessor* processor)
{
    if (auto* bypassable = dynamic_cast<BypassableInstance*>(processor))
    {
        auto* plugin = bypassable->getPlugin();
        if (dynamic_cast<PedalboardProcessor*>(plugin) == nullptr)
            return plugin;
    }

    return nullptr;
}

bool isUsefulPreviewParameter(AudioProcessorParameter* parameter)
{
    return parameter != nullptr && parameter->isAutomatable() && !parameter->isMetaParameter();
}

AudioProcessorParameter* getPreviewParameter(AudioProcessor* processor, int previewIndex)
{
    if (previewIndex < 0)
        return nullptr;

    if (auto* plugin = getPreviewParameterProcessor(processor))
    {
        int usefulIndex = 0;
        auto& params = plugin->getParameters();
        for (auto* parameter : params)
        {
            if (!isUsefulPreviewParameter(parameter))
                continue;

            if (usefulIndex == previewIndex)
                return parameter;

            ++usefulIndex;
        }
    }

    return nullptr;
}

int countPreviewParameters(AudioProcessor* processor)
{
    if (auto* plugin = getPreviewParameterProcessor(processor))
    {
        int count = 0;
        auto& params = plugin->getParameters();
        for (auto* parameter : params)
        {
            if (isUsefulPreviewParameter(parameter) && ++count >= kMaxNodeParameterControls)
                return kMaxNodeParameterControls;
        }

        return count;
    }

    return 0;
}

String formatPreviewParameterValue(AudioProcessorParameter& parameter)
{
    const auto value = parameter.getValue();
    auto text = parameter.getText(value, 16).trim();
    if (text.isEmpty())
        text = parameter.getCurrentValueAsText().trim();
    if (text.isEmpty())
        text = String(roundToInt(value * 100.0f)) + "%";

    auto label = parameter.getLabel().trim();
    if (label.isNotEmpty() && !text.containsIgnoreCase(label))
        text << " " << label;

    return text.substring(0, 18);
}

class PluginNodeFooterButtonLookAndFeel final : public LookAndFeel_V4
{
  public:
    void drawButtonBackground(Graphics& g, Button& button, const Colour&, bool isMouseOverButton,
                              bool isButtonDown) override
    {
        auto& schemeColours = ::ColourScheme::getInstance().colours;
        auto bounds = button.getLocalBounds().toFloat().reduced(0.5f);
        if (bounds.isEmpty())
            return;

        auto accent = schemeColours["Accent Colour"];
        if (auto* owner = button.findParentComponentOfClass<PluginComponent>())
            accent = owner->getVisualAccentColour();

        const auto base = schemeColours["Plugin Background"].interpolatedWith(accent, isMouseOverButton ? 0.18f : 0.10f);
        if (!isButtonDown)
        {
            g.setColour(schemeColours["Window Background"].darker(0.62f).withAlpha(0.26f));
            g.fillRoundedRectangle(bounds.translated(0.0f, 1.0f), 5.0f);
        }

        ColourGradient fill(base.brighter(isMouseOverButton ? 0.16f : 0.09f), bounds.getX(), bounds.getY(),
                            base.darker(isButtonDown ? 0.20f : 0.12f), bounds.getX(), bounds.getBottom(), false);
        fill.addColour(0.54, base);
        g.setGradientFill(fill);
        g.fillRoundedRectangle(bounds, 5.0f);

        g.setColour(schemeColours["Text Colour"].contrasting(0.20f).withAlpha(isMouseOverButton ? 0.10f : 0.055f));
        g.drawLine(bounds.getX() + 4.0f, bounds.getY() + 1.0f, bounds.getRight() - 4.0f, bounds.getY() + 1.0f,
                   1.0f);

        g.setColour(schemeColours["Plugin Border"].interpolatedWith(accent, isMouseOverButton ? 0.54f : 0.30f));
        g.drawRoundedRectangle(bounds.reduced(0.5f), 5.0f, isMouseOverButton ? 1.1f : 0.8f);
    }

    void drawButtonText(Graphics& g, TextButton& button, bool, bool) override
    {
        auto& schemeColours = ::ColourScheme::getInstance().colours;
        auto accent = schemeColours["Accent Colour"];
        if (auto* owner = button.findParentComponentOfClass<PluginComponent>())
            accent = owner->getVisualAccentColour();

        g.setFont(FontManager::getInstance().getBadgeFont().withHeight(button.getHeight() <= 20 ? 8.5f : 9.4f));
        g.setColour(schemeColours["Text Colour"].interpolatedWith(accent, 0.12f).withAlpha(button.isEnabled() ? 0.88f : 0.36f));
        g.drawText(button.getButtonText(), button.getLocalBounds().reduced(3, 1), Justification::centred, true);
    }
};

PluginNodeFooterButtonLookAndFeel pluginNodeFooterButtonLookAndFeel;
} // namespace

//------------------------------------------------------------------------------
class NodeParameterMiniControl final : public Component, public SettableTooltipClient
{
  public:
    explicit NodeParameterMiniControl(AudioProcessorParameter& parameterToUse) : parameter(parameterToUse)
    {
        setRepaintsOnMouseActivity(true);
        refreshTooltip();
    }

    ~NodeParameterMiniControl() override
    {
        endGestureIfNeeded();
    }

    void paint(Graphics& g) override
    {
        auto& colours = ColourScheme::getInstance().colours;
        auto bounds = getLocalBounds().toFloat().reduced(0.5f);
        const auto value = jlimit(0.0f, 1.0f, parameter.getValue());
        const auto active = isMouseOverOrDragging();

        auto accent = colours["Accent Colour"];
        if (auto* owner = dynamic_cast<PluginComponent*>(getParentComponent()))
            accent = owner->getVisualAccentColour();

        auto panel = colours["Plugin Background"].interpolatedWith(accent, active ? 0.14f : 0.08f);
        ColourGradient fill(panel.brighter(0.10f), bounds.getX(), bounds.getY(), panel.darker(0.12f), bounds.getX(),
                            bounds.getBottom(), false);
        fill.addColour(0.52, panel);
        g.setGradientFill(fill);
        g.fillRoundedRectangle(bounds, 5.0f);

        g.setColour(colours["Plugin Border"].interpolatedWith(accent, active ? 0.42f : 0.22f));
        g.drawRoundedRectangle(bounds, 5.0f, active ? 1.2f : 0.8f);

        auto textBounds = bounds.reduced(7.0f, 3.0f).toNearestInt();
        auto nameArea = textBounds.removeFromLeft(jmax(44, textBounds.getWidth() / 2));
        auto valueArea = textBounds;

        g.setFont(FontManager::getInstance().getCaptionFont());
        g.setColour(colours["Text Colour"].withAlpha(0.78f));
        g.drawFittedText(parameter.getName(18), nameArea, Justification::centredLeft, 1);

        g.setFont(FontManager::getInstance().getBadgeFont());
        g.setColour(colours["Text Colour"].withAlpha(0.68f));
        g.drawFittedText(formatPreviewParameterValue(parameter), valueArea, Justification::centredRight, 1);

        auto track = Rectangle<float>(bounds.getX() + 7.0f, bounds.getBottom() - 5.0f, bounds.getWidth() - 14.0f, 2.0f);
        g.setColour(colours["Window Background"].darker(0.25f).withAlpha(0.72f));
        g.fillRoundedRectangle(track, 1.0f);

        auto filled = track.withWidth(track.getWidth() * value);
        ColourGradient valueFill(accent.brighter(0.30f), filled.getX(), filled.getY(), accent.darker(0.12f),
                                 filled.getRight(), filled.getY(), false);
        g.setGradientFill(valueFill);
        g.fillRoundedRectangle(filled, 1.0f);

        const auto thumbX = track.getX() + track.getWidth() * value;
        g.setColour(accent.withAlpha(active ? 0.95f : 0.72f));
        g.fillEllipse(thumbX - 2.5f, track.getCentreY() - 2.5f, 5.0f, 5.0f);
    }

    void mouseDown(const MouseEvent& event) override
    {
        if (event.getNumberOfClicks() >= 2)
        {
            beginGestureIfNeeded();
            setParameterValue(parameter.getDefaultValue());
            endGestureIfNeeded();
            return;
        }

        beginGestureIfNeeded();
        setValueFromX((float)event.x);
    }

    void mouseDrag(const MouseEvent& event) override
    {
        beginGestureIfNeeded();
        setValueFromX((float)event.x);
    }

    void mouseUp(const MouseEvent&) override
    {
        endGestureIfNeeded();
    }

    void mouseWheelMove(const MouseEvent&, const MouseWheelDetails& wheel) override
    {
        beginGestureIfNeeded();
        const auto delta = wheel.deltaY * (wheel.isReversed ? -1.0f : 1.0f) * 0.06f;
        setParameterValue(parameter.getValue() + delta);
        endGestureIfNeeded();
    }

  private:
    void beginGestureIfNeeded()
    {
        if (!gestureActive)
        {
            parameter.beginChangeGesture();
            gestureActive = true;
        }
    }

    void endGestureIfNeeded()
    {
        if (gestureActive)
        {
            parameter.endChangeGesture();
            gestureActive = false;
        }
    }

    void setValueFromX(float x)
    {
        setParameterValue(x / jmax(1.0f, (float)getWidth()));
    }

    void setParameterValue(float newValue)
    {
        parameter.setValueNotifyingHost(jlimit(0.0f, 1.0f, newValue));
        refreshTooltip();
        repaint();
    }

    void refreshTooltip()
    {
        setTooltip(parameter.getName(64) + ": " + formatPreviewParameterValue(parameter) +
                   " (drag to adjust, double-click to reset)");
    }

    AudioProcessorParameter& parameter;
    bool gestureActive = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NodeParameterMiniControl)
};

bool shouldSuppressWholeNodeDragFrom(Component* component)
{
    for (auto* current = component; current != nullptr; current = current->getParentComponent())
    {
        if (dynamic_cast<PluginComponent*>(current) != nullptr)
            return false;

        if (dynamic_cast<Button*>(current) != nullptr || dynamic_cast<Slider*>(current) != nullptr ||
            dynamic_cast<ComboBox*>(current) != nullptr || dynamic_cast<TextEditor*>(current) != nullptr ||
            dynamic_cast<NodeParameterMiniControl*>(current) != nullptr ||
            dynamic_cast<ResizableBorderComponent*>(current) != nullptr)
            return true;
    }

    return false;
}

bool isStickyNoteResizeHandleEvent(const String& pluginName, const MouseEvent& event)
{
    if (!isStickyNoteNodeName(pluginName))
        return false;

    for (auto* current = event.originalComponent; current != nullptr; current = current->getParentComponent())
    {
        if (auto* notesControl = dynamic_cast<NotesControl*>(current))
            return notesControl->isResizeHandleHit(event.getEventRelativeTo(notesControl).getPosition());

        if (dynamic_cast<PluginComponent*>(current) != nullptr)
            break;
    }

    return false;
}

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
      titleLabel(0), editButton(0), mappingsButton(0), bypassButton(0), deleteButton(0), node(n), pluginWindow(0),
      beingDragged(false), dragX(0), dragY(0)
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
    displayName = getRackBoundaryDisplayName(node, pluginName);
    const auto visualStyle = getNodeVisualStyle(node->getProcessor(), pluginName);
    visualCategoryName = visualStyle.category;
    visualAccentColour = visualStyle.accent;
    spdlog::debug("[PluginComponent] creating '{}'", pluginName.toStdString());

    setRepaintsOnMouseActivity(true);

    determineSize();

    titleLabel = new Label("titleLabe", displayName);
    titleLabel->setBounds(20, 3, getWidth() - 28, 20);
    titleLabel->setInterceptsMouseClicks(false, false);
    titleLabel->setFont(FontManager::getInstance().getSubheadingFont());
    titleLabel->setJustificationType(Justification::centredLeft);
    titleLabel->addListener(this);
    addAndMakeVisible(titleLabel);
    layoutTitleLabel();

    if ((pluginName != "Audio Input") && (pluginName != "MIDI Input") && (pluginName != "Audio Output") &&
        (pluginName != "OSC Input") && (pluginName != "Virtual MIDI Input"))
    {
        const bool stickyNoteNode = isStickyNoteNodeName(pluginName);
        std::unique_ptr<Drawable> closeUp;
        std::unique_ptr<Drawable> closeOver;
        std::unique_ptr<Drawable> closeDown;
        if (stickyNoteNode)
        {
            const auto noteCloseBase = ColourScheme::getInstance().colours["Warning Colour"];
            closeUp = createNoteCloseDrawable(noteCloseBase.darker(0.18f), 0.76f);
            closeOver = createNoteCloseDrawable(noteCloseBase.brighter(0.12f), 0.94f);
            closeDown = createNoteCloseDrawable(noteCloseBase.darker(0.32f), 0.98f);
        }
        else
        {
            closeUp.reset(
                JuceHelperStuff::loadSVGFromMemory(Vectors::closefilterbutton_svg, Vectors::closefilterbutton_svgSize));
            closeOver.reset(JuceHelperStuff::loadSVGFromMemory(Vectors::closefilterbuttonover_svg,
                                                               Vectors::closefilterbuttonover_svgSize));
            closeDown.reset(JuceHelperStuff::loadSVGFromMemory(Vectors::closefilterbuttondown_svg,
                                                               Vectors::closefilterbuttondown_svgSize));
        }
        std::unique_ptr<Drawable> bypassOff(
            JuceHelperStuff::loadSVGFromMemory(Vectors::bypassbuttonoff_svg, Vectors::bypassbuttonoff_svgSize));
        std::unique_ptr<Drawable> bypassOn(
            JuceHelperStuff::loadSVGFromMemory(Vectors::bypassbuttonon_svg, Vectors::bypassbuttonon_svgSize));

        // So the audio I/O etc. don't get their titles squeezed by the
        // non-existent close button.
        titleLabel->setBounds(20, 3, getWidth() - 40, 20);

        const bool labelNode = isLabelNodeName(pluginName);
        const bool suppressHostEditorButton =
            labelNode || isDirectPaintedEmbeddedNodeName(pluginName) || usesEmbeddedParameterSurface(pluginName);
        const bool suppressHostMappingsButton = labelNode || isDirectPaintedEmbeddedNodeName(pluginName) || visualCategoryName == "rack";
        const bool suppressHostBypassButton = labelNode || isDirectPaintedEmbeddedNodeName(pluginName);
        int footerButtonX = 10;

        if (!suppressHostEditorButton)
        {
            editButton = new TextButton("e", "Open plugin editor (right-click for options)");
            editButton->setLookAndFeel(&pluginNodeFooterButtonLookAndFeel);
            int editButtonWidth = 20;
            if (isHeroChassisNodeName(pluginName))
            {
                editButton->setButtonText("Edit");
                editButtonWidth = 44;
            }
            else if (visualCategoryName == "rack")
            {
                editButton->setButtonText("Open");
                editButtonWidth = 48;
            }
            editButton->setBounds(footerButtonX, getHeight() - 30, editButtonWidth, 20);
            footerButtonX += editButtonWidth + 2;
            editButton->addListener(this);
            // Add mouse listener for right-click context menu
            editButton->addMouseListener(this, false);
            addAndMakeVisible(editButton);
        }

        if (!suppressHostMappingsButton)
        {
            mappingsButton = new TextButton("m", "Open mappings editor");
            mappingsButton->setLookAndFeel(&pluginNodeFooterButtonLookAndFeel);
            int mappingsButtonWidth = 24;
            if (isHeroChassisNodeName(pluginName))
            {
                mappingsButton->setButtonText("Map");
                mappingsButtonWidth = 42;
            }
            mappingsButton->setBounds(footerButtonX, getHeight() - 30, mappingsButtonWidth, 20);
            mappingsButton->addListener(this);
            addAndMakeVisible(mappingsButton);
        }

        if (!suppressHostBypassButton)
        {
            bypassButton = new DrawableButton("BypassFilterButton", DrawableButton::ImageOnButtonBackground);
            bypassButton->setImages(bypassOff.get(), nullptr, nullptr, nullptr, bypassOn.get());
            bypassButton->setClickingTogglesState(true);
            if (isHeroChassisNodeName(pluginName))
                bypassButton->setEdgeIndent(4);
            bypassButton->setBounds(getWidth() - 30, getHeight() - 30, 20, 20);
            bypassButton->addListener(this);
            addAndMakeVisible(bypassButton);
        }

        deleteButton = new DrawableButton("DeleteFilterButton", DrawableButton::ImageRaw);
        deleteButton->setImages(closeUp.get(), closeOver.get(), closeDown.get());
        deleteButton->setEdgeIndent(0);
        deleteButton->setBounds(getWidth() - 17, 5, 12, 12);
        deleteButton->addListener(this);
        addAndMakeVisible(deleteButton);
    }

    if (visualCategoryName == "rack")
    {
        rackBoundsConstrainer.setMinimumSize(248, 152);
        rackBoundsConstrainer.setMaximumSize(620, 520);
        rackResizeBorder = std::make_unique<ResizableBorderComponent>(this, &rackBoundsConstrainer);
        rackResizeBorder->setBorderThickness(BorderSize<int>(5));
        addAndMakeVisible(rackResizeBorder.get());
    }

    if (proc)
    {
        int tempint;
        Component* comp = proc->getControls();
        Point<int> compSize = proc->getSize();
        compSize = getEmbeddedNodeControlSize(proc, pluginName);

        spdlog::debug("[PluginComponent] proc valid, getControls()={}, getSize()={}x{}", comp != nullptr,
                      compSize.getX(), compSize.getY());

        addAndMakeVisible(comp);
        comp->addMouseListener(this, true);

        if (pluginName == "Tuner")
        {
            if (auto* tunerControl = dynamic_cast<TunerControl*>(comp))
            {
                tunerControl->setBypassController(
                    [bypassable]() { return bypassable != nullptr && bypassable->getBypass(); },
                    [bypassable, this](bool bypassed)
                    {
                        if (bypassable != nullptr)
                        {
                            bypassable->setBypass(bypassed);
                            repaint();
                        }
                    });
            }
        }

        tempint = getEmbeddedNodeControlLeftOffset(getWidth(), compSize, pluginName);
        comp->setBounds(tempint, getEmbeddedNodeControlTopOffset(pluginName), compSize.getX(), compSize.getY());
        if (deleteButton != nullptr)
            deleteButton->toFront(false);

        spdlog::debug("[PluginComponent] Control positioned: x={}, y={}, PluginComponent size={}x{}", tempint,
                      getEmbeddedNodeControlTopOffset(pluginName), getWidth(), getHeight());
    }

    createPins();
    rebuildNodeParameterControls();
    layoutNodeParameterControls();

    // Create per-channel gain sliders for Audio I/O nodes (inline with pins)
    if (isAudioIONode())
    {
        AudioProcessor* plugin = node->getProcessor();
        bool isInput = (pluginName == "Audio Input");
        int numCh = isInput ? countOutputChannelsFromBuses(plugin) : countInputChannelsFromBuses(plugin);
        auto& state = MasterGainState::getInstance();

        const float meterStartY = 44.0f;
        const float pinSpacing = 40.0f;
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
            float initDb = isInput ? state.inputChannelGainDb[ch].load(std::memory_order_relaxed)
                                   : state.outputChannelGainDb[ch].load(std::memory_order_relaxed);
            slider->setValue(initDb, dontSendNotification);

            addAndMakeVisible(slider);
            channelGainSliders.add(slider);
        }
    }

    if (node->properties.getWithDefault("windowOpen", false) && editButton != nullptr)
        buttonClicked(editButton);
}

//------------------------------------------------------------------------------
PluginComponent::~PluginComponent()
{
    stopTimer();                     // Stop drag lerp timer
    if (editButton != nullptr)
        editButton->setLookAndFeel(nullptr);
    if (mappingsButton != nullptr)
        mappingsButton->setLookAndFeel(nullptr);
    rackResizeBorder.reset();
    channelGainSliders.clear(false); // Release without deleting - deleteAllChildren() handles it
    nodeParameterControls.clear(false);
    deleteAllChildren();
    if (pluginWindow)
        delete pluginWindow;
}

//------------------------------------------------------------------------------
void PluginComponent::resized()
{
    layoutTitleLabel();

    layoutFooterButtons();
    if (deleteButton != nullptr)
        deleteButton->setBounds(getWidth() - 17, 5, 12, 12);

    for (auto* pin : outputPins)
    {
        if (pin != nullptr)
            pin->setTopLeftPosition(isAudioIONode() ? getWidth() - 8 : getWidth() - 6, pin->getY());
    }

    if (visualCategoryName == "rack" && node != nullptr)
    {
        node->properties.set(kRackNodeWidthProperty, getWidth());
        node->properties.set(kRackNodeHeightProperty, getHeight());
    }

    layoutNodeParameterControls();

    if (rackResizeBorder != nullptr)
    {
        rackResizeBorder->setBounds(getLocalBounds());
        rackResizeBorder->toFront(false);
        sendChangeMessage();
    }
}

//------------------------------------------------------------------------------
void PluginComponent::layoutTitleLabel()
{
    const bool midiInputSourceNode = pluginName == "MIDI Input";
    const int titleLeft = isRackBoundaryNode(node) ? 12 : (isAudioIONode() ? 22 : 20);
    const int titleRightInset = deleteButton ? 24 : 8;
    if (titleLabel != nullptr)
    {
        titleLabel->setJustificationType(midiInputSourceNode ? Justification::centredRight
                                                             : Justification::centredLeft);
        titleLabel->setBounds(titleLeft, 3, jmax(0, getWidth() - titleLeft - titleRightInset), 20);
        titleLabel->setVisible(shouldShowHostTitleLabel(pluginName));
    }
}

Rectangle<float> PluginComponent::getEffectRackSubgraphPreviewBounds() const
{
    constexpr float headerHeight = 23.0f;
    const float w = static_cast<float>(getWidth());
    const float h = static_cast<float>(getHeight());
    return {12.0f, headerHeight + 10.0f, w - 24.0f, jmax(86.0f, h - headerHeight - 66.0f)};
}

//------------------------------------------------------------------------------
void PluginComponent::layoutFooterButtons()
{
    const bool heroNode = isHeroChassisNodeName(pluginName);
    const bool rackNode = visualCategoryName == "rack";

    if (heroNode)
    {
        const int y = getHeight() - 31;
        const int h = 22;

        if (editButton != nullptr)
            editButton->setBounds(16, y, 46, h);
        if (mappingsButton != nullptr)
            mappingsButton->setBounds(68, y, 48, h);
        if (bypassButton != nullptr)
            bypassButton->setBounds(getWidth() - 40, y, 26, h);

        return;
    }

    if (rackNode)
    {
        const int y = getHeight() - 31;
        const int h = 22;
        const int buttonGap = 8;

        if (bypassButton != nullptr)
            bypassButton->setBounds(getWidth() - 40, y, 26, h);
        const int openButtonRight = bypassButton != nullptr ? bypassButton->getX() - buttonGap : getWidth() - 48;
        if (editButton != nullptr)
            editButton->setBounds(openButtonRight - 54, y, 54, h);
        if (mappingsButton != nullptr)
            mappingsButton->setBounds((editButton != nullptr ? editButton->getX() : openButtonRight) - buttonGap - 46,
                                      y, 46, h);

        return;
    }

    if (usesEmbeddedParameterSurface(pluginName))
    {
        const int y = getHeight() - 25;
        const int h = 18;

        if (mappingsButton != nullptr)
            mappingsButton->setBounds(14, y, 22, h);
        if (bypassButton != nullptr)
            bypassButton->setBounds(getWidth() - 28, y, 18, h);

        return;
    }

    if (editButton != nullptr)
        editButton->setBounds(10, getHeight() - 30, 20, 20);
    if (mappingsButton != nullptr)
        mappingsButton->setBounds(32, getHeight() - 30, 24, 20);
    if (bypassButton != nullptr)
        bypassButton->setBounds(getWidth() - 30, getHeight() - 30, 20, 20);
}

//------------------------------------------------------------------------------
void PluginComponent::paint(Graphics& g)
{
    int i;
    auto& colours = ColourScheme::getInstance().colours;
    float w = (float)getWidth();
    float h = (float)getHeight();
    const float cornerRadius = 8.0f;
    const bool bypassed = bypassButton != nullptr && bypassButton->getToggleState();
    const bool highlighted = beingDragged || isMouseOver(true);
    const bool rackNode = visualCategoryName == "rack";
    const bool heroChassisNode = isHeroChassisNodeName(pluginName);
    const bool directPaintedNode = isDirectPaintedEmbeddedNodeName(pluginName);
    Colour accentColour = rackNode ? colours["Graph Category Modulation"] : visualAccentColour;

    if (directPaintedNode)
        return;

    if (heroChassisNode)
    {
        drawHeroChassisNodeChrome(g, getLocalBounds().toFloat(), pluginName, node->getProcessor(), highlighted,
                                  bypassed);
        return;
    }

    // === MAIN FILL (gradient for premium feel) ===
    Colour bgBase =
        colours["Plugin Background"].interpolatedWith(accentColour, rackNode ? 0.145f : (isAudioIONode() ? 0.10f : 0.065f));
    Colour bgTop = bgBase.brighter(0.11f);
    Colour bgBottom = bgBase.darker(0.13f);
    g.setGradientFill(ColourGradient(bgTop, 0, 0, bgBottom, 0, h, false));
    g.fillRoundedRectangle(2.0f, 2.0f, w - 4.0f, h - 4.0f, cornerRadius);

    // === BORDER (thin mockup-style chrome) ===
    g.setColour(colours["Plugin Border"].interpolatedWith(accentColour, highlighted ? 0.30f : 0.15f));
    const float nodeBorderWidth = highlighted ? 0.82f : 0.58f;
    g.drawRoundedRectangle(2.0f, 2.0f, w - 4.0f, h - 4.0f, cornerRadius, nodeBorderWidth);

    // === HEADER BAR (title area with gradient) ===
    const float headerHeight = 23.0f;
    {
        Colour headerTop, headerBottom;
        Colour base = colours["Plugin Background"].interpolatedWith(accentColour, isAudioIONode() ? 0.34f : 0.24f);
        if (bypassed)
            base = base.interpolatedWith(colours["Warning Colour"], 0.22f);
        headerTop = base.brighter(0.16f);
        headerBottom = base.darker(0.08f);
        g.setGradientFill(ColourGradient(headerTop, 0, 2.0f, headerBottom, 0, headerHeight + 2.0f, false));
    }
    {
        Path headerPath;
        headerPath.addRoundedRectangle(2.0f, 2.0f, w - 4.0f, headerHeight, cornerRadius, cornerRadius, true, true,
                                       false, false);
        g.fillPath(headerPath);
    }

    {
        const bool showHeaderDot = !isAudioIONode();
        const float dotRadius = showHeaderDot ? 4.0f : 3.0f;
        const float dotX = showHeaderDot ? 11.0f : w - 13.0f;
        const float dotY = 13.5f;
        Path dot;
        dot.addEllipse(dotX - dotRadius, dotY - dotRadius, dotRadius * 2.0f, dotRadius * 2.0f);
        melatonin::DropShadow dotGlow{accentColour.withAlpha(bypassed ? 0.30f : 0.55f), 5, {0, 0}};
        dotGlow.render(g, dot);
        g.setColour(accentColour.withAlpha(bypassed ? 0.48f : 0.94f));
        g.fillPath(dot);
        g.setColour(colours["Text Colour"].withAlpha(0.18f));
        g.drawEllipse(dotX - dotRadius, dotY - dotRadius, dotRadius * 2.0f, dotRadius * 2.0f, 0.8f);
    }

    // Subtle top highlight (inner bevel)
    g.setColour(colours["Text Colour"].withAlpha(0.06f));
    g.drawHorizontalLine(3, 4.0f, w - 4.0f);

    // Separator line at bottom of header
    g.setColour(colours["Plugin Border"].interpolatedWith(accentColour, 0.12f).withAlpha(0.58f));
    g.drawLine(3.0f, headerHeight + 1.0f, w - 3.0f, headerHeight + 1.0f, 0.75f);

    // === ICON for Audio I/O nodes ===
    if (isAudioIONode())
    {
        const float iconSize = 14.0f;
        const float iconX = 5.0f;
        const float iconY = 5.0f;

        auto& iconManager = IconManager::getInstance();
        std::unique_ptr<Drawable> icon;

        if (pluginName == "Audio Input")
            icon = iconManager.getMicIcon(accentColour.brighter(0.18f));
        else
            icon = iconManager.getSpeakerIcon(accentColour.brighter(0.18f));

        if (icon)
            icon->drawWithin(g, Rectangle<float>(iconX, iconY, iconSize, iconSize), RectanglePlacement::centred, 1.0f);

        // Draw device name subtitle
        if (auto* tap = DeviceMeterTap::getInstance())
        {
            String deviceName = tap->getDeviceName();
            if (deviceName.isNotEmpty())
            {
                g.setColour(colours["Text Colour"].withAlpha(0.6f));
                g.setFont(FontManager::getInstance().getCaptionFont());
                g.drawText(deviceName, 4.0f, 25.0f, w - 8.0f, 14.0f, Justification::centred, true);
            }
        }
    }

    // === INNER BODY HIGHLIGHT (subtle top edge below header) ===
    g.setColour(colours["Text Colour"].withAlpha(0.03f));
    g.fillRect(3.0f, headerHeight + 2.0f, w - 6.0f, 1.0f);

    // === FOOTER SEPARATOR (above edit/bypass buttons) ===
    if ((pluginName != "Audio Input") && (pluginName != "MIDI Input") && (pluginName != "Audio Output") &&
        (pluginName != "OSC Input") && (pluginName != "Virtual MIDI Input"))
    {
        float footerY = usesEmbeddedParameterSurface(pluginName) ? h - 31.0f : h - 36.0f;
        g.setColour(colours["Plugin Border"].withAlpha(0.28f));
        g.drawLine(6.0f, footerY, w - 6.0f, footerY, 0.75f);
    }

    if (bypassed)
    {
        g.setColour(colours["Warning Colour"].withAlpha(0.20f));
        g.fillRoundedRectangle(5.0f, headerHeight + 4.0f, w - 10.0f, 3.0f, 1.5f);
        g.setColour(colours["Warning Colour"].withAlpha(0.42f));
        g.drawRoundedRectangle(4.0f, 4.0f, w - 8.0f, h - 8.0f, cornerRadius - 2.0f, 1.3f);
    }

    if (highlighted)
    {
        g.setColour(accentColour.withAlpha(beingDragged ? 0.40f : 0.18f));
        g.drawRoundedRectangle(1.0f, 1.0f, w - 2.0f, h - 2.0f, cornerRadius + 1.0f,
                               beingDragged ? 1.12f : 0.72f);
    }

    if (rackNode)
    {
        drawEffectRackSubgraphPreview(g, getEffectRackSubgraphPreviewBounds(), accentColour);

        auto rackFooterSummary = Rectangle<float>(16.0f, h - 31.0f, jmax(72.0f, w - 186.0f), 22.0f);
        drawEffectRackFooterSummary(g, rackFooterSummary, node != nullptr ? node->getProcessor() : nullptr,
                                    accentColour);
    }

    // Draw port label backplates before the existing glyph arrangements.
    if (!isAudioIONode() && !rackNode)
    {
        drawPortLabelBackplates(g, inputText, accentColour, w, false);
        drawPortLabelBackplates(g, outputText, accentColour, w, true);
    }

    if (!rackNode)
    {
        // Draw the input channels.
        g.setColour(colours["Text Colour"]);
        for (i = 0; i < inputText.size(); ++i)
            inputText[i]->draw(g);

        // Draw the output channels.
        for (i = 0; i < outputText.size(); ++i)
            outputText[i]->draw(g);
    }

    // Draw horizontal VU meters for Audio I/O nodes (full width)
    if (isAudioIONode() && cachedMeterChannelCount > 0)
    {
        const float pinMargin = 22.0f;
        const float edgeMargin = 8.0f;
        const float meterWidth = w - pinMargin - edgeMargin;
        const float meterHeight = 8.0f;
        const float meterStartY = 44.0f;
        const float pinSpacing = 40.0f;

        for (int ch = 0; ch < cachedMeterChannelCount && ch < 16; ++ch)
        {
            float level = cachedMeterLevels[ch];
            float levelDb = (level > 0.001f) ? 20.0f * std::log10(level) : -60.0f;
            float normalizedLevel = jlimit(0.0f, 1.0f, (levelDb + 60.0f) / 60.0f);

            float mx = (pluginName == "Audio Input") ? edgeMargin : pinMargin;
            float my = meterStartY + ch * pinSpacing;

            // Meter background
            g.setColour(colours["Plugin Background"].darker(0.5f));
            g.fillRoundedRectangle(mx, my, meterWidth, meterHeight, 2.0f);

            // Gradient-filled meter bar
            if (normalizedLevel > 0.0f)
            {
                float barWidth = meterWidth * normalizedLevel;

                // Glow effect when level is hot (> -6 dB = 0.9 normalized)
                if (normalizedLevel > 0.9f)
                {
                    float glowAlpha = jlimit(0.0f, 1.0f, (normalizedLevel - 0.9f) * 3.0f);
                    Colour glowColour = (level >= 1.0f) ? colours["Danger Colour"].withAlpha(glowAlpha)
                                                        : colours["Warning Colour"].withAlpha(glowAlpha * 0.7f);
                    Path meterBar;
                    meterBar.addRoundedRectangle(mx, my, barWidth, meterHeight, 2.0f);
                    melatonin::DropShadow meterGlow{glowColour, 6, {0, 0}};
                    meterGlow.render(g, meterBar);
                }

                // Green-to-yellow-to-red gradient across full meter width
                ColourGradient gradient(colours["VU Meter Lower Colour"], mx, my, colours["VU Meter Over Colour"],
                                        mx + meterWidth, my, false);
                gradient.addColour(0.65, colours["VU Meter Upper Colour"]);
                g.setGradientFill(gradient);

                // Clip to actual level width
                g.saveState();
                g.reduceClipRegion(Rectangle<int>((int)mx, (int)my, (int)(barWidth + 1.0f), (int)(meterHeight + 1.0f)));
                g.fillRoundedRectangle(mx, my, meterWidth, meterHeight, 2.0f);
                g.restoreState();
            }

            // Peak hold indicator
            if (peakHoldLevels[ch] > 0.01f)
            {
                float peakX = mx + meterWidth * peakHoldLevels[ch];
                // Color based on peak position
                Colour peakColour;
                if (peakHoldLevels[ch] > 0.95f)
                    peakColour = colours["VU Meter Over Colour"];
                else if (peakHoldLevels[ch] > 0.65f)
                    peakColour = colours["VU Meter Upper Colour"];
                else
                    peakColour = colours["VU Meter Lower Colour"].brighter(0.3f);

                float alpha = (peakHoldCounters[ch] > 0) ? 1.0f : jmax(0.3f, peakHoldLevels[ch]);
                g.setColour(peakColour.withAlpha(alpha));
                g.fillRect(peakX - 1.0f, my, 2.0f, meterHeight);
            }

            // dB scale tick marks
            g.setColour(colours["Plugin Border"].withAlpha(0.25f));
            const float dbMarks[] = {-48.0f, -24.0f, -12.0f, -6.0f, -3.0f, 0.0f};
            for (float db : dbMarks)
            {
                float tickNorm = (db + 60.0f) / 60.0f;
                float tickX = mx + meterWidth * tickNorm;
                g.drawVerticalLine((int)tickX, my, my + meterHeight);
            }

            // Border
            g.setColour(colours["Plugin Border"].withAlpha(0.3f));
            g.drawRoundedRectangle(mx, my, meterWidth, meterHeight, 2.0f, 0.5f);
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

    if (bypassable != nullptr && bypassButton != nullptr)
        bypassButton->setToggleState(bypassable->getBypass(), false);

    // Update meter levels for Audio I/O nodes
    if (isAudioIONode())
    {
        bool needsRepaint = false;
        int numChannels = 0;

        if (auto* limiter = SafetyLimiterProcessor::getInstance())
        {
            AudioProcessor* plugin = node->getProcessor();
            bool isInput = (pluginName == "Audio Input");
            numChannels = isInput ? countOutputChannelsFromBuses(plugin) : countInputChannelsFromBuses(plugin);
            numChannels = jmin(numChannels, 16);
            for (int ch = 0; ch < numChannels; ++ch)
            {
                // VU ballistic level for smooth meter bar
                float vuLevel =
                    (pluginName == "Audio Input") ? limiter->getInputVuLevel(ch) : limiter->getOutputVuLevel(ch);
                if (std::abs(vuLevel - cachedMeterLevels[ch]) > 0.001f)
                {
                    cachedMeterLevels[ch] = vuLevel;
                    needsRepaint = true;
                }

                // Peak level for peak hold indicator (sharp, instantaneous)
                float peakLevel =
                    (pluginName == "Audio Input") ? limiter->getInputLevel(ch) : limiter->getOutputLevel(ch);
                cachedPeakLevels[ch] = peakLevel;
            }
        }

        cachedMeterChannelCount = numChannels;

        // Update peak hold indicators
        for (int ch = 0; ch < numChannels; ++ch)
        {
            // Peak hold uses peak (not VU) for accurate transient capture
            float peakDb = (cachedPeakLevels[ch] > 0.001f) ? 20.0f * std::log10(cachedPeakLevels[ch]) : -60.0f;
            float normalized = jlimit(0.0f, 1.0f, (peakDb + 60.0f) / 60.0f);

            if (normalized >= peakHoldLevels[ch])
            {
                peakHoldLevels[ch] = normalized;
                peakHoldCounters[ch] = 60; // Hold for ~2 seconds at 30fps
            }
            else if (peakHoldCounters[ch] > 0)
            {
                --peakHoldCounters[ch];
            }
            else
            {
                // Decay peak hold after hold period
                peakHoldLevels[ch] *= 0.92f;
                if (peakHoldLevels[ch] < 0.01f)
                    peakHoldLevels[ch] = 0.0f;
            }
        }

        if (needsRepaint || peakHoldLevels[0] > 0.0f || peakHoldLevels[1] > 0.0f)
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
                    float currentDb = isInput ? state.inputChannelGainDb[ch].load(std::memory_order_relaxed)
                                              : state.outputChannelGainDb[ch].load(std::memory_order_relaxed);

                    if (std::abs((float)slider->getValue() - currentDb) > 0.01f)
                        slider->setValue(currentDb, dontSendNotification);
                }
            }
        }
    }

    for (auto* control : nodeParameterControls)
    {
        if (control != nullptr)
            control->repaint();
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

    auto localEvent = e.getEventRelativeTo(this);

    if (e.mods.isPopupMenu() || shouldSuppressWholeNodeDragFrom(e.originalComponent) ||
        isStickyNoteResizeHandleEvent(pluginName, e))
        return;

    if (localEvent.y < 21 && e.getNumberOfClicks() == 2)
    {
        titleLabel->showEditor();
        return;
    }

    if (visualCategoryName == "rack" && e.getNumberOfClicks() >= 2 &&
        getEffectRackSubgraphPreviewBounds().contains(localEvent.position.toFloat()))
    {
        openPluginEditor(false); // Open rack editor from preview double-click
        return;
    }

    if (e.getNumberOfClicks() >= 2)
        return;

    beingDragged = true;
    dragX = localEvent.getPosition().getX();
    dragY = localEvent.getPosition().getY();
    toFront(true);

    // Subtle transparency during drag
    setAlpha(0.88f);
    repaint();
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

        float newX = (float)(eField.x - dragX);
        float newY = (float)(eField.y - dragY);

        // Snap to grid if enabled
        if (SettingsManager::getInstance().getBool("SnapToGrid", false))
        {
            constexpr int gridSize = 20;
            newX = (float)(((int)newX / gridSize) * gridSize);
            newY = (float)(((int)newY / gridSize) * gridSize);
        }

        // Clamp to non-negative
        if (newX < 0.0f)
            newX = 0.0f;
        if (newY < 0.0f)
            newY = 0.0f;

        // Move directly — no interpolation
        int posX = (int)newX;
        int posY = (int)newY;
        setTopLeftPosition(posX, posY);
        node->properties.set("x", posX);
        node->properties.set("y", posY);
        sendChangeMessage();
    }
}

//------------------------------------------------------------------------------
void PluginComponent::mouseUp(const MouseEvent& e)
{
    if (beingDragged)
    {
        beingDragged = false;

        // Remove visual effects
        setAlpha(1.0f);
        repaint();
        sendChangeMessage();
    }

    if (pluginWindow)
        node->properties.set("windowOpen", false);
}

//------------------------------------------------------------------------------
void PluginComponent::timerCallback()
{
    // Timer no longer used for drag interpolation
    stopTimer();
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

    // Safety: Verify button pointers are valid before comparing
    if (!button)
    {
        return;
    }
    if (!node || !node->getProcessor())
    {
        return;
    }

    if ((button == editButton) && !pluginWindow)
    {
        openPluginEditor(false); // Default to custom editor on left-click
    }

    else if (button == mappingsButton)
    {
        openMappingsWindow();
    }
    else if (button == bypassButton)
    {
        BypassableInstance* bypassable = dynamic_cast<BypassableInstance*>(node->getProcessor());

        if (bypassable)
        {
            bypassable->setBypass(bypassButton->getToggleState());
            repaint();
        }
    }
    else if (button == deleteButton)
    {
        PluginField* parent = dynamic_cast<PluginField*>(getParentComponent());

        if (pluginWindow)
        {
            pluginWindow->closeButtonPressed();
        }

        if (parent)
        {
            parent->deleteFilter(node);
            // PluginField doesn't own us via OwnedArray, so we need to delete ourselves
            delete this;
        }
        else if (auto* canvas = dynamic_cast<SubGraphCanvas*>(getParentComponent()))
        {
            canvas->deleteFilter(node);
            // SubGraphCanvas::deleteFilter() already deleted 'this' via OwnedArray.remove()
            // DO NOT call delete this here - it would be a double-delete!
            return; // 'this' is already deleted, return immediately
        }
    }
}

//------------------------------------------------------------------------------
void PluginComponent::labelTextChanged(Label* label)
{
    int i, y;
    PluginField* parent = dynamic_cast<PluginField*>(getParentComponent());

    pluginName = label->getText();
    displayName = getRackBoundaryDisplayName(node, pluginName);

    // Update processor name in main canvas (SubGraphCanvas doesn't track names)
    if (parent)
        parent->updateProcessorName(node->nodeID.uid, pluginName);

    // Reset the Component's size/layout.
    determineSize(true);
    layoutTitleLabel();
    if (deleteButton)
        deleteButton->setBounds(getWidth() - 17, 5, 12, 12);
    layoutFooterButtons();

    {
        const bool largePin = isAudioIONode();
        const int ps = largePin ? 40 : 22;
        const int psY = largePin ? 40 : 34;
        const int xRight = largePin ? (getWidth() - 8) : (getWidth() - 6);
        y = psY;
        for (i = 0; i < outputPins.size(); ++i)
        {
            outputPins[i]->setTopLeftPosition(xRight, y);
            y += ps;
        }
        for (i = 0; i < paramPins.size(); ++i)
        {
            if (paramPins[i]->getX() > 0)
            {
                paramPins[i]->setTopLeftPosition(xRight, y);
                y += 22;
            }
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
    displayName = getRackBoundaryDisplayName(node, val);
    titleLabel->setText(displayName, sendNotification);
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
    JuceHelperStuff::showModalDialog(tempstr, &dlg, getParentComponent(),
                                     ColourScheme::getInstance().colours["Dialog Background"], false, true);
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
    int w = 160;
    int h = 100;
    float x;
    float y = 22.0f;
    int numInputPins = 0;
    int numOutputPins = 0;
    PedalboardProcessor* proc = nullptr;
    Font tempFont = FontManager::getInstance().getSubheadingFont();
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
    nameText.addLineOfText(tempFont, displayName, 10.0f, y);
    // nameText.getBoundingBox(0, -1, l, t, r, b, true);
    bounds = nameText.getBoundingBox(0, -1, true);
    nameWidth = bounds.getWidth();

    // Add on space for the close button if necessary.
    if ((pluginName != "Audio Input") && (pluginName != "MIDI Input") && (pluginName != "Audio Output") &&
        (pluginName != "OSC Input") && (pluginName != "Virtual MIDI Input"))
    {
        nameWidth += 20.0f;
    }
    else
        nameWidth += 4.0f;

    inputText.clear();
    outputText.clear();

    bool showLabels = (!proc) || shouldDrawHostPinText(pluginName);
    const bool compactPinLabels = usesCompactHostPinLabels(pluginName);
    auto positionOutputTextForCurrentWidth = [&]()
    {
        const float outputRightInset = pluginName == "MIDI Input" ? 18.0f : 10.0f;
        x = (w - outputWidth - outputRightInset);

        for (i = 0; i < outputText.size(); ++i)
            outputText[i]->moveRangeOfGlyphs(0, -1, x, 0.0f);
    };

    // Use larger spacing for Audio I/O nodes (40px for VU + slider per channel)
    const float pinSpacing = isAudioIONode() ? 40.0f : 22.0f;

    if (showLabels)
    {
        int numIn = countInputChannelsFromBuses(plugin);
        int numOut = countOutputChannelsFromBuses(plugin);
        // Determine plugin input channel name bounds.
        y = 44.0f;
        tempFont.setHeight(12.0f);
        tempFont.setStyleFlags(Font::plain);
        for (i = 0; i < numIn; ++i)
        {
            // Use numbered names for Audio Output (its inputs are device output channels)
            bool useNumberedNames = compactPinLabels || ignorePinNames || (pluginName == "Audio Output");

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
                if (compactPinLabels)
                    tempstr << i + 1;
                else if (pluginName == "Audio Output")
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
        if (shouldCreateHostMidiOrParamPin(plugin, pluginName, numIn, numOut) && pluginName != "MIDI Input")
        {
            // if(!ignorePinNames)
            {
                GlyphArrangement* g = new GlyphArrangement;

                g->addLineOfText(tempFont, getMidiOrParameterPinLabel(plugin, pluginName, false), 10.0f, y);
                bounds = g->getBoundingBox(0, -1, true);

                inputText.add(g);

                if (bounds.getWidth() > inputWidth)
                    inputWidth = bounds.getWidth();
            }

            y += 18.0f;
            ++numInputPins;
        }

        // Determine plugin output channel name bounds.
        y = 44.0f;
        for (i = 0; i < numOut; ++i)
        {
            // Use numbered names for Audio Input (its outputs are device input channels)
            bool useNumberedNames = compactPinLabels || ignorePinNames || (pluginName == "Audio Input");

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
                if (compactPinLabels)
                    tempstr << i + 1;
                else if (pluginName == "Audio Input")
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
        if (producesMidiSafe(plugin) || pluginName == "MIDI Input" || (plugin->getName() == "OSC Input"))
        {
            // if(!ignorePinNames)
            {
                GlyphArrangement* g = new GlyphArrangement;

                g->addLineOfText(tempFont, getMidiOrParameterPinLabel(plugin, pluginName, true),
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
            compSize = getEmbeddedNodeControlSize(proc, pluginName);
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
            w = jmax(w, 160);
        if (isRackBoundaryNode(node))
            w = jmax(w, 140);

        h = jmax(numInputPins, numOutputPins);
        h *= (int)pinSpacing;

        float minH = (float)h + 70.0f;
        if (proc && minH < procH + (float)getEmbeddedNodeControlHeightPadding(pluginName))
            minH = procH + (float)getEmbeddedNodeControlHeightPadding(pluginName);

        if ((pluginName != "Audio Input") && (pluginName != "MIDI Input") && (pluginName != "Audio Output") &&
            (pluginName != "OSC Input"))
        {
            h = (int)minH;
        }
        else if (proc)
            h = (int)minH;

        if ((pluginName != "Audio Input") && (pluginName != "MIDI Input") && (pluginName != "Audio Output") &&
            (pluginName != "OSC Input"))
        {
            if (usesEmbeddedParameterSurface(pluginName))
                h = (int)minH;
            else
                h = jmax((int)minH, h + 70);
        }
        else
        {
            h = jmax((int)minH, h + 44);
        }
    }
    else
    {
        Point<int> compSize = proc->getSize();
        compSize = getEmbeddedNodeControlSize(proc, pluginName);

        if (isDirectPaintedEmbeddedNodeName(pluginName))
        {
            w = compSize.getX();
            h = compSize.getY();
        }
        else if (nameWidth > (compSize.getX() + 24.0f))
            w = (int)(nameWidth + 20.0f);
        else
            w = (int)(compSize.getX() + 24.0f);

        if (!isDirectPaintedEmbeddedNodeName(pluginName))
            h = compSize.getY() + getEmbeddedNodeControlHeightPadding(pluginName);
    }

    if (visualCategoryName == "rack")
    {
        const auto rackDefault = getDefaultRackNodeSize();
        const int storedWRaw = (int)node->properties.getWithDefault(kRackNodeWidthProperty, rackDefault.getX());
        const int storedHRaw = (int)node->properties.getWithDefault(kRackNodeHeightProperty, rackDefault.getY());
        const int storedW = storedWRaw > 0 ? storedWRaw : rackDefault.getX();
        const int storedH = storedHRaw > 0 ? storedHRaw : rackDefault.getY();
        w = jmax(w, jlimit(248, 620, storedW));
        h = jmax(h, jlimit(152, 520, storedH));
    }

    // Enforce matching size for MIDI input node pair
    if (pluginName == "MIDI Input" || pluginName == "Virtual MIDI Input")
    {
        // Compute common width from the longer name so both nodes are identical
        Font midiFont = FontManager::getInstance().getSubheadingFont();
        int refWidth = (int)(midiFont.getStringWidthFloat("Virtual MIDI Input") + 40.0f);
        spdlog::info("[determineSize] '{}': w={} h={} refWidth={} nameWidth={:.1f}", pluginName.toStdString(), w, h,
                     refWidth, nameWidth);
        w = jmax(w, refWidth);
        h = 92;
        spdlog::info("[determineSize] '{}': FINAL w={} h={}", pluginName.toStdString(), w, h);
    }

    if (showLabels)
        positionOutputTextForCurrentWidth();

    if (!onlyUpdateWidth && !isDirectPaintedEmbeddedNodeName(pluginName))
        h += getNodeParameterControlsHeight();

    if (onlyUpdateWidth)
        setSize(w, getHeight());
    else
        setSize(w, h);
}

//------------------------------------------------------------------------------
bool PluginComponent::isAudioIONode() const
{
    return !isRackAudioBoundaryNode(node) && ((pluginName == "Audio Input") || (pluginName == "Audio Output"));
}

//------------------------------------------------------------------------------
void PluginComponent::updateNodeSize()
{
    determineSize();
    layoutTitleLabel();

    // Reposition bottom buttons after size change
    layoutFooterButtons();

    // Reposition PedalboardProcessor control component
    auto* bypassable = dynamic_cast<BypassableInstance*>(node->getProcessor());
    auto* proc = bypassable ? dynamic_cast<PedalboardProcessor*>(bypassable->getPlugin()) : nullptr;
    if (!proc)
        proc = dynamic_cast<PedalboardProcessor*>(node->getProcessor());
    if (proc)
    {
        Point<int> compSize = proc->getSize();
        compSize = getEmbeddedNodeControlSize(proc, pluginName);
        for (int ci = 0; ci < getNumChildComponents(); ++ci)
        {
            auto* child = getChildComponent(ci);
            if (dynamic_cast<PluginPinComponent*>(child) != nullptr)
                continue;
            if (child == titleLabel || child == editButton || child == mappingsButton || child == bypassButton ||
                child == deleteButton)
                continue;
            if (dynamic_cast<Slider*>(child) != nullptr)
                continue;
            if (dynamic_cast<NodeParameterMiniControl*>(child) != nullptr)
                continue;
            int cx = getEmbeddedNodeControlLeftOffset(getWidth(), compSize, pluginName);
            child->setBounds(cx, getEmbeddedNodeControlTopOffset(pluginName), compSize.getX(), compSize.getY());
            break;
        }
    }

    layoutFooterButtons();

    createPins();
    rebuildNodeParameterControls();
    layoutNodeParameterControls();
    repaint();
}

//------------------------------------------------------------------------------
void PluginComponent::refreshNodeParameterControls()
{
    determineSize();
    layoutTitleLabel();

    layoutFooterButtons();

    rebuildNodeParameterControls();
    layoutNodeParameterControls();
    repaint();
    sendChangeMessage();
}

//------------------------------------------------------------------------------
int PluginComponent::getNodeParameterControlCount() const
{
    if (!areNodeParameterControlsEnabled() || isAudioIONode() || node == nullptr)
        return 0;

    if (isDirectPaintedEmbeddedNodeName(pluginName) || usesEmbeddedParameterSurface(pluginName))
        return 0;

    const auto excludedSystemNode =
        pluginName == "MIDI Input" || pluginName == "OSC Input" || pluginName == "Virtual MIDI Input";
    if (excludedSystemNode)
        return 0;

    return countPreviewParameters(node->getProcessor());
}

//------------------------------------------------------------------------------
int PluginComponent::getNodeParameterControlsHeight() const
{
    const int count = getNodeParameterControlCount();
    if (count <= 0)
        return 0;

    return kNodeParameterControlVerticalPadding + count * kNodeParameterControlHeight +
           (count - 1) * kNodeParameterControlGap;
}

//------------------------------------------------------------------------------
void PluginComponent::rebuildNodeParameterControls()
{
    for (auto* control : nodeParameterControls)
        removeChildComponent(control);
    nodeParameterControls.clear(true);

    const int count = getNodeParameterControlCount();
    if (count <= 0)
        return;

    auto* processor = node != nullptr ? node->getProcessor() : nullptr;
    for (int i = 0; i < count; ++i)
    {
        if (auto* parameter = getPreviewParameter(processor, i))
        {
            auto* control = new NodeParameterMiniControl(*parameter);
            addAndMakeVisible(control);
            nodeParameterControls.add(control);
        }
    }

    for (auto* control : nodeParameterControls)
    {
        if (control != nullptr)
            control->repaint();
    }
}

//------------------------------------------------------------------------------
void PluginComponent::layoutNodeParameterControls()
{
    const int count = nodeParameterControls.size();
    if (count <= 0)
        return;

    const int totalHeight = count * kNodeParameterControlHeight + (count - 1) * kNodeParameterControlGap;
    int y = getHeight() - 38 - totalHeight;
    const int x = 26;
    const int width = jmax(74, getWidth() - 52);

    for (auto* control : nodeParameterControls)
    {
        if (control != nullptr)
        {
            control->setBounds(x, y, width, kNodeParameterControlHeight);
            y += kNodeParameterControlHeight + kNodeParameterControlGap;
        }
    }
}

//------------------------------------------------------------------------------
void PluginComponent::refreshPins()
{
    // Before deleting old pins, remove any PluginConnection cables that reference
    // them. Otherwise the cable objects hold dangling pointers and crash when
    // accessed (e.g. toggling mono/stereo on a mixer strip with cables attached).
    uint32 myNodeId = node->nodeID.uid;
    if (auto* parentCanvas = getParentComponent())
    {
        // Get the FilterGraph for removing graph-level connections
        FilterGraph* filterGraph = nullptr;
        if (auto* field = dynamic_cast<PluginField*>(parentCanvas))
            filterGraph = field->getFilterGraph();

        for (int i = parentCanvas->getNumChildComponents() - 1; i >= 0; --i)
        {
            if (auto* conn = dynamic_cast<PluginConnection*>(parentCanvas->getChildComponent(i)))
            {
                const auto* src = conn->getSource();
                const auto* dst = conn->getDestination();
                bool touchesMe = (src && src->getUid() == myNodeId) || (dst && dst->getUid() == myNodeId);
                if (touchesMe)
                {
                    // Remove graph connection (safe even if already removed)
                    if (src && dst && filterGraph)
                    {
                        filterGraph->removeConnection(AudioProcessorGraph::NodeID(src->getUid()), src->getChannel(),
                                                      AudioProcessorGraph::NodeID(dst->getUid()), dst->getChannel());
                    }
                    parentCanvas->removeChildComponent(conn);
                    delete conn;
                }
            }
        }
    }

    // Remove and delete all existing pins
    for (auto* pin : inputPins)
    {
        removeChildComponent(pin);
        delete pin;
    }
    for (auto* pin : outputPins)
    {
        removeChildComponent(pin);
        delete pin;
    }
    for (auto* pin : paramPins)
    {
        removeChildComponent(pin);
        delete pin;
    }

    inputPins.clear();
    outputPins.clear();
    paramPins.clear();

    // Resync the BypassableInstance wrapper's channel count before recalculating
    // size and pins. Dynamic PedalboardProcessors (Mixer/Splitter) update their
    // real bus layout first; createPins() must see that current layout.
    if (auto* bypassable = dynamic_cast<BypassableInstance*>(node->getProcessor()))
    {
        if (dynamic_cast<PedalboardProcessor*>(bypassable->getPlugin()))
            bypassable->resyncChannelCount();
    }

    // Remove existing gain sliders (Audio I/O nodes)
    for (auto* slider : channelGainSliders)
        removeChildComponent(slider);
    channelGainSliders.clear(true);

    // Recalculate size and recreate pins
    determineSize();
    layoutTitleLabel();
    createPins();

    // Reposition bottom buttons after size change (prevents clipping by growing controls)
    layoutFooterButtons();

    // Reposition the internal PedalboardProcessor control component if present
    // (mirrors the positioning logic in the constructor)
    auto* innerProc = dynamic_cast<PedalboardProcessor*>(node->getProcessor());
    if (innerProc == nullptr)
    {
        if (auto* bypassable = dynamic_cast<BypassableInstance*>(node->getProcessor()))
            innerProc = dynamic_cast<PedalboardProcessor*>(bypassable->getPlugin());
    }

    if (auto* proc = innerProc)
    {
        Point<int> compSize = proc->getSize();
        compSize = getEmbeddedNodeControlSize(proc, pluginName);
        // Find the control component among our children and reposition it
        for (int ci = 0; ci < getNumChildComponents(); ++ci)
        {
            auto* child = getChildComponent(ci);
            // Skip pins, buttons, labels, sliders - the control is the large internal component
            if (dynamic_cast<PluginPinComponent*>(child) != nullptr)
                continue;
            if (child == titleLabel || child == editButton || child == mappingsButton || child == bypassButton ||
                child == deleteButton)
                continue;
            if (dynamic_cast<Slider*>(child) != nullptr)
                continue;
            if (dynamic_cast<NodeParameterMiniControl*>(child) != nullptr)
                continue;
            // This should be the PedalboardProcessor's control component
            int cx = getEmbeddedNodeControlLeftOffset(getWidth(), compSize, pluginName);
            child->setBounds(cx, getEmbeddedNodeControlTopOffset(pluginName), compSize.getX(), compSize.getY());
            break;
        }
    }

    // Recreate per-channel gain sliders for Audio I/O nodes
    if (isAudioIONode())
    {
        AudioProcessor* plugin = node->getProcessor();
        bool isInput = (pluginName == "Audio Input");
        int numCh = isInput ? countOutputChannelsFromBuses(plugin) : countInputChannelsFromBuses(plugin);
        auto& state = MasterGainState::getInstance();

        const float meterStartY = 44.0f;
        const float pinSpacing = 40.0f;
        const int sliderHeight = 18;
        const int pinMargin = 22;
        const int edgeMargin = 8;
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

            int sliderY = (int)(meterStartY + ch * pinSpacing + 10.0f);
            int sliderX = isInput ? edgeMargin : pinMargin;
            slider->setBounds(sliderX, sliderY, sliderW, sliderHeight);

            float initDb = isInput ? state.inputChannelGainDb[ch].load(std::memory_order_relaxed)
                                   : state.outputChannelGainDb[ch].load(std::memory_order_relaxed);
            slider->setValue(initDb, dontSendNotification);

            addAndMakeVisible(slider);
            channelGainSliders.add(slider);
        }
    }

    rebuildNodeParameterControls();
    layoutNodeParameterControls();
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

    // Check for PedalboardProcessor custom pin layout (mixer/splitter alignment)
    PedalboardProcessor::PinLayout inputLayout;
    PedalboardProcessor::PinLayout outputLayout;
    bool isPedalboardProc = false;

    if (auto* bypassable = dynamic_cast<BypassableInstance*>(plugin))
    {
        if (auto* pbProc = dynamic_cast<PedalboardProcessor*>(bypassable->getPlugin()))
        {
            inputLayout = pbProc->getInputPinLayout();
            outputLayout = pbProc->getOutputPinLayout();
            isPedalboardProc = true;
        }
    }
    else if (auto* pbProc = dynamic_cast<PedalboardProcessor*>(plugin))
    {
        inputLayout = pbProc->getInputPinLayout();
        outputLayout = pbProc->getOutputPinLayout();
        isPedalboardProc = true;
    }

    // Fallback generation if empty (for standard plugins or when layout not provided)
    if (inputLayout.pinY.empty())
    {
        int startY = largePin ? 40 : 34;
        int spacing = largePin ? 40 : 22;
        for (int k = 0; k < 256; ++k)
            inputLayout.pinY.push_back(startY + k * spacing);
    }
    if (outputLayout.pinY.empty())
    {
        int startY = largePin ? 40 : 34;
        int spacing = largePin ? 40 : 22;
        for (int k = 0; k < 256; ++k)
            outputLayout.pinY.push_back(startY + k * spacing);
    }

    const int pinXOffset = largePin ? -10 : -8;
    const int pinXOffsetRight = largePin ? (getWidth() - 8) : (getWidth() - 6);

    // Setup Input Pins
    int numIn = countInputChannelsFromBuses(plugin);
    for (i = 0; i < numIn; ++i)
    {
        Point<int> pinPos;

        pin = new PluginPinComponent(false, uid, i, false, largePin);
        pin->setTooltip(getInputChannelNameSafe(plugin, i));

        // Use layout or extrapolate
        if (i < (int)inputLayout.pinY.size())
            y = inputLayout.pinY[i];
        else
            y = inputLayout.pinY.back() + (i - inputLayout.pinY.size() + 1) * 22;

        pinPos.setXY(pinXOffset, y);
        pin->setTopLeftPosition(pinPos.getX(), pinPos.getY());
        addAndMakeVisible(pin);

        inputPins.add(pin);
    }

    // Determine Y passed the last input pin for the param pin
    if (numIn < (int)inputLayout.pinY.size())
        y = inputLayout.pinY[numIn];
    else
        y = inputLayout.pinY.back() + (numIn - inputLayout.pinY.size() + 1) * 22;

    int numOut = countOutputChannelsFromBuses(plugin);

    if (shouldCreateHostMidiOrParamPin(plugin, pluginName, numIn, numOut) && pluginName != "MIDI Input")
    {
        Point<int> pinPos;

        pin = new PluginPinComponent(false, uid, AudioProcessorGraph::midiChannelIndex, true);
        pin->setTooltip("MIDI In");

        // For PedalboardProcessors (mixer/splitter), place MIDI pin at bottom-left
        // so it doesn't float after the last audio pin
        int midiY = isPedalboardProc ? (getHeight() - 40) : y;
        pinPos.setXY(-8, midiY);
        pin->setTopLeftPosition(pinPos.getX(), pinPos.getY());
        addAndMakeVisible(pin);

        paramPins.add(pin);

        y += 22;
    }

    // Setup Output Pins
    for (i = 0; i < numOut; ++i)
    {
        Point<int> pinPos;

        pin = new PluginPinComponent(true, uid, i, false, largePin);
        pin->setTooltip(getOutputChannelNameSafe(plugin, i));

        // Use layout or extrapolate
        if (i < (int)outputLayout.pinY.size())
            y = outputLayout.pinY[i];
        else
            y = outputLayout.pinY.back() + (i - outputLayout.pinY.size() + 1) * 22;

        pinPos.setXY(pinXOffsetRight, y);
        pin->setTopLeftPosition(pinPos.getX(), pinPos.getY());
        addAndMakeVisible(pin);

        outputPins.add(pin);
    }

    // Determine Y passed the last output pin for the param/MIDI out pin
    if (numOut < (int)outputLayout.pinY.size())
        y = outputLayout.pinY[numOut];
    else
        y = outputLayout.pinY.back() + (numOut - outputLayout.pinY.size() + 1) * 22;

    if (producesMidiSafe(plugin) || pluginName == "MIDI Input" || (plugin->getName() == "OSC Input"))
    {
        Point<int> pinPos;

        pin = new PluginPinComponent(true, uid, AudioProcessorGraph::midiChannelIndex, true);
        pin->setTooltip("MIDI Out");
        pinPos.setXY(getWidth() - 6, y);
        pin->setTopLeftPosition(pinPos.getX(), pinPos.getY());
        addAndMakeVisible(pin);

        paramPins.add(pin);

        y += 22;
    }
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
PluginPinComponent::PluginPinComponent(bool dir, uint32 id, int chan, bool param, bool large)
    : Component(), direction(dir), uid(id), channel(chan), parameterPin(param), largePin(large)
{
    setRepaintsOnMouseActivity(true);

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
    const bool audioPin = !parameterPin;
    auto pinBounds = Rectangle<float>(1.0f, 1.0f, w, h);
    const float pinCorner = largePin ? 4.0f : 3.0f;
    auto& colours = ColourScheme::getInstance().colours;

    // Get base color
    Colour ownerAccent = colours["Audio Connection"];
    if (auto* owner = dynamic_cast<PluginComponent*>(getParentComponent()))
        ownerAccent = owner->getVisualAccentColour();

    Colour baseColour = parameterPin ? colours["Parameter Connection"].interpolatedWith(ownerAccent, 0.18f)
                                     : ownerAccent.interpolatedWith(colours["Audio Connection"], 0.35f);

    // === Hover glow (melatonin_blur) ===
    if (isMouseOver())
    {
        Path pinShape;
        if (audioPin)
            pinShape.addRoundedRectangle(pinBounds.expanded(1.0f), pinCorner);
        else
            pinShape.addEllipse(0.0f, 0.0f, (float)getWidth(), (float)getHeight());

        melatonin::DropShadow pinGlow{baseColour.withAlpha(0.32f), 6, {0, 0}};
        pinGlow.render(g, pinShape);
    }

    auto socketBounds = pinBounds.expanded(1.0f);
    g.setColour(colours["Window Background"].withAlpha(0.58f));
    if (audioPin)
        g.fillRoundedRectangle(socketBounds, pinCorner + 1.0f);
    else
        g.fillEllipse(socketBounds);

    g.setColour(colours["Plugin Border"].interpolatedWith(baseColour, 0.22f).withAlpha(0.72f));
    if (audioPin)
        g.drawRoundedRectangle(socketBounds.reduced(0.5f), pinCorner + 1.0f, 1.0f);
    else
        g.drawEllipse(socketBounds.reduced(0.5f), 1.0f);

    // === 3D Gradient body ===
    ColourGradient sphereGrad(baseColour.brighter(0.4f), cx - radius * 0.3f, cy - radius * 0.3f,
                              baseColour.darker(0.3f), cx + radius * 0.5f, cy + radius * 0.5f, true);
    g.setGradientFill(sphereGrad);
    if (audioPin)
        g.fillRoundedRectangle(pinBounds, pinCorner);
    else
        g.fillEllipse(pinBounds);

    // === Highlight for gloss effect ===
    g.setColour(baseColour.contrasting(0.25f));
    if (audioPin)
    {
        auto highlightBounds = pinBounds.reduced(3.0f, 3.0f);
        highlightBounds = highlightBounds.removeFromTop(jmax(2.0f, h * 0.26f));
        g.fillRoundedRectangle(highlightBounds, 1.5f);
    }
    else
        g.fillEllipse(cx - radius * 0.5f, cy - radius * 0.6f, radius * 0.6f, radius * 0.4f);

    // === Border ===
    g.setColour(baseColour.darker(0.5f));
    if (audioPin)
        g.drawRoundedRectangle(pinBounds, pinCorner, 1.5f);
    else
        g.drawEllipse(pinBounds, 1.5f);

    // === Direction indicator (chevron) ===
    Path chevron;
    const float chevronSize = radius * 0.5f;
    g.setColour(baseColour.contrasting(0.8f));

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
    const bool isEffectRackEditor = c->getNode()->getProcessor()->getName() == "Effect Rack";
    if (isEffectRackEditor)
    {
        setResizable(true, true);
    }
    else if ((c->getNode()->getProcessor()->getName() != "VAZPlusVSTi") &&
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
    if (component && component->getNode())
    {
        component->getNode()->properties.set("uiLastX", getX());
        component->getNode()->properties.set("uiLastY", getY());
        // Clear the pluginWindow reference so the edit button works again
        component->setWindow(0);
    }
}

//------------------------------------------------------------------------------
void PluginEditorWindow::closeButtonPressed()
{
    if (component)
    {
        component->setWindow(0);
    }
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
    // Since we use createEditor() (not createEditorIfNeeded()), the caller owns
    // the editor and must delete it. The old comment was incorrect - we MUST delete
    // the editor here, otherwise the plugin won't be able to create a new one.
    if (editor)
    {
        removeChildComponent(editor);
        delete editor;
        editor = nullptr;
    }
    delete presetBar;
    presetBar = nullptr;
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
