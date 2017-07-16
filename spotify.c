/*
 * Copyright (C) 2016-2017 Johann Friedrichs
 *
 * This file is part of vdr-plugin-spotify.
 *
 * vdr-plugin-spotify is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * vdr-plugin-spotify is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with vdr-plugin-spotify.  If not, see <http://www.gnu.org/licenses/>.
 * */

#include <vdr/plugin.h>
#include <vdr/osd.h>
#include <vdr/status.h>
#include "spotidbus.h"
#include "spoticontrol.h"

static const char *VERSION = "0.7";
static const char *DESCRIPTION = "spotify dbus interface";
static const char *MAINMENUENTRY = "Spotify";

cSpotifyControl *spotiControl = NULL;
cSpotiPlayer *spotiPlayer = NULL;

/// --- cMenuSpotiMain ----------------------------------------------
/// initial spotify Menu
/// and starts player
class cMenuSpotiMain:public cOsdMenu
{
public:
	cMenuSpotiMain(void);
	void Status(void);
	virtual eOSState ProcessKey(eKeys Key);
};

cMenuSpotiMain::cMenuSpotiMain(void):cOsdMenu("Spotify")
{
	dsyslog("spotify: MainMenu created");
	SetHelp(tr(""), tr(""), tr("Start Player"), tr("Exit"));
}

eOSState cMenuSpotiMain::ProcessKey(eKeys Key)
{
	eOSState state = cOsdMenu::ProcessKey(Key);

	if (state == osUnknown) {
		switch (Key) {
			case kOk:
			case kYellow:
				dsyslog("spotify: MainMenu kOk");
				if (!spotiControl) {
					cControl::Launch(new cSpotifyControl());
				} else {
					dsyslog("spotify: use existing Control");
					cControl::Attach();
				}
				return osEnd;
			case kBack:
				dsyslog("spotify: MainMenu kBack");
				return osBack;
			case kBlue:
				dsyslog("spotify: MainMenu kBlue");
				if (spotiPlayer)
					spotiPlayer->Quit();
				return osEnd;
			default:
				if (Key != kNone)
					dsyslog("spotify: MainMenu kUnknown");
		}
	}
	return state;
}

class cPluginSpotify:public cPlugin
{
private:
	// Add any member variables or functions you may need here.
public:
	cPluginSpotify(void);
	virtual ~ cPluginSpotify();
	virtual const char *Version(void)
	{
		return VERSION;
	}
	virtual const char *Description(void)
	{
		return DESCRIPTION;
	}
	virtual const char *CommandLineHelp(void);
	virtual bool ProcessArgs(int argc, char *argv[]);
	virtual bool Initialize(void);
	virtual bool Start(void);
	virtual void Stop(void);
	virtual void Housekeeping(void);
	virtual void MainThreadHook(void);
	virtual cString Active(void);
	virtual time_t WakeupTime(void);
	virtual const char *MainMenuEntry(void)
	{
		return MAINMENUENTRY;
	}
	virtual cOsdObject *MainMenuAction(void);
	virtual cMenuSetupPage *SetupMenu(void);
	virtual bool SetupParse(const char *Name, const char *Value);
	virtual bool Service(const char *Id, void *Data = NULL);
	virtual const char **SVDRPHelpPages(void);
	virtual cString SVDRPCommand(const char *Command, const char *Option,
		int &ReplyCode);
};

cPluginSpotify::cPluginSpotify(void)
{
	// Initialize any member variables here.
	// DON'T DO ANYTHING ELSE THAT MAY HAVE SIDE EFFECTS, REQUIRE GLOBAL
	// VDR OBJECTS TO EXIST OR PRODUCE ANY OUTPUT!
}

cPluginSpotify::~cPluginSpotify()
{
	// Clean up after yourself!
}

const char *cPluginSpotify::CommandLineHelp(void)
{
	// Return a string that describes all known command line options.
	return NULL;
}

bool cPluginSpotify::ProcessArgs(int argc, char *argv[])
{
	// Implement command line argument processing here if applicable.
	return true;
}

bool cPluginSpotify::Initialize(void)
{
	// Initialize any background activities the plugin shall perform.
	return true;
}

bool cPluginSpotify::Start(void)
{
	// Start any background activities the plugin shall perform.
	return true;
}

void cPluginSpotify::Stop(void)
{
	// Stop any background activities the plugin is performing.
}

void cPluginSpotify::Housekeeping(void)
{
	// Perform any cleanup or other regular tasks.
}

void cPluginSpotify::MainThreadHook(void)
{
	// Perform actions in the context of the main program thread.
	// WARNING: Use with great care - see PLUGINS.html!
}

cString cPluginSpotify::Active(void)
{
	// do we wish shutdown to be postponed then we need a string
	return NULL;
}

time_t cPluginSpotify::WakeupTime(void)
{
	// Return custom wakeup time for shutdown script
	return 0;
}

cOsdObject *cPluginSpotify::MainMenuAction(void)
{
	// Perform the action when selected from the main VDR menu.
	return new cMenuSpotiMain;
}

cMenuSetupPage *cPluginSpotify::SetupMenu(void)
{
	// Return a setup menu in case the plugin supports one.
	return NULL;
}

bool cPluginSpotify::SetupParse(const char *Name, const char *Value)
{
	// Parse your own setup parameters and store their values.
	return false;
}

bool cPluginSpotify::Service(const char *Id, void *Data)
{
	// Handle custom service requests from other plugins
	return false;
}

const char **cPluginSpotify::SVDRPHelpPages(void)
{
	// Return help text for SVDRP commands this plugin implements
	return NULL;
}

cString cPluginSpotify::SVDRPCommand(const char *Command, const char *Option,
	int &ReplyCode)
{
	// Process SVDRP commands this plugin implements
	return NULL;
}

VDRPLUGINCREATOR(cPluginSpotify);				// Don't touch this!
