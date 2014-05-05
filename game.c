/* game.c - contains the actual game code */

#include "game.h"

// messagebox (speech bubble) used on this screen
#include "message.h"

#include "texops.h"

// standard math functions
#include <math.h>

// other SDL includes
#include <SDL/SDL_net.h>

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

// Object definitions, as linked list.
//  Objects can have zero or more "child" objects, which means drawing one is
//  recursively calling pushMatrix and translating.
struct object
{
	unsigned short id;
	unsigned char team, angle, sprite;
	short x, y;
	char speed;

// Specific to OBJ_HUMAN
	//unsigned char weapon, torso_angle;
// Specific to OBJ_PARTICLE and OBJ_PROJECTILE
	float life;
// Specific to OBJ_PARTICLE
	float r,g,b;
// Specific to OBJ_VEHICLE and OBJ_HUMAN
	struct object *child;

	struct object *next;
} *objects[4];

////////////
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
// Player ID number
unsigned int id;
// Zoom window size
unsigned int oldzoom, newzoom;
// Is player chatting?  What do they say?
unsigned char bChatting;
char cMesg[50];

// Cheats
unsigned char iSeeAll;

// Player controls buffer
// 0=up, 1=down, 2=left, 3=right, 4=prim-fire, 5=second-fire, 6=weaponswap, 7=enter/exit
unsigned char localcontrols[9];

// OK !   Static items.  Key game components go here.
// loads of sound effects.
Mix_Chunk *sfx[NUM_SFX];

// GL texture objects
GLuint tex_tile[ NUM_TILES+4 ]; // Map stuff
GLuint tex_bldg[ NUM_BLDGS ];
int bldg_w[ NUM_BLDGS ], bldg_h[ NUM_BLDGS ];

GLuint tex_hud[ 4 ]; // Minimap and UI
GLuint tex_digit[10];
GLuint tex_hqmenu;
GLuint tex_minimap;
GLuint tex_dir;

GLuint tex_object[ NUM_OBJECTS ];
int obj_w[NUM_OBJECTS], obj_h[NUM_OBJECTS];
GLuint tex_human[ NUM_HUMANS ];
GLuint tex_bullet[ NUM_BULLETS ];
int bullet_w[NUM_OBJECTS], bullet_h[NUM_OBJECTS];
GLuint tex_particle[ NUM_PARTICLES ];
int particle_w[NUM_OBJECTS], particle_h[NUM_OBJECTS];
//GLuint clouds;

// However those textures aren't usually used directly
//  Call the appropriate display-list instead to draw object of choice
GLuint list_tree;  // trees
GLuint list_map;   // landscape
GLuint list_cappoint; // capture point

GLuint list_hqmenu; // HQ menu: HQ only
GLuint list_minimap; // on-screen minimap: non-HQ only
GLuint list_hud; // HUD (ammo, health, cap-points)
GLuint list_digit; // Numbers
GLuint list_direction; // Triangle showing current direction

GLuint list_human; // Four classes of mobile object
GLuint list_object;
GLuint list_object_shadow;
GLuint list_bullet;
GLuint list_particle;

// gotta know your FPS...
int frames;
unsigned long fpsticks;
int smooth[5]={UPDATE_FREQ,UPDATE_FREQ,UPDATE_FREQ,UPDATE_FREQ,UPDATE_FREQ};

// Game variables
static int ammo, health, abase,nbase, cash[2]={0,0}, owner[3] = {0,0,0};

//some stuff for drawing the map
unsigned char map[MAP_X][MAP_Y];
unsigned char trees[MAP_X][MAP_Y];
unsigned char bldloc[15][3];

#define min(a,b) (a < b ? a : b)
#define max(a,b) (a > b ? a : b)

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

static void game_create_list(GLuint base, int i, GLuint *tex, int *a_w, int *a_h)
{
	int w,h;
	if (a_w == NULL)
	{
		w = 32; h = 32;
	} else {
		w = a_w[i]; h = a_h[i];
	}
	glNewList(base+i, GL_COMPILE);
	glBindTexture(GL_TEXTURE_2D, tex[i]); 
	glBegin(GL_QUADS);
		glBox(-w / 2, -h / 2, w, h);
	glEnd();
	glEndList();
}

static void game_create_shadow_list(GLuint base, int i, GLuint *tex, int *w, int *h)
{
	glNewList(base+i, GL_COMPILE);
	glBindTexture(GL_TEXTURE_2D, tex[i]); 
	glDisable(GL_ALPHA_TEST);
	glEnable(GL_BLEND);
	glColor4f(0,0,0,0.5);
	glBegin(GL_QUADS);
		glBox(-w[i] / 2, -h[i] / 2, w[i], h[i]);
	glEnd();
	glColor4f(1,1,1,1);
	glDisable(GL_BLEND);
	glEnable(GL_ALPHA_TEST);
	glEndList();
}

static void game_load()
{
	unsigned int i;
	char buffer[80];

	srand(time(NULL));

	glGenTextures( NUM_TILES+4, tex_tile );
	glGenTextures( NUM_BLDGS, tex_bldg );

	glGenTextures( 10, tex_digit );
	glGenTextures( 1, &tex_hqmenu );
	glGenTextures( 4, tex_hud );
	glGenTextures( 1, &tex_dir );
	glGenTextures( 1, &tex_minimap );	// minimap not actually generated until later.

	glGenTextures( NUM_OBJECTS, tex_object );
	glGenTextures( NUM_HUMANS, tex_human );
	glGenTextures( NUM_BULLETS, tex_bullet );
	glGenTextures( NUM_PARTICLES, tex_particle );


	///////////////
	// Also, Create display lists for all the items we will have.
	///////
	// list_map, list_cappoint and list_tree generated in load_map

	// Map elements: tiles and buildings
	for (i=0; i<NUM_TILES+4; i++)
	{
		sprintf(buffer,"img/tiles/%d.png",i);
		tex_tile[i] = load_texture(buffer,GL_NEAREST,GL_NEAREST);
	}

	for (i=0; i<NUM_BLDGS; i++)
	{
		sprintf(buffer,"img/bldg/%d.png",i);
		tex_bldg[i] = load_texture_extra(buffer,GL_NEAREST,GL_NEAREST,&bldg_w[i],&bldg_h[i]);
	}

	// Load textures for UI elements
	tex_hqmenu = load_texture("img/hud/hqmenu.png",GL_LINEAR,GL_LINEAR);
	list_hqmenu = glGenLists(1);
	glNewList(list_hqmenu, GL_COMPILE); {
		// No need for alpha test
		glDisable(GL_ALPHA_TEST);
		glBindTexture(GL_TEXTURE_2D, tex_hqmenu); 
		glBegin(GL_QUADS);
			glBox(SCREEN_X-200,0,200,SCREEN_Y);
		glEnd();
	} glEndList();

	tex_dir = load_texture("img/ui/dir.png",GL_NEAREST,GL_NEAREST);
	// Which way does player face?
	list_direction = glGenLists(1);
	glNewList(list_minimap, GL_COMPILE); {
		glBindTexture(GL_TEXTURE_2D, tex_dir);
		glBegin(GL_QUADS);
			glBox(0,0,8,8);
		glEnd();
	} glEndList();

	// for the numbers.
	list_digit = glGenLists(10);
	for (i=0; i<10; i++)
	{
		sprintf(buffer,"img/ui/n%d.png",i);
		tex_digit[i] = load_texture(buffer,GL_NEAREST,GL_NEAREST);
		glNewList(list_digit+i, GL_COMPILE);
		glBindTexture(GL_TEXTURE_2D, tex_digit[i]); 
		glBegin(GL_QUADS);
			glBox(0,0,32,32);
		glEnd();
		glEndList();

	}

	// HUD elements
	for (i=0; i<4; i++)
	{
		sprintf(buffer,"img/hud/%d.png",i);
		tex_hud[i] = load_texture(buffer,GL_NEAREST,GL_NEAREST);
	}
	list_hud = glGenLists(1);
	glNewList(list_hud, GL_COMPILE); {
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
	} glEndList();

	// Minimap generated in load_map, but these parameters are needed to make it display.
	glBindTexture( GL_TEXTURE_2D, tex_minimap);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	// Don't yet have a minimap texture, but we can make a DL to show it.
	list_minimap = glGenLists(1);

	// for the particles.
	list_particle = glGenLists(NUM_PARTICLES);
	for (i=0; i<NUM_PARTICLES; i++)
	{
		sprintf(buffer,"img/particle/%d.png",i);
		tex_particle[i] = load_texture_extra(buffer,GL_NEAREST,GL_NEAREST,&particle_h[i],&particle_w[i]);

		game_create_list(list_particle,i,tex_particle,particle_w,particle_h);
	}

	list_bullet = glGenLists(NUM_BULLETS);
	for (i=0; i<NUM_BULLETS; i++)
	{
		sprintf(buffer,"img/bul/%d.png",i);
		tex_bullet[i] = load_texture_extra(buffer,GL_NEAREST,GL_NEAREST, &bullet_w[i],&bullet_h[i]);

		game_create_list(list_bullet,i,tex_bullet,bullet_w,bullet_h);
	}

	list_object = glGenLists(NUM_OBJECTS);
	list_object_shadow = glGenLists(NUM_OBJECTS);
	for (i=0; i<NUM_OBJECTS; i++)
	{
		sprintf(buffer,"img/gfx/%d.png",i);
		tex_object[i] = load_texture_extra(buffer,GL_NEAREST,GL_NEAREST,&obj_w[i],&obj_h[i]);

		game_create_list(list_object,i,tex_object,obj_w,obj_h);
		game_create_shadow_list(list_object_shadow,i,tex_object,obj_w,obj_h);
	}

	list_human = glGenLists(NUM_HUMANS);
	for (i=0; i<NUM_HUMANS; i++)
	{
		sprintf(buffer,"img/humgfx/%d.png",i);
		tex_human[i] = load_texture(buffer,GL_NEAREST,GL_NEAREST);

		game_create_list(list_human,i,tex_human,NULL,NULL);
	}

/////

	// Load sound effects.
	for (i=0; i<NUM_SFX; i++)
	{
		sprintf(buffer,"audio/sfx/%d.wav",i);
		sfx[i] = Mix_LoadWAV(buffer);
		if (!sfx[i]) fprintf(stderr,"Error: couldn't load WAV file %s: %s\n",buffer,Mix_GetError());
	}

//iSeeAll = 1;

	frames = 0;
	fpsticks = SDL_GetTicks();
/*	camx = 400;
	camy = 300;*/
	oldzoom = 0;
	newzoom = 0;

	ticks=SDL_GetTicks();

//	for (i=0; i<9; i++)
//		localcontrols[i]=0;

	message_clear();
}

/*static void create_particle(int x, int y, unsigned char angle, unsigned int speed, unsigned char life, int r, int g, int b)
{
	object *np;
	np = (particle *) malloc (sizeof (struct particle));
	np->x=x;
	np->y=y;
	np->angle=angle;
	np->speed=speed;
	np->type=type;
	np->life=life;
	np->next=toppart;
	toppart=np;
}*/

// Draws an object on screen.
static void draw_object(struct object *obj)
{
	glPushMatrix();
	glTranslatef(obj->x,obj->y,0);
	glRotatef(obj->angle,0,0,1);
	glColor4f(obj->r,obj->g,obj->b,min(obj->life,1));
	glCallList(list_object + obj->sprite);

	struct object *child = obj->child;
	while (child != NULL)
	{
		draw_object(child);
		child = child->next;
	}
	glPopMatrix();
}

// Draws a shadow under an object on screen.
static void draw_shadow(struct object *obj)
{
	int offset = max(30, obj->speed - 30);

	glPushMatrix();
	glTranslatef(obj->x+offset,obj->y+offset,0);
	glRotatef(obj->angle,0,0,1);
	glCallList(list_object_shadow + obj->sprite);

	glPopMatrix();
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
	//  Trees are annoying because of shadows, so we have to actually draw it twice.
	list_tree = glGenLists(1);
	glNewList(list_tree, GL_COMPILE);
	{
		glPushMatrix();
		glTranslatef(-10,-10,0);
		// Begin with solid, shadowed tree blocks.
		glColor4f(0,0,0,.5);

		glDisable(GL_ALPHA_TEST);
		glDisable(GL_TEXTURE_2D);
		glEnable(GL_BLEND);

		glBegin(GL_QUADS);
		for (x = 0; x < MAP_X; x ++)
		{
			for (y = 0; y < MAP_Y; y ++)
			{
				if (trees[x][y])
				{
					drawgame_tile(x,y,0);
				}
			}
		}
		glEnd();

		// Next, tree borders.  These use the darkened border texture.
		glEnable(GL_TEXTURE_2D);
		glBindTexture( GL_TEXTURE_2D, tex_tile[19] );
		glBegin(GL_QUADS);
		for (x = 0; x < MAP_X; x ++)
		{
			for (y = 0; y < MAP_Y; y ++)
			{
				if (tb[x][y])
				{
					if (tb[x][y] & 8) drawgame_tile(x,y,2);
					if (tb[x][y] & 4) drawgame_tile(x,y,0);
					if (tb[x][y] & 2) drawgame_tile(x,y,1);
					if (tb[x][y] & 1) drawgame_tile(x,y,3);
				}
			}
		}
		glEnd();
		// Shadows done.
		glDisable(GL_BLEND);
		glColor4f(1,1,1,1);
		glPopMatrix();
		
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
		for (i = 3; i < 15; i ++)
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

	// Almost done.  list_cappoint is 9 lists, one per capture point per color.
	list_cappoint = glGenLists(9);
	for (i = 0; i < 3; i ++)
	{
		glNewList(list_cappoint + (3*i), GL_COMPILE);
		{
		  glBindTexture(GL_TEXTURE_2D, tex_bldg[0]);
		  glBegin(GL_QUADS);
			glBox(32 * bldloc[i][1] - (bldg_w[0] / 2),32 * bldloc[i][2] - (bldg_h[0] / 2),bldg_w[0],bldg_h[0]);
		  glEnd();
		}
		glEndList();

		glNewList(list_cappoint + (3*i)+1, GL_COMPILE);
		{
		  glBindTexture(GL_TEXTURE_2D, tex_bldg[8]);
		  glBegin(GL_QUADS);
			glBox(32 * bldloc[i][1] - (bldg_w[0] / 2),32 * bldloc[i][2] - (bldg_h[0] / 2),bldg_w[0],bldg_h[0]);
		  glEnd();
		}
		glEndList();

		glNewList(list_cappoint + (3*i)+2, GL_COMPILE);
		{
		  glBindTexture(GL_TEXTURE_2D, tex_bldg[9]);
		  glBegin(GL_QUADS);
			glBox(32 * bldloc[i][1] - (bldg_w[0] / 2),32 * bldloc[i][2] - (bldg_h[0] / 2),bldg_w[0],bldg_h[0]);
		  glEnd();
		}
		glEndList();

    }
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

	glBindTexture( GL_TEXTURE_2D, tex_minimap);
	glTexImage2D(GL_TEXTURE_2D, 0, 4, ts->w, ts->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, ts->pixels);
	SDL_FreeSurface(ts);

	glNewList(list_minimap, GL_COMPILE); {
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
	} glEndList();

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

static int game_connect()
{
	int i;
	//char buffer[80];
	char map_name[80] = "maps/x-isle.map";

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

	health=100; ammo=30;cash[0]=cash[1]=500;
 
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

	glDeleteLists(list_tree,1);
	glDeleteLists(list_map,1);
	glDeleteLists(list_cappoint,9);

	glDeleteLists(list_hqmenu,1);
	glDeleteLists(list_minimap,1);
	glDeleteLists(list_hud,1);
	glDeleteLists(list_digit,10);
	glDeleteLists(list_direction,1);

	glDeleteLists(list_human,NUM_HUMANS);
	glDeleteLists(list_object,NUM_OBJECTS);
	glDeleteLists(list_object_shadow,NUM_OBJECTS);
	glDeleteLists(list_bullet,NUM_BULLETS);
	glDeleteLists(list_particle,NUM_PARTICLES);

	glDeleteTextures( 10, tex_digit );
	glDeleteTextures( 4, tex_hud );
	glDeleteTextures( NUM_OBJECTS, tex_object );
	glDeleteTextures( NUM_BULLETS, tex_bullet );
	glDeleteTextures( NUM_BLDGS, tex_bldg );
	glDeleteTextures( NUM_TILES+4, tex_tile );
	glDeleteTextures( NUM_HUMANS, tex_human );
	glDeleteTextures( NUM_PARTICLES, tex_particle );
	glDeleteTextures( 1, &tex_hqmenu );
	glDeleteTextures( 1, &tex_minimap );
	glDeleteTextures( 1, &tex_dir );

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
	int i;
/*	int j, drawTrail=0; */
	int startx, starty, endx, endy;

	struct object *obj;

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

		glTranslatef(-1500,-1500,0);
	}

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho( startx, endx, starty, endy, -1.0, 1.0);

	glMatrixMode(GL_MODELVIEW);
	glClear(GL_COLOR_BUFFER_BIT);// | GL_DEPTH_BUFFER_BIT);

//	glLoadIdentity();

//	locatecamera(&camx,&camy);

	// Draw landscapes.
	glCallList(list_map);
	// Ground-based objects
	// Capture points
	for (i = 0; i < 3; i ++)
	{
		glCallList(list_cappoint + (3*i) + owner[i]);
	}

	// Everythnig from here on uses alpha test or blend.
	glEnable(GL_ALPHA_TEST);

	// Projectiles
	obj = objects[OBJ_PROJECTILE];
	while (obj != NULL)
	{
		draw_object(obj);
		obj = obj->next;

		/* 		if (drawTrail && (projptr->type == 1 || projptr->type == 13 || projptr->type == 15))
			createparticle((int)(projptr->x -((float)projptr->speed*sintable[projptr->angle] * rate)),(int)(projptr->y-((float)projptr->speed*-costable[projptr->angle] * rate)),(projptr->angle + 118 + (rand() % 20))%256,rand()%20+1,rand()%3+1,rand()%4+1);
			 */
	}

	// Humans
	obj = objects[OBJ_HUMAN];
	while (obj != NULL)
	{
		draw_object(obj);
		obj = obj->next;
	}
	
	// Ground-based vehicles
	obj = objects[OBJ_VEHICLE];
	while (obj != NULL)
	{
		if (obj->sprite < 15 || obj->speed <= 30)
		{
			draw_shadow(obj);
			draw_object(obj);
		}
		obj = obj->next;
	}

	// Trees
	glCallList(list_tree);

	// Airborne objects (planes)
	obj = objects[OBJ_VEHICLE];
	while (obj != NULL)
	{
		if (obj->sprite >= 15 && obj->speed > 30) {
			draw_shadow(obj);
			draw_object(obj);
		}
		obj = obj->next;
	}

	glDisable(GL_ALPHA_TEST);
	glEnable(GL_BLEND);
	// Particles
	obj = objects[OBJ_PARTICLE];
	while (obj != NULL)
	{
		draw_object(obj);
		obj = obj->next;
	}
	glDisable(GL_BLEND);

	// Time to draw the UI.
	//gotta go back to the old way to draw the cursor right size, and UI.
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, SCREEN_X, SCREEN_Y, 0, -1.0, 1.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// if player is commander, draw hq menu
	if (status == 3)
		glCallList(list_hqmenu);

	// Draw HUD
	glCallList(list_hud);
	// HUD items (numbers)
	unsigned int base = 0;
	draw_number(abase,40,0);
	draw_number(nbase,120,0);
	draw_number(ammo,40,40);
	draw_number(health,40,80);

	// Team cash
	if (status == 3)
	{
		draw_number(cash[0],640,20);
		draw_number(cash[1],640,80);
	} else {
		unsigned int x;
        if (cash[0] > 9999) x = 640;
        else if (cash[0] > 999) x = 672;
        else if (cash[0] > 99) x = 704;
        else if (cash[0] > 9) x = 736;
        else x = 768;
		draw_number(cash[0],x,104);

        if (cash[1] > 9999) x = 640;
        else if (cash[1] > 999) x = 672;
        else if (cash[1] > 99) x = 704;
        else if (cash[1] > 9) x = 736;
        else x = 768;
		draw_number(cash[1],x,140);

		// Draw minimap
		glCallList(list_minimap);
	}

	// If player is on foot, draw directional arrow.
/*	glPushMatrix();
		glTranslatef(0, -50, 0);
		glRotatef(90,0,0,1);
		glCallList(list_direction);
	glPopMatrix();*/

	// Chat window
	if (bChatting == 1)
	{
		glColor3f(1,1,0);
		glPrint(144,536,"CHAT:");
		glPrint(160,536,cMesg);
		glColor3f(1,1,1);
	} else if (bChatting == 2) {
		glColor3f(1,1,0);
		glPrint(144,536,"TEAM CHAT:");
		glPrint(160,536,cMesg);
		glColor3f(1,1,1);
	}
	// Draw cursor.
	glPushMatrix();
		glTranslatef(mx, my, 0);
		glCallList(list_cursor);
	glPopMatrix();

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

unsigned int rd_int32(unsigned char *p)
{
	return ( p[0] << 24 ) |
			( p[1] << 16 ) |
			( p[2] << 8 ) |
			p[3];
}

unsigned short rd_int16(unsigned char *p)
{
	return 	( p[0] << 8 ) |
			p[1];
}

static struct object *object_find(unsigned int id, struct object *start)
{
	while (start != NULL && start->id != id)
	{
		start=start->next;
	}
	return start;
}

static struct object *make_object()
{
	struct object *c = malloc(sizeof(struct object));
	c->next = NULL;
	c->child = NULL;
	return c;
}

static unsigned char *child_unserialize(struct object *obj, unsigned char obj_kids, unsigned char *payload)
{
	unsigned char obj_sub_kids, i;

	// deal with empty list
	if (obj->child == NULL)
	{
		obj->child = make_object();
	}
	struct object *c = obj->child;

	// Update the item.
	for (i=0; i<obj_kids; i++)
	{
		c->x = rd_int16(payload); payload += 2;
		c->y = rd_int16(payload); payload += 2;
		c->angle = *payload; payload ++;
		c->sprite = *payload; payload ++;

		obj_sub_kids = *payload; payload ++;

		// subobj kids
		payload = child_unserialize(obj->child, obj_sub_kids, payload);

		// deal with partial list
		if (i < obj_kids - 1 && c->next == NULL)
		{
			c->next = make_object();
		}
		c = c->next;
	}

	return payload;
}

static void unserialize(unsigned char *payload)
{
	int i, obj_count, obj_id, obj_type, obj_kids;
	struct object *obj;

	// Unserializes the server-response for "game update".
	status = *payload; payload ++;

	// Player ID and other local info
	id = rd_int16(payload); payload += 2;
	newzoom = *payload; payload ++;
	health = *payload; payload ++;
	ammo = *payload; payload ++;

	// Unserialize capture-points owner
	for (i = 0; i < 3; i ++) {
		owner[i] = *payload; payload ++;
	}

	// Cash
	for (i = 0; i < 2; i ++) {
		cash[i] = rd_int16(payload); payload += 2;
	}

	// Object updates.
	obj_count = *payload; payload ++;
	for (i = 0; i < obj_count; i ++)
	{
		unsigned char type_and_team = *payload; payload ++;
		obj_type = type_and_team & 0x7F;
		obj_id = rd_int16(payload); payload ++;
	
		// Search for object in the list.
		obj = object_find(obj_id, objects[obj_type]);
		if (obj == NULL)
		{
			obj = make_object();
			obj->next = objects[obj_type];
			objects[obj_type] = obj;
		}
		
		// Update the item.
		obj->team = type_and_team >> 7;
		obj->x = rd_int16(payload); payload += 2;
		obj->y = rd_int16(payload); payload += 2;
		obj->angle = *payload; payload ++;
		obj->speed = *payload; payload ++;
		switch(obj_type)
		{
		case OBJ_PROJECTILE:
			obj->life = *payload; payload ++;
			break;
		case OBJ_VEHICLE:
		case OBJ_HUMAN:
			// Update child objects, if any (humans always have 1, for torso)
			obj_kids = *payload; payload ++;
			if (obj_kids > 0)
			{
				payload = child_unserialize(obj, obj_kids, payload);
			}
			break;
		}
	}
}

static char game_update()
{
	static Uint32 next_control_tick = 0;
	static Uint32 next_server_tick = 0;

	char retval = gs_game;

	char buffer[80];
	
	unsigned int i;

	/* Periodic client-side activity here: things like particle trails, etc. */
	ticks=SDL_GetTicks();

	/* The next thing to do is to check for player inputs. */
	SDL_Event event;
	while (SDL_PollEvent (&event))
	{
		switch (event.type)
		{
			case SDL_KEYDOWN:
				if (!bChatting) {
					if (event.key.keysym.sym == SDLK_LEFT ) //|| event.key.keysym.sym == SDLK_A)
						localcontrols[0]=1;
					else if (event.key.keysym.sym == SDLK_RIGHT) // || event.key.keysym.sym == SDLK_D)
						localcontrols[1]=1;
					else if (event.key.keysym.sym == SDLK_UP) // || event.key.keysym.sym == SDLK_W)
						localcontrols[2]=1;
					else if (event.key.keysym.sym == SDLK_DOWN) // || event.key.keysym.sym == SDLK_S)
						localcontrols[3]=1;
					else if (event.key.keysym.sym == SDLK_RETURN)
						localcontrols[6]=1;
				} else {
					if (event.key.keysym.sym == SDLK_RETURN)
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
                            if (strcmp(cMesg,"raise the roof") == 0) iSeeAll=!iSeeAll;
							else if (strcmp(cMesg,"victory is mine") == 0) retval=gs_win;
							else if (strcmp(cMesg,"game over man") == 0) retval=gs_lose;
						}

						bChatting=0;
				        SDL_EnableUNICODE( 0 );
						memset(cMesg,0,50);
					} else if (event.key.keysym.sym == SDLK_BACKSPACE && strlen(cMesg) > 0)
						cMesg[strlen(cMesg)-1]='\0';
					else if (strlen(cMesg) < 49 && event.key.keysym.unicode < 0x80 && event.key.keysym.unicode >= 0x20)
						cMesg[strlen(cMesg)]=(char)(event.key.keysym.unicode);
				}
				break;
			case SDL_KEYUP:
				if (event.key.keysym.sym == SDLK_ESCAPE)
				{
					if (bChatting) {
						bChatting=0;
				        SDL_EnableUNICODE( 0 );
						memset(cMesg,0,50);
					} else {
						retval=gs_title;
					}
				}

				else if (event.key.keysym.sym == SDLK_LEFT) // || event.key.keysym.sym == SDLK_A)
					localcontrols[0]=0;
				else if (event.key.keysym.sym == SDLK_RIGHT) // || event.key.keysym.sym == SDLK_D)
					localcontrols[1]=0;
				else if (event.key.keysym.sym == SDLK_UP) // || event.key.keysym.sym == SDLK_W)
					localcontrols[2]=0;
				else if (event.key.keysym.sym == SDLK_DOWN) // || event.key.keysym.sym == SDLK_S)
					localcontrols[3]=0;
/*				else if (event.key.keysym.sym == SDLK_RETURN)
					localcontrols(6,0);*/

				else if (event.key.keysym.sym == SDLK_c)
				{
					bChatting = 1;
			        SDL_EnableUNICODE( 1 );
				}
				else if (multiplayer && (event.key.keysym.sym == SDLK_t))
				{
					bChatting = 2;
			        SDL_EnableUNICODE( 1 );
				}
				break;
/*			case SDL_MOUSEBUTTONDOWN:
				if (status == 3) {
					if (event.button.button == SDL_BUTTON_MIDDLE)
						localcontrols(6,1);
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
							localcontrols(0, hqmenusel);
							localcontrols(5,(unsigned char)((int)(mx / 6)));
							localcontrols(8,(unsigned char)((int)(my / 6)));
							localcontrols(4,1);
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
						localcontrols(4,1);
						unshoot[0]=1;
					} else if (event.button.button == SDL_BUTTON_RIGHT) {
						localcontrols(5,1);
						unshoot[1]=1;
					} else if (event.button.button == SDL_BUTTON_MIDDLE)
						localcontrols(6,1);
					else if (event.button.button == SDL_BUTTON_WHEELUP)
						localcontrols(7,1);
					else if (event.button.button == SDL_BUTTON_WHEELDOWN)
						localcontrols(7,1);
				}
				break;
			case SDL_MOUSEBUTTONUP:
				if (event.button.button == SDL_BUTTON_LEFT)
						unshoot[0]=0;
				else if (event.button.button == SDL_BUTTON_RIGHT)
						unshoot[1]=0;
				break;*/
			case SDL_MOUSEMOTION:
				mx=event.motion.x;
				my=event.motion.y;
				// figure out my new facing.
/*				if (status == 3)
				{
					mxt = mx;
					if (mxt > 600) mxt = 600;

					localcontrols(5,(unsigned char)((int)(mxt / 6)));
					localcontrols(8,(unsigned char)((int)(my / 6)));
				} else {
					if (my!=300) newang = 57.2957795*atan(((float)mx-400)/((float)-my+300));
					else if (mx>400) newang=90;
					else newang=270;
					if (my>300) newang=newang-180;

					localcontrols(8,(unsigned char)((int)(newang*255/360)));
				}*/
				break;
			case SDL_QUIT:
				retval = gs_exit;
				break;
			default:
				break;
		}
	}

	/* Send updates to the server, and check for server activity */
	if (multiplayer)
	{
		if (ticks >= next_control_tick)
		{
			next_control_tick = ticks + CONTROL_FREQ;

			if (status != 3) {
				// A regular control update.
				//  A two-byte packet: bitfields packed in byte 1, and byte 2 holds angle as char.
				p->data[0] = 0;
				for (i=0; i<8; i++)
					p->data[0] = p->data[0] | ((localcontrols[i] & 1) << i);
				p->data[1] = localcontrols[8];
				p->len = 2;
			} else {
				// A from-HQ control update.
				//  A three-byte packet: selected item, mouse X, mouse Y, and mouse-click.  Also, enter key.
				p->data[0] = (localcontrols[6] << 7) | (localcontrols[4] << 6) | (localcontrols[0] & 63);
				p->data[1] = localcontrols[5];
				p->data[2] = localcontrols[8];
				p->len = 3;
			}

			// Send the update.
			p->address.host = srvadd.host;	/* Set the destination host */
			p->address.port = srvadd.port;	/* And destination host */
			SDLNet_UDP_Send(sd, -1, p); /* This sets the p->channel */
		}

		if (SDLNet_UDP_Recv(sd,p) && p->len > 0)
		{
			unsigned char *payload = p->data + 1;
			switch (p->data[0])
			{
				case 'C':	// Chat message received
					message_post(0, (char *)&payload);
					break;
				case 'M':	// Map change message
					snprintf(buffer,49,"maps/%s",(char *)&payload);
					load_map(buffer);
					break;
				case 'U':	// Game update.
					unserialize(payload);
					break;
				default:
					break;
			}
		}
	} else {
		if (status == STATUS_HQ)
		{
			control_game_hq(id, localcontrols);
		} else {
			control_game_regular(id, localcontrols);
		}

		//control_game(0, controls);

		if (next_server_tick >= ticks)
		{
			next_server_tick = ticks + UPDATE_FREQ;
			update_game();

			unserialize(serialize_game(id));
		}
	}

	return retval;
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
			retval = game_update();

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
