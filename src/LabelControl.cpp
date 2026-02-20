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
    editor->setMultiLine(true, false);       // Multi-line, no word wrap
    editor->setReturnKeyStartsNewLine(true); // Allow Enter for new lines
    editor->addListener(this);

    // Style the editor to match theme
    auto& scheme = ColourScheme::getInstance();
    auto bgColour = scheme.colours["Plugin Background"];
    auto textColour = scheme.colours["Text Colour"];

    editor->setColour(TextEditor::backgroundColourId, bgColour);
    editor->setColour(TextEditor::textColourId, textColour);
    editor->setColour(TextEditor::outlineColourId, scheme.colours["Slider Colour"]);
    editor->setColour(TextEditor::focusedOutlineColourId, scheme.colours["Slider Colour"]);
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
    int newWidth = maxWidth + 24;
    int newHeight = textHeight + 12;

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
    auto& scheme = ColourScheme::getInstance();
    auto bgColour = scheme.colours["Plugin Background"];
    auto textColour = scheme.colours["Text Colour"];

    // Semi-transparent background with rounded corners
    g.setColour(bgColour.withAlpha(0.85f));
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 4.0f);

    // Only draw text when not in edit mode
    if (!editMode)
    {
        g.setFont(labelFont);
        g.setColour(textColour);

        // Draw multi-line text centered
        String text = processor->getText();
        auto bounds = getLocalBounds().reduced(4);
        g.drawFittedText(text, bounds, Justification::centred, 10, 1.0f);
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
