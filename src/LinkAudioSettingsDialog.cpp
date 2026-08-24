#include "LinkAudioSettingsDialog.h"

#include "ColourScheme.h"
#include "FontManager.h"
#include "MainPanel.h"

using namespace juce;

namespace
{
void configureLabel(Label& label, const String& text, const Font& font)
{
    label.setText(text, dontSendNotification);
    label.setFont(font);
    label.setJustificationType(Justification::centredLeft);
}
} // namespace

LinkAudioSettingsDialog::LinkAudioSettingsDialog(MainPanel* panel)
    : mainPanel(panel)
{
    setSize(820, 540);

    addAndMakeVisible(linkSectionLabel);
    configureLabel(linkSectionLabel, "Link Audio", FontManager::getInstance().getSubheadingFont());

    addAndMakeVisible(linkDescriptionLabel);
    configureLabel(linkDescriptionLabel, "Publish Pedalboard3's master output to enabled Link Audio peers on your local network.",
                   FontManager::getInstance().getBodyFont());

    addAndMakeVisible(enableAudioButton);
    enableAudioButton.setButtonText("Enable Link Audio publishing");
    enableAudioButton.setToggleState(mainPanel->isAbletonLinkAudioEnabled(), dontSendNotification);
    enableAudioButton.addListener(this);

    addAndMakeVisible(peerNameLabel);
    configureLabel(peerNameLabel, "Peer name", FontManager::getInstance().getBodyFont());

    addAndMakeVisible(peerNameEditor);
    peerNameEditor.setText(mainPanel->getAbletonLinkPeerName(), false);
    peerNameEditor.setSelectAllWhenFocused(true);
    peerNameEditor.addListener(this);

    addAndMakeVisible(publishedChannelLabel);
    configureLabel(publishedChannelLabel, "Published channels", FontManager::getInstance().getBodyFont());

    addAndMakeVisible(publishedChannelValue);
    configureLabel(publishedChannelValue,
                   "Pedalboard3 Master 1-2, 3-4, ... (post-master, one stereo pair per Link "
                   "channel, matching your output device)",
                   FontManager::getInstance().getBodyFont());

    addAndMakeVisible(limitationsLabel);
    configureLabel(limitationsLabel,
                   "Remote audio is available through the built-in Link Audio Input graph source. Start/stop sync is not implemented yet.",
                   FontManager::getInstance().getBodyFont());
    limitationsLabel.setColour(Label::textColourId, Colours::darkgrey);

    addAndMakeVisible(peersSectionLabel);
    configureLabel(peersSectionLabel, "Session Peers", FontManager::getInstance().getSubheadingFont());

    addAndMakeVisible(peersHeaderLabel);
    configureLabel(peersHeaderLabel, "Peer and available audio channels", FontManager::getInstance().getBodyFont());

    addAndMakeVisible(peersList);
    peersList.setMultiLine(true);
    peersList.setReadOnly(true);
    peersList.setScrollbarsShown(true);
    peersList.setCaretVisible(false);
    peersList.setColour(TextEditor::backgroundColourId, ColourScheme::getInstance().colours["Dialog Inner Background"]);

    addAndMakeVisible(peerStatusLabel);
    configureLabel(peerStatusLabel, {}, FontManager::getInstance().getBodyFont());

    refreshStatus();
    startTimerHz(2);
}

void LinkAudioSettingsDialog::paint(Graphics& g)
{
    auto& colours = ColourScheme::getInstance().colours;
    g.fillAll(colours["Window Background"]);
    g.setColour(colours["Plugin Border"].withAlpha(0.35f));
    g.drawLine(24.0f, 188.0f, static_cast<float>(getWidth() - 24), 188.0f, 1.0f);
    g.drawRect(getLocalBounds().reduced(24, 252), 1);
}

void LinkAudioSettingsDialog::resized()
{
    const auto bounds = getLocalBounds().reduced(24);
    const auto fieldX = 230;
    const auto fieldWidth = jmax(220, bounds.getRight() - fieldX);

    linkSectionLabel.setBounds(bounds.getX(), 16, bounds.getWidth(), 28);
    linkDescriptionLabel.setBounds(bounds.getX(), 48, bounds.getWidth(), 24);
    enableAudioButton.setBounds(bounds.getX(), 84, 280, 28);
    peerNameLabel.setBounds(bounds.getX(), 124, 180, 24);
    peerNameEditor.setBounds(fieldX, 122, fieldWidth, 28);
    publishedChannelLabel.setBounds(bounds.getX(), 158, 180, 24);
    publishedChannelValue.setBounds(fieldX, 158, fieldWidth, 24);
    limitationsLabel.setBounds(bounds.getX(), 204, bounds.getWidth(), 24);

    peersSectionLabel.setBounds(bounds.getX(), 264, bounds.getWidth(), 28);
    peersHeaderLabel.setBounds(bounds.getX() + 12, 300, bounds.getWidth() - 24, 24);
    peersList.setBounds(bounds.getX() + 12, 326, bounds.getWidth() - 24, getHeight() - 390);
    peerStatusLabel.setBounds(bounds.getX() + 12, getHeight() - 52, bounds.getWidth() - 24, 24);
}

void LinkAudioSettingsDialog::buttonClicked(Button* button)
{
    if (button == &enableAudioButton)
        mainPanel->enableAbletonLinkAudio(enableAudioButton.getToggleState());

    refreshStatus();
}

void LinkAudioSettingsDialog::textEditorReturnKeyPressed(TextEditor& editor)
{
    if (&editor == &peerNameEditor)
        applyPeerName();
}

void LinkAudioSettingsDialog::textEditorFocusLost(TextEditor& editor)
{
    if (&editor == &peerNameEditor)
        applyPeerName();
}

void LinkAudioSettingsDialog::timerCallback()
{
    refreshStatus();
}

void LinkAudioSettingsDialog::applyPeerName()
{
    mainPanel->setAbletonLinkPeerName(peerNameEditor.getText());
    peerNameEditor.setText(mainPanel->getAbletonLinkPeerName(), false);
}

void LinkAudioSettingsDialog::refreshStatus()
{
    const auto peerCount = mainPanel->getAbletonLinkPeerCount();
    const auto channels = mainPanel->getAbletonLinkAudioChannels();
    StringArray lines;

    if (!channels.isEmpty() && mainPanel->getAbletonLinkIncomingChannel() < 0)
        mainPanel->selectAbletonLinkIncomingChannel(0);

    if (!mainPanel->isAbletonLinkAudioEnabled())
        lines.add("Link Audio publishing is disabled.");
    else if (peerCount == 0)
        lines.add("No Link peers discovered on this local network.");
    else
        lines.add(String(peerCount) + " Link peer" + (peerCount == 1 ? "" : "s") + " discovered.");

    if (channels.isEmpty())
        lines.add("No remote Link Audio channels available.");
    else
    {
        lines.add("Remote Link Audio channels:");
        lines.addArray(channels);
    }

    peersList.setText(lines.joinIntoString("\n"), false);
    peerStatusLabel.setText("Link peers: " + String(peerCount) + "    Publishing: " +
                                String(mainPanel->isAbletonLinkAudioEnabled() ? "On" : "Off"),
                            dontSendNotification);
}
