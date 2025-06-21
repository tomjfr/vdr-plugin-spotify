
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with vdr-plugin-spotifyd.  If not, see <http://www.gnu.org/licenses/>.
 * */

#include <vdr/tools.h>
#include "spotiplayer.h"
#include "spotidbus.h"
#include "spoticontrol.h"

extern bool conn_ok;
extern cSpotiStat *spotistat;
cMutex _posmutex;

// --- cSpotiPlayer --------------------------------------------------

cSpotiPlayer::cSpotiPlayer(void):cPlayer(pmAudioOnly),
cThread("spotify-Player")
{
  run = true;
  Start();
}

cSpotiPlayer::~cSpotiPlayer()
{
  run = false;
  Quit();
}

void cSpotiPlayer::Activate(bool On)
{
  if (On) {
    dsyslog("spotifyd: activate Player");
    //runcommand Play
  } else {
    run = false;
    Cancel(2);
  }
}

void cSpotiPlayer::Quit(void)
{
  if (conn_ok)
    SpotiCmd("Quit");
  Activate(false);
  Cancel(3);                            // FIXME: double cancel
  Detach();
  cControl::Shutdown();
}

void cSpotiPlayer::Action(void)
{
  while (run) {
    cCondWait::SleepMs(1000);
  }
}

bool cSpotiPlayer::GetIndex(int &Current, int &Total, bool SnapToIFrame)
{
  static time_t lastCall = 0;

  cMutexLock lock(&_posmutex);  // must serialize calls
  time_t now = time(NULL);
  if (now - lastCall > 0) {
    if (spotistat->spoti_playing) {
      // increment 1 sec
      int internalcurrent = spotistat->current + 1;
      if (internalcurrent - spotistat->total < 0)
        spotistat->current = internalcurrent;
    }
    lastCall = now;
  }
  Current = SecondsToFrames(spotistat->current);
  if (Current < 0)                      // can this happen?
    Current = 0;
  Total = SecondsToFrames(spotistat->total);
  return true;
}

bool cSpotiPlayer::GetReplayMode(bool & Play, bool & Forward, int &Speed)
{
  if (spotistat->spoti_playing) {
    Play = true;
  }
  else {
    Play = false;
  }
  Forward = true;
  Speed = -1;
  return true;
}

#ifdef USE_GRAPHTFT
bool cSpotiPlayer::SetCover(const char *artUrl)
{
  char commandString[1024];
  bool result = false;
  int ret = false;

  //dsyslog("nohup /usr/local/bin/spoticopycover.sh %s &", artUrl);
  sprintf((char *)commandString, "nohup /usr/local/bin/spoticopycover.sh %s > /tmp/copycover.log 2>&1 &", artUrl);
  ret = system((const char *)commandString);

  if (ret)
    result = true;

  return result;
}

void cSpotiPlayer::RemoveOldCover(void)
{
  FILE *fp;

  if ((fp = fopen("/tmp/graphTFT.cover", "rb"))) {
    fclose(fp);
    system("rm /tmp/graphTFT.cover");
  }
}
#endif
