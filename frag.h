/*
	frag.h
	
	(c) Richard Thrippleton
	Licensing terms are in the 'LICENSE' file
	If that file is not included with this source then permission is not given to use this source in any way whatsoever.
*/

class ship;
struct cord;

class frag //A frag object, like phaser fire, torpedo, debris
{
	public:
	enum {ENERGY=1,HOMER=2,DEBRIS=3}; //Frag types
	enum {FIRE=1}; //Index of burning fire graphic
	static const int ISIZE=512; //Capacity of frag index

	frag(cord loc,int typ,short spr,short col,ship* trg,ship* own,vect mov,short rot,short pow,short trck,short rng); //Constructor to create a new frag from scratch

	~frag(); //Destructor

	static void init(); //Initialise the frag system datastructures
	static void purgeall(); //Clean out the frag system after use
	static void simulateall(); //Simulate all the frags and physics
	static void notifydelete(ship* tshp); //Notify the frags of the deletion of a ship, to resolve dangling target and owner references
	static void saveall(); //Save all frags to the database
	static void loadall(); //Load all frags from database
	static frag* get(int indx); //Return the frag of given index

	void netout(int typ,unsigned char* buf); //Get type of data from frag into a network buffer

	int self; //Self index in the database
	cord loc; //Position
	int typ; //Type (see the top enum)
	ship* trg; //Target ship index
	ship* own; //Owner ship index

	private:
	frag(int self); //Constructor, give it its index value and it will load from the database

	void physics(); //Move the frag, do collisions
	void home(); //Sub-function of physics, handles homing
	void save(); //Save frag to a database
	void load(); //Load frag from a database

	static frag* frags[ISIZE]; //The frag index

	int spr; //Associated sprite index
	int col; //Colour if relevant (makes it a beam weapon)
	long ox,oy; //Old co-ordinates, for purposes of clientside interpolation
	vect mov; //Velocity vector
	int rot; //Rotation frame to use
	int pow; //Power/damage
	long trck; //Tracking power
	int rng; //Range left
};
