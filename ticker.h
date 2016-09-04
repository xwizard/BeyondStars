/*
	ticker.h
	
	(c) Richard Thrippleton
	Licensing terms are in the 'LICENSE' file
	If that file is not included with this source then permission is not given to use this source in any way whatsoever.
*/

class ticker //FPS regulator
{
	public:
	ticker(int fps); //Constructor, give desired fps value

	void start(); //Start the timer
	void tick(); //Mark next tick and block for appropriate time

	double afps; //Actual fps

	private:
	long stm,otm,ntm; //Start 500 tick segment, before, and after times
	long pas; //Precalculated delay required
	long tcks; //Ticks so far
};
