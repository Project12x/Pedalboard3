#pragma once

#include <JuceHeader.h>

//==============================================================================
/**
    A simple Tokeniser for Markdown syntax highlighting in CodeEditorComponent.
*/
class MarkdownTokeniser : public CodeTokeniser
{
  public:
    MarkdownTokeniser() = default;
    ~MarkdownTokeniser() override = default;

    int readNextToken(CodeDocument::Iterator& source) override;
    CodeEditorComponent::ColourScheme getDefaultColourScheme() override;

  private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MarkdownTokeniser)
};

// Define token types
namespace MarkdownTokenTypes
{
enum TokenType
{
    plainText = 0,
    header, // # Header
    bold,   // **Bold**
    italic, // *Italic*
    quote,  // > Quote
    list,   // - List
    code,   // `Code`
    link    // [Link]
};
}
