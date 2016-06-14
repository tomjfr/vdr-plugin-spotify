#include <vdr/tools.h>
#include "spotiplayer.h"
#include "spotidbus.h"

// --- cSpotiPlayer --------------------------------------------------

cSpotiPlayer::cSpotiPlayer(void)
{
    run = true;
    dsyslog("spotify: Player created");
    Start();
}

cSpotiPlayer::~cSpotiPlayer()
{
    run = false;
    dsyslog("spotify: Player detached");
    Detach();
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
        if (!getStatusPlaying())
            run = false;
#if 0
        artist = getMetaData("xesam:artist");
        title = getMetaData("xesam:title");
        dsyslog("got metadata %s : %s", artist,title);
#endif
        // this is no control !!! cStatus::MsgReplaying(this, "TITLE", "ARTIST", true);
        cCondWait::SleepMs(1000);
    }
}
