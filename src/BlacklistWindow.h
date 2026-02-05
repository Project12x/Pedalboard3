/*
  ==============================================================================

    BlacklistWindow.h

    Window for managing the plugin blacklist.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

/**
 * @class BlacklistListModel
 * @brief ListBox model for displaying blacklisted plugins.
 */
class BlacklistListModel : public ListBoxModel
{
  public:
    BlacklistListModel() { refresh(); }

    void refresh();

    int getNumRows() override { return items.size(); }

    void paintListBoxItem(int rowNumber, Graphics& g, int width, int height, bool rowIsSelected) override;

    void listBoxItemClicked(int row, const MouseEvent& e) override {}

    String getItemAt(int index) const { return index >= 0 && index < items.size() ? items[index] : String(); }

  private:
    StringArray items;
};

/**
 * @class BlacklistComponent
 * @brief Component for managing the plugin blacklist.
 */
class BlacklistComponent : public Component, public Button::Listener
{
  public:
    BlacklistComponent();
    ~BlacklistComponent() override = default;

    void paint(Graphics& g) override;
    void resized() override;
    void buttonClicked(Button* button) override;

  private:
    void refreshList();

    std::unique_ptr<Label> titleLabel;
    std::unique_ptr<Label> infoLabel;
    std::unique_ptr<ListBox> listBox;
    std::unique_ptr<TextButton> removeButton;
    std::unique_ptr<TextButton> clearAllButton;
    std::unique_ptr<TextButton> closeButton;

    BlacklistListModel listModel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BlacklistComponent)
};

/**
 * @class BlacklistWindow
 * @brief Window for managing plugin blacklist.
 */
class BlacklistWindow : public DocumentWindow
{
  public:
    BlacklistWindow();
    ~BlacklistWindow() override = default;

    void closeButtonPressed() override { setVisible(false); }

    static void showWindow();

  private:
    static std::unique_ptr<BlacklistWindow> instance;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BlacklistWindow)
};
