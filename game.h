/* game.h - functions for game work */

#include "common.h"


typedef struct
{
	double x;
	double y;
} GPOINT;

typedef struct
{
	GPOINT Center;
	int NumSides;
	GPOINT Points[4];
} POLYGON;

void initgame();
void destroygame();
bool handlegame();
void drawgame();
void drawgame_bld(int type, int x, int y);
void drawgame_object(struct gameobj*, int);
void drawgame_tile(int, int, unsigned char, int);
gameobj * recobjload(FILE *);

void networkgame();
void singlegame();
void recursivemove(gameobj *);
void controlgame(unsigned char, unsigned char);
gameobj *locate_object(gameobj *, unsigned int);
gameobj* makeajeep(int,int);
void drawhud(int, int, int, int);
struct human *locate_human(struct human *, unsigned int);

void locatecamera(int*, int*);
void load_sounds();
void free_sounds();
void play_sound(int, int x, int y);
void drawnumber(int,int,int);
void load_map(const char *);

void killhuman (unsigned int id, int blood);
void createbloodfx (int x, int y);
void createparticle (int x, int y, unsigned char angle, unsigned int speed, unsigned char life, unsigned char type);
void createexplosion (int x, int y, int size);
void createsplash (int x, int y);
void createsparks (int x, int y);
void texthqmenu(int x, int y, const char* message, SDL_Surface *hqmenu);

//double distance_seg_to_point(int, int, int, int, int, int);
void drawchat (const char *message,int);
gameobj* createobject (int type, int x, int y, unsigned char align, int id);
gameobj *createobjfromlib (gameobj *src, gameobj *, unsigned char align);

int retrieveangles(gameobj *go,int);

void vehcleanup();
void recdv (gameobj *veh);
void processborder();
void chatup (const char *message);





void simulategame();
void updateplayers();

human *locate_human(human *, unsigned int);
gameobj *locate_object(gameobj *, unsigned int);
void createbullet(unsigned char align,int x,int y,unsigned char angle, unsigned int speed, unsigned char type, unsigned char life);

void recobjmove(gameobj *);
void appendangles(gameobj *go);
void driverhuman(unsigned int id); // sorts drivers and humans
int seatsearch (human *humptr, gameobj *start);
int enterveh(human *humptr);
int checkcontrol (unsigned int number, char token);
void undriverhuman(unsigned int id); // re-sorts drivers and humans
void destroyvehicle(unsigned int);
void srecdv (gameobj *veh);
int playersearch(int id);

int bldhumcollide (int id, human *v2);
int bldvehcollide (int id, gameobj *v2);
int humhumcollide (human *, human *);
int humvehcollide (human *, gameobj *);
int vehvehcollide (gameobj *, gameobj *);
int bulhumcollide (human *v1, int oldx, int oldy, int newx, int newy);
int bulvehcollide (gameobj *v1, int oldx, int oldy, int newx, int newy, int type);
int polypolyCollision(POLYGON *, POLYGON *);

void aithink();
int leftturn(int a, int t);
