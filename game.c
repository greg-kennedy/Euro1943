/* game.c - contains the actual game code */

#include "game.h"

// messagebox (speech bubble) used on this screen
#include "message.h"

#include "texops.h"

// standard math functions
#include <math.h>

// game logics
#include "game_logic.h"

/* Loadable objects */
#define NUM_OBJECTS 30
#define NUM_BLDGS 12
#define NUM_HUMANS 18
#define NUM_TILES 16
#define NUM_BULLETS 17
#define NUM_PARTICLES 10
#define NUM_SFX 31

// got some externs here
extern int vol_sfx, gamestate;
extern long mx, my;
extern GLuint list_cursor;

// state variables used at init-time to figure out where to connect to the brain
//  multiplayer or singleplayer?
int multiplayer;
//  single-player important init elements
int level;
//  on the other hand, these are defined for the multiplayer
char HOSTNAME[80];

UDPsocket sd;
IPaddress srvadd;
UDPpacket *p;

// OK !   Key game components go here.
// loads of sound effects.
Mix_Chunk *sfx[NUM_SFX];

// GL texture objects
GLuint tex_object[ NUM_OBJECTS ];
GLuint tex_bldg[ NUM_BLDGS ];
GLuint tex_human[ NUM_HUMANS ];
GLuint tex_tile[ NUM_TILES+4 ];
GLuint tex_bullet[ NUM_BULLETS ];
GLuint tex_particle[ NUM_PARTICLES ];
GLuint tex_hud[ 7 ];
GLuint tex_digit[10];
GLuint tex_hqmenu;
GLuint tex_minimap;
GLuint tex_dir;
//GLuint clouds;

// However those textures aren't usually used directly
//  Call the appropriate display-list instead to draw object of choice
GLuint list_object[ NUM_OBJECTS ];
GLuint list_bldg[ NUM_BLDGS ];
GLuint list_human_bottom[ 2 ];
GLuint list_human_top[ NUM_HUMANS ];
GLuint list_submap[ MAP_X / 10 ][ MAP_Y / 10 ];
GLuint list_bullet[ NUM_BULLETS ];
GLuint list_particle[ NUM_PARTICLES ];
GLuint list_hud_no_hq;
GLuint list_hud_hq;
GLuint list_digit[10];

// gotta know your FPS...
int frames;
unsigned long fpsticks;
int smooth[5]={UPDATEFREQ,UPDATEFREQ,UPDATEFREQ,UPDATEFREQ,UPDATEFREQ};

//some stuff for drawing the map
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

float lerp (int val_a, int val_b, unsigned int time_a, unsigned int time_b, unsigned int time_t)
{
	if (time_t < time_a) return val_a;
	else if (time_t > time_b) return val_b;
	else
	{
		float increment = (float)(val_b - val_a) / (time_b - time_a);

		return (increment * (time_b - time_t)) + val_a;
	}
}

static void game_load()
{
	int i;
	char buffer[80];

	srand(time(NULL));

	glGenTextures( 7, tex_hud );
	glGenTextures( NUM_OBJECTS, tex_object );
	glGenTextures( NUM_BLDGS, tex_bldg );
	glGenTextures( NUM_TILES+4, tex_tile );
	glGenTextures( NUM_HUMANS, tex_human );
	glGenTextures( NUM_BULLETS, tex_bullet );
	glGenTextures( NUM_PARTICLES, tex_particle );
	glGenTextures( 10, tex_digit );
	glGenTextures( 1, &tex_hqmenu );

	tex_hqmenu = load_texture("img/hud/hqmenu.png",GL_LINEAR,GL_LINEAR);
/*	texthqmenu(9,150,"  5 Health Kit",surf);
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
	texthqmenu(9,350,"250 Bomber",surf); */

	glBindTexture( GL_TEXTURE_2D, tex_minimap );
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glBindTexture( GL_TEXTURE_2D, tex_hud[4] );
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glBindTexture( GL_TEXTURE_2D, tex_hud[5] );
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	tex_dir = load_texture("img/ui/dir.png",GL_NEAREST,GL_NEAREST);

	for (i=0; i<10; i++)
	{
		sprintf(buffer,"img/ui/n%d.png",i);
		tex_digit[i] = load_texture(buffer,GL_NEAREST,GL_NEAREST);
	}

	for (i=0; i<4; i++)
	{
		sprintf(buffer,"img/hud/%d.png",i);
		tex_hud[i] = load_texture(buffer,GL_NEAREST,GL_NEAREST);
	}

	for (i=0; i<NUM_PARTICLES; i++)
	{
		sprintf(buffer,"img/particle/%d.png",i);
		tex_particle[i] = load_texture(buffer,GL_NEAREST,GL_NEAREST);
	}

	for (i=0; i<NUM_TILES+4; i++)
	{
		sprintf(buffer,"img/tiles/%d.png",i);
		tex_tile[i] = load_texture(buffer,GL_NEAREST,GL_NEAREST);
	}

	for (i=0; i<NUM_BULLETS; i++)
	{
		sprintf(buffer,"img/bul/%d.png",i);
		tex_bullet[i] = load_texture(buffer,GL_NEAREST,GL_NEAREST);

/*		bulw[i]=surf->w;
		bulh[i]=surf->h; */
	}

	for (i=0; i<NUM_OBJECTS; i++)
	{
		sprintf(buffer,"img/gfx/%d.png",i);
		tex_object[i] = load_texture(buffer,GL_NEAREST,GL_NEAREST);

/*		objw[i]=surf->w;
		objh[i]=surf->h; */
	}

	for (i=0; i<NUM_BLDGS; i++)
	{
		sprintf(buffer,"img/bldg/%d.png",i);
		tex_bldg[i] = load_texture(buffer,GL_NEAREST,GL_NEAREST);

/*		bldw[i]=surf->w;
		bldh[i]=surf->h; */
	}

	for (i=0; i<NUM_HUMANS; i++)
	{
		sprintf(buffer,"img/humgfx/%d.png",i);
		tex_human[i] = load_texture(buffer,GL_NEAREST,GL_NEAREST);
	}

	// Create display lists for all the items we will have.




	// Load sound effects.
	for (i=0; i<NUM_SFX; i++)
	{
		sprintf(buffer,"audio/sfx/%d.wav",i);
		sfx[i] = Mix_LoadWAV(buffer);
		if (!sfx[i]) fprintf(stderr,"Error: couldn't load WAV file %s: %s\n",buffer,Mix_GetError());
	}

//	showmoney = 0;
	/*iSeeAll = 0;

	frames = 0;
	fpsticks = SDL_GetTicks();
	camx = 400;
	camy = 300;
	oldzoom = 0;
	newzoom = 0;

	ticks=SDL_GetTicks();
	curScripticks = 0;
	curScriptloc = 0;
	controlticks = 0;

	for (i=0; i<9; i++)
		localcontrols[i]=0; */

	message_clear();
}

static void game_connect()
{
	int i;
	char buffer[80];

	glGenTextures( 1, &tex_minimap );

	if (multiplayer)
	{
		// connect to remote server
        unsigned short int PORT=DEFAULT_PORT;

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
	}
	else
	{
		// need to start up a local engine.

		sprintf(buffer,"maps/sp/%d.map",level);

		init_game(buffer);
//		load_map(buffer);

/*		i=0;
		sprintf(buffer,"maps/sp/%d.txt",level);
		FILE *fp = fopen(buffer,"r");
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
		fclose(fp);*/
	}
}

static void game_disconnect()
{
	if (multiplayer)
	{
		p->data[0]='D';
		p->len=1;

		SDLNet_UDP_Send(sd, -1, p);
		SDLNet_FreePacket(p);
	} else {
		shutdown_game();
	}
}

static void game_quit()
{
	int i;

	glDeleteTextures( 10, tex_digit );
	glDeleteTextures( 7, tex_hud );
	glDeleteTextures( NUM_OBJECTS, tex_object );
	glDeleteTextures( NUM_BLDGS, tex_bldg );
	glDeleteTextures( NUM_TILES+4, tex_tile );
	glDeleteTextures( NUM_HUMANS, tex_human );
	glDeleteTextures( NUM_PARTICLES, tex_particle );
	glDeleteTextures( 1, &tex_hqmenu );
	glDeleteTextures( 1, &tex_minimap );
	glLoadIdentity();

	for (i=0; i<NUM_SFX; i++)
	{
		if (sfx[i])
		{
			Mix_FreeChunk(sfx[i]);
			sfx[i] = NULL;
		}
	}
}




unsigned char do_gs_game()
{
	// Initialize game components.
	game_load();

	// connect to game brain
	game_connect();

	// play the game

	// disconnect from game brain
	game_disconnect();

	// quit the game
	game_quit();

	return 1;
}
