#ifndef __SPOTI_CONTROL_H
#define __SPOTI_CONTROL_H
#include "spotiplayer.h"

class cSpotifyControl:public cControl
{
private:
	cSpotiPlayer * Player;
	cSkinDisplayReplay *displayMenu;
	bool running;
	int visible;
	void SpotiExec(void);
	void ForkAndExec(void);
	void ShowProgress(void);
	virtual void Show(void);
	virtual void Hide(void);
public:
	 cSpotifyControl(void);
	 virtual ~ cSpotifyControl();
	virtual eOSState ProcessKey(eKeys Key);
};
#endif
