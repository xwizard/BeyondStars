/*
	frag.cc
	
	(c) Richard Thrippleton
	Licensing terms are in the 'LICENSE' file
	If that file is not included with this source then permission is not given to use this source in any way whatsoever.
*/

#include <signal.h>
#include <SDL.h>
#include "calc.h"
#include "database.h"
#include "error.h"
#include "ship.h"
#include "planet.h"
#include "protocol.h"
#include "frag.h"

frag::frag(cord loc,int typ,short spr,short col,ship* trg,ship* own,vect mov,short rot,short pow,short trck,short rng)
{
	self=-1;
	for(int i=0,j=0;i<50;i++)
	{
		j=calc::rnd(ISIZE);
		if(!frags[j])
		{
			self=j;
			break;
		}
	}
	if(self==-1)
		throw error("No free slots for frag");
	frags[self]=this;

	this->loc=loc;
	this->typ=typ;
	this->spr=spr;
	this->col=col;
	this->trg=trg;
	this->own=own;
	this->mov=mov;
	this->rot=rot;
	this->pow=pow;
	this->trck=trck;
	this->rng=rng;
}

frag::~frag()
{
	if(self!=-1)
		frags[self]=NULL;
}

void frag::init()
{
	for(int i=0;i<ISIZE;i++)
		frags[i]=NULL;
}

void frag::purgeall()
{
	for(int i=0;i<ISIZE;i++)
		if(frags[i])
			delete frags[i];
}

void frag::simulateall()
{
	for(int i=0;i<ISIZE;i++)
	{
		if(frags[i])
			frags[i]->physics();
		if(frags[i] && frags[i]->trg)
			frags[i]->home();
	}	
}

void frag::notifydelete(ship* tshp)
{
	for(int i=0;i<ISIZE;i++)
	{
		if(frags[i])
		{
			if(frags[i]->trg==tshp)
				frags[i]->trg=NULL;
			if(frags[i]->own==tshp)
				frags[i]->own=NULL;
		}
	}
}

void frag::saveall()
{
	char obsc[33]; //Object name scratchpad

	for(int i=0;i<ISIZE;i++)
	{
		sprintf(obsc,"Frag%hd",i);
		if(frags[i])
		{
			database::putobject(obsc);
			frags[i]->save();
		}
	}
}

void frag::loadall()
{
	for(int i=0;i<ISIZE;i++)
	{
		try
		{
			new frag(i);
		}
		catch(error it)
		{
		}
	}
}

frag* frag::get(int indx)
{
	if(indx>=0 && indx<ISIZE)
		if(frags[indx])
			return frags[indx];
		else
			return NULL;
	else
		return NULL;
}

void frag::netout(int typ,unsigned char* buf)
{
	buf[0]=typ;
	buf+=1;

	calc::inttodat(frag2pres(self),buf);
	buf+=2;
	switch(typ)
	{
		case SERV_NEW:
		*buf=PT_FRAG;
		buf+=1;
		if(col>=0)
			calc::inttodat(-col,buf);
		else
			calc::inttodat(spr,buf);
		buf+=2;
		if(own)
			calc::inttodat(ship2pres(own->self),buf);
		else
			calc::inttodat(-1,buf);
		break;

		case SERV_UPD:
		calc::longtodat(loc.x,buf);
		buf+=4;
		calc::longtodat(loc.y,buf);
		buf+=4;
		calc::longtodat(mov.xx,buf);
		buf+=4;
		calc::longtodat(mov.yy,buf);
		buf+=4;
		calc::inttodat(rot*10,buf);
		buf+=2;
		*buf=0;
		buf+=1;
		*buf=100;
		buf+=1;
		break;
	}
}

frag::frag(int self)
{
	char obsc[16]; //Object name scratchpad

	this->self=-1;
	sprintf(obsc,"Frag%hd",self);
	database::switchobj(obsc);
	load();
	this->self=self;
	frags[self]=this;
}

void frag::physics()
{
	rng--;
	if(rng<1)
	{
		delete this;
		return;
	}
	if(typ==2)
		rot=(rot+3)%36;
	if(trg && (typ==1 || typ==2))
	{
		if(trg->colldetect(loc,mov))
		{
			trg->hit(pow,loc,mov,own);
			delete this;
			return;
		}
	}
	
	loc.x+=mov.xx;
	loc.y+=mov.yy;
}

void frag::home()
{
	vect trv; //Target vector
	pol trp; //Target polar

	trv.xx=((trg->loc.x+trg->mov.xx)-(loc.x+mov.xx));
	trv.yy=((trg->loc.y+trg->mov.yy)-(loc.y+mov.yy));
	trp=trv.topol();

	//Enforce acceleration restriction
	if(trp.rad>trck)
		trp.rad=trck;

	trv=trp.tovect();
	mov.xx+=trv.xx;
	mov.yy+=trv.yy;
}

void frag::save()
{
	database::putvalue("Type",typ);
	database::putvalue("Sprite",spr);
	database::putvalue("Colour",col);
	if(trg)
		database::putvalue("Target",trg->self);
	if(own)
		database::putvalue("Owner",own->self);
	database::putvalue("XLoc",loc.x);
	database::putvalue("YLoc",loc.y);
	database::putvalue("XVect",mov.xx);
	database::putvalue("YVect",mov.yy);
	database::putvalue("Rotation",rot);
	database::putvalue("Power",pow);
	database::putvalue("Tracking",trck);
	database::putvalue("Range",rng);
}

void frag::load()
{
	typ=database::getvalue("Type");
	spr=database::getvalue("Sprite");
	col=database::getvalue("Colour");
	trg=ship::get(database::getvalue("Target"));
	own=ship::get(database::getvalue("Owner"));
	loc.x=database::getvalue("XLoc");
	loc.y=database::getvalue("YLoc");
	mov.xx=database::getvalue("XVect");
	mov.yy=database::getvalue("YVect");
	rot=database::getvalue("Rotation");
	pow=database::getvalue("Power");
	trck=database::getvalue("Tracking");
	rng=database::getvalue("Range");
}

frag* frag::frags[ISIZE];
