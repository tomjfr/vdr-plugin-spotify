
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
bool gotLoad; // necessary for Stop of MetaData-Loop?
extern cString spotifydConf;
extern cSpotiStat *spotistat;

struct sigaction sa, oact;

extern "C" void HandleSigChld(int sig)
{
  int status = 0;
  pid_t pid = waitpid(-1, &status, WNOHANG);

  if (status && (spotistat->pid == pid)) {
    if WIFSIGNALED(status)
        dsyslog("spotifyd: SIGCHILD with signal %d", WTERMSIG(status));
    if WIFEXITED(status) {
        dsyslog("spotifyd: SIGCHILD with RC=%d", WEXITSTATUS(status));
	    spotistat->status = eRestartRequested;
    }
    dsyslog("spotifyd: spotifyd binary pid=%d exited and restart requested", pid);
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
  spotistat->status = eStopped;
  displayMenu = NULL;
  fifoThread = 0;
  spotiControl = this;
  ForkAndExec();
  cCondWait::SleepMs(250);
  dsyslog("spotifyd: new Control");
  spotistat->status=eStarted;
  fifoThread = new cFifo();
  fifoThread->Start();

#ifdef USE_GRAPHTFT
  cPlugin* Plugin = cPluginManager::GetPlugin("graphtftng");
  if (Plugin) {
     tft_exist = true;
  }
  else {
     tft_exist = false;
  }
#endif
  cStatus::MsgReplaying(this, "MP3", 0, true);
}

cSpotifyControl::~cSpotifyControl()
{
  if (visible)
    Hide();
  dsyslog("spotifyd: delete Control");
  cStatus::MsgReplaying(this, 0, 0, false);
  if (spotiPlayer) {
    gotLoad = false;
    DELETENULL(spotiPlayer);            // calling quit might not work
    cCondWait::SleepMs(250);
    if (waitpid(spotistat->pid, 0, WNOHANG) == 0) { // Child still active
      dsyslog("spotifyd: had to kill binary");
      kill(spotistat->pid, SIGTERM);
      cCondWait::SleepMs(250);
    }
    if (spotistat->pid > 0 && conn_ok) {  // might hang indefinitly if no communication possible
      int status;

      dsyslog("spotifyd: wait for child");
      waitpid(spotistat->pid, &status, 0);
      dsyslog("spotifyd: child exited with status %d", status);
    }
    spotistat->pid = -1;
  }
  if (sigaction(SIGCHLD, &oact, 0) == -1)
    esyslog("spotifyd: could not reset signal handler");
  spotiControl = NULL;
  if (fifoThread) delete fifoThread;
  if (spotistat->status == eRestartRequested) {
    spotistat->status=eStopped;
    gotLoad = false;
    dsyslog("spotifyd: Restart requested");
    cRemote::CallPlugin("spotifyd");
  }
  else
    spotistat->status=eStopped;
}

void cSpotifyControl::SpotiExec()
{
  //const char *args[7];
  const char *args[8];
  const char *myConf = spotifydConf;

  //dsyslog("spotifyd SpotiExec()");
  args[0] = "/usr/local/bin/spotifyd";
  args[1] = "--config-path";
  args[2] = myConf; // set to CONFIGDIR(spotifyd)/spotifyd.conf
  args[3] = "--on-song-change-hook";
  args[4] = "/usr/local/bin/vdr-spotify-onevent.sh";
  args[5] = "--no-daemon";
  args[6] = "--verbose"; // TODO make configurable in conf.d
  args[7] = NULL;
  dsyslog("spotifyd: exec %s %s %s %s %s %s %s", args[0], args[1], args[2],
    args[3], args[4], args[5], args[6]);
  //dsyslog("spotifyd: exec %s %s %s %s %s %s", args[0], args[1], args[2],
    //args[3], args[4], args[5]);
  execvp(args[0], (char *const *)args);
  esyslog("spotifyd: execvp of '%s' failed: %s", args[0], strerror(errno));
}

void cSpotifyControl::ForkAndExec()
{
  //dsyslog("spotifyd ForkAndExec()");
  spotistat->bus = "";
  spotistat->pid = 0;
  sa.sa_handler = &HandleSigChld;
  sigemptyset(&sa.sa_mask);
  if (sigaction(SIGCHLD, &sa, &oact) == -1) {
    esyslog("spotifyd: Could not activate SigChld");
    return;
  }

  if ((spotistat->pid = fork()) == -1) {
    esyslog("spotifyd: Could not fork child");
    return;
  }
  if (!spotistat->pid) {                // child
    setpgid(spotistat->pid, 0);
    for (int fd = 3; fd < sysconf(_SC_OPEN_MAX); fd++)
      close(fd);
    SpotiExec();
    esyslog("spotifyd: Fork after SpotiExec() should not be reached");
    return;
  }
  spotistat->bus = cString::sprintf("org.mpris.MediaPlayer2.spotifyd.instance%d", spotistat->pid);
  dsyslog("spotifyd: started child with pid %d instance=%s", spotistat->pid, (const char *)spotistat->bus);
  return;
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
    static int metatries = 0;
    string buffer = "";

    //if (!gotMeta && conn_ok && gotLoad) {     // try again if no info
    if (!gotMeta && conn_ok && spotistat->trackId.length()) {     // try again if no info
      //dsyslog("spotifyd: 1st getMeta");
      gotMeta = getMetaData();
      if (gotMeta)
        metatries = 0;
      else {
        if (metatries++ > 4) {
          // should we restart the player?
          esyslog("spotifyd: no Metadata after retries");
          conn_ok = false;
          metatries = 0;
          return;
        }
      }
    }
    if (change_event) { // we got event with new trackid
      //dsyslog("spotifyd: change_event getMeta");
      gotMeta = getMetaData();
      if (!spotistat->total) // fallback if we miss the first LOAD
        spotistat->total = getLength() / 1000000;
      dsyslog("spotifyd change_event total = %d, pos = %d", spotistat->total,
          spotistat->current);
      change_event = false;
    }
    if (!gotMeta) { // don't use old data
      spotistat->song = "";
      spotistat->artist = "";
      spotistat->album = "";
      esyslog("spotifyd: getMeta failed after change_event");
      return; // getMetaData failed retry next time
    }
    // now we have metadata
    metatries = 0;
#ifdef USE_GRAPHTFT
    static string trackid = "";

    if (spotistat->trackId != trackid) {
      trackid = spotistat->trackId;
      string arturl = spotistat->artUrl;
      if (arturl.length() > 0) {
        spotiPlayer->RemoveOldCover();
        spotiPlayer->SetCover(arturl.c_str());
      }
      if (tft_exist) {
        MusicServicePlayerInfo_1_0 tftStatus;
        tftStatus.filename = spotistat->song.c_str();
        tftStatus.artist = spotistat->artist.c_str();
        tftStatus.album = spotistat->album.c_str();
        tftStatus.genre = ""; // not supplied
        tftStatus.comment = ""; // not supplied
        tftStatus.year = 0; // not supplied
        tftStatus.frequence = 0; // not supported
        tftStatus.bitrate = 0; // not supported
        tftStatus.smode = ""; // STEREO not supported
        tftStatus.index = spotistat->track;
        tftStatus.count = 0; // not yet ??
        tftStatus.status = ""; // not yet (get_playing)
        tftStatus.currentTrack = spotistat->title.c_str();
        tftStatus.loop = spotistat->repeat;
        tftStatus.shuffle = spotistat->shuffle;
        tftStatus.shutdown = false; // always disabled
        tftStatus.recording = false; // not supported
        tftStatus.rating = 0; // not supported
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
  ShowProgress();                       // called every second
  if (!spotiPlayer || !spotiPlayer->Active()) {
    return osEnd;
  }
  switch ((int)Key & ~k_Repeat) {
    case kBack:
      dsyslog("spotifyd: Key Back");
      if (visible)
        Hide();
      cRemote::CallPlugin("spotifyd");
      return osContinue;
    case kBlue:
      dsyslog("spotifyd: Key Blue");
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
      dsyslog("spotifyd: Key PlayPause");
      PlayerCmd("PlayPause");
      break;
    case kPlay:
      dsyslog("spotifyd: Key Play");
      PlayerCmd("Play");
      break;
    case kStop:
      dsyslog("spotifyd: Key Stop");
      PlayerCmd("Stop");
      break;
    case kPause:
      dsyslog("spotifyd: Key Pause");
      PlayerCmd("Pause");
      break;
    case kYellow:
    case kNext:
      dsyslog("spotifyd: Key Next");
      PlayerCmd("Next");
      break;
    case kGreen:
    case kPrev:
      dsyslog("spotifyd: Key Prev");
      PlayerCmd("Previous");
      break;
    default:
      break;
  }
  return osContinue;
}
