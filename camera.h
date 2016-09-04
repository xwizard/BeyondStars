/*
	camera.h
	
	(c) Richard Thrippleton
	Licensing terms are in the 'LICENSE' file
	If that file is not included with this source then permission is not given to use this source in any way whatsoever.
*/

struct box;
struct cord;
class presence;
class sound;

struct star //Background star
{
	cord loc; //Location
	int dep; //Depth
	int rot; //Rotation (for warp stars)
};

class camera //Viewer of the client's universe
{
	public:
	static void init(); //Initialise camera datastructures
	static void turnon(); //Turn camera on
	static void turnoff(); //Turn camera off
	static void bind(presence* src); //Bind the camera to given source
	static void unbind(); //Camera floats free, no longer bound
	static void render(); //Render what the camera can see
	static void update(); //Update camera position
	static void noise(sound* snd,presence* src); //Play noise from given source
	static void shake(int mag); //Shake the camera with given magnitude
	static void radarzoom(int dir); //Zoom the sensor in or out, with dir +1 or -1 respectively
	static void viewzoom(); //Toggle the camera through different zooms

	static box cov; //Total camera coverage

	private:
	static void rendermainview(); //Render the main screen view
	static void renderstars(); //Render the background stars
	static void renderradar(); //Render the radar view
	
	static bool on; //Is camera turned on?
	static presence* src; //Camera source
	static icord pov; //Camera point of view in universe

	static int vzm; //Zoom out factor on viewer
	static long rzm; //Sensor zoom out
	static star strs[64]; //Background stars
	static int shak; //Current shaking magnitude
	static bool flck; //Flicker for radar
};
