#pragma once

#include <JuceHeader.h>

namespace ScratchPanelLayout
{
constexpr int rowHeight = 102;
constexpr int rowPadding = 10;
constexpr int actionTop = 42;
constexpr int actionGap = 8;
constexpr int playWidth = 58;
constexpr int reampWidth = 68;
constexpr int revealWidth = 66;
constexpr int destinationActionGap = 8;
constexpr int destinationChooseWidth = 82;
constexpr int destinationResetWidth = 70;
constexpr int destinationRevealWidth = 70;

struct TakeRowActions
{
    juce::Rectangle<int> play;
    juce::Rectangle<int> reamp;
    juce::Rectangle<int> reveal;
    int rowRight = 0;
};

struct DestinationLayout
{
    juce::Rectangle<int> text;
    juce::Rectangle<int> choose;
    juce::Rectangle<int> reset;
    juce::Rectangle<int> reveal;
    int rowRight = 0;
};

inline TakeRowActions calculateTakeRowActions(juce::Rectangle<int> row)
{
    auto content = row.reduced(rowPadding, rowPadding);
    const auto totalWidth = playWidth + reampWidth + revealWidth + actionGap * 2;
    auto actions = content.removeFromRight(totalWidth).withTrimmedTop(actionTop);

    TakeRowActions layout;
    layout.rowRight = row.getRight() - rowPadding;
    layout.play = actions.removeFromLeft(playWidth);
    actions.removeFromLeft(actionGap);
    layout.reamp = actions.removeFromLeft(reampWidth);
    actions.removeFromLeft(actionGap);
    layout.reveal = actions.removeFromLeft(revealWidth);
    return layout;
}

inline TakeRowActions calculateTakeRowActions(int rowWidth)
{
    return calculateTakeRowActions({0, 0, rowWidth, rowHeight});
}

inline DestinationLayout calculateDestinationLayout(juce::Rectangle<int> destination)
{
    constexpr int horizontalPadding = 8;
    constexpr int verticalPadding = 10;
    constexpr int textGap = 8;
    const auto totalActionWidth = destinationChooseWidth + destinationResetWidth + destinationRevealWidth
                                  + destinationActionGap * 2;

    auto actions = destination.reduced(horizontalPadding, verticalPadding).removeFromRight(totalActionWidth);
    auto text = destination.reduced(12, 4);
    const auto textRight = actions.getX() - textGap;
    text.setWidth(juce::jmax(0, textRight - text.getX()));

    DestinationLayout layout;
    layout.text = text;
    layout.rowRight = destination.getRight() - horizontalPadding;
    layout.choose = actions.removeFromLeft(destinationChooseWidth);
    actions.removeFromLeft(destinationActionGap);
    layout.reset = actions.removeFromLeft(destinationResetWidth);
    actions.removeFromLeft(destinationActionGap);
    layout.reveal = actions.removeFromLeft(destinationRevealWidth);
    return layout;
}

inline juce::String compactDestinationPathForDisplay(juce::String path, int maxCharacters)
{
    path = path.trim();
    if (maxCharacters <= 0 || path.length() <= maxCharacters)
        return path;

    if (maxCharacters <= 3)
        return path.substring(0, maxCharacters);

    const int tailLength = juce::jmax(4, maxCharacters - 7);
    const int headLength = juce::jmax(1, maxCharacters - tailLength - 3);
    return path.substring(0, headLength) + "..." + path.substring(path.length() - tailLength);
}
} // namespace ScratchPanelLayout
