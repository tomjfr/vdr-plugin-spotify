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
	static int Length = 0;
	static time_t Start;

	Current = 0;
	Total = getLength() / 1000000;
	if (Total != Length) {
		Length = Total;
		Start = time(NULL);
	}
	if (!getStatusPlaying())
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
