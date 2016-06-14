#ifndef ___SPOTIPLAYER_H
#define ___SPOTIPLAYER_H

#include <vdr/player.h>
#include <vdr/thread.h>
#include <vdr/status.h>

class cSpotiPlayer:public cPlayer, cThread
{
    private:
        bool run;
    protected:
        virtual void Action(void);
    public:
        cSpotiPlayer(void);
        virtual ~cSpotiPlayer();
        bool Active(void)
        {
            return run;
        }
};

#endif
