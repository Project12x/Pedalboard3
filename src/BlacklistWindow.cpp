/*
  ==============================================================================

    BlacklistWindow.cpp

    Window for managing the plugin blacklist.

  ==============================================================================
*/

#include "BlacklistWindow.h"

#include "ColourScheme.h"
#include "PluginBlacklist.h"

#include <spdlog/spdlog.h>


//==============================================================================
// BlacklistListModel
//==============================================================================

void BlacklistListModel::refresh()
{
    items.clear();

    auto& blacklist = PluginBlacklist::getInstance();

    // Get both paths and IDs
    auto paths = blacklist.getBlacklistedPaths();
    auto ids = blacklist.getBlacklistedIds();

    for (const auto& path : paths)
        items.add("Path: " + path);

    for (const auto& id : ids)
        items.add("ID: " + id);
}

void BlacklistListModel::paintListBoxItem(int rowNumber, Graphics& g, int width, int height, bool rowIsSelected)
{
    auto& colours = ColourScheme::getInstance().colours;

    if (rowIsSelected)
        g.fillAll(colours["List Selection"]);
    else if (rowNumber % 2 == 0)
        g.fillAll(colours["Dialog Inner Background"]);
    else
        g.fillAll(colours["Dialog Inner Background"].darker(0.05f));

    g.setColour(colours["Text Colour"]);
    g.setFont(13.0f);

    if (rowNumber >= 0 && rowNumber < items.size())
    {
        g.drawText(items[rowNumber], 8, 0, width - 16, height, Justification::centredLeft, true);
    }
}

//==============================================================================
// BlacklistComponent
//==============================================================================

BlacklistComponent::BlacklistComponent()
{
    auto& colours = ColourScheme::getInstance().colours;

    // Title
    titleLabel = std::make_unique<Label>("title", "Plugin Blacklist");
    titleLabel->setFont(Font(18.0f, Font::bold));
    titleLabel->setColour(Label::textColourId, colours["Text Colour"]);
    addAndMakeVisible(titleLabel.get());

    // Info label
    infoLabel =
        std::make_unique<Label>("info", "Blacklisted plugins will not be loaded. Remove items to allow loading again.");
    infoLabel->setFont(Font(12.0f));
    infoLabel->setColour(Label::textColourId, colours["Text Colour"].withAlpha(0.7f));
    addAndMakeVisible(infoLabel.get());

    // List box
    listBox = std::make_unique<ListBox>("blacklist", &listModel);
    listBox->setRowHeight(24);
    listBox->setColour(ListBox::backgroundColourId, colours["Dialog Inner Background"]);
    listBox->setColour(ListBox::outlineColourId, colours["Text Colour"].withAlpha(0.3f));
    listBox->setOutlineThickness(1);
    addAndMakeVisible(listBox.get());

    // Remove button
    removeButton = std::make_unique<TextButton>("Remove Selected");
    removeButton->addListener(this);
    addAndMakeVisible(removeButton.get());

    // Clear all button
    clearAllButton = std::make_unique<TextButton>("Clear All");
    clearAllButton->addListener(this);
    addAndMakeVisible(clearAllButton.get());

    // Close button
    closeButton = std::make_unique<TextButton>("Close");
    closeButton->addListener(this);
    addAndMakeVisible(closeButton.get());

    refreshList();
    setSize(500, 400);
}

void BlacklistComponent::paint(Graphics& g)
{
    auto& colours = ColourScheme::getInstance().colours;
    g.fillAll(colours["Window Background"]);
}

void BlacklistComponent::resized()
{
    auto bounds = getLocalBounds().reduced(16);

    titleLabel->setBounds(bounds.removeFromTop(30));
    infoLabel->setBounds(bounds.removeFromTop(24));
    bounds.removeFromTop(8);

    auto buttonArea = bounds.removeFromBottom(36);
    bounds.removeFromBottom(8);

    // Buttons
    closeButton->setBounds(buttonArea.removeFromRight(80));
    buttonArea.removeFromRight(8);
    clearAllButton->setBounds(buttonArea.removeFromRight(80));
    buttonArea.removeFromRight(8);
    removeButton->setBounds(buttonArea.removeFromRight(120));

    // List
    listBox->setBounds(bounds);
}

void BlacklistComponent::buttonClicked(Button* button)
{
    if (button == removeButton.get())
    {
        auto selectedRow = listBox->getSelectedRow();
        if (selectedRow >= 0)
        {
            auto item = listModel.getItemAt(selectedRow);
            auto& blacklist = PluginBlacklist::getInstance();

            if (item.startsWith("Path: "))
            {
                auto path = item.substring(6);
                blacklist.removeFromBlacklist(path);
                spdlog::info("[BlacklistWindow] Removed path from blacklist: {}", path.toStdString());
            }
            else if (item.startsWith("ID: "))
            {
                auto id = item.substring(4);
                blacklist.removeFromBlacklistById(id);
                spdlog::info("[BlacklistWindow] Removed ID from blacklist: {}", id.toStdString());
            }

            refreshList();
        }
    }
    else if (button == clearAllButton.get())
    {
        PluginBlacklist::getInstance().clearBlacklist();
        spdlog::info("[BlacklistWindow] Cleared entire blacklist");
        refreshList();
    }
    else if (button == closeButton.get())
    {
        if (auto* window = findParentComponentOfClass<BlacklistWindow>())
            window->closeButtonPressed();
    }
}

void BlacklistComponent::refreshList()
{
    listModel.refresh();
    listBox->updateContent();
    listBox->repaint();
}

//==============================================================================
// BlacklistWindow
//==============================================================================

std::unique_ptr<BlacklistWindow> BlacklistWindow::instance;

BlacklistWindow::BlacklistWindow()
    : DocumentWindow("Plugin Blacklist", ColourScheme::getInstance().colours["Window Background"],
                     DocumentWindow::closeButton)
{
    setContentOwned(new BlacklistComponent(), true);
    setResizable(true, false);
    setUsingNativeTitleBar(true);
    centreWithSize(500, 400);
}

void BlacklistWindow::showWindow()
{
    if (!instance)
        instance = std::make_unique<BlacklistWindow>();

    instance->setVisible(true);
    instance->toFront(true);
}
