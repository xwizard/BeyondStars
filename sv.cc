/*
	sv.cc
	
	(c) Richard Thrippleton
	Licensing terms are in the 'LICENSE' file
	If that file is not included with this source then permission is not given to use this source in any way whatsoever.
*/

#include <SDL.h>
#include <SDL_net.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include "calc.h"
#include "graphic.h"
#include "ship.h"
#include "frag.h"
#include "planet.h"
#include "sound.h"
#include "error.h"
#include "ticker.h"
#include "equip.h"
#include "presence.h"
#include "alliance.h"
#include "camera.h"
#include "server.h"
#include "client.h"
#include "database.h"
#include "interface.h"
#include "game.h"
#include "player.h"
#include "os.h"
#include "config.h"

void splashscreen(); //Splashscreen function

int main(int argc,char* argv[])
{
	char path[sizeof(DATADIR)+16]; //Pathnames for databases
	bool hles; //Headless?
	bool bigs; //Big screen?
	bool load; //Load universe on startup?
	bool full; //Fullscreen?
	FILE* clog; //Crash log

	try
	{
		os::init();
		sound::init();
		graphic::init();
		equip::init();
		ship::init();
		frag::init();
		planet::init();
		alliance::init();
		camera::init();
		presence::init();
		calc::init();
		server::init();
		client::init();
		player::init();
		database::init();

		hles=false;
		bigs=true;
		load=false;
		full=false;

		for(int i=1;i<argc;i++)
		{
			if(argv[i][0]=='-')
			{
				switch(argv[i][1])
				{
					case 'h':
					printf("Star Voyager %s\n\n-h	Command line help\n-f	Fullscreen mode\n-x	Run in extra-large window\n-s	Run as standalone server\n",VERSION);
					exit(0);
					break;

					case 'x':
					bigs=false;
					break;

					case 's':
					hles=true;
					break;

					case 'f':
					full=true;
					break;
				}
			}
		}

		sprintf(path,"%s/ships.svd",DATADIR);
		database::openreader(fopen(path,"rb"));
		sprintf(path,"%s/equip.svd",DATADIR);
		database::openreader(fopen(path,"rb"));
		sprintf(path,"%s/alliances.svd",DATADIR);
		database::openreader(fopen(path,"rb"));

		equip::loadlib();
		ship::loadlib();
		alliance::loadlib();

		database::closereader();
		database::closereader();
		database::closereader();

		if(hles)
			game::runheadless();
		else
		{
			sound::start();
			graphic::setup(bigs,full);
			interface::setup();
			splashscreen();
		}
	}
	catch(error it)
	{
		clog=os::openpersonal("exitlog","w");
		if(clog)
		{
			fprintf(clog,"%s\n",it.str);
			fclose(clog);
		}
		printf("Star Voyager exited: %s\n",it.str);
		fflush(NULL);
		os::finish();
		return 0;
	}
	printf("Star Voyager exited abnormally");
	fflush(NULL);
	os::finish();
	return 1;
}

void splashscreen()
{
	ticker poll(16); //Ticker for polling loop
	char host[65]; //Host to connect to
	bool rset; //Reset splash screen?
	graphic* logo; //SV logo
	graphic* panel; //Right-side panel image
	FILE* wipe; //Savegame to wipe

	rset=true;
	logo=graphic::get(graphic::LOGO);
	panel=graphic::get(graphic::PANEL);
	poll.start();
	for(;;)
	{
		if(rset)
		{
			rset=false;
			graphic::clean();
			graphic::clip(&interface::panelb);
			graphic::box(&interface::panelb,graphic::BLACK);
			if(panel)
				panel->draw(interface::panelb.x+interface::panelb.w/2,interface::panelb.y+interface::panelb.h/2,0,1,0,false);
			graphic::clip(&interface::viewb);
			graphic::box(&interface::viewb,graphic::BLACK);
			if(logo)
				logo->draw(interface::viewb.x+interface::viewb.w/2,interface::viewb.y+interface::viewb.h/2,0,1,0,false);
			graphic::embed();

			interface::printtocons("Star Voyager %s\n\n[1] Run local game\n[2] Reset game\n[3] Connect to a server\n[4] Quit\n",VERSION);
			interface::render();
			graphic::blit();
		}
		poll.tick();
		interface::poll();
		switch(interface::lasc)
		{
			case '1':
			interface::printtomesg("Initialising game...");	
			interface::render();
			graphic::clip(&interface::viewb);
			graphic::box(&interface::viewb,graphic::BLACK);
			graphic::blit();
			game::runlocal();
			rset=true;
			break;

			case '2':
			wipe=os::openpersonal("universe.svd","w");
			if(wipe)
				fclose(wipe);
			interface::printtomesg("Game reset");
			rset=true;
			break;

			case '3':
			while(!interface::getline(host,false))
			{
				interface::render();
				graphic::blit();
				interface::poll();
				poll.tick();
			}
			graphic::clip(&interface::viewb);
			graphic::box(&interface::viewb,graphic::BLACK);
			graphic::blit();
			game::runclient(host);
			rset=true;
			break;

			case '4':
			case 'q':
			throw error("User quit");
			break;

			default:
			break;
		}
	}
}
