#ifndef ___SPOTIPLAYER_H
#define ___SPOTIPLAYER_H

#include <vdr/player.h>
#include <vdr/thread.h>
#include <vdr/status.h>

class cSpotiPlayer:public cPlayer, public cThread
{
    private:
        bool run;
        void Quit(void);
    protected:
        virtual void Activate(bool On);
        virtual void Action(void);
    public:
        cSpotiPlayer(void);
        virtual ~cSpotiPlayer();
        virtual bool GetReplayMode(bool &Play, bool &Forward, int &Speed);
        bool Active(void)
        {
            return run;
        }
};

#endif
