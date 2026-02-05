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
    Colour cableColour = paramCon ? colours["Parameter Connection"] : colours["Audio Connection"];

    auto bounds = getLocalBounds().toFloat();

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
            g.strokePath(glowPath, PathStrokeType(strokeWidth, PathStrokeType::mitered, PathStrokeType::rounded));
        }
    }
    */

    // === Gradient fill from source to destination (bidirectional) ===
    Colour startCol = cableColour.brighter(selected ? 0.6f : 0.25f);
    Colour endCol = cableColour.darker(selected ? 0.0f : 0.15f);

    // Use actual start/end points from the bezier curve for proper bidirectional gradient
    Point<float> gradStart = glowPath.getBounds().getTopLeft();
    Point<float> gradEnd = glowPath.getBounds().getBottomRight();

    ColourGradient wireGrad(startCol, gradStart.x, gradStart.y, endCol, gradEnd.x, gradEnd.y, false);
    g.setGradientFill(wireGrad);
    g.fillPath(drawnCurve);

    // === Thin highlight stroke for depth ===
    g.setColour(Colours::white.withAlpha(0.12f));
    g.strokePath(glowPath, PathStrokeType(1.0f, PathStrokeType::mitered, PathStrokeType::rounded));
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

    if (drawnCurve.contains((float)x, (float)y))
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

    if (source && destination && parentCanvas)
    {
        Point<int> sourcePoint(source->getX() + 7, source->getY() + 8);
        Point<int> destPoint(destination->getX() + 7, destination->getY() + 8);
        sourcePoint = parentCanvas->getLocalPoint(source->getParentComponent(), sourcePoint);
        destPoint = parentCanvas->getLocalPoint(destination->getParentComponent(), destPoint);

        if (destPoint.getX() > sourcePoint.getX())
            updateBounds(sourcePoint.getX(), sourcePoint.getY(), destPoint.getX(), destPoint.getY());
        else
            updateBounds(destPoint.getX(), destPoint.getY(), sourcePoint.getX(), sourcePoint.getY());
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

    // Calculate bounding rectangle with 5px padding (JUCE uses 4, we use 5 for thicker cables)
    auto p1 = Point<float>((float)sX, (float)sY);
    auto p2 = Point<float>((float)dX, (float)dY);

    auto newBounds = Rectangle<float>(p1, p2).expanded(5.0f).getSmallestIntegerContainer();

    // Set bounds - JUCE allows negative component positions
    setBounds(newBounds);

    // Convert to local coordinates by subtracting component position
    // This is the key JUCE pattern: p -= getPosition().toFloat()
    auto pos = getPosition().toFloat();
    p1 -= pos;
    p2 -= pos;

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

    // Create stroked path for hit testing and rendering
    PathStrokeType drawnType(9.0f, PathStrokeType::mitered, PathStrokeType::rounded);
    drawnType.createStrokedPath(drawnCurve, tempPath);
}
