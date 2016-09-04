/*
	sound.cc
	
	(c) Richard Thrippleton
	Licensing terms are in the 'LICENSE' file
	If that file is not included with this source then permission is not given to use this source in any way whatsoever.
*/

#include <SDL.h>
#include <string.h>
#include "error.h"
#include "sound.h"

void sound::init()
{
	on=false;
	for(int i=0;i<ISIZE;i++)
		sounds[i]=NULL;
	for(int i=0;i<16;i++)
		chns[i].snd=NULL;
}

void sound::start()
{
	SDL_AudioSpec want; //Desired audio specification

	if(SDL_InitSubSystem(SDL_INIT_AUDIO)==-1)
		throw error(SDL_GetError());
	if(on)
		return;
	want.freq=22050;
	want.format=AUDIO_S16SYS;
	want.channels=1;
	want.samples=1024;
	want.callback=sound::callback;
	if(SDL_OpenAudio(&want,&spec)==-1)
		on=false;
	else
	{
		on=true;
		SDL_PauseAudio(0);
	}
}

void sound::stop()
{
	if(!on)
		return;
	on=false;
	SDL_CloseAudio();
}

sound* sound::get(int indx)
{
	if(!(indx>=0 && indx<ISIZE))
		return NULL;
	if(!sounds[indx])
	{
		sounds[indx]=new sound(indx);
	}
	return sounds[indx];
}

void sound::play(int div)
{
	if(miss || !on)
		return;
	if(!imem)
	{
		try
		{
			load();
		}
		catch(error it)
		{
			miss=true;
		}
	}
	for(int i=0;i<16;i++)
		if(chns[i].snd==this && chns[i].pntr<64)
			return;
	for(int i=0;i<16;i++)
	{
		if(!chns[i].snd)
		{
			chns[i].snd=this;
			chns[i].pntr=0;
			chns[i].div=div;
			break;
		}	
	}
}

sound::sound(int self)
{
	this->self=self;
	miss=false;
	imem=false;
}

void sound::callback(void* null,Uint8* bfil,int lfil)
{
	long plen; //Length to blit to stream

	for(int i=0;i<16;i++)
	{
		if(chns[i].snd)
		{
			plen=chns[i].snd->blen-chns[i].pntr;
			if(plen>0)
			{
				if(plen>lfil)
					plen=lfil;
				SDL_MixAudio(bfil,&(chns[i].snd->buff[chns[i].pntr]),plen,SDL_MIX_MAXVOLUME/chns[i].div);
				chns[i].pntr+=plen;
			}
			else
			{
				chns[i].snd=NULL;
			}
		}
	}
}

void sound::load()
{
	SDL_AudioSpec wspc; //Wav specification
	SDL_AudioCVT cvt; //Wav to native format conversion structure
	char* path; //Path to load sprite from

	path=new char[strlen(DATADIR)+32];
	sprintf(path,"%s/snd/%i.wav",DATADIR,self);
	if(!SDL_LoadWAV(path,&wspc,&buff,&blen))
	{
		delete[] path;
		throw error("Can't load wave file");
	}
	delete[] path;
	if(SDL_BuildAudioCVT(&cvt,wspc.format,wspc.channels,wspc.freq,spec.format,spec.channels,spec.freq)==-1)
		throw error("Couldn't build the audio conversion structure");
	cvt.buf=new Uint8[blen*cvt.len_mult];
	cvt.len=blen;
	memcpy(cvt.buf,buff,cvt.len);
	SDL_ConvertAudio(&cvt);
	SDL_FreeWAV(buff);
	buff=cvt.buf;
	blen=cvt.len_cvt;
	imem=true;
}

channel sound::chns[16];
SDL_AudioSpec sound::spec;
bool sound::on;
sound* sound::sounds[ISIZE];
