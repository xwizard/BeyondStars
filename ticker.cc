/*
	ticker.cc
	
	(c) Richard Thrippleton
	Licensing terms are in the 'LICENSE' file
	If that file is not included with this source then permission is not given to use this source in any way whatsoever.
*/

#include <SDL.h>
#include "error.h"
#include "ticker.h"

ticker::ticker(int fps)
{
	tcks=0;
	pas=1+1000/fps;
	afps=fps;
}

void ticker::start()
{
	otm=SDL_GetTicks();
	stm=SDL_GetTicks();
}

void ticker::tick()
{
	long dly; //Delay to apply

	ntm=SDL_GetTicks();
	dly=pas-(ntm-otm);
	otm=ntm;
	otm+=dly;
	if(dly>0)
		SDL_Delay(dly);
	if(tcks%10==0)
	{
		if((ntm-stm)!=0)
			afps=(double)(10*1000)/(double)(ntm-stm);
		stm=ntm;
	}
	tcks++;
}
