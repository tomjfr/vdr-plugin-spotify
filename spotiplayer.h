
/*
Copyright (C) 2016-2022 Johann Friedrichs

This file is part of vdr-plugin-spotify.

vdr-plugin-spotify is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

vdr-plugin-spotify is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with vdr-plugin-spotify.	If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef ___SPOTIPLAYER_H
#define ___SPOTIPLAYER_H

#include <string>
#include <vdr/player.h>
#include <vdr/thread.h>
#include <vdr/status.h>
using namespace std;

enum eStatus
    { eStarted, eStopped, eStopRequested, eRestartRequested, eConnected };

class cSpotiStat {
 public:
   pid_t pid;
   string song;
   string artist;
   string album;
   string title;
   string artUrl;
   int track;
   int total;
   int current;
   eStatus status;
   bool spoti_playing;
    cSpotiStat(void) {
      song = artist = album = title = artUrl = "";
      track = total = current = 0;
      spoti_playing = false;
}};

class cSpotiPlayer:public cPlayer, public cThread {
 private:
   bool run;
 protected:
    virtual void Activate(bool On);
   virtual void Action(void);
 public:
    cSpotiPlayer(void);
    virtual ~ cSpotiPlayer();
   virtual bool GetIndex(int &Current, int &Total, bool SnapToIFrame = false);
   virtual bool GetReplayMode(bool &Play, bool &Forward, int &Speed);
   void Quit(void);
   bool Active(void) {
      return run;
   }
#ifdef USE_GRAPHTFT
   bool SetCover(const char *arturl);
   void RemoveOldCover(void);
#endif
};

#endif
