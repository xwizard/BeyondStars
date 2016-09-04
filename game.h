/*
	game.h
	
	(c) Richard Thrippleton
	Licensing terms are in the 'LICENSE' file
	If that file is not included with this source then permission is not given to use this source in any way whatsoever.
*/

class game //This modules allows access to start a general game type
{
	public:
	static void runheadless(); //Run as a headless server
	static void runlocal(); //Run a local single player game
	static void runclient(char* host); //Just run the client, connecting to given hostname

	private:
	static void save(); //Save the entire game state to the universe file
	static void load(); //Load the entire game state from the universe file
};
