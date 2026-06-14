//	PluginConnection.cpp - Connection cable between two plugin pins.
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

#include "ColourScheme.h"
#include "PluginComponent.h"
#include "PluginField.h"
#include "SubGraphEditorComponent.h"

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
PluginConnection::PluginConnection(PluginPinComponent* s, PluginPinComponent* d, bool allOutputs)
    : Component(), source(s), selected(false), representsAllOutputs(allOutputs)
{
    if (source)
    {
        Point<int> tempPoint(source->getX() + 7, source->getY() + 8);

        // Find the parent canvas (either PluginField or SubGraphCanvas)
        Component* parentCanvas = source->findParentComponentOfClass<PluginField>();
        if (!parentCanvas)
            parentCanvas = source->findParentComponentOfClass<SubGraphCanvas>();

        if (parentCanvas)
            tempPoint = parentCanvas->getLocalPoint(source->getParentComponent(), tempPoint);

        setTopLeftPosition(tempPoint.getX(), tempPoint.getY());

        ((PluginComponent*)source->getParentComponent())->addChangeListener(this);

        paramCon = source->getParameterPin();
    }

    if (d)
        setDestination(d);
    else
        destination = 0;
}

//------------------------------------------------------------------------------
PluginConnection::~PluginConnection()
{
    if (source)
    {
        PluginComponent* sourceComp = dynamic_cast<PluginComponent*>(source->getParentComponent());
        if (sourceComp)
            sourceComp->removeChangeListener(this);
    }
    if (destination)
    {
        PluginComponent* destComp = dynamic_cast<PluginComponent*>(destination->getParentComponent());
        if (destComp)
            destComp->removeChangeListener(this);
    }
}

//------------------------------------------------------------------------------
void PluginConnection::paint(Graphics& g)
{
    auto& colours = ColourScheme::getInstance().colours;
    Colour sourceAccent = colours["Audio Connection"];
    Colour destAccent = colours["Audio Connection"];

    if (auto* sourceComp = source != nullptr ? dynamic_cast<PluginComponent*>(source->getParentComponent()) : nullptr)
        sourceAccent = sourceComp->getVisualAccentColour();
    if (auto* destComp =
            destination != nullptr ? dynamic_cast<PluginComponent*>(destination->getParentComponent()) : nullptr)
        destAccent = destComp->getVisualAccentColour();

    Colour cableColour = paramCon ? colours["Parameter Connection"]
                                  : sourceAccent.interpolatedWith(destAccent, 0.48f)
                                        .interpolatedWith(colours["Audio Connection"], 0.24f);

    // === Signal-based glow (DISABLED - low priority, potentially distracting) ===
    // TODO: Re-enable when true per-connection signal detection is implemented
    /*
    bool showGlow = false;
    if (!paramCon) // Only audio cables get glow
    {
        if (auto* pluginField = dynamic_cast<PluginField*>(getParentComponent()))
        {
            if (auto* filterGraph = pluginField->getFilterGraph())
            {
                showGlow = filterGraph->isAudioPlaying();
            }
        }
    }

    if (showGlow)
    {
        for (int i = 3; i >= 1; --i)
        {
            float strokeWidth = 8.0f + (i * 3.0f);
            float alpha = 0.06f / (float)i;
            g.setColour(cableColour.withAlpha(alpha));
            g.strokePath(glowPath, PathStrokeType(strokeWidth, PathStrokeType::curved, PathStrokeType::rounded));
        }
    }
    */

    // === Cable bed, glow, and wire ===
    const float outerGlowWidth = selected ? 11.0f : 7.5f;
    const float innerGlowWidth = selected ? 6.5f : 4.5f;

    g.setColour(cableColour.withAlpha(selected ? 0.12f : 0.045f));
    g.strokePath(glowPath, PathStrokeType(outerGlowWidth, PathStrokeType::curved, PathStrokeType::rounded));

    g.setColour(cableColour.withAlpha(selected ? 0.18f : 0.07f));
    g.strokePath(glowPath, PathStrokeType(innerGlowWidth, PathStrokeType::curved, PathStrokeType::rounded));

    // === Gradient fill from source to destination (bidirectional) ===
    Colour startCol = paramCon ? colours["Parameter Connection"].brighter(selected ? 0.55f : 0.24f)
                               : sourceAccent.brighter(selected ? 0.55f : 0.24f);
    Colour endCol = paramCon ? colours["Parameter Connection"].darker(selected ? 0.0f : 0.08f)
                             : destAccent.brighter(selected ? 0.30f : 0.06f);

    ColourGradient wireGrad(startCol, gradientStart.x, gradientStart.y, endCol, gradientEnd.x, gradientEnd.y, false);
    g.setGradientFill(wireGrad);
    g.fillPath(drawnCurve);

    // === Thin highlight stroke for depth ===
    g.setColour(ColourScheme::getInstance().colours["Text Colour"].withAlpha(0.12f));
    g.strokePath(glowPath, PathStrokeType(1.0f, PathStrokeType::curved, PathStrokeType::rounded));

    if (selected && destination != nullptr)
    {
        const float pathLength = glowPath.getLength();
        Point<float> mid = glowPath.getPointAlongPath(pathLength * 0.5f);
        auto bubble = Rectangle<float>(mid.x - 7.0f, mid.y - 7.0f, 14.0f, 14.0f);
        g.setColour(colours["Window Background"].withAlpha(0.86f));
        g.fillEllipse(bubble);
        g.setColour(cableColour.withAlpha(0.90f));
        g.drawEllipse(bubble, 1.2f);

        auto xBounds = bubble.reduced(4.2f);
        g.drawLine(xBounds.getX(), xBounds.getY(), xBounds.getRight(), xBounds.getBottom(), 1.4f);
        g.drawLine(xBounds.getRight(), xBounds.getY(), xBounds.getX(), xBounds.getBottom(), 1.4f);
    }
}

//------------------------------------------------------------------------------
void PluginConnection::mouseDown(const MouseEvent& e)
{
    if (e.mods.isPopupMenu()) // Right-click
    {
        // Select this connection and trigger deletion
        selected = true;

        // Try PluginField first (main canvas)
        if (PluginField* field = findParentComponentOfClass<PluginField>())
        {
            field->deleteConnection();
            // Don't touch 'this' after deleteConnection() - we've been deleted!
            return;
        }

        // Fallback to SubGraphCanvas (Effect Rack)
        if (SubGraphCanvas* canvas = findParentComponentOfClass<SubGraphCanvas>())
        {
            canvas->deleteConnection(this);
            // Don't touch 'this' after deleteConnection() - we've been deleted!
            return;
        }
    }
    else // Left-click
    {
        selected = !selected;
        repaint();
    }
}

//------------------------------------------------------------------------------
bool PluginConnection::hitTest(int x, int y)
{
    bool retval = false;

    if (hitCurve.contains((float)x, (float)y))
    {
        // Make sure clicking the source pin doesn't select this connection.
        if (x > 10)
            retval = true;
    }

    return retval;
}

//------------------------------------------------------------------------------
void PluginConnection::changeListenerCallback(ChangeBroadcaster* changedObject)
{
    Component* field = getParentComponent();

    if (source && destination)
    {
        Point<int> sourcePoint(source->getX() + 7, source->getY() + 8);
        Point<int> destPoint(destination->getX() + 7, destination->getY() + 8);
        sourcePoint = field->getLocalPoint(source->getParentComponent(), sourcePoint);
        destPoint = field->getLocalPoint(destination->getParentComponent(), destPoint);

        updateBounds(sourcePoint.getX(), sourcePoint.getY(), destPoint.getX(), destPoint.getY());
    }
}

//------------------------------------------------------------------------------
void PluginConnection::drag(int x, int y)
{
    Component* field = getParentComponent();

    if (source)
    {
        Point<int> sourcePoint(source->getX() + 7, source->getY() + 8);
        sourcePoint = field->getLocalPoint(source->getParentComponent(), sourcePoint);

        updateBounds(sourcePoint.getX(), sourcePoint.getY(), x, y);
    }
}

//------------------------------------------------------------------------------
void PluginConnection::setDestination(PluginPinComponent* d)
{
    // Find the parent canvas (either PluginField or SubGraphCanvas)
    Component* parentCanvas = source->findParentComponentOfClass<PluginField>();
    if (!parentCanvas)
        parentCanvas = source->findParentComponentOfClass<SubGraphCanvas>();

    destination = d;
    if (destination)
        ((PluginComponent*)destination->getParentComponent())->addChangeListener(this);

    if (source && destination)
        setTooltip(String(paramCon ? "MIDI" : "Audio") + " connection");

    if (source && destination && parentCanvas)
    {
        Point<int> sourcePoint(source->getX() + 7, source->getY() + 8);
        Point<int> destPoint(destination->getX() + 7, destination->getY() + 8);
        sourcePoint = parentCanvas->getLocalPoint(source->getParentComponent(), sourcePoint);
        destPoint = parentCanvas->getLocalPoint(destination->getParentComponent(), destPoint);

        updateBounds(sourcePoint.getX(), sourcePoint.getY(), destPoint.getX(), destPoint.getY());
    }
}

//------------------------------------------------------------------------------
void PluginConnection::setRepresentsAllOutputs(bool val)
{
    representsAllOutputs = val;
}

//------------------------------------------------------------------------------
void PluginConnection::getPoints(int& sX, int& sY, int& dX, int& dY)
{
    int tX, tY;

    if (dY < sY)
    {
        if (sX < dX)
        {
            dX -= sX;
            sX = 5;
            dX += 5;

            tX = dX;
            dX = sX;
            sX = tX;
        }
        else
        {
            sX -= dX;
            dX = 5;
            sX += 5;

            tX = dX;
            dX = sX;
            sX = tX;
        }

        sY -= dY;
        dY = 5;
        sY += 5;

        tY = dY;
        dY = sY;
        sY = tY;
    }
    else if (sX < dX)
    {
        dX -= sX;
        sX = 5;
        dX += 5;

        dY -= sY;
        sY = 5;
        dY += 5;
    }
    else
    {
        sX -= dX;
        dX = 5;
        sX += 5;

        tX = dX;
        dX = sX;
        sX = tX;

        sY -= dY;
        dY = 5;
        sY += 5;

        tY = dY;
        dY = sY;
        sY = tY;
    }
}

//------------------------------------------------------------------------------
void PluginConnection::updateBounds(int sX, int sY, int dX, int dY)
{
    // JUCE AudioPluginHost pattern:
    // 1. Calculate bounds from raw parent coordinates (allow negative)
    // 2. Set bounds
    // 3. Build path by subtracting getPosition() for local coords

    // Calculate bounding rectangle with padding for the visible cable halo.
    auto p1 = Point<float>((float)sX, (float)sY);
    auto p2 = Point<float>((float)dX, (float)dY);

    auto newBounds = Rectangle<float>(p1, p2).expanded(18.0f).getSmallestIntegerContainer();

    // Set bounds - JUCE allows negative component positions
    setBounds(newBounds);

    // Convert to local coordinates by subtracting component position
    // This is the key JUCE pattern: p -= getPosition().toFloat()
    auto pos = getPosition().toFloat();
    p1 -= pos;
    p2 -= pos;
    gradientStart = p1;
    gradientEnd = p2;

    // Build the bezier curve in local coordinates
    // Using JUCE's vertical curve style: control points at 0.33 and 0.66 of height
    Path tempPath;
    tempPath.startNewSubPath(p1);

    // Horizontal bezier (our existing style) - control points at half width
    float halfWidth = std::abs(p2.x - p1.x) * 0.5f;
    float minX = jmin(p1.x, p2.x);
    tempPath.cubicTo(minX + halfWidth, p1.y, minX + halfWidth, p2.y, p2.x, p2.y);

    // Store for glow rendering
    glowPath = tempPath;

    // Create separate stroked paths for rendering and forgiving hit testing.
    PathStrokeType drawnType(4.4f, PathStrokeType::curved, PathStrokeType::rounded);
    drawnType.createStrokedPath(drawnCurve, tempPath);
    PathStrokeType hitType(18.0f, PathStrokeType::curved, PathStrokeType::rounded);
    hitType.createStrokedPath(hitCurve, tempPath);
}
