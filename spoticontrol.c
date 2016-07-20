#include <string>
using namespace std;
#include <vdr/osd.h>
#include <vdr/status.h>
#include <vdr/remote.h>
#include "spotidbus.h"
#include "spotiplayer.h"
#include "spoticontrol.h"

extern cSpotifyControl *spotiControl;

cSpotifyControl::cSpotifyControl(void)
:cControl(player = new cSpotiPlayer())
    , cThread("Spotify Control Thread")
{
    visible = false;
    running = true;
    displayMenu = NULL;
    dsyslog("spotify: new Control");
    spotiControl = this;
    cStatus::MsgReplaying(this, "MP3", 0 , true);
    Start();
}

cSpotifyControl::~cSpotifyControl()
{
    if (visible)
        Hide();
    running = false;
    dsyslog("spotify: Control destroyed");
    cStatus::MsgReplaying(this, 0, 0, false);
    if (player) {
        Stop();
        DELETENULL(player);
    }
    spotiControl = NULL;
}

void cSpotifyControl::Stop(void)
{
    dsyslog("spotify: Stop()");
    cStatus::MsgReplaying(this, 0, 0, false);
    delete player;

    player = 0;
}

void cSpotifyControl::ShowProgress(void)
{
    if (displayMenu || (!cOsd::IsOpen())) {
        bool play, forward;
        string artist;
        string title;
        string buffer;
        int speed, Current, Total;

        if (player && player->GetReplayMode(play, forward, speed)) {
            if (!visible)
                return;
            if (!displayMenu) {
                dsyslog("spotify:  ShowProgress creates displaymenu");
                displayMenu = Skins.Current()->DisplayReplay(false);
                displayMenu->SetTitle("Spotify PlayerControl");
            }
        }
        if (!player)
            return;

        artist = getMetaData("xesam:artist");
        title = getMetaData("xesam:title");
        player->GetIndex(Current, Total, false);
        if (artist != "")
            buffer = artist + " - " + title;
        else
            buffer = title;
        displayMenu->SetMessage(mtInfo, buffer.c_str());
        displayMenu->SetProgress(Current, Total);
        displayMenu->SetMode(play, forward, speed);
        displayMenu->SetCurrent(IndexToHMSF(Current,false));
        displayMenu->SetTotal(IndexToHMSF(Total,false));
        //cStatus::MsgReplaying(this, buffer.c_str(), 0, true);
        //free(buffer);
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
    BuildMsgReplay();
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
    if (Key != kNone)
        dsyslog("spotify: Control ProcessKey()");
    eOSState state = osContinue;
    Show();
    if (!player->Active()) {
        dsyslog("spotify: Control ProcessKey not active");
        return osEnd;
    }
    switch ((int)Key & ~k_Repeat) {
       case kBack:
           dsyslog("spotify: kBack");
           if (visible)
               Hide();
           cRemote::CallPlugin("spotify");
           return  osContinue;
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

void cSpotifyControl::Action(void)
// we must provide this routine but we are never started (no Start())
// instead the player thread is started
{
    dsyslog("spotify: Control Action started");

    while (running) {
        cCondWait::SleepMs(1000);
    }
}

void cSpotifyControl::BuildMsgReplay(void)
{
    string buf = "";
    string artist;
    string title;
    artist = getMetaData("xesam:artist");
    title = getMetaData("xesam:title");
    //dsyslog("spotify: Control BuildMenu got %s - %s",artist.c_str(),title.c_str());
    if (title != "") {
        if (artist != "")
            buf = artist + " - " + title;
        else
            buf = title;
    }
    cStatus::MsgReplaying(this, buf.c_str(), 0, true);
}
