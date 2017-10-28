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

#include <sys/types.h>
#include <sys/wait.h>
#include <string>
using namespace std;

#include <vdr/osd.h>
#include <vdr/status.h>
#include <vdr/remote.h>
#include "spotidbus.h"
#include "spotiplayer.h"
#include "spoticontrol.h"

extern cSpotifyControl *spotiControl;
extern cSpotiPlayer *spotiPlayer;
extern bool conn_ok;

cSpotifyControl::cSpotifyControl(void):cControl(spotiPlayer =
	new cSpotiPlayer())
{
	visible = true;
	conn_ok = false;
	pid = -1;
	starting = true;
	displayMenu = NULL;
	ForkAndExec(); // start spotify binary
	cCondWait::SleepMs(500);
	dsyslog("spotify: new Control");
	spotiControl = this;
	cStatus::MsgReplaying(this, "MP3", 0, true);
}

cSpotifyControl::~cSpotifyControl()
{
	if (visible)
		Hide();
	dsyslog("spotify: delete Control");
	cStatus::MsgReplaying(this, 0, 0, false);
	if (spotiPlayer) {
		spotiPlayer->Quit();
		SpotiCmd("Quit");
		cCondWait::SleepMs(250);
		if (waitpid(pid,0,WNOHANG)==0) { // Child still active
			dsyslog("spotify: had to kill binary");
			kill(pid,SIGTERM);
		}
		if (pid > 0) {
			dsyslog("spotify: wait for child");
			waitpid(pid,0,0);
		}
		pid = -1;
		DELETENULL(spotiPlayer);
	}
	spotiControl = NULL;
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
	if ((pid = fork()) == -1) {
		return;
	}
	if (!pid) { // child
		SpotiExec();
		// should not be reached
		return;
	}
	dsyslog("spotify: started binary with pid %d", pid);
}

void cSpotifyControl::ShowProgress(void)
{
	if (conn_ok && starting) // 1st succesful connect to binary
		starting = false;
	if (!conn_ok)
		if (!starting) { // error in running binary
			starting = true; // ??
			if (spotiPlayer) {
				dsyslog("spotify: connection to binary failed");
				spotiPlayer->Quit();
			}
			return;
		}
	if (displayMenu || (!cOsd::IsOpen())) {
		bool play, forward;
		string artist;
		string title;
		string buffer;
		int speed, Current, Total;

                if (!spotiPlayer) return;

		artist = getMetaData("xesam:artist");
		title = getMetaData("xesam:title");
		spotiPlayer->GetReplayMode(play, forward, speed);
		spotiPlayer->GetIndex(Current, Total, false);
		if (artist != "")
			buffer = "[SPOT] " + artist + " - " + title;
		else
			buffer = "[SPOT] " + title;
		cStatus::MsgReplaying(this, buffer.c_str(), 0, true);

		if (!visible)
			return;
		if (!displayMenu) {
			displayMenu = Skins.Current()->DisplayReplay(false);
			displayMenu->SetTitle("Spotify Replay");
			// not working in DisplayReplay
			// displayMenu->SetButtons(tr(""),tr("Previous"),tr("Next"),tr("Exit"));
			// not working displayMenu->SetMessage(mtInfo, "<Blue> to Exit");
		}

		displayMenu->SetMessage(mtInfo, buffer.c_str());
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
	ShowProgress(); // called every second
	if (!spotiPlayer || !spotiPlayer->Active()) {
		return osEnd;
	}
	switch ((int)Key & ~k_Repeat) {
		case kBack:
			if (visible)
				Hide();
			cRemote::CallPlugin("spotify");
			return osContinue;
		case kBlue:
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
		case kPlay:
			PlayerCmd("PlayPause");
			break;
		case kStop:
			PlayerCmd("Stop");
			break;
		case kDown:
		case kPause:
			PlayerCmd("Pause");
			break;
		case kYellow:
		case kNext:
			PlayerCmd("Next");
			break;
		case kGreen:
		case kPrev:
			PlayerCmd("Previous");
			break;
		default:
			break;
	}
	return osContinue;
}
