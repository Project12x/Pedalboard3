/*
  ==============================================================================

  This is an automatically generated file created by the Jucer!

  Creation date:  2 Jul 2011 12:03:01pm

  Be careful when adding custom code to these files, as only the code within
  the "//[xyz]" and "//[/xyz]" sections will be retained when the file is loaded
  and re-saved.

  Jucer version: 1.12

  ------------------------------------------------------------------------------

  The Jucer is part of the JUCE library - "Jules' Utility Class Extensions"
  Copyright 2004-6 by Raw Material Software ltd.

  ==============================================================================
*/

//[Headers] You can add your own extra header files here...

#include "ColourScheme.h"

//[/Headers]

#include "AboutPage.h"

//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

//==============================================================================
AboutPage::AboutPage(const String& ip)
    : ipAddress(ip), titleLabel(0), descriptionLabel(0), creditsLabel(0), authorLabel(0), niallmoodyLink(0),
      juceLink(0), versionLabel(0), juceVersionLabel(0), ipAddressLabel(0)
{
    addAndMakeVisible(titleLabel = new Label("titleLabel", "Pedalboard 3"));
    titleLabel->setFont(Font(32.0000f, Font::bold));
    titleLabel->setJustificationType(Justification::centredLeft);
    titleLabel->setEditable(false, false, false);
    titleLabel->setColour(Label::textColourId, Colour(0x80000000));
    titleLabel->setColour(TextEditor::textColourId, Colours::black);
    titleLabel->setColour(TextEditor::backgroundColourId, Colour(0x0));

    addAndMakeVisible(descriptionLabel =
                          new Label("descriptionLabel", "A modern VST3 plugin host for live performance.\nPlugin "
                                                        "parameters easily mapped to MIDI or OSC controllers."));
    descriptionLabel->setFont(Font(15.0000f, Font::plain));
    descriptionLabel->setJustificationType(Justification::topLeft);
    descriptionLabel->setEditable(false, false, false);
    descriptionLabel->setColour(Label::textColourId, Colour(0x80000000));
    descriptionLabel->setColour(TextEditor::textColourId, Colours::black);
    descriptionLabel->setColour(TextEditor::backgroundColourId, Colour(0x0));

    addAndMakeVisible(creditsLabel =
                          new Label("creditsLabel",
                                    "Modernized from Pedalboard2 by Niall Moody.\nBuilt with JUCE 8 and modern C++."));
    creditsLabel->setFont(Font(15.0000f, Font::plain));
    creditsLabel->setJustificationType(Justification::topLeft);
    creditsLabel->setEditable(false, false, false);
    creditsLabel->setColour(Label::textColourId, Colour(0x80000000));
    creditsLabel->setColour(TextEditor::textColourId, Colours::black);
    creditsLabel->setColour(TextEditor::backgroundColourId, Colour(0x0));

    addAndMakeVisible(
        authorLabel = new Label("authorLabel",
                                "Author: Eric Steenwerth (2024-2026)\nOriginal: Niall Moody (2011) | License: GPL v3"));
    authorLabel->setFont(Font(15.0000f, Font::plain));
    authorLabel->setJustificationType(Justification::topLeft);
    authorLabel->setEditable(false, false, false);
    authorLabel->setColour(Label::textColourId, Colour(0x80000000));
    authorLabel->setColour(TextEditor::textColourId, Colours::black);
    authorLabel->setColour(TextEditor::backgroundColourId, Colour(0x0));

    addAndMakeVisible(niallmoodyLink = new HyperlinkButton("niallmoody.com", URL("http://www.niallmoody.com")));
    niallmoodyLink->setTooltip("http://www.niallmoody.com");
    niallmoodyLink->setButtonText("niallmoody.com");

    addAndMakeVisible(juceLink = new HyperlinkButton("juce.com", URL("https://juce.com")));
    juceLink->setTooltip("https://juce.com");
    juceLink->setButtonText("juce.com");

    addAndMakeVisible(versionLabel = new Label("versionLabel", "Version: 2.00"));
    versionLabel->setFont(Font(12.0000f, Font::plain));
    versionLabel->setJustificationType(Justification::centredRight);
    versionLabel->setEditable(false, false, false);
    versionLabel->setColour(Label::textColourId, Colour(0x80000000));
    versionLabel->setColour(TextEditor::textColourId, Colours::black);
    versionLabel->setColour(TextEditor::backgroundColourId, Colour(0x0));

    addAndMakeVisible(juceVersionLabel = new Label("juceVersionLabel", "JUCE Version: 1.5.36"));
    juceVersionLabel->setFont(Font(12.0000f, Font::plain));
    juceVersionLabel->setJustificationType(Justification::centredRight);
    juceVersionLabel->setEditable(false, false, false);
    juceVersionLabel->setColour(Label::textColourId, Colour(0x80000000));
    juceVersionLabel->setColour(TextEditor::textColourId, Colours::black);
    juceVersionLabel->setColour(TextEditor::backgroundColourId, Colour(0x0));

    addAndMakeVisible(ipAddressLabel = new Label("ipAddressLabel", "Current IP Address: 192.168.1.68"));
    ipAddressLabel->setFont(Font(15.0000f, Font::plain));
    ipAddressLabel->setJustificationType(Justification::centredLeft);
    ipAddressLabel->setEditable(false, false, false);
    ipAddressLabel->setColour(Label::textColourId, Colour(0x80000000));
    ipAddressLabel->setColour(TextEditor::textColourId, Colours::black);
    ipAddressLabel->setColour(TextEditor::backgroundColourId, Colour(0x0));

    addAndMakeVisible(kofiLink = new HyperlinkButton("Support on Ko-fi", URL("https://ko-fi.com/worriedwalrus")));
    kofiLink->setTooltip("https://ko-fi.com/worriedwalrus");
    kofiLink->setButtonText("Support on Ko-fi");

    addAndMakeVisible(paypalLink = new HyperlinkButton("Donate via PayPal", URL("https://paypal.me/worriedwalrus")));
    paypalLink->setTooltip("https://paypal.me/worriedwalrus");
    paypalLink->setButtonText("Donate via PayPal");

    //[UserPreSize]

    String tempstr;
    Colour textCol = ColourScheme::getInstance().colours["Text Colour"].withAlpha(0.5f);

    tempstr << "Version: " << JUCEApplication::getInstance()->getApplicationVersion();
    versionLabel->setText(tempstr, dontSendNotification);

    tempstr = "";
    tempstr << SystemStats::getJUCEVersion();
    juceVersionLabel->setText(tempstr, dontSendNotification);

    tempstr = "";
    tempstr << "Current IP Address: " << ipAddress;
    ipAddressLabel->setText(tempstr, dontSendNotification);

    titleLabel->setColour(Label::textColourId, textCol);
    descriptionLabel->setColour(Label::textColourId, textCol);
    creditsLabel->setColour(Label::textColourId, textCol);
    authorLabel->setColour(Label::textColourId, textCol);
    versionLabel->setColour(Label::textColourId, textCol);
    juceVersionLabel->setColour(Label::textColourId, textCol);
    ipAddressLabel->setColour(Label::textColourId, textCol);

    //[/UserPreSize]

    setSize(400, 340);

    //[Constructor] You can add your own custom stuff here..
    //[/Constructor]
}

AboutPage::~AboutPage()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    //[/Destructor_pre]

    delete titleLabel;
    titleLabel = nullptr;
    delete descriptionLabel;
    descriptionLabel = nullptr;
    delete creditsLabel;
    creditsLabel = nullptr;
    delete authorLabel;
    authorLabel = nullptr;
    delete niallmoodyLink;
    niallmoodyLink = nullptr;
    delete juceLink;
    juceLink = nullptr;
    delete versionLabel;
    versionLabel = nullptr;
    delete juceVersionLabel;
    juceVersionLabel = nullptr;
    delete ipAddressLabel;
    ipAddressLabel = nullptr;
    delete kofiLink;
    kofiLink = nullptr;
    delete paypalLink;
    paypalLink = nullptr;

    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void AboutPage::paint(Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    //[/UserPrePaint]

    g.fillAll(Colour(0xffeeece1));

    //[UserPaint] Add your own custom painting code here..

    g.fillAll(ColourScheme::getInstance().colours["Window Background"]);

    //[/UserPaint]
}

void AboutPage::resized()
{
    titleLabel->setBounds(8, 8, 208, 32);
    descriptionLabel->setBounds(16, 48, getWidth() - 16, 56);
    creditsLabel->setBounds(16, 104, getWidth() - 16, 56);
    authorLabel->setBounds(16, 152, getWidth() - 16, 40);
    niallmoodyLink->setBounds(proportionOfWidth(0.5000f) - ((150) / 2), 224, 150, 24);
    juceLink->setBounds(proportionOfWidth(0.5000f) - ((252) / 2), 248, 252, 24);
    versionLabel->setBounds(getWidth() - 154, 0, 150, 24);
    juceVersionLabel->setBounds(getWidth() - 154, 16, 150, 24);
    ipAddressLabel->setBounds(16, 192, getWidth() - 16, 24);
    kofiLink->setBounds(proportionOfWidth(0.5000f) - ((150) / 2), 280, 150, 24);
    paypalLink->setBounds(proportionOfWidth(0.5000f) - ((150) / 2), 304, 150, 24);
    //[UserResized] Add your own custom resize handling here..
    //[/UserResized]
}

//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...
//[/MiscUserCode]

//==============================================================================
#if 0
/*  -- Jucer information section --

    This is where the Jucer puts all of its metadata, so don't change anything in here!

BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="AboutPage" componentName=""
                 parentClasses="public Component" constructorParams="const String&amp; ip"
                 variableInitialisers="ipAddress(ip)" snapPixels="8" snapActive="1"
                 snapShown="1" overlayOpacity="0.330000013" fixedSize="0" initialWidth="400"
                 initialHeight="280">
  <BACKGROUND backgroundColour="ffeeece1"/>
  <LABEL name="titleLabe" id="dd6542ae3fcb9b5c" memberName="titleLabe"
         virtualName="" explicitFocusOrder="0" pos="8 8 208 32" textCol="80000000"
         edTextCol="ff000000" edBkgCol="0" labelText="Pedalboard 2" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="32" bold="1" italic="0" justification="33"/>
  <LABEL name="descriptionLabe" id="de5989d753bc0220" memberName="descriptionLabe"
         virtualName="" explicitFocusOrder="0" pos="16 48 16M 56" textCol="80000000"
         edTextCol="ff000000" edBkgCol="0" labelText="A simple plugin host intended for live use, with plugin&#10;parameters easily mapped to MIDI or Open Sound Control inputs."
         editableSingleClick="0" editableDoubleClick="0" focusDiscardsChanges="0"
         fontname="Default font" fontsize="15" bold="0" italic="0" justification="9"/>
  <LABEL name="creditsLabe" id="b05a22c6f03e6de8" memberName="creditsLabe"
         virtualName="" explicitFocusOrder="0" pos="16 104 16M 56" textCol="80000000"
         edTextCol="ff000000" edBkgCol="0" labelText="Written using the JUCE library, with sections taken from&#10;the 'audio plugin host' example code."
         editableSingleClick="0" editableDoubleClick="0" focusDiscardsChanges="0"
         fontname="Default font" fontsize="15" bold="0" italic="0" justification="9"/>
  <LABEL name="authorLabe" id="2e1803d1a35e2f3" memberName="authorLabe"
         virtualName="" explicitFocusOrder="0" pos="16 152 16M 40" textCol="80000000"
         edTextCol="ff000000" edBkgCol="0" labelText="Author: Niall Moody&#10;License: GPL v3"
         editableSingleClick="0" editableDoubleClick="0" focusDiscardsChanges="0"
         fontname="Default font" fontsize="15" bold="0" italic="0" justification="9"/>
  <HYPERLINKBUTTON name="niallmoodyLink" id="6d1145279dcd5cdb" memberName="niallmoodyLink"
                   virtualName="" explicitFocusOrder="0" pos="50%c 224 150 24" tooltip="http://www.niallmoody.com"
                   buttonText="niallmoody.com" connectedEdges="0" needsCallback="0"
                   radioGroupId="0" url="http://www.niallmoody.com"/>
  <HYPERLINKBUTTON name="juceLink" id="4a2a19a55d5d801b" memberName="juceLink" virtualName=""
                   explicitFocusOrder="0" pos="50%c 248 252 24" tooltip="http://www.rawmaterialsoftware.com/juce"
                   buttonText="rawmaterialsoftware.com/juce" connectedEdges="0"
                   needsCallback="0" radioGroupId="0" url="http://www.rawmaterialsoftware.com/juce"/>
  <LABEL name="versionLabe" id="7874d6b2fe271548" memberName="versionLabe"
         virtualName="" explicitFocusOrder="0" pos="154R 0 150 24" textCol="80000000"
         edTextCol="ff000000" edBkgCol="0" labelText="Version: 2.00" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="12" bold="0" italic="0" justification="34"/>
  <LABEL name="juceVersionLabe" id="4488baef6ec2c53b" memberName="juceVersionLabe"
         virtualName="" explicitFocusOrder="0" pos="154R 16 150 24" textCol="80000000"
         edTextCol="ff000000" edBkgCol="0" labelText="JUCE Version: 1.5.36"
         editableSingleClick="0" editableDoubleClick="0" focusDiscardsChanges="0"
         fontname="Default font" fontsize="12" bold="0" italic="0" justification="34"/>
  <LABEL name="ipAddressLabe" id="481375ac125b41f8" memberName="ipAddressLabe"
         virtualName="" explicitFocusOrder="0" pos="16 192 16M 24" textCol="80000000"
         edTextCol="ff000000" edBkgCol="0" labelText="Current IP Address: 192.168.1.68"
         editableSingleClick="0" editableDoubleClick="0" focusDiscardsChanges="0"
         fontname="Default font" fontsize="15" bold="0" italic="0" justification="33"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif
