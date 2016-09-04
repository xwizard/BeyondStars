/*
	sound.h
	
	(c) Richard Thrippleton
	Licensing terms are in the 'LICENSE' file
	If that file is not included with this source then permission is not given to use this source in any way whatsoever.
*/


class sound;

struct channel //Used to represent a virtual channel for mixing
{
	sound* snd; //Sound currently playing
	long pntr; //Playing pointer for each sound in queue
	int div; //Div factor for each sound
};

struct SDL_AudioSpec;

class sound //A sound in the database
{
	public:
	static const int ISIZE=256; //Maximum index size of sounds

	static void init(); //Initialise the sound system datastructures
	static void start(); //Start the sound system
	static void stop(); //Stop the sound system
	static sound* get(int indx); //Returns sound of given index from the database

	void play(int div); //Schedules sound to play with given div factor

	private:
	sound(int self); //Constructor, give self-index of sound

	static void callback(void* null,Uint8* bfil,int lfil); //Callback slave for the SDL sound thread

	void load(); //Loads sound in from appropriate file

	static channel chns[16]; //Virtual channels
	static SDL_AudioSpec spec; //Obtained audio device specification
	static bool on; //Sound on?
	static sound* sounds[ISIZE]; //Playable sound index

	int self; //Self-index value of sound
	Uint8* buff; //Audio buffer
	Uint32 blen; //Buffer length
	bool imem; //In memory and loaded?
	bool miss; //Sound missing? Keep this as a record and try not to load it again
};


