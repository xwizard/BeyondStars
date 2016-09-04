/*
	equip.cc
	
	(c) Richard Thrippleton
	Licensing terms are in the 'LICENSE' file
	If that file is not included with this source then permission is not given to use this source in any way whatsoever.
*/

#include <stdio.h>
#include "database.h"
#include "error.h"
#include "equip.h"

void equip::init()
{
}

void equip::loadlib()
{
	char nam[16]; //Object name to load equipment data from

	for(int i=0;i<LIBSIZE;i++)
	{
		sprintf(nam,"Equipment%hd",i);
		try
		{
			database::switchobj(nam);
			equips[i]=new equip(i);
			equips[i]->load();
		}
		catch(error it)
		{
		}
	}
}

equip* equip::get(int indx)
{
	if(indx>=0 && indx<LIBSIZE)
	{
		if(equips[indx])
			return equips[indx];
		else
			return NULL;
	}
	else
		return NULL;
}

equip::equip(int self)
{
	this->self=self;
}

void equip::load()
{
	database::getvalue("Name",nam);
	typ=database::getvalue("Type");
	mss=database::getvalue("Mass");
	spr=database::getvalue("Sprite");
	col=database::getvalue("Colour");
	snd=database::getvalue("Sound");
	pow=database::getvalue("Power");
	rdy=database::getvalue("CycleTime");
	cap=database::getvalue("Capacity");
	rng=database::getvalue("Range");
	trck=database::getvalue("Tracking");
	acov=database::getvalue("Coverage");
	cost=database::getvalue("Cost");
}

equip* equip::equips[LIBSIZE];
