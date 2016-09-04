/*
	server.cc
	
	(c) Richard Thrippleton
	Licensing terms are in the 'LICENSE' file
	If that file is not included with this source then permission is not given to use this source in any way whatsoever.
*/

#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include "sockhelper.h"
#include "calc.h"
#include "error.h"
#include "protocol.h"
#include "ship.h"
#include "frag.h"
#include "planet.h"
#include "alliance.h"
#include "constants.h"
#include "player.h"
#include "os.h"
#include "server.h"

struct octets //Structure for extraction of octets from an IP
{
	unsigned char o1;
	unsigned char o2;
	unsigned char o3;
	unsigned char o4;
};

void server::init()
{
	for(int i=0;i<ISIZE;i++)
		connections[i]=NULL;
	lstn=NULL;
}

void server::start(bool locg)
{
	IPaddress serv; //Server listening interface

	tcks=0;
	qsig=false;

	try
	{
		logf=os::openpersonal("log","a");
	}
	catch(error it)
	{
		logf=stdout;
	}
	
	SDLNet_ResolveHost(&serv,NULL,PORT);
	lstn=SDLNet_TCP_Open(&serv);
	if(!lstn)
		throw error("Can't open listening socket");
	fprintf(logf,"%s: Server started listening on port %hd\n",os::gettime(),PORT);
	fflush(logf);
	server::locg=locg;
	locl=false;
}

void server::stop()
{
	for(int i=0;i<ISIZE;i++)
		if(connections[i])
			delete connections[i];
	if(lstn!=NULL)
	{
		SDLNet_TCP_Close(lstn);
		lstn=NULL;
		fprintf(logf,"%s: Server shutdown\n",os::gettime());
		fflush(logf);
	}
	if(logf && logf!=stdout)
		fclose(logf);
}

void server::cycle()
{
	TCPsocket sock; //Sample socket

	tcks++;
	if(tcks>32766)
		tcks=1;
	sock=NULL;
	if(!locl)
		sock=SDLNet_TCP_Accept(lstn);
	if(sock)
	{
		for(int i=0;i<ISIZE;i++)
		{
			if(!connections[i])
			{
				connections[i]=new server(i,sock);
				if(locg)
					locl=true;
				break;
			}
		}
	}
	for(int i=0;i<ISIZE;i++)
	{
		if(connections[i])
		{
			try
			{
				connections[i]->poll();
				connections[i]->uploads();
			}
			catch(error it)
			{
				connections[i]->log("Dropped from the game: %s",it.str);
				delete connections[i];
			}
		}
	}
	if(qsig)
		throw error("Server caught quit signal");
}

void server::notifydelete(player* ply)
{
	for(int i=0;i<ISIZE;i++)
		if(connections[i] && connections[i]->ply==ply)
			connections[i]->ply=NULL;
}

void server::notifykill(player* ply)
{
	for(int i=0;i<ISIZE;i++)
		if(connections[i] && connections[i]->ply==ply && connections[i]->ply->in)
			connections[i]->kill();
}


void server::hail(player* fr,player* to,char* msg)
{
	char txt[256]; //Communications text
	char* frnm; //Name of from

	if(!to)
		return;

	frnm=NULL;
	if(fr)
	{
		frnm=fr->nam;
		if(!frnm)
			if(fr->in)
				frnm=fr->in->cls;
			else
				frnm="";
		sprintf(txt,"%s: %s",frnm,msg);
	}
	else
		sprintf(txt,"%s",msg);

	for(int i=0;i<ISIZE;i++)
	{	
		if(connections[i] && (connections[i]->ply==to || (fr && connections[i]->ply==fr)))
		{
			if(fr && fr->in)
				connections[i]->hilight(fr->in);
			connections[i]->printtomesg("%s",txt);
			if(connections[i]->ply==to)
			{
				if(connections[i]->ply->in)
					server::registersound(connections[i]->ply->in,SND_COMM);
				connections[i]->log("Received hail \"%s\"",txt);
			}
		}
	}
}

void server::bulletin(char* fmt,...)
{
	char buf[132]; //Outgoing buffer
	va_list fmts;

	if(fmt[0]!='\0')
	{
		va_start(fmts,fmt);
		vsprintf(buf,fmt,fmts);
		va_end(fmts);

		for(int i=0;i<ISIZE;i++)
			if(connections[i])
				connections[i]->printtomesg("%s",buf);
		
		fprintf(logf,"%s: Bulletin - %s\n",os::gettime(),buf);
		fflush(logf);
	}
}

void server::registernoise(ship* fr,int snd)
{
	unsigned char buf[SERV_NOISE_SZ]; //Buffer for sending sound

	for(int i=0;i<ISIZE;i++)
	{
		if(connections[i] && connections[i]->ply && connections[i]->ply->in && connections[i]->lsnd!=snd && connections[i]->ply->in->see(fr))
		{	
			connections[i]->lsnd=snd;	
			buf[0]=SERV_NOISE;
			calc::inttodat(snd,buf+1);
			calc::inttodat(ship2pres(fr->self),buf+3);
			connections[i]->hlpr->send(buf,SERV_NOISE_SZ);
		}
	}
}

void server::registersound(ship* to,int snd)
{
	unsigned char buf[SERV_SND_SZ]; //Buffer for sending sound

	for(int i=0;i<ISIZE;i++)
	{
		if(connections[i] && connections[i]->ply && connections[i]->ply->in==to && connections[i]->lsnd!=snd)
		{
			connections[i]->lsnd=snd;
			buf[0]=SERV_SND;
			calc::inttodat(snd,buf+1);
			connections[i]->hlpr->send(buf,SERV_SND_SZ);
		}

	}
}

void server::registershake(ship* to,int mag)
{
	unsigned char buf[SERV_SHAKE_SZ]; //Buffer for sending shake

	for(int i=0;i<ISIZE;i++)
	{
		if(connections[i] && connections[i]->ply && connections[i]->ply->in==to)
		{
			buf[0]=SERV_SHAKE;
			calc::inttodat(mag,buf+1);
			connections[i]->hlpr->send(buf,SERV_SHAKE_SZ);
		}

	}
}

void server::quitsignal(int sig)
{
	qsig=true;
}

server::server(int self,TCPsocket sock)
{
	unsigned long ip; //ip of connecting client
	octets* oip; //Ip as octets, for writing to Ip record

	this->self=self;
	this->sock=sock;
	hlpr=NULL;
	hlpr=new sockhelper(sock);
	hlpr->pump();
	auth=false;
	lsnd=-1;
	inpb[0]='\0';
	ply=NULL;
	foc=-1;
	shpu=new bool[ship::ISIZE];
	for(int i=0;i<ship::ISIZE;i++)
		shpu[i]=false;
	plnu=new bool[planet::ISIZE];
	for(int i=0;i<planet::ISIZE;i++)
		plnu[i]=false;
	frgu=new bool[frag::ISIZE];
	for(int i=0;i<frag::ISIZE;i++)
		frgu[i]=false;
	log("Connection opened");
	urat=10;
	ip=SDLNet_TCP_GetPeerAddress(sock)->host;
	oip=(octets*)&ip;
	if(oip->o1==127 && oip->o2==0 && oip->o3==0 && oip->o4==1)
		cbnd=999999999;
	else
		cbnd=0;
	tout=0;
}

server::~server()
{
	if(ply)
		ply->logout();

	log("Connection terminated");
	connections[self]=NULL;
	if(hlpr)
		delete hlpr;
	SDLNet_TCP_Close(sock);
}

void server::log(char* fmt,...)
{
	unsigned long ip; //ip of connecting client
	octets* oip; //Ip as octets, for writing to Ip record
	va_list fmts; //For resolving the format string

	fprintf(logf,"%s: ",os::gettime());
	fprintf(logf,"[%hd] ",self);
	if(ply)
		fprintf(logf,"%s ",ply->nam);
	else
		fprintf(logf," ");
	ip=SDLNet_TCP_GetPeerAddress(sock)->host;
	oip=(octets*)&ip;
	fprintf(logf,"(%hd.%hd.%hd.%hd) ",(int)oip->o1,(short)oip->o2,(short)oip->o3,(short)oip->o4);
	va_start(fmts,fmt);
	vfprintf(logf,fmt,fmts);
	va_end(fmts);
	fprintf(logf,"\n");
	fflush(logf);
}

void server::poll()
{
	int typ,opr; //Action description
	unsigned char* buf; //Incoming data buffer
	long sbnd; //Server bandwidth used by this player
	unsigned char fldb[SERV_FLOOD_SZ]; //Flooding buffer

	hlpr->pump();
	tout++;
	if(tout>200)
		throw error("Activity timeout");
	if(auth)
	{
		lsnd=-1;
		for(int i=0;i<32;i++)
			acth[i]=false;
		for(int i=0;i<4;i++)
		{
			buf=hlpr->request(3);
			if(buf)
			{
				typ=((int)buf[0]);
				opr=calc::dattoint(buf+1);

				if(typ>=0 && typ<32)
				{
					if(typ==CLIENT_CHAR || !acth[typ])
					{
						action(typ,opr);
						hlpr->suck();
					}
					acth[typ]=true;
				}
				else
					hlpr->suck();
			}
			else
				break;
			tout=0;
		}
	}
	else
	{
		buf=hlpr->request(6);
		if(buf)
		{
			if(calc::dateq((unsigned char*)SIGN,buf,6))
				auth=true;
			else
				throw error("Using incorrect protocol");
			hlpr->suck();
			fldb[0]=SERV_FLOOD;
			for(int i=1;i<SERV_FLOOD_SZ;i++)
				fldb[i]=0;
			for(int i=0;i<110;i++)
				hlpr->send(fldb,SERV_FLOOD_SZ);
			changecmod(CMOD_NAME);
		}
	}
	if(tcks%24==0)
	{
		sbnd=hlpr->getcount();
		if(sbnd>(cbnd/2))
			urat+=6;
		if(sbnd<(cbnd/2))
			urat-=2;
		if(urat<10)
			urat=10;
		if(urat>70)
			urat=70;
	}
}

void server::action(int typ,short opr)
{
	if((typ==CLIENT_CONS || typ==CLIENT_CHAR || typ==CLIENT_BANDWIDTH) || (ply && ply->in))
	{
		try
		{
			switch(typ)
			{
				case CLIENT_ACCEL:
				ply->in->aity=ship::AI_NULL;
				if(opr==-2)
					ply->in->accel(-1,true);	
				if(opr==-1)
					ply->in->accel(-1,false);	
				if(opr==1)	
					ply->in->accel(+1,false);	
				if(opr==2)	
					ply->in->accel(+1,true);	
				break;

				case CLIENT_TURN:
				ply->in->aity=ship::AI_NULL;
				if(opr==-1)
					ply->in->turn(-1);
				if(opr==+1)
					ply->in->turn(+1);
				break;

				case CLIENT_SHOOT:
				if(opr==0)
					ply->in->shoot(false);
				if(opr==1)
					ply->in->shoot(true);
				break;

				case CLIENT_TRG:
				ply->in->aity=ship::AI_NULL;
				ply->in->enem=NULL;
				ply->in->frnd=NULL;
				ply->in->plnt=NULL;
				if(opr>=0 && opr<(planet::ISIZE+ship::ISIZE))
				{
					if(opr<planet::ISIZE)
						ply->in->plnt=planet::get(opr);
					else
					{
						ply->in->enem=ship::get(opr-planet::ISIZE);
						if(ply->in->enem==ply->in)
							ply->in->enem=NULL;
						if(ply->in->enem)
							if(ply->in->all->opposes(ply->in->enem->all))
								registersound(ply->in,SND_PROXIMITY);
							else
								registersound(ply->in,SND_BEEP2);
					}
				}
				if(cmod==CMOD_SCAN || cmod==CMOD_HAIL || cmod==CMOD_WHOIS)
					changecmod(cmod);
				if(cmod==CMOD_REFIT)
					changecmod(CMOD_HAIL);
				break;

				case CLIENT_CMOD:
				switch(opr)
				{
					case REQ_STAT:
					changecmod(CMOD_STAT);
					break;

					case REQ_EQUIP:
					changecmod(CMOD_EQUIP);
					break;

					case REQ_SCAN:
					changecmod(CMOD_SCAN);
					break;

					case REQ_HAIL:
					changecmod(CMOD_HAIL);
					break;

					case REQ_CHAT:
					changecmod(CMOD_CHAT);
					break;

					case REQ_WHOIS:
					changecmod(CMOD_WHOIS);
					break;

					case REQ_HACK:
					changecmod(CMOD_HACK);
					break;
				}
				break;

				case CLIENT_CONS:
				cons(opr);
				break;

				case CLIENT_CHAR:
				if(opr==0)
				{
					input();
					inpb[0]='\0';
				}
				if(opr>=32 && opr<127)
				{
					if(opr>=65 && opr<=90)
						opr+=32;
					for(int i=0;i<64;i++)
					{
						if(inpb[i]=='\0')
						{
							inpb[i]=opr;
							inpb[i+1]='\0';
							break;
						}
					}
				}
				break;

				case CLIENT_BANDWIDTH:
				if(opr>cbnd)
				{
					cbnd=opr;
					log("Bandwidth exploration reports %ld maximum",cbnd);
				}
				break;

				default:
				break;
			}
		}
		catch(error it)
		{
			printtomesg(it.str);
			changecmod(cmod);
		}
	}
}

void server::changecmod(int opr)
{
	char* txtp;
	char txt[1024]; //Text buffer for constructing output
	alliance* tali; //Alliance for possible choosing
	int spr; //Sprite to spritetocons with

	cmod=opr;
	txtp=txt;
	spr=-1;
	txt[0]='\0';
	if(ply && ply->in)
		registersound(ply->in,SND_BEEP1);
	if(cmod!=CMOD_NAME && cmod!=CMOD_PASS && cmod!=CMOD_CHOOSE && !(ply && ply->in))
		return;
	switch(opr)
	{
		case CMOD_NAME:
		printtocons("Choose player name\n");
		requestline(false);
		break;

		case CMOD_PASS:
		printtocons("Player name currently exists\nPlease input player password");
		requestline(true);
		break;

		case CMOD_CHOOSE:
		txtp+=sprintf(txtp,"Choose alliance\n");
		for(int i=0;i<alliance::LIBSIZE;i++)
		{
			tali=alliance::get(i);
			if(tali)
				txtp+=sprintf(txtp,"[%hd] %s\n",i,tali->nam);
		}
		printtocons(txt);
		break;

		case CMOD_STAT:
		spr=ply->in->interact(txt,CMOD_STAT,-1,ply->in);
		printtocons(txt);
		spritetocons(spr);
		break;
		case CMOD_EQUIP:
		ply->in->interact(txt,CMOD_EQUIP,-1,ply->in);
		printtocons(txt);
		break;

		case CMOD_SCAN:
		if(ply->in->enem)
		{
			spr=ply->in->enem->interact(txt,CMOD_SCAN,-1,ply->in);
			printtocons(txt);
			spritetocons(spr);
		}
		else
		{
			if(ply->in->plnt)
			{
				spr=ply->in->plnt->interact(txt,CMOD_SCAN,-1,ply->in);
				printtocons(txt);
				spritetocons(spr);
			}
			else
			{
				printtocons("No target");
			}
		}
		break;

		case CMOD_HAIL:
		if(ply->in->enem)
		{
			ply->in->enem->interact(txt,CMOD_HAIL,-1,ply->in);
			printtocons(txt);
		}
		else
		{
			if(ply->in->plnt)
			{
				ply->in->plnt->interact(txt,CMOD_HAIL,-1,ply->in);
				printtocons(txt);
			}
			else
			{
				printtocons("No target");
			}
		}
		break;
		
		case CMOD_REFIT:
		if(ply->in->plnt)
		{
			ply->in->plnt->interact(txt,CMOD_REFIT,-1,ply->in);
			printtocons(txt);
		}
		break;

		case CMOD_CHAT:
		txtp+=sprintf(txtp,"Messaging\n\n");
		if(ply->in->enem && ply->in->enem->ply)
			txtp+=sprintf(txtp,"[1] Chat with target player\n");
		txtp+=sprintf(txtp,"[2] Chat with team\n");
		txtp+=sprintf(txtp,"[3] Chat with all\n");
		printtocons(txt);
		break;

		case CMOD_CHATPRIVATE:
		printtocons("Chat with target player\n");
		requestline(false);
		break;

		case CMOD_CHATTEAM:
		printtocons("Chat with team");
		requestline(false);
		break;

		case CMOD_CHATALL:
		printtocons("Chat with all");
		requestline(false);
		break;

		case CMOD_WHOIS:
		txtp+=sprintf(txtp,"WHOIS\n\n");
		if(ply->in->enem)
		{
			spr=ply->in->enem->interact(txtp,CMOD_WHOIS,-1,ply->in);
			txtp=txt+strlen(txt);
		}
		else
			txtp+=sprintf(txtp,"No target\n");
		txtp+=sprintf(txtp,"\n[1] Cycle to next player");
		printtocons(txt);
		spritetocons(spr);
		break;

		case CMOD_HACK:
		if(ply->op && !locg)
		{
			printtocons("Admin\n\n[1] Set account password\n[2] Kick user\n[3] Delete user\n[4] Shutdown server");
		}
		else
		{
			printtocons("Admin\n\n[1] Set account password");
		}
		break;
		
		case CMOD_PASS1:
		printtocons("Choose your password");
		requestline(true);
		break;

		case CMOD_PASS2:
		printtocons("Confirm your password");
		requestline(true);
		break;

		case CMOD_KICK:
		printtocons("Input username to kick");
		requestline(false);
		break;

		case CMOD_DELETE:
		printtocons("Input username to delete");
		requestline(false);
		break;

	}
}

void server::cons(int opr)
{
	char txt[1024]; //Text buffer for constructing output
	alliance* tali; //Alliance to choose
	ship* tshp; //Temporary ship scratchpad

	if(opr<0)
		return;
	if(cmod!=CMOD_CHOOSE && !(ply && ply->in))
		return;
	txt[0]='\0';
	switch(cmod)
	{
		case CMOD_CHOOSE:
		tali=alliance::get(opr);
		if(tali)
		{
			ply->spawn(tali);
			log("Spawned as %s(%s)",ply->in->cls,ply->in->all->nam);
			changecmod(CMOD_HACK);
		}
		else
		{
			printtomesg("No such alliance");
			changecmod(CMOD_CHOOSE);
		}
		break;

		case CMOD_EQUIP:
		ply->in->interact(txt,CMOD_EQUIP,opr,ply->in);
		printtomesg(txt);
		changecmod(CMOD_EQUIP);
		break;

		case CMOD_SCAN:
		if(ply->in->enem)
			ply->in->enem->interact(txt,CMOD_SCAN,opr,ply->in);
		else
			if(ply->in->plnt)
				ply->in->plnt->interact(txt,CMOD_SCAN,opr,ply->in);
		break;

		case CMOD_HAIL:
		if(ply->in->enem)
		{
			ply->in->enem->interact(txt,CMOD_HAIL,opr,ply->in);
			printtomesg(txt);
			changecmod(CMOD_HAIL);
		}
		else
		{
			if(ply->in->plnt)
			{
				if(opr==5)
				{
					if(ply->in->see(ply->in->plnt))
					{
						if(ply->in->plnt->all==ply->in->all)
						{
							ply->in->transport(ply->in->plnt);
							ply->commit();
							printtomesg("Restore position saved");
						}
						else
							throw error("Cannot save with a different allegiance");
					}
					else
					{
						throw error("Out of range");
					}
				}
				else if(opr==4)
				{
					changecmod(CMOD_REFIT);
				}
				else
				{
					ply->in->plnt->interact(txt,CMOD_HAIL,opr,ply->in);
					printtomesg(txt);
					changecmod(CMOD_HAIL);
				}
			}
		}
		break;

		case CMOD_REFIT:
		if(ply->in->plnt)
		{
			ply->in->plnt->interact(txt,CMOD_REFIT,opr,ply->in);
			printtomesg(txt);
			changecmod(CMOD_REFIT);
		}
		break;

		case CMOD_CHAT:
		if(opr==1 && ply->in->enem && ply->in->enem->ply)
			changecmod(CMOD_CHATPRIVATE);
		if(opr==2)
			changecmod(CMOD_CHATTEAM);
		if(opr==3)
			changecmod(CMOD_CHATALL);
		break;

		case CMOD_WHOIS:
		if(opr==1)
		{
			for(int i=0,j=(ply->in->enem ? ply->in->enem->self+1 : 0);i<ship::ISIZE;i++,j++)
			{
				if(j>=ship::ISIZE)
					j=0;
				tshp=ship::get(j);
				if(tshp && tshp->ply && tshp!=ply->in)
				{
					ply->in->enem=tshp;
					ply->in->frnd=NULL;
					ply->in->plnt=NULL;
					break;
				}
			}
			changecmod(CMOD_WHOIS);
		}
		break;

		case CMOD_HACK:
		if(opr==1)
		{
			changecmod(CMOD_PASS1);
		}
		if(ply->op && !locg)
		{
			if(opr==2)
				changecmod(CMOD_KICK);
			if(opr==3)
				changecmod(CMOD_DELETE);
			if(opr==4)
			{
				log("Server shutdown requested");
				qsig=true;
			}
		}
		break;
	}
}

void server::requestline(bool hide)
{
	unsigned char buf[SERV_READLN_SZ]; //For sending the request byte

	buf[0]=SERV_READLN;
	buf[1]=(unsigned char)hide;
	hlpr->send(buf,SERV_READLN_SZ);
}

void server::input()
{
	player* delp; //Player to delete

	switch(cmod)
	{
		case CMOD_NAME:
		if(inpb[0]=='\0')
			throw error("Aborted name entry");
		try
		{
			if(strlen(inpb)<2)
			{
				inpb[0]='\0';
				throw error("Name too short");
			}
			ply=player::get(inpb);
			if(ply)
			{
				log("Attempting login");
				if(locg)
				{
					ply->login(NULL);
					log("Local game login");
					changecmod(CMOD_STAT);
				}
				else
				{
					changecmod(CMOD_PASS);
				}
			}
			else
			{
				ply=new player(inpb);
				log("New player created");
				changecmod(CMOD_CHOOSE);
			}
		}
		catch(error it)
		{
			ply=NULL;	
			log(it.str);
			printtomesg(it.str);
			changecmod(CMOD_NAME);
		}
		break;

		case CMOD_PASS:
		try
		{
			ply->login(inpb);
			log("Login succeeded");
			changecmod(CMOD_STAT);
		}
		catch(error it)
		{
			ply=NULL;	
			log(it.str);
			printtomesg(it.str);
			changecmod(CMOD_NAME);
		}
		break;

		case CMOD_PASS1:
		inpb[32]='\0';
		strcpy(tpas,inpb);
		changecmod(CMOD_PASS2);
		break;

		case CMOD_PASS2:
		inpb[32]='\0';
		if(strcmp(inpb,tpas)==0)
		{
			ply->setpass(inpb);
			printtomesg("Password set successfully");
			log("Set password");
		}
		else
		{
			printtomesg("Passwords don't match!");
		}
		changecmod(CMOD_HACK);
		break;

		case CMOD_CHATPRIVATE:
		if(ply && ply->in && ply->in->enem && ply->in->enem->ply)
		{
			hail(ply,ply->in->enem->ply,inpb);
			changecmod(CMOD_CHATPRIVATE);
		}
		break;
		
		case CMOD_CHATTEAM:
		if(ply && ply->in)
		{
			for(int i=0;i<ISIZE;i++)
				if(connections[i] && connections[i]->ply && connections[i]->ply->in && connections[i]->ply->in->all==ply->in->all)
					hail(ply,connections[i]->ply,inpb);
			changecmod(CMOD_CHATTEAM);
		}
		break;
		
		case CMOD_CHATALL:
		if(ply->in)
		{
			for(int i=0;i<ISIZE;i++)
				if(connections[i] && connections[i]->ply)
					hail(ply,connections[i]->ply,inpb);
			changecmod(CMOD_CHATALL);
		}
		break;

		case CMOD_KICK:
		log("Attemped kick of %s",inpb);
		for(int i=0;i<ISIZE;i++)
		{
			if(connections[i] && connections[i]!=this && connections[i]->ply && strcmp(inpb,connections[i]->ply->nam)==0)
			{
				bulletin("%s was kicked from the server",inpb);
				delete connections[i];
				break;
			}
		}
		changecmod(CMOD_HACK);
		break;
		
		case CMOD_DELETE:
		log("Attemped deletion of %s",inpb);
 		for(int i=0;i<ISIZE;i++)
		{
			if(connections[i] && connections[i]!=this && connections[i]->ply && strcmp(inpb,connections[i]->ply->nam)==0)
			{
				bulletin("%s was kicked from the server",inpb);
				delete connections[i];
				break;
			}
		}
		delp=player::get(inpb);
		if(delp && delp!=ply)
			delete delp;
		changecmod(CMOD_HACK);
		break;

 
	}
}

void server::printtocons(char* fmt,...)
{
	unsigned char buf[1028]; //Outgoing buffer
	va_list fmts;

	va_start(fmts,fmt);
	vsprintf((char*)buf+3,fmt,fmts);
	va_end(fmts);

	buf[0]=SERV_CONS;
	calc::inttodat(strlen((char*)buf+3),buf+1);

	hlpr->send(buf,strlen((char*)buf+3)+3);
}

void server::spritetocons(int indx)
{
	unsigned char buf[SERV_CSPR_SZ]; //Outgoing buffer

	if(indx>=0)
	{
		buf[0]=SERV_CSPR;
		calc::inttodat(indx,buf+1);
		hlpr->send(buf,SERV_CSPR_SZ);
	}
}

void server::printtomesg(char* fmt,...)
{
	unsigned char buf[132]; //Outgoing buffer
	va_list fmts;

	if(fmt[0]!='\0')
	{
		va_start(fmts,fmt);
		vsprintf((char*)buf+3,fmt,fmts);
		va_end(fmts);

		buf[0]=SERV_MESG;
		calc::inttodat(strlen((char*)buf+3),buf+1);

		hlpr->send(buf,strlen((char*)buf+3)+3);
	}
}

void server::uploads()
{
	unsigned char buf[256]; //Buffer for outgoing data

	if(ply && ply->in)
	{
		ply->in->netout(SERV_SELF,buf);
		hlpr->send(buf,SERV_SELF_SZ);
		calc::inttodat(foc,buf+21);
		/*if(!shpu[ply->in->self])
		{
			ply->in->netout(SERV_NEW,buf);
			hlpr->send(buf,SERV_NEW_SZ);
			ply->in->netout(SERV_NAME,buf);
			hlpr->send(buf,SERV_NAME_SZ);
			shpu[ply->in->self]=true;
		}
		ply->in->netout(SERV_UPD,buf);
		buf[21]=0;
		hlpr->send(buf,SERV_UPD_SZ);*/
		
		if(tcks%(urat/10)==0)
		{
			if(tcks%((urat*4)/10)==0)
				uploadplanets();
			uploadships();
			uploadfrags();
		}
		hlpr->pump();
	}
}

void server::uploadplanets()
{
	unsigned char buf[256]; //Outgoing scratchpad buffer to use
	planet* tpln; //Concerned planet

	for(int i=0;i<planet::ISIZE;i++)
	{
		tpln=planet::get(i);
		if(tpln)
		{
			if(ply->in->see(tpln))
			{
				if(!plnu[i])
				{
					tpln->netout(SERV_NEW,buf);
					hlpr->send(buf,SERV_NEW_SZ);
					tpln->netout(SERV_NAME,buf);
					hlpr->send(buf,SERV_NAME_SZ);
					plnu[i]=true;
				}
				tpln->netout(SERV_UPD,buf);
				hlpr->send(buf,SERV_UPD_SZ);
			}
			else
			{
				if(plnu[i])
				{
					buf[0]=SERV_DEL;
					calc::inttodat(planet2pres(i),buf+1);
					hlpr->send(buf,SERV_DEL_SZ);
					plnu[i]=false;
				}
			}
		}
		else
		{
			if(plnu[i])
			{
				buf[0]=SERV_DEL;
				calc::inttodat(planet2pres(i),buf+1);
				hlpr->send(buf,SERV_DEL_SZ);
				plnu[i]=false;
			}
		}
	}
}

void server::uploadships()
{
	unsigned char buf[256]; //Outgoing scratchpad buffer to use
	ship* tshp; //Concerned ship

	for(int i=0;i<ship::ISIZE;i++)
	{
		tshp=ship::get(i);
		//if(tshp!=ply->in)
		{
			if(tshp)
			{
				if(ply->in->see(tshp))
				{
					if(!shpu[i])
					{
						tshp->netout(SERV_NEW,buf);
						hlpr->send(buf,SERV_NEW_SZ);
						tshp->netout(SERV_NAME,buf);
						hlpr->send(buf,SERV_NAME_SZ);
						shpu[i]=true;
					}
					tshp->netout(SERV_UPD,buf);
					if(ply->in->all->opposes(tshp->all))
						buf[21]=1;
					hlpr->send(buf,SERV_UPD_SZ);
				}
				else
				{
					if(shpu[i])
					{
						buf[0]=SERV_DEL;
						calc::inttodat(ship2pres(i),buf+1);
						hlpr->send(buf,SERV_DEL_SZ);
						shpu[i]=false;
					}
				}
			}
			else
			{
				if(shpu[i])
				{
					buf[0]=SERV_DEL;
					calc::inttodat(ship2pres(i),buf+1);
					hlpr->send(buf,SERV_DEL_SZ);
					shpu[i]=false;
				}
			}
		}
	}
}

void server::uploadfrags()
{
	unsigned char buf[256]; //Outgoing scratchpad buffer to use
	frag* tfrg; //Concerned frag

	for(int i=0;i<frag::ISIZE;i++)
	{
		tfrg=frag::get(i);
		if(tfrg)
		{
			if(ply->in->see(tfrg))
			{
				if(frgu[i])
				{
					tfrg->netout(SERV_UPD,buf);
					hlpr->send(buf,SERV_UPD_SZ);
				}
				else
				{
					tfrg->netout(SERV_NEW,buf);
					hlpr->send(buf,SERV_NEW_SZ);
					tfrg->netout(SERV_UPD,buf);
					hlpr->send(buf,SERV_UPD_SZ);
					frgu[i]=true;
				}
			}
			else
			{
				if(frgu[i])
				{
					buf[0]=SERV_DEL;
					calc::inttodat(frag2pres(i),buf+1);
					hlpr->send(buf,SERV_DEL_SZ);
					frgu[i]=false;
				}
			}
		}
		else
		{
			if(frgu[i])
			{
				buf[0]=SERV_DEL;
				calc::inttodat(frag2pres(i),buf+1);
				hlpr->send(buf,SERV_DEL_SZ);	
				frgu[i]=false;
			}
		}
	}
}

void server::kill()
{
	unsigned char buf[SERV_DEL_SZ]; //Buffer for deleting player's ship clientside

	try
	{
		uploadplanets();
		uploadships();
		uploadfrags();
		buf[0]=SERV_DEL;
		calc::inttodat(ship2pres(ply->in->self),buf+1);
		hlpr->send(buf,SERV_DEL_SZ);
		printtomesg("You have been destroyed: Game Over");
		printtocons("Game over");
	}
	catch(error it)
	{
	}
}

void server::hilight(ship* tshp)
{
	unsigned char buf[SERV_HILIGHT_SZ]; //Buffer for sending hilight information

	if(tshp)
	{
		buf[0]=SERV_HILIGHT;
		calc::inttodat(ship2pres(tshp->self),buf+1);
		hlpr->send(buf,SERV_HILIGHT_SZ);
	}
}

server* server::connections[ISIZE];
long server::tcks;
TCPsocket server::lstn;
bool server::qsig;
FILE* server::logf;
bool server::locg;
bool server::locl;
