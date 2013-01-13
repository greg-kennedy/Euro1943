#include "texops.h"
/* game.c - contains the actual game code */

struct particle
{
	int x;
	int y;
	unsigned char angle;
	unsigned int speed;
	unsigned char life;
	unsigned char type;
	struct particle *next;
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

struct human
{
	unsigned char align;
	unsigned char legsimg;
	unsigned char angle;
	unsigned char torsoangle;
	unsigned char armed;
	unsigned char ammo;
	unsigned char weapon;
	char anglediff;
	unsigned int id;
	int x;
	int y;
	char speed;

	int health;
	int carriedarms;
	int cooldown;

	unsigned char touched;

	struct human *next;
};

struct gameobj
{
	unsigned int id;
	unsigned char img;
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
	int cool;
	unsigned int occupied;
	unsigned char align;

	int width;
	int height;

	unsigned char seat;
	unsigned char seatedimg;
	unsigned char waterborne;
	unsigned char plane;
	int coolleft;

	char ktoken;
	char mtoken;

	unsigned char touched;
	int type;

	unsigned char tag;

	gameobj *subobj;
	gameobj *next;
	gameobj *parent;
};

struct player
{
	unsigned int controller_k;
	unsigned int controller_m;
	unsigned int vehid;
	unsigned char status;
	int controls[9];
	unsigned char zoomlevel;
	int reg[8];
};

#include "game.h"
#include "sintable.h"
#include "costable.h"

// got some externs here
extern SDL_Surface *screen;
extern int sfxon, gamestate, multiplayer,level;
extern Mix_Music *music;
extern long mx, my;
extern GLuint cursor;

// loads of sound effects.
Mix_Chunk *sfx[NUM_SFX];

// these are defined for the multiplayer
extern char HOSTNAME[80];
extern UDPsocket sd;
extern IPaddress srvadd;
extern UDPpacket *p;

// k, we need the master object list
gameobj *topobj=NULL;
human *tophuman=NULL, *drivers = NULL;
gameobj *objlib=NULL;
projectile *proj=NULL;
particle *toppart=NULL;

unsigned long ticks, controlticks,trailticks;
float globalzoom;
int oldzoom, newzoom;
int camx, camy;
int objidnum;
float rate;
int myid=0, ammo=0, health=0, status=0;
int chatoff=0, chatlife=0;
int cash[2];
int hqmenusel=0;

// and here is for drawing the objects
GLuint objects[ NUM_OBJECTS ];
GLuint blds[ NUM_BLDS ];
GLuint humans[ NUM_HUMANS ];
GLuint tiles [ NUM_TILES+4 ];
GLuint bullets [ NUM_BULLETS ];
GLuint particles [ NUM_PARTICLES ];
GLuint hud [ 7 ];
GLuint digits[10];
GLuint hqmenu;
GLuint minimap;
//GLuint clouds;
int objw[NUM_OBJECTS];
int objh[NUM_OBJECTS];
int bulw[NUM_BULLETS];
int bulh[NUM_BULLETS];
int bldw[NUM_BLDS];
int bldh[NUM_BLDS];
int sbldw[NUM_BLDS];
int sbldh[NUM_BLDS];
int lastusedtex;  // I'm so clever.
extern SDL_Surface *font;
SDL_Surface *message, *bubble, *speakers[NUM_SPEAKERS];
char oldmessage[70];

// gotta know your FPS...
int frames;
unsigned long fpsticks;
int smooth[5]={UPDATEFREQ,UPDATEFREQ,UPDATEFREQ,UPDATEFREQ,UPDATEFREQ};

//single player uses lots of variables that multi won't
//int controller_k, controller_m;
int localcontrols[9]={0};
int unshoot[2]={0};
int objtags[10] = {0};
player players[32];
int bChatting;
char cMesg[50];
char gscript[20][60];
long scriptTime[20][2];
long curScripticks;
long curScriptloc;
unsigned char initang[2];
//bool showmoney = false;
bool iSeeAll = false;

int idnum=1;
int sobjidnum=1;

int cashmoney=0;

int hqock[2] = {0,0};
int hqobjects[] = {0,3,6,10,13,16,21,31,44,45};
int prices[] = {5,5,25,25,15,20,15,50,100,150,50,50,50,150,300,100,150};

//some stuff for drawing the map
#define MAP_X 100
#define MAP_Y 100
unsigned char map[MAP_X][MAP_Y];
unsigned char trees[MAP_X][MAP_Y];
unsigned char bldloc[15][3];

unsigned char mb[MAP_X][MAP_Y]={{0}};
unsigned char tb[MAP_X][MAP_Y]={{0}};

int min (int a, int b)
{
	if (a < b) return a; return b;
}
int max (int a, double b)
{
	if ((double)a > b) return a; return (int)b;
}
/*
void rectreeprint(gameobj *o, int depth)
{
	int i;

	while (o != NULL) {
		for (i=0; i<depth; i++) printf("  ");
		printf("%d: id %d, addr: %x, addr->sub %x, addr->next %x\n", depth,o->id, o, o->subobj, o->next);
		rectreeprint(o->subobj,depth+1);
		o = o->next;
	}
}
*/
gameobj * recobjload(FILE *fp)
{
	gameobj *temp=NULL, *temp2=NULL;
	char name[40], mouse[5], keys[5];
	int kids=0,i;

	temp = (gameobj*) malloc (sizeof(gameobj));

	fscanf(fp,"%d %s %d %d %d %d %d %d %d %d %d %d %d %s %s %d %d %d %d %f %d %d %d\n",
			&(temp->id),name,&(temp->img), &(temp->x), &(temp->y), &(temp->xoff), &(temp->yoff),
			&(temp->topspeed), &(temp->accel), &(temp->maxang), &(temp->hp),
			&(temp->proj),&(temp->cool),
			mouse, keys,&(temp->seat),
			&(temp->seatedimg),&(temp->waterborne),&(temp->plane),
			&(temp->zoom),&kids,&(temp->width),&(temp->height));

	temp->ktoken = keys[0];
	temp->mtoken = mouse[0];

	temp->subobj=NULL;
	temp->occupied = 0;
	temp->next=NULL;
	temp->parent=NULL;

		/*fprintf(stderr,"%d %s %d %d %d %d %d %d %d %d %d %d %d %c %c %d %d %d %d %f %d %d %d\n",
		temp->id,name,temp->img, temp->x, temp->y, temp->xoff, temp->yoff,
		temp->topspeed, temp->accel, temp->maxang, temp->hp,
		temp->proj,temp->cool,
		temp->mtoken, temp->ktoken,temp->seat,
		temp->seatedimg,temp->waterborne,temp->plane,
		temp->zoom,kids,temp->width,temp->height);*/

	for (i=0;i<kids;i++)
	{
		temp2=temp->subobj;
		temp->subobj=recobjload(fp);
		temp->subobj->next=temp2;
		temp->subobj->parent=temp;
	}

	temp2 = temp->subobj;
	i=0;
	while (temp2 != NULL) {i++; temp2=temp2->next;}
	if (i != kids) printf("warning\n");

	return temp;
}

void loadobjects()
{
	int i;
	FILE* fp;
	gameobj *to=NULL;

	fp=fopen("obj.txt","r");
	if (fp==NULL)
	{
		fprintf(stderr,"Couldn't open object library!\n");
		exit(1);
	}

	while(!feof(fp))
	{
		if (objlib!=NULL)
		{
			to=objlib;
			objlib=recobjload(fp);
			objlib->next=to;
		} else {
			objlib=recobjload(fp);
			objlib->next=NULL;
		}
	}
	fclose(fp);

	fp=fopen("bldg.txt","r");
	if (fp==NULL)
	{
		fprintf(stderr,"Couldn't open bldg-size library!\n");
		exit(1);
	}
	for (i=0; i<NUM_BLDS; i++)
	{
		fscanf(fp,"%d %d\n", &sbldw[i], &sbldh[i]);
	}
	fclose(fp);

	//rectreeprint(objlib,0);
}

void initgame()
{
	int i;
	char buffer[80];
	SDL_Surface *surf;
	FILE *fp;

	srand(time(NULL));

	objlib = NULL;
	proj = NULL;
	toppart = NULL;
	drivers = NULL;
	topobj = NULL;
	tophuman = NULL;

	glGenTextures( 7, hud );
	glGenTextures( NUM_OBJECTS, objects );
	glGenTextures( NUM_BLDS, blds );
	glGenTextures( NUM_TILES+4, tiles );
	glGenTextures( NUM_HUMANS, humans );
	glGenTextures( NUM_BULLETS, bullets );
	glGenTextures( NUM_PARTICLES, particles );
	glGenTextures( 10, digits );
	glGenTextures( 1, &hqmenu );
	glGenTextures( 1, &minimap );

	if (multiplayer) load_map("maps/x-isle.map");
	else { sprintf(buffer,"maps/sp/%d.map",level); load_map(buffer); }

	bubble=loadGLStyle("img/hud/bubble.png");

	for (i=0; i<NUM_SPEAKERS; i++)
	{
		sprintf(buffer,"img/hud/speakers/%d.png",i);
		surf=IMG_Load(buffer);
		speakers[i] = SDL_ConvertSurface(surf,bubble->format,bubble->flags);
		SDL_FreeSurface(surf);
	}

	surf=loadGLStyle("img/hud/hqmenu.png");
	texthqmenu(9,150,"  5 Health Kit",surf);
	texthqmenu(9,160,"  5 Ammo Kit",surf);
	texthqmenu(9,170," 25 R. Launch",surf);
	texthqmenu(9,180," 25 S. Rifle",surf);
	texthqmenu(9,190," 15 A. Rifle",surf);
	texthqmenu(9,200," 20 Explosives",surf);
	texthqmenu(9,210," 15 Backpack",surf);
	texthqmenu(9,240," 75 Jeep",surf);
	texthqmenu(9,250,"175 Light Tank",surf);
	texthqmenu(9,260,"250 Heavy Tank",surf);
	texthqmenu(9,270," 50 Turret",surf);
	texthqmenu(9,280," 50 AA Turret",surf);
	texthqmenu(9,300," 75 Lander",surf);
	texthqmenu(9,310,"200 Gunboat",surf);
	texthqmenu(9,320,"400 Battleship",surf);
	texthqmenu(9,340,"150 Fighter",surf);
	texthqmenu(9,350,"250 Bomber",surf);

	glBindTexture( GL_TEXTURE_2D, hqmenu );
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, 4, surf->w, surf->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, surf->pixels);
	SDL_FreeSurface(surf);

	glBindTexture( GL_TEXTURE_2D, minimap );
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glBindTexture( GL_TEXTURE_2D, hud[4] );
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glBindTexture( GL_TEXTURE_2D, hud[5] );
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	surf=loadGLStyle("img/ui/dir.png");
	glBindTexture( GL_TEXTURE_2D, hud[6] );
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, 4, surf->w, surf->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, surf->pixels);
	SDL_FreeSurface(surf);

	for (i=0; i<10; i++)
	{
		sprintf(buffer,"img/ui/n%d.png",i);
		digits[i] = load_texture(buffer,GL_NEAREST,GL_NEAREST);
	}

	for (i=0; i<4; i++)
	{
		sprintf(buffer,"img/hud/%d.png",i);
		hud[i] = load_texture(buffer,GL_NEAREST,GL_NEAREST);
	}

	for (i=0; i<NUM_PARTICLES; i++)
	{
		sprintf(buffer,"img/particle/%d.png",i);
		particles[i] = load_texture(buffer,GL_NEAREST,GL_NEAREST);
	}

	for (i=0; i<NUM_TILES+4; i++)
	{
		sprintf(buffer,"img/tiles/%d.png",i);
		tiles[i] = load_texture(buffer,GL_NEAREST,GL_NEAREST);
	}

	for (i=0; i<NUM_BULLETS; i++)
	{
		sprintf(buffer,"img/bul/%d.png",i);
		surf=loadGLStyle(buffer);
		bulw[i]=surf->w;
		bulh[i]=surf->h;
		glBindTexture( GL_TEXTURE_2D, bullets[i] );
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, 4, surf->w, surf->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, surf->pixels);
		SDL_FreeSurface(surf);
	}

	for (i=0; i<NUM_OBJECTS; i++)
	{
		sprintf(buffer,"img/gfx/%d.png",i);
		surf=loadGLStyle(buffer);
//		if (surf == NULL) fprintf(stderr,"Unable to load image img/gfx/%d.png\n",i);
		objw[i]=surf->w;
		objh[i]=surf->h;
		glBindTexture( GL_TEXTURE_2D, objects[i] );
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, 4, surf->w, surf->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, surf->pixels);
		SDL_FreeSurface(surf);
	}

	for (i=0; i<NUM_BLDS; i++)
	{
		sprintf(buffer,"img/bldg/%d.png",i);
		blds[i] = load_texture(buffer,GL_NEAREST,GL_NEAREST);
		surf=loadGLStyle(buffer);
//		if (surf == NULL) fprintf(stderr,"Unable to load image img/bldg/%d.png\n",i);
		bldw[i]=surf->w;
		bldh[i]=surf->h;
/*		glBindTexture( GL_TEXTURE_2D, blds[i] );
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, 4, surf->w, surf->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, surf->pixels); */
		SDL_FreeSurface(surf);
	}

	for (i=0; i<NUM_HUMANS; i++)
	{
		sprintf(buffer,"img/humgfx/%d.png",i);
		humans[i] = load_texture(buffer,GL_NEAREST,GL_NEAREST);
	}
	
	unsigned short int PORT=5010;

	// need to do some packet setup here
	if (multiplayer)
	{
	 for (i=0; i<strlen(HOSTNAME); i++)
	   if (HOSTNAME[i] == ':' || HOSTNAME[i]==' ') { PORT=atoi(&HOSTNAME[i+1]); HOSTNAME[i]='\0'; break; }
 
		if (!(sd = SDLNet_UDP_Open(0)))
		{
			fprintf(stderr, "SDLNet_UDP_Open: %s\n", SDLNet_GetError());
			gamestate=0;
		}
		if (SDLNet_ResolveHost(&srvadd, HOSTNAME, PORT))
		{
			fprintf(stderr, "SDLNet_ResolveHost(%s %d): %s\n", HOSTNAME, PORT, SDLNet_GetError());
			gamestate=0;
		}
		/* Allocate memory for the packet */
		if (!(p = SDLNet_AllocPacket(512)))
		{
			fprintf(stderr, "SDLNet_AllocPacket: %s\n", SDLNet_GetError());
			gamestate=0;
		}
		p->address.port=srvadd.port;
		p->address.host=srvadd.host;
		p->data[0]='L';
		p->len=1;

		SDLNet_UDP_Send(sd, -1, p);
	} else {
		cash[0]=2500;
		cash[1]=2500;
		for (i=0; i<32; i++)
			players[i].status = 0;
		for (i=0;i<10;i++)
			objtags[i]=0;
		sobjidnum = 1;
		objidnum = 0;
		idnum = 1;
	}

//	showmoney = false;
	iSeeAll = false;

	frames = 0;
	fpsticks = SDL_GetTicks();
	camx = 400;
	camy = 300;
	globalzoom=0;
	oldzoom = 0;
	newzoom = 0;
	lastusedtex=99;

	loadobjects();
	load_sounds();

	ticks=SDL_GetTicks();
	curScripticks = 0;
	curScriptloc = 0;
	controlticks = 0;

	if (!multiplayer)
	{
		i=0;
		sprintf(buffer,"maps/sp/%d.txt",level);
		fp = fopen(buffer,"r");
		while (!feof(fp))
		{
			fgets(gscript[i],59,fp);
			gscript[i][strlen(gscript[i])-1]='\0';
			strncpy(buffer,gscript[i],4);
			scriptTime[i][0] = atoi(buffer);
			strncpy(buffer,&gscript[i][4],2);
			scriptTime[i][1] = atoi(buffer);
			strncpy(buffer, &gscript[i][6],49);
			strcpy(gscript[i],buffer);
			i++;
		}
		fclose(fp);
	}
	
	for (i=0; i<9; i++)
	    localcontrols[i]=0;

	drawchat("",1);
}

void destroygame()
{
	int i;
	if (multiplayer)
	{
		p->data[0]='D';
		p->len=1;

		SDLNet_UDP_Send(sd, -1, p);
		SDLNet_FreePacket(p);
	}

	glDeleteTextures( 10, digits );
	glDeleteTextures( 7, hud );
	glDeleteTextures( NUM_OBJECTS, objects );
	glDeleteTextures( NUM_BLDS, blds );
	glDeleteTextures( NUM_TILES+4, tiles );
	glDeleteTextures( NUM_HUMANS, humans );
	glDeleteTextures( NUM_PARTICLES, particles );
	glDeleteTextures( 1, &hqmenu );
	glDeleteTextures( 1, &minimap );
	glLoadIdentity();

	free_sounds();

	human *thum;
	while (tophuman != NULL)
	{
		thum = tophuman->next;
		free(tophuman);
		tophuman=thum;
	}
	while (drivers != NULL)
	{
		thum = drivers->next;
		free(drivers);
		drivers=thum;
	}
	projectile *tpro;
	while (proj != NULL)
	{
		tpro = proj->next;
		free(proj);
		proj=tpro;
	}
	gameobj *tobj;
	while (topobj != NULL)
	{
		tobj = topobj->next;
		recdv(topobj);
		topobj = tobj;
	}
	while (objlib != NULL)
	{
		tobj = objlib->next;
		recdv(objlib);
		objlib = tobj;
	}
	particle *tpart;
	while (toppart != NULL)
	{
		tpart = toppart->next;
		free(toppart);
		toppart = tpart;
	}

	SDL_FreeSurface(bubble);
	for (i=0; i<NUM_SPEAKERS; i++)
		SDL_FreeSurface(speakers[i]);
}

bool handlegame()
{
	SDL_Event event;
	bool retval=true;
	int mxt;
	float newang;

	/* Check for events */
	while (SDL_PollEvent (&event))
	{
		switch (event.type)
		{
			case SDL_KEYDOWN:
				if (!bChatting) {
					if (event.key.keysym.sym == SDLK_LEFT || event.key.keysym.sym == 'a')
						controlgame(0,1);
					else if (event.key.keysym.sym == SDLK_RIGHT || event.key.keysym.sym == 'e' || event.key.keysym.sym == 'd')
						controlgame(1,1);
					else if (event.key.keysym.sym == SDLK_UP || event.key.keysym.sym == 'w' || event.key.keysym.sym == ',')
						controlgame(2,1);
					else if (event.key.keysym.sym == SDLK_DOWN || event.key.keysym.sym == 's' || event.key.keysym.sym == 'o')
						controlgame(3,1);
					else if (event.key.keysym.sym == SDLK_RETURN)
						controlgame(6,1);
				}
				break;
			case SDL_KEYUP:
				if (bChatting) {
					if (event.key.keysym.sym == SDLK_ESCAPE)
					{
						bChatting=0;
						memset(cMesg,0,50);
					}else if (event.key.keysym.sym == SDLK_RETURN)
					{
						if (multiplayer) {
							if (bChatting == 1)
								sprintf((char *)p->data,"C%s",cMesg);
							else
								sprintf((char *)p->data,"T%s",cMesg);
							p->len = strlen(cMesg) + 2;
							p->address.host = srvadd.host;	/* Set the destination host */
							p->address.port = srvadd.port;	/* And destination host */
							SDLNet_UDP_Send(sd, -1, p); /* This sets the p->channel */
						} else {
							//if (strcmp(cMesg,"show me the money") == 0) showmoney=!showmoney;
							//else
                            if (strcmp(cMesg,"raise the roof") == 0) iSeeAll=!iSeeAll;
							else if (strcmp(cMesg,"victory is mine") == 0) gamestate=4;
							else if (strcmp(cMesg,"game over man") == 0) gamestate=5;
						}

						bChatting=0;
						memset(cMesg,0,50);
					}else if (event.key.keysym.sym == SDLK_BACKSPACE && strlen(cMesg) > 0)
					cMesg[strlen(cMesg)-1]='\0';
					else if (strlen(cMesg) < 49 && (event.key.keysym.sym == '.' || (event.key.keysym.sym >= '0' && event.key.keysym.sym <= '9') || (event.key.keysym.sym >= 'a' && event.key.keysym.sym <= 'z') || (event.key.keysym.sym >= 'A' && event.key.keysym.sym <= 'Z') || event.key.keysym.sym == ' '))
						cMesg[strlen(cMesg)]=(char)(event.key.keysym.sym);
					chatup(cMesg);
				} else {
					if (event.key.keysym.sym == SDLK_ESCAPE)
						gamestate=0;
					else if (event.key.keysym.sym == SDLK_LEFT || event.key.keysym.sym == 'a')
						controlgame(0,0);
					else if (event.key.keysym.sym == SDLK_RIGHT || event.key.keysym.sym == 'e' || event.key.keysym.sym == 'd')
						controlgame(1,0);
					else if (event.key.keysym.sym == SDLK_UP || event.key.keysym.sym == 'w' || event.key.keysym.sym == ',')
						controlgame(2,0);
					else if (event.key.keysym.sym == SDLK_DOWN || event.key.keysym.sym == 's' || event.key.keysym.sym == 'o')
						controlgame(3,0);
					else if (event.key.keysym.sym == 'c' || event.key.keysym.sym == 'j')
					{
						bChatting = 1;
						chatup("");
					}
					else if (multiplayer && (event.key.keysym.sym == 't' || event.key.keysym.sym == 'y'))
					{
						bChatting = 2;
						chatup("");
					}
				}
				break;
			case SDL_MOUSEBUTTONDOWN:
				if (status == 3) {
					if (event.button.button == SDL_BUTTON_MIDDLE)
						controlgame(6,1);
					else if (event.button.button == SDL_BUTTON_WHEELUP)
					{
						hqmenusel--;
						if (hqmenusel < 0) hqmenusel = 16;
					}else if (event.button.button == SDL_BUTTON_WHEELDOWN)
					{
						hqmenusel++;
						if (hqmenusel > 16) hqmenusel = 0;
					} else {
						if (mx <= 600) {
							controlgame(0, hqmenusel);
							controlgame(5,(unsigned char)((int)(mx / 6)));
							controlgame(8,(unsigned char)((int)(my / 6)));
							controlgame(4,1);
						} else {
							hqmenusel = (int) ((float)(my - 175) / 11.71875);
							if (hqmenusel < 0) hqmenusel = 0;
							if (hqmenusel > 7) hqmenusel -= 2;
							if (hqmenusel > 11) hqmenusel--;
							if (hqmenusel > 14) hqmenusel--;
							if (hqmenusel > 16) hqmenusel = 16;
						}
					}
				} else {
					if (event.button.button == SDL_BUTTON_LEFT) {
						controlgame(4,1);
						unshoot[0]=1;
					} else if (event.button.button == SDL_BUTTON_RIGHT) {
						controlgame(5,1);
						unshoot[1]=1;
					} else if (event.button.button == SDL_BUTTON_MIDDLE)
						controlgame(6,1);
					else if (event.button.button == SDL_BUTTON_WHEELUP)
						controlgame(7,1);
					else if (event.button.button == SDL_BUTTON_WHEELDOWN)
						controlgame(7,1);
				}
				break;
			case SDL_MOUSEBUTTONUP:
				if (event.button.button == SDL_BUTTON_LEFT)
						unshoot[0]=0;
				else if (event.button.button == SDL_BUTTON_RIGHT)
						unshoot[1]=0;
				break;
			case SDL_MOUSEMOTION:
				mx=event.motion.x;
				my=event.motion.y;
				// figure out my new facing.
				if (status == 3)
				{
					mxt = mx;
					if (mxt > 600) mxt = 600;

					controlgame(5,(unsigned char)((int)(mxt / 6)));
					controlgame(8,(unsigned char)((int)(my / 6)));
				} else {
					if (my!=300) newang = 57.2957795*atan(((float)mx-400)/((float)-my+300));
					else if (mx>400) newang=90;
					else newang=270;
					if (my>300) newang=newang-180;

					controlgame(8,(unsigned char)((int)(newang*255/360)));
				}
				break;
			case SDL_QUIT:
				retval = false;
				break;
			default:
				break;
		}
	}

	return retval;
}

void drawgame_particle(particle* cur)
{
	int size=16;
	glPushMatrix();
	glTranslatef(cur->x-((float)cur->speed*sintable[cur->angle] * rate),
			cur->y-((float)cur->speed*-costable[cur->angle] * rate),0);
	glRotatef(((float)cur->angle * 1.40625), 0, 0, 1);

	if (cur->type==0) size=8;
	glBindTexture(GL_TEXTURE_2D, particles[cur->type]);
	if (cur->life == 1) glColor4f(1.0f,1.0f,1.0f,1+rate);
	glBegin(GL_QUADS);
	glTexCoord2f(0, 0); glVertex2i(-size/2, -size/2);
	glTexCoord2f(1, 0); glVertex2i(size/2, -size/2);
	glTexCoord2f(1, 1); glVertex2i(size/2, size/2);
	glTexCoord2f(0, 1); glVertex2i(-size/2, size/2);
	glEnd();
	glColor4f(1.0f,1.0f,1.0f,1.0f);
	glPopMatrix();
}

void drawgame_bld(int type, int x, int y)
{
	glPushMatrix();
	glTranslatef(x, y, 0);

	glBindTexture(GL_TEXTURE_2D, blds[type]);
	glBegin(GL_QUADS);
	glTexCoord2f(0, 0); glVertex2i(-bldw[type]/2, -bldh[type]/2);
	glTexCoord2f(1, 0); glVertex2i(bldw[type]/2, -bldh[type]/2);
	glTexCoord2f(1, 1); glVertex2i(bldw[type]/2, bldh[type]/2);
	glTexCoord2f(0, 1); glVertex2i(-bldw[type]/2, bldh[type]/2);
	glEnd();
	glPopMatrix();
}

void drawgame_projectile(projectile* cur)
{
	glPushMatrix();
	glTranslatef(cur->x-((float)cur->speed*sintable[cur->angle] * rate),
			cur->y-((float)cur->speed*-costable[cur->angle] * rate),0);
	glRotatef(((float)cur->angle * 1.40625), 0, 0, 1);

	glBindTexture(GL_TEXTURE_2D, bullets[cur->type]);
	glBegin(GL_QUADS);
	glTexCoord2f(0, 0); glVertex2i(-bulw[cur->type]/2, -bulh[cur->type]/2);
	glTexCoord2f(1, 0); glVertex2i(bulw[cur->type]/2, -bulh[cur->type]/2);
	glTexCoord2f(1, 1); glVertex2i(bulw[cur->type]/2, bulh[cur->type]/2);
	glTexCoord2f(0, 1); glVertex2i(-bulw[cur->type]/2, bulh[cur->type]/2);
	glEnd();
	glPopMatrix();
}

void draw_shadow(gameobj* cur)
{
	glPushMatrix();
	glTranslatef(cur->x-((float)cur->speed*sintable[cur->angle] * rate) - 5,
			cur->y-((float)cur->speed*-costable[cur->angle] * rate) + 5,0);
	glRotatef(((float)cur->angle * 1.40625) - ((float)cur->anglediff * rate * 1.40625), 0, 0, 1);
	glScalef(cur->zoom,cur->zoom,1);

	glBindTexture(GL_TEXTURE_2D, objects[cur->img-1]);
	glColor4f(0.0,0.0,0.0,0.5f);
	glBegin(GL_QUADS);
	glTexCoord2f(0, 0); glVertex2i(-objw[cur->img-1]/2, -objh[cur->img-1]/2);
	glTexCoord2f(1, 0); glVertex2i(objw[cur->img-1]/2, -objh[cur->img-1]/2);
	glTexCoord2f(1, 1); glVertex2i(objw[cur->img-1]/2, objh[cur->img-1]/2);
	glTexCoord2f(0, 1); glVertex2i(-objw[cur->img-1]/2, objh[cur->img-1]/2);
	glEnd();
	glColor4f(1.0f,1.0f,1.0f,1.0f);
	glPopMatrix();
}

void drawgame_human(human* cur)
{
	int wep = cur->weapon;
	glPushMatrix();

	glTranslatef(cur->x-((float)cur->speed*sintable[cur->angle] * rate),
			cur->y-((float)cur->speed*-costable[cur->angle] * rate),0);
	glRotatef((float)cur->angle * 1.40625, 0, 0, 1);

	if (map[cur->x/32][cur->y/32] != 0 || trees[cur->x/32][cur->y/32]!=0) {

		glBindTexture(GL_TEXTURE_2D, humans[(9 * cur->align) + cur->legsimg]);
		glBegin(GL_QUADS);
		glTexCoord2f(0, 0); glVertex2i(-16, -16);
		glTexCoord2f(1, 0); glVertex2i(16, -16);
		glTexCoord2f(1, 1); glVertex2i(16, 16);
		glTexCoord2f(0, 1); glVertex2i(-16, 16);
		glEnd();
	} else {
		wep = 2;
	}

	glPushMatrix();
	if (wep != 2) glRotatef((float)cur->torsoangle * 1.40625, 0, 0, 1);

	glBindTexture(GL_TEXTURE_2D, humans[(9 * cur->align) + wep]);
	glBegin(GL_QUADS);
	glTexCoord2f(0, 0); glVertex2i(-16, -16);
	glTexCoord2f(1, 0); glVertex2i(16, -16);
	glTexCoord2f(1, 1); glVertex2i(16, 16);
	glTexCoord2f(0, 1); glVertex2i(-16, 16);
	glEnd();

	glPopMatrix();
	glPopMatrix();
}

void drawgame_object(gameobj* cur, int align)
{
	gameobj *temp;

	if (cur->img != 0)
	{
		glPushMatrix();
		glTranslatef(cur->x-((float)cur->speed*sintable[cur->angle] * rate),
				cur->y-((float)cur->speed*-costable[cur->angle] * rate),0);
		glRotatef(((float)cur->angle * 1.40625) - ((float)cur->anglediff * rate * 1.40625), 0, 0, 1);
		glScalef(cur->zoom,cur->zoom,1);
		glTranslatef(cur->xoff,cur->yoff,0);

		glBindTexture(GL_TEXTURE_2D, objects[(15 * align) + cur->img-1]);
		glBegin(GL_QUADS);
		glTexCoord2f(0, 0); glVertex2i(-objw[cur->img-1]/2, -objh[cur->img-1]/2);
		glTexCoord2f(1, 0); glVertex2i(objw[cur->img-1]/2, -objh[cur->img-1]/2);
		glTexCoord2f(1, 1); glVertex2i(objw[cur->img-1]/2, objh[cur->img-1]/2);
		glTexCoord2f(0, 1); glVertex2i(-objw[cur->img-1]/2, objh[cur->img-1]/2);
		glEnd();

		temp = cur->subobj;
		while (temp!=NULL)
		{
			drawgame_object(temp, align);
			temp=temp->next;
		}
		glPopMatrix();
	} else if (cur->seatedimg == 1 && cur->occupied != 0) {
		glPushMatrix();
		glTranslatef(cur->x-((float)cur->speed*sintable[cur->angle] * rate),
				cur->y-((float)cur->speed*-costable[cur->angle] * rate),0);
		glRotatef(((float)cur->angle * 1.40625) - ((float)cur->anglediff * rate * 1.40625), 0, 0, 1);
		glScalef(cur->zoom,cur->zoom,1);
		glTranslatef(cur->xoff,cur->yoff,0);

		glBindTexture(GL_TEXTURE_2D, humans[2 + (9 *align)]);
		glBegin(GL_QUADS);
		glTexCoord2f(0, 0); glVertex2i(-16, -16);
		glTexCoord2f(1, 0); glVertex2i(16, -16);
		glTexCoord2f(1, 1); glVertex2i(16, 16);
		glTexCoord2f(0, 1); glVertex2i(-16, 16);
		glEnd();

		glPopMatrix();
	}
}

void drawgame()
{
	int i;
	int j, drawTrail=0;
	int startx, starty, endx, endy;

	gameobj *temp;
	human *humptr;
	projectile *projptr;
	particle *partptr;

	//	rate=((float)ticks-SDL_GetTicks())/ UPDATEFREQ;
	rate=((float)ticks-SDL_GetTicks())/ ((smooth[0]+smooth[1]+smooth[2]+smooth[3]+smooth[4]) / 5);

	globalzoom = (float)(oldzoom - newzoom) * rate + (float)oldzoom;

	glClear(GL_COLOR_BUFFER_BIT);// | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	if (status == 3 || iSeeAll)
		glOrtho(0,4267,3200,0, -1.0, 1.0);
	else
		glOrtho((int)(-globalzoom*4), (int)(SCREEN_X + (globalzoom*4)), (int)(SCREEN_Y +(globalzoom*3)), (int)(-globalzoom*3), -1.0, 1.0);
	glMatrixMode(GL_MODELVIEW);
	glClear(GL_COLOR_BUFFER_BIT);// | GL_DEPTH_BUFFER_BIT);

	glLoadIdentity();

	if (status != 3 && !iSeeAll) {
		locatecamera(&camx,&camy);

		glTranslatef(400-camx, 300-camy,0);

		startx = (int)(((camx - 400) - globalzoom * 4) / 32);
		if (startx < 0) startx=0;
		endx = (int)(1+((camx + 400) + globalzoom * 4) / 32);
		if (endx > MAP_X ) endx=MAP_X;
		starty = (int)(((camy - 300) - globalzoom * 3) / 32);
		if (starty < 0) starty=0;
		endy = (int)(1+((camy + 300) + globalzoom * 3) / 32);
		if (endy > MAP_Y ) endy=MAP_Y;
	} else {
		startx = 0;
		endx = MAP_X;
		starty = 0;
		endy = MAP_Y;
	}
	glColor3f(0.0f,0.0f,0.4f);
	glBegin(GL_QUADS);
	glVertex2i(0,0);
	glVertex2i(0,3200);
	glVertex2i(3200,3200);
	glVertex2i(3200,0);
	glEnd();
	glColor3f(1.0f,1.0f,1.0f);

	glEnable(GL_TEXTURE_2D);

	lastusedtex=99; // hack to prevent unneeded texture binding
	for (i=startx;i<endx; i++)
		for (j=starty;j<endy; j++)
		{
			if (map[i][j]) drawgame_tile(32*i,32*j,map[i][j]-1,0);
			if (mb[i][j] & 192) drawgame_tile(32*i,32*j,15 + ((mb[i][j] & 192) >> 6),2);
			if (mb[i][j] & 48) drawgame_tile(32*i,32*j,15 + ((mb[i][j] & 48) >> 4),0);
			if (mb[i][j] & 12) drawgame_tile(32*i,32*j,15 + ((mb[i][j] & 12) >> 2),1);
			if (mb[i][j] & 3) drawgame_tile(32*i,32*j,15 + (mb[i][j] & 3),3);
		}

	glEnd();

	for (i=0; i<15; i++)
	{
		if ((i < 4 || bldloc[i][0] != 0))  // todo: limit to on-screen only
		{
			drawgame_bld(bldloc[i][0], bldloc[i][1]*32, bldloc[i][2]*32);
		}
	}

	temp=topobj;
	while (temp!=NULL)
	{
		if (temp->plane == 0 || temp->speed <= 30)
		{
			draw_shadow(temp);
			drawgame_object(temp, temp->align);
       }
		temp=temp->next;
	}

	humptr=tophuman;
	while (humptr!=NULL)
	{
		drawgame_human(humptr);
		humptr=humptr->next;
	}

	if (trailticks + 15 < SDL_GetTicks()) {drawTrail = 1;
		trailticks = SDL_GetTicks();
	}
	projptr=proj;
	while (projptr!=NULL)
	{
		drawgame_projectile(projptr);
		if (drawTrail && (projptr->type == 1 || projptr->type == 13 || projptr->type == 15))
			createparticle((int)(projptr->x -((float)projptr->speed*sintable[projptr->angle] * rate)),(int)(projptr->y-((float)projptr->speed*-costable[projptr->angle] * rate)),(projptr->angle + 118 + (rand() % 20))%256,rand()%20+1,rand()%3+1,rand()%4+1);
		projptr=projptr->next;
	}

	partptr=toppart;
	while (partptr!=NULL)
	{
		drawgame_particle(partptr);
		partptr=partptr->next;
	} 

//	glEnable(GL_TEXTURE_2D);
            glColor4f(0.0,0.0,0.0,0.5);

	for (i=startx;i<max(endx+1,MAP_X); i++)
		for (j=min(0,starty-1);j<endy; j++)
		{
			if (trees[i][j]) drawgame_tile(32*i-10,32*j+10,trees[i][j],0); else {

				if (tb[i][j] & 8) drawgame_tile(32*i-10,32*j+10,19,2);
				if (tb[i][j] & 4) drawgame_tile(32*i-10,32*j+10,19,0);
				if (tb[i][j] & 2) drawgame_tile(32*i-10,32*j+10,19,1);
				if (tb[i][j] & 1) drawgame_tile(32*i-10,32*j+10,19,3);
            }
		}
            glColor4f(1.0,1.0,1.0,1.0);

	for (i=startx;i<endx; i++)
		for (j=starty;j<endy; j++)
		{
			if (trees[i][j]) drawgame_tile(32*i,32*j,trees[i][j],0); else {
				if (tb[i][j] & 8) drawgame_tile(32*i,32*j,19,2);
				if (tb[i][j] & 4) drawgame_tile(32*i,32*j,19,0);
				if (tb[i][j] & 2) drawgame_tile(32*i,32*j,19,1);
				if (tb[i][j] & 1) drawgame_tile(32*i,32*j,19,3);
			}
		}

	glEnd();

	temp=topobj;
	while (temp!=NULL)
	{
		if (temp->plane == 1 && temp->speed > 30)
		{
            temp->x-=(temp->speed-30);
            temp->y+=(temp->speed-30);
			draw_shadow(temp);
            temp->x+=(temp->speed-30);
            temp->y-=(temp->speed-30);
			drawgame_object(temp, temp->align);
		}
		temp=temp->next;
	}
	
/*	if (globalzoom > 30)
	{
		glBindTexture(GL_TEXTURE_2D, clouds);
        glColor4f(1.0,1.0,1.0,0.2);
		glBegin(GL_QUADS);
		for (i = 0; i < 6; i++)
		{
		for (j=0; j<6; j++)
		{
		glTexCoord2f(0, 0); glVertex2i(0+(512*i),0+(512*j));
		glTexCoord2f(1, 0); glVertex2i(0+(512*i),512+(512*j));
		glTexCoord2f(1, 1); glVertex2i(512+(512*i),512+(512*j));
		glTexCoord2f(0, 1); glVertex2i(512+(512*i),0+(512*j));
    }
}
		glEnd();
        glColor4f(1.0,1.0,1.0,1.0);
    }*/
    
    if (status == 1)
    {
               humptr = locate_human(tophuman,myid);
               if (humptr != NULL) {
	           glPushMatrix();
	           glTranslatef(humptr->x-((float)humptr->speed*sintable[humptr->angle] * rate),
			humptr->y-((float)humptr->speed*-costable[humptr->angle] * rate),0);
	glRotatef((float)humptr->angle * 1.40625, 0, 0, 1);

	glPushMatrix();
	    glTranslatef(0,-50,0);
		glBindTexture(GL_TEXTURE_2D, hud[6]);
		glBegin(GL_QUADS);
		glTexCoord2f(0,0);
		glVertex2i(0, 0);
		glTexCoord2f(1,0);
		glVertex2i(8, 0);
		glTexCoord2f(1,1);
		glVertex2i(8, 8);
		glTexCoord2f(0,1);
		glVertex2i(0, 8);
		glEnd();
	glPopMatrix();
	glPopMatrix();
    }
    }

	i = 1; j=1;
	if (bldloc[0][0] == 8) i++; else if (bldloc[0][0] == 9) j++;
	if (bldloc[1][0] == 8) i++; else if (bldloc[1][0] == 9) j++;
	if (bldloc[2][0] == 8) i++; else if (bldloc[2][0] == 9) j++;
	drawhud(i,j,health,ammo);

	//Disable texturing
	glDisable(GL_TEXTURE_2D);

	//Flip the backbuffer to the primary
	SDL_GL_SwapBuffers();
	/* Don't run too fast */
	SDL_Delay (1);

	// FPS counter : )
	frames++;    
	if (fpsticks + 1000 < SDL_GetTicks()) {
		fpsticks = SDL_GetTicks();
		//		printf("FPS: %d\n",frames);
		frames=0;
	}
}

void controlgame(unsigned char device, unsigned char action)
{
     localcontrols[device] = action;
}

void networkgame()
{
	int i,poff;
	int id;
	int numprojectiles;
	int oldhealth,oldx,oldy;
	human *humptr;
	gameobj *objptr;
	projectile *newproj, *pp2;
	particle *partptr, *partptr2;
	char buffer[50];

	if (controlticks + CONTROLFREQ < SDL_GetTicks()) {
		if (status != 3) {
			p->data[0] = 0;
			for (i=0; i<8; i++)
				p->data[0] = p->data[0] | ((localcontrols[i] & 1) << i);
			p->data[1] = localcontrols[8];
			p->len = 2;
		} else {
			p->data[0] = (localcontrols[6] << 7) | (localcontrols[4] << 6) | (localcontrols[0] & 63);
			p->data[1] = localcontrols[5];
			p->data[2] = localcontrols[8];
			p->len = 3;
		}
		p->address.host = srvadd.host;	/* Set the destination host */
		p->address.port = srvadd.port;	/* And destination host */
		SDLNet_UDP_Send(sd, -1, p); /* This sets the p->channel */

	}

	if (SDLNet_UDP_Recv(sd,p))
	{
		localcontrols[6] = 0;
		localcontrols[7] = 0;
		localcontrols[4] = unshoot[0];
		localcontrols[5] = unshoot[1];

		if (p->len >0 && p->data[0] == 'C')
		{
			drawchat((char *)&p->data[1],0);
		} else if (p->len >0 && p->data[0] == 'M') // map change!
		{
               snprintf(buffer,49,"maps/%s",(char *)&p->data[1]);
			load_map(buffer);
		} else if (p->len > 0) {
			if (chatlife > 0) chatlife--;
			else if (chatoff < 0) chatoff+=3;
			else {chatoff=0; chatlife = 0;}

			humptr = tophuman;
			while (humptr != NULL)
			{
				if (map[humptr->x/32][humptr->y/32] == 0 && trees[humptr->x/32][humptr->y/32] == 0)
				{
					createparticle(humptr->x,humptr->y,(humptr->angle + 118 + (rand() % 20))%256,rand()%20+1,rand()%3+3,rand()%4+4);
					createparticle(humptr->x,humptr->y,(humptr->angle + 118 + (rand() % 20))%256,rand()%20+1,rand()%3+3,rand()%4+4);
					createparticle(humptr->x,humptr->y,(humptr->angle + 118 + (rand() % 20))%256,rand()%20+1,rand()%3+3,rand()%4+4);
				}
				humptr->touched = 0;
				humptr=humptr->next;
			}

			objptr = topobj;
			while (objptr != NULL)
			{
				if (objptr->waterborne == 1)
				{
					createparticle((int)(objptr->x - (objptr->height/2 * sintable[(int)(objptr->angle + 252 + (rand() % 6)) % 256])),(int)(objptr->y - (objptr->height/2 * -costable[(int)(objptr->angle + 252 + (rand() % 6)) % 256])),(objptr->angle + 118 + (rand() % 20))%256,rand()%20+1,rand()%3+3,rand()%4+4);
					createparticle((int)(objptr->x - (objptr->height/2 * sintable[(int)(objptr->angle + 252 + (rand() % 6)) % 256])),(int)(objptr->y - (objptr->height/2 * -costable[(int)(objptr->angle + 252 + (rand() % 6)) % 256])),(objptr->angle + 118 + (rand() % 20))%256,rand()%20+1,rand()%3+3,rand()%4+4);
					createparticle((int)(objptr->x - (objptr->height/2 * sintable[(int)(objptr->angle + 252 + (rand() % 6)) % 256])),(int)(objptr->y - (objptr->height/2 * -costable[(int)(objptr->angle + 252 + (rand() % 6)) % 256])),(objptr->angle + 118 + (rand() % 20))%256,rand()%20+1,rand()%3+3,rand()%4+4);
				}
				objptr->touched = 0;
				objptr=objptr->next;
			}

			partptr = toppart;
			partptr2 = toppart;
			while (partptr!=NULL)
			{
				partptr->x = partptr->x-(int)((double)partptr->speed*sintable[partptr->angle] * rate);
				partptr->y = partptr->y-(int)((double)partptr->speed*-costable[partptr->angle] * rate);
				partptr->life--;
				if (partptr->life==0)
				{
					if (partptr2==partptr) {
						toppart=partptr->next;
						free(partptr);
						partptr2=toppart;
						partptr=toppart;
					} else {
						partptr2->next=partptr->next;
						free(partptr);
						partptr=partptr2->next;
					}
				} else {
					partptr2=partptr;
					partptr=partptr2->next;
				}
			} 

			newproj=proj;
			pp2=proj;
			while (newproj!=NULL)
			{
				oldx=newproj->x;
				oldy=newproj->y;
				newproj->x = newproj->x-(int)((double)newproj->speed*sintable[newproj->angle] * rate);
				newproj->y = newproj->y-(int)((double)newproj->speed*-costable[newproj->angle] * rate);

				humptr=tophuman;
				while (humptr!=NULL)
				{
					if (newproj->type < 6) {
						if ((newproj->align != humptr->align) && bulhumcollide(humptr,oldx,oldy,newproj->x,newproj->y))
						{
							createbloodfx(humptr->x,humptr->y);
							newproj->life=1;
						}
					}else if (newproj->type < 8) {
						if (humptr->weapon != 8 && (newproj->align == humptr->align) && bulhumcollide(humptr,oldx,oldy,newproj->x,newproj->y))
						{
							play_sound(12,humptr->x,humptr->y);
							newproj->life=1;
						}
					} else {
						if (bulhumcollide(humptr,oldx,oldy,newproj->x,newproj->y))
						{
							play_sound(12,humptr->x,humptr->y);
							newproj->life=1;
						}
					}

					humptr=humptr->next;
				}

				objptr=topobj;
				while (objptr!=NULL)
				{
					if (newproj->type < 6 || newproj->type > 12) {
						if ((newproj->align != objptr->align) && bulvehcollide(objptr,oldx,oldy,newproj->x,newproj->y, newproj->type))
						{
							if (newproj->type == 0 || newproj->type == 2 || newproj->type == 3 || newproj->type == 14)
							{
								createsparks(newproj->x, newproj->y);
								if (rand() % 2) play_sound(29+rand()%2,newproj->x,newproj->y);
								newproj->life=1;
							} else { // just blow up.
								newproj->life=1;
								createexplosion(newproj->x, newproj->y,1);
							}
						}
					}
					objptr=objptr->next;
				}


				newproj->life--;
				if (newproj->life==0)
				{
					if (pp2==newproj) {
						proj=newproj->next;
						free(newproj);
						pp2=proj;
						newproj=pp2;
					} else {
						pp2->next=newproj->next;
						free(newproj);
						newproj=pp2->next;
					}
				} else {
					pp2=newproj;
					newproj=pp2->next;
				}
			}

			/* extract data from packets! */
			status = (unsigned int)p->data[0];
			if (status == 3)
			{
				localcontrols[5]=0;
				localcontrols[4]=0;
				localcontrols[0]=0;
			} else {
				myid = (unsigned int)p->data[1] * 256 + (unsigned int)p->data[2];
				oldzoom = newzoom;
				newzoom = (int)p->data[3];
				oldhealth=health;
				health = (unsigned int)p->data[4];
				if (health != oldhealth)
				{
					if (status == 1) play_sound(rand()%3+14,camx,camy);
					// TODO: ricochet / ping sound for getting shot in veh.
				}
				ammo = (unsigned int)p->data[5];
			}

			bldloc[0][0] = (unsigned char) p->data[6];
			bldloc[1][0] = (unsigned char) p->data[7];
			bldloc[2][0] = (unsigned char) p->data[8];

				cash[0] = p->data[9] * 256 + p->data[10];
				cash[1] = p->data[11] * 256 + p->data[12];

			poff = 13;
			numprojectiles = p->data[poff];
			poff++;

			for (i=0; i<numprojectiles; i++)
			{
				//printf("%d\n",p->data[0]);
				id = (int)p->data[poff]*256 + (int)p->data[poff+1];
				humptr = locate_human(tophuman, id);
				if (humptr == NULL)
					/* must be a new guy... */
				{
					humptr = (human*)malloc(sizeof(human));
					humptr->next=tophuman;
					tophuman=humptr;
					humptr->id=id;
					humptr->legsimg=0;
				}
				humptr->touched = 1;
				humptr->x = ((int) p->data[poff+2] * 256 + (int)p->data[poff+3]);
				humptr->y = ((int) p->data[poff+4] * 256 + (int)p->data[poff+5]);
				humptr->align = p->data[poff+6] >> 7 & 1;
				if ((p->data[poff+6] >> 5) & 1)
					humptr->speed=15;
				else if ((p->data[poff+6] >> 4) & 1)
					humptr->speed=-15;
				else
					humptr->speed=0;
				if (humptr->speed != 0)
				{
					if (humptr->legsimg==0) humptr->legsimg=1;
					else if (map[humptr->x/32][humptr->y/32] != 0 || trees[humptr->x/32][humptr->y/32] != 0) {humptr->legsimg = 0;
						play_sound(rand() % 5,humptr->x,humptr->y);}
				}
				humptr->weapon = p->data[poff+6] & 15;
				humptr->torsoangle = p->data[poff+7];
				humptr->angle = p->data[poff+8];

				poff +=9;
			}

			numprojectiles = p->data[poff];
			poff++;
			for (i=0; i<numprojectiles; i++)
			{
				newproj = (projectile *)malloc(sizeof(projectile));
				newproj->x= (int)p->data[poff]*256 + (int)p->data[poff+1];
				newproj->y= (int)p->data[poff+2]*256 + (int)p->data[poff+3];
				newproj->angle = p->data[poff+4];
				newproj->speed = ((unsigned int)p->data[poff+5])*10;
				newproj->life = p->data[poff+6];
				newproj->align = (p->data[poff+7] & 0x80) >> 7;
				newproj->type = (p->data[poff+7] & 0x7F);
				if (newproj->type < 5) play_sound(5+newproj->type,newproj->x,newproj->y);
				else if (newproj->type < 8) play_sound(10,newproj->x,newproj->y);
				else if (newproj->type > 12) play_sound(7+newproj->type,newproj->x,newproj->y);
				else play_sound(11,newproj->x,newproj->y);
				newproj->next=proj;
				proj=newproj;
				poff+=8;
			}

			numprojectiles = p->data[poff];
			poff++;
			for (i=0; i<numprojectiles; i++)
			{
				killhuman((int)p->data[poff]*256 + (int)p->data[poff+1],1);
				play_sound(rand()%2+17,camx,camy);
				poff+=2;
			}
			killhuman(0,0);

			numprojectiles = p->data[poff];
			poff++;
			for (i=0; i<numprojectiles; i++)
			{
				if ((int)p->data[poff+4] == 255)
				{
					createsplash((int)p->data[poff]*256 + (int)p->data[poff+1],
							(int)p->data[poff+2]*256 + (int)p->data[poff+3]);
				} else if ((int)p->data[poff+4] == 254)
				{
					createsparks((int)p->data[poff]*256 + (int)p->data[poff+1],
							(int)p->data[poff+2]*256 + (int)p->data[poff+3]);
					play_sound(13,(int)p->data[poff]*256 + (int)p->data[poff+1],(int)p->data[poff+2]*256 + (int)p->data[poff+3]);
				} else {
					createexplosion((int)p->data[poff]*256 + (int)p->data[poff+1],
							(int)p->data[poff+2]*256 + (int)p->data[poff+3],
							(int)p->data[poff+4]);
				}
				poff+=5;
			}

			numprojectiles = p->data[poff];
			poff++;
			//		printf("%d objects.\n",numprojectiles);
			for (i=0; i<numprojectiles; i++)
			{
				objptr = locate_object(topobj,(int)p->data[poff]*256+(int)p->data[poff+1]);
				if (objptr == NULL)
				{
					objptr = createobject((int)(p->data[poff+7] & 0x3F),
							(int)p->data[poff+2]*256 + (int)p->data[poff+3],
							(int)p->data[poff+4]*256 + (int)p->data[poff+5],
							0,
							(int)p->data[poff]*256+(int)p->data[poff+1]);

				}
				objptr->x = (int)p->data[poff+2]*256 + (int)p->data[poff+3];
				objptr->y = (int)p->data[poff+4]*256 + (int)p->data[poff+5];
				objptr->speed = (int)p->data[poff+6];
				objptr->touched=1;
				poff+=8;
				poff = retrieveangles(objptr, poff);
			}

			vehcleanup();

			smooth[4] = smooth[3];
			smooth[3] = smooth[2];
			smooth[2] = smooth[1];
			smooth[1] = smooth[0];
			smooth[0] = SDL_GetTicks() - ticks;
			ticks=SDL_GetTicks() ;
		}
	}
}

void drawgame_tile(int x, int y, unsigned char obj_id, int dir)
{
	if (lastusedtex==99)
	{
		glBindTexture(GL_TEXTURE_2D, tiles[obj_id]); 
		lastusedtex=obj_id;
		glBegin(GL_QUADS);
	}
	else if (lastusedtex!=obj_id)
	{
		glEnd();
		glBindTexture(GL_TEXTURE_2D, tiles[obj_id]); 
		lastusedtex=obj_id;
		glBegin(GL_QUADS);
	}

	switch (dir)
	{
		case 0:
			glTexCoord2f(0, 0); glVertex2i(x, y);
			glTexCoord2f(1, 0); glVertex2i(x+32, y);
			glTexCoord2f(1, 1); glVertex2i(x+32, y+32);
			glTexCoord2f(0, 1); glVertex2i(x, y+32);
			break;
		case 1:
			glTexCoord2f(1, 0); glVertex2i(x, y);
			glTexCoord2f(1, 1); glVertex2i(x+32, y);
			glTexCoord2f(0, 1); glVertex2i(x+32, y+32);
			glTexCoord2f(0, 0); glVertex2i(x, y+32);
			break;
		case 2:
			glTexCoord2f(1, 1); glVertex2i(x, y);
			glTexCoord2f(0, 1); glVertex2i(x+32, y);
			glTexCoord2f(0, 0); glVertex2i(x+32, y+32);
			glTexCoord2f(1, 0); glVertex2i(x, y+32);
			break;
		default:
			glTexCoord2f(0, 1); glVertex2i(x, y);
			glTexCoord2f(0, 0); glVertex2i(x+32, y);
			glTexCoord2f(1, 0); glVertex2i(x+32, y+32);
			glTexCoord2f(1, 1); glVertex2i(x, y+32);
	}
}

void locatecamera(int *camx, int *camy)
{
	human *cur;
	gameobj *obj;
	float x, y,tx, ty,tangle,dist;

	if (status==1){
		cur = locate_human(tophuman,myid);

		if (cur == NULL) { *camx=400; *camy=300;} else {
			*camx = cur->x-(int)((float)cur->speed*sintable[cur->angle] * rate);
			*camy = cur->y-(int)((float)cur->speed*-costable[cur->angle] * rate);
		}
	}else if (status==2) {
		*camx=400; *camy=300;
		obj = locate_object(topobj,myid);
		if (obj==NULL) {*camx=400; *camy=300;} else {
			x=0; y=0;
			while (obj != NULL)
			{
				tangle = atan2(y,x) + (((double)obj->angle - (double)obj->anglediff * rate) * 0.02463994235294);
				dist = sqrt(x*x+y*y);

				tx = dist * cos(tangle);
				ty = dist * sin(tangle);
				if (obj->parent != NULL)
				{
					tx = tx + (float)obj->parent->xoff;
					ty = ty + (float)obj->parent->yoff;
				}
				x = (obj->zoom) * tx + (float)obj->x - ((float)obj->speed*sintable[obj->angle] * rate);
				y = (obj->zoom) * ty + (float)obj->y - ((float)obj->speed*-costable[obj->angle] * rate);
				obj = obj->parent;
			}
			*camx = (int)x;
			*camy = (int)y;
		}
	} else {
		*camx=400; *camy=300;
	}
}

void drawhud(int abase, int nbase, int health, int ammo)
{
	int  co;
	//gotta go back to the old way to draw the cursor right size, and UI.
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, SCREEN_X, SCREEN_Y, 0, -1.0, 1.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glBindTexture(GL_TEXTURE_2D, hud[0]); 
	glBegin(GL_QUADS);
	glTexCoord2f(0,0);
	glVertex2i(0, 0);
	glTexCoord2f(1,0);
	glVertex2i(32, 0);
	glTexCoord2f(1,1);
	glVertex2i(32, 32);
	glTexCoord2f(0,1);
	glVertex2i(0, 32);
	glEnd();
	glBindTexture(GL_TEXTURE_2D, hud[1]); 
	glBegin(GL_QUADS);
	glTexCoord2f(0,0);
	glVertex2i(80, 0);
	glTexCoord2f(1,0);
	glVertex2i(112, 0);
	glTexCoord2f(1,1);
	glVertex2i(112, 32);
	glTexCoord2f(0,1);
	glVertex2i(80, 32);
	glEnd();
	glBindTexture(GL_TEXTURE_2D, hud[2]); 
	glBegin(GL_QUADS);
	glTexCoord2f(0,0);
	glVertex2i(0, 40);
	glTexCoord2f(1,0);
	glVertex2i(32, 40);
	glTexCoord2f(1,1);
	glVertex2i(32, 72);
	glTexCoord2f(0,1);
	glVertex2i(0, 72);
	glEnd();
	glBindTexture(GL_TEXTURE_2D, hud[3]); 
	glBegin(GL_QUADS);
	glTexCoord2f(0,0);
	glVertex2i(0, 80);
	glTexCoord2f(1,0);
	glVertex2i(32, 80);
	glTexCoord2f(1,1);
	glVertex2i(32, 112);
	glTexCoord2f(0,1);
	glVertex2i(0, 112);
	glEnd();

	if (chatlife > 0) co = chatoff; else co = chatoff - (int)(3*rate);
	glBindTexture(GL_TEXTURE_2D, hud[4]);
	glBegin(GL_QUADS);
	glTexCoord2f(0,0);
	glVertex2i(144, 600+co);
	glTexCoord2f(1,0);
	glVertex2i(656, 600+co);
	glTexCoord2f(1,1);
	glVertex2i(656, 632+co);
	glTexCoord2f(0,1);
	glVertex2i(144, 632+co);
	glEnd();

	if (bChatting != 0) {
		glBindTexture(GL_TEXTURE_2D, hud[5]);
		glBegin(GL_QUADS);
		glTexCoord2f(0,0);
		glVertex2i(144, 536);
		glTexCoord2f(1,0);
		glVertex2i(656, 536);
		glTexCoord2f(1,1);
		glVertex2i(656, 568);
		glTexCoord2f(0,1);
		glVertex2i(144, 568);
		glEnd();
	}

	if (status == 3)
	{
		glBindTexture(GL_TEXTURE_2D, hqmenu);
		glBegin(GL_QUADS);
		glTexCoord2f(0,0);
		glVertex2i(600, 0);
		glTexCoord2f(1,0);
		glVertex2i(800, 0);
		glTexCoord2f(1,1);
		glVertex2i(800, 600);
		glTexCoord2f(0,1);
		glVertex2i(600, 600);
		glEnd();

		glBindTexture(GL_TEXTURE_2D,0);
		glColor3f(1.0f,0.0f,0.0f);

		if (hqmenusel < 7) {
			glBegin(GL_TRIANGLES);
			glVertex2i(600, hqmenusel * 12 + 175);
			glVertex2i(609, hqmenusel * 12 + 180);
			glVertex2i(600, hqmenusel * 12 + 185);
			glVertex2i(800, hqmenusel * 12 + 175);
			glVertex2i(791, hqmenusel * 12 + 180);
			glVertex2i(800, hqmenusel * 12 + 185);
			glEnd();
		} else if (hqmenusel < 12) {
			glBegin(GL_TRIANGLES);
			glVertex2i(600, (hqmenusel-5) * 12 + 255);
			glVertex2i(609, (hqmenusel-5) * 12 + 260);
			glVertex2i(600, (hqmenusel-5) * 12 + 265);
			glVertex2i(800, (hqmenusel-5) * 12 + 255);
			glVertex2i(791, (hqmenusel-5) * 12 + 260);
			glVertex2i(800, (hqmenusel-5) * 12 + 265);
			glEnd();
		} else if (hqmenusel < 15) {
			glBegin(GL_TRIANGLES);
			glVertex2i(600, (hqmenusel-11) * 12 + 338);
			glVertex2i(609, (hqmenusel-11) * 12 + 343);
			glVertex2i(600, (hqmenusel-11) * 12 + 348);
			glVertex2i(800, (hqmenusel-11) * 12 + 338);
			glVertex2i(791, (hqmenusel-11) * 12 + 343);
			glVertex2i(800, (hqmenusel-11) * 12 + 348);
			glEnd();
		} else {
			glBegin(GL_TRIANGLES);
			glVertex2i(600, (hqmenusel-14) * 12 + 385);
			glVertex2i(609, (hqmenusel-14) * 12 + 390);
			glVertex2i(600, (hqmenusel-14) * 12 + 395);
			glVertex2i(800, (hqmenusel-14) * 12 + 385);
			glVertex2i(791, (hqmenusel-14) * 12 + 390);
			glVertex2i(800, (hqmenusel-14) * 12 + 395);
			glEnd();
		}

		glColor3f(1.0f,1.0f,1.0f);
		drawnumber(640,20,cash[0]);
		drawnumber(640,80,cash[1]);

	} else {
		glColor4f(1.0f,1.0f,1.0f,0.75f);
		glBindTexture(GL_TEXTURE_2D, minimap);
		glBegin(GL_QUADS);
		glTexCoord2f(0,0);
		glVertex2i(700, 0);
		glTexCoord2f(0.78125,0);
		glVertex2i(800, 0);
		glTexCoord2f(0.78125,0.78125);
		glVertex2i(800, 100);
		glTexCoord2f(0,0.78125);
		glVertex2i(700, 100);
		glEnd();
		glColor4f(1.0f,1.0f,1.0f,1.0f);

        if (cash[0] > 9999)
		   drawnumber(640,100,cash[0]);
        else if (cash[0] > 999)
		   drawnumber(672,100,cash[0]);
        else if (cash[0] > 99)
		   drawnumber(704,100,cash[0]);
        else if (cash[0] > 9)
		   drawnumber(736,100,cash[0]);
        else
		   drawnumber(768,100,cash[0]);

        if (cash[1] > 9999)
		   drawnumber(640,140,cash[1]);
        else if (cash[1] > 999)
		   drawnumber(672,140,cash[1]);
        else if (cash[1] > 99)
		   drawnumber(704,140,cash[1]);
        else if (cash[1] > 9)
		   drawnumber(736,140,cash[1]);
        else
		   drawnumber(768,140,cash[1]);
    }

/*	if (!multiplayer && showmoney) {
		drawnumber(640,20,cash[0]);
		drawnumber(640,80,cash[1]);
	}*/


	glBindTexture(GL_TEXTURE_2D, cursor); 
	glBegin(GL_QUADS);
	glTexCoord2f(0,0);
	glVertex2i(mx-16, my-16);
	glTexCoord2f(1,0);
	glVertex2i(mx+16, my-16);
	glTexCoord2f(1,1);
	glVertex2i(mx+16, my+16);
	glTexCoord2f(0,1);
	glVertex2i(mx-16, my+16);
	glEnd();
	drawnumber(40,0,abase);
	drawnumber(120,0,nbase);
	drawnumber(40,40,ammo);
	drawnumber(40,80,health);

}

void drawnumber(int x, int y, int num)
{
	int digit,numdigs=1;

	while (numdigs <= num)
		numdigs *= 10;

	while (numdigs > 1)
	{
		numdigs/=10;
		digit = num / numdigs;
		num = num % numdigs;

		glBindTexture(GL_TEXTURE_2D, digits[digit]);
		glBegin(GL_QUADS);
		glTexCoord2f(0,0);
		glVertex2i(x, y);
		glTexCoord2f(1,0);
		glVertex2i(x+32, y);
		glTexCoord2f(1,1);
		glVertex2i(x+32, y+32);
		glTexCoord2f(0,1);
		glVertex2i(x, y+32);
		glEnd();
		x+=32;
	}

}

void free_sounds()
{
	int i;
	Mix_HaltChannel(-1);
	for (i=0; i<NUM_SFX; i++)
		Mix_FreeChunk(sfx[i]);
}
void load_sounds()
{
	int i;
	char buffer[80];
	for (i=0; i<NUM_SFX; i++)
	{
		sprintf(buffer,"audio/sfx/%d.wav",i);
		sfx[i] = Mix_LoadWAV(buffer);
		if (!sfx[i]) fprintf(stderr,"Error: couldn't load WAV file %s\n",buffer);
	}
}
void play_sound (int number, int x, int y)
{
	int channel;
	float distance;
	Sint16 angle;

	angle = (Sint16)(90-(atan2((float)y-camy,(float)x-camx) * 180 / 3.1415926));
	distance = sqrt(((float)y-camy)*((float)y-camy) + ((float)x-camx)*((float)x-camx)) / 2;
	if (number < 5) distance *= 2;
	if (sfxon && distance <= 200 )
	{
		channel = Mix_PlayChannel (-1, sfx[number], 0);
		if (distance > 5)
			Mix_SetPosition(channel, angle, (Uint8)distance);
	}
}

void load_map(const char* name)
{
	int i, j;
	unsigned char incoming;
	FILE *fp;
    Uint32 *ts_pix, pcolor;

	SDL_Surface *ts;
	
	ts = SDL_CreateRGBSurface(SDL_SWSURFACE, 128, 128, 32,
	   #if SDL_BYTEORDER == SDL_LIL_ENDIAN // OpenGL RGBA masks 
                               0x000000FF, 
                               0x0000FF00, 
                               0x00FF0000, 
                               0xFF000000
#else
                               0xFF000000,
                               0x00FF0000, 
                               0x0000FF00, 
                               0x000000FF
#endif
);

	fp = fopen (name,"r");
	if (fp == NULL)
	{
		fprintf(stderr,"Error opening map %s!\n",name);
		gamestate=0;
	} else {
           SDL_LockSurface(ts);
           ts_pix = (Uint32*)ts->pixels;
		for (j=0; j<MAP_Y; j++)
			for (i=0; i<MAP_X; i+=2)
			{
				trees[i][j]=0;
				trees[i+1][j]=0;
				map[i][j]=0;
				map[i+1][j]=0;
				incoming = fgetc(fp);
/*				ts_pix[100] = 1000;
				ts_pix[101] = 1000;
				ts_pix[102] = 1000;
				ts_pix[103] = 1000;
				ts_pix[104] = 1000;*/
				if (incoming & 0x40) trees[i][j]=rand()%4+12;
				if (incoming & 0x04) trees[i+1][j]=rand()%4+12;
				if (incoming & 0x30) map[i][j]=(((incoming & 0x30) >> 4) - 1)* 4 + rand()%4+1;
				if (incoming & 0x03) map[i+1][j]=((incoming & 0x03) - 1) * 4 + rand()%4+1;

//				ts_pix[j*MAP_X+i] = SDL_MapRGB(ts->format,0,0,255);
				if (incoming & 0x40) ts_pix[j*128+i] = SDL_MapRGB(ts->format,0,255,0);
				else if ((incoming & 0x30) >> 4 == 1) ts_pix[j*128+i] = SDL_MapRGB(ts->format,255,255,0);
				else if ((incoming & 0x30) >> 4 == 2) ts_pix[j*128+i] = SDL_MapRGB(ts->format,128,64,0);
				else if ((incoming & 0x30) >> 4== 3) ts_pix[j*128+i] = SDL_MapRGB(ts->format,128,255,128);
				else ts_pix[j*128+i] = SDL_MapRGB(ts->format,0,0,255);

				if (incoming & 0x04) ts_pix[j*128+i+1] = SDL_MapRGB(ts->format,0,255,0);
				else if ((incoming & 0x03) == 1) ts_pix[j*128+i+1] = SDL_MapRGB(ts->format,255,255,0);
				else if ((incoming & 0x03) == 2) ts_pix[j*128+i+1] = SDL_MapRGB(ts->format,128,64,0);
				else if ((incoming & 0x03) == 3) ts_pix[j*128+i+1] = SDL_MapRGB(ts->format,128,255,128);
				else ts_pix[j*128+i+1] = SDL_MapRGB(ts->format,0,0,255);
                //*((Uint32 *)ts->pixels + (j * ts->w) + i) = SDL_MapRGBA(ts->format,0,0,255,255);

			}

		for (i=0; i<15; i++)
		{
			for (j=0; j<3; j++)
			{
				bldloc[i][j] = (int)fgetc(fp);
			}
			if (bldloc[i][0] == 0 && i < 5)
               pcolor = SDL_MapRGB(ts->format,255,0,0);
			else if (bldloc[i][0] != 0)
               pcolor = SDL_MapRGB(ts->format,0,0,0);
            else
               pcolor = 1;
            
            if (pcolor != 1)
            {
			   ts_pix[(bldloc[i][2]-1)*128+bldloc[i][1]-1] = pcolor;
			   ts_pix[(bldloc[i][2]-1)*128+bldloc[i][1]] = pcolor;
			   ts_pix[(bldloc[i][2]-1)*128+bldloc[i][1]+1] = pcolor;
			   ts_pix[(bldloc[i][2])*128+bldloc[i][1]-1] = pcolor;
			   ts_pix[(bldloc[i][2])*128+bldloc[i][1]] = pcolor;
			   ts_pix[(bldloc[i][2])*128+bldloc[i][1]+1] = pcolor;
			   ts_pix[(bldloc[i][2]+1)*128+bldloc[i][1]-1] = pcolor;
			   ts_pix[(bldloc[i][2]+1)*128+bldloc[i][1]] = pcolor;
			   ts_pix[(bldloc[i][2]+1)*128+bldloc[i][1]+1] = pcolor;
            }
          }
           SDL_UnlockSurface(ts);
           //SDL_SaveBMP(ts, "c:\\minimap.bmp");

		initang[0] = (unsigned char)fgetc(fp);
		initang[1] = (unsigned char)fgetc(fp);
	}
	fclose(fp);

	processborder();
	
	glBindTexture( GL_TEXTURE_2D, minimap);
	glTexImage2D(GL_TEXTURE_2D, 0, 4, ts->w, ts->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, ts->pixels);
	SDL_FreeSurface(ts);
}

void killhuman (unsigned int id, int blood)
{
	human *target = tophuman, *temp;
	tophuman = NULL;

	while (target != NULL)
	{
		if (target->id == id || (!multiplayer && target->health == 0))
		{
			temp = target->next;
			if (blood || (!multiplayer && target->health == 0)) {
				createbloodfx(target->x, target->y);
				createbloodfx(target->x, target->y);
				createbloodfx(target->x, target->y);
				createbloodfx(target->x, target->y);
				createbloodfx(target->x, target->y);
				if (!multiplayer && target->health == 0)
					play_sound(rand()%2+17,target->x,target->y);
			}
			free(target);
			target = temp;
		} else if (multiplayer && target->touched == 0) {  // untouched players simply disappear.
			temp = target->next;
			free(target);
			target = temp;
		} else {
			temp = target->next;
			target->next = tophuman;
			tophuman = target;
			target = temp;
		}
	}
}

void createbloodfx (int x, int y)
{
	int i;
	for (i=0; i<5; i++)
	{
		createparticle(x,y,rand()%256,rand()%7+1,5,0);
	}
}

void createexplosion (int x, int y, int size)
{
	int i;
	if (map[x/32][y/32] == 0 && trees[x/32][y/32] == 0) createsplash(x,y);
	for (i=0; i<size*50; i++)
	{
		createparticle(x,y,rand()%256,rand()%20+1,rand()%3+3,rand()%4+1);
	}
	play_sound(24+rand()%3,x,y);
}

void createsplash (int x, int y)
{
	int i;
	for (i=0; i<50; i++)
	{
		createparticle(x,y,rand()%256,rand()%20+1,rand()%3+3,rand()%4+4);
	}
	play_sound(19,x,y);
}

void createsparks (int x, int y)
{
	int i;
	for (i=0; i<5; i++)
	{
		createparticle(x,y,rand()%256,rand()%20+1,rand()%3+3,rand()%2+8);
	}
}

void createparticle (int x, int y, unsigned char angle, unsigned int speed, unsigned char life, unsigned char type)
{
	particle *np;
	np = (particle *) malloc (sizeof (struct particle));
	np->x=x;
	np->y=y;
	np->angle=angle;
	np->speed=speed;
	np->type=type;
	np->life=life;
	np->next=toppart;
	toppart=np;
}

void texthqmenu(int x, int y, const char* message, SDL_Surface *hqmenu)
{
	unsigned int i;
	SDL_Rect sr, dr;
	sr.h=8;
	sr.w=8;
	sr.y=0;
	dr.x=x;
	dr.y=y;
	dr.w=8;
	dr.h=8;
	for (i=0; i<strlen(message); i++)
	{
		sr.x = 8 * (message[i] & 0x7F);
		SDL_BlitSurface(font,&sr,hqmenu,&dr);
		dr.x+=8;
	}
}

void drawchat (const char *message, int speaker)
{
	unsigned int i;
	SDL_Surface *temp;
	SDL_Rect sr,dr;
	sr.h=8;
	sr.w=8;
	sr.y=0;
	dr.x=64;
	dr.y=6;
	dr.w=8;
	dr.h=8;
	temp = SDL_ConvertSurface(bubble, bubble->format, bubble->flags);
	SDL_BlitSurface(speakers[speaker],NULL,temp,NULL);

	for (i=0; i<strlen(oldmessage); i++)
	{
		sr.x = 8 * (oldmessage[i] & 0x7F);
		SDL_BlitSurface(font,&sr,temp,&dr);
		dr.x+=8;
	}

	dr.x = 64;
	dr.y = 16;
	for (i=0; i<strlen(message); i++)
	{
		sr.x = 8 * (message[i] & 0x7F);
		SDL_BlitSurface(font,&sr,temp,&dr);
		dr.x+=8;
	}
	strncpy(oldmessage,message,69);
	glBindTexture(GL_TEXTURE_2D,hud[4]);
	glTexImage2D(GL_TEXTURE_2D, 0, 4, temp->w, temp->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, temp->pixels);
	SDL_FreeSurface(temp);
	chatoff=-32;
	chatlife=20;
}

void chatup (const char *message)
{
	unsigned int i;
	SDL_Surface *temp;
	SDL_Rect sr,dr;
	char msg1[20];
	sr.h=8;
	sr.w=8;
	sr.y=0;
	dr.x=64;
	dr.y=6;
	dr.w=8;
	dr.h=8;
	temp = SDL_ConvertSurface(bubble, bubble->format, bubble->flags);
	SDL_BlitSurface(speakers[0],NULL,temp,NULL);

	if (bChatting == 1)
		strcpy(msg1,"GLOBAL CHAT");
	else
		strcpy(msg1,"TEAM CHAT");

	for (i=0; i<strlen(msg1); i++)
	{
		sr.x = 8 * (msg1[i] & 0x7F);
		SDL_BlitSurface(font,&sr,temp,&dr);
		dr.x+=8;
	}

	dr.x = 64;
	dr.y = 16;
	for (i=0; i<strlen(message); i++)
	{
		sr.x = 8 * (message[i] & 0x7F);
		SDL_BlitSurface(font,&sr,temp,&dr);
		dr.x+=8;
	}
	glBindTexture(GL_TEXTURE_2D,hud[5]);
	glTexImage2D(GL_TEXTURE_2D, 0, 4, temp->w, temp->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, temp->pixels);
	SDL_FreeSurface(temp);
}

gameobj * createobject (int type, int x, int y, unsigned char align,int objid)
{
	gameobj *temp;
	gameobj *src = locate_object(objlib,type);
	//	      fprintf(stderr,"I located type %d.\n",type);
	if (src != NULL)
	{
		objidnum = objid;
		temp = createobjfromlib(src,NULL,align);
		//		              fprintf(stderr,"I created...\n");
		temp->x = x;
		temp->y = y;
		temp->align = align;
		temp->next = topobj;
		topobj = temp;
		sobjidnum = objidnum;
		//fprintf(stderr,"Spawned a new type %d at %d, %d (id %d).\n",type,x,y,temp->id);
		return temp;
	} else {
		printf("Warning: object %d not found in library!\n",type);
		return NULL;
	}
}

gameobj *createobjfromlib (gameobj *src,gameobj *parent,unsigned char align)
{
	gameobj *dest = NULL;
	gameobj *kids = NULL, *temp;

	dest = (gameobj *) malloc (sizeof (struct gameobj));
	//	fprintf(stderr,"I malloc'd...\n");
	//	strcpy(dest->name,src->name);
	dest->type=src->id;
	dest->img=src->img;
	//	fprintf(stderr,"I set type...\n");

	dest->id=objidnum;
	objidnum++;

	//	fprintf(stderr,"I set ID num=%d...\n",dest->id);
	dest->x=src->x;
	dest->y=src->y;
	dest->xoff=src->xoff;
	dest->yoff=src->yoff;
	if (parent == NULL) dest->angle=initang[align]; else dest->angle = 0;
	dest->topspeed=src->topspeed;
	dest->accel=src->accel;
	dest->maxang=src->maxang;
	dest->hp=src->hp;
	dest->proj=src->proj;
	dest->cool=src->cool;
	dest->mtoken=src->mtoken;
	dest->ktoken=src->ktoken;
	dest->seat=src->seat;
	dest->seatedimg=src->seatedimg;
	dest->waterborne=src->waterborne;
	dest->plane=src->plane;
	dest->zoom=src->zoom;
	dest->width=src->width;
	dest->height=src->height;
	kids = src->subobj;

	dest->coolleft=0;
	dest->align=align;
	dest->speed=0;
	dest->anglediff=0;
	dest->occupied=0;
	dest->subobj=NULL;
	dest->parent=parent;

	while (kids!=NULL)
	{
		temp = createobjfromlib(kids,dest,align);
		temp->next = dest->subobj;
		dest->subobj=temp;
		kids=kids->next;
	}
	return dest;
}

int retrieveangles(gameobj *go,int offset)
{
	gameobj *kid;

	if (go != NULL)
	{
		go->angle = p->data[offset];
		if (go->parent == NULL)
		{
			go->align = p->data[offset + 1] >> 7;
		} else {
			go->occupied = p->data[offset + 1] >> 7;
		}
		if (p->data[offset + 1] & 0x40)
		{
			go->anglediff = (p->data[offset + 1] & 0x7F) | 0x80;
		} else {
			go->anglediff = p->data[offset + 1] & 0x7F;
		}
		offset += 2;

		kid = go->subobj;
		while (kid != NULL)
		{
			offset = retrieveangles(kid,offset);
			kid = kid->next;
		}
	}
	return offset;
}

void vehcleanup()
{
	// this destroys a vehicle, kills players and drivers, fixes status, etc.
	gameobj *target = topobj, *temp;
	topobj = NULL;

	while (target != NULL)
	{
		if (target->touched == 0)
		{
			play_sound(27+rand()%2,target->x,target->y);
			temp = target;
			target=target->next;
			recdv(temp);
		} else {
			temp = target->next;
			target->next = topobj;
			topobj = target;
			target = temp;
		}
	}
}

void recdv (gameobj *veh)
{
	gameobj *sub = veh->subobj,*t;
	while (sub != NULL)
	{
		t = sub->next;
		recdv(sub);
		sub=t;
	}
	free(veh);
}



void processborder()
{
	int i, j, mc;

	for (i=1; i<99; i++)
	{
		for (j=1; j<99; j++)
		{
			tb[i][j]=0;
			if (trees[i][j] == 0) {
				if (trees[i][j-1] != 0)
					tb[i][j] = 1;
				if (trees[i][j+1] != 0)
					tb[i][j] = tb[i][j] | 2;
				if (trees[i-1][j] != 0)
					tb[i][j] = tb[i][j] | 4;
				if (trees[i+1][j] != 0)
					tb[i][j] = tb[i][j] | 8;
			}

			mb[i][j]=0;
			mc = (map[i][j] + 3) / 4;

			if ((map[i][j-1] + 3) / 4 > mc)
				mb[i][j] = (int)(map[i][j-1] + 3) / 4;
			if ((map[i][j+1] + 3) / 4 > mc)
				mb[i][j] = mb[i][j] | (((int)(map[i][j+1] + 3) / 4) << 2);
			if ((map[i-1][j] + 3) / 4 > mc)
				mb[i][j] = mb[i][j] | (((int)(map[i-1][j] + 3) / 4) << 4);
			if ((map[i+1][j] + 3) / 4 > mc)
				mb[i][j] = mb[i][j] | (((int)(map[i+1][j] + 3) / 4) << 6);
		}
	}
}

/* GO SINGLE PLAYER GO. */
/* GO SINGLE PLAYER GO. */
/* GO SINGLE PLAYER GO. */
/* GO SINGLE PLAYER GO. */
/* GO SINGLE PLAYER GO. */
/* GO SINGLE PLAYER GO. */
/* GO SINGLE PLAYER GO. */
/* GO SINGLE PLAYER GO. */
/* GO SINGLE PLAYER GO. */
/* GO SINGLE PLAYER GO. */
/* GO SINGLE PLAYER GO. */
/* GO SINGLE PLAYER GO. */
/* GO SINGLE PLAYER GO. */
/* GO SINGLE PLAYER GO. */

/*simulategame();
  updateplayers();*/

void recobjmove(gameobj * temp)
{
	while (temp!=NULL)
	{
		temp->angle=temp->angle+temp->anglediff;
		temp->anglediff = 0;

		if (temp->coolleft>0) temp->coolleft--; else temp->coolleft = 0;

		recobjmove(temp->subobj);

		temp=temp->next;
	}
}


void singlegame()
{
	struct gameobj *objptr=topobj, *drill,*nxobj;
	struct human *humptr=tophuman;
	struct particle *partptr, *partptr2;
	projectile *projptr=proj, *pp2;
	int i, vehid,j,landed, oldhealth;
	int oldx, oldy;
	float dist, tangle2, tx, ty, tx2, ty2;
	unsigned char tangle;

	if (ticks+UPDATEFREQ<SDL_GetTicks())
	{
        localcontrols[4] = unshoot[0];
		localcontrols[5] = unshoot[1];

		ticks=SDL_GetTicks() ;
		curScripticks++;
		if (curScripticks == scriptTime[curScriptloc][0])
		{
			drawchat(gscript[curScriptloc], scriptTime[curScriptloc][1]);
			curScriptloc++;
		}

		if (chatlife > 0) chatlife--;
		else if (chatoff < 0) chatoff+=3;
		else {chatoff=0; chatlife = 0;}

		aithink();

		for (i=0; i<9; i++)
			players[0].controls[i] = localcontrols[i];

		cashmoney++;
		if (cashmoney > 4) {
			cash[0]-=2;  //upkeep
			cash[1]-=2;
			if (bldloc[0][0] == 8) cash[0]+=4; else if (bldloc[0][0] == 9) cash[1]+=4;
			if (bldloc[1][0] == 8) cash[0]+=4; else if (bldloc[1][0] == 9) cash[1]+=4;
			if (bldloc[2][0] == 8) cash[0]+=4; else if (bldloc[2][0] == 9) cash[1]+=4;
			cashmoney = 0;
			//	sprintf(blah,"%d vs %d",cash[0],cash[1]);
			//	drawchat(blah,1);
		}

		if (cash[0] <=0 || cash[1] <= 0)
		{
			humptr = tophuman;
			while (humptr != NULL)
			{
				humptr->health = 0;
				humptr=humptr->next;
			}
			while (objptr != NULL)
			{
				objptr->hp = 0;
				objptr=objptr->next;
			}
			killhuman(0,0);
			destroyvehicle(0);

			if (cash[0] <= 0)
			{
				gamestate = 5;
			} else if (cash[1] <= 0) {
				gamestate = 4;
			}
		} else {

			partptr = toppart;
			partptr2 = toppart;
			while (partptr!=NULL)
			{
				partptr->x = partptr->x-(int)((double)partptr->speed*sintable[partptr->angle] * rate);
				partptr->y = partptr->y-(int)((double)partptr->speed*-costable[partptr->angle] * rate);
				partptr->life--;
				if (partptr->life==0)
				{
					if (partptr2==partptr) {
						toppart=partptr->next;
						free(partptr);
						partptr2=toppart;
						partptr=toppart;
					} else {
						partptr2->next=partptr->next;
						free(partptr);
						partptr=partptr2->next;
					}
				} else {
					partptr2=partptr;
					partptr=partptr2->next;
				}
			} 

			humptr=tophuman;
			while (humptr!=NULL)
			{
				oldx = humptr->x;
				oldy = humptr->y;
				humptr->x+=(int)((float)humptr->speed * sintable[humptr->angle]);
				humptr->y+=(int)((float)humptr->speed * -costable[humptr->angle]);
				if (humptr->speed != 0)
				{
					if (humptr->legsimg==0) humptr->legsimg=1;
					else if (map[humptr->x/32][humptr->y/32] != 0 || trees[humptr->x/32][humptr->y/32] != 0) {humptr->legsimg = 0;
						play_sound(rand() % 5,humptr->x,humptr->y);}
				}
				if (humptr->x<0) humptr->x=0;
				if (humptr->y<0) humptr->y=0;
				if (humptr->x>3199) humptr->x=3199; // values in pixels!
				if (humptr->y>3199) humptr->y=3199;
				if (humptr->cooldown>0) humptr->cooldown--;

				for (i=0; i<3; i++)
				{
					if (bldhumcollide(i,humptr))
					{
						bldloc[i][0] = 8 + humptr->align;
					}
				}

				if (map[humptr->x/32][humptr->y/32] == 0 && trees[humptr->x/32][humptr->y/32] == 0)
				{
					createparticle(humptr->x,humptr->y,(humptr->angle + 118 + (rand() % 20))%256,rand()%20+1,rand()%3+3,rand()%4+4);
					createparticle(humptr->x,humptr->y,(humptr->angle + 118 + (rand() % 20))%256,rand()%20+1,rand()%3+3,rand()%4+4);
					createparticle(humptr->x,humptr->y,(humptr->angle + 118 + (rand() % 20))%256,rand()%20+1,rand()%3+3,rand()%4+4);
				}

				humptr=humptr->next;
			}

			pp2=proj;
			while (projptr != NULL)
			{
				oldx = projptr->x;
				oldy = projptr->y;
				projptr->x += (int)((float)projptr->speed*sintable[projptr->angle]);
				projptr->y += (int)((float)projptr->speed*-costable[projptr->angle]);

				// let's see if we hit any humans.  Point collision with a circle: use distance formula.
				humptr=tophuman;
				while (humptr!=NULL)
				{
					if (projptr->type < 6 || projptr->type > 12) {
						if ((projptr->align != humptr->align) && bulhumcollide(humptr,oldx,oldy,projptr->x,projptr->y))
						{
							if (projptr->type == 0 || projptr->type == 3 || projptr->type == 14)
							{
								humptr->health-=10;
							} else if (projptr->type == 2 || projptr->type == 15) {
								humptr->health-=75;
							} else {
								humptr->health-=100;
								createexplosion(projptr->x, projptr->y,1);
							}
							createbloodfx(humptr->x,humptr->y);
							projptr->life=1;
						}
					}else if (projptr->type < 8) {
						if (humptr->weapon != 8 && (projptr->align == humptr->align) && bulhumcollide(humptr,oldx,oldy,projptr->x,projptr->y))
						{
							play_sound(12,humptr->x,humptr->y);
							projptr->life=1;
							if (projptr->type == 6)
							{
								humptr->health=100;
							} else { // type = 8
								humptr->ammo=20;
							}
						}
					} else {
						if (bulhumcollide(humptr,oldx,oldy,projptr->x,projptr->y))
						{
							play_sound(12,humptr->x,humptr->y);
							humptr->carriedarms = projptr->type - 4;
							humptr->weapon = humptr->carriedarms;
							humptr->ammo = 20;
							projptr->life=1;
						}
					}

					humptr=humptr->next;
				}

				objptr=topobj;
				while (objptr!=NULL)
				{
					if (projptr->type < 6 || projptr->type > 12) {
						if ((projptr->align != objptr->align) && bulvehcollide(objptr,oldx,oldy,projptr->x,projptr->y, projptr->type))
						{
							createsparks(projptr->x, projptr->y);

							if (projptr->type == 0 || projptr->type == 3 || projptr->type == 2)
							{
								if (rand() % 2) play_sound(29+rand()%2,projptr->x,projptr->y);
								objptr->hp-=3; // pistol or machine gun bullets are not too effective
								projptr->life=1;
							} else if (projptr->type == 14) { // special case for aa gun
								if (objptr->plane == 1) objptr->hp-=15; else {if (rand() % 2)play_sound(29+rand()%2,projptr->x,projptr->y); objptr->hp-=5;}
								projptr->life = 1;
							} else if (projptr->type == 15) {
								objptr->hp-=50; // dgun bullets are pretty tough
								projptr->life=1;
							} else if (projptr->type == 16) {
								if (objptr->type == 21 || objptr->type == 31) objptr->hp-=40; else // bombs must be weakened against ships
									objptr->hp-=100;
								projptr->life=1;
								createexplosion(projptr->x, projptr->y,1);
							} else { // just blow up.
								objptr->hp-=100;
								projptr->life=1;
								createexplosion(projptr->x, projptr->y,1);
							}
						}
					}
					objptr=objptr->next;
				}

				projptr->life--;
				if (projptr->x < 0 || projptr->y < 0 || projptr->life <= 0)
				{
					if (projptr->x < 0 || projptr->y < 0)
					{
						projptr->x = 6000; // some huge number.
						projptr->y = 6000;
					}
					if (projptr->type == 1 || projptr->type == 4 || projptr->type == 5 || projptr->type == 13 || projptr->type == 16)
						createexplosion(projptr->x, projptr->y,1);
					if (pp2==projptr) {
						proj=projptr->next;
						free(projptr);
						pp2=proj;
						projptr=pp2;
					} else {
						pp2->next=projptr->next;
						free(projptr);
						projptr=pp2->next;
					}
					//	      printf("expired.");
				} else {
					pp2 = projptr;
					projptr=projptr->next;
				}
			}

			objptr = topobj;
			while (objptr != NULL)
			{
				nxobj = objptr->next;
				oldx = objptr->x;
				oldy = objptr->y;
				objptr->x=(int)(objptr->x+(objptr->speed*sintable[objptr->angle]));
				objptr->y=(int)(objptr->y+(objptr->speed*-costable[objptr->angle]));
				if (objptr->speed > 0) objptr->speed--;
				if (objptr->speed < 0) objptr->speed++;
				if (objptr->x < 0) objptr->x=0;
				if (objptr->x > 3199) objptr->x=3199;
				if (objptr->y < 0) objptr->y=0;
				if (objptr->y > 3199) objptr->y=3199;


				//printf("(%d,%d)\n",objptr->x/32,objptr->y/32);
				if (objptr->plane == 1) {
					if (objptr->speed <= 30) {
						if (map[objptr->x / 32][objptr->y / 32] == 0 && trees[objptr->x/32][objptr->y/32] == 0) {
							createsplash(objptr->x, objptr->y);
							play_sound(27+rand()%2,objptr->x,objptr->y);
							destroyvehicle(objptr->id);
							objptr = NULL;
						} else if (trees[objptr->x/32][objptr->y/32] != 0) {
							createexplosion(objptr->x, objptr->y,1);
							play_sound(27+rand()%2,objptr->x,objptr->y);
							destroyvehicle(objptr->id);
							objptr = NULL;
						}
					}
				} else if (objptr->waterborne == 1) {
					createparticle((int)(objptr->x - (objptr->height/2 * sintable[(int)(objptr->angle + 252 + (rand() % 6)) % 256])),(int)(objptr->y - (objptr->height/2 * -costable[(int)(objptr->angle + 252 + (rand() % 6)) % 256])),(objptr->angle + 118 + (rand() % 20))%256,rand()%20+1,rand()%3+3,rand()%4+4);
					createparticle((int)(objptr->x - (objptr->height/2 * sintable[(int)(objptr->angle + 252 + (rand() % 6)) % 256])),(int)(objptr->y - (objptr->height/2 * -costable[(int)(objptr->angle + 252 + (rand() % 6)) % 256])),(objptr->angle + 118 + (rand() % 20))%256,rand()%20+1,rand()%3+3,rand()%4+4);
					createparticle((int)(objptr->x - (objptr->height/2 * sintable[(int)(objptr->angle + 252 + (rand() % 6)) % 256])),(int)(objptr->y - (objptr->height/2 * -costable[(int)(objptr->angle + 252 + (rand() % 6)) % 256])),(objptr->angle + 118 + (rand() % 20))%256,rand()%20+1,rand()%3+3,rand()%4+4);
					if (map[objptr->x/32][objptr->y/32] != 0 || trees[objptr->x/32][objptr->y/32] != 0)
					{
						objptr->x = oldx;
						objptr->y = oldy;
					} else if (map[min(99,max(0,(objptr->x + objptr->height/2 * sintable[objptr->angle]) / 32))]
							[min(99,max(0,(objptr->y + objptr->height/2 * -costable[objptr->angle]) / 32))] != 0 ||
							trees[min(99,max(0,(objptr->x + objptr->height/2 * sintable[objptr->angle]) / 32))]
							[min(99,max(0,(objptr->y + objptr->height/2 * -costable[objptr->angle]) / 32))] != 0 ) {
						objptr->x = oldx;
						objptr->y = oldy;
					} else if (map[min(99,max(0,(objptr->x - objptr->height/2 * sintable[objptr->angle]) / 32))]
							[min(99,max(0,(objptr->y - objptr->height/2 * -costable[objptr->angle]) / 32))] != 0 ||
							trees[min(99,max(0,(objptr->x - objptr->height/2 * sintable[objptr->angle]) / 32))]
							[min(99,max(0,(objptr->y - objptr->height/2 * -costable[objptr->angle]) / 32))] != 0  ) {
						objptr->x = oldx;
						objptr->y = oldy;
					}
				} else if (map[objptr->x / 32][objptr->y / 32] == 0 && trees[objptr->x / 32][objptr->y / 32] == 0) {
					createsplash(objptr->x, objptr->y);
					play_sound(27+rand()%2,objptr->x,objptr->y);
					destroyvehicle(objptr->id);
					objptr = NULL;
				} 
				if (objptr != NULL) {

					humptr = tophuman;
					while (humptr != NULL)
					{
						if (humvehcollide(humptr,objptr)) {
							if (abs(objptr->speed) > 5)
							{
								humptr->health = 0;
								objptr->speed -=5;
							}
							else objptr->speed = 0;
						}
						//	      createfx(humptr->x, humptr->y, 1);
						humptr=humptr->next;
					}

					drill = topobj;
					while (drill != NULL)
					{
						if (drill != objptr && vehvehcollide(drill,objptr))
						{
                            if (3 * (abs(objptr->speed) + abs(drill->speed)) > 0) {
							createsparks(drill->x,drill->y);
							createsparks(objptr->x,objptr->y);
					play_sound(13,objptr->x,objptr->y);
							drill->hp -= 3*(abs(objptr->speed) + abs(drill->speed));
							objptr->hp -= 3*(abs(objptr->speed) + abs(drill->speed));

							drill->speed = 0;
							objptr->speed = 0;
							objptr->x = oldx;
							objptr->y = oldy;
                            }
						}
						drill=drill->next;
					}

					for (i=0; i<15; i++)
					{
						if (bldloc[i][0] != 0 && bldloc[i][0] != 8 && bldloc[i][0] != 9 && bldvehcollide(i,objptr))
						{
                            if (3 * abs(objptr->speed) > 0) {
					play_sound(13,objptr->x,objptr->y);
							createsparks(objptr->x,objptr->y);
							objptr->hp -= 3*(abs(objptr->speed));
							objptr->speed = 0;
							objptr->x = oldx;
							objptr->y = oldy;
                        }
						}
					}
				}
				objptr=nxobj;
			}

			recobjmove(topobj);  // does all the angle updating.

			// last apply all new player input
			for (i=0; i<32; i++)
			{
				if (players[i].status==0) {
//					printf("Player %d is dead and needs a new body.\n",i);
					humptr=(human *) malloc (sizeof (human));
					if (humptr==NULL) { fprintf(stderr,"Memory error!\n"); exit(1); }
					humptr->next=tophuman;
					tophuman=humptr;
					tophuman->id=idnum;
					tophuman->align=i % 2;
					tophuman->legsimg = 0;
					landed=0;
					while (!landed)
					{
						landed = 1;
						if (tophuman->align == 0) {
							tophuman->x=bldloc[3][1]*32 + rand()%400-200;
							tophuman->y=bldloc[3][2]*32 + rand()%400-200;
						} else {
							tophuman->x=bldloc[4][1]*32 + rand()%400-200;
							tophuman->y=bldloc[4][2]*32 + rand()%400-200;
						}
						for (j = 0; j < 15; j++)
						{
							if (bldhumcollide(j,tophuman)) landed=0;
						}
					}
					tophuman->angle=initang[tophuman->align];
					tophuman->torsoangle=0;
					tophuman->speed=0;
					tophuman->weapon=3;
					tophuman->ammo=20;
					tophuman->health=100;
					tophuman->carriedarms=3; //4 + (rand()%3);
					tophuman->cooldown=0;
					players[i].status=1;
					players[i].controller_k=idnum;
					idnum++;
					cash[tophuman->align] -=15;
				} else if (players[i].status==1)
				{
					humptr = locate_human(tophuman,players[i].controller_k);
					if (humptr!=NULL)
					{
						// first of all: are we dead?
						if (humptr->health <= 0) {humptr->health = 0; players[i].status=0;}
						else {
							if (players[i].controls[0]) humptr->angle-=16;
							if (players[i].controls[1]) humptr->angle+=16;
							if (players[i].controls[2]) humptr->speed=15;
							else if (players[i].controls[3]) humptr->speed=-15;
							else humptr->speed=0;

							players[i].zoomlevel=0;
							if (players[i].controls[4])
							{
								if (humptr->cooldown == 0 && (map[humptr->x/32][humptr->y/32] != 0 || trees[humptr->x/32][humptr->y/32] != 0))
								{
									if (humptr->weapon == 3)
									{
										humptr->cooldown=2;
										createbullet(humptr->align,humptr->x,humptr->y,humptr->torsoangle + humptr->angle, 150, 0, 3);
									}
									else if (humptr->weapon == 4 && humptr->ammo>0)
									{
										humptr->ammo--;
										humptr->cooldown=15;
										createbullet(humptr->align,humptr->x,humptr->y,humptr->torsoangle + humptr->angle, 100, 1, 4);
									}
									else if (humptr->weapon == 5 && humptr->ammo>0)
									{
										humptr->ammo--;
										humptr->cooldown=10;
										createbullet(humptr->align,humptr->x,humptr->y,humptr->torsoangle + humptr->angle, 200, 2, 3);
									}
									else if (humptr->weapon == 6 && humptr->ammo>0)
									{
										humptr->ammo--;
										createbullet(humptr->align,humptr->x,humptr->y,humptr->torsoangle + humptr->angle, 100, 3, 4);
									}
									else if (humptr->weapon == 7 && humptr->ammo>0)
									{
										humptr->ammo--;
										humptr->cooldown=5;
										createbullet(humptr->align,humptr->x,humptr->y,humptr->torsoangle + humptr->angle, 20, 5, 10);
									}
									else if (humptr->weapon == 8 && humptr->ammo>0)
									{
										humptr->ammo--;
										humptr->cooldown=5;
										createbullet(humptr->align,humptr->x,humptr->y,humptr->torsoangle + humptr->angle, 10, 6, 10);
									}
								}
							}
							if (players[i].controls[5])
							{
								if (humptr->weapon == 5) // sniper rifle zoom!
								{
									humptr->speed=0;
									players[i].zoomlevel=100;
								}
								if (humptr->cooldown == 0)
								{
									if (humptr->weapon == 7 && humptr->ammo>0)
									{
										humptr->ammo--;
										humptr->cooldown=8;
										createbullet(humptr->align,humptr->x,humptr->y,humptr->torsoangle + humptr->angle, 0, 4, 255);
									}
									else if (humptr->weapon == 8 && humptr->ammo>0)
									{
										humptr->ammo--;
										humptr->cooldown=5;
										createbullet(humptr->align,humptr->x,humptr->y,humptr->torsoangle + humptr->angle, 10, 7, 10);
									}
								}

							}
							if (players[i].controls[6])	// vehicle entrance!
							{
								//		   printf("Player tried to enter vehicle.\n");
								vehid = enterveh(humptr);
								if (vehid)
								{
									players[i].status = 2;
									players[i].vehid = vehid;
									players[i].controller_k = checkcontrol(vehid,'K');
									players[i].controller_m = checkcontrol(vehid,'M');
								}
								players[i].controls[0]=0;
								players[i].controls[6]=0;
							}
							if (players[i].controls[7])
							{
								players[i].controls[7]=0;
								if (humptr->weapon!=3)
									humptr->weapon=3;
								else
									humptr->weapon=humptr->carriedarms;
							}
							humptr->torsoangle=players[i].controls[8]-humptr->angle;
							if (humptr->torsoangle > 127 && humptr->torsoangle < 192)
								humptr->torsoangle=192;
							else if (humptr->torsoangle > 63 && humptr->torsoangle < 128)
								humptr->torsoangle=63;
							//           humptr->anglediff=players[i].controls[9]-humptr->angle;
						}
					} else {
						printf("Got a player %d with no body: setting status to DEAD\n",i);
						players[i].status=0;
					}
				} else if (players[i].status==2) {
					if (players[i].controls[6])	// vehicle exit!
					{
						objptr = locate_object(topobj, players[i].vehid);
						players[i].status=1;
						undriverhuman(objptr->occupied);
						players[i].controller_m = objptr->occupied;
						players[i].controller_k = objptr->occupied;
						humptr = locate_human(tophuman,players[i].controller_k);
						objptr->occupied=0;
						while (objptr->parent != NULL) objptr=objptr->parent;
						humptr->x = objptr->x - (int)(((double)objptr->width/2 + 32) * costable[objptr->angle]);
						humptr->y = objptr->y - (int)(((double)objptr->width/2 + 32) * sintable[objptr->angle]);
						players[i].controls[6] = 0;
						//		      printf("There is NO ESCAPE.\n");
					} else {
						objptr = locate_object(topobj, players[i].controller_k);
						if (objptr != NULL) {
							if (players[i].controls[0]) objptr->anglediff = -objptr->maxang;
							if (players[i].controls[1]) objptr->anglediff = objptr->maxang;
							if (players[i].controls[2]) objptr->speed+=objptr->accel;
							else if (players[i].controls[3]) objptr->speed-=objptr->accel;
							if (objptr->speed > objptr->topspeed) objptr->speed = objptr->topspeed;
							if (objptr->speed < -objptr->topspeed) objptr->speed = -objptr->topspeed;

							players[i].controls[6]=0;
							players[i].controls[7]=0;
						}
						// let's do mouse control now.
						objptr = locate_object(topobj, players[i].controller_m);

						if (objptr != NULL) {
							tangle = 0;
							drill = objptr;
							tx2 = 0;
							ty2 = 0;

							while (drill!=NULL)
							{
								tangle2 = atan2(ty2,tx2) + ((double)(drill->angle + drill->anglediff) * 0.02463994235294);
								dist = sqrt(tx2*tx2+ty2*ty2);

								tx = dist * cos(tangle2);
								ty = dist * sin(tangle2);
								if (drill->parent != NULL)
								{
									tx += drill->parent->xoff;
									ty += drill->parent->yoff;
								}
								tx2 = (drill->zoom)*tx + (float)drill->x;
								ty2 = (drill->zoom)*ty + (float)drill->y;

								tangle+=(drill->angle + drill->anglediff);
								drill=drill->parent;
							}
							oldx = (int)tx2;
							oldy = (int)ty2;
							objptr->anglediff=players[i].controls[8]-tangle;
							if (objptr->anglediff > objptr->maxang) objptr->anglediff = objptr->maxang;
							if (objptr->anglediff < -objptr->maxang) objptr->anglediff = -objptr->maxang;

							if (players[i].controls[4] && objptr->coolleft == 0) // oh my, here come bullets.
							{
								objptr->coolleft = objptr->cool;
								if (objptr->proj == 4)
									createbullet(objptr->align,oldx,oldy,(256 + tangle + objptr->anglediff) % 256, 0, objptr->proj, 255);
								else if (objptr->proj == 14)
									createbullet(objptr->align,oldx,oldy,(256 + tangle + objptr->anglediff) % 256, 100, objptr->proj, 6);
								else if (objptr->proj == 16)
									createbullet(objptr->align,oldx,oldy,(256 + tangle + objptr->anglediff) % 256, 0, objptr->proj, 4);
								else if (objptr->proj == 17)
									createbullet(objptr->align,oldx,oldy,(256 + tangle + objptr->anglediff) % 256, 100, 13, 6);
								else
									createbullet(objptr->align,oldx,oldy,(256 + tangle + objptr->anglediff) % 256, 100, objptr->proj, 5);
							}
						}
					}
				}
			}


			killhuman(0,0); // hack: remove all humans with 0 health.
			destroyvehicle(0); // hack: remove all vehicles with 0 health.


			if (players[0].status == 2)
			{
				if (players[0].controller_m != 0) {
					myid = players[0].controller_m;
				} else {
					myid = players[0].vehid;
				}
				drill = locate_object(topobj, players[0].vehid);
				while (drill->parent != NULL) drill = drill->parent;
				health = (unsigned char) drill->hp;
				ammo = 0;
				if (drill->type == 21 || drill->type == 31) players[0].zoomlevel = 100 + abs(drill->speed); else
					players[0].zoomlevel = abs(drill->speed);
			} else if (players[0].status == 1) {
				myid = players[0].controller_k;

				humptr = locate_human(tophuman,myid);

				if (humptr!=NULL)
				{
					oldhealth = health;
					health = humptr->health;
					if (health != oldhealth)
					{
						if (status == 1) play_sound(rand()%3+14,camx,camy);
						// TODO: ricochet / ping sound for getting shot in veh.
					}
					ammo = humptr->ammo;
				}
			}
			localcontrols[6]=0;
			localcontrols[7]=0;
			status = players[0].status;
			oldzoom = newzoom;
			newzoom = (int)players[0].zoomlevel;
		}
	}
}

gameobj *locate_object(gameobj *start, unsigned int target)
{
	gameobj *t, *t2=NULL;

	if (start==NULL) return NULL;
	else if (target == start->id) return start;
	else
	{
		t=start->subobj;
		while (t!=NULL && t2==NULL)
		{
			t2=locate_object(t, target);
			if (t2==NULL) t=t->next;
		}
		if (t2!=NULL) return t2;
		else return locate_object(start->next, target);
	}
}

human *locate_human(human *start, unsigned int target)
{
	while (start!=NULL)
	{
		if (start->id == target) return start;
		start=start->next;
	}
	return NULL;
}

void createbullet(unsigned char align,int x,int y,unsigned char angle, unsigned int speed, unsigned char type, unsigned char life)
{
	projectile *np;
	np = (projectile *)malloc(sizeof(projectile));
	np->next=proj;
	proj=np;
	np->align=align;
	np->x=x;
	np->y=y;
	np->angle=angle;
	np->speed=speed;
	np->type=type;
	np->life=life;
	if (np->type < 5) play_sound(5+np->type,x,y);
	else if (np->type < 8) play_sound(10,x,y);
	else if (np->type > 12) play_sound(7+np->type,x,y);
	else play_sound(11,x,y);

}

void appendangles(gameobj *go)
{
	gameobj *kid;
	if (go != NULL)
	{
		p->data[p->len]=go->angle;
		if (go->parent == NULL)
			p->data[p->len+1]=(go->align << 7) | (go->anglediff & 0x7F);
		else
			p->data[p->len+1]=(go->occupied>0 ? 0x80 : 0) | (go->anglediff & 0x7F);
		p->len+=2;
		kid = go->subobj;
		while (kid != NULL)
		{
			appendangles(kid);
			kid=kid->next;
		}
	}
}

int enterveh(human *humptr)
{
	int i;
	gameobj *start = topobj;

	while (start != NULL)
	{
		if (humptr->align == start->align && sqrt( pow(humptr->x - start->x,2) + pow(humptr->y - start->y,2)) < 160) //set some appropriate dist.
		{
			//               		fprintf (stderr, "There's a thing in range, id %d\n",start->id);
			i = seatsearch(humptr,start);
			if (i !=0) return i;
		}
		start = start->next;
	}
	//        fprintf (stderr, "There's nothing in range!\n");
	return 0;
}

int seatsearch (human *humptr, gameobj *start)
{
	int i;
	gameobj *temp;
	if (start==NULL)
	{
		//               fprintf (stderr, "No seats here.\n");
		return 0;
	} else if (start->seat && !start->occupied) {
		//               fprintf (stderr, "Got an empty seat for %d, id is %d.\n",humptr->id, start->id);
		start->occupied = humptr->id;
		driverhuman(humptr->id);
		return start->id;
	} else {
		//               fprintf (stderr, "No seats here, checking the subobjects.\n");
		temp = start->subobj;
		while (temp != NULL)
		{
			i=seatsearch(humptr,temp);
			if (i != 0) return i;
			temp=temp->next;
		}
		return 0;
	}
}

void undriverhuman(unsigned int id) // re-sorts drivers and humans
{
	human *target = drivers, *temp;
	drivers = NULL;
	//     fprintf (stderr, "undriving (%d).\n",target->id);

	while (target != NULL)
	{
		if (target->id == id)
		{
			//               fprintf (stderr, "Got a driver, undriving (%d).\n",target->id);
			temp = target->next;
			target->next = tophuman;
			tophuman = target;
			target = temp;
		} else {
			//               fprintf(stderr,"%d isn't %d.\n",target->id,id);
			temp = target->next;
			target->next = drivers;
			drivers = target;
			target = temp;
		}
	}
}

void driverhuman(unsigned int id) // sorts drivers and humans
{
	human *target = tophuman, *temp;
	tophuman = NULL;

	while (target != NULL)
	{
		if (target->id == id)
		{
			//               fprintf (stderr, "Got a driver, sorting (%d).\n",target->id);
			temp = target->next;
			target->next = drivers;
			drivers = target;
			target = temp;
		} else {
			temp = target->next;
			target->next = tophuman;
			tophuman = target;
			target = temp;
		}
	}
}

int checkcontrol (unsigned int number, char token)
{
	gameobj *obj;
	obj = locate_object(topobj, number);
	//	printf("Player got in %d, controller type %c is ", number, token);
	if (token == 'M') token = obj->mtoken;
	else if (token == 'K') token = obj->ktoken;

	//	printf("%c\n",token);
	if (token == 'N') return 0;
	if (token == 'B') return obj->id;
	if (token == 'P') return obj->parent->id;
	if (token == 'Q') return obj->parent->parent->parent->id;
	return 0;
}

void destroyvehicle(unsigned int id)
{
	// this destroys a vehicle, kills players and drivers, fixes status, etc.
	gameobj *target = topobj, *temp;
	topobj = NULL;

	while (target != NULL)
	{
		if (target->hp <=0)
		{
			play_sound(27+rand()%2,target->x,target->y);
			createexplosion(target->x,target->y,1);
        }
		if (target->id == id || target->hp <= 0)
		{
			//			printf("A type %d owned by %d blew up.\n",target->type,target->align);
			if (!multiplayer) {objtags[target->tag]--;}
			temp = target;
			target=target->next;
			srecdv(temp);
		} else {
			temp = target->next;
			target->next = topobj;
			topobj = target;
			target = temp;
		}
	}
}

int playersearch(unsigned int id)
{
	int i;
	for (i=0; i<32; i++)
		if (players[i].vehid == id) return i;
	return 32;
}

void srecdv (gameobj *veh)
{
	gameobj *sub = veh->subobj,*t;
	while (sub != NULL)
	{
		t = sub->next;
		srecdv(sub);
		sub=t;
	}
	if (veh->occupied != 0) {
		players[playersearch(veh->id)].status=0;
		undriverhuman(veh->occupied);
		killhuman(veh->occupied,0);
	}
	free(veh);
}

int humhumcollide (human *v1, human *v2)
{
	POLYGON p1;
	POLYGON p2;
	p1.NumSides = 4;
	p2.NumSides = 4;
	p1.Center.x = (double)v1->x;
	p1.Center.y = (double)v1->y;
	p2.Center.x = (double)v2->x;
	p2.Center.y = (double)v2->y;

	p1.Points[0].x = -16;
	p1.Points[0].y = 16;
	p1.Points[1].x = 16;
	p1.Points[1].y = 16;
	p1.Points[2].x = 16;
	p1.Points[2].y = -16;
	p1.Points[3].x = -16;
	p1.Points[3].y = -16;

	p2.Points[0].x = -16;
	p2.Points[0].y = 16;
	p2.Points[1].x = 16;
	p2.Points[1].y = 16;
	p2.Points[2].x = 16;
	p2.Points[2].y = -16;
	p2.Points[3].x = -16;
	p2.Points[3].y = -16;

	return polypolyCollision(&p1,&p2);
}

int humvehcollide (human *v1, gameobj *v2)
{
	POLYGON p1;
	POLYGON p2;
	double dist, w, h;
	double newangle, addangle;

	if (v2->plane == 1 && v2->speed > 30) return 0;
	p1.NumSides = 4;
	p2.NumSides = 4;

	p1.Center.x = v1->x;
	p1.Center.y = v1->y;
	p2.Center.x = v2->x;
	p2.Center.y = v2->y;

	p1.Points[0].x = -16;
	p1.Points[0].y = 16;
	p1.Points[1].x = 16;
	p1.Points[1].y = 16;
	p1.Points[2].x = 16;
	p1.Points[2].y = -16;
	p1.Points[3].x = -16;
	p1.Points[3].y = -16;

	w = (double) v2->width / 2;
	h = (double) v2->height / 2;
	dist = sqrt(w*w+h*h);
	if (v2->plane == 1)
		addangle = (double)(v2->angle + 32) * 0.02463994235294;
	else
		addangle = (double)v2->angle * 0.02463994235294;

	newangle = atan2(h,-w) + addangle;
	p2.Points[0].x = dist * cos(newangle);
	p2.Points[0].y = dist * sin(newangle);
	newangle = atan2(h,w) + addangle;
	p2.Points[1].x = dist * cos(newangle);
	p2.Points[1].y = dist * sin(newangle);
	newangle = atan2(-h,w) + addangle;
	p2.Points[2].x = dist * cos(newangle);
	p2.Points[2].y = dist * sin(newangle);
	newangle = atan2(-h,-w) + addangle;
	p2.Points[3].x = dist * cos(newangle);
	p2.Points[3].y = dist * sin(newangle);

	return polypolyCollision(&p1,&p2);
}

int bldhumcollide (int id, human *v2)
{
	POLYGON p1;
	POLYGON p2;
	p1.NumSides = 4;
	p2.NumSides = 4;
	p1.Center.x = bldloc[id][1] * 32;
	p1.Center.y = bldloc[id][2] * 32;
	p2.Center.x = (double)v2->x;
	p2.Center.y = (double)v2->y;

	p1.Points[0].x = -sbldw[bldloc[id][0]];
	p1.Points[0].y = sbldh[bldloc[id][0]];
	p1.Points[1].x = sbldw[bldloc[id][0]];
	p1.Points[1].y = sbldh[bldloc[id][0]];
	p1.Points[2].x = sbldw[bldloc[id][0]];
	p1.Points[2].y = -sbldh[bldloc[id][0]];
	p1.Points[3].x = -sbldw[bldloc[id][0]];
	p1.Points[3].y = -sbldh[bldloc[id][0]];

	p2.Points[0].x = -16;
	p2.Points[0].y = 16;
	p2.Points[1].x = 16;
	p2.Points[1].y = 16;
	p2.Points[2].x = 16;
	p2.Points[2].y = -16;
	p2.Points[3].x = -16;
	p2.Points[3].y = -16;

	return polypolyCollision(&p1,&p2);
}

int bldvehcollide (int id, gameobj *v2)
{
	POLYGON p1;
	POLYGON p2;
	double dist, w, h;
	double newangle, addangle;
	if (v2->plane == 1 && v2->speed > 30) return 0;

	p1.NumSides = 4;
	p2.NumSides = 4;

	p1.Center.x = bldloc[id][1] * 32;
	p1.Center.y = bldloc[id][2] * 32;
	p2.Center.x = v2->x;
	p2.Center.y = v2->y;

	p1.Points[0].x = -sbldw[bldloc[id][0]];
	p1.Points[0].y = sbldh[bldloc[id][0]];
	p1.Points[1].x = sbldw[bldloc[id][0]];
	p1.Points[1].y = sbldh[bldloc[id][0]];
	p1.Points[2].x = sbldw[bldloc[id][0]];
	p1.Points[2].y = -sbldh[bldloc[id][0]];
	p1.Points[3].x = -sbldw[bldloc[id][0]];
	p1.Points[3].y = -sbldh[bldloc[id][0]];

	w = (double) v2->width / 2;
	h = (double) v2->height / 2;
	dist = sqrt(w*w+h*h);
	if (v2->plane == 1)
		addangle = (double)(v2->angle + 32) * 0.02463994235294;
	else
		addangle = (double)v2->angle * 0.02463994235294;

	newangle = atan2(h,-w) + addangle;
	p2.Points[0].x = dist * cos(newangle);
	p2.Points[0].y = dist * sin(newangle);
	newangle = atan2(h,w) + addangle;
	p2.Points[1].x = dist * cos(newangle);
	p2.Points[1].y = dist * sin(newangle);
	newangle = atan2(-h,w) + addangle;
	p2.Points[2].x = dist * cos(newangle);
	p2.Points[2].y = dist * sin(newangle);
	newangle = atan2(-h,-w) + addangle;
	p2.Points[3].x = dist * cos(newangle);
	p2.Points[3].y = dist * sin(newangle);

	return polypolyCollision(&p1,&p2);
}

int vehvehcollide (gameobj *v1, gameobj *v2)
{
	POLYGON p1;
	POLYGON p2;
	double dist, w, h;
	double newangle, addangle;

	if (v1->plane == 1 && v1->speed > 30 && (v2->plane == 0 || v2->speed <= 30)) return 0;
	if (v2->plane == 1 && v2->speed > 30 && (v1->plane == 0 || v1->speed <= 30)) return 0;

	p1.NumSides = 4;
	p2.NumSides = 4;

	p1.Center.x = v1->x;
	p1.Center.y = v1->y;
	p2.Center.x = v2->x;
	p2.Center.y = v2->y;

	w = (double) v1->width / 2;
	h = (double) v1->height / 2;
	dist = sqrt(w*w+h*h);
	if (v1->plane == 1)
		addangle = (double)(v1->angle + 32) * 0.02463994235294;
	else
		addangle = (double)v1->angle * 0.02463994235294;

	newangle = atan2(h,-w) + addangle;
	p1.Points[0].x = dist * cos(newangle);
	p1.Points[0].y = dist * sin(newangle);
	newangle = atan2(h,w) + addangle;
	p1.Points[1].x = dist * cos(newangle);
	p1.Points[1].y = dist * sin(newangle);
	newangle = atan2(-h,w) + addangle;
	p1.Points[2].x = dist * cos(newangle);
	p1.Points[2].y = dist * sin(newangle);
	newangle = atan2(-h,-w) + addangle;
	p1.Points[3].x = dist * cos(newangle);
	p1.Points[3].y = dist * sin(newangle);

	w = (double) v2->width / 2;
	h = (double) v2->height / 2;
	dist = sqrt(w*w+h*h);
	if (v2->plane == 1)
		addangle = (double)(v2->angle + 32) * 0.02463994235294;
	else
		addangle = (double)v2->angle * 0.02463994235294;

	newangle = atan2(h,-w) + addangle;
	p2.Points[0].x = dist * cos(newangle);
	p2.Points[0].y = dist * sin(newangle);
	newangle = atan2(h,w) + addangle;
	p2.Points[1].x = dist * cos(newangle);
	p2.Points[1].y = dist * sin(newangle);
	newangle = atan2(-h,w) + addangle;
	p2.Points[2].x = dist * cos(newangle);
	p2.Points[2].y = dist * sin(newangle);
	newangle = atan2(-h,-w) + addangle;
	p2.Points[3].x = dist * cos(newangle);
	p2.Points[3].y = dist * sin(newangle);

	return polypolyCollision(&p1,&p2);
}

int bulvehcollide (gameobj *v1, int oldx, int oldy, int newx, int newy, int type)
{
	POLYGON p1;
	POLYGON p2;
	double dist, w, h;
	double newangle, addangle;

	if (v1->plane == 1 && v1->speed > 30 && (type == 1 || type == 2 || type == 3 || type == 4 ||
				type == 5 || type == 13 || type == 15 || type == 16)) return 0;

	//	if (v1->plane == 1 && v1->speed > 30) return 0;

	p1.NumSides = 4;
	p2.NumSides = 2;

	p1.Center.x = v1->x;
	p1.Center.y = v1->y;
	p2.Center.x = oldx;
	p2.Center.y = oldy;

	w = (double) v1->width / 2;
	h = (double) v1->height / 2;
	dist = sqrt(w*w+h*h);
	if (v1->plane == 1)
		addangle = (double)(v1->angle + 32) * 0.02463994235294;
	else
		addangle = (double)v1->angle * 0.02463994235294;

	newangle = atan2(h,-w) + addangle;
	p1.Points[0].x = dist * cos(newangle);
	p1.Points[0].y = dist * sin(newangle);
	newangle = atan2(h,w) + addangle;
	p1.Points[1].x = dist * cos(newangle);
	p1.Points[1].y = dist * sin(newangle);
	newangle = atan2(-h,w) + addangle;
	p1.Points[2].x = dist * cos(newangle);
	p1.Points[2].y = dist * sin(newangle);
	newangle = atan2(-h,-w) + addangle;
	p1.Points[3].x = dist * cos(newangle);
	p1.Points[3].y = dist * sin(newangle);

	p2.Points[0].x = 0;
	p2.Points[0].y = 0;
	p2.Points[1].x = newx-oldx;
	p2.Points[1].y = newy-oldy;

	return polypolyCollision(&p1,&p2);
}

int bulhumcollide (human *v1, int oldx, int oldy, int newx, int newy)
{
	POLYGON p1;
	POLYGON p2;
	//	double newangle, addangle;

	p1.NumSides = 4;
	p2.NumSides = 2;

	p1.Center.x = v1->x;
	p1.Center.y = v1->y;
	p2.Center.x = oldx;
	p2.Center.y = oldy;

	p1.Points[0].x = -16;
	p1.Points[0].y = 16;
	p1.Points[1].x = 16;
	p1.Points[1].y = 16;
	p1.Points[2].x = 16;
	p1.Points[2].y = -16;
	p1.Points[3].x = -16;
	p1.Points[3].y = -16;

	//	newangle = atan2(newy-oldy,newx-oldx);

	p2.Points[0].x = 0;
	p2.Points[0].y = 0;
	//	p2.Points[1].x = 4 * cos(newangle - 90);
	//	p2.Points[1].y = 4 * sin(newangle - 90);
	p2.Points[1].x = newx-oldx;
	p2.Points[1].y = newy-oldy;

	/*	if (newx - oldx == 0 && newy - oldy == 0)
		{
		p2.Points[1].x = 4;
		}*/

	return polypolyCollision(&p1,&p2);
}


/* Tests for a collision between any two convex polygons.
 * Returns 0 if they do not intersect
 * Returns 1 if they intersect
 */
int polypolyCollision(POLYGON *a, POLYGON *b)
{
	GPOINT axis;
	double tmp, minA, maxA, minB, maxB;
	int side, i;

	axis.x=0;
	axis.y=0;

	/*	if (b->NumSides == 2)
		{
		fprintf(stderr,"(%lf,%lf)-(%lf,%lf)\n",b->Points[0].x,
		b->Points[0].y,b->Points[1].x,b->Points[1].y);
		}*/

	/* test polygon A's sides */
	for (side = 0; side < a->NumSides; side++)
	{
		/* get the axis that we will project onto */
		if (side == 0)
		{
			axis.x = a->Points[a->NumSides - 1].y - a->Points[0].y;
			axis.y = a->Points[0].x - a->Points[a->NumSides - 1].x;
		}
		else
		{
			axis.x = a->Points[side - 1].y - a->Points[side].y;
			axis.y = a->Points[side].x - a->Points[side - 1].x;
		}

		/* normalize the axis */
		tmp = sqrt(axis.x * axis.x + axis.y * axis.y);
		axis.x /= tmp;
		axis.y /= tmp;

		/* project polygon A onto axis to determine the min/max */
		minA = maxA = a->Points[0].x * axis.x + a->Points[0].y * axis.y;
		for (i = 1; i < a->NumSides; i++)
		{
			tmp = a->Points[i].x * axis.x + a->Points[i].y * axis.y;
			if (tmp > maxA)
				maxA = tmp;
			else if (tmp < minA)
				minA = tmp;
		}
		/* correct for offset */
		tmp = a->Center.x * axis.x + a->Center.y * axis.y;
		minA += tmp;
		maxA += tmp;

		/* project polygon B onto axis to determine the min/max */
		minB = maxB = b->Points[0].x * axis.x + b->Points[0].y * axis.y;
		for (i = 1; i < b->NumSides; i++)
		{
			tmp = b->Points[i].x * axis.x + b->Points[i].y * axis.y;
			if (tmp > maxB)
				maxB = tmp;
			else if (tmp < minB)
				minB = tmp;
		}
		/* correct for offset */
		tmp = b->Center.x * axis.x + b->Center.y * axis.y;
		minB += tmp;
		maxB += tmp;

		/* test if lines intersect, if not, return false */
		if (maxA < minB || minA > maxB)
			return 0;
	}

	/* test polygon B's sides */
	for (side = 0; side < b->NumSides; side++)
	{
		/* get the axis that we will project onto */
		if (side == 0)
		{
			axis.x = b->Points[b->NumSides - 1].y - b->Points[0].y;
			axis.y = b->Points[0].x - b->Points[b->NumSides - 1].x;
		}
		else
		{
			axis.x = b->Points[side - 1].y - b->Points[side].y;
			axis.y = b->Points[side].x - b->Points[side - 1].x;
		}

		/* normalize the axis */
		tmp = sqrt(axis.x * axis.x + axis.y * axis.y);
		axis.x /= tmp;
		axis.y /= tmp;

		/* project polygon A onto axis to determine the min/max */
		minA = maxA = a->Points[0].x * axis.x + a->Points[0].y * axis.y;
		for (i = 1; i < a->NumSides; i++)
		{
			tmp = a->Points[i].x * axis.x + a->Points[i].y * axis.y;
			if (tmp > maxA)
				maxA = tmp;
			else if (tmp < minA)
				minA = tmp;
		}
		/* correct for offset */
		tmp = a->Center.x * axis.x + a->Center.y * axis.y;
		minA += tmp;
		maxA += tmp;

		/* project polygon B onto axis to determine the min/max */
		minB = maxB = b->Points[0].x * axis.x + b->Points[0].y * axis.y;
		for (i = 1; i < b->NumSides; i++)
		{
			tmp = b->Points[i].x * axis.x + b->Points[i].y * axis.y;
			if (tmp > maxB)
				maxB = tmp;
			else if (tmp < minB)
				minB = tmp;
		}
		/* correct for offset */
		tmp = b->Center.x * axis.x + b->Center.y * axis.y;
		minB += tmp;
		maxB += tmp;

		/* test if lines intersect, if not, return false */
		if (maxA < minB || minA > maxB)
			return 0;
	}

	return 1;
}

void aithink()
{
	int i;
	human *humptr, *h2;
	gameobj *objptr, *o2;

	float a,b,d,e,f,x,y;
	unsigned char c;

	float tx, ty,tangle,dist;

	for (i=0; i<2; i++)
	{
		if (level == 0)
		{
            c = rand() % 2; if (c == 0) humptr = locate_human(tophuman, players[0].controller_k); else
   			humptr = locate_human(tophuman, players[i + 2 * (rand() % 16)].controller_k);
			if (humptr == NULL) c = 10;
			else if (humptr->health < 20) c = 0;
			else if (humptr->carriedarms != 3 && humptr->ammo < 1) c = 1;
			else if (humptr->carriedarms == 3) c = rand() % 4 + 2;
			else c = 10;
			if (c != 10 && cash [i] > 400) {
				createbullet(humptr->align,(int)(humptr->x + (humptr->speed)*sintable[humptr->angle]),(int)(humptr->y - costable[humptr->angle] * humptr->speed),0,0,c + 6,255);
				cash[i] -= prices[c];
			}

			// LEVEL 1 AI
		} else if (level == 1 && cash[i] > 400) {
			c = rand() % 3;
			if (c == 0) // equip a human
			{
                  c = rand() % 2; if (c == 0) humptr = locate_human(tophuman, players[0].controller_k); else
				humptr = locate_human(tophuman, players[i + 2 * (rand() % 16)].controller_k);
				if (humptr == NULL) c = 10;
				else if (humptr->health < 20) c = 0;
				else if (humptr->carriedarms != 3 && humptr->ammo < 1) c = 1;
				else if (humptr->carriedarms == 3) c = rand() % 4 + 2;
				else c = 10;
				if (c != 10) {
					createbullet(humptr->align,(int)(humptr->x + (humptr->speed)*sintable[humptr->angle]),(int)(humptr->y - costable[humptr->angle] * humptr->speed),0,0,c + 6,255);
					cash[i] -= prices[c];
				}
			} else if (c == 1 && objtags[5*i] < 3) { // make a drivable
				e = rand() % 3;
				objptr = createobject(hqobjects[(int)e],1,1, i,sobjidnum);
				objtags[5*i]++;
				objptr->tag=5*i;
				f = 0;
				while (f != 1)
				{
					f = 1;
					if (i == 0) {
						objptr->x = (int)max(min(bldloc[3][1]*32 + 50 + (rand() % 500),3199),0);
						objptr->y = (int)max(min(bldloc[3][2]*32 + 50 + (rand() % 500),3199),0);
					} else {
						objptr->x = (int)max(min(bldloc[4][1]*32 - 50 - (rand() % 500),3199),0);
						objptr->y = (int)max(min(bldloc[4][2]*32 - 50 - (rand() % 500),3199),0);
					}
					o2 = topobj;
					while (o2 != NULL)
					{
						if (objptr != o2 && vehvehcollide(objptr,o2)){f = 0; o2 = NULL;} else
							o2=o2->next;
					}
				}
				cash[i] -= prices[(int)e+7];
			} else if (c == 2) { // make a turret
				e = rand() % 3;
				if (bldloc[(int)e][0] == 8+i && objtags[5*i+1+(int)e] < 2) { // only attempt to defend held points
					a = max(min(bldloc[(int)e][1]*32 - 100 + (rand() % 200),3199),0);
					objptr=createobject(hqobjects[(rand()%2)+3],(int)a,(int)(3199-a), i,sobjidnum);
					objtags[5*i+1+(int)e]++;
					objptr->tag=5*i+1+(int)e;
					cash[i] -= prices[10];
				}
			}

			// LEVEL 2 AI
		} else if (level == 2 && cash[i] > 600) {
			c = rand() % 3;
			if (c == 0) // equip a human
			{
                  c = rand() % 2; if (c == 0) humptr = locate_human(tophuman, players[0].controller_k); else
                  humptr = locate_human(tophuman, players[i + 2 * (rand() % 16)].controller_k);
				if (humptr == NULL) c = 10;
				else if (humptr->health < 20) c = 0;
				else if (humptr->carriedarms != 3 && humptr->ammo < 1) c = 1;
				else if (humptr->carriedarms == 3) c = rand() % 4 + 2;
				else c = 10;
				if (c != 10) {
					createbullet(humptr->align,(int)(humptr->x + (humptr->speed)*sintable[humptr->angle]),(int)(humptr->y - costable[humptr->angle] * humptr->speed),0,0,c + 6,255);
					cash[i] -= prices[c];
				}
			} else if (c == 1 && objtags[5*i] < 1) { // make a ship
				e = rand() % 3;
				objptr = createobject(hqobjects[(int)e+5],1,1, i,sobjidnum);
				objptr->tag = 5*i;
				f = 0;
				while (f != 1)
				{
					f = 1;
					if (i == 0) {
						objptr->x = (int)max(min(bldloc[3][1]*32 + 500 + (rand() % 500),3199),0);
						objptr->y = (int)max(min(bldloc[3][2]*32 - 500 - (rand() % 500),3199),0);
					} else {
						objptr->x = (int)max(min(bldloc[4][1]*32 - 500 - (rand() % 500),3199),0);
						objptr->y = (int)max(min(bldloc[4][2]*32 + 500 + (rand() % 500),3199),0);
					}
					o2 = topobj;
					while (o2 != NULL)
					{
						if (objptr != o2 && vehvehcollide(objptr,o2)){f = 0; o2 = NULL;} else
							o2=o2->next;
					}
				}
				objtags[5*i]++;
				cash[i] -= prices[(int)e+5];
			} else if (c == 2 && objtags[5*i+1] < 2) { // make a plane
				e = rand() % 2 + 8;
				objptr = createobject(hqobjects[(int)e],1,1, i,sobjidnum);
				objptr->tag = 5*i+1;
				f = 0;
				while (f != 1)
				{
					f = 1;
					if (i == 0) {
						objptr->x = (int)max(min(bldloc[3][1]*32 - 400 + (rand() % 600),3199),0);
						objptr->y = (int)max(min(bldloc[3][2]*32 - 100 - (rand() % 200),3199),0);
					} else {
						objptr->x = (int)max(min(bldloc[4][1]*32 - 400 + (rand() % 600),3199),0);
						objptr->y = (int)max(min(bldloc[4][2]*32 + 100 + (rand() % 200),3199),0);
					}
					if (map[objptr->x/32][objptr->y/32] == 0 || trees[objptr->x/32][objptr->y/32] != 0) f=0;
					o2 = topobj;
					while (o2 != NULL)
					{
						if (objptr != o2 && vehvehcollide(objptr,o2)){f = 0; o2 = NULL;} else
							o2=o2->next;
					}
				}
				objtags[5*i+1]++;
				cash[i] -= prices[(int)e];
			}

			// LEVEL 3 AI

		} else if (level == 3 && i == 0) {
			humptr = locate_human(tophuman, players[i + 2 * (rand() % 16)].controller_k);
			if (humptr == NULL) c = 10;
			else if (humptr->health < 20) c = 0;
			else if (humptr->carriedarms != 3 && humptr->ammo < 1) c = 1;
			else if (humptr->carriedarms == 3) c = rand() % 3 + 2;
			else c = 10;
			if (c != 10 && cash [i] > 400) {
				createbullet(i,(int)(humptr->x + (humptr->speed)*sintable[humptr->angle]),(int)(humptr->y - costable[humptr->angle] * humptr->speed),0,0,c + 6,255);
				cash[i] -= prices[c];
			}
		}
	}

	for (i=1; i<32; i++)
	{
		if (bldloc[players[i].reg[0]][0] == 8 + (i % 2))  { players[i].reg[0] = rand() % 3;}

		if (players[i].status == 0)
		{
			players[i].reg[0] = rand()%3; // go to a target building.
			players[i].reg[2] = 0; // sail target
		}
		else if (players[i].status == 1)
		{
			players[i].controls[6]=0;
			humptr = locate_human(tophuman,players[i].controller_k);
			if (humptr != NULL) {
				a = humptr->x - (bldloc[players[i].reg[0]][1] * 32);
				b = humptr->y - (bldloc[players[i].reg[0]][2] * 32);
				c = (unsigned char)(192 + (atan2(b,a) * 256 / 6.28));
				if (abs((int)humptr->angle - (int)c) < 10 || (abs((int)humptr->angle - (int)c) > 245))
				{
					players[i].controls[0] = 0;
					players[i].controls[1] = 0;
				} else if (leftturn((int)humptr->angle,(int)c))
				{
					players[i].controls[0] = 1;
					players[i].controls[1] = 0;
				} else {
					players[i].controls[0] = 0;
					players[i].controls[1] = 1;
				}

				if (sqrt(a * a + b * b) > 50) {
					players[i].controls[2] = 1;
					players[i].controls[3] = 0;
				} else {
					players[i].controls[2] = 0;
					players[i].controls[3] = 0;
				}

				if (sqrt(a * a + b * b) > 200 || bldloc[players[i].reg[0]][0] == 8+(i % 2))
					players[i].controls[6] = 1;

				h2 = tophuman;
				d=800;
				while (h2 != NULL)
				{
					if (h2->align != humptr->align && h2 != humptr) {
						a = humptr->x - h2->x;
						b = humptr->y - h2->y;
						e = sqrt((a * a) + (b * b));
						if (e < d)
						{
							d = e;
							c = (unsigned char)(192 + (atan2(b,a) * 256 / 6.28));
						}
					}
					h2=h2->next;
				}

				if (humptr->weapon == 4 || humptr->weapon == 7 || d > 400) {
					objptr = topobj;
					while (objptr != NULL)
					{
						if (objptr->align != humptr->align) {
							a = humptr->x - objptr->x;
							b = humptr->y - objptr->y;
							e = sqrt((a * a) + (b * b));
							if (e < d)
							{
								d = e;
								c = (unsigned char)(192 + (atan2(b,a) * 256 / 6.28));
							}
						}
						objptr=objptr->next;
					}
				}

				players[i].controls[8] = c;
				if (level == 3 && i % 2 == 1)
				{
					humptr->weapon = 2;
					humptr->carriedarms = 2;
					players[i].controls[4] = 0;
				} else {
					if (d <= 400) players[i].controls[4] = 1;
					else if (humptr->weapon == 5 && d <= 600) players[i].controls[4] = 1;
					else players[i].controls[4] = 0;
				}

				if (humptr->ammo != 0 && humptr->weapon != humptr->carriedarms)
					players[i].controls[7] = 1;
				else if (humptr->ammo == 0 && humptr->weapon == humptr->carriedarms)
					players[i].controls[7] = 1;
				else
					players[i].controls[7] = 0;
			} else {
				players[i].reg[0] = rand()%3; // go to a target building.
			}
		} else if (players[i].status == 2) {

			players[i].controls[6] = 0;
			players[i].controls[7] = 0;

			objptr = locate_object(topobj, players[i].controller_k);
			if (objptr != NULL) {
				if (objptr->waterborne)
				{
					if (players[i].reg[2] == 0) {
						if (objptr->y < 1600) {
							if (objptr->x < 1600) {
								players[i].reg[3] = 2400;
								players[i].reg[4] = 800;
							}else {
								players[i].reg[3] = 2400;
								players[i].reg[4] = 2400;
							}
						} else {
							if (objptr->x < 1600) {
								players[i].reg[3] = 800;
								players[i].reg[4] = 800;
							}else {
								players[i].reg[3] = 800;
								players[i].reg[4] = 2400;
							}
						}
						players[i].reg[2]=1;
					}
					a = objptr->x - players[i].reg[3];
					b = objptr->y - players[i].reg[4];
					c = (unsigned char)(192 + (atan2(b,a) * 256 / 6.28318531));
					if (abs((int)objptr->angle - (int)c) < objptr->maxang || (abs((int)objptr->angle - (int)c) > (255 - objptr->maxang)))
					{
						players[i].controls[0] = 0;
						players[i].controls[1] = 0;
					} else if (leftturn((int)objptr->angle,(int)c))
					{
						players[i].controls[0] = 1;
						players[i].controls[1] = 0;
					} else {
						players[i].controls[0] = 0;
						players[i].controls[1] = 1;
					}

					if (sqrt(a * a + b * b) <= 200)
						players[i].reg[2] = 0;

                    if (objptr->speed < 5 || objptr->type == 16 || ((objptr->x < 1800 && objptr->y < 1800) || (objptr->x > 1400 && objptr->y > 1400)))
					players[i].controls[2] = 1; else players[i].controls[2]=0;
					players[i].controls[3] = 0;

				} else if (objptr->plane) {
					players[i].controls[0] = 0;
					players[i].controls[1] = 0;
					players[i].controls[2] = 1;
					players[i].controls[3] = 0;
				} else {

					a = objptr->x - (bldloc[players[i].reg[0]][1] * 32);
					b = objptr->y - (bldloc[players[i].reg[0]][2] * 32);
					c = (unsigned char)(192 + (atan2(b,a) * 256 / 6.28318531));
					if (abs((int)objptr->angle - (int)c) < objptr->maxang || (abs((int)objptr->angle - (int)c) > (255 - objptr->maxang)))
					{
						players[i].controls[0] = 0;
						players[i].controls[1] = 0;
					} else if (leftturn((int)objptr->angle,(int)c))
					{
						players[i].controls[0] = 1;
						players[i].controls[1] = 0;
					} else {
						players[i].controls[0] = 0;
						players[i].controls[1] = 1;
					}

					if (objptr->speed == 0 && sqrt(a * a + b * b) <= 200 && bldloc[players[i].reg[0]][0] != 8+(i % 2)) {
						players[i].controls[6] = 1;
					} else if (sqrt(a * a + b * b) > 200) {
						players[i].controls[2] = 1;
						players[i].controls[3] = 0;
					} else if (objptr->speed > objptr->accel) {
						players[i].controls[2] = 0;
						players[i].controls[3] = 1;
					} else {
						players[i].controls[2] = 0;
						players[i].controls[3] = 0;
					}
				}

				o2 = topobj;
				while (o2 != NULL && objptr->plane != 1)
				{
					if (o2 != objptr)
					{
						a = objptr->x - o2->x;
						b = objptr->y - o2->y;
						c = (unsigned char)(192 + (atan2(b,a) * 256 / 6.28318531));
						e = sqrt((a*a)+(b*b));

						if (e < 300 && (abs((int)objptr->angle - (int)c) < (2*objptr->maxang) || (abs((int)objptr->angle - (int)c) > (255 - (2*objptr->maxang))))) {
							players[i].controls[0]=1;
							players[i].controls[1]=0;
							players[i].controls[2]=0;
							players[i].controls[3]=1;
						}
					}
					o2=o2->next;
				}
			}
			objptr = locate_object(topobj, players[i].controller_m);
			if (objptr != NULL) {
				if (objptr->plane == 1 && objptr->speed <= 30)
					players[i].controls[8] = 128 * objptr->align;
				else {
					x=0; y=0; o2 = objptr;

					while (o2 != NULL)
					{
						tangle = atan2(y,x) + (((double)o2->angle - (double)o2->anglediff * rate) * 0.02463994235294);
						dist = sqrt(x*x+y*y);

						tx = dist * cos(tangle);
						ty = dist * sin(tangle);
						if (o2->parent != NULL)
						{
							tx = tx + (float)o2->parent->xoff;
							ty = ty + (float)o2->parent->yoff;
						}
						x = (o2->zoom) * tx + (float)o2->x - ((float)o2->speed*sintable[o2->angle] * rate);
						y = (o2->zoom) * ty + (float)o2->y - ((float)o2->speed*-costable[o2->angle] * rate);
						o2 = o2->parent;
					}

					d=800;
					if (level == 1) {
						c=96 + (128 * (i % 2));
					} else {
						c=32 + (128 * (i % 2));
					}

					h2 = tophuman;
					while (h2 != NULL)
					{
						if (h2->align != objptr->align) {
							a = x - h2->x;
							b = y - h2->y;
							e = sqrt((a * a) + (b * b));
							if (e < d)
							{
								d = e;
								c = (unsigned char)(192 + (atan2(b,a) * 256 / 6.28318531));
							}
						}
						h2=h2->next;
					}

					o2 = topobj;
					while (o2 != NULL)
					{
						if (o2->align != objptr->align) {
							a = x - o2->x;
							b = y - o2->y;
							e = sqrt((a * a) + (b * b));
							if (e < d)
							{
								d = e;
                                if (objptr->plane == 0 || o2->plane == 0 || e > 300)
								  c = (unsigned char)(192 + (atan2(b,a) * 256 / 6.28318531));
								else
								  c = (unsigned char)(atan2(b,a) * 256 / 6.28318531);
							}
						}
						o2=o2->next;
					}
					
					o2=objptr;
					while (o2->parent != NULL) o2=o2->parent;
					players[i].controls[8] = c;
					if (d <= 400) players[i].controls[4] = 1;
					else if (o2->waterborne == 1 && d <= 600) players[i].controls[4] = 1;
					else players[i].controls[4] = 0;
				}
			} else {
				o2 = locate_object(topobj, players[i].controller_k);
				if (o2 == NULL) {
					objptr = locate_object(topobj, players[i].vehid);
					if (objptr != NULL) {
						while (objptr->parent != NULL)
							objptr = objptr->parent;
						if (objptr->speed == 0) players[i].controls[6]=1;
						else players[i].controls[6]=0;

						if (level == 2 && (i % 2) == 0 && objptr->x>2000 && objptr->y < 1200) players[i].controls[6]=1;
						if (level == 2 && i % 2 && objptr->x<1200 && objptr->y < 2000) players[i].controls[6]=1;
					}
				}
			}
		}
	}
}

int leftturn(int a, int t)
{
	if (a < t) a+=256;
	return (a - t < 128);
}
