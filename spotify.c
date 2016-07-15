/*
 * spotify.c: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#include <vdr/plugin.h>
#include <vdr/osd.h>
#include <vdr/status.h>
#include "spotidbus.h"
#include "spoticontrol.h"

static const char *VERSION        = "0.0.1";
static const char *DESCRIPTION    = "display spotify mediadata";
static const char *MAINMENUENTRY  = "Spotify";

cSpotifyControl *spotiControl = NULL;

/// --- cMenuSpotiMain ----------------------------------------------
/// initial spotify Menu
/// shows status of spotify player
/// and starts metadata-player
class cMenuSpotiMain:public cOsdMenu {
   public:
     cMenuSpotiMain(void);
     void Status(void);
     virtual eOSState ProcessKey(eKeys Key);
};

cMenuSpotiMain::cMenuSpotiMain(void):cOsdMenu("Spotify")
{
    dsyslog("spotify: MainMenu created");
    Status();
}

void cMenuSpotiMain::Status(void)
{
    if (getStatusPlaying()) {
        Add(new cOsdItem("Playing", osUnknown, false));
//       Skins.Message(mtStatus,"Playing",3);
    }
    else {
        Add(new cOsdItem("Not Playing", osUnknown, false));
//       Skins.Message(mtStatus,"Not Playing",3);
    }
    Display();
}

eOSState cMenuSpotiMain::ProcessKey(eKeys Key)
{
    eOSState state = cOsdMenu::ProcessKey(Key);
    Status();
    if (state == osUnknown) {
       switch (Key) {
           case kOk:
               dsyslog("spotify: MainMenu kOk");
               if (spotiControl == NULL) {
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
               // player->Quit
               return osEnd;
           default:
               if (Key != kNone)
                    dsyslog("spotify: MainMenu kUnknown");
       }
    }
    return state;
}

class cPluginSpotify : public cPlugin {
private:
  // Add any member variables or functions you may need here.
public:
  cPluginSpotify(void);
  virtual ~cPluginSpotify();
  virtual const char *Version(void) { return VERSION; }
  virtual const char *Description(void) { return DESCRIPTION; }
  virtual const char *CommandLineHelp(void);
  virtual bool ProcessArgs(int argc, char *argv[]);
  virtual bool Initialize(void);
  virtual bool Start(void);
  virtual void Stop(void);
  virtual void Housekeeping(void);
  virtual void MainThreadHook(void);
  virtual cString Active(void);
  virtual time_t WakeupTime(void);
  virtual const char *MainMenuEntry(void) { return MAINMENUENTRY; }
  virtual cOsdObject *MainMenuAction(void);
  virtual cMenuSetupPage *SetupMenu(void);
  virtual bool SetupParse(const char *Name, const char *Value);
  virtual bool Service(const char *Id, void *Data = NULL);
  virtual const char **SVDRPHelpPages(void);
  virtual cString SVDRPCommand(const char *Command, const char *Option, int &ReplyCode);
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
  // Return a message string if shutdown should be postponed
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

cString cPluginSpotify::SVDRPCommand(const char *Command, const char *Option, int &ReplyCode)
{
  // Process SVDRP commands this plugin implements
  return NULL;
}

VDRPLUGINCREATOR(cPluginSpotify); // Don't touch this!
