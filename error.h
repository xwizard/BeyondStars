/*
	error.h
	
	(c) Richard Thrippleton
	Licensing terms are in the 'LICENSE' file
	If that file is not included with this source then permission is not given to use this source in any way whatsoever.
*/

class error //Game failure exception (human readable by design)
{
	public:
	error(char* str); //Constructor sets the string

	static void debug(const char* str,long num); //Prints a debug string and integer immediately to console

	char str[129]; //Error string
};
