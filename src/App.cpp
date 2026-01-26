//	App.cpp - Main application stuff.
//	----------------------------------------------------------------------------
//	This file is part of Pedalboard3, an audio plugin host.
//	Copyright (c) 2009 Niall Moody.
//
//	This program is free software: you can redistribute it and/or modify
//	it under the terms of the GNU General Public License as published by
//	the Free Software Foundation, either version 3 of the License, or
//	(at your option) any later version.
//
//	This program is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//	GNU General Public License for more details.
//
//	You should have received a copy of the GNU General Public License
//	along with this program.  If not, see <http://www.gnu.org/licenses/>.
//	----------------------------------------------------------------------------

#include "InternalFilters.h"
#include "MainPanel.h"

// #include "LADSPAPluginFormat.h"
#include "App.h"
#include "AudioSingletons.h"
#include "ColourScheme.h"
#include "Images.h"
#include "JuceHelperStuff.h"
#include "LogFile.h"
#include "MainTransport.h"
#include "MidiMappingManager.h"
#include "NiallsAudioPluginFormat.h"
#include "OscMappingManager.h"
#include "SettingsManager.h"
#include "TrayIcon.h"

//------------------------------------------------------------------------------
void App::initialise(const String& commandLine)
{
    bool useTrayIcon, startInTray;
    // Initialise our settings file.
    // PropertiesSingleton removal: SettingsManager handles this internally now.

    // Initialize new SettingsManager
    SettingsManager::getInstance().initialise();

#ifndef __APPLE__
    useTrayIcon = SettingsManager::getInstance().getBool("useTrayIcon");
#else
    useTrayIcon = false;
#endif
    startInTray = SettingsManager::getInstance().getBool("startInTray");

    // Check for --debug flag to auto-start logging
    if (commandLine.contains("--debug") || commandLine.contains("-d"))
    {
        LogFile::getInstance().start();
        LogFile::getInstance().logEvent("Pedalboard", "Debug mode enabled - logging started");
        LogFile::getInstance().logEvent("Pedalboard", "Pedalboard3 v3.0 starting...");
    }

    win = new StupidWindow(commandLine, (useTrayIcon && startInTray));

#ifndef __APPLE__
    if (useTrayIcon)
        trayIcon = new TrayIcon(win);
#endif

#ifdef __APPLE__
    MainPanel* pan = dynamic_cast<MainPanel*>(win->getContentComponent());
    if (pan)
        MenuBarModel::setMacMainMenu(pan);
#endif
}

//------------------------------------------------------------------------------
void App::shutdown()
{
#ifdef __APPLE__
    MenuBarModel::setMacMainMenu(0);
#endif

    delete win;

#ifndef __APPLE__
    if (trayIcon)
        delete trayIcon;
#endif

    MainTransport::deleteInstance();
}

//------------------------------------------------------------------------------
void App::anotherInstanceStarted(const String& commandLine)
{
#ifdef __APPLE__
    File initialFile(commandLine);

    if (initialFile.existsAsFile())
    {
        MainPanel* pan = dynamic_cast<MainPanel*>(win->getContentComponent());

        if (initialFile.getFileExtension() == ".pdl" && pan)
            pan->loadDocument(initialFile);
    }
#endif
}

//------------------------------------------------------------------------------
void App::showTrayIcon(bool val)
{
#ifndef __APPLE__
    if (val && !trayIcon)
        trayIcon = new TrayIcon(win);
    else if (!val && trayIcon)
    {
        delete trayIcon;
        trayIcon = 0;
    }
#endif
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
StupidWindow::StupidWindow(const String& commandLine, bool startHidden)
    : DocumentWindow("Pedalboard 3", Colour(0xff333333), DocumentWindow::allButtons)
{
    // Make sure we've loaded all the available plugin formats before we create
    // the main panel.
    {
        InternalPluginFormat* internalFormat = new InternalPluginFormat;
        VST3PluginFormat* vst3Format = new VST3PluginFormat;

        AudioPluginFormatManagerSingleton::getInstance().addFormat(internalFormat);
        AudioPluginFormatManagerSingleton::getInstance().addFormat(vst3Format);
    }

    // Load correct colour scheme.
    {
        // Load correct colour scheme.
        {
            String scheme = SettingsManager::getInstance().getString("colourScheme");

            if (scheme != String())
                ColourScheme::getInstance().loadPreset(scheme);
        }

        laf.reset(new BranchesLAF());
        LookAndFeel::setDefaultLookAndFeel(laf.get());
        setResizable(true, false);
        setContentOwned(mainPanel = new MainPanel(&commandManager), true);
        // mainPanel->setCommandManager(&commandManager);
        centreWithSize(1024, 580);
        setUsingNativeTitleBar(true);
        // setDropShadowEnabled(false);
        if (!startHidden)
            setVisible(true);
#ifndef __APPLE__
        setMenuBar(mainPanel);
#endif

        // Attempts to associate our icon with the window's titlebar.
        getPeer()->setIcon(ImageCache::getFromMemory(Images::icon512_png, Images::icon512_pngSize));

        commandManager.registerAllCommandsForTarget(mainPanel);
        commandManager.registerAllCommandsForTarget(JUCEApplication::getInstance());

        commandManager.getKeyMappings()->resetToDefaultMappings();

        loadKeyMappings();

        addKeyListener(commandManager.getKeyMappings());

        restoreWindowStateFromString(SettingsManager::getInstance().getString("WindowState"));

        // See if we can load a .pdl file from the commandline.
        File initialFile(commandLine);

        if (initialFile.existsAsFile())
        {
            if (initialFile.getFileExtension() == ".pdl")
            {
                mainPanel->loadDocument(initialFile);
                mainPanel->setLastDocumentOpened(initialFile);
                mainPanel->setFile(initialFile);
                mainPanel->setChangedFlag(false);
            }
        }
    }
}

//------------------------------------------------------------------------------
StupidWindow::~StupidWindow()
{
    saveKeyMappings();
    SettingsManager::getInstance().setValue("WindowState", getWindowStateAsString());

    setMenuBar(0);
    setContentOwned(0, true);
    LookAndFeel::setDefaultLookAndFeel(0);

    AudioPluginFormatManagerSingleton::killInstance();
    AudioFormatManagerSingleton::killInstance();
    AudioThumbnailCacheSingleton::killInstance();
    // PropertiesSingleton::killInstance();
}

//------------------------------------------------------------------------------
void StupidWindow::closeButtonPressed()
{
    if (SettingsManager::getInstance().getBool("useTrayIcon"))
    {
        if (isVisible())
            setVisible(false);
        else
        {
            FileBasedDocument::SaveResult result = mainPanel->saveIfNeededAndUserAgrees();

            if (result == FileBasedDocument::savedOk)
                JUCEApplication::quit();
        }
    }
    else
    {
        FileBasedDocument::SaveResult result = mainPanel->saveIfNeededAndUserAgrees();

        if (result == FileBasedDocument::savedOk)
            JUCEApplication::quit();
    }
}

//------------------------------------------------------------------------------
void StupidWindow::updateWindowTitle(const String& filename)
{
    if (filename.isEmpty())
        setName("Pedalboard 3");
    else
        setName("Pedalboard 3 - " + filename);
}

//------------------------------------------------------------------------------
void StupidWindow::loadKeyMappings()
{
    int i;
    OscMappingManager* oscManager = mainPanel->getOscMappingManager();
    MidiMappingManager* midiManager = mainPanel->getMidiMappingManager();
    File mappingsFile = JuceHelperStuff::getAppDataFolder().getChildFile("AppMappings.xm");

    if (mappingsFile.existsAsFile())
    {
        std::unique_ptr<XmlElement> rootXml(XmlDocument::parse(mappingsFile));

        if (rootXml)
        {
            XmlElement* keyMappings = rootXml->getChildByName("KEYMAPPINGS");
            XmlElement* midiMappings = rootXml->getChildByName("MidiMappings");
            XmlElement* oscMappings = rootXml->getChildByName("OscMappings");

            if (keyMappings)
                commandManager.getKeyMappings()->restoreFromXml(*keyMappings);
            if (midiMappings)
            {
                for (i = 0; i < midiMappings->getNumChildElements(); ++i)
                {
                    XmlElement* tempEl = midiMappings->getChildElement(i);

                    if (tempEl->hasTagName("MidiAppMapping"))
                    {
                        MidiAppMapping* newMapping = new MidiAppMapping(midiManager, tempEl);
                        midiManager->registerAppMapping(newMapping);
                    }
                }
            }
            if (oscMappings)
            {
                for (i = 0; i < oscMappings->getNumChildElements(); ++i)
                {
                    XmlElement* tempEl = oscMappings->getChildElement(i);

                    if (tempEl->hasTagName("OscAppMapping"))
                    {
                        OscAppMapping* newMapping = new OscAppMapping(oscManager, tempEl);
                        oscManager->registerAppMapping(newMapping);
                    }
                }
            }
        }
    }
}

//------------------------------------------------------------------------------
void StupidWindow::saveKeyMappings()
{
    int i;
    File mappingsFile = JuceHelperStuff::getAppDataFolder().getChildFile("AppMappings.xm");
    XmlElement* mappingsXml = commandManager.getKeyMappings()->createXml(false).release();
    XmlElement rootXml("Pedalboard3AppMappings");
    XmlElement* midiXml = new XmlElement("MidiMappings");
    XmlElement* oscXml = new XmlElement("OscMappings");
    MidiMappingManager* midiManager = mainPanel->getMidiMappingManager();
    OscMappingManager* oscManager = mainPanel->getOscMappingManager();

    // Add the KeyPress mappings.
    rootXml.addChildElement(mappingsXml);

    // Add the MIDI CC mappings.
    for (i = 0; i < midiManager->getNumAppMappings(); ++i)
        midiXml->addChildElement(midiManager->getAppMapping(i)->getXml());
    rootXml.addChildElement(midiXml);

    // Add the OSC mappings.
    for (i = 0; i < oscManager->getNumAppMappings(); ++i)
        oscXml->addChildElement(oscManager->getAppMapping(i)->getXml());
    rootXml.addChildElement(oscXml);

    // Save to the file.
    rootXml.writeToFile(mappingsFile, "");
}

START_JUCE_APPLICATION(App)
