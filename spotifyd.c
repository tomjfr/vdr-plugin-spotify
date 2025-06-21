
/*
 * Copyright (C) 2016-2019 Johann Friedrichs
 *
 * This file is part of vdr-plugin-spotifyd.
 *
 * vdr-plugin-spotifyd is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * vdr-plugin-spotifyd is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with vdr-plugin-spotifyd.  If not, see <http://www.gnu.org/licenses/>.
 * */

#include <vdr/plugin.h>
#include <vdr/osd.h>
#include <vdr/status.h>
#include <vdr/remote.h>
#include "spotidbus.h"
#include "spoticontrol.h"

static const char *VERSION = "0.9";
static const char *DESCRIPTION = "spotifyd dbus interface";
static const char *MAINMENUENTRY = "Spotifyd";

cSpotifyControl *spotiControl = NULL;
cSpotiPlayer *spotiPlayer = NULL;
cString spotifydConf = "";
cSpotiStat *spotistat = NULL;

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
  SetHelp(NULL, NULL, tr("Start Player"), tr("Exit"));
  cRemote::Put(kYellow);                // immediate start
}

eOSState cMenuSpotiMain::ProcessKey(eKeys Key)
{
  eOSState state = cOsdMenu::ProcessKey(Key);

  if (state == osUnknown) {
    switch (Key) {
      case kOk:
      case kYellow:
        dsyslog("spotifyd: main Key Ok/Yellow");
        if (!spotiControl) {
          dsyslog("spotifyd: Mainmenu: create new Control");
          cControl::Launch(new cSpotifyControl());
        } else {
          dsyslog("spotifyd: Mainmenu: use existing Control");
          cControl::Attach();
        }
        return osEnd;
      case kBack:
        dsyslog("spotifyd: main Key Back");
        return osBack;
      case kBlue:
        dsyslog("spotifyd: main Key Blue");
        return osEnd;
      default:
        if (Key != kNone)
          dsyslog("spotifyd: main unknown Key %d", Key);
    }
  }
  return state;
}

class cPluginSpotifyd:public cPlugin
{
private:
  // Add any member variables or functions you may need here.
public:
  cPluginSpotifyd(void);
  virtual ~ cPluginSpotifyd();
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

cPluginSpotifyd::cPluginSpotifyd(void)
{
  // Initialize any member variables here.
  // DON'T DO ANYTHING ELSE THAT MAY HAVE SIDE EFFECTS, REQUIRE GLOBAL
  // VDR OBJECTS TO EXIST OR PRODUCE ANY OUTPUT!
}

cPluginSpotifyd::~cPluginSpotifyd()
{
  // Clean up after yourself!
}

const char *cPluginSpotifyd::CommandLineHelp(void)
{
  // Return a string that describes all known command line options.
  return NULL;
}

bool cPluginSpotifyd::ProcessArgs(int argc, char *argv[])
{
  // Implement command line argument processing here if applicable.
  return true;
}

bool cPluginSpotifyd::Initialize(void)
{
  // Initialize any background activities the plugin shall perform.
  return true;
}

bool cPluginSpotifyd::Start(void)
{
  // Start any background activities the plugin shall perform.
  spotifydConf =
    cString::sprintf("%s/spotifyd.conf", cPlugin::ConfigDirectory("spotifyd"));
  spotistat = new cSpotiStat;
  return true;
}

void cPluginSpotifyd::Stop(void)
{
  // Stop any background activities the plugin is performing.
  if (spotiPlayer)
    spotiPlayer->Quit();
}

void cPluginSpotifyd::Housekeeping(void)
{
  // Perform any cleanup or other regular tasks.
}

void cPluginSpotifyd::MainThreadHook(void)
{
  // Perform actions in the context of the main program thread.
  // WARNING: Use with great care - see PLUGINS.html!
}

cString cPluginSpotifyd::Active(void)
{
  // do we wish shutdown to be postponed then we need a string
  if (spotiPlayer)
    return tr("Spotify Player active");
  return NULL;
}

time_t cPluginSpotifyd::WakeupTime(void)
{
  // Return custom wakeup time for shutdown script
  return 0;
}

cOsdObject *cPluginSpotifyd::MainMenuAction(void)
{
  // Perform the action when selected from the main VDR menu.
  return new cMenuSpotiMain;
}

cMenuSetupPage *cPluginSpotifyd::SetupMenu(void)
{
  // Return a setup menu in case the plugin supports one.
  return NULL;
}

bool cPluginSpotifyd::SetupParse(const char *Name, const char *Value)
{
  // Parse your own setup parameters and store their values.
  return false;
}

bool cPluginSpotifyd::Service(const char *Id, void *Data)
{
  // Handle custom service requests from other plugins
  return false;
}

const char **cPluginSpotifyd::SVDRPHelpPages(void)
{
  // Return help text for SVDRP commands this plugin implements
  return NULL;
}

cString cPluginSpotifyd::SVDRPCommand(const char *Command, const char *Option,
  int &ReplyCode)
{
  // Process SVDRP commands this plugin implements
  return NULL;
}

VDRPLUGINCREATOR(cPluginSpotifyd);       // Don't touch this!
