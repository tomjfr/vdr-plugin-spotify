#ifndef __SPOTI_CONTROL_H
#define __SPOTI_CONTROL_H
#include "spotiplayer.h"

class cSpotifyControl: public cControl, public cThread
{
    private:
        cSpotiPlayer *player;
        cSkinDisplayReplay *displayMenu;
        bool running;
        int visible;
//        void Stop(void);
        void ShowProgress(void);
        virtual void Show(void);
        virtual void Hide(void);
        void Stop(void);
        void BuildMsgReplay(void);
        virtual void Action(void);
    public:
        cSpotifyControl(void);
        virtual ~ cSpotifyControl();
        virtual eOSState ProcessKey(eKeys Key);
};
#endif
