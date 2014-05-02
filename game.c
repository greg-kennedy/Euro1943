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
#define NUM_PARTICLES 4
#define NUM_SFX 31

// got some externs here
extern int vol_sfx;
extern long mx, my;
extern GLuint list_cursor;

// state variables used at init-time to figure out where to connect to the brain
//  multiplayer or singleplayer?
extern int multiplayer;
//  single-player important init elements
extern int level;
//  on the other hand, these are defined for the multiplayer
extern char HOSTNAME[80];

UDPsocket sd;
IPaddress srvadd;
UDPpacket *p;

// Global time counter
unsigned long ticks;
// Player status
unsigned char status;
// Zoom window size
unsigned int oldzoom, newzoom;

// Cheats
unsigned char iSeeAll;

// OK !   Static items.  Key game components go here.
// loads of sound effects.
Mix_Chunk *sfx[NUM_SFX];

// GL texture objects
GLuint tex_object[ NUM_OBJECTS ];
GLuint tex_bldg[ NUM_BLDGS ];
int bldg_w[ NUM_BLDGS ], bldg_h[ NUM_BLDGS ];
GLuint tex_human[ NUM_HUMANS ];
GLuint tex_tile[ NUM_TILES+4 ];
GLuint tex_bullet[ NUM_BULLETS ];
GLuint tex_particle[ NUM_PARTICLES ];
GLuint tex_hud[ 4 ];
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
GLuint list_tree;
GLuint list_map;
GLuint list_minimap;
GLuint list_bullet[ NUM_BULLETS ];
GLuint list_particle;
GLuint list_hud;
GLuint list_hqmenu;
GLuint list_digit;

// gotta know your FPS...
int frames;
unsigned long fpsticks;
int smooth[5]={UPDATEFREQ,UPDATEFREQ,UPDATEFREQ,UPDATEFREQ,UPDATEFREQ};

// Game variables
int nbase, ammo, health, abase,cash[2];

//some stuff for drawing the map
unsigned char map[MAP_X][MAP_Y];
unsigned char trees[MAP_X][MAP_Y];
unsigned char bldloc[15][3];

int min (int a, int b)
{
	if (a < b) return a; return b;
}
int max (int a, double b)
{
	if ((double)a > b) return a; return (int)b;
}

float lerp (int val_a, int val_b, float time_t)
{
	if (time_t < 0) return val_a;
	else if (time_t > 1) return val_b;
	else
	{
		return (time_t * (val_b - val_a)) + val_a;
	}
}

/*float lerp (int val_a, int val_b, unsigned int time_a, unsigned int time_b, unsigned int time_t)
{
	if (time_t < time_a) return val_a;
	else if (time_t > time_b) return val_b;
	else
	{
		float increment = (float)(val_b - val_a) / (time_b - time_a);

		return (increment * (time_b - time_t)) + val_a;
	}
}*/

static void game_load()
{
	unsigned int i;
	char buffer[80];

	srand(time(NULL));

	glGenTextures( 4, tex_hud );
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

/*	glBindTexture( GL_TEXTURE_2D, tex_minimap );
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); */

/*	glBindTexture( GL_TEXTURE_2D, tex_hud[4] );
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glBindTexture( GL_TEXTURE_2D, tex_hud[5] );
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); */

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

	// for the particles.
	list_particle = glGenLists(NUM_PARTICLES);
	for (i=0; i<NUM_PARTICLES; i++)
	{
		sprintf(buffer,"img/particle/%d.png",i);
		tex_particle[i] = load_texture(buffer,GL_NEAREST,GL_NEAREST);

		glNewList(list_particle+i, GL_COMPILE);
		glBindTexture(GL_TEXTURE_2D, tex_particle[i]); 
		glBegin(GL_QUADS);
			glBox(-8,-8,8,8);
		glEnd();
		glEndList();
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
		tex_bldg[i] = load_texture_extra(buffer,GL_NEAREST,GL_NEAREST,&bldg_w[i],&bldg_h[i]);

/*		bldw[i]=surf->w;
		bldh[i]=surf->h; */
	}

	for (i=0; i<NUM_HUMANS; i++)
	{
		sprintf(buffer,"img/humgfx/%d.png",i);
		tex_human[i] = load_texture(buffer,GL_NEAREST,GL_NEAREST);
	}

	// Create display lists for all the items we will have.
	list_hud = glGenLists(1);
	glNewList(list_hud, GL_COMPILE);
	{
		// Use simple alpha test
		glEnable(GL_ALPHA_TEST);
		glBindTexture(GL_TEXTURE_2D, tex_hud[0]); 
		glBegin(GL_QUADS);
			glBox(0,0,32,32);
		glEnd();
		glBindTexture(GL_TEXTURE_2D, tex_hud[1]); 
		glBegin(GL_QUADS);
			glBox(80,0,32,32);
		glEnd();
		glBindTexture(GL_TEXTURE_2D, tex_hud[2]); 
		glBegin(GL_QUADS);
			glBox(0,40,32,32);
		glEnd();
		glBindTexture(GL_TEXTURE_2D, tex_hud[3]); 
		glBegin(GL_QUADS);
			glBox(0,80,32,32);
		glEnd();
	}
	glEndList();

	list_hqmenu = glGenLists(1);
	glNewList(list_hqmenu, GL_COMPILE);
	{
		// No need for alpha test
		glDisable(GL_ALPHA_TEST);
		glBindTexture(GL_TEXTURE_2D, tex_hqmenu); 
		glBegin(GL_QUADS);
			glBox(SCREEN_X-200,0,200,SCREEN_Y);
		glEnd();
	}
	glEndList();

	// for the numbers.
	list_digit = glGenLists(10);
	for (i = 0; i < 10; i ++)
	{
		glNewList(list_digit+i, GL_COMPILE);
		glBindTexture(GL_TEXTURE_2D, tex_digit[i]); 
		glBegin(GL_QUADS);
			glBox(0,0,32,32);
		glEnd();
		glEndList();
	}

	// Load sound effects.
	for (i=0; i<NUM_SFX; i++)
	{
		sprintf(buffer,"audio/sfx/%d.wav",i);
		sfx[i] = Mix_LoadWAV(buffer);
		if (!sfx[i]) fprintf(stderr,"Error: couldn't load WAV file %s: %s\n",buffer,Mix_GetError());
	}

//	showmoney = 0;
iSeeAll = 1;

	/*	frames = 0;
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

static void draw_number(int num, int x, int y)
{
	// Draws a number to the screen by calling digit display lists.
	int digit,numdigs=1;

	while (numdigs <= num)
		numdigs *= 10;

	while (numdigs > 1)
	{
		numdigs/=10;
		digit = num / numdigs;
		num = num % numdigs;

		glPushMatrix();
			glTranslatef(x,y,0);
			glCallList(list_digit + digit);
		glPopMatrix();
		x+=32;
	}
}

// Map-draw helper.
static void drawgame_tile(int i, int j, int dir)
{
	int x = 32 * i;
	int y = 32 * j;
	switch (dir)
	{
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
		case 3:
			glTexCoord2f(0, 1); glVertex2i(x, y);
			glTexCoord2f(0, 0); glVertex2i(x+32, y);
			glTexCoord2f(1, 0); glVertex2i(x+32, y+32);
			glTexCoord2f(1, 1); glVertex2i(x, y+32);
			break;
		default:
			glBox(x,y,32,32);
			break;
	}
}

// Helper function: creates the pair of map lists from the map array and tree array.
static void draw_map()
{
	unsigned int i,j,x,y;

	// Step 1 in map prettification: border overlaps
	//  These pair of arrays hold the border overlaps, in a stupid manner.
	unsigned char mb[MAP_X][MAP_Y]={{0}};
	unsigned char tb[MAP_X][MAP_Y]={{0}};

	// Now!  Compute border transitions from segment to segment.
	for (i=0; i<MAP_X; i++)
	{
		for (j=0; j< MAP_Y; j++)
		{
			// Tree border first.  If this tile has no trees, and the next tile has them,
			//  then set a flag.
			tb[i][j]=0;
			// For map borders, there are 4 positions within a byte (up, down, left, right)
			//  Set which border to use based on that.
			mb[i][j]=0;

			if (trees[i][j] == 0) {
				// Check tile above
				if (j > 0) {
					if (trees[i][j-1])
						tb[i][j] |= 1;
					else if (map[i][j-1] > map[i][j])
						mb[i][j] |= (map[i][j-1]);
				}

				// Tile below
				if (j < MAP_Y - 1)
				{
					if (trees[i][j+1])
						tb[i][j] |= 2;
					else if (map[i][j+1] > map[i][j])
						mb[i][j] |= (map[i][j+1] << 2);
				}

				// Tile left
				if (i > 0)
				{
					if (trees[i-1][j])
						tb[i][j] |= 4;
					else if (map[i-1][j] > map[i][j])
						mb[i][j] |= (map[i-1][j] << 4);
				}

				// and right
				if (i < MAP_X - 1)
				{
					if (trees[i+1][j])
						tb[i][j] |= 8;
					else if (map[i+1][j] > map[i][j])
						mb[i][j] |= (map[i+1][j] << 6);
				}
			}
		}
	}

	// Step 2 of Map Prettification: randomization of tiles.
	//  Each tile has 4 different random looks.
	//  A noise array is generated here.
	unsigned char tile_shuff[MAP_X][MAP_Y];
	for(j=0;j<MAP_Y;j++)
	  for (i=0;i<MAP_X;i++)
	    tile_shuff[i][j] = (rand() % 4);

	// Now we are ready to create the display lists.
	// This display list holds all the trees.
	list_tree = glGenLists(1);
	glNewList(list_tree, GL_COMPILE);
	{
		// Disable alpha test - these are the solid tree tiles.
		glDisable(GL_ALPHA_TEST);
		// Step through each tree texture.
		for (i = 0; i < 4; i++)
		{
			unsigned char any_tiles = 0;
			for (x = 0; x < MAP_X; x ++)
			{
				for (y = 0; y < MAP_Y; y ++)
				{
					if (trees[x][y] && tile_shuff[x][y] == i)
					{
						int tile_to_use = 12 + tile_shuff[x][y];
						if (any_tiles == 0)
						{
							any_tiles = 1;
							glBindTexture( GL_TEXTURE_2D, tex_tile[tile_to_use] );
							glBegin(GL_QUADS);
						}
						drawgame_tile(x,y,0);
					}
				}
			}
			if (any_tiles) glEnd();
		}

		// Tree borders.
		// enable alpha test for simple transparency
		glEnable(GL_ALPHA_TEST);

		unsigned char any_tiles = 0;
		for (x = 0; x < MAP_X; x ++)
		{
			for (y = 0; y < MAP_Y; y ++)
			{
				if (tb[x][y])
				{
					if (any_tiles == 0)
					{
						any_tiles = 1;
						glBindTexture( GL_TEXTURE_2D, tex_tile[19] );
						glBegin(GL_QUADS);
					}
					if (tb[x][y] & 8) drawgame_tile(x,y,2);
					if (tb[x][y] & 4) drawgame_tile(x,y,0);
					if (tb[x][y] & 2) drawgame_tile(x,y,1);
					if (tb[x][y] & 1) drawgame_tile(x,y,3);
				}
			}
		}
		if (any_tiles) glEnd();
	}
	glEndList();

	// This display list holds all the map.
	list_map = glGenLists(1);
	glNewList(list_map, GL_COMPILE);
	{
		// DISable alpha test: no transparency.
		glDisable(GL_ALPHA_TEST);
		// First step: draw an all blue, untextured quad for water.
		glDisable(GL_TEXTURE_2D);
		glColor3f(0.0,0.0,0.4);
		glBegin(GL_QUADS);
			glVertex2i(0,0);
			glVertex2i(32*MAP_X,0);
			glVertex2i(32*MAP_X,32*MAP_Y);
			glVertex2i(0,32*MAP_Y);
		glEnd();
		glColor3f(1.0,1.0,1.0);
		glEnable(GL_TEXTURE_2D);
		// Step through each map tile.
		for (i = 0; i < 4; i++)
		{
			for (j = 1; j < 4; j++)
			{
				unsigned char any_tiles = 0;
				for (x = 0; x < MAP_X; x ++)
				{
					for (y = 0; y < MAP_Y; y ++)
					{
						if (map[x][y] == j && tile_shuff[x][y] == i)
						{
							int tile_to_use = (4 * (j-1)) + tile_shuff[x][y];
							if (any_tiles == 0)
							{
								any_tiles = 1;
								glBindTexture( GL_TEXTURE_2D, tex_tile[tile_to_use] );
								glBegin(GL_QUADS);
							}
							drawgame_tile(x,y,0);
						}
					}
				}
				if (any_tiles) glEnd();
			}
		}

		glEnable(GL_ALPHA_TEST);
		for (j = 1; j < 4; j++) {
			unsigned char any_tiles = 0;
			for (x = 0; x < MAP_X; x ++)
			{
				for (y = 0; y < MAP_Y; y ++)
				{
					if ((mb[x][y] & 0xC0) == (j << 6) ||
					    (mb[x][y] & 0x30) == (j << 4) ||
					    (mb[x][y] & 0x0C) == (j << 2) ||
					    (mb[x][y] & 0x03) == j)
					{
						if (any_tiles == 0)
						{
							any_tiles = 1;
							glBindTexture( GL_TEXTURE_2D, tex_tile[15 + j] );
							glBegin(GL_QUADS);
						}
						if ((mb[x][y] & 0xC0) == (j << 6)) drawgame_tile(x,y,2);
						if ((mb[x][y] & 0x30) == (j << 4)) drawgame_tile(x,y,0);
						if ((mb[x][y] & 0x0C) == (j << 2)) drawgame_tile(x,y,1);
						if ((mb[x][y] & 0x03) == j) drawgame_tile(x,y,3);
					}
				}
			}
			if (any_tiles) glEnd();
		}

// Buildings
		for (i = 0; i < 15; i ++)
		{
			unsigned char type = bldloc[i][0];
			if (type || i<4)
			{
				glBindTexture(GL_TEXTURE_2D, tex_bldg[type]);
				glBegin(GL_QUADS);
					glBox(32 * bldloc[i][1] - (bldg_w[type] / 2),32 * bldloc[i][2] - (bldg_h[type] / 2),bldg_w[type],bldg_h[type]);
				glEnd();				
			}
		}
	}
	glEndList();

	// Well.  All done.
}

// Helper function: creates the minimap display list.
static void draw_minimap()
{
	unsigned int i, j;
    Uint32 *ts_pix; //, pcolor;

	// Create a temp surface, where we will draw the minimap.
	SDL_Surface *ts = SDL_CreateRGBSurface(SDL_SWSURFACE, 128, 128, 32,
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

	// Make some SDL colors.
	Uint32 clr_tile[4];
	clr_tile[0] = SDL_MapRGB(ts->format,0,0,255);
	clr_tile[1] = SDL_MapRGB(ts->format,255,255,0);
	clr_tile[2] = SDL_MapRGB(ts->format,128,64,0);
	clr_tile[3] = SDL_MapRGB(ts->format,128,255,128);
	Uint32 clr_tree = SDL_MapRGB(ts->format,0,255,0);

	// Lock the surface and get access to its pixel structure.
    SDL_LockSurface(ts);
    ts_pix = (Uint32*)ts->pixels;

	// Put colors down on the minimap.
	for (j=0; j<MAP_Y; j++)
	{
		for (i=0; i<MAP_X; i++)
		{
			if (trees[i][j]) ts_pix[j*128+i] = clr_tree;
			else ts_pix[j*128+i] = clr_tile[map[i][j]];
		}
	}

    SDL_UnlockSurface(ts);
//    SDL_SaveBMP(ts, "minimap.bmp");

	glGenTextures( 1, &tex_minimap );
	glBindTexture( GL_TEXTURE_2D, tex_minimap);
	glTexImage2D(GL_TEXTURE_2D, 0, 4, ts->w, ts->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, ts->pixels);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	SDL_FreeSurface(ts);

	// That makes the minimap texture, how about a DL to show it?
	list_minimap = glGenLists(1);
	glNewList(list_minimap, GL_COMPILE);
		// Disable simple Alpha test
		glDisable(GL_ALPHA_TEST);
		// enable blending for detailed transparency
		glEnable(GL_BLEND);
		// bind minimap texture
		glBindTexture(GL_TEXTURE_2D, tex_minimap);
		glColor4f(1.0f,1.0f,1.0f,0.75f);
		// draw a quad at top-left.
		glBegin(GL_QUADS);
			glTexCoord2f(0,0);
			glVertex2i(SCREEN_X-100, 0);
			glTexCoord2f(0.78125,0);
			glVertex2i(SCREEN_X, 0);
			glTexCoord2f(0.78125,0.78125);
			glVertex2i(SCREEN_X, 100);
			glTexCoord2f(0,0.78125);
			glVertex2i(SCREEN_X-100, 100);
		glEnd();
		glColor4f(1.0f,1.0f,1.0f,1.0f);
		// Turn blending back off, we don't need it any more, probably.
		glDisable(GL_BLEND);
	glEndList();

	// Got it, see ya later bye.
}

// Client-side map loading function.
static char load_map(const char* name)
{
	int i, j;

	FILE *fp = fopen (name,"rb");
	if (fp == NULL)
	{
		fprintf(stderr,"Error opening map %s!\n",name);
		return 0;
	}

	for (j=0; j<MAP_Y; j++)
	{
		for (i=0; i<MAP_X; i+=2)
		{
			// Read next (binary) character from file.
			unsigned char incoming = fgetc(fp);

			// Maps are 100x100 with up to 16 choices for tile.
			//  Pack two tiles in a nibble.  Because, you know... space is valuable?
			int low_tile = (incoming & 0xF0) >> 4;
			int high_tile = (incoming & 0x0F);

			// Special purpose for "trees".
			if (low_tile > 3)
			{
				trees[i][j]=1;
				map[i][j] = 0;
			} else {
				trees[i][j] = 0;
				map[i][j] = low_tile;
			}
			if (high_tile > 3)
			{
				trees[i+1][j]=1;
				map[i+1][j] = 0;
			} else {
				trees[i+1][j] = 0;
				map[i+1][j] = high_tile;
			}
		}
	}

	for (i=0; i<15; i++)
	{
		for (j=0; j<3; j++)
		{
			bldloc[i][j] = (int)fgetc(fp);
		}
   }
/*		initang[0] = (unsigned char)fgetc(fp);
	initang[1] = (unsigned char)fgetc(fp);*/
	fclose(fp);

	draw_map();
	draw_minimap();

	return 1;
}

/*
void drawgame_particle(particle* cur)
{
	int size=16;
	glPushMatrix();
	glTranslatef(cur->x-((float)cur->speed*sintable[cur->angle] * rate),
			cur->y-((float)cur->speed*-costable[cur->angle] * rate),0);
	glRotatef(((float)cur->angle * 1.40625), 0, 0, 1);

	if (cur->type==0) size=8;
	glBindTexture(GL_TEXTURE_2D, particles[cur->type]);
	glBegin(GL_QUADS);
	glTexCoord2f(0, 0); glVertex2i(-size/2, -size/2);
	glTexCoord2f(1, 0); glVertex2i(size/2, -size/2);
	glTexCoord2f(1, 1); glVertex2i(size/2, size/2);
	glTexCoord2f(0, 1); glVertex2i(-size/2, size/2);
	glEnd();
	if (cur->life == 1) glColor4f(1.0f,1.0f,1.0f,1+rate);
	glCallList()
	glColor4f(1.0f,1.0f,1.0f,1.0f);
	glPopMatrix();
}
*/

static int game_connect()
{
	int i;
	//char buffer[80];
	char map_name[80] = "maps/bridge.map";

	if (multiplayer)
	{
		// connect to remote server
        unsigned short int PORT=DEFAULT_PORT;

		 for (i=0; i<(int)strlen(HOSTNAME); i++)
		   if (HOSTNAME[i] == ':' || HOSTNAME[i]==' ') { PORT=atoi(&HOSTNAME[i+1]); HOSTNAME[i]='\0'; break; }
	 
			if (!(sd = SDLNet_UDP_Open(0)))
			{
				fprintf(stderr, "SDLNet_UDP_Open: %s\n", SDLNet_GetError());
				return 0;
			}
			if (SDLNet_ResolveHost(&srvadd, HOSTNAME, PORT))
			{
				fprintf(stderr, "SDLNet_ResolveHost(%s %d): %s\n", HOSTNAME, PORT, SDLNet_GetError());
				return 0;
			}
			/* Allocate memory for the packet */
			if (!(p = SDLNet_AllocPacket(512)))
			{
				fprintf(stderr, "SDLNet_AllocPacket: %s\n", SDLNet_GetError());
				return 0;
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

//		sprintf(map_name,"maps/sp/%d.map",level);

		init_game(map_name);
	}

	abase=1; nbase=1; health=100; ammo=30;cash[0]=cash[1]=500;
 
	// Client load map.
	return load_map(map_name);

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
//	return 1;
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

	glDeleteLists(list_digit,10);
	glDeleteLists(list_hqmenu,1);
	glDeleteLists(list_hud,1);
	glDeleteLists(list_minimap,1);
	glDeleteLists(list_tree,1);
	glDeleteLists(list_map,1);

	glDeleteTextures( 10, tex_digit );
	glDeleteTextures( 4, tex_hud );
	glDeleteTextures( NUM_OBJECTS, tex_object );
	glDeleteTextures( NUM_BLDGS, tex_bldg );
	glDeleteTextures( NUM_TILES+4, tex_tile );
	glDeleteTextures( NUM_HUMANS, tex_human );
	glDeleteTextures( NUM_PARTICLES, tex_particle );
	glDeleteTextures( 1, &tex_hqmenu );
	glDeleteTextures( 1, &tex_minimap );

	for (i=0; i<NUM_SFX; i++)
	{
		if (sfx[i])
		{
			Mix_FreeChunk(sfx[i]);
			sfx[i] = NULL;
		}
	}
}

static void game_draw()
{
/*	int i;
	int j, drawTrail=0; */
	int startx, starty, endx, endy;

	/* gameobj *temp;
	human *humptr;
	projectile *projptr;
	particle *partptr;*/

	//	rate=((float)ticks-SDL_GetTicks())/ UPDATEFREQ;

	// "rate" is lerp-point, between 0 and 1.
	float rate=((float)ticks-SDL_GetTicks())/ ((smooth[0]+smooth[1]+smooth[2]+smooth[3]+smooth[4]) / 5);

	// Set up the orthographic perspective.  Either fully zoomed out (in HQ or cheating), or
	//  set by globalZoom parameter.
//	glClear(GL_COLOR_BUFFER_BIT);// | GL_DEPTH_BUFFER_BIT);
	if (status == 3 || iSeeAll)
	{
		startx = 0;
		endx = 4267;
		starty = 3200;
		endy = 0;
	}
	else
	{
		float globalzoom = lerp(oldzoom, newzoom, rate);
		globalzoom = 100;
		float gz4 = globalzoom * 4;
		float gz3 = globalzoom * 3;
		startx = (int)(SCREEN_X / 2 - gz4);
		endx = (int)(SCREEN_X / 2 + gz4);
		starty = (int)(SCREEN_Y / 2 - gz3);
		endy = (int)(SCREEN_Y / 2 + gz3);

		glTranslatef(-1600,-1600,0);
	}

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho( startx, endx, starty, endy, -1.0, 1.0);

	glMatrixMode(GL_MODELVIEW);
	glClear(GL_COLOR_BUFFER_BIT);// | GL_DEPTH_BUFFER_BIT);

//	glLoadIdentity();

//	locatecamera(&camx,&camy);

	// Draw trees and landscapes.
	glCallList(list_map);
	// Ground-based objects
	
	// Trees
	glCallList(list_tree);
	// Airborne objects

	// Projectiles

	// Particles
	/*partptr=toppart;
	while (partptr!=NULL)
	{
		drawgame_particle(partptr);
		partptr=partptr->next;
	} */

	// Time to draw the UI.
	//gotta go back to the old way to draw the cursor right size, and UI.
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, SCREEN_X, SCREEN_Y, 0, -1.0, 1.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// if player is commander, draw hq menu
	glCallList(list_hqmenu);

	// Draw HUD
	glCallList(list_hud);
	// HUD items (numbers)
	draw_number(abase,40,0);
	draw_number(nbase,120,0);
	draw_number(ammo,40,40);
	draw_number(health,40,80);

	// Team cash
	draw_number(cash[0],640,20);
	draw_number(cash[1],640,80);

	// Draw minimap
	glCallList(list_minimap);

	// Draw cursor.
	glPushMatrix();
		glTranslatef(mx, my, 0);
		glCallList(list_cursor);
	glPopMatrix();

/*
	glColor3f(0.0f,0.0f,0.4f);
	glBegin(GL_QUADS);
	glVertex2i(0,0);
	glVertex2i(0,3200);
	glVertex2i(3200,3200);
	glVertex2i(3200,0);
	glEnd();
	glColor3f(1.0f,1.0f,1.0f); */

/*	glEnable(GL_TEXTURE_2D);

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
	} */
	
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
    
/*    if (status == 1)
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
	glDisable(GL_TEXTURE_2D); */

	//Flip the backbuffer to the primary
	SDL_GL_SwapBuffers();

	// FPS counter : )
	frames++;
	if (fpsticks + 1000 < SDL_GetTicks()) {
		fpsticks = SDL_GetTicks();
		printf("FPS: %d\n",frames);
		frames=0;
	}
}

static char game_input()
{
	SDL_Event event;

	char retval = gs_game;
	
	/* Check for events */
	while (SDL_PollEvent (&event))
	{
		switch (event.type)
		{
			case SDL_KEYUP:
				if (event.key.keysym.sym == SDLK_ESCAPE)
					retval = gs_title;
				break;
			case SDL_MOUSEMOTION:
				mx=event.motion.x;
				my=event.motion.y;
				break;
			case SDL_QUIT:
				retval = gs_exit;
				break;
		}
	}
	return retval;
}

static int game_update()
{
	return 1;
}

char do_gs_game()
{
	char retval = gs_title;

	// Initialize game components.
	game_load();

	// connect to game brain
	if (game_connect())
	{
		retval = gs_game;
		// play the game
		while (retval == gs_game)
		{
			game_draw();
			retval = game_input();
	//		playing = game_update();
			/* Don't run too fast */
			SDL_Delay (1);
		}

		// disconnect from game brain
		game_disconnect();
	}

	// quit the game
	game_quit();

	return retval;
}
