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

#include <vdr/tools.h>
#include "spotiplayer.h"
#include "spotidbus.h"
#include "spoticontrol.h"

// --- cSpotiPlayer --------------------------------------------------

cSpotiPlayer::cSpotiPlayer(void):cPlayer(pmAudioOnly),
cThread("spotify-Player")
{
	run = true;
	dsyslog("spotify: Player created");
	Start();
}

cSpotiPlayer::~cSpotiPlayer()
{
	run = false;
	dsyslog("spotify: delete Player");
	Quit();
}

void cSpotiPlayer::Activate(bool On)
{
	if (On) {
		dsyslog("spotify: Player_Activate On");
		//runcommand Play
	} else {
		run = false;
		Cancel(2);
	}
}

void cSpotiPlayer::Quit(void)
{
	dsyslog("spotify: Player Quit");
	Activate(false);
	Cancel(3);
	Detach();
	cControl::Shutdown();
}

void cSpotiPlayer::Action(void)
{
	while (run) {
		cCondWait::SleepMs(1000);
	}
	dsyslog("spotify: player-Action ended");
}

bool cSpotiPlayer::GetIndex(int &Current, int &Total, bool SnapToIFrame)
{
	// we cannot use Player2.Pos(), this is always 0
	// for the first song - Current might be wrong
	static int Length = 0;
	static time_t Start;

	Current = 0;
	Total = getLength() / 1000000;
	if (Total != Length) {
		Length = Total;
		Start = time(NULL);
	}
	if (!getStatusPlaying()) // paused
		Start++;
	Total = SecondsToFrames(Total);
	Current = SecondsToFrames(time(NULL) - Start);
	if (Current < 0)
		Current=0;
	return true;
}

bool cSpotiPlayer::GetReplayMode(bool & Play, bool & Forward, int &Speed)
{
	//get state
	if (getStatusPlaying())
		Play = true;
	else
		Play = false;
	Forward = true;
	Speed = -1;
	return true;
}
