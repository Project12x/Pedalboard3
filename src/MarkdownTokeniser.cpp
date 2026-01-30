#include "MarkdownTokeniser.h"

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

    cs.set("Plain Text", Colours::white); // 0
    cs.set("Header", Colours::gold);      // 1
    cs.set("Bold", Colours::orange);      // 2
    cs.set("Italic", Colours::lightblue); // 3
    cs.set("Quote", Colours::grey);       // 4
    cs.set("List", Colours::lightgreen);  // 5
    cs.set("Code", Colours::pink);        // 6
    cs.set("Link", Colours::cyan);        // 7

    return cs;
}
