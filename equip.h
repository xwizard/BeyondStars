/*
	equip.h
	
	(c) Richard Thrippleton
	Licensing terms are in the 'LICENSE' file
	If that file is not included with this source then permission is not given to use this source in any way whatsoever.
*/

class equip //Equipment item
{
	public:
	enum {LIBSIZE=64}; //Maximum equipment library size
	enum {PHASER=1,LAUNCHER=2,TRANSPORTER=3,CLOAK=4,POWER=5,SHIELD=6,SENSOR=7,FUELTANK=8,CARGO=9}; //Different equipment types

	static void init(); //Initialise the equipment datastructures
	static void loadlib(); //Load the equipment library
	static equip* get(int indx); //Get an equipment item by index

	int self; //Self index in the equipment cache
	char nam[65]; //Name
	int typ; //Type (see top enum);
	int mss; //Mass
	int spr; //Associated sprite index
	int col; //Colour (makes it a beam weapon)
	int snd; //Associated sound index
	int pow; //Power consumption
	int rdy; //Readiness cycle time
	long cap; //Capacity
	long rng; //Range
	int trck; //Tracking power
	int acov; //Angle coverage
	long cost; //Base cost

	private:
	equip(int self); //Constructor, give the equipment self-index value
	void load(); //Load this equipment from the database

	static equip* equips[LIBSIZE]; //Equipment database
};
