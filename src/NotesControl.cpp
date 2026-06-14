#include "NotesControl.h"
#include "FontManager.h"

#include "ColourScheme.h"
#include "NotesProcessor.h"
#include "md4c.h"

#include <spdlog/spdlog.h>

//==============================================================================
// MarkdownEditor Implementation
//==============================================================================

MarkdownEditor::MarkdownEditor(CodeDocument& doc, CodeTokeniser* tokens) : CodeEditorComponent(doc, tokens)
{
    // Enable standard key commands
    setWantsKeyboardFocus(true);
}

bool MarkdownEditor::keyPressed(const KeyPress& key)
{
    // Escape: exit edit mode
    if (key == KeyPress::escapeKey)
    {
        if (onEscapePressed)
            onEscapePressed();
        return true;
    }

    bool isCmd = key.getModifiers().isCommandDown();

    // Ctrl+B: Bold
    if (isCmd && key.getKeyCode() == 'B')
    {
        wrapSelection("**");
        return true;
    }

    // Ctrl+I: Italic
    if (isCmd && key.getKeyCode() == 'I')
    {
        wrapSelection("*");
        return true;
    }

    return CodeEditorComponent::keyPressed(key);
}

void MarkdownEditor::mouseDown(const MouseEvent& e)
{
    if (e.mods.isPopupMenu())
    {
        performPopup(e);
    }
    else
    {
        CodeEditorComponent::mouseDown(e);
    }
}

void MarkdownEditor::wrapSelection(const String& symbol)
{
    Range<int> selection = getHighlightedRegion();

    if (selection.isEmpty())
    {
        insertTextAtCaret(symbol + symbol);
        moveCaretLeft(false, false);
        if (symbol.length() > 1)
            moveCaretLeft(false, false);
        return;
    }

    // Get text manually since getTextBetween(Range) isn't direct
    String total = getDocument().getAllContent();
    String text = total.substring(selection.getStart(), selection.getEnd());

    insertTextAtCaret(symbol + text + symbol);
}

void MarkdownEditor::toggleList()
{
    insertTextAtCaret("- ");
}

void MarkdownEditor::performPopup(const MouseEvent& e)
{
    PopupMenu m;
    m.addItem(1, "Cut");
    m.addItem(2, "Copy");
    m.addItem(3, "Paste");
    m.addSeparator();
    m.addItem(4, "Bold (Ctrl+B)");
    m.addItem(5, "Italic (Ctrl+I)");
    m.addItem(6, "List Item");

    m.showMenuAsync(PopupMenu::Options().withTargetComponent(this),
                    [this](int result)
                    {
                        if (result == 0)
                            return;

                        Range<int> sel = getHighlightedRegion();
                        String total = getDocument().getAllContent();

                        switch (result)
                        {
                        case 1: // Cut
                        {
                            if (!sel.isEmpty())
                            {
                                String text = total.substring(sel.getStart(), sel.getEnd());
                                SystemClipboard::copyTextToClipboard(text);
                                insertTextAtCaret(""); // Delete
                            }
                            break;
                        }
                        case 2: // Copy
                        {
                            if (!sel.isEmpty())
                            {
                                String text = total.substring(sel.getStart(), sel.getEnd());
                                SystemClipboard::copyTextToClipboard(text);
                            }
                            break;
                        }
                        case 3: // Paste
                        {
                            insertTextAtCaret(SystemClipboard::getTextFromClipboard());
                            break;
                        }
                        case 4:
                            wrapSelection("**");
                            break;
                        case 5:
                            wrapSelection("*");
                            break;
                        case 6:
                            toggleList();
                            break;
                        }
                    });
}

//==============================================================================
// MarkdownRenderer Helper
//==============================================================================

namespace MarkdownRenderer
{
struct RenderContext
{
    AttributedString& attributedString;
    Font baseFont;
    Colour baseColour;

    struct State
    {
        Font font;
        Colour colour;
    };
    std::vector<State> stateStack;

    RenderContext(AttributedString& s) : attributedString(s)
    {
        auto& colours = ColourScheme::getInstance().colours;
        baseFont = FontManager::getInstance().getBodyFont();
        baseColour = colours["Window Background"].contrasting(0.84f);
        stateStack.push_back({baseFont, baseColour});
    }

    State& current() { return stateStack.back(); }
    void push() { stateStack.push_back(current()); }
    void pop()
    {
        if (stateStack.size() > 1)
            stateStack.pop_back();
    }
};

static int enter_block(MD_BLOCKTYPE type, void* detail, void* userdata)
{
    auto* ctx = static_cast<RenderContext*>(userdata);
    ctx->push();

    if (type == MD_BLOCK_H)
    {
        auto level = static_cast<MD_BLOCK_H_DETAIL*>(detail)->level;
        float size = 18.0f - (level - 1) * 2.0f;
        if (size < 12.0f)
            size = 12.0f;
        ctx->current().font = ctx->current().font.withHeight(size).withStyle(Font::bold);
        ctx->current().colour = ColourScheme::getInstance().colours["Warning Colour"].darker(0.46f);
        ctx->attributedString.append("\n", ctx->current().font, ctx->current().colour); // Spacing
    }
    else if (type == MD_BLOCK_QUOTE)
    {
        ctx->current().colour = ColourScheme::getInstance().colours["Window Background"].contrasting(0.56f);
        ctx->current().font = ctx->current().font.withStyle(Font::italic);
    }
    else if (type == MD_BLOCK_LI)
    {
        ctx->attributedString.append(CharPointer_UTF8("\xe2\x80\xa2 "), ctx->current().font, ctx->current().colour);
    }

    return 0;
}

static int leave_block(MD_BLOCKTYPE type, void* detail, void* userdata)
{
    auto* ctx = static_cast<RenderContext*>(userdata);
    if (type == MD_BLOCK_P || type == MD_BLOCK_H)
    {
        ctx->attributedString.append("\n", ctx->current().font, ctx->current().colour);
    }
    ctx->pop();
    return 0;
}

static int enter_span(MD_SPANTYPE type, void* detail, void* userdata)
{
    auto* ctx = static_cast<RenderContext*>(userdata);
    ctx->push();

    if (type == MD_SPAN_STRONG)
    {
        ctx->current().font = ctx->current().font.boldened();
        ctx->current().colour = ColourScheme::getInstance().colours["Warning Colour"].darker(0.52f);
    }
    else if (type == MD_SPAN_EM)
    {
        ctx->current().font = ctx->current().font.italicised();
        ctx->current().colour = ColourScheme::getInstance().colours["Graph Category Modulation"].darker(0.28f);
    }
    else if (type == MD_SPAN_CODE)
    {
        ctx->current().font = FontManager::getInstance().getMonoFont(13.0f);
        ctx->current().colour = ColourScheme::getInstance().colours["Graph Category Delay"].darker(0.34f);
    }

    return 0;
}

static int leave_span(MD_SPANTYPE type, void* detail, void* userdata)
{
    static_cast<RenderContext*>(userdata)->pop();
    return 0;
}

static int text_callback(MD_TEXTTYPE type, const MD_CHAR* text, MD_SIZE size, void* userdata)
{
    auto* ctx = static_cast<RenderContext*>(userdata);
    String s(text, size);
    ctx->attributedString.append(s, ctx->current().font, ctx->current().colour);
    return 0;
}
} // namespace MarkdownRenderer

//==============================================================================
// NotesControl Implementation
//==============================================================================

NotesControl::NotesControl(NotesProcessor* proc) : processor(proc), editMode(false)
{
    // Initialize Code Editor with our custom Markdown subclass
    codeDocument.addListener(this);
    editor.reset(new MarkdownEditor(codeDocument, &tokeniser));

    addAndMakeVisible(editor.get());

    auto& colours = ColourScheme::getInstance().colours;
    const auto paper = colours["Warning Colour"].withMultipliedSaturation(0.30f).brighter(1.26f);
    const auto ink = colours["Window Background"].contrasting(0.84f);
    editor->setColour(CodeEditorComponent::backgroundColourId, paper);
    editor->setColour(CodeEditorComponent::lineNumberBackgroundId, paper.darker(0.03f));
    editor->setColour(CodeEditorComponent::highlightColourId, colours["Warning Colour"].withAlpha(0.24f));
    editor->setColour(CodeEditorComponent::defaultTextColourId, ink);
    editor->setColour(CaretComponent::caretColourId, ink);
    editor->setFont(FontManager::getInstance().getMonoFont(14.0f));
    editor->setLineNumbersShown(false);

    // Wire up Escape key to exit edit mode
    editor->onEscapePressed = [this]() { setEditMode(false); };

    // Load text
    codeDocument.replaceAllContent(processor->getText());

    // Start in View Mode
    editor->setVisible(false);
    renderMarkdown(processor->getText());

    // Enable mouse/keyboard interaction
    setInterceptsMouseClicks(true, true);
    setWantsKeyboardFocus(true);

    setSize(200, 150);
}

NotesControl::~NotesControl()
{
    editor = nullptr;
}

void NotesControl::resized()
{
    if (!editor)
        return;

    if (editMode)
        editor->setBounds(getLocalBounds().reduced(10, 8));
    else
        editor->setBounds(0, 0, 0, 0);
}

void NotesControl::paint(Graphics& g)
{
    auto& colours = ColourScheme::getInstance().colours;
    auto bounds = getLocalBounds().toFloat().reduced(2.0f);
    const auto paper = colours["Warning Colour"].withMultipliedSaturation(0.30f).brighter(1.26f);
    const auto edge = colours["Warning Colour"].withMultipliedSaturation(0.74f).darker(0.18f);

    g.setColour(Colours::black.withAlpha(0.17f));
    g.fillRoundedRectangle(bounds.translated(2.2f, 3.0f), 7.0f);

    Path paperPath;
    paperPath.startNewSubPath(bounds.getX() + 4.0f, bounds.getY());
    paperPath.lineTo(bounds.getRight() - 2.0f, bounds.getY() + 1.0f);
    paperPath.lineTo(bounds.getRight(), bounds.getBottom() - 4.0f);
    paperPath.lineTo(bounds.getX() + 1.0f, bounds.getBottom());
    paperPath.closeSubPath();

    ColourGradient paperFill(paper.brighter(0.10f), bounds.getX(), bounds.getY(), paper.darker(0.08f), bounds.getX(),
                             bounds.getBottom(), false);
    paperFill.addColour(0.58, paper);
    g.setGradientFill(paperFill);
    g.fillPath(paperPath);
    g.setColour(edge.withAlpha(0.58f));
    g.strokePath(paperPath, PathStrokeType(1.0f));

    g.setColour(Colours::white.withAlpha(0.14f));
    g.drawLine(bounds.getX() + 9.0f, bounds.getY() + 7.0f, bounds.getRight() - 10.0f, bounds.getY() + 8.0f, 1.0f);

    if (!editMode)
    {
        // View Mode: Rendered rich text
        renderedText.draw(g, getLocalBounds().reduced(12, 10).toFloat());
    }

    // Draw subtle resize corner indicator.
    auto intBounds = getLocalBounds();
    Path resizeTriangle;
    resizeTriangle.addTriangle((float)intBounds.getRight() - 12, (float)intBounds.getBottom() - 3.0f,
                               (float)intBounds.getRight() - 3.0f, (float)intBounds.getBottom() - 12.0f,
                               (float)intBounds.getRight() - 3.0f, (float)intBounds.getBottom() - 3.0f);
    g.setColour(edge.withAlpha(0.42f));
    g.fillPath(resizeTriangle);
}

void NotesControl::codeDocumentTextInserted(const String& newText, int insertIndex)
{
    if (processor)
        processor->setText(codeDocument.getAllContent());
}

void NotesControl::codeDocumentTextDeleted(int startIndex, int endIndex)
{
    if (processor)
        processor->setText(codeDocument.getAllContent());
}

void NotesControl::updateText(const String& newText)
{
    if (codeDocument.getAllContent() != newText)
        codeDocument.replaceAllContent(newText);

    renderMarkdown(newText);
    repaint();
}

void NotesControl::mouseDown(const MouseEvent& event)
{
    spdlog::debug("[NotesControl::mouseDown] pos=({}, {}) clicks={}", event.getPosition().x, event.getPosition().y,
                  event.getNumberOfClicks());

    // Check for resize corner first
    if (isInResizeCorner(event.getPosition()))
    {
        spdlog::debug("[NotesControl::mouseDown] resize corner detected");
        resizing = true;
        dragStart = event.getPosition();
        boundsAtDragStart = getBounds();
        return;
    }

    // Double click to toggle edit mode
    if (!editMode && event.mods.isLeftButtonDown() && event.getNumberOfClicks() >= 2)
    {
        spdlog::debug("[NotesControl::mouseDown] double-click, entering edit mode");
        setEditMode(true);
    }
}

void NotesControl::mouseDrag(const MouseEvent& event)
{
    if (resizing)
    {
        spdlog::debug("[NotesControl::mouseDrag] resizing");
        auto delta = event.getPosition() - dragStart;
        int newWidth = jmax(100, boundsAtDragStart.getWidth() + delta.x);
        int newHeight = jmax(50, boundsAtDragStart.getHeight() + delta.y);

        // Resize this control
        setSize(newWidth, newHeight);

        // Also resize the parent PluginComponent
        if (auto* parent = getParentComponent())
        {
            int parentWidth = newWidth + 20;
            int parentHeight = newHeight + 50;
            parent->setSize(parentWidth, parentHeight);
        }
    }
}

void NotesControl::mouseUp(const MouseEvent& event)
{
    resizing = false;
}

void NotesControl::mouseMove(const MouseEvent& event)
{
    if (isInResizeCorner(event.getPosition()))
        setMouseCursor(MouseCursor::BottomRightCornerResizeCursor);
    else
        setMouseCursor(MouseCursor::NormalCursor);
}

bool NotesControl::isInResizeCorner(const Point<int>& pos) const
{
    auto bounds = getLocalBounds();
    auto corner = Rectangle<int>(bounds.getRight() - 20, bounds.getBottom() - 20, 20, 20);
    return corner.contains(pos);
}

void NotesControl::setEditMode(bool shouldEdit)
{
    spdlog::debug("[NotesControl::setEditMode] entering, shouldEdit={}", shouldEdit);
    editMode = shouldEdit;

    if (!editor)
    {
        spdlog::error("[NotesControl::setEditMode] editor is null!");
        return;
    }

    if (editMode)
    {
        spdlog::debug("[NotesControl::setEditMode] entering edit mode");
        editor->setVisible(true);
        editor->setBounds(getLocalBounds().reduced(10, 8));
        repaint();

        // Defer focus grab to avoid issues during mouse event handling
        auto* ed = editor.get();
        Timer::callAfterDelay(50,
                              [ed]()
                              {
                                  if (ed != nullptr)
                                      ed->grabKeyboardFocus();
                              });
    }
    else
    {
        spdlog::debug("[NotesControl::setEditMode] exiting edit mode");
        editor->setVisible(false);
        renderMarkdown(codeDocument.getAllContent());
        repaint();
    }
}

void NotesControl::renderMarkdown(const String& markdown)
{
    if (!markdown.isNotEmpty())
    {
        renderedText = AttributedString();
        return;
    }

    renderedText = AttributedString();
    renderedText.setJustification(Justification::topLeft);

    MarkdownRenderer::RenderContext ctx(renderedText);

    MD_PARSER parser;
    memset(&parser, 0, sizeof(MD_PARSER));
    parser.abi_version = 0;
    parser.flags = MD_DIALECT_COMMONMARK | MD_FLAG_PERMISSIVEAUTOLINKS;
    parser.enter_block = MarkdownRenderer::enter_block;
    parser.leave_block = MarkdownRenderer::leave_block;
    parser.enter_span = MarkdownRenderer::enter_span;
    parser.leave_span = MarkdownRenderer::leave_span;
    parser.text = MarkdownRenderer::text_callback;

    md_parse(markdown.toUTF8(), markdown.getNumBytesAsUTF8(), &parser, &ctx);
}
