/*
	camera.cc
	
	(c) Richard Thrippleton
	Licensing terms are in the 'LICENSE' file
	If that file is not included with this source then permission is not given to use this source in any way whatsoever.
*/

#include <stdio.h>
#include <string.h>
#include "calc.h"
#include "graphic.h"
#include "interface.h"
#include "protocol.h"
#include "presence.h"
#include "constants.h"
#include "sound.h"
#include "error.h"
#include "camera.h"

#define x2screen(rx) (long)cx+(rx-pov.x)/vzm
#define y2screen(ry) (long)cy+(ry-pov.y)/vzm

void camera::init()
{
	src=NULL;
	on=false;
	for(int i=0;i<64;i++)
	{
		strs[i].loc.x=-1;
		strs[i].loc.y=-1;
		strs[i].dep=1;
	}
	vzm=1;
}

void camera::turnon()
{
	on=true;
}

void camera::turnoff()
{
	on=false;
}

void camera::bind(presence* src)
{
	if(camera::src!=src)
	{
		camera::src=src;
		rzm=presence::srng;
		vzm=1;
		update();
	}
}

void camera::unbind()
{
	src=NULL;
}

void camera::render()
{
	if(!on)
		return;

	graphic::clip(&interface::viewb);
	renderstars();
	rendermainview();

	if(src)
	{
		graphic::clip(&interface::radarb);
		renderradar();
	}
}

void camera::update()
{
	if(src)
		pov=src->loc;

	//Handle camera shaking
	if(shak)
	{
		shak--;
		pov.x+=calc::rnd(shak*2)-shak;
		pov.y+=calc::rnd(shak*2)-shak;
	}

	//Update covered universe area
	cov.x1=pov.x-rzm;
	cov.x2=pov.x+rzm;
	cov.y1=pov.y-rzm;
	cov.y2=pov.y+rzm;
}

void camera::noise(sound* snd,presence* src)
{
	vect ssrc; //Sound source vector
	double dis; //Rough distance from PoV

	ssrc.xx=src->loc.x-pov.x;
	ssrc.yy=src->loc.y-pov.y;
	if(ssrc.xx<0)
		ssrc.xx=-ssrc.xx;
	if(ssrc.yy<0)
		ssrc.yy=-ssrc.yy;
	dis=ssrc.xx+ssrc.yy;
	if(dis<600)
	{
		snd->play(1);
		return;
	}
	if(dis<6400)
	{
		snd->play(3);
		return;
	}
	if(dis<12800)
	{
		snd->play(6);
		return;
	}	
}

void camera::shake(int mag)
{
	if(mag>20)
		mag=20;
	shak=mag;
}

void camera::radarzoom(int dir)
{
	if(dir==+1)
		if(rzm>0)
			rzm=(rzm*2)/3;
	if(dir==-1)
		if(rzm>0)
			rzm=(rzm*3)/2;
	if(rzm>presence::lrng)
		rzm=presence::lrng;
	if(rzm<200)
		rzm=200;
}

void camera::viewzoom()
{
	vzm++;
	if(vzm>3)
		vzm=1;
	for(int i=0;i<64;i++)
	{
		strs[i].loc.x=-1;
		strs[i].loc.y=-1;
		strs[i].dep=1;
	}
}

box camera::cov;

void camera::rendermainview()
{
	char txt[33]; //For rendering distance on the pointer
	presence* tprs; //Pointer to objects to draw
	int cx,cy; //Centering screen position
	long sx,sy; //Screen co-ordinates
	pol pptr;
	vect vptr; //Pointer to target
	graphic* ptr; //Pointer graphic
	
	cx=interface::viewb.x+(interface::viewb.w/2);
	cy=interface::viewb.y+(interface::viewb.h/2);
	for(int i=PT_PLANET;i<=PT_FRAG;i++)
	{
		for(int j=0;j<presence::ISIZE;j++)
		{
			tprs=presence::get(j);
			if(tprs && tprs->typ==i)
			{
				sx=(long)x2screen(tprs->loc.x);
				sy=(long)y2screen(tprs->loc.y);
				if(sx>interface::viewb.x-100 && sx<interface::viewb.x+interface::viewb.w+100 && sy>interface::viewb.y-100 && sy<interface::viewb.y+interface::viewb.h+100)
					tprs->drawat(sx,sy,vzm);
						
			}
		}
	}

	if(presence::trg)
	{
		vptr.xx=presence::trg->loc.x-pov.x;
		vptr.yy=presence::trg->loc.y-pov.y;
		pptr=vptr.topol();
		if(pptr.rad>(interface::viewb.w/5)*vzm)
		{
			sprintf(txt,"%ld",(long)pptr.rad/100);
			pptr.rad=interface::viewb.w/2-50;
			vptr=pptr.tovect();
			vptr.xx+=interface::viewb.x+interface::viewb.w/2;
			vptr.yy+=interface::viewb.y+interface::viewb.h/2;
			ptr=graphic::get(graphic::NAV);
			if(ptr)
				ptr->draw(vptr.xx,vptr.yy,(((int)pptr.ang+5)/10)%36,1,0,false);
			graphic::string(txt,vptr.xx,vptr.yy+4,false);
		}
	}
}


void camera::renderstars()
{
	int cx,cy; //Centering screen position
	int astx,asty; //For holding star position on screen
	graphic* warp; //Warp star sprite
	
	warp=NULL;
	cx=interface::viewb.x+(interface::viewb.w/2);
	cy=interface::viewb.y+(interface::viewb.h/2);
	//If we appear to be travelling at warp speed, get the appropriate graphic
	if(presence::vel.rad>99)
		warp=graphic::get(graphic::WARP);
	//Iterate through the background stars
	for(int i=0;i<64;i++)
	{
		//At impulse, some stars will be too 'deep' from warp speed and shouldn't be
		if(!warp && strs[i].dep>10)
			strs[i].dep=1;
		//Calculate the actual screen position
		astx=(short)(strs[i].loc.x+(cx-pov.x)/(vzm*strs[i].dep));
		asty=(short)(strs[i].loc.y+(cy-pov.y)/(vzm*strs[i].dep));
		//If a star is outside of view, make another
		if(astx<interface::viewb.x || astx>interface::viewb.x+interface::viewb.w || asty<interface::viewb.y || asty>interface::viewb.y+interface::viewb.h)
		{
			astx=interface::viewb.x+calc::rnd(interface::viewb.w);
			asty=interface::viewb.y+calc::rnd(interface::viewb.h);

			if(warp)
				strs[i].dep=calc::rnd(140)+60;
			else
				strs[i].dep=calc::rnd(10)+1;

			//Calculate the 'real' location from generated screen position
			strs[i].loc.x=astx-(interface::viewb.x+interface::viewb.w/2-pov.x)/(vzm*strs[i].dep);
			strs[i].loc.y=asty-(interface::viewb.y+interface::viewb.h/2-pov.y)/(vzm*strs[i].dep);
		}
		//Draw the star
		if(warp)
		{
			warp->draw(astx,asty,((presence::vel.ang+5)/10)%36,1,0,false);
		}
		else
		{
			if(strs[i].dep>5)
				graphic::pix(astx,asty,graphic::GREY);
			else
				graphic::pix(astx,asty,graphic::WHITE);
		}
	}
}


void camera::renderradar()
{
	long sx,sy; //Screen co-ordinates
	presence* tprs; //Pointer to objects to draw
	char txt[33]; //For rendering co-ordinates on radar
	int col; //Color to use on radar
	sbox tbox; //Target drawing box
	ipol pdir;
	ivect vdir; //Direction of travel

	flck=!flck;
	if((cov.x2-cov.x1)<(presence::srng*4))
	{
		for(long x=(long)cov.x1/2000,l=(long)cov.x2/2000;x<=l;x++)
		{
			tbox.x=(int)((((x*2000)-pov.x)*(interface::radarb.w/2))/(cov.x2-pov.x)+interface::radarb.x+interface::radarb.w/2);
			tbox.y=interface::radarb.y+interface::radarb.h-2;
			tbox.w=1;
			tbox.h=3;
			graphic::box(&tbox,graphic::GREY);
		}
		for(long y=(long)cov.y1/2000,l=(long)cov.y2/2000;y<=l;y++)
		{
			tbox.y=(int)((((y*2000)-pov.y)*(interface::radarb.h/2))/(cov.y2-pov.y)+interface::radarb.y+interface::radarb.h/2);
			tbox.x=interface::radarb.x+interface::radarb.w-2;
			tbox.w=3;
			tbox.h=1;
			graphic::box(&tbox,graphic::GREY);
		}
	}

	if(cov.x2-pov.x>presence::srng)
	{
		tbox.x=(-presence::srng)/((cov.x2-cov.x1)/(long)interface::radarb.w)+interface::radarb.x+interface::radarb.w/2;
		tbox.y=(-presence::srng)/((cov.y2-cov.y1)/(long)interface::radarb.h)+interface::radarb.y+interface::radarb.h/2;
		tbox.w=(presence::srng)/((cov.x2-cov.x1)/(long)interface::radarb.w)+interface::radarb.x+interface::radarb.w/2;
		tbox.h=(presence::srng)/((cov.y2-cov.y1)/(long)interface::radarb.h)+interface::radarb.y+interface::radarb.h/2;
		graphic::line(tbox.x,tbox.y,tbox.w,tbox.y,graphic::LIGHTBLUE);
		graphic::line(tbox.w,tbox.h,tbox.w,tbox.y,graphic::LIGHTBLUE);
		graphic::line(tbox.w,tbox.h,tbox.x,tbox.h,graphic::LIGHTBLUE);
		graphic::line(tbox.x,tbox.h,tbox.x,tbox.y,graphic::LIGHTBLUE);
	}

	for(int i=0;i<presence::ISIZE;i++)
	{
		tprs=presence::get(i);
		if(tprs)
		{
			sx=(tprs->loc.x-pov.x)/((cov.x2-cov.x1)/(long)interface::radarb.w)+interface::radarb.x+interface::radarb.w/2;
			sy=(tprs->loc.y-pov.y)/((cov.y2-cov.y1)/(long)interface::radarb.h)+interface::radarb.y+interface::radarb.h/2;
			if(tprs->loc.x>cov.x1 && tprs->loc.x<cov.x2 && tprs->loc.y>cov.y1 && tprs->loc.y<cov.y2)
			{
				if(tprs==presence::me)
				{
					if(flck)
						col=graphic::RED;
					else
						col=graphic::BLUE;
				}
				else
				{
					switch(tprs->typ)
					{
						case PT_SHIP:
						if(tprs->enem)
							col=graphic::RED;
						else
							col=graphic::WHITE;
						break;

						case PT_PLANET:
						col=graphic::GREEN;
						break;

						case PT_FRAG:
						col=graphic::BLUE;
						break;
					}
				}

				if(tprs->typ==PT_SHIP)
				{
					pdir=tprs->mov.topol();
					if(pdir.rad!=0)
					{
						vdir.xx=(tprs->mov.xx*7)/pdir.rad;
						vdir.yy=(tprs->mov.yy*7)/pdir.rad;
						graphic::line(sx,sy,sx+vdir.xx,sy+vdir.yy,graphic::BLUE);
					}
				}
				if(tprs==presence::trg || tprs==presence::hl)
				{
					tbox.x=(int)sx-1;
					tbox.y=(int)sy-1;
					tbox.w=3;
					tbox.h=3;
					if(tprs==presence::hl)
					{
						if(flck)
							graphic::box(&tbox,col);
					}
					else
						graphic::box(&tbox,col);
				}
				else
					graphic::pix(sx,sy,col);
			}
		}
	}
	if(presence::trg)
	{
		graphic::string(presence::trg->nam,interface::radarb.x+1,interface::radarb.y+interface::radarb.h-13,false);
		graphic::string(presence::trg->anno,interface::radarb.x+1,interface::radarb.y+interface::radarb.h-6,false);
	}
	sprintf(txt,"%ld , %ld",(long)pov.x/100,(long)pov.y/100);
	graphic::string(txt,interface::radarb.x,interface::radarb.y,false);
	calc::getspeed(presence::vel.rad,txt);
	graphic::string(txt,interface::radarb.x+interface::radarb.w-6*strlen(txt)-2,interface::radarb.y,false);
}

bool camera::on;
presence* camera::src;
icord camera::pov;
int camera::vzm;
long camera::rzm;
star camera::strs[64];
int camera::shak;
bool camera::flck;
