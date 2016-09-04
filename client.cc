/*
	client.cc	
	
	(c) Richard Thrippleton
	Licensing terms are in the 'LICENSE' file
	If that file is not included with this source then permission is not given to use this source in any way whatsoever.
*/

#include <string.h>
#include "calc.h"
#include "sockhelper.h"
#include "protocol.h"
#include "sound.h"
#include "presence.h"
#include "camera.h"
#include "error.h"
#include "interface.h"
#include "graphic.h"
#include "client.h"

void client::init()
{
	sock=NULL;
	hlpr=NULL;
	edit=false;
}

void client::stop()
{
	if(sock)
	{
		SDLNet_TCP_Close(sock);
		sock=NULL;
	}
	if(hlpr)
	{
		delete hlpr;
		hlpr=NULL;
	}
}

void client::connect(char* host)
{
	IPaddress serv; //Server address

	SDLNet_ResolveHost(&serv,host,PORT);
	if(serv.host==INADDR_NONE)
		throw error("Couldn't resolve hostname");
	sock=SDLNet_TCP_Open(&serv);
	if(!sock)
		throw error("Could not connect to server");
	hlpr=new sockhelper(sock);
	hlpr->send((unsigned char*)SIGN,strlen(SIGN));
	btck=0;
}

void client::flush()
{
	hlpr->pump();
}

void client::poll()
{
	unsigned char otyp; //Incoming object type
	unsigned char* buf; //Incoming buffer to use
	sound* snd; //Sound to play
	graphic* cspr; //Console sprite
	int n; //Array subscript to access
	long cbnd; //Client bandwidth usage to report
	presence* from; //Sound source if applicable
	int len; //Length of console/message text if applicable
	char txt[1025]; //Text to print to console
	bool exl; //Loop exiting flag

	hlpr->pump();
	btck=(btck+1)%24;
	if(btck==0)
	{
		cbnd=hlpr->getcount();
		action(CLIENT_BANDWIDTH,cbnd);
	}
	exl=false;
	while(!exl && hlpr->request(1))
	{
		otyp=*hlpr->request(1);
		switch(otyp)
		{
			case SERV_SELF:
			buf=hlpr->request(SERV_SELF_SZ);
			if(buf)
			{
				presence::feed(buf);
				hlpr->suck();
			}
			else
				exl=true;
			break;

			case SERV_NEW:
			buf=hlpr->request(SERV_NEW_SZ);
			if(buf)
			{
				presence::feed(buf);
				hlpr->suck();
			}
			else
				exl=true;
			break;

			case SERV_NAME:
			buf=hlpr->request(SERV_NAME_SZ);
			if(buf)
			{
				presence::feed(buf);
				hlpr->suck();
			}
			else
				exl=true;
			break;

			case SERV_UPD:
			buf=hlpr->request(SERV_UPD_SZ);
			if(buf)
			{
				presence::feed(buf);
				hlpr->suck();
			}
			else
				exl=true;
			break;

			case SERV_DEL:
			buf=hlpr->request(SERV_DEL_SZ);
			if(buf)
			{
				presence::feed(buf);
				hlpr->suck();
			}
			else
				exl=true;
			break;

			case SERV_HILIGHT:
			buf=hlpr->request(SERV_HILIGHT_SZ);
			if(buf)
			{
				presence::feed(buf);
				hlpr->suck();
			}
			else
				exl=true;
			break;

			case SERV_SND:
			buf=hlpr->request(SERV_SND_SZ);
			if(buf)
			{
				snd=sound::get(calc::dattoint(buf+1));
				if(snd)
					snd->play(1);
				hlpr->suck();
			}
			else
				exl=true;
			break;

			case SERV_NOISE:
			buf=hlpr->request(SERV_NOISE_SZ);
			if(buf)
			{
				snd=sound::get(calc::dattoint(buf+1));
				n=calc::dattoint(buf+3);
				if(n>=0 && n<presence::ISIZE)
					from=presence::get(n);
				else
					from=NULL;
				if(snd && from)
					camera::noise(snd,from);
				hlpr->suck();
			}
			else
				exl=true;
			break;

			case SERV_SHAKE:
			buf=hlpr->request(SERV_SHAKE_SZ);
			if(buf)
			{
				camera::shake(calc::dattoint(buf+1));
				hlpr->suck();
			}
			else
				exl=true;
			break;

			case SERV_CONS:
			buf=hlpr->request(3);
			if(buf)
			{
				edit=false;
				len=calc::dattoint(buf+1);
				if(len>1024)
					throw error("Overflow from server in SERV_CONS");
				buf=hlpr->request(3+len);
				if(buf)
				{
					memcpy(txt,buf+3,len);
					txt[len]='\0';
					interface::printtocons("%s\n",txt);
					hlpr->suck();
				}
				else
					exl=true;
			}
			else
				exl=true;
			break;

			case SERV_READLN:
			buf=hlpr->request(SERV_READLN_SZ);
			if(buf)
			{
				edit=true;
				if(buf[1])
					hide=true;
				else
					hide=false;
				hlpr->suck();
			}
			else
				exl=true;
			break;

			case SERV_MESG:
			buf=hlpr->request(3);
			if(buf)
			{
				len=calc::dattoint(buf+1);
				if(len>128)
					throw error("Overflow from server in SERV_MESG");
				buf=hlpr->request(3+len);
				if(buf)
				{
					memcpy(txt,buf+3,len);
					txt[len]='\0';
					interface::printtomesg("%s",txt);
					hlpr->suck();
				}
			}
			else
				exl=true;
			break;

			case SERV_CSPR:
			buf=hlpr->request(3);
			if(buf)
			{
				cspr=graphic::get(calc::dattoint(buf+1));
				if(cspr)
					interface::spritetocons(cspr);
				hlpr->suck();
			}
			else
				exl=true;
			break;

			case SERV_FLOOD:
			buf=hlpr->request(SERV_FLOOD_SZ);
			if(buf)
			{
				hlpr->suck();
			}
			hlpr->pump();
			break;

			default:
			error::debug("Strange default packet",otyp);
			break;
		}
	}
	hlpr->pump();
	if(edit)
		readln();
}

void client::action(int typ,long opr)
{
	unsigned char buf[3]; //Outgoing buffer

	if(opr>32767)
		opr=32767;
	if(opr<-32766)
		opr=-32766;
	buf[0]=(unsigned char)typ;
	calc::inttodat(opr,buf+1);
	hlpr->send(buf,3);
}

void client::readln()
{
	char txt[65]; //Readline text

	if(interface::getline(txt,hide))
	{
		edit=false;
		for(int i=0;i<65;i++)
		{
			action(CLIENT_CHAR,txt[i]);
			if(txt[i]=='\0')
				break;
		}
	}
}

bool client::edit;
bool client::hide;
TCPsocket client::sock;
sockhelper* client::hlpr;
int client::btck;
