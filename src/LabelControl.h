//  LabelControl.h - UI control for the LabelProcessor.
//  ----------------------------------------------------------------------------
//  This file is part of Pedalboard3, an audio plugin host.
//  Copyright (c) 2024 Pedalboard3 Project.
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//  ----------------------------------------------------------------------------

#pragma once

#include <JuceHeader.h>

class LabelProcessor;

/**
    Simple inline text label with double-click editing.
    Uses themed colors from ColourScheme and Space Grotesk Bold font.
*/
class LabelControl : public Component, public TextEditor::Listener
{
  public:
    LabelControl(LabelProcessor* processor);
    ~LabelControl();

    void resized() override;
    void paint(Graphics& g) override;
    void mouseDoubleClick(const MouseEvent& event) override;

    // TextEditor::Listener
    void textEditorReturnKeyPressed(TextEditor& editor) override;
    void textEditorEscapeKeyPressed(TextEditor& editor) override;
    void textEditorFocusLost(TextEditor& editor) override;

    void updateText(const String& newText);

  private:
    LabelProcessor* processor;
    std::unique_ptr<TextEditor> editor;
    Font labelFont;
    bool editMode;

    void setEditMode(bool shouldEdit);
    void commitText();
    void autoResize();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LabelControl)
};
