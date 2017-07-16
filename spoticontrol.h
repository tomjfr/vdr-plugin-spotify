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

#ifndef __SPOTI_CONTROL_H
#define __SPOTI_CONTROL_H
#include "spotiplayer.h"

class cSpotifyControl:public cControl
{
private:
	cSkinDisplayReplay *displayMenu;
	bool starting;
	int visible;
	pid_t pid; // Pid of Childprocess (Spotify binary)
	void SpotiExec(void);
	void ForkAndExec(void);
	void ShowProgress(void);
	virtual void Hide(void);
public:
	 cSpotifyControl(void);
	 virtual ~ cSpotifyControl();
	virtual eOSState ProcessKey(eKeys Key);
};
#endif
