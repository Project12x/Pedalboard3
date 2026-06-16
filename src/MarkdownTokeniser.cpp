#include "MarkdownTokeniser.h"

#include "ColourScheme.h"

int MarkdownTokeniser::readNextToken(CodeDocument::Iterator& source)
{
    source.skipWhitespace();

    auto firstChar = source.peekNextChar();

    if (firstChar == 0)
        return (int)MarkdownTokenTypes::plainText;

    // Headers: # space
    if (firstChar == '#')
    {
        CodeDocument::Iterator t(source);
        while (t.peekNextChar() == '#')
            t.nextChar();
        if (t.peekNextChar() == ' ')
        {
            source.nextChar();
            while (source.peekNextChar() != '\n' && source.peekNextChar() != 0)
                source.nextChar();
            return (int)MarkdownTokenTypes::header;
        }
    }

    // LIST: - space or * space
    if ((firstChar == '-' || firstChar == '*') && source.peekNextChar() != firstChar)
    {
        CodeDocument::Iterator t(source);
        t.nextChar();
        if (t.peekNextChar() == ' ')
        {
            source.nextChar();
            return (int)MarkdownTokenTypes::list;
        }
    }

    // QUOTE: > space
    if (firstChar == '>')
    {
        CodeDocument::Iterator t(source);
        t.nextChar();
        if (t.peekNextChar() == ' ')
        {
            while (source.peekNextChar() != '\n' && source.peekNextChar() != 0)
                source.nextChar();
            return (int)MarkdownTokenTypes::quote;
        }
    }

    // BOLD: **text**
    if (firstChar == '*')
    {
        CodeDocument::Iterator t(source);
        t.nextChar();
        if (t.peekNextChar() == '*') // It's **
        {
            source.nextChar();
            source.nextChar();

            while (!(source.peekNextChar() == '*' && CodeDocument::Iterator(source).nextChar() == '*'))
            {
                if (source.peekNextChar() == 0 || source.peekNextChar() == '\n')
                    break;
                source.nextChar();
            }
            if (source.peekNextChar() == '*')
            {
                source.nextChar();
                if (source.peekNextChar() == '*')
                    source.nextChar();
            }
            return (int)MarkdownTokenTypes::bold;
        }
    }

    source.nextChar();
    return (int)MarkdownTokenTypes::plainText;
}

CodeEditorComponent::ColourScheme MarkdownTokeniser::getDefaultColourScheme()
{
    CodeEditorComponent::ColourScheme cs;

    // Must strictly match enum order in MarkdownTokeniser.h
    /*
    enum TokenType
    {
        plainText = 0,
        header,      // # Header
        bold,        // **Bold**
        italic,      // *Italic*
        quote,       // > Quote
        list,        // - List
        code,        // `Code`
        link         // [Link]
    };
    */

    const auto ink = Colour(0xFF5C3D0F);
    const auto accent = Colour(0xFFB45309);
    const auto link = ColourScheme::getInstance().colours["Audio Connection"].interpolatedWith(ink, 0.35f);

    cs.set("Plain Text", ink);                       // 0
    cs.set("Header", accent.darker(0.12f));          // 1
    cs.set("Bold", accent.darker(0.08f));            // 2
    cs.set("Italic", ink.withAlpha(0.82f));          // 3
    cs.set("Quote", ink.withAlpha(0.68f));           // 4
    cs.set("List", accent);                          // 5
    cs.set("Code", accent.darker(0.20f));            // 6
    cs.set("Link", link.withMultipliedAlpha(0.92f)); // 7

    return cs;
}
