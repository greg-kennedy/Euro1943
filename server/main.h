/* Header file for Euro1943 server. */

/* Uncomment this to build a GUI (Windows users) */
#define DO_GUI 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <SDL/SDL.h>
#include <SDL/SDL_net.h>

#include "costable.h"
#include "sintable.h"
#include "oss.h"

#define PORT 5010
#define UPDATEFREQ 200
#define MAX_PLAYERS 32

#define MAP_X 100
#define MAP_Y 100

typedef struct
{
	double x;
	double y;
} POINT;

typedef struct
{
	POINT Center;
	int NumSides;
	POINT Points[4];
} POLYGON;

struct fx
{
	int x;
	int y;
	unsigned char type;
	struct fx *next;
};

struct projectile
{
	int x;
	int y;
	unsigned char angle;
	unsigned int speed;
	unsigned char align;
	unsigned char life;
	unsigned char type;
	struct projectile *next;
};

struct player
{
	unsigned char connected;
	unsigned int controller_k;
	unsigned int controller_m;
	unsigned int vehid;
	unsigned char status; // 0 = dead (waiting for spawn)
	// 1 = as a human
	// 2 = driving a vehicle
	int controls[10];
	unsigned char zoomlevel;
	IPaddress addr;
	unsigned long lastup;
};

struct gameobj
{
//	char name[40];
	unsigned int id;
	unsigned char type;
	int x;
	int y;
	int xoff;
	int yoff;
	float zoom;
	unsigned char angle;
	char anglediff;
	char speed;
	char topspeed;
	char accel;
	char maxang;
	int hp;
	unsigned char proj;
	unsigned char cool;
	unsigned char coolleft;
	
	int width;
	int height;
	
	char ktoken;
	char mtoken;
	
//	char proj2[20];
//	unsigned char cool2;
	unsigned int occupied;
	unsigned char align;

	unsigned char seat;
	unsigned char seatedimg;
	unsigned char waterborne;
	unsigned char plane;

	struct gameobj *subobj;
	gameobj *next;
	gameobj *parent;
};

struct human
{
	unsigned char align;
	unsigned char angle;
	unsigned char torsoangle;
	unsigned char armed;
	unsigned char ammo;
	int health;
	unsigned char weapon;
	unsigned char carriedarms;
	unsigned char cooldown;
	char anglediff;
	unsigned int id;
	int x;
	int y;
	char speed;
	struct human *next;
};

void draw ();
int main (int argc, char *argv[]);
int checkforquit();

int loadmap(const char *);
void mapchange (int mapnum);

void checkforpacket();
void simulategame();
void updateplayers();
void initgame();

human *locate_human(human *, unsigned int);
gameobj *locate_object(gameobj *, unsigned int);
void createbullet(unsigned char align,int x,int y,unsigned char angle, unsigned int speed, unsigned char type, unsigned char life);

void killhuman(unsigned int id); // sorts corpses and humans
void createfx (int x, int y, unsigned char type);

gameobj * recobjload(FILE *fp);
void recobjmove(gameobj *);
void loadobjects();
void globalchat(const char *message);
void teamchat(const char *message, int team);
void createobject (int type, int x, int y, unsigned char align);
gameobj *createobjfromlib (gameobj *src, gameobj *, unsigned char align);
void appendangles(gameobj *go);
void driverhuman(unsigned int id); // sorts drivers and humans
int seatsearch (human *humptr, gameobj *start);
int enterveh(human *humptr);
int checkcontrol (unsigned int number, char token);
void undriverhuman(unsigned int id); // re-sorts drivers and humans
void destroyvehicle(unsigned int);
void recdv (gameobj *veh);
int playersearch(unsigned int id);

int bldhumcollide (int id, human *v2);
int bldvehcollide (int id, gameobj *v2);
int humhumcollide (human *, human *);
int humvehcollide (human *, gameobj *);
int vehvehcollide (gameobj *, gameobj *);
int bulhumcollide (human *v1, int oldx, int oldy, int newx, int newy);
int bulvehcollide (gameobj *v1, int oldx, int oldy, int newx, int newy, int type);
int polypolyCollision(POLYGON *, POLYGON *);
