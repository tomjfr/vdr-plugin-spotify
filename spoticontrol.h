
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

#ifndef __SPOTI_CONTROL_H
#define __SPOTI_CONTROL_H
#include "spotiplayer.h"

class cSpotifyControl:public cControl {
 private:
   cSkinDisplayReplay * displayMenu;
   int visible;
   void SpotiExec(void);
   void ForkAndExec(void);
   void ShowProgress(void);
   virtual void Hide(void);
#ifdef USE_GRAPHTFT
   bool tft_exist;
#endif
 public:
    cSpotifyControl(void);
    virtual ~ cSpotifyControl();
   virtual eOSState ProcessKey(eKeys Key);
};

#ifdef USE_GRAPHTFT
struct MusicServicePlayerInfo_1_0 {
   const char *filename;
   const char *artist;
   const char *album;
   const char *genre;
   const char *comment;
   int year;
   double frequence;
   int bitrate;
   const char *smode;
   int index;                   // current index in tracklist
   int count;                   // total items in tracklist
   const char *status;          // player status
   const char *currentTrack;
   bool loop;
   bool shuffle;
   bool shutdown;
   bool recording;
   int rating;
};
struct MusicServiceInfo_1_0 {
   const char *info;
};

#define GRAPHTFT_STATUS_ID      "GraphTftStatus-v1.0"
#define GRAPHTFT_INFO_ID "GraphTftInfo-v1.0"
#endif
#endif
