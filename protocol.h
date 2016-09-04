/*
	protocol.h
	
	(c) Richard Thrippleton
	Licensing terms are in the 'LICENSE' file
	If that file is not included with this source then permission is not given to use this source in any way whatsoever.
*/


enum {CLIENT_ACCEL=0,CLIENT_TURN=1,CLIENT_SHOOT=2,CLIENT_TRG=3,CLIENT_SHIELDS=4,CLIENT_CLOAK=5,CLIENT_CMOD=6,CLIENT_CONS=7,CLIENT_CHAR=8,CLIENT_BANDWIDTH=9}; //Multiplayer client->server action types
enum {SERV_SELF=0,SERV_CONS=1,SERV_READLN=2,SERV_CSPR=3,SERV_MESG=4,SERV_NEW=5,SERV_NAME=6,SERV_DEL=7,SERV_UPD=8,SERV_SND=9,SERV_NOISE=10,SERV_SHAKE=11,SERV_HILIGHT=12,SERV_FLOOD=13}; //Multiplayer server->client packet types 
enum {SERV_HILIGHT_SZ=3,SERV_FLOOD_SZ=256,SERV_SELF_SZ=25,SERV_READLN_SZ=2,SERV_CSPR_SZ=3,SERV_NEW_SZ=8,SERV_NAME_SZ=131,SERV_DEL_SZ=3,SERV_UPD_SZ=23,SERV_SND_SZ=3,SERV_NOISE_SZ=5,SERV_SHAKE_SZ=3,SERV_KILL_SZ=1}; //Multiplayer server->client object types 
enum {PT_PLANET=1,PT_SHIP=2,PT_FRAG=3}; //Presence types
enum {REQ_STAT=0,REQ_EQUIP=1,REQ_SCAN=2,REQ_HAIL=3,REQ_CHAT=4,REQ_WHOIS=5,REQ_HACK=6}; //Mode request numbers client->server
enum {CMOD_NULL,CMOD_NAME,CMOD_PASS,CMOD_CHOOSE,CMOD_STAT,CMOD_EQUIP,CMOD_SCAN,CMOD_HAIL,CMOD_REFIT,CMOD_CHAT,CMOD_CHATPRIVATE,CMOD_CHATTEAM,CMOD_CHATALL,CMOD_WHOIS,CMOD_HACK,CMOD_PASS1,CMOD_PASS2,CMOD_KICK,CMOD_DELETE}; //Communications modes, statekeeping

const char SIGN[]="SVST01"; //Signature of this version for networking
const int PORT=2300; //Default TCP port for the multiplayer protocol

#define planet2pres(indx) indx
#define ship2pres(indx) indx+planet::ISIZE
#define frag2pres(indx) indx+ship::ISIZE+planet::ISIZE //Convert object ids to client side presence ids

/*Packet definitions
SERV_READLN
1	Header
1	Password mode or not
2

SERV_HILIGHT
1	Header
2	Presence id
3

SERV_FLOOD
1	Header
255	Filler data
256

SERV_CONS
1	Header
2	Length (Maximum 1024)
-

SERV_MESG
1	Header
2	Length (Maximum 128)
-

SERV_CSPR
1	Header
2	Sprite index
3


SERV_SELF
1	Header
2	Self id
2	Hull
2	Power
2	Shields
2	Fuel
4	Sensor range
4	Long range
2	Target
2	Background sprite
2	Background density
25

SERV_NEW
1	Header
2	id
1	Type
2	Sprite
2	Link
8

SERV_NAME
1	Header
2	id
64	Name
64	Annotation
131

SERV_UPD
1	Header
2	id
4	X
4	Y
4	XX
4	YY
2	Angle
1	Hostile
1	Visibility
23

SERV_DEL
1	Header
2	id
3

SERV_NOISE
1	Header
2	sound index
2	originating presence
5

SERV_SOUND
1	Header
2	sound index
3

SERV_SHAKE
1	Header
2	magnitude
3
*/
