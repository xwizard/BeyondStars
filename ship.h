/*
	ship.h
	
	(c) Richard Thrippleton
	Licensing terms are in the 'LICENSE' file
	If that file is not included with this source then permission is not given to use this source in any way whatsoever.
*/

struct pol;
struct vect;
struct cord;
class alliance;
class planet;
class equip;
class frag;
class player;

struct slot //Internal equipment slot of ship
{
	pol pos; //Position (ang=-1 indicates not external)
	double face; //Facing direction
	equip* item; //Equipment item index
	int rdy; //Readiness
	long cap; //Capacity
};

class ship //Spaceship type in game
{
	public:
	const static int ISIZE=256; //Size of ship index
	const static int LIBSIZE=32; //Size of ship library
	enum {AI_NULL=0,AI_AUTOPILOT=1,AI_PATROLLER=2,AI_INVADER=3,AI_CARAVAN=4,AI_BUDDY=5,AI_FLEET=6}; //AI behaviour types
	enum {PRCH_FUEL,PRCH_ARMS,PRCH_HULL}; //Codes for different purchases

	ship(cord loc,ship* lshp,alliance* tali,int aity); //Creates a new ship at given place, with given alliance and given ai type, loaded from given library ship
	ship(); //Blank constructor

	~ship(); //Destructor, deletes ship resolving all dependencies (like frags and other ships)

	static void init(); //Initialise all ship datastructures
	static void loadlib(); //Load the library ships
	static void purgeall(); //Clean up the ship index (erasure)
	static void simulateall(); //Simulate the physics of all the ships
	static void behaveall(); //Simulates primitive behaviour for all ships
	static void saveall(); //Save all ships to database
	static void loadall(); //Load all ships from database
	static ship* get(int indx); //Returns a pointer to a ship of given index
	static ship* libget(int indx); //Returns a ship from the library
	static bool freeslot(); //Quickly looks for a free slot if possible for a (non-player) ship, returning true if ok to go ahead and do the hard work

	void turn(int dir); //Turns the ship by specified amount(in 1/100ths of an angle)
	void accel(int dir,bool wrp); //Accelerates the ship positively or negatively (+1 or -1 in dir), wrp deciding if you are willing to make the impulse<=>warp transition
	void shoot(bool torp); //Shoot at target with torpedoes if torp, otherwise phasers
	bool see(ship* tshp); //Returns true if the given ship is visible to the current one, otherwise false
	bool see(planet* tpln); //Returns true if the given planet is visible to the current ship, otherwise false
	bool see(frag* tfrg); //Returns true if the given frag is visible to the current ship, otherwise false
	int interact(char* txt,short cmod,short opr,ship* mshp); //Handles a server request for information/action from this ship, with the given comm mode, operand and player's ship, writing the text into txt and returning the sprite index (-1 if n/a)
	int freemass(); //Returns free mass on board
	void cloak(); //Cloak the ship
	void uncloak(); //Uncloak the ship
	void shieldsup(); //Raise the shields
	void shieldsdown(); //Drop the shields
	void netout(int typ,unsigned char* buf); //Get type of data from ship into a network buffer
	bool colldetect(cord frgl,vect frgv); //Given an intruder location and vector, determines if a collision occurs, returning true if it does, false otherwise
	void hit(int mag,cord frgl,vect frgv,ship* src); //Damages the ship with given magnitude, also location and vector of striking object, plus the source of the attack (expected to be frag owner)
	void assign(player* ply); //Sets the ship up for entrance of given player
	long purchase(int prch,short ripo,bool buy);
	long purchase(equip* prch,int ripo,bool buy); //Returns the cost of a given purchase type at given rip-off index, with buy set actually deducts the cash
	void transport(planet* to);
	void transport(ship* to); //Attempts transport to destination, deducting power and generating sound as appropriate, error may be thrown if not possible
	void save(); //Saves the ship data to database
	void load(); //Load ship data from database, object should already have been selected
	void insert(); //Insert ship into the game
	void insert(int self); //Insert ship into the game, with given index in slot demanded (say for loading a universe)
	void resequip(); //Resolve key equipment within the ship, should be called whenever the equipment loadout is changed at all

	int self; //Self index in the ship database
	player* ply; //Pointer to player associated, null if none
	alliance* all; //Alliance
	cord loc; //Co-ordinates 
	char cls[65]; //Ship class 
	int typ; //Ship type (index in library)
	int spr; //Sprite index
	ship* frnd; //Friend target
	ship* enem; //Enemy target
	planet* plnt; //Planetary target
	int aity; //AI type
	vect mov; //Current actual velocity vector

	private:
	ship(int self); //Constructor that loads ship of given index out of the database into the universe

	void physics(); //Handles physics of motion on this ship
	void autonav(planet* tpln); //Autonavigate to given planet
	void follow(ship* tshp); //Follow given ship
	void attackpattern(ship* tshp,int str); //Run attack pattern on given ship, given a strobing value
	void loadlink(); //Extension to load that only resolves the links between ships
	void maintain(); //Does routine work such as recharging shields and power, reloading weapons
	void behave(); //Execute characteristic behaviour for this ship
	void act(int actn); //Perform given action type, called by behave()
	ship* pickhostile(); //Return a nearby hostile ship, null if none found
	ship* pickally(); //Return a nearby allied ship, null if none found
	void lynch(); //Get all ships in the area to come attack this ship, usually as a result of friendly fire
	void firecontrol(int str); //Makes AI decisions concerning phaser fire, given a strobing value

	static ship* ships[ISIZE]; //Main ship list
	static ship* lib[LIBSIZE]; //Ship library
	static int mstr; //Master strobe and individual strobe, used for periodic behaviours

	int w[36],h[36]; //Width and height (halved)
	int fspr; //Frag sprite
	int fsnd; //Frag sound
	int dsnd; //Destruction sound
	pol vel; //Velocity
	int trn; //Turning speed
	double mip; //Maximum impulse velocity (<100)
	double aip; //Impulse acceleration
	double mwp; //Maximum warp velocity
	double awp; //Warp acceleration
	int mss; //Mass
	int hul; //Hull integrity
	int mhul; //Maximum hull integrity
	
	slot* pow; //Power plant
	slot* shd; //Shield generator
	slot* sens; //Sensor platform
	slot* clk; //Cloaking device
	slot* ful; //Fuel tank
	
	int esel; //Equipment selection
	slot slots[32]; //Equipment
	bool mlck; //Mass locked?
	bool crip; //Crippled?
};
