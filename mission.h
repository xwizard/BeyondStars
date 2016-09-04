/*
	mission.h
	
	(c) Richard Thrippleton
	Licensing terms are in the 'LICENSE' file
	If that file is not included with this source then permission is not given to use this source in any way whatsoever.
*/

class player;
class ship;
class planet;

class mission //Handles in progress missions
{
	public:
	static const ISIZE=64; //Size of mission index	
	enum {CARGO=0,ESCORT=1,DESTROY=2,DEFEND=3,RECON=4,CAPTURE=5}; //Mission types

	mission(alliance* all,int typ); //Constructor generates a mission of given type for given alliance

	static void init(); //Initialise the missions subsystem
	static void poll(); //Poll current missions, see how they're progressing
		
	private:
	static mission* missions[ISIZE]; //Index of current missions
	static int lev; //Mission difficulty level in current environment

	int typ; //Mission type
	long pay; //Mission pay
	player* cmdr; //Mission commander
	ship* help; //Ship assisting
	alliance* all; //Alliance controlling the mission
	planet* home; //Home planet of mission, place to return for payment
	ship* sinv; //Involved ship (if any)
	planet* pinv; //Involved planet (if any)
	equip* crf; //Cargo if appropriate
}
