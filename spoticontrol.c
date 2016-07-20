#include <string>
using namespace std;

#include <vdr/osd.h>
#include <vdr/status.h>
#include <vdr/remote.h>
#include "spotidbus.h"
#include "spotiplayer.h"
#include "spoticontrol.h"

extern cSpotifyControl *spotiControl;

cSpotifyControl::cSpotifyControl(void):cControl(Player =
	new cSpotiPlayer())
{
	visible = false;
	running = true;
	displayMenu = NULL;
	ForkAndExec(); // start spotify binary
	dsyslog("spotify: new Control");
	spotiControl = this;
	cStatus::MsgReplaying(this, "MP3", 0, true);
}

cSpotifyControl::~cSpotifyControl()
{
	if (visible)
		Hide();
	running = false;
	dsyslog("spotify: Control in Destroy");
	cStatus::MsgReplaying(this, 0, 0, false);
	if (Player) {
		dsyslog("spotify: send Quit");
		Player->Quit();
		dsyslog("spotify: quit spoti-binary");
		SpotiCmd("Quit");
		DELETENULL(Player);
	}
	spotiControl = NULL;
}

void cSpotifyControl::SpotiExec()
{
	const char *args[2];
	args[0] = "/usr/bin/spotify";
	args[1] = NULL;
	execvp(args[0], (char *const *)args);
	dsyslog("spotify: execvp of '%s' failed: %s", args[0], strerror(errno));
}

void cSpotifyControl::ForkAndExec()
{
	pid_t pid;
	if ((pid = fork()) == -1) {
		dsyslog("spotify: Fork failed");
		return;
	}
	if (!pid) { // child
		SpotiExec();
		return;
	}
	dsyslog("spotify: started binary with pid %d", pid);
}

void cSpotifyControl::ShowProgress(void)
{
	if (displayMenu || (!cOsd::IsOpen())) {
		bool play, forward;
		string artist;
		string title;
		string buffer;
		int speed, Current, Total;

		if (Player && Player->GetReplayMode(play, forward, speed)) {
			if (!visible)
				return;
			if (!displayMenu) {
				dsyslog("spotify:  ShowProgress creates displaymenu");
				displayMenu = Skins.Current()->DisplayReplay(false);
				displayMenu->SetTitle("Spotify PlayerControl");
			}
		}
		if (!Player)
			return;

		artist = getMetaData("xesam:artist");
		title = getMetaData("xesam:title");
		Player->GetIndex(Current, Total, false);
		if (artist != "")
			buffer = artist + " - " + title;
		else
			buffer = title;
		displayMenu->SetMessage(mtInfo, buffer.c_str());
		displayMenu->SetProgress(Current, Total);
		displayMenu->SetMode(play, forward, speed);
		displayMenu->SetCurrent(IndexToHMSF(Current, false));
		displayMenu->SetTotal(IndexToHMSF(Total, false));
		cStatus::MsgReplaying(this, buffer.c_str(), 0, true);
		Skins.Flush();
	}
}

void cSpotifyControl::Show(void)
{
	if ((!displayMenu && !cOsd::IsOpen())) {
		dsyslog("spotify:  Show creates displaymenu");
		displayMenu = Skins.Current()->DisplayReplay(false);
		displayMenu->SetTitle("Spotify Replay");
	}
	visible = true;
	ShowProgress();
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
	eOSState state = osContinue;

	Show();
	if (!Player->Active()) {
		dsyslog("spotify: Control ProcessKey not active");
		return osEnd;
	}
	switch ((int)Key & ~k_Repeat) {
		case kBack:
			dsyslog("spotify: kBack");
			if (visible)
				Hide();
			cRemote::CallPlugin("spotify");
			return osContinue;
		case kBlue:
			dsyslog("spotify: kBlue");
			if (visible)
				Hide();
			return osEnd;
		case kOk:
			dsyslog("spotify: kOk");
			if (visible)
				Hide();
			else
				Show();
			break;
		case kUp:
		case kPlay:
			dsyslog("spotify: kPlay");
			PlayerCmd("PlayPause");
			break;
		case kStop:
			dsyslog("spotify: kStop");
			PlayerCmd("Stop");
			break;
		case kDown:
		case kPause:
			dsyslog("spotify: kPause");
			PlayerCmd("Pause");
			break;
		case kNext:
			dsyslog("spotify: kNext");
			PlayerCmd("Next");
			break;
		case kPrev:
			dsyslog("spotify: kPrev");
			PlayerCmd("Previous");
			break;
		default:
			state = osContinue;
			break;
	}
	return state;
}
