//  LabelControl.cpp - Implementation of LabelControl.
//  ----------------------------------------------------------------------------
//  This file is part of Pedalboard3, an audio plugin host.
//  Copyright (c) 2024 Pedalboard3 Project.
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//  ----------------------------------------------------------------------------

#include "LabelControl.h"

#include "ColourScheme.h"
#include "FontManager.h"
#include "LabelProcessor.h"

LabelControl::LabelControl(LabelProcessor* proc) : processor(proc), editMode(false)
{
    // Register with processor for state updates
    processor->registerControl(this);

    // Create the inline text editor (hidden initially)
    editor.reset(new TextEditor());
    editor->setMultiLine(true, true);        // Multi-line with wrapping
    editor->setReturnKeyStartsNewLine(true); // Allow Enter for new lines
    editor->addListener(this);

    // Style the editor to match theme
    auto& scheme = ColourScheme::getInstance();
    auto bgColour = scheme.colours["Warning Colour"].withMultipliedSaturation(0.28f).brighter(1.35f);
    auto textColour = scheme.colours["Window Background"].contrasting(0.82f);

    editor->setColour(TextEditor::backgroundColourId, bgColour);
    editor->setColour(TextEditor::textColourId, textColour);
    editor->setColour(TextEditor::outlineColourId, scheme.colours["Warning Colour"].withAlpha(0.36f));
    editor->setColour(TextEditor::focusedOutlineColourId, scheme.colours["Warning Colour"].withAlpha(0.72f));
    editor->setColour(TextEditor::highlightColourId, scheme.colours["Warning Colour"].withAlpha(0.28f));
    editor->setColour(CaretComponent::caretColourId, textColour);

    // Use Space Grotesk Bold if available
    auto& fontMgr = FontManager::getInstance();
    if (fontMgr.areFontsAvailable())
        labelFont = fontMgr.getSubheadingFont();
    else
        labelFont = FontManager::getInstance().getSubheadingFont();

    editor->setFont(labelFont);
    editor->setJustification(Justification::centred);
    editor->setText(processor->getText(), false);
    editor->setVisible(false);
    addChildComponent(editor.get());

    // Set initial size (autoResize would fail since parent doesn't exist yet)
    setSize(120, 32);
}

LabelControl::~LabelControl()
{
    if (processor)
        processor->unregisterControl(this);
}

void LabelControl::autoResize()
{
    String text = processor->getText();
    if (text.isEmpty())
        text = "Label";

    // Calculate text dimensions
    StringArray lines;
    lines.addLines(text);

    int maxWidth = 60; // Minimum width
    for (const auto& line : lines)
    {
        int lineWidth = labelFont.getStringWidth(line);
        maxWidth = jmax(maxWidth, lineWidth);
    }

    int numLines = jmax(1, lines.size());
    int lineHeight = (int)labelFont.getHeight();
    int textHeight = numLines * lineHeight;

    // Add padding
    int newWidth = maxWidth + 34;
    int newHeight = textHeight + 20;

    // Enforce minimum sizes
    newWidth = jmax(60, newWidth);
    newHeight = jmax(28, newHeight);

    setSize(newWidth, newHeight);

    // Also resize the parent PluginComponent to keep in sync
    if (auto* parent = getParentComponent())
    {
        int parentWidth = newWidth + 20;
        int parentHeight = newHeight + 50;
        parent->setSize(parentWidth, parentHeight);
    }
}

void LabelControl::resized()
{
    if (editor)
        editor->setBounds(getLocalBounds().reduced(4));
}

void LabelControl::paint(Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced(2.0f);
    auto& colours = ColourScheme::getInstance().colours;
    const auto paper = colours["Warning Colour"].withMultipliedSaturation(0.32f).brighter(1.24f);
    const auto edge = colours["Warning Colour"].withMultipliedSaturation(0.72f).darker(0.18f);
    const auto ink = colours["Window Background"].contrasting(0.84f);

    Path shadow;
    shadow.addRoundedRectangle(bounds.translated(2.2f, 2.8f), 6.0f);
    g.setColour(Colours::black.withAlpha(0.16f));
    g.fillPath(shadow);

    Path paperPath;
    paperPath.startNewSubPath(bounds.getX() + 3.0f, bounds.getY());
    paperPath.lineTo(bounds.getRight() - 2.0f, bounds.getY() + 1.0f);
    paperPath.lineTo(bounds.getRight(), bounds.getBottom() - 3.0f);
    paperPath.lineTo(bounds.getX() + 1.0f, bounds.getBottom());
    paperPath.closeSubPath();

    ColourGradient fill(paper.brighter(0.10f), bounds.getX(), bounds.getY(), paper.darker(0.08f), bounds.getX(),
                        bounds.getBottom(), false);
    fill.addColour(0.55, paper);
    g.setGradientFill(fill);
    g.fillPath(paperPath);
    g.setColour(edge.withAlpha(0.58f));
    g.strokePath(paperPath, PathStrokeType(1.0f));

    g.setColour(Colours::white.withAlpha(0.16f));
    g.drawLine(bounds.getX() + 7.0f, bounds.getY() + 5.0f, bounds.getRight() - 9.0f, bounds.getY() + 6.0f, 1.0f);

    // Only draw text when not in edit mode
    if (!editMode)
    {
        g.setFont(labelFont);
        g.setColour(ink);

        // Draw multi-line text centered
        String text = processor->getText();
        auto textBounds = getLocalBounds().reduced(10, 7);
        g.drawFittedText(text, textBounds, Justification::centred, 10, 0.82f);
    }
}

void LabelControl::mouseDoubleClick(const MouseEvent&)
{
    setEditMode(true);
}

void LabelControl::textEditorReturnKeyPressed(TextEditor&)
{
    // With multi-line enabled, Return adds a new line
    // Use Escape or click outside to commit
}

void LabelControl::textEditorEscapeKeyPressed(TextEditor&)
{
    // Commit text and exit edit mode
    commitText();
    setEditMode(false);
}

void LabelControl::textEditorFocusLost(TextEditor&)
{
    if (editMode)
    {
        commitText();
        setEditMode(false);
    }
}

void LabelControl::updateText(const String& newText)
{
    if (editor)
        editor->setText(newText, false);
    autoResize();
    repaint();
}

void LabelControl::setEditMode(bool shouldEdit)
{
    editMode = shouldEdit;

    if (editMode)
    {
        editor->setText(processor->getText(), false);
        editor->setVisible(true);
        editor->grabKeyboardFocus();
        editor->selectAll();
    }
    else
    {
        editor->setVisible(false);
        autoResize();
    }

    repaint();
}

void LabelControl::commitText()
{
    if (processor && editor)
    {
        processor->setText(editor->getText());
        autoResize();
    }
}
