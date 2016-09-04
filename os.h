/*
	os.h
	
	(c) Richard Thrippleton
	Licensing terms are in the 'LICENSE' file
	If that file is not included with this source then permission is not given to use this source in any way whatsoever.
*/

#include <stdio.h>

class os //A module defining some things that are regrettably os specific
{
	public:
	static void init(); //Initialise anything os specific, such as signals	
	static void finish(); //OS level cleanup tasks
	static FILE* openpersonal(const char* fnam,const char* flag); //Open up a file of given name from personal settings area, with given flags to fopen
	static char* gettime(); //Get the current time and date as a string
	static long getseed(); //Get a number suitable for a random number seed, usually from the clock

	private:
	static char tbuf[256]; //Time string buffer
};
