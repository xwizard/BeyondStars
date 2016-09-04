/*
	player.cc
	
	(c) Richard Thrippleton
	Licensing terms are in the 'LICENSE' file
	If that file is not included with this source then permission is not given to use this source in any way whatsoever.
*/

#include <stdio.h>
#include <string.h>
#include "calc.h"
#include "ship.h"
#include "error.h"
#include "planet.h"
#include "database.h"
#include "alliance.h"
#include "server.h"
#include "player.h"

long lcount=0;

player::player()
{
	lcount++;
}

player::player(char* nam)
{
	self=-1;
	nam[32]='\0';
	for(int i=0;i<ISIZE && self==-1;i++)
	{
		if(!players[i])
			self=i;
	}
	if(self==-1)
		throw error("No more space for new accounts");
	players[self]=this;
	sprintf(this->nam,"%s",nam);
	pass[0]='\0';
	in=NULL;
	if(self==0)
		op=true;
	else
		op=false;
	mshp=NULL;
	cash=0;
	cashi=0;
	lcount++;
}

player::~player()
{
	server::notifydelete(this);
	if(self>=0 && self<ISIZE)
		players[self]=NULL;
	if(mshp)
		delete mshp;
	if(in)
		delete in;
	lcount--;
}

void player::init()
{
	for(int i=0;i<ISIZE;i++)
	{
		players[i]=NULL;
	}
}

void player::purgeall()
{
	for(int i=0;i<ISIZE;i++)
		if(players[i])
			delete players[i];
}

void player::saveall()
{
	char obsc[33]; //Object name scratchpad

	for(int i=0;i<ISIZE;i++)
	{
		if(players[i] && players[i]->mshp)
		{
			sprintf(obsc,"Account%hd",i);
			database::putobject(obsc);
			players[i]->save();
		}
	}
}

void player::loadall()
{
	char obsc[33]; //Object name scratchpad

	for(int i=0;i<ISIZE;i++)
	{
		try
		{
			sprintf(obsc,"Account%hd",i);
			database::switchobj(obsc);
			players[i]=new player();
			players[i]->self=i;
			players[i]->load();
		}
		catch(error it)
		{
		}
	}
}

player* player::get(char* nam)
{
	for(int i=0;i<ISIZE;i++)
		if(players[i] && strcmp(nam,players[i]->nam)==0)
			return players[i];
	return NULL;
}

void player::spawn(alliance* tali)
{
	planet* tpln; //Planet to spawn near
	cord sloc; //Location to spawn at

	tpln=planet::pick(tali);
	if(tpln)
	{	
		sloc=tpln->loc;
		sloc.x+=calc::rnd(150)-calc::rnd(150);
		sloc.y+=calc::rnd(150)-calc::rnd(150);
		if(mshp)
			delete mshp;
		if(!(tali->spw))
		{
			throw error("Cannot play for this alliance");
		}
		try
		{
			mshp=new ship(sloc,tali->spw,tali,ship::AI_NULL);
		}
		catch(error it)
		{
			throw it;
		}
		in=new ship();
		*in=*mshp;
		in->assign(this);
		try
		{
			in->insert();
			server::bulletin("%s entered the game",nam);
		}
		catch(error it)
		{
			delete in;
			throw it;
		}
	}
	else
		throw error("Can't find a suitable body to spawn at");
}

void player::login(char* pass)
{
	if(in)
		throw error("Already logged in");
	if(!mshp)
		throw error("No ship associated with this user");
	if(pass)
	{
		pass[32]='\0';
		if(pass[0]=='\0')
			throw error("Invalid password");
		calc::obscure(pass);
		if(strcmp(pass,this->pass)!=0)
			throw error("Invalid password");
	}
	in=new ship();
	*in=*mshp;
	in->assign(this);
	try
	{
		in->insert();
		server::bulletin("%s entered the game",nam);
	}
	catch(error it)
	{
		delete in;
		throw it;
	}
	cashi=cash;
}

void player::setpass(char* pass)
{
	sprintf(this->pass,"%s",pass);
	calc::obscure(this->pass);
}

void player::commit()
{
	if(!in)
		return;
	if(mshp)
	{
		mshp->ply=NULL;
		delete mshp;
	}
	mshp=new ship();
	*mshp=*in;
	mshp->self=-1;
	cash=cashi;
}

void player::transfer(ship* tshp)
{
	in->assign(NULL);
	in->frnd=tshp;
	in->aity=ship::AI_FLEET;
	tshp->all=in->all;
	in=tshp;
	in->assign(this);
}

void player::debit(long amt)
{
	if(amt>cashi)
		throw error("Not enough credits");
	cashi-=amt;
}

void player::credit(long amt)
{
	cashi+=amt;
	if(cashi>999999999)
		cashi=999999999;
}
	
void player::notifydelete()
{
	server::notifykill(this);
	in=NULL;
}

void player::logout()
{
	if(in)
		delete in;
	server::bulletin("%s left the game",nam);
	if(!mshp || pass[0]=='\0')
		delete this;
}

void player::save()
{
	database::putvalue("Name",nam);
	database::putvalue("Password",pass);
	database::putvalue("Op",op);
	database::putvalue("Cash",cash);
	mshp->save();
}

void player::load()
{
	in=NULL;
	database::getvalue("Name",nam);
	database::getvalue("Password",pass);
	op=database::getvalue("Op");
	cash=database::getvalue("Cash");
	mshp=new ship();
	mshp->load();
}

player* player::players[ISIZE];
