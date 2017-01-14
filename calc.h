/*
  calc.h

  (c) Richard Thrippleton
  Licensing terms are in the 'LICENSE' file
  If that file is not included with this source then permission is not given to use this source in any way whatsoever.
*/

#include <math.h>
#include <random>
#include <SDL_types.h>
#include <SDL_endian.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

class vect;
class pol //A polar vector
{
public:
  inline vect tovect(); //Returns conversion to a vect

  double ang; //Angle
  double rad; //Radius
};

class vect //A vector
{
public:
  inline pol topol(); //Returns conversion to a pol

  double xx;
  double yy; //X and Y components
};

inline vect pol::tovect()
{
  vect out; //Return value
  double cang; //Converted angle

  cang = ((ang - 90)*M_PI) / 180;
  out.xx = rad*cos(cang);
  out.yy = rad*sin(cang);
  return out;
}

inline pol vect::topol()
{
  pol out; //Return value

  out.rad = sqrt(xx*xx + yy*yy);
  if (xx != 0)
  {
    out.ang = atan(yy / xx)*(180 / M_PI);
    if (yy > 0)
    {
      if (xx > 0)
        out.ang = 90 + out.ang;
      else
        out.ang = 270 + out.ang;
    }
    else
    {
      if (xx > 0)
        out.ang = 90 + out.ang;
      else
        out.ang = 360 - (90 - out.ang);
    }
  }
  else
  {
    if (yy > 0)
      out.ang = 180;
    else
      out.ang = 0;
  }
  return out;
}

class ivect;
class ipol //Integer version of pol
{
public:
  inline ivect tovect(); //Returns conversion to an ivect

  int ang;
  long rad;
};

class ivect //Integer version of vect
{
public:
  inline ipol topol(); //Returns conversion to an ipol

  long xx;
  long yy;
};

inline ivect ipol::tovect()
{
  ivect out; //Return value
  double cang; //Converted angle

  cang = (((double)ang - 90)*M_PI) / 180;
  out.xx = (long)(rad*cos(cang));
  out.yy = (long)(rad*sin(cang));
  return out;
}

inline ipol ivect::topol()
{
  ipol out; //Return value

  out.rad = (long)sqrt(xx*xx + yy*yy);
  if (xx != 0)
  {
    out.ang = (int)((double)atan(yy / xx)*(180 / M_PI));
    if (yy > 0)
    {
      if (xx > 0)
        out.ang = 90 + out.ang;
      else
        out.ang = 270 + out.ang;
    }
    else
    {
      if (xx > 0)
        out.ang = 90 + out.ang;
      else
        out.ang = 360 - (90 - out.ang);
    }
  }
  else
  {
    if (yy > 0)
      out.ang = 180;
    else
      out.ang = 0;
  }
  return out;
}

struct cord //A co-ordinate in the game
{
  double x;
  double y; //X and Y components
};

struct icord //Integer version of cord
{
  long x;
  long y;
};


struct box //A universe bounding box
{
  long x1, y1; //Top-left corner
  long x2, y2; //Top-right corner
};

class calc //Mathematics module
{
public:
  static void init(); //Initialise some calculation data
  static long rnd(long max); //Return random integer from 0 to max-1
  static void getspeed(long spd, char* put); //Convert given game velocity to a string
  inline static long dattolong(unsigned char* in) //Converts a four byte buffer portably into a long
  {
    Uint32 tmp; //Temporary value holder
    unsigned char* tmpp = (unsigned char*)&tmp; //Accessor for tmp
    long out; //Value to output

    tmpp[0] = in[0];
    tmpp[1] = in[1];
    tmpp[2] = in[2];
    tmpp[3] = in[3];
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
    tmp = SDL_Swap32(tmp);
#endif	
    out = tmp - 2147483647;

    return out;
  }
  inline static void longtodat(long in, unsigned char* out) //Puts a long portably into a four byte buffer
  {
    Uint32 tmp; //Temporary value holder
    unsigned char* tmpp = (unsigned char*)&tmp; //Accessor for tmp

    tmp = in + 2147483647;
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
    tmp = SDL_Swap32(tmp);
#endif	
    out[0] = tmpp[0];
    out[1] = tmpp[1];
    out[2] = tmpp[2];
    out[3] = tmpp[3];
    return;
  }
  inline static int dattoint(unsigned char* in) //Converts a two byte buffer portably into a short
  {
    Uint16 tmp; //Temporary value holder
    unsigned char* tmpp = (unsigned char*)&tmp; //Accessor for tmp
    int out; //Value to output

    tmpp[0] = in[0];
    tmpp[1] = in[1];
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
    tmp = SDL_Swap16(tmp);
#endif	
    out = tmp - 32768;

    return out;
  }
  inline static void inttodat(short in, unsigned char* out) //Puts a long portably into a four byte buffer
  {
    Uint16 tmp; //Temporary value holder
    unsigned char* tmpp = (unsigned char*)&tmp; //Accessor for tmp

    tmp = in + 32768;
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
    tmp = SDL_Swap16(tmp);
#endif	
    out[0] = tmpp[0];
    out[1] = tmpp[1];
    return;
  }
  static bool dateq(unsigned char* d1, unsigned char* d2, int n); //Test two data streams for equality, up to n bytes
  static void obscure(char* str); //Munges the string so it is no longer human readable; the munging is consisten, like a very weak crypt

private:
  static long wrp[10]; //Warp speed table
  static char spds[33]; //Speed string (saves having to malloc, but it ain't threadsafe!)
  static std::mt19937 randomGenerator;
};
