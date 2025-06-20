
/*
 * Copyright (C) 2016-2022 Johann Friedrichs
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with vdr-plugin-spotify.  If not, see <http://www.gnu.org/licenses/>.
 * */

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <string>
using namespace std;

#include <vdr/osd.h>
#include <vdr/status.h>
#include <vdr/remote.h>
#include <vdr/plugin.h>
#include "spotidbus.h"
#include "spotiplayer.h"
#include "spoticontrol.h"

extern cSpotifyControl *spotiControl;
extern cSpotiPlayer *spotiPlayer;
extern bool conn_ok;
bool change_event;
bool gotLoad;                   // necessary for Stop of MetaData-Loop?
extern cSpotiStat *spotistat;

struct sigaction sa, oact;

extern "C" void HandleSigChld(int sig)
{
   int status = 0;
   pid_t pid = waitpid(-1, &status, WNOHANG);

   if (spotistat->pid == pid) {
      if WIFSIGNALED
         (status)
             dsyslog("spotify: SIGCHILD with signal %d", WTERMSIG(status));
      if WIFEXITED
         (status) {
         dsyslog("spotify: SIGCHILD with RC=%d", WEXITSTATUS(status));
         if (status)
            spotistat->status = eRestartRequested;
         }
      dsyslog("spotify: spotify binary pid=%d exited", pid);
      spotistat->pid = -1;
      if (spotistat->status != eStopRequested)
         cRemote::Put(kBlue);
   }
}

cSpotifyControl::cSpotifyControl(void):cControl(spotiPlayer =
                                                new cSpotiPlayer())
{
   visible = true;
   conn_ok = false;
   change_event = false;
   gotLoad = false;
   spotistat->pid = -1;
   displayMenu = NULL;
   ForkAndExec();               // start spotify binary
   cCondWait::SleepMs(250);
   dsyslog("spotify: new Control");
   spotiControl = this;
   spotistat->status = eStarted;
#ifdef USE_GRAPHTFT
   cPlugin *Plugin = cPluginManager::GetPlugin("graphtftng");

   if (Plugin) {
      tft_exist = true;
   } else {
      tft_exist = false;
      dsyslog("plugin graphtft not started");
   }
#endif
   cStatus::MsgReplaying(this, "MP3", 0, true);
}

cSpotifyControl::~cSpotifyControl()
{
   if (visible)
      Hide();
   dsyslog("spotify: delete Control");
   cStatus::MsgReplaying(this, 0, 0, false);
   if (spotiPlayer) {
      gotLoad = false;
      DELETENULL(spotiPlayer);  // calling quit might not work
      cCondWait::SleepMs(250);
      if ((spotistat->pid > 0) && waitpid(spotistat->pid, 0, WNOHANG) == 0) {   // Child still active
         dsyslog("spotify: terminate spotify binary");
         kill(spotistat->pid, SIGKILL);
         cCondWait::SleepMs(1000);
      }
      if (spotistat->pid > 0 && conn_ok) {      // might hang indefinitly if no communication possible
         int status;

         dsyslog("spotify: wait for child");
         waitpid(spotistat->pid, &status, 0);
         dsyslog("spotify: child exited with status %d", status);
      }
      spotistat->pid = -1;
   }
   if (sigaction(SIGCHLD, &oact, 0) == -1)
      dsyslog("spotify: could not reset signal handler");
   spotiControl = NULL;
   if (spotistat->status == eRestartRequested) {
      spotistat->status = eStopped;
      gotLoad = false;
      dsyslog("spotify: Restart requested");
      cRemote::CallPlugin("spotify");
   } else
      spotistat->status = eStopped;
}

void cSpotifyControl::SpotiExec()
{
   const char *args[2];

   args[0] = "/usr/bin/spotify";
   args[1] = NULL;
   execvp(args[0], (char *const *)args);
   esyslog("spotify: execvp of '%s' failed: %s", args[0], strerror(errno));
}

void cSpotifyControl::ForkAndExec()
{
   sa.sa_handler = &HandleSigChld;
   sigemptyset(&sa.sa_mask);
   if (sigaction(SIGCHLD, &sa, &oact) == -1) {
      dsyslog("spotify: Could not activate SigChld");
      return;
   }

   if ((spotistat->pid = fork()) == -1) {
      return;
   }
   if (!spotistat->pid) {       // child
      setpgid(spotistat->pid, 0);
      /* only for rust env ?
         for (int fd = 3; fd < sysconf (_SC_OPEN_MAX); fd++)
         close (fd);
       */
      SpotiExec();
      // should not be reached
      return;
   }
   dsyslog("spotify: started binary with pid %d", spotistat->pid);
}

void cSpotifyControl::ShowProgress(void)
{
   if (!spotiPlayer)
      return;

   if (visible && !displayMenu) {
      displayMenu = Skins.Current()->DisplayReplay(false);
      displayMenu->SetTitle("Spotify Replay");
      cStatus::MsgReplaying(this, "[SPOT]", 0, true);
      // not working in DisplayReplay
      // displayMenu->SetButtons(tr(""),tr("Previous"),tr("Next"),tr("Exit"));
      // not working displayMenu->SetMessage(mtInfo, "<Blue> to Exit");
      displayMenu->SetMessage(mtInfo, "<Blue> to Exit");
      Skins.Flush();
   }
   if (!conn_ok)
      return;
   if (displayMenu || (!cOsd::IsOpen())) {
      bool play, forward;
      int speed, Current, Total;
      static bool gotMeta = false;
      static int nometa = 0;
      string buffer = "";

      //if (!gotMeta && conn_ok && gotLoad) {     // try again if no info
      if (!gotMeta && conn_ok) {        // try again if no info
         dsyslog("spotify: retry getMeta");
         gotMeta = getMetaData();
         if (gotMeta)
            nometa = 0;
         else {
            if (nometa++ > 4) {
               conn_ok = false;
               // should we restart the player?
               dsyslog("spotify: no Metadata after retries");
               nometa = 0;
            }
         }
      }
      //int length=getLength() / 1000000;
      int length = getLength();

      if (spotistat->total != length) {
         change_event = true;
         spotistat->total = length;
      }
      if (change_event) {
         gotMeta = getMetaData();
         spotistat->current = 0;        // not always correct !
         change_event = false;
      }
      if (!gotMeta) {           // don't use old data
         spotistat->song = "";
         spotistat->artist = "";
         spotistat->album = "";
         return;                // getMetaData failed retry next time
      }
      // now we have metadata
      nometa = 0;
#ifdef USE_GRAPHTFT
      static string lastsong = "";

      if (spotistat->song != lastsong) {
         string arturl = spotistat->artUrl;

         if (arturl.length() > 0) {
            spotiPlayer->RemoveOldCover();
            spotiPlayer->SetCover(arturl.c_str());
         }
         lastsong = spotistat->song;
         if (tft_exist) {
            MusicServicePlayerInfo_1_0 tftStatus;

            tftStatus.filename = spotistat->song.c_str();
            tftStatus.artist = spotistat->artist.c_str();
            tftStatus.album = spotistat->album.c_str();
            tftStatus.genre = "";       // not supplied
            tftStatus.comment = "";     // not supplied
            tftStatus.year = 0; // not supplied
            tftStatus.frequence = 0;    // not supported
            tftStatus.bitrate = 0;      // not supported
            tftStatus.smode = "";       // STEREO not supported
            tftStatus.index = spotistat->track;
            tftStatus.count = 0;        // not yet ??
            tftStatus.status = "";      // not yet (get_playing)
            tftStatus.currentTrack = spotistat->title.c_str();
            tftStatus.loop = false;     // not yet ??
            tftStatus.shuffle = false;  // not yet ??
            tftStatus.shutdown = false; // always disabled
            tftStatus.recording = false;        // not supported
            tftStatus.rating = 0;       // not supported
            cPluginManager::CallFirstService(GRAPHTFT_STATUS_ID, &tftStatus);
         }
      }
#endif

      spotiPlayer->GetReplayMode(play, forward, speed);
      spotiPlayer->GetIndex(Current, Total, false);

      buffer = "[SPOT] " + spotistat->song;
      cStatus::MsgReplaying(this, buffer.c_str(), 0, true);

      if (!visible)
         return;

      displayMenu->SetMessage(mtInfo, spotistat->song.c_str());
      displayMenu->SetProgress(Current, Total);
      displayMenu->SetMode(play, forward, speed);
      displayMenu->SetCurrent(IndexToHMSF(Current, false));
      displayMenu->SetTotal(IndexToHMSF(Total, false));
      Skins.Flush();
   }
}

void cSpotifyControl::Hide(void)
{
   if (displayMenu)
      DELETENULL(displayMenu);
   SetNeedsFastResponse(false);
   visible = false;
}

eOSState cSpotifyControl::ProcessKey(eKeys Key)
{
   ShowProgress();              // called every second
   if (!spotiPlayer || !spotiPlayer->Active()) {
      return osEnd;
   }
   switch ((int)Key & ~k_Repeat) {
   case kBack:
      //dsyslog("spotify: Key Back");
      /* not useful ?
         if (visible)
         Hide();
         cRemote::CallPlugin("spotify");
         return osContinue;
       */
   case kBlue:
      //dsyslog("spotify: Key Blue");
      dsyslog("spotify: Key Back or Blue");
      if (spotistat->status != eRestartRequested)
         spotistat->status = eStopRequested;
      if (visible)
         Hide();
      return osEnd;
   case kOk:
      if (visible)
         Hide();
      else {
         visible = true;
         ShowProgress();
      }
      break;
   case kUp:
   case kDown:
      dsyslog("spotify: Key PlayPause");
      PlayerCmd("PlayPause");
      break;
   case kPlay:
      dsyslog("spotify: Key Play");
      PlayerCmd("Play");
      break;
   case kStop:
      dsyslog("spotify: Key Stop");
      PlayerCmd("Stop");
      break;
   case kPause:
      dsyslog("spotify: Key Pause");
      PlayerCmd("Pause");
      break;
   case kYellow:
   case kNext:
      dsyslog("spotify: Key Next");
      PlayerCmd("Next");
      break;
   case kGreen:
   case kPrev:
      dsyslog("spotify: Key Prev");
      PlayerCmd("Previous");
      break;
   default:
      break;
   }
   return osContinue;
}
