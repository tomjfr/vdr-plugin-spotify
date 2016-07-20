#include <vdr/tools.h>
#include "spotiplayer.h"
#include "spotidbus.h"
#include "spoticontrol.h"

// --- cSpotiPlayer --------------------------------------------------

cSpotiPlayer::cSpotiPlayer(void)
:cPlayer(pmAudioOnly)
, cThread("spotify-Player")
{
    run = true;
    dsyslog("spotify: Player created");
    Start();
}

cSpotiPlayer::~cSpotiPlayer()
{
    run = false;
    dsyslog("spotify: Player detached");
    Quit();
    //Detach();
}

void cSpotiPlayer::Activate(bool On)
{
    if (On) {
        dsyslog("spotify: Player_Activate On");
        //runcommand Play
    } else {
        run = false;
        Cancel(2);
        dsyslog("spotify: Player_Activate False done");
    }
}

void cSpotiPlayer::Action(void)
{
    dsyslog("spotify: playerthread started (pid=%d)", getpid());
#if 0
    char *artist;
    char *title;
#endif
    while (run)
    {
#if 0
        if (!getStatusPlaying()) {
            dsyslog("spotify: Action False done");
            run = false;
        }
        // this is no control !!! cStatus::MsgReplaying(this, "TITLE", "ARTIST", true);
#endif
        cCondWait::SleepMs(1000);
    }
    Activate(false);
}

void cSpotiPlayer::Quit(void)
{
    dsyslog("spotify: Player Quit");
    Cancel(3);
    //Stop();
    Detach();
    cControl::Shutdown();
}

bool cSpotiPlayer::GetIndex(int &Current, int &Total, bool SnapToIFrame)
{
    Current = 0;
    Total = getLength()/1000000;
    Total = SecondsToFrames(Total);
    return true;
}
bool cSpotiPlayer::GetReplayMode(bool & Play, bool &Forward, int &Speed)
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
