/*
	game.cc
	
	(c) Richard Thrippleton
	Licensing terms are in the 'LICENSE' file
	If that file is not included with this source then permission is not given to use this source in any way whatsoever.
*/

#include <stdio.h>
#include "alliance.h"
#include "calc.h"
#include "server.h"
#include "ship.h"
#include "planet.h"
#include "frag.h"
#include "client.h"
#include "ticker.h"
#include "error.h"
#include "interface.h"
#include "camera.h"
#include "presence.h"
#include "graphic.h"
#include "player.h"
#include "database.h"
#include "os.h"
#include "game.h"

void game::runheadless() //Run as a headless server
{
	ticker lreg(24); //Loop regulator
	int sdly; //Game saving delay

	sdly=0;
	lreg.start();
	try
	{
		server::start(false);
		try
		{
			load();
		}
		catch(error it)
		{
			alliance::maketerritories();
		}
		while(true)
		{
			server::cycle();
			ship::simulateall();
			frag::simulateall();
			ship::behaveall();
			planet::shipyards();
			lreg.tick();
			if(sdly<=0)
			{
				save();
				sdly=3000;
			}
			sdly--;
		}
	}
	catch(error it)
	{
		server::stop();
		save();
		player::purgeall();
		ship::purgeall();
		planet::purgeall();
		frag::purgeall();
		throw it;
	}
}

void game::runlocal()
{
	ticker lreg(24); //Loop regulator
	int fdrp; //Frame drop rate
	int fcnt; //Frame drop counter

	lreg.start();
	fcnt=0;
	fdrp=100;
	try
	{
		interface::printtomesg(NULL);
		server::start(true);
		try
		{
			load();
		}
		catch(error it)
		{
			alliance::maketerritories();
		}
		client::connect("127.0.0.1");
		while(true)
		{
			interface::poll();
			presence::controls();
			client::flush();
			server::cycle();

			lreg.tick();

			presence::interpolateall();
			client::poll();
			camera::update();
			fcnt+=100;
			if(fcnt>=fdrp)
			{
				fcnt-=fdrp;
				camera::render();
				interface::render();
				presence::render();
				graphic::blit();
				graphic::clean();
			}
			ship::simulateall();
			frag::simulateall();
			ship::behaveall();
			planet::shipyards();
/*			if(lreg.afps<23)
				fdrp++;
			if(lreg.afps>23.9 && fdrp>100)
				fdrp--;*/
		}
	}
	catch(error it)
	{
		client::stop();
		server::stop();

		save();

		player::purgeall();
		ship::purgeall();
		planet::purgeall();
		frag::purgeall();

		presence::purgeall();
		camera::turnoff();
		camera::unbind();
		interface::printtomesg(NULL);
		interface::printtomesg(it.str);
	}
}

void game::runclient(char* host)
{
	ticker lreg(25); //Loop regulator
	int fdrp; //Frame drop rate
	int fcnt; //Frame drop counter

	lreg.start();
	fcnt=0;
	fdrp=100;
	try
	{
		interface::printtomesg(NULL);
		client::connect(host);
		while(true)
		{
			interface::poll();
			presence::controls();
			client::flush();

			lreg.tick();

			presence::interpolateall();
			client::poll();
			camera::update();
			fcnt+=100;
			if(fcnt>=fdrp)
			{
				fcnt-=fdrp;
				camera::render();
				interface::render();
				presence::render();
				graphic::blit();
				graphic::clean();
			}
			if(lreg.afps<23)
				fdrp++;
			if(lreg.afps>23.9 && fdrp>100)
				fdrp--;
		}
	}
	catch(error it)
	{
		client::stop();
		presence::purgeall();
		camera::turnoff();
		camera::unbind();
		interface::printtomesg(NULL);
		interface::printtomesg(it.str);
	}
}

void game::save()
{
	database::openwriter(os::openpersonal("universe.svd","w"));
	planet::saveall();
	ship::saveall();
	frag::saveall();
	player::saveall();
	database::closewriter();
}

void game::load()
{
	database::openreader(os::openpersonal("universe.svd","r"));
	planet::loadall();
	ship::loadall();
	frag::loadall();
	player::loadall();
	database::closereader();
}
