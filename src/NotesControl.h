#pragma once

#include "MarkdownTokeniser.h"
#include "PedalboardProcessors.h"

#include <JuceHeader.h>

// Check if md4c is available -- assumption: yes via CMake
class NotesProcessor;

//==============================================================================
/**
    Custom CodeEditor with Markdown-specific shortcuts and context menu.
*/
class MarkdownEditor : public CodeEditorComponent
{
  public:
    MarkdownEditor(CodeDocument& doc, CodeTokeniser* tokens);

    // Callback for when Escape is pressed (to exit edit mode)
    std::function<void()> onEscapePressed;

    // Customize interaction
    bool keyPressed(const KeyPress& key) override;
    void mouseDown(const MouseEvent& e) override;

    // Formatting helpers
    void wrapSelection(const String& symbol); // e.g. ** for bold
    void toggleList();                        // Adds/removes "- "

  private:
    void performPopup(const MouseEvent& e);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MarkdownEditor)
};

//==============================================================================
/**
    A draggable corner for resizing the Notes node.
*/
class ResizeCorner : public Component
{
  public:
    ResizeCorner(std::function<void(int, int)> onResize) : resizeCallback(onResize)
    {
        setSize(16, 16);
        setMouseCursor(MouseCursor::BottomRightCornerResizeCursor);
    }

    void paint(Graphics& g) override
    {
        // Draw a solid colored background to make corner visible
        g.setColour(Colour(0xFF666666));
        g.fillAll();

        // Draw diagonal lines for resize indicator
        g.setColour(Colours::white);
        g.drawLine(4, 12, 12, 4, 2.0f);
        g.drawLine(8, 12, 12, 8, 2.0f);
    }

    void mouseDown(const MouseEvent& e) override
    {
        dragStart = e.getEventRelativeTo(getParentComponent()).getPosition();
        parentBoundsAtDragStart = getParentComponent()->getBounds();
    }

    void mouseDrag(const MouseEvent& e) override
    {
        auto current = e.getEventRelativeTo(getParentComponent()).getPosition();
        auto delta = current - dragStart;
        int newWidth = jmax(100, parentBoundsAtDragStart.getWidth() + delta.x);
        int newHeight = jmax(50, parentBoundsAtDragStart.getHeight() + delta.y);
        if (resizeCallback)
            resizeCallback(newWidth, newHeight);
    }

  private:
    std::function<void(int, int)> resizeCallback;
    Point<int> dragStart;
    Rectangle<int> parentBoundsAtDragStart;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ResizeCorner)
};

//==============================================================================
/**
    The UI for the NotesProcessor.
*/
class NotesControl : public Component, public CodeDocument::Listener
{
  public:
    NotesControl(NotesProcessor* processor);
    ~NotesControl();

    void resized() override;
    void paint(Graphics& g) override;
    void mouseDown(const MouseEvent& event) override;
    void mouseDrag(const MouseEvent& event) override;
    void mouseUp(const MouseEvent& event) override;
    void mouseMove(const MouseEvent& event) override;

    // CodeDocument::Listener helpers
    void codeDocumentTextInserted(const String& newText, int insertIndex) override;
    void codeDocumentTextDeleted(int startIndex, int endIndex) override;

    void updateText(const String& newText);

  private:
    NotesProcessor* processor;

    // Editor components
    CodeDocument codeDocument;
    MarkdownTokeniser tokeniser;
    std::unique_ptr<MarkdownEditor> editor;

    // View mode state
    bool editMode;
    AttributedString renderedText;

    // Resize state
    bool resizing = false;
    Point<int> dragStart;
    Rectangle<int> boundsAtDragStart;

    void setEditMode(bool shouldEdit);
    void renderMarkdown(const String& markdown);
    bool isInResizeCorner(const Point<int>& pos) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NotesControl)
};
