//	VuMeterEditors.cpp - VU Meter control and editor implementations.
//	----------------------------------------------------------------------------
//	This file is part of Pedalboard3, an audio plugin host.
//	Copyright (c) 2011 Niall Moody.
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
#include "FontManager.h"
#include "PedalboardProcessorEditors.h"
#include "PedalboardProcessors.h"

#include <cmath>

using namespace std;

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
VuMeterControl::VuMeterControl(VuMeterProcessor* proc) : processor(proc)
{
    startTimer(60);

    setSize(proc != nullptr ? proc->getSize().getX() : 180, proc != nullptr ? proc->getSize().getY() : 190);
}

//------------------------------------------------------------------------------
VuMeterControl::~VuMeterControl() {}

//------------------------------------------------------------------------------
void VuMeterControl::paint(Graphics& g)
{
    auto& colours = ColourScheme::getInstance().colours;
    auto bounds = getLocalBounds().toFloat().reduced(4.0f);
    drawChromeShell(g, bounds);

    auto content = bounds.reduced(10.0f, 9.0f);
    auto header = content.removeFromTop(42.0f);
    auto glyph = header.removeFromLeft(38.0f).reduced(1.0f);
    drawMeterGlyph(g, glyph);

    auto status = header.removeFromRight(44.0f).withHeight(20.0f);
    status = status.withCentre({status.getCentreX(), header.getCentreY()});

    auto titleArea = header.reduced(4.0f, 0.0f);
    g.setColour(colours["Text Colour"].withAlpha(0.92f));
    g.setFont(FontManager::getInstance().getUIFont(16.0f, true));
    g.drawFittedText("VU METER", titleArea.toNearestInt(), Justification::centredLeft, 1);

    const auto hottestPeakDb = jmax(leftMeter.peakDb, rightMeter.peakDb);
    drawVuMeterValuePill(g, status, hottestPeakDb > -58.0f ? "LIVE" : "IDLE",
                         hottestPeakDb >= -0.1f ? colours["VU Meter Over Colour"] : colours["Graph Category Meter"]);

    content.removeFromTop(5.0f);
    auto meterArea = content.removeFromTop(112.0f);
    auto leftColumn = meterArea.removeFromLeft(46.0f);
    auto scale = meterArea.removeFromLeft(58.0f);
    auto rightColumn = meterArea.removeFromLeft(46.0f);
    drawVuMeterColumn(g, leftColumn, leftMeter, "L");
    drawVuMeterScale(g, scale.reduced(0.0f, 15.0f));
    drawVuMeterColumn(g, rightColumn, rightMeter, "R");

    content.removeFromTop(5.0f);
    auto footer = content.removeFromTop(21.0f);
    drawVuMeterValuePill(g, footer.removeFromLeft(76.0f), "PK " + formatDb(jmax(leftMeter.peakDb, rightMeter.peakDb)),
                         colours["VU Meter Upper Colour"]);
    drawVuMeterValuePill(g, footer.removeFromRight(76.0f), "RMS " + formatDb(jmax(leftMeter.rmsDb, rightMeter.rmsDb)),
                         colours["VU Meter Lower Colour"]);
}

//------------------------------------------------------------------------------
void VuMeterControl::resized() {}

//------------------------------------------------------------------------------
void VuMeterControl::timerCallback()
{
    if (processor)
    {
        auto updateSnapshot = [](MeterSnapshot& snapshot, float peak, float rms, float vu, bool clipped)
        {
            snapshot.peakDb = amplitudeToDb(peak);
            snapshot.rmsDb = amplitudeToDb(rms);
            snapshot.vuDb = amplitudeToDb(vu);
            if (clipped)
                snapshot.clipHoldFrames = 36;
            else if (snapshot.clipHoldFrames > 0)
                --snapshot.clipHoldFrames;
        };

        updateSnapshot(leftMeter, processor->getLeftLevel(), processor->getLeftRmsLevel(), processor->getLeftVuLevel(),
                       processor->getLeftAndClearClip());
        updateSnapshot(rightMeter, processor->getRightLevel(), processor->getRightRmsLevel(), processor->getRightVuLevel(),
                       processor->getRightAndClearClip());

        repaint();
    }
}

//------------------------------------------------------------------------------
void VuMeterControl::drawChromeShell(Graphics& g, Rectangle<float> bounds)
{
    auto& colours = ColourScheme::getInstance().colours;
    const auto background = colours["Plugin Background"];
    const auto border = colours["Plugin Border"];
    const auto accent = colours["Graph Category Meter"];

    ColourGradient shell(background.brighter(0.16f), bounds.getTopLeft(), background.darker(0.36f),
                         bounds.getBottomRight(), false);
    shell.addColour(0.22, background.brighter(0.08f));
    shell.addColour(0.74, background.darker(0.18f));
    g.setGradientFill(shell);
    g.fillRoundedRectangle(bounds, 8.0f);

    g.setColour(accent.withAlpha(0.22f));
    g.fillRoundedRectangle(bounds.withHeight(36.0f), 8.0f);

    g.setColour(accent.withAlpha(0.72f));
    g.drawRoundedRectangle(bounds.reduced(0.5f), 8.0f, 1.2f);
    g.setColour(border.withAlpha(0.34f));
    g.drawRoundedRectangle(bounds.reduced(2.0f), 6.0f, 0.8f);

    g.setColour(accent.withAlpha(0.75f));
    g.fillEllipse(bounds.getX() + 9.0f, bounds.getY() + 10.0f, 8.0f, 8.0f);
}

//------------------------------------------------------------------------------
void VuMeterControl::drawMeterGlyph(Graphics& g, Rectangle<float> bounds)
{
    auto& colours = ColourScheme::getInstance().colours;
    const auto accent = colours["Graph Category Meter"];
    const auto surface = colours["Plugin Background"].darker(0.18f);

    g.setColour(surface.withAlpha(0.76f));
    g.fillRoundedRectangle(bounds, 7.0f);
    g.setColour(accent.withAlpha(0.75f));
    g.drawRoundedRectangle(bounds.reduced(0.5f), 7.0f, 1.1f);

    auto dial = bounds.reduced(7.0f);
    g.setColour(accent.withAlpha(0.26f));
    g.drawEllipse(dial, 1.2f);
    g.drawEllipse(dial.reduced(5.0f), 1.0f);

    const auto centre = dial.getCentre();
    Path needle;
    needle.startNewSubPath(centre);
    needle.lineTo(centre.x + dial.getWidth() * 0.28f, centre.y - dial.getHeight() * 0.18f);
    g.setColour(accent.withAlpha(0.86f));
    g.strokePath(needle, PathStrokeType(1.8f, PathStrokeType::curved, PathStrokeType::rounded));
    g.fillEllipse(centre.x - 2.0f, centre.y - 2.0f, 4.0f, 4.0f);
}

//------------------------------------------------------------------------------
void VuMeterControl::drawVuMeterColumn(Graphics& g, Rectangle<float> bounds, const MeterSnapshot& snapshot, const String& label)
{
    auto& colours = ColourScheme::getInstance().colours;
    const auto text = colours["Text Colour"];
    const auto surface = colours["Plugin Background"].darker(0.35f);
    const auto border = colours["Plugin Border"];
    const auto accent = colours["Graph Category Meter"];
    const auto low = colours["VU Meter Lower Colour"];
    const auto high = colours["VU Meter Upper Colour"];
    const auto over = colours["VU Meter Over Colour"];

    auto labelArea = bounds.removeFromTop(14.0f);
    g.setFont(FontManager::getInstance().getUIFont(12.0f, true));
    g.setColour(text.withAlpha(0.86f));
    g.drawText(label, labelArea, Justification::centred);

    auto clipArea = bounds.removeFromTop(8.0f).reduced(5.0f, 0.5f);
    g.setColour(snapshot.clipHoldFrames > 0 ? over.withAlpha(0.92f) : border.withAlpha(0.28f));
    g.fillRoundedRectangle(clipArea, 3.0f);

    bounds.removeFromTop(4.0f);
    auto valueArea = bounds.removeFromBottom(18.0f);
    auto track = bounds.reduced(7.0f, 1.0f);

    g.setColour(surface.withAlpha(0.88f));
    g.fillRoundedRectangle(track, 4.0f);
    g.setColour(border.withAlpha(0.44f));
    g.drawRoundedRectangle(track.reduced(0.5f), 4.0f, 0.8f);

    const auto vuNorm = dbToNormalised(snapshot.vuDb);
    const auto rmsNorm = dbToNormalised(snapshot.rmsDb);
    const auto peakNorm = dbToNormalised(snapshot.peakDb);
    const auto fillBottom = track.getBottom() - 2.0f;
    const auto fillWidth = track.getWidth() - 4.0f;

    auto drawLevelFill = [&](float normalised, float inset, float alpha)
    {
        if (normalised <= 0.0f)
            return;

        auto fill = track.reduced(2.0f + inset, 2.0f);
        const float fillHeight = fill.getHeight() * normalised;
        fill = fill.withY(fillBottom - fillHeight).withHeight(fillHeight);

        ColourGradient gradient(low.withAlpha(alpha), fill.getBottomLeft(), over.withAlpha(alpha), fill.getTopLeft(),
                                false);
        gradient.addColour(0.68, high.withAlpha(alpha));
        g.setGradientFill(gradient);
        g.fillRoundedRectangle(fill, 3.0f);
    };

    drawLevelFill(vuNorm, 0.0f, 0.72f);
    drawLevelFill(rmsNorm, fillWidth * 0.28f, 0.42f);

    if (peakNorm > 0.0f)
    {
        const float peakY = track.getBottom() - track.getHeight() * peakNorm;
        g.setColour(peakNorm > 0.94f ? over.withAlpha(0.95f) : accent.withAlpha(0.9f));
        g.drawLine(track.getX() + 2.0f, peakY, track.getRight() - 2.0f, peakY, 1.5f);
    }

    drawVuMeterValuePill(g, valueArea.reduced(1.0f, 1.0f), formatDb(snapshot.peakDb),
                         snapshot.peakDb >= -0.1f ? over : accent);
}

//------------------------------------------------------------------------------
void VuMeterControl::drawVuMeterScale(Graphics& g, Rectangle<float> bounds)
{
    auto& colours = ColourScheme::getInstance().colours;
    const auto text = colours["Text Colour"];
    const auto border = colours["Plugin Border"];
    const float tickDb[] = {0.0f, -6.0f, -12.0f, -24.0f, -48.0f};

    auto rail = bounds.reduced(7.0f, 0.0f);
    g.setColour(border.withAlpha(0.18f));
    g.drawLine(rail.getCentreX(), rail.getY(), rail.getCentreX(), rail.getBottom(), 1.0f);

    g.setFont(FontManager::getInstance().getMonoFont(9.0f));
    for (float db : tickDb)
    {
        const float y = rail.getBottom() - rail.getHeight() * dbToNormalised(db);
        g.setColour(border.withAlpha(0.42f));
        g.drawLine(rail.getX(), y, rail.getRight(), y, 0.8f);
        g.setColour(text.withAlpha(0.52f));
        g.drawText(String(static_cast<int>(db)), Rectangle<float>(bounds.getX(), y - 6.0f, bounds.getWidth(), 12.0f),
                   Justification::centred);
    }
}

//------------------------------------------------------------------------------
void VuMeterControl::drawVuMeterValuePill(Graphics& g, Rectangle<float> bounds, const String& text, Colour accent)
{
    auto& colours = ColourScheme::getInstance().colours;
    const auto base = colours["Plugin Background"].darker(0.22f);
    const auto foreground = base.contrasting(0.88f);

    g.setColour(base.withAlpha(0.86f));
    g.fillRoundedRectangle(bounds, bounds.getHeight() * 0.48f);
    g.setColour(accent.withAlpha(0.72f));
    g.drawRoundedRectangle(bounds.reduced(0.5f), bounds.getHeight() * 0.48f, 1.0f);

    g.setFont(FontManager::getInstance().getMonoFont(jmin(11.0f, bounds.getHeight() - 5.0f)));
    g.setColour(foreground.withAlpha(0.88f));
    g.drawFittedText(text, bounds.toNearestInt().reduced(3, 0), Justification::centred, 1);
}

//------------------------------------------------------------------------------
float VuMeterControl::amplitudeToDb(float amplitude)
{
    if (!std::isfinite(amplitude) || amplitude <= 0.00025f)
        return -72.0f;

    return jlimit(-72.0f, 12.0f, 20.0f * std::log10(amplitude));
}

//------------------------------------------------------------------------------
float VuMeterControl::dbToNormalised(float db)
{
    return jlimit(0.0f, 1.0f, (db + 60.0f) / 66.0f);
}

//------------------------------------------------------------------------------
String VuMeterControl::formatDb(float db)
{
    if (db <= -69.0f)
        return "-inf";

    return String(db, db > -10.0f ? 1 : 0) + " dB";
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
VuMeterEditor::VuMeterEditor(AudioProcessor* processor, const Rectangle<int>& windowBounds)
    : AudioProcessorEditor(processor), parentBounds(windowBounds), setPos(false)
{
    meter = new VuMeterControl(dynamic_cast<VuMeterProcessor*>(processor));
    addAndMakeVisible(meter);

    setSize(128, 256);
}

//------------------------------------------------------------------------------
VuMeterEditor::~VuMeterEditor()
{
    VuMeterProcessor* proc = dynamic_cast<VuMeterProcessor*>(getAudioProcessor());

    if (proc && getParentComponent())
    {
        parentBounds = getTopLevelComponent()->getBounds();
        proc->updateEditorBounds(parentBounds);
    }

    deleteAllChildren();
    getAudioProcessor()->editorBeingDeleted(this);
}

//------------------------------------------------------------------------------
void VuMeterEditor::resized()
{
    // Resize the meter.
    meter->setSize(getWidth(), getHeight());
}

//------------------------------------------------------------------------------
void VuMeterEditor::paint(Graphics& g)
{
    g.fillAll(ColourScheme::getInstance().colours["Window Background"]);
}
