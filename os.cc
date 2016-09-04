/*
	os.cc
	
	(c) Richard Thrippleton
	Licensing terms are in the 'LICENSE' file
	If that file is not included with this source then permission is not given to use this source in any way whatsoever.
*/

#include <SDL.h>
#include <SDL_net.h>
#include <signal.h>
#include <string.h>
#ifdef POSIX
#include <unistd.h>
#include <pwd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <time.h>
#endif
#include "error.h"
#include "server.h"
#include "os.h"

void os::init()
{
	#ifdef POSIX
	signal(SIGPIPE,SIG_IGN);
	signal(SIGTERM,server::quitsignal);
	#endif
	SDL_Init(SDL_INIT_TIMER|SDL_INIT_NOPARACHUTE);
	SDLNet_Init();
}

void os::finish()
{
	SDLNet_Quit();
	SDL_Quit();
}

FILE* os::openpersonal(const char* fnam,const char* flag)
{
	char* path; //Path to generate
	FILE* out; //File handler to return

	#ifdef POSIX
	DIR* dir; //Opened dir
	passwd* me; //Who are we running as?

	me=getpwuid(getuid());
	if(!me)
		throw error("getpwuid failed: cannot find user's home directory");
	path=new char[strlen(fnam)+strlen(me->pw_dir)+15];
	sprintf(path,"%s/.starvoyager",me->pw_dir);
	dir=opendir(path);
	if(dir)	
		closedir(dir);
	else
		mkdir(path,0700);
	sprintf(path,"%s/.starvoyager/%s",me->pw_dir,fnam);
	#else
	path=new char[strlen(fnam)+1];
	sprintf(path,"%s",fnam);
	#endif

	out=fopen(path,flag);
	delete[] path;
	if(out)
		return out;
	else
		throw error("Cannot open user's file");
}

char* os::gettime()
{
	#ifdef POSIX
	time_t tst; //Time structure

	time(&tst);
	sprintf(tbuf,"%s",ctime(&tst));
	tbuf[strlen(tbuf)-1]='\0';
	#else
	tbuf[0]='\0';
	#endif		
	return tbuf;
}

long os::getseed()
{
	#ifdef POSIX
	return (long)time(NULL);
	#else
	return 12345;
	#endif
}

char os::tbuf[256];
