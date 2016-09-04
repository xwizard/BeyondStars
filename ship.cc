/*
	ship.cc
	
	(c) Richard Thrippleton
	Licensing terms are in the 'LICENSE' file
	If that file is not included with this source then permission is not given to use this source in any way whatsoever.
*/

#include <stdio.h>
#include "calc.h"
#include "error.h"
#include "frag.h"
#include "database.h"
#include "planet.h"
#include "equip.h"
#include "server.h"
#include "alliance.h"
#include "constants.h"
#include "protocol.h"
#include "player.h"
#include "ship.h"

ship::ship(cord loc,ship* lshp,alliance* tali,int aity)
{
	self=-1;
	if(!tali)
		throw error("Null alliance given");
 	if(!lshp)
		throw error("Null ship template given");
	*this=*lshp;
	vel.ang=calc::rnd(360);
	vel.rad=0;
	this->loc=loc;
	this->all=tali;
	mlck=true;
	crip=false;
	ply=NULL;
	this->aity=aity;
	resequip();
	if(freemass()<=0)
	{
		error::debug(cls,freemass());
	}
}

ship::ship()
{
	self=-1;
	ply=NULL;
}

ship::~ship()
{
	if(self>=0 && self<ISIZE)
		ships[self]=NULL;
	for(int i=0;i<ISIZE;i++)
	{
		if(ships[i])
		{
			if(ships[i]->enem==this)
				ships[i]->enem=NULL;
			if(ships[i]->frnd==this)
				ships[i]->frnd=NULL;
		}
	}
	frag::notifydelete(this);
	if(ply)
		ply->notifydelete();
}

void ship::init()
{
	for(int i=0;i<ISIZE;i++)
		ships[i]=NULL;
}

void ship::loadlib()
{
	char obnm[33]; //Object name
	
	for(int i=0;i<LIBSIZE;i++)
	{
		try
		{
			lib[i]=new ship();
			try
			{
				sprintf(obnm,"ShipLib%hd",i);
				database::switchobj(obnm);
				lib[i]->load();
				lib[i]->typ=i;
			}
			catch(error it)
			{
				if(lib[i])
					delete lib[i];
				lib[i]=NULL;
			}
		}
		catch(error it)
		{
			lib[i]=NULL;
		}
	}
}

void ship::purgeall()
{
	for(int i=0;i<ISIZE;i++)
		if(ships[i])
			delete ships[i];
}

void ship::simulateall()
{
	for(int i=0;i<ISIZE;i++)
	{
		if(ships[i])
		{
			ships[i]->physics();
			ships[i]->maintain();
		}
	}
}

void ship::behaveall()
{
	mstr=(mstr+1)%1000; //Increment the master strobe
	for(int i=0;i<ISIZE;i++)
	{
		if(ships[i] && !ships[i]->crip) //Ships that exist and are non-crippled should do behaviour
			ships[i]->behave();
	}
	for(int i=0,j=0;i<10;i++)
	{
		j=(mstr*(i+1))%ISIZE;
		if(ships[j] && !ships[j]->crip) //Non-crippled ships need masslock checking periodically
			ships[j]->mlck=planet::masslock(ships[j]->loc);
	}
}

void ship::saveall()
{
	char obsc[16]; //Object name scratchpad

	for(int i=0;i<ISIZE;i++)
	{
		if(ships[i])
		{
			sprintf(obsc,"Ship%hd",i);
			database::putobject(obsc);
			ships[i]->save();
		}
	}
}

void ship::loadall()
{
	char obsc[16]; //Object name scratchpad

	for(int i=0;i<ISIZE;i++)
	{
		try
		{
			new ship(i);
		}
		catch(error it)
		{
		}
	}
	for(int i=0;i<ISIZE;i++)
	{
		try
		{
			if(ships[i])
			{
				sprintf(obsc,"Ship%hd",i);
				database::switchobj(obsc);
				ships[i]->loadlink();
			}
		}
		catch(error it)
		{
		}
	}
}

ship* ship::get(int indx)
{
	if(indx>=0 && indx<ISIZE)
	{
		if(ships[indx])
			return ships[indx];
		else
			return NULL;
	}
	else
	{
		return NULL;
	}
}

ship* ship::libget(int indx)
{
	if(indx>=0 && indx<LIBSIZE)
	{
		if(lib[indx])
			return lib[indx];
		else
			return NULL;
	}
	else
	{
		return NULL;
	}
}

bool ship::freeslot()
{
	for(int i=0;(i<(ISIZE-server::ISIZE));i++)
		if(!ships[i])
			return true;
	return false;
}

void ship::turn(int dir)
{
	if(pow && pow->cap>0)
	{
		if(dir==+1)
			vel.ang+=trn;
		if(dir==-1)
			vel.ang-=trn;
		if(vel.ang>=360)
			vel.ang-=360;
		if(vel.ang<0)
			vel.ang+=360;
	}
}

void ship::accel(int dir,bool wrp)
{
	int pcon; //Power consumption
	double nsp; //New speed
	
	nsp=vel.rad;
	//Handle acceleration while at warp
	if(vel.rad>99)
	{
		if(dir==+1)
			nsp=vel.rad+awp;
		if(dir==-1)
			nsp=vel.rad-awp;
		if(nsp<100)
			if(wrp)
				nsp=mip;
			else
				nsp=100;
	}
	//Handle acceleration if currently at impulse
	//if(vel.rad<=mip)
	else
	{
		if(dir==+1)
		{
			nsp=vel.rad+aip;
		}
		if(dir==-1)
		{
			nsp=vel.rad-aip;
		}
		if(nsp>mip)
		{
			if(wrp)
				nsp=100;
			else
				nsp=mip;
		}
	}

	if(nsp>mip && nsp<100)
		nsp=mip;
	if(nsp>mwp)
		nsp=mwp;

	if(nsp<-(mip/3)) //Prevent going faster backwards than reverse speed
		nsp=-mip/3;

	if(nsp<0 && !wrp && vel.rad>=0) //Prevent moving into reverse if transition not specified
		nsp=0;

	if(nsp>=100)
		pcon=(int)(((vel.rad-nsp)*mss)/2000);
	else
		pcon=(int)(((vel.rad-nsp)*mss)/2);
	if(pcon<0)
		pcon=-pcon;
	if((nsp>=100 && vel.rad<100) || (nsp<100 && vel.rad>=100))
		pcon=0;

	if(pow && pow->cap>=pcon)
	{
		pow->cap-=pcon;
		vel.rad=nsp;
	}
}

void ship::shoot(bool torp)
{
	pol ptmp;
	vect vtmp; //Temporaries for calculations
	vect corr; //Correction vector
	pol ptrg; //Polar to target
	double dis; //Distance to target
	vect vtrg; //Vector to target
	cord cemt; //Location to emit frag at
	pol pemt; //Polar velocity to emit frag at
	vect vemt; //Vector to emit frag at
	double dif; //Angle difference
	long rng; //Range
	bool can; //Can shoot from this slot?
	equip* lnch; //Equipment doing the launching

	if(!see(enem))
		return;
	if(vel.rad>=100)
		return;
	rng=0;
	for(int i=0;i<32;i++)
	{
		if(slots[i].item && ((torp && slots[i].item->typ==equip::LAUNCHER) || (!torp && slots[i].item->typ==equip::PHASER)))
		{
			lnch=slots[i].item;
			vtrg.xx=enem->loc.x-loc.x;
			vtrg.yy=enem->loc.y-loc.y;
			ptrg=vtrg.topol();
			dis=ptrg.rad;

			can=false;
			if(lnch->typ==equip::PHASER)
			{
				rng=lnch->rng*lnch->trck;
				if(lnch->trck)
				{
					corr.xx=((enem->mov.xx-mov.xx)*(ptrg.rad/lnch->trck));
					corr.yy=((enem->mov.yy-mov.yy)*(ptrg.rad/lnch->trck));
					if(vtrg.xx && corr.xx*10/vtrg.xx<10)
						vtrg.xx+=corr.xx;
					if(vtrg.yy && corr.yy*10/vtrg.yy<10)
						vtrg.yy+=corr.yy;
					ptrg=vtrg.topol();
					ptrg.rad=dis;
				}
			}
			if(lnch->typ==equip::LAUNCHER)
				rng=((lnch->rng*lnch->rng)/2)*lnch->trck;

			if(ptrg.rad<=rng)
				can=true;
			if(can)
			{
				dif=ptrg.ang-(vel.ang+slots[i].face);
				if(dif>180)
					dif=dif-360;
				if(dif<-180)
					dif=dif+360;
				if(dif>(lnch->acov) || dif<-(lnch->acov))
					can=false;	
				if(dif>(lnch->acov) || dif<-(lnch->acov))
					can=false;		
				if(lnch->typ==equip::PHASER && pow->cap<lnch->pow)
					can=false;
				if(lnch->typ==equip::LAUNCHER && !(slots[i].cap>0))
					can=false;
				if(slots[i].rdy!=0)
					can=false;
			}
			if(can)
			{
				uncloak();
				
				ptmp=slots[i].pos;
				ptmp.ang=(ptmp.ang+vel.ang);
				if(ptmp.ang>=360)
					ptmp.ang-=360;
				vtmp=ptmp.tovect();
				
				cemt.x=vtmp.xx+loc.x;
				cemt.y=vtmp.yy+loc.y;

				if(torp)
				{
					pemt.ang=slots[i].face+vel.ang; //Sort out the angle a torpedo is shot at
					if(pemt.ang>=360)
						pemt.ang-=360;
					pemt.rad=lnch->trck*2; //and the speed
				}
				else
				{
					pemt.ang=ptrg.ang; //velocity of phaser fire emission, towards target at fixed speed for this weapon
					pemt.rad=lnch->trck;
				}

				vemt=pemt.tovect();
				vemt.xx+=mov.xx;
				vemt.yy+=mov.yy;

				if(lnch->typ==equip::PHASER)
				{
					pow->cap-=lnch->pow;
					try
					{
						new frag(cemt,frag::ENERGY,lnch->spr,lnch->col,enem,this,vemt,((ptrg.ang+5)/10),lnch->pow,0,lnch->rng);
					}
					catch(error it)
					{
					}
				}
				if(lnch->typ==equip::LAUNCHER)
				{
					slots[i].cap--;
					try
					{
						new frag(cemt,frag::HOMER,lnch->spr,lnch->col,enem,this,vemt,0,lnch->pow,lnch->trck,lnch->rng);
					}
					catch(error it)
					{
					}
				}
				server::registernoise(this,lnch->snd);
				slots[i].rdy=lnch->rdy;
				if(torp)
					break;
			}
		}
	}
}

bool ship::see(ship* tshp)
{
	double rng; //Effective range

	if(!tshp) //Null ship
		return false;
	if(tshp==this) //Can always see self
		return true;
	if(sens) //Set the sensor range, or default if no sensor suite
		rng=sens->item->rng;
	else
		rng=1000;
	if(tshp->vel.rad<20) //Slower ships less visible
		rng-=(((rng/2)*(20-tshp->vel.rad))/20);
	if(tshp->clk && tshp->clk->cap==tshp->clk->item->cap) //Cloaked ships even less visible
		rng/=8;
	if((tshp->loc.x-loc.x)>rng) //Bounds checking
		return false;
	if((tshp->loc.x-loc.x)<-rng)
		return false;
	if((tshp->loc.y-loc.y)>rng)
		return false;
	if((tshp->loc.y-loc.y)<-rng)
		return false;
	return true;
}

bool ship::see(planet* tpln)
{
	double rng; //Effective range

	if(!tpln) //Null planet
		return false;
	if(sens) //Set the sensor range, or default if no sensor suite
		rng=sens->item->rng;
	else
		rng=1000;
	if(tpln->typ==planet::STAR) //Can always see stars
		return true;
	if((tpln->loc.x-loc.x)>rng) //Bounds checking
		return false;
	if((tpln->loc.x-loc.x)<-rng)
		return false;
	if((tpln->loc.y-loc.y)>rng)
		return false;
	if((tpln->loc.y-loc.y)<-rng)
		return false;
	return true;		
}

bool ship::see(frag* tfrg)
{
	double rng; //Effective range

	if(!tfrg) //Null frag
		return false;
	if(sens) //Set the sensor range, or default if no sensor suite
		rng=sens->item->rng;
	else
		rng=1000;
	if(tfrg->trg==this || tfrg->own==this) //For bandwidth spamming reasons, only see frags when really close unless they concern you
	{
		if((tfrg->loc.x-loc.x)>rng)
			return false;
		if((tfrg->loc.x-loc.x)<-rng)
			return false;
		if((tfrg->loc.y-loc.y)>rng)
			return false;
		if((tfrg->loc.y-loc.y)<-rng)
			return false;
	}
	else
	{
		if((tfrg->loc.x-loc.x)>500)
			return false;
		if((tfrg->loc.x-loc.x)<-500)
			return false;
		if((tfrg->loc.y-loc.y)>500)
			return false;
		if((tfrg->loc.y-loc.y)<-500)
			return false;
	}
	return true;		
}

int ship::interact(char* txt,short cmod,short opr,ship* mshp)
{
	char spd[32]; //Speed

	if(!(mshp && mshp->ply))
		return -1;
	switch(cmod)
	{
		case CMOD_STAT:
		case CMOD_SCAN:
		if(opr==-1)
		{
			if(mshp->see(this))
			{
				txt+=sprintf(txt,"%s\n",cls);
				if(mshp->all->opposes(all))
					txt+=sprintf(txt,"Alignment:%s [hostile]\n",all->nam);
				else
					txt+=sprintf(txt,"Alignment:%s\n",all->nam);
				if(ply)
					txt+=sprintf(txt,"Commanded by %s\n",ply->nam);
				if(shd && shd->cap>0)
					txt+=sprintf(txt,"\nShields: Raised\n");
				else
					txt+=sprintf(txt,"\nShields: Down\n");
				calc::getspeed(mwp,spd);
				txt+=sprintf(txt,"Maximum velocity: %s\n",spd);
				if(shd)
					txt+=sprintf(txt,"Shield capability: %ld\n",shd->item->cap);
				else
					txt+=sprintf(txt,"No shields");
				if(pow)
					txt+=sprintf(txt,"Maximum power capacity: %ld\n",pow->item->cap);
				else
					txt+=sprintf(txt,"No power plant");
				if(ful)
					txt+=sprintf(txt,"Maximum fuel storage: %ld\n",ful->item->cap);
				else
					txt+=sprintf(txt,"No fuel storage");

				txt+=sprintf(txt,"\nAvailable mass: %hd\n",freemass());

				if(this==mshp)
				{
//					txt+=sprintf(txt,"\nAvailable mass: %hd\n",freemass());
					txt+=sprintf(txt,"\nCredits: %ld\n",ply->cashi);
				}
				if(this==mshp->enem)
					txt+=sprintf(txt,"\n[1] Lay in an intercept course\n");
				return spr;
			}
			else
			{
				txt+=sprintf(txt,"Target not visible\n");
				if(this==mshp->enem)
					txt+=sprintf(txt,"\n[1] Lay in an intercept course\n");
				return -1;
			}
		}
		if(opr==1 && this==mshp->enem)
		{
			mshp->aity=AI_AUTOPILOT;
		}
		break;

		
		case CMOD_EQUIP:
		if(!(esel>=0 && esel<32 && slots[esel].item))
		{
			esel=-1;
			for(int i=0;i<32;i++)
			{
				if(slots[i].item)
				{
					esel=i;
					break;
				}
			}
		}
		if(opr==-1)
		{
			txt+=sprintf(txt,"Internal systems\n\n");
			for(int i=0;i<32;i++)
			{
				if(slots[i].item)
				{
					if(slots[i].item->typ==equip::LAUNCHER)
						if(i==esel)
							txt+=sprintf(txt,">%s [%ld]<\n",slots[i].item->nam,slots[i].cap);
						else
							txt+=sprintf(txt," %s [%ld]\n",slots[i].item->nam,slots[i].cap);
					else if(slots[i].item->typ==equip::FUELTANK && slots[i].cap==0)
						if(i==esel)
							txt+=sprintf(txt,">%s< [empty]\n",slots[i].item->nam);
						else
							txt+=sprintf(txt," %s [empty]\n",slots[i].item->nam);
					else
						if(i==esel)
							txt+=sprintf(txt,">%s<\n",slots[i].item->nam);
						else
							txt+=sprintf(txt," %s\n",slots[i].item->nam);
				}
				else
				{
					if(slots[i].pos.rad>=0)
					{
						if(slots[i].face<=90 || slots[i].face>=270)
							txt+=sprintf(txt," <Free forward port>\n");
						else
							txt+=sprintf(txt," <Free rear port>\n");
					}
				}
			}
			txt+=sprintf(txt,"\n");
			if(shd)
				txt+=sprintf(txt,"[1] Toggle shields\n");
			if(clk)
				txt+=sprintf(txt,"[2] Toggle cloak\n");
			txt+=sprintf(txt,"\n[3] Select equipment\n");
			txt+=sprintf(txt,"[4] Jettison selection\n");
		}
		if(opr==1)
			if(shd && shd->rdy==-1)
				shieldsup();
			else
				shieldsdown();
		if(opr==2)
			if(clk && clk->rdy==-1)
				cloak();
			else
				uncloak();
		if(opr==3)
		{
			for(int i=0,j=esel+1;i<32;i++,j++)
			{
				if(j>=32)
					j=0;
				if(slots[j].item)
				{
					esel=j;
					break;
				}
			}
		}
		if(opr==4)
		{
			if(esel>=0 && esel<32 && slots[esel].item)
			{
				if(slots[esel].item->typ==equip::TRANSPORTER)
				{
					sprintf(txt,"Cannot jettison transporters");
				}
				else
				{
					sprintf(txt,"%s jettisoned",slots[esel].item->nam);
					slots[esel].item=NULL;
					slots[esel].rdy=0;
					slots[esel].cap=0;
					resequip();
				}
			}
		}
		break;

		case CMOD_HAIL:
		if(crip)
		{
			if(opr==-1)
			{
				txt+=sprintf(txt,"Hailing ship\n\n");
				txt+=sprintf(txt,"Vessel is disabled\n\n[1] Attempt to recover it");
			}
			if(opr==1)
			{
				mshp->transport(this);
				enem=NULL;
				plnt=NULL;
				frnd=mshp;
				for(int i=0;i<ISIZE;i++)
					if(ships[i] && ships[i]->enem==this)
						ships[i]->enem=NULL;
				txt+=sprintf(txt,"Vessel successfully acquired");
				mshp->ply->transfer(this);
			}
		}
		else
		{
			if(frnd==mshp)
			{
				if(opr==-1)
				{
					txt+=sprintf(txt,"Hailing ship\n\n");
					txt+=sprintf(txt,"Vessel is under your command\n\n[1] Transfer to this vessel");
				}
				if(opr==1)
				{
					try
					{
						mshp->transport(this);	
						mshp->ply->transfer(this);
						txt+=sprintf(txt,"Transfer of command successful");
					}
					catch(error it)
					{
						try
						{
							transport(mshp);
							mshp->ply->transfer(this);
						}
						catch(error iti)
						{
							throw it;
						}
					}
				}
			}
			else
			{
				txt+=sprintf(txt,"Hailing ship\n\n");
				txt+=sprintf(txt,"No reply");
			}
		}
		break;
		
		case CMOD_WHOIS:
		if(ply)
		{
			txt+=sprintf(txt,"Player: %s\n",ply->nam);
			txt+=sprintf(txt,"Alliance: %s\n",all->nam);
			return spr;
		}
		else
			txt+=sprintf(txt,"Target not player controlled\n");
		break;
	}
	return -1;
}

int ship::freemass()
{
	int out; //Outputted free space

	out=mss;
	for(int i=0;i<32;i++)
	{
		if(slots[i].item)
			out-=slots[i].item->mss;
	}
	return out;
}

void ship::cloak()
{
	if(clk && clk->rdy!=0)
	{
		clk->rdy=0;
		server::registernoise(this,clk->item->snd);
	}
}

void ship::uncloak()
{
	if(clk && clk->rdy!=-1)
	{
		clk->rdy=-1;
		if(clk->cap>0)
			clk->cap=-clk->cap;
		server::registernoise(this,clk->item->snd);
	}
}

void ship::shieldsup()
{
	if(shd)
		shd->rdy=0;
}

void ship::shieldsdown()
{
	if(shd)
		shd->rdy=-1;
}

void ship::netout(int typ,unsigned char* buf)
{
	buf[0]=typ;
	buf+=1;

	calc::inttodat(ship2pres(self),buf);
	buf+=2;
	switch(typ)
	{
		case SERV_SELF:
		if(mss>0)
			calc::inttodat((100*hul)/mhul,buf);
		else
			calc::inttodat(0,buf);
		buf+=2;

		if(pow && pow->cap>0)
			calc::inttodat((100*pow->cap)/(pow->item->cap),buf);
		else
			calc::inttodat(0,buf);
		buf+=2;

		if(shd && shd->cap>0)
			calc::inttodat((100*shd->cap)/(shd->item->cap),buf);
		else
			calc::inttodat(0,buf);
		buf+=2;

		if(ful && ful->cap>0)
			calc::inttodat((100*ful->cap)/(ful->item->cap),buf);
		else
			calc::inttodat(0,buf);
		buf+=2;
		if(sens)
			calc::longtodat(sens->item->rng,buf);
		else
			calc::longtodat(0,buf);
		buf+=4;
		calc::longtodat(LIMIT,buf);
		buf+=4;
		if(plnt)
		{
			calc::inttodat(planet2pres(plnt->self),buf);
		}
		else
		{
			if(enem)
				calc::inttodat(ship2pres(enem->self),buf);
			else
				calc::inttodat(-1,buf);
		}
		buf+=2;
		calc::inttodat(-1,buf);
		buf+=2;
		calc::inttodat(-1,buf);
		break;

		case SERV_NEW:
		*buf=PT_SHIP;
		buf+=1;
		calc::inttodat(spr,buf);
		buf+=2;
		calc::inttodat(-1,buf);
		buf+=2;
		break;

		case SERV_NAME:
		sprintf((char*)buf,"%s",cls);
		buf+=64;
		sprintf((char*)buf,"%s",all->nam);
		buf+=64;
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
		calc::inttodat(vel.ang,buf);
		buf+=2;
		*buf=0;
		buf+=1;
		if(clk && clk->item->cap)
		{
			if(clk->cap>=0)
				*buf=100-((100*clk->cap)/clk->item->cap);
			else
				*buf=100+((100*clk->cap)/clk->item->cap);
		}
		else
			*buf=100;
		buf+=1;
		break;

		default:
		break;
	}
}

bool ship::colldetect(cord frgl,vect frgv)
{
	int rot; //Target rotation
	double x1,y1,x2,y2,xx,yy; //Target bounding box

	rot=(int)(((vel.ang+5)/10))%36;
	xx=(frgv.xx-mov.xx)/2;
	yy=(frgv.yy-mov.yy)/2;
	if(xx<0)
		xx=-xx;
	if(yy<0)
		yy=-yy;
	x1=loc.x-(w[rot]*3)/2-xx;
	y1=loc.y-(h[rot]*3)/2-yy;
	x2=loc.x+(w[rot]*3)/2+xx;
	y2=loc.y+(h[rot]*3)/2+yy;
	if(frgl.x>x1 && frgl.x<x2 && frgl.y>y1 && frgl.y<y2)
		return true;
	else
		return false;
}

void ship::hit(int mag,cord frgl,vect frgv,ship* src)
{
	cord tmpc;
	vect tmpv; //Temporary for working out detonations
	int rot; //Target rotation
	int ndeb; //Number of debris bits

	uncloak();
	rot=(int)(((vel.ang+5)/10))%36;
	if(shd)
		shd->cap-=mag;
	server::registershake(this,mag/100);
	if(src && enem!=src && !(all->opposes(src->all)) && src->ply)
		src->lynch();
	if(shd && shd->cap>0)
	{
		try
		{
			new frag(frgl,frag::DEBRIS,shd->item->spr,-1,NULL,this,mov,calc::rnd(36),0,0,2);
		}
		catch(error it)
		{
		}
	}
	else
	{
		if(shd)
			shd->cap=0;
		frgl.x=(frgl.x+2*loc.x)/3;
		frgl.y=(frgl.y+2*loc.y)/3;
		for(int i=0;i<5;i++)
		{
			tmpv=mov;
			frgl.x+=calc::rnd(2)-calc::rnd(2);
			frgl.y+=calc::rnd(2)-calc::rnd(2);
			tmpv.xx+=calc::rnd(2)-calc::rnd(2);
			tmpv.yy+=calc::rnd(2)-calc::rnd(2);
			try
			{
				new frag(frgl,frag::DEBRIS,frag::FIRE,-1,NULL,this,tmpv,calc::rnd(36),0,0,calc::rnd(5)+5);
			}
			catch(error it)
			{
			}
		}
		server::registernoise(this,fsnd);
	}

	if(shd && shd->cap!=-10)
		hul-=(mag*4)/(shd->cap+10);
	else
		hul-=(mag*4)/10;

	if(hul<=0)
	{
		hul=0;
		ndeb=mss/8+4;
		if(ndeb>70)
			ndeb=70;
		for(int i=0;i<ndeb;i++)
		{	
			if(i==0 || calc::rnd(5)==0)
			{
				tmpc=loc;
				tmpc.x+=calc::rnd(2*w[rot])-calc::rnd(2*w[rot]);
				tmpc.y+=calc::rnd(2*h[rot])-calc::rnd(2*h[rot]);
			}
			tmpv=mov;
			try
			{
				if(calc::rnd(10)<3)
				{
					new frag(tmpc,frag::DEBRIS,frag::FIRE,-1,NULL,this,tmpv,calc::rnd(36),0,0,calc::rnd(20)+5);
				}
				else
				{
					tmpv.xx+=calc::rnd(2)-calc::rnd(2);
					tmpv.yy+=calc::rnd(2)-calc::rnd(2);
					new frag(tmpc,frag::DEBRIS,fspr,-1,NULL,this,tmpv,calc::rnd(36),0,0,calc::rnd(20)+5);
				}
			}
			catch(error it)
			{
			}
		}
		if(src && src->ply && src->all->opposes(all))
		{
			server::hail(NULL,src->ply,"Target destroyed; bounty paid");
			src->ply->credit(mss/2);
		}
		server::registernoise(this,dsnd);
		if(ply)
			server::bulletin("%s has been destroyed",ply->nam);
		delete this;
	}
	else
	{
		if(hul<mhul/2 && src && src->ply && !ply && !crip)
		{
			server::hail(NULL,src->ply,"Target crippled");
			crip=true;
		}
	}
}

void ship::assign(player* ply)
{
	this->ply=ply;
	enem=NULL;
	frnd=NULL;
	crip=false;
	aity=AI_NULL;
	resequip();
}

long ship::purchase(int prch,short ripo,bool buy)
{
	long cost; //Value to output

	cost=0;
	if(prch==PRCH_FUEL)
	{
		if(ful)
		{
			if((ful->cap)<(ful->item->cap))
				cost=ripo/4;
			if(buy)
			{
				ful->cap=ful->item->cap;
				ply->debit(cost);
			}
		}
	}
	if(prch==PRCH_ARMS)
	{
		for(int i=0;i<32;i++)
		{
			if(slots[i].item && slots[i].item->typ==equip::LAUNCHER)
			{
				if(slots[i].cap<slots[i].item->cap)
				{
					cost=(slots[i].item->cost*ripo)/1000;
					if(buy)
					{
						ply->debit(cost);
						slots[i].cap=slots[i].item->cap;
					}
					break;
				}
			}
		}
	}
	if(prch==PRCH_HULL)
	{
		cost=((mhul-hul)*ripo)/100;
		if(buy)
		{
			ply->debit(cost);
			hul=mhul;
		}
	}
	return cost;
}

long ship::purchase(equip* prch,int ripo,bool buy)
{
	long cost; //Value to output

	if(!prch)
		return 0;
	cost=(prch->cost*ripo)/100;
	if(buy && !(prch->mss>freemass()))
	{
		for(int i=0;i<32;i++)
		{
			if(!slots[i].item)
			{
				if((slots[i].pos.rad>=0 && (prch->typ==equip::PHASER || prch->typ==equip::LAUNCHER)) || (slots[i].pos.rad==-1 && prch->typ!=equip::PHASER && prch->typ!=equip::LAUNCHER))
				{
					ply->debit(cost);
					slots[i].item=prch;
					slots[i].cap=prch->cap;
					slots[i].rdy=prch->rdy;
					resequip();
					break;
				}
			}
		}
	}
	return cost;
}

void ship::transport(planet* to)
{
	vect vto;
	pol pto; //Vectors to the target

	vto.xx=to->loc.x-loc.x;
	vto.yy=to->loc.y-loc.y;
	pto=vto.topol();
	if(shd && shd->cap>0)
		throw error("Cannot transport with shields up");
	if(!pow)
		throw error("Not enough power to transport");
	if(clk && clk->cap!=0)
		throw error("Cannot transport while cloaked");
	for(int i=0;i<32;i++)
	{
		if(slots[i].item && slots[i].item->typ==equip::TRANSPORTER && slots[i].rdy==0 && pow->cap>=slots[i].item->pow && slots[i].item->rng>=pto.rad)
		{
			pow->cap-=slots[i].item->pow;
			slots[i].rdy=slots[i].item->rdy;
			server::registersound(this,slots[i].item->snd);
			return;
		}
	}
	throw error("No available transporters ready or powered");
}

void ship::transport(ship* to)
{
	vect vto;
	pol pto; //Vectors to the target

	vto.xx=to->loc.x-loc.x;
	vto.yy=to->loc.y-loc.y;
	pto=vto.topol();
	if(shd && shd->cap>0)
		throw error("Cannot transport with shields up");
	if(!pow)
		throw error("Not enough power to transport");
	if(clk && clk->cap!=0)
		throw error("Cannot transport while cloaked");
	if(to->shd && to->shd->cap>0)
		throw error("Cannot transport through destination's shields");
	if(to->clk && to->clk->cap!=0)
		throw error("Cannot transport through destination's cloak");
	for(int i=0;i<32;i++)
	{
		if(slots[i].item && slots[i].item->typ==equip::TRANSPORTER && slots[i].rdy==0 && pow->cap>=slots[i].item->pow && slots[i].item->rng>=pto.rad)
		{
			pow->cap-=slots[i].item->pow;
			slots[i].rdy=slots[i].item->rdy;
			server::registersound(this,slots[i].item->snd);
			return;
		}
	}
	throw error("No available transporters ready or powered");
}

void ship::save()
{
	char atsc[33]; //Attribute scratchpad

	database::putvalue("Class",cls);
	database::putvalue("Type",typ);
	database::putvalue("ShipSprite",spr);
	database::putvalue("Width",w[0]);
	database::putvalue("Height",h[0]);
	if(fspr)
		database::putvalue("FragSprite",fspr);
	if(fsnd)
		database::putvalue("FragSound",fsnd);
	if(dsnd)
		database::putvalue("DeathSound",dsnd);
	if(all)
		database::putvalue("Team",all->self);
	database::putvalue("AIType",aity);
	database::putvalue("XLoc",loc.x);
	database::putvalue("YLoc",loc.y);
	database::putvalue("Heading",vel.ang);
	database::putvalue("Speed",vel.rad);
	database::putvalue("TurnRate",trn);
	database::putvalue("SublightLimit",mip);
	database::putvalue("SublightAcceleration",aip*10);
	database::putvalue("WarpLimit",mwp);
	database::putvalue("WarpAcceleration",awp);
	database::putvalue("Mass",mss);
	database::putvalue("HullStrength",hul);
	database::putvalue("HullStrengthLimit",mhul);
	if(frnd)
		database::putvalue("FriendTarget",frnd->self);
	if(enem)
		database::putvalue("EnemyTarget",enem->self);
	if(plnt)
		database::putvalue("PlanetTarget",plnt->self);
	database::putvalue("MassLock",mlck);
	database::putvalue("Crippled",crip);
	for(int i=0;i<32;i++)
	{
		if(slots[i].item || slots[i].pos.rad!=-1)
		{
			sprintf(atsc,"Slot%hdAngle",i);
			database::putvalue(atsc,slots[i].pos.ang);
			sprintf(atsc,"Slot%hdRadius",i);
			database::putvalue(atsc,slots[i].pos.rad);
			sprintf(atsc,"Slot%hdFace",i);
			database::putvalue(atsc,slots[i].face);
			sprintf(atsc,"Slot%hdItem",i);
			if(slots[i].item)
				database::putvalue(atsc,slots[i].item->self);
			else
				database::putvalue(atsc,-1);
			sprintf(atsc,"Slot%hdReadiness",i);
			database::putvalue(atsc,slots[i].rdy);
			sprintf(atsc,"Slot%hdCapacity",i);
			database::putvalue(atsc,slots[i].cap);
		}
	}
}

void ship::load()
{
	char atsc[33]; //Attribute scratchpad
	pol bpol;
	vect vct1,vct2; //Temporaries for calculating the bounding box

	database::getvalue("Class",cls);
	typ=database::getvalue("Type");
	spr=database::getvalue("ShipSprite");
	w[0]=database::getvalue("Width");
	h[0]=database::getvalue("Height");
	for(int i=1;i<36;i++)
	{
		bpol.ang=i*10;
		bpol.rad=h[0];
		vct1=bpol.tovect();
		bpol.ang=(i*10+90)%360;
		bpol.rad=w[0];
		vct2=bpol.tovect();
		if(vct1.xx<0)
			vct1.xx=-vct1.xx;
		if(vct2.xx<0)
			vct2.xx=-vct2.xx;
		if(vct1.yy<0)
			vct1.yy=-vct1.yy;
		if(vct2.yy<0)
			vct2.yy=-vct2.yy;
		if(vct1.xx>vct2.xx)
			w[i]=(int)vct1.xx;
		else
			w[i]=(int)vct2.xx;
		if(vct1.yy>vct2.yy)
			h[i]=(int)vct1.yy;
		else
			h[i]=(int)vct2.yy;
	}
	fspr=database::getvalue("FragSprite");
	fsnd=database::getvalue("FragSound");
	dsnd=database::getvalue("DeathSound");
	all=alliance::get(database::getvalue("Team"));
	aity=database::getvalue("AIType");
	loc.x=database::getvalue("XLoc");
	loc.y=database::getvalue("YLoc");
	vel.ang=database::getvalue("Heading");
	vel.rad=database::getvalue("Speed");
	trn=database::getvalue("TurnRate");
	mip=database::getvalue("SublightLimit");
	aip=(double)database::getvalue("SublightAcceleration")/10;
	mwp=database::getvalue("WarpLimit");
	awp=database::getvalue("WarpAcceleration");
	mss=database::getvalue("Mass");
	hul=database::getvalue("HullStrength");
	mhul=database::getvalue("HullStrengthLimit");
	
	mlck=database::getvalue("MassLock");
	crip=database::getvalue("Crippled");

	esel=-1;
	for(int i=0;i<32;i++)
	{
		sprintf(atsc,"Slot%hdAngle",i);
		slots[i].pos.ang=database::getvalue(atsc);
		sprintf(atsc,"Slot%hdRadius",i);
		slots[i].pos.rad=database::getvalue(atsc);
		sprintf(atsc,"Slot%hdFace",i);
		slots[i].face=database::getvalue(atsc);
		if(slots[i].face==-1)
			slots[i].face=slots[i].pos.ang;
		sprintf(atsc,"Slot%hdItem",i);
		slots[i].item=equip::get(database::getvalue(atsc));
		sprintf(atsc,"Slot%hdReadiness",i);
		slots[i].rdy=database::getvalue(atsc);
		sprintf(atsc,"Slot%hdCapacity",i);
		slots[i].cap=database::getvalue(atsc);
		if(slots[i].cap==-1 && slots[i].item)
			slots[i].cap=slots[i].item->cap;
	}

	if(hul==-1)
		hul=mhul;
	ply=false;

	mov.xx=0;
	mov.yy=0;

	frnd=NULL;
	enem=NULL;
	plnt=NULL;

	if(aity==-1)
		aity=AI_NULL;

	resequip();
}

void ship::insert()
{
	self=-1;
	if(ply)
	{
		for(int i=0;i<ISIZE && self==-1;i++)
			if(!ships[i])
				self=i;
	}
	else
	{
		for(int i=0;(i<(ISIZE-server::ISIZE)) && self==-1;i++)
			if(!ships[i])
				self=i;
	}
	if(self==-1)
		throw error("No free ship index available");
	else
		ships[self]=this;
}

void ship::insert(int self)
{
	this->self=-1;
	if(!(self>=0 && self<ISIZE))
		return;
	this->self=self;
	if(ships[self])
		delete ships[self];
	ships[self]=this;
}

ship::ship(int self)
{
	char obsc[16]; //Object name scratchpad

	sprintf(obsc,"Ship%hd",self);
	database::switchobj(obsc);
	load();
	insert(self);
}

void ship::physics()
{
	vect nmov; //New movement vector

	//Slow down vessels at warp under masslock influence
	if(vel.rad>=100 && mlck)
		vel.rad=mip;
	//Handle ships in between warp 1 and maximum impulse
	if(vel.rad<100 && vel.rad>mip)
		vel.rad=mip;

	//Handle ships going beyond the boundaries of the 'universe'; they bounce
	if(loc.x>LIMIT || loc.x<-LIMIT || loc.y>LIMIT || loc.y<-LIMIT)
	{
		vel.ang=(vel.ang+180);
		if(vel.ang>=360)
			vel.ang-=360;
	}
	if(loc.x>LIMIT)
		loc.x=LIMIT;
	if(loc.x<-LIMIT)
		loc.x=-LIMIT;
	if(loc.y>LIMIT)
		loc.y=LIMIT;
	if(loc.y<-LIMIT)
		loc.y=-LIMIT;
	
	nmov=vel.tovect();
	if(vel.rad<100 && (mss/100)!=0)
	{
		mov.xx+=(nmov.xx-mov.xx)/(mss/100);
		mov.yy+=(nmov.yy-mov.yy)/(mss/100);
		//loc.x+=(nmov.xx-mov.xx);
		//loc.y+=(nmov.yy-mov.yy);
	}
	else
	{
		mov=nmov;
	}

	loc.x+=mov.xx;
	loc.y+=mov.yy;
}


void ship::autonav(planet* tpln)
{
	vect vtrg; //Vector to target
	pol ptrg; //Polar to target
	double dd; //Directional difference
	double tol; //Angular tolerance

	vtrg.xx=(self*497)%800-400+tpln->loc.x-loc.x;
	vtrg.yy=(self*273)%800-400+tpln->loc.y-loc.y; //Vector to deterministic but arbitrary location near target planet

	ptrg=vtrg.topol(); //...make polar

	dd=ptrg.ang-vel.ang;
	if(dd>180)
		dd=dd-360;
	if(dd<-180)
		dd=dd+360; //Evaluate angle between current heading and target bearing

	tol=trn+2;
	if(vel.rad<=5) 
		tol=20; //Low speed; not too fussed about fine direction finding

	ptrg.rad-=150; //Stand off distance

	if(ptrg.rad<0) //Don't turn when too close
		dd=0;

	if(dd<tol && dd>-tol) //Don't turn when within angle tolerance
		dd=0;

	if(dd==0 && ptrg.rad>0) //Only accelerate when heading at target
	{
		if(vel.rad>=100)
		{
			if(vel.rad<sqrt(2*awp*ptrg.rad)-awp)
				accel(+1,true);
			else if(vel.rad>sqrt(2*awp*ptrg.rad)+awp)
				accel(-1,true);
		}
		else
		{
			if(vel.rad<(sqrt(2*aip*ptrg.rad)-aip))  //Intended speed is sqrt(2as)
				accel(+1,true);
			else if(vel.rad>(sqrt(3*aip*ptrg.rad))+aip)
				accel(-1,true);
		}
	}
	else
		accel(-1,false);
	if(dd>0)
		turn(+1);
	if(dd<0)
		turn(-1);
}

void ship::follow(ship* tshp)
{
	vect vtrg; //Vector to target
	pol ptrg; //Polar to target
	double dd; //Directional difference
	double tol; //Angular tolerance

	ptrg.ang=tshp->vel.ang+90+(self*29)%180; //Find deterministic formation angle to hold at around target ship
	ptrg.rad=100+(self*17)%((sens ? sens->item->rng : 1000)/16); //Deterministic range to hold based on sensor range
	vtrg=ptrg.tovect();
		
	vtrg.xx+=tshp->loc.x-loc.x;
	vtrg.yy+=tshp->loc.y-loc.y;
	ptrg=vtrg.topol(); //Get polar vector to this formation position

	dd=ptrg.ang-vel.ang;
	if(dd>180)
		dd=dd-360;
	if(dd<-180)
		dd=dd+360; //Evaluate angle between current heading and target bearing

	if(!see(tshp))
		ptrg.rad-=(sens ? sens->item->rng : 1000)/3; //If you can't see the target, stand off a little

	tol=trn*2+2;
	if(vel.rad<=5) 
		tol=20; //Low speed; not too fussed about fine direction finding

	ptrg.rad-=150; //Default stand off

	if(ptrg.rad<0) //Don't turn when too close
		dd=0;

	if(dd<tol && dd>-tol) //Don't turn when within angle tolerance
		dd=0;

	if(dd==0 && ptrg.rad>0) //Only accelerate when heading at target
	{
		if(vel.rad>=100)
		{
			if(vel.rad<sqrt(2*awp*ptrg.rad)-awp)
				accel(+1,true);
			else if(vel.rad>sqrt(2*awp*ptrg.rad)+awp)
				accel(-1,true);
		}
		else
		{
			if(vel.rad<(sqrt(2*aip*ptrg.rad)-2*aip))  //Intended speed is sqrt(2as)
				accel(+1,true);
			else if(vel.rad>(sqrt(2*aip*ptrg.rad)+2*aip))
				accel(-1,true);
		}
	}
	else
		accel(-1,false);

	if(dd>0)
		turn(+1);
	if(dd<0)
		turn(-1);
}

void ship::attackpattern(ship* tshp,int str)
{
	vect vtrg; //Vector to target
	pol ptrg; //Polar to target
	double dd; //Directional difference
	double tol; //Angular tolerance

	if(!see(tshp) || vel.rad>=100) //If you can't see or are warp pursuing the target ship, default to the follow method
	{
		follow(tshp);
		return;
	}
	if(str>(200+calc::rnd((self%7)*12)-50)) //Alternate on tailing target from one of two sides
		ptrg.ang=tshp->vel.ang+45+(str-self*29)%135;
	else
		ptrg.ang=tshp->vel.ang-45-(str+self*29)%135;
	ptrg.rad=100+(self*17)%((str+(sens ? sens->item->rng : 1000))/16); //Back off a little depending on sensor range
	vtrg=ptrg.tovect();
		
	vtrg.xx+=tshp->loc.x-loc.x;
	vtrg.yy+=tshp->loc.y-loc.y;
	ptrg=vtrg.topol(); //And finally get a polar to the 'formation' position

	dd=ptrg.ang-vel.ang;
	if(dd>180)
		dd=dd-360;
	if(dd<-180)
		dd=dd+360; //Evaluate angle between current heading and target bearing

	if(!see(tshp))
		ptrg.rad-=(sens ? sens->item->rng : 1000)/3; //If you can't see the target, stand off a little
	tol=trn*2+2;

	if(vel.rad<=5) 
		tol=20; //Low speed; not too fussed about fine direction finding

	tol+=45; //Widen angle tolerance for close combat flair

	if(ptrg.rad<0) //Don't turn when too close
		dd=0;

	if(dd<tol && dd>-tol) //Don't turn when within angle tolerance
		dd=0;

	if(dd==0 && ptrg.rad>0) //Only accelerate when heading at target
	{
		if(vel.rad>=100)
		{
			if(ptrg.rad && (ptrg.rad/vel.rad) && ((vel.rad)/(12*ptrg.rad/vel.rad))>=awp-30)
				accel(-1,true);
			else
				accel(+1,true);
		}
		else
		{
			if(ptrg.rad && ((5*vel.rad*vel.rad)/(ptrg.rad))>=aip)
				accel(-1,true);
			else
				accel(+1,true);
		}
	}
	else
		accel(-1,false);
	if(dd>0)
		turn(+1);
	if(dd<0)
		turn(-1);
}

void ship::loadlink()
{
	frnd=get(database::getvalue("FriendTarget"));
	enem=get(database::getvalue("EnemyTarget"));
	plnt=planet::get(database::getvalue("PlanetTarget"));
}

void ship::maintain()
{
	if(crip)
	{
		if(shd)
			shd->rdy=-1;	
		if(clk)
			clk->rdy=-1;
		if(calc::rnd(402)==0)
			hit(1000,loc,mov,NULL);
	}
	else
	{
		if(pow && ful)
		{
			if((pow->cap)<(pow->item->cap) && ful->cap>0)
			{
				ful->cap-=pow->item->pow;
				pow->cap+=pow->item->pow;
				if(ful->cap<0)
					ful->cap=0;
			}
			if((pow->cap)>(pow->item->cap))
			{
				ful->cap+=(pow->cap)-(pow->item->cap);
				pow->cap=pow->item->cap;
			}
		}
	}
	
	for(int i=0;i<32;i++)
	{
		if(slots[i].item)
		{
			if(slots[i].rdy>0)
				slots[i].rdy--;
			else
				if(slots[i].rdy<0 && (slots[i].item->typ==equip::PHASER || slots[i].item->typ==equip::LAUNCHER || slots[i].item->typ==equip::TRANSPORTER))
					slots[i].rdy=0;
		}
	}

	if(shd && shd->rdy==0 && pow && (pow->cap)>=shd->item->pow)
	{
		shd->cap+=shd->item->pow;
		pow->cap-=shd->item->pow;
		if(shd->cap>shd->item->cap)
			shd->cap=shd->item->cap;
	}
	else
	{
		if(shd)
			shd->cap/=2;
	}
	if(clk)
	{
		if(clk->rdy==0)
		{
			if(pow && (pow->cap)>=(clk->item->pow*mss)/20)
				pow->cap-=(clk->item->pow*mss)/20;
			else
				uncloak();
			if(clk->cap<clk->item->cap)
				clk->cap++;
		}
		else
		{
			if(clk->cap>=0)
				clk->cap=0;
			else
				clk->cap++;
		}
	}

	if(pow && pow->cap<0)
		pow->cap=0;
	if((!ful || (ful && ful->cap==0)) && !ply && calc::rnd(100)==0)
		delete this;
}

void ship::behave()
{
	int istr; //Individual strobe for this ship
	bool amrt; //Run amortised cost code for this state?
	planet* tpln; //A planet for use in ai code
	ship* tshp; //A ship for use in ai code

	istr=(mstr+self*7)%400;
	if(istr%40==0)
		amrt=true;
	else
		amrt=false;

	//Run behaviours for each case of this ship's behaviour state
	switch(aity)
	{
		case AI_AUTOPILOT:
		if(enem)
			follow(enem);
		else if(plnt)
			autonav(plnt);
		break;

		case AI_PATROLLER:
		if(enem)
		{
			attackpattern(enem,istr);
			firecontrol(istr);
		}
		else if(plnt)
			autonav(plnt);

		if(amrt)
		{
			if(!enem)
			{
				shieldsdown();
				enem=pickhostile();
			}
			else
			{
				shieldsup();
				if(plnt && !see(plnt))
					enem=NULL;
			}
			if(!plnt || plnt->all!=all || vel.rad<=5)
			{
				tpln=planet::pick(all);
				if(tpln && tpln->typ!=planet::STAR && see(tpln))
					plnt=tpln;
			}
		}
		break;

		case AI_INVADER:
		if(enem)
		{
			attackpattern(enem,istr);
			firecontrol(istr);
		}
		else if(plnt)
			autonav(plnt);

		if(amrt)
		{
			if(plnt)
				cloak();
			if(enem && calc::rnd(10)==0)
				enem=NULL;
			if(!enem)
			{
				shieldsdown();
				tshp=pickhostile();
				if(tshp)
				{
					enem=tshp;
					plnt=NULL;
				}
			}
			else
				shieldsup();
			if(!enem && !plnt)
				plnt=planet::pickhostile(all);
		}
		break;

		case AI_CARAVAN:
		if(enem)
		{
			firecontrol(istr);
			if((plnt && istr<200) || !plnt)
				attackpattern(enem,istr);
			else
				autonav(plnt);
		}
		else if(plnt)
			autonav(plnt);

		if(amrt)
		{
			if(!enem)
			{
				shieldsdown();
				enem=pickhostile();
			}
			else
				shieldsup();
			if(!plnt || all->opposes(plnt->all) || vel.rad<=5)
			{
				tpln=planet::pickally(all);
				if(tpln && tpln->typ!=planet::STAR)
					plnt=tpln;
			}
		}
		break;

		case AI_BUDDY:
		if(enem)
		{
			attackpattern(enem,istr);	
			firecontrol(istr);
		}
		else if(frnd)
			follow(frnd);

		if(amrt)
		{
			if(!enem)
				shieldsdown();
			if(enem)
			{
				shieldsup();
				if(calc::rnd(10)==0)
					enem=NULL;
			}
			if(!enem)
				enem=pickhostile();
			if(frnd)
			{
				if(frnd->clk && frnd->clk->cap!=0)
					cloak();
				if(enem && !see(frnd))
					enem=NULL;
				if(frnd->frnd)
					frnd=frnd->frnd;
				if(frnd==this)
					frnd=NULL;
			}
			else
				frnd=pickally();
		}
		break;

		case AI_FLEET:
		if(enem)
		{
			if(shd && shd->cap<shd->item->cap)
				attackpattern(enem,istr);	
			else if(frnd)
				follow(frnd);
			firecontrol(istr);
		}
		else if(frnd)
			follow(frnd);

		if(amrt)
		{
			if(!enem)
				shieldsdown();
			if(enem)
			{
				shieldsup();
				if(calc::rnd(10)==0)
					enem=NULL;
			}
			if(!enem)
				if(frnd && frnd->enem)
					enem=frnd->enem;
				else
					enem=pickhostile();
			if(frnd)
			{
				if(frnd->clk && frnd->clk->cap!=0)
					cloak();
				if(enem && !see(frnd))
					enem=NULL;
				if(frnd->frnd)
					frnd=frnd->frnd;
				if(frnd==this)
					frnd=NULL;
			}
			else
				frnd=pickally();
		}
		break;
	}
}

ship* ship::pickhostile()
{
        for(int i=0,j=0;i<ISIZE;i++)
        {
                j=calc::rnd(ISIZE);
                if(ships[j] && ships[j]!=this && all->opposes(ships[j]->all) && see(ships[j]))
                        return ships[j];
        }
        return NULL;
}

ship* ship::pickally()
{
        for(int i=0,j=0;i<ISIZE;i++)
        {
                j=calc::rnd(ISIZE);
                if(ships[j] && ships[j]!=this && !ships[j]->ply && all==ships[j]->all && see(ships[j]))
                        return ships[j];
        }
        return NULL;
}

void ship::lynch()
{
	for(int i=0;i<ISIZE;i++)
		if(ships[i] && !ships[i]->ply && ships[i]!=this && !ships[i]->enem && ships[i]->see(this))
			ships[i]->enem=this;
}

void ship::firecontrol(int str)
{
	if(enem && see(enem))
	{
		shoot(false);
		//Shoot torpedoes if they appear more threatening; i.e. have a greater maximum shield capacity
		if(str>50 && enem->shd && this->shd && ((enem->shd->item->cap)*2)>(this->shd->item->cap))
		{
			shoot(true);
		}
	}
}

void ship::resequip()
{
	pow=NULL;
	shd=NULL;
	sens=NULL;
	clk=NULL;
	ful=NULL;

	for(int i=0;i<32;i++)
	{
		if(slots[i].item)
		{
			switch(slots[i].item->typ)
			{
				case equip::POWER:
				pow=&slots[i];
				break;

				case equip::SHIELD:
				shd=&slots[i];
				break;

				case equip::SENSOR:
				sens=&slots[i];
				break;

				case equip::CLOAK:
				clk=&slots[i];
				break;

				case equip::FUELTANK:
				ful=&slots[i];
				break;
			}
		}
	}
}

ship* ship::ships[ISIZE];
ship* ship::lib[LIBSIZE];
int ship::mstr;
