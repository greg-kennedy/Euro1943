/* game.c - contains the actual game code */

#include "game.h"

// standard math functions
#define _USE_MATH_DEFINES
#include <math.h>
#ifndef M_TAU
#define M_TAU (M_PI * 2.0)
#endif

// other SDL includes
#include <SDL/SDL.h>
#include <SDL/SDL_net.h>

// game logics
#include "game_logic.h"
#include "data.h"

/* Loadable objects */
#define NUM_UNITS 23
#define NUM_BLDGS 12
#define NUM_TILES 16
#define NUM_BULLETS 16
#define NUM_PARTICLES 2
#define NUM_SFX 31

// Object definitions, as linked lists.

#define UNIT_OCCUPIED 0x80
#define UNIT_FRAME 0x01

// game globals
extern struct env_t env;

// Units (humans, units etc), these come in multiple parts
struct unit_sub {
	float angle; // 16384 step

	// show human in seat or no
	unsigned char flags;

	// pointer to definition (template) in object list
	const struct object_sub * o;

	// links to other items
//	struct unit_sub * parent;
	struct unit_sub * sub;
	struct unit_sub * next;
};

static struct unit {
	unsigned short id;

	unsigned char team;

	unsigned short x, y;
	char speed; // -64 to +63
	float angle;	// 512 step

	// animation frame (0 or 1) and occupied
	unsigned char flags;

	// calculated locally
	float dx, dy;

	// pointer to definition (template) in object list
	const struct object_base * o;

	struct unit_sub * sub;
	struct unit * next;
} * unit_list = NULL;

static struct projectile {
	// received from server
	unsigned short id;

	unsigned char type;

	unsigned short x, y;
	float angle, life;

	// calculated locally
	float dx, dy;

	struct projectile * next;
} * projectile_list = NULL;

// special effect (splash, explosion, dust cloud, etc)
static struct effect {
	// received from server
	unsigned char type;

	unsigned short x, y;
	float time;

	struct effect * next;
} * effect_list = NULL;

static struct particle {
	unsigned char type;
	float r, g, b;

	float x, y;
	float angle, life, scale, speed;

	// calculated locally
	float dx, dy;

	struct particle * next;
} * particle_list = NULL;

//  single-player important init elements
int slot;
//  on the other hand, these are defined for the env.multiplayer
UDPsocket sd;
IPaddress srvadd;
UDPpacket * p;

// Global time counter
unsigned long ticks, last_update_tick;
// Player status
unsigned char status;
// Respawn counter
unsigned char respawn_time;
// Player ID number
unsigned int id;
// Camera
//unsigned short camX, camY;
// Zoom window size
unsigned int oldzoom, newzoom;
//
int mx, my;
// gotta have screen shake bro!
float shake;
// Is player chatting?  What do they say?
unsigned char bChatting;
char cMesg[50];

// Cheats
unsigned char iSeeAll;

// Player controls buffer
// 0=up, 1=down, 2=left, 3=right, 4=prim-fire, 5=second-fire, 6=weaponswap, 7=enter/exit
unsigned char localcontrols[8];
float localaim;

// OK !   Static items.  Key game components go here.
// loads of sound effects.
Mix_Chunk * sfx[NUM_SFX];

// GL texture objects
GLuint tex_tile[ NUM_TILES + 4 ]; // Map stuff
GLuint tex_bldg[ NUM_BLDGS ];
int bldg_w[ NUM_BLDGS ], bldg_h[ NUM_BLDGS ];

GLuint tex_hud[ 4 ]; // Minimap and UI
GLuint tex_digit[10];
GLuint tex_sidebar[ 2 ];
GLuint tex_minimap;
GLuint tex_fog;
GLuint tex_dir;

GLuint tex_unit[2][ NUM_UNITS ];
int unit_w[NUM_UNITS], unit_h[NUM_UNITS];
GLuint tex_bullet[ NUM_BULLETS ];
int bullet_w[NUM_BULLETS], bullet_h[NUM_BULLETS];
GLuint tex_particle[ NUM_PARTICLES ];
int particle_w[NUM_PARTICLES], particle_h[NUM_PARTICLES];
//GLuint clouds;

// However those textures aren't usually used directly
//  Call the appropriate display-list instead to draw object of choice
GLuint list_tree;  // trees
GLuint list_map;   // landscape
GLuint list_cappoint; // capture point

GLuint list_sidebar; // HQ menu and Redeploy Indicator
GLuint list_minimap; // on-screen minimap: non-HQ only
GLuint list_fog; // Fog of War for HQ player
GLuint list_hud; // HUD (ammo, health, cap-points)
GLuint list_digit; // Numbers
GLuint list_direction; // Triangle showing current direction

GLuint list_unit[2]; // one per team
GLuint list_unit_shadow; // shadow should be same for both
GLuint list_bullet;
GLuint list_particle;

// gotta know your FPS...
int frames;
unsigned long fpsticks;
int smooth[5] = {UPDATE_FREQ, UPDATE_FREQ, UPDATE_FREQ, UPDATE_FREQ, UPDATE_FREQ};

// Game variables
static int ammo, health, abase, nbase, cash[2] = {0, 0}, owner[3] = {0, 0, 0};

//some stuff for drawing the map
unsigned char map[MAP_X][MAP_Y];
unsigned char trees[MAP_X][MAP_Y];
unsigned char bldloc[15][3];

// entity the player controls
struct unit * player;
char seat;

#define min(a,b) (a < b ? a : b)
#define max(a,b) (a > b ? a : b)

// "fast" float square
static float sqr(float x)
{
	return x * x ;
}

static float lerp(int val_a, int val_b, float time_t)
{
	if (time_t < 0) return val_a;
	else if (time_t > 1) return val_b;
	else
		return (time_t * (val_b - val_a)) + val_a;
}

// Create a display list for drawing a texture
//  Binds a texture and draws a box.
static void game_create_list(GLuint base, GLuint tex, int w, int h)
{
	glNewList(base, GL_COMPILE);
	glBox(tex, w, h);
	glEndList();
}

// Create a display list for drawing an object shadow
//  Same texture but all black, 50% opacity
/*
static void game_create_shadow_list(GLuint base, GLuint tex, int w, int h)
{
	glNewList(base, GL_COMPILE);
	glBindTexture(GL_TEXTURE_2D, tex);
	glDisable(GL_ALPHA_TEST);
	glEnable(GL_BLEND);
	glColor4f(0,0,0,0.5);
	glBegin(GL_QUADS);
		glBox(-w / 2, -h / 2, w, h);
	glEnd();
	glColor4f(1,1,1,1);
	glDisable(GL_BLEND);
	glEnable(GL_ALPHA_TEST);
	glEndList();
}
*/

// Load all game objects and set everything up
static void game_load()
{
	unsigned int i;
	char buffer[80];
	glGenTextures(NUM_TILES + 4, tex_tile);
	glGenTextures(NUM_BLDGS, tex_bldg);
	glGenTextures(10, tex_digit);
	glGenTextures(2, tex_sidebar);
	glGenTextures(4, tex_hud);
	glGenTextures(1, &tex_dir);
	glGenTextures(1, &tex_fog);
	glGenTextures(1, &tex_minimap);	// minimap not actually generated until later.
	glGenTextures(NUM_UNITS, tex_unit[0]);
	glGenTextures(NUM_UNITS, tex_unit[1]);
	glGenTextures(NUM_BULLETS, tex_bullet);
	glGenTextures(NUM_PARTICLES, tex_particle);

	///////////////
	// Also, Create display lists for all the items we will have.
	///////
	// list_map, list_cappoint and list_tree generated in load_map

	// Map elements: tiles and buildings
	for (i = 0; i < NUM_TILES + 4; i++) {
		sprintf(buffer, "img/tiles/%d.png", i);
//		tex_tile[i] = load_texture(buffer,GL_LINEAR,GL_NEAREST);
		tex_tile[i] = load_texture(buffer, GL_NEAREST, GL_NEAREST);
	}

	for (i = 0; i < NUM_BLDGS; i++) {
		sprintf(buffer, "img/bldg/%d.png", i);
		tex_bldg[i] = load_texture_extra(buffer, GL_NEAREST, GL_NEAREST, &bldg_w[i], &bldg_h[i]);
	}

	// Load textures for UI elements
	list_sidebar = glGenLists(2);

	for (i = 0; i < 2; i ++) {
		sprintf(buffer, "img/hud/sidebar/%d.png", i);
		tex_sidebar[i] = load_texture(buffer, GL_LINEAR, GL_LINEAR);
		glNewList(list_sidebar + i, GL_COMPILE);
		// No need for alpha test
		glDisable(GL_ALPHA_TEST);
		glBox(tex_sidebar[i], 200, SCREEN_Y);
		glEnd();
		glEndList();
	}

	tex_dir = load_texture("img/ui/dir.png", GL_NEAREST, GL_NEAREST);
	// Which way does player face?
	list_direction = glGenLists(1);
	glNewList(list_direction, GL_COMPILE);
	glBox(tex_dir, 8, 8);
	glEnd();
	// for the numbers.
	list_digit = glGenLists(10);

	for (i = 0; i < 10; i++) {
		sprintf(buffer, "img/ui/n%d.png", i);
		tex_digit[i] = load_texture(buffer, GL_NEAREST, GL_NEAREST);
		glNewList(list_digit + i, GL_COMPILE);
		glBox(tex_digit[i], 32, 32);
		glEndList();
	}

	// HUD elements
	for (i = 0; i < 4; i++) {
		sprintf(buffer, "img/hud/%d.png", i);
		tex_hud[i] = load_texture(buffer, GL_NEAREST, GL_NEAREST);
	}

	list_hud = glGenLists(1);
	glNewList(list_hud, GL_COMPILE);
	{
		// Use simple alpha test
		glEnable(GL_ALPHA_TEST);
		glBindTexture(GL_TEXTURE_2D, tex_hud[0]);
		glBegin(GL_QUADS);
//			glBox(0,0,32,32);
		glEnd();
		glBindTexture(GL_TEXTURE_2D, tex_hud[1]);
		glBegin(GL_QUADS);
//			glBox(80,0,32,32);
		glEnd();
		glBindTexture(GL_TEXTURE_2D, tex_hud[2]);
		glBegin(GL_QUADS);
//			glBox(0,40,32,32);
		glEnd();
		glBindTexture(GL_TEXTURE_2D, tex_hud[3]);
		glBegin(GL_QUADS);
//			glBox(0,80,32,32);
		glEnd();
	}

	glEndList();
	// Minimap generated in load_map, but these parameters are needed to make it display.
	glBindTexture(GL_TEXTURE_2D, tex_minimap);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	// Don't yet have a minimap texture, but we can make a DL to show it.
	list_minimap = glGenLists(1);
	// set up fog-of-war, which is all black,
	//  but tex_fog will be selectively updated as things light up or dim
	glBindTexture(GL_TEXTURE_2D, tex_fog);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	list_fog = glGenLists(1);
	glNewList(list_fog, GL_COMPILE);
	{
		// Disable simple Alpha test
		glDisable(GL_ALPHA_TEST);
		// enable blending for detailed transparency
		glEnable(GL_BLEND);
		// bind minimap texture
		glBindTexture(GL_TEXTURE_2D, tex_fog);
		glColor4f(0.0f, 0.0f, 0.0f, 0.75f);
		// draw a quad at top-left.
		glBegin(GL_QUADS);
		glTexCoord2f(0, 0);
		glVertex2i(0, 0);
		glTexCoord2f(0.78125, 0);
		glVertex2i(SCREEN_Y, 0);
		glTexCoord2f(0.78125, 0.78125);
		glVertex2i(SCREEN_Y, SCREEN_Y);
		glTexCoord2f(0, 0.78125);
		glVertex2i(0, SCREEN_Y);
		glEnd();
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		// Turn blending back off, we don't need it any more, probably.
		glDisable(GL_BLEND);
	}

	glEndList();
	// for the particles.
	list_particle = glGenLists(NUM_PARTICLES);

	for (i = 0; i < NUM_PARTICLES; i++) {
		sprintf(buffer, "img/particle/%d.png", i);
		int w, h;
		tex_particle[i] = load_texture_extra(buffer, GL_NEAREST, GL_NEAREST, &w, &h);
		game_create_list(list_particle + i, tex_particle[i], w, h);
	}

	list_bullet = glGenLists(NUM_BULLETS);

	for (i = 0; i < NUM_BULLETS; i++) {
		sprintf(buffer, "img/bul/%d.png", i);
		int w, h;
		tex_bullet[i] = load_texture_extra(buffer, GL_NEAREST, GL_NEAREST, &w, &h);
		game_create_list(list_bullet + i, tex_bullet[i], w, h);
	}

	list_unit[0] = glGenLists(NUM_UNITS);
	list_unit[1] = glGenLists(NUM_UNITS);

//	list_unit_shadow = glGenLists(NUM_UNITS);
	for (i = 0; i < NUM_UNITS; i++) {
		int w, h;
		sprintf(buffer, "img/gfx/0/%d.png", i);
//		tex_unit[0][i] = load_texture_extra(buffer,GL_LINEAR,GL_LINEAR,&w, &h);
		tex_unit[0][i] = load_texture_extra(buffer, GL_NEAREST, GL_NEAREST, &w, &h);
		game_create_list(list_unit[0] + i, tex_unit[0][i], w, h);
//		game_create_shadow_list(list_unit_shadow + i,tex_unit[0][i],w, h);
		sprintf(buffer, "img/gfx/1/%d.png", i);
		tex_unit[1][i] = load_texture_extra(buffer, GL_NEAREST, GL_NEAREST, &w, &h);
		game_create_list(list_unit[1] + i, tex_unit[1][i], w, h);
	}

/////

	// Load sound effects.
	for (i = 0; i < NUM_SFX; i++) {
		sprintf(buffer, "audio/sfx/%d.wav", i);
		sfx[i] = Mix_LoadWAV(buffer);

		if (!sfx[i]) fprintf(stderr, "Error: couldn't load WAV file %s: %s\n", buffer, Mix_GetError());
	}

//iSeeAll = 1;
	frames = 0;
	fpsticks = ticks = SDL_GetTicks();
//	camx = 400;
//	camy = 300;
	oldzoom = 0;
	newzoom = 0;
	bChatting = 0;
	shake = 0;
	// mouse state
	SDL_GetMouseState(&mx, &my);
//	for (i=0; i<9; i++)
//		localcontrols[i]=0;
	message_clear();
}

// Draws an object on screen.
static void draw_unit_sub(int team, struct unit_sub * c)
{
	do {
		glPushMatrix();
		glTranslatef(c->o->offset_x, c->o->offset_y, 0);
		glRotatef((180.0 / M_PI) * c->angle, 0, 0, 1);
		glTranslatef(c->o->center_x, c->o->center_y, 0);

		if ((c->flags & UNIT_OCCUPIED) && (c->o->flags & SEAT_IMG))
			glCallList(list_unit[team] + 2);
		else if (c->o->sprite != -1)
			glCallList(list_unit[team] + c->o->sprite);

		if (c->sub)
			draw_unit_sub(team, c->sub);

		glPopMatrix();
		c = c->next;
	} while (c != NULL);
}

static void draw_unit(struct unit * v, float rate)
{
	glPushMatrix();
	glTranslatef(v->x + rate * v->dx, v->y + rate * v->dy, 0);
	glRotatef((180.0 / M_PI) * v->angle, 0, 0, 1);
	draw_unit_sub(v->team, v->sub);
	glPopMatrix();
}

static void draw_projectile(struct projectile * j, float rate)
{
	glPushMatrix();
	glTranslatef(j->x + rate * j->dx, j->y + rate * j->dy, 0);
	glRotatef((180.0 / M_PI) * j->angle, 0, 0, 1);
	glCallList(list_bullet + j->type);
	glPopMatrix();
}

static void draw_particle(struct particle * t, float rate)
{
	glPushMatrix();
	glTranslatef(t->x + rate * t->dx, t->y + rate * t->dy, 0);
//	glRotatef((180.0 / M_PI) * t->angle ,0,0,1);
	glScalef(t->scale, t->scale, 1.0);
	glColor4f(t->r, t->g, t->b, t->life - rate);
	glCallList(list_particle + t->type);
	glPopMatrix();
}

// Draws a shadow under an object on screen.
/*
static void draw_shadow(struct entity *obj)
{
	int offset = max(30, obj->speed - 30);

	glPushMatrix();
	glTranslatef(obj->x+offset,obj->y+offset,0);
	glRotatef(obj->angle,0,0,1);
	glCallList(list_unit_shadow + obj->sprite);


	glPopMatrix();
}
*/

static void draw_number(int num, int x, int y)
{
	// Draws a number to the screen by calling digit display lists.
	int digit, numdigs = 1;

	while (numdigs <= num)
		numdigs *= 10;

	while (numdigs > 1) {
		numdigs /= 10;
		digit = num / numdigs;
		num = num % numdigs;
		glPushMatrix();
		glTranslatef(x, y, 0);
		glCallList(list_digit + digit);
		glPopMatrix();
		x += 32;
	}
}

// Map-draw helper.
static void drawgame_tile(int i, int j, int dir)
{
	int x = 32 * i;
	int y = 32 * j;

	switch (dir) {
	case 0:
		glTexCoord2f(0, 0);
		glVertex2i(x, y);
		glTexCoord2f(1, 0);
		glVertex2i(x + 32, y);
		glTexCoord2f(1, 1);
		glVertex2i(x + 32, y + 32);
		glTexCoord2f(0, 1);
		glVertex2i(x, y + 32);
		break;

	case 1:
		glTexCoord2f(1, 0);
		glVertex2i(x, y);
		glTexCoord2f(1, 1);
		glVertex2i(x + 32, y);
		glTexCoord2f(0, 1);
		glVertex2i(x + 32, y + 32);
		glTexCoord2f(0, 0);
		glVertex2i(x, y + 32);
		break;

	case 2:
		glTexCoord2f(1, 1);
		glVertex2i(x, y);
		glTexCoord2f(0, 1);
		glVertex2i(x + 32, y);
		glTexCoord2f(0, 0);
		glVertex2i(x + 32, y + 32);
		glTexCoord2f(1, 0);
		glVertex2i(x, y + 32);
		break;

	default:
		glTexCoord2f(0, 1);
		glVertex2i(x, y);
		glTexCoord2f(0, 0);
		glVertex2i(x + 32, y);
		glTexCoord2f(1, 0);
		glVertex2i(x + 32, y + 32);
		glTexCoord2f(1, 1);
		glVertex2i(x, y + 32);
		break;
	}
}

// Helper function: creates the pair of map lists from the map array and tree array.
static void draw_map()
{
	unsigned int i, j, x, y;
	// Step 1 in map prettification: border overlaps
	//  These pair of arrays hold the border overlaps, in a stupid manner.
	unsigned char mb[MAP_X][MAP_Y] = {{0}};
	unsigned char tb[MAP_X][MAP_Y] = {{0}};

	// Now!  Compute border transitions from segment to segment.
	for (i = 0; i < MAP_X; i++) {
		for (j = 0; j < MAP_Y; j++) {
			// Tree border first.  If this tile has no trees, and the next tile has them,
			//  then set a flag.
			tb[i][j] = 0;
			// For map borders, there are 4 positions within a byte (up, down, left, right)
			//  Set which border to use based on that.
			mb[i][j] = 0;

			if (trees[i][j] == 0) {
				// Check tile above
				if (j > 0) {
					if (trees[i][j - 1])
						tb[i][j] |= 1;
					else if (map[i][j - 1] > map[i][j])
						mb[i][j] |= (map[i][j - 1]);
				}

				// Tile below
				if (j < MAP_Y - 1) {
					if (trees[i][j + 1])
						tb[i][j] |= 2;
					else if (map[i][j + 1] > map[i][j])
						mb[i][j] |= (map[i][j + 1] << 2);
				}

				// Tile left
				if (i > 0) {
					if (trees[i - 1][j])
						tb[i][j] |= 4;
					else if (map[i - 1][j] > map[i][j])
						mb[i][j] |= (map[i - 1][j] << 4);
				}

				// and right
				if (i < MAP_X - 1) {
					if (trees[i + 1][j])
						tb[i][j] |= 8;
					else if (map[i + 1][j] > map[i][j])
						mb[i][j] |= (map[i + 1][j] << 6);
				}
			}
		}
	}

	// Step 2 of Map Prettification: randomization of tiles.
	//  Each tile has 4 different random looks.
	//  A noise array is generated here.
	unsigned char tile_shuff[MAP_X][MAP_Y];

	for (j = 0; j < MAP_Y; j++)
		for (i = 0; i < MAP_X; i++)
			tile_shuff[i][j] = (rand() % 4);

	// Now we are ready to create the display lists.
	// This display list holds all the trees.
	//  Trees are annoying because of shadows, so we have to actually draw it twice.
	list_tree = glGenLists(1);
	glNewList(list_tree, GL_COMPILE);
	{
		glPushMatrix();
		glTranslatef(-10, -10, 0);
		// Begin with solid, shadowed tree blocks.
		glColor4f(0, 0, 0, .5);
		glDisable(GL_ALPHA_TEST);
		glDisable(GL_TEXTURE_2D);
		glEnable(GL_BLEND);
		glBegin(GL_QUADS);

		for (x = 0; x < MAP_X; x ++) {
			for (y = 0; y < MAP_Y; y ++) {
				if (trees[x][y])
					drawgame_tile(x, y, 0);
			}
		}

		glEnd();
		// Next, tree borders.  These use the darkened border texture.
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, tex_tile[19]);
		glBegin(GL_QUADS);

		for (x = 0; x < MAP_X; x ++) {
			for (y = 0; y < MAP_Y; y ++) {
				if (tb[x][y]) {
					if (tb[x][y] & 8) drawgame_tile(x, y, 2);

					if (tb[x][y] & 4) drawgame_tile(x, y, 0);

					if (tb[x][y] & 2) drawgame_tile(x, y, 1);

					if (tb[x][y] & 1) drawgame_tile(x, y, 3);
				}
			}
		}

		glEnd();
		// Shadows done.
		glDisable(GL_BLEND);
		glColor4f(1, 1, 1, 1);
		glPopMatrix();
		// Disable alpha test - these are the solid tree tiles.
		glDisable(GL_ALPHA_TEST);

		// Step through each tree texture.
		for (i = 0; i < 4; i++) {
			unsigned char any_tiles = 0;

			for (x = 0; x < MAP_X; x ++) {
				for (y = 0; y < MAP_Y; y ++) {
					if (trees[x][y] && tile_shuff[x][y] == i) {
						int tile_to_use = 12 + tile_shuff[x][y];

						if (any_tiles == 0) {
							any_tiles = 1;
							glBindTexture(GL_TEXTURE_2D, tex_tile[tile_to_use]);
							glBegin(GL_QUADS);
						}

						drawgame_tile(x, y, 0);
					}
				}
			}

			if (any_tiles) glEnd();
		}

		// Tree borders.
		// enable alpha test for simple transparency
		glEnable(GL_ALPHA_TEST);
		unsigned char any_tiles = 0;

		for (x = 0; x < MAP_X; x ++) {
			for (y = 0; y < MAP_Y; y ++) {
				if (tb[x][y]) {
					if (any_tiles == 0) {
						any_tiles = 1;
						glBindTexture(GL_TEXTURE_2D, tex_tile[19]);
						glBegin(GL_QUADS);
					}

					if (tb[x][y] & 8) drawgame_tile(x, y, 2);

					if (tb[x][y] & 4) drawgame_tile(x, y, 0);

					if (tb[x][y] & 2) drawgame_tile(x, y, 1);

					if (tb[x][y] & 1) drawgame_tile(x, y, 3);
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
		glColor3f(0.0, 0.0, 0.4);
		glBegin(GL_QUADS);
		glVertex2i(0, 0);
		glVertex2i(32 * MAP_X, 0);
		glVertex2i(32 * MAP_X, 32 * MAP_Y);
		glVertex2i(0, 32 * MAP_Y);
		glEnd();
		glColor3f(1.0, 1.0, 1.0);
		glEnable(GL_TEXTURE_2D);

		// Step through each map tile.
		for (i = 0; i < 4; i++) {
			for (j = 1; j < 4; j++) {
				unsigned char any_tiles = 0;

				for (x = 0; x < MAP_X; x ++) {
					for (y = 0; y < MAP_Y; y ++) {
						if (map[x][y] == j && tile_shuff[x][y] == i) {
							int tile_to_use = (4 * (j - 1)) + tile_shuff[x][y];

							if (any_tiles == 0) {
								any_tiles = 1;
								glBindTexture(GL_TEXTURE_2D, tex_tile[tile_to_use]);
								glBegin(GL_QUADS);
							}

							drawgame_tile(x, y, 0);
						}
					}
				}

				if (any_tiles) glEnd();
			}
		}

		glEnable(GL_ALPHA_TEST);

		for (j = 1; j < 4; j++) {
			unsigned char any_tiles = 0;

			for (x = 0; x < MAP_X; x ++) {
				for (y = 0; y < MAP_Y; y ++) {
					if ((mb[x][y] & 0xC0) == (j << 6) ||
						(mb[x][y] & 0x30) == (j << 4) ||
						(mb[x][y] & 0x0C) == (j << 2) ||
						(mb[x][y] & 0x03) == j) {
						if (any_tiles == 0) {
							any_tiles = 1;
							glBindTexture(GL_TEXTURE_2D, tex_tile[15 + j]);
							glBegin(GL_QUADS);
						}

						if ((mb[x][y] & 0xC0) == (j << 6)) drawgame_tile(x, y, 2);

						if ((mb[x][y] & 0x30) == (j << 4)) drawgame_tile(x, y, 0);

						if ((mb[x][y] & 0x0C) == (j << 2)) drawgame_tile(x, y, 1);

						if ((mb[x][y] & 0x03) == j) drawgame_tile(x, y, 3);
					}
				}
			}

			if (any_tiles) glEnd();
		}

		// Buildings
		for (i = 3; i < 15; i ++) {
			unsigned char type = bldloc[i][0];

			if (type) { // || i<4)
				glBindTexture(GL_TEXTURE_2D, tex_bldg[type]);
				glBegin(GL_QUADS);
//					glBox(32 * bldloc[i][1] - (bldg_w[type] / 2) + 16,32 * bldloc[i][2] - (bldg_h[type] / 2) + 16,bldg_w[type],bldg_h[type]);
				glEnd();
			}
		}
	}

	glEndList();
	// Almost done.  list_cappoint is 9 lists, one per capture point per color.
	list_cappoint = glGenLists(9);

	for (i = 0; i < 3; i ++) {
		glNewList(list_cappoint + (3 * i), GL_COMPILE);
		{
			glBindTexture(GL_TEXTURE_2D, tex_bldg[0]);
			glBegin(GL_QUADS);
//			glBox(32 * bldloc[i][1] - (bldg_w[0] / 2) + 16,32 * bldloc[i][2] - (bldg_h[0] / 2) + 16,bldg_w[0],bldg_h[0]);
			glEnd();
		}

		glEndList();
		glNewList(list_cappoint + (3 * i) + 1, GL_COMPILE);
		{
			glBindTexture(GL_TEXTURE_2D, tex_bldg[8]);
			glBegin(GL_QUADS);
//			glBox(32 * bldloc[i][1] - (bldg_w[0] / 2) + 16,32 * bldloc[i][2] - (bldg_h[0] / 2) + 16,bldg_w[0],bldg_h[0]);
			glEnd();
		}

		glEndList();
		glNewList(list_cappoint + (3 * i) + 2, GL_COMPILE);
		{
			glBindTexture(GL_TEXTURE_2D, tex_bldg[9]);
			glBegin(GL_QUADS);
//			glBox(32 * bldloc[i][1] - (bldg_w[0] / 2) + 16,32 * bldloc[i][2] - (bldg_h[0] / 2) + 16,bldg_w[0],bldg_h[0]);
			glEnd();
		}

		glEndList();
	}

	// Well.  All done.
}

// Helper function: update fog-of-war texture based on HQ and capture points.
static void draw_fog(int team)
{
	// create an all-alpha buffer
	unsigned char * fogbuf = (unsigned char *)malloc(128 * 128);
	memset(fogbuf, 0xFF, 128 * 128);
	// for each point owned by this team, clear out a section of the mask
	const int r = 12;

	for (int y_off = 1; y_off <= r; y_off ++) {
		float x_off = sqrt(float((r * r) - (y_off * y_off)));

		for (int i = 0; i < 15; i ++) {
			if (bldloc[i][0] || i < 4) {
				int x1 = bldloc[i][1] - (x_off);

				if (x1 < 0) x1 = 0;

				int x2 = bldloc[i][1] + (x_off);

				if (x2 > 99) x2 = 99;

				if (bldloc[i][2] + y_off < 100)
					memset(& fogbuf[(bldloc[i][2] + y_off) * 128 + x1], 0, x2 - x1);

				if (bldloc[i][2] - y_off >= 0)
					memset(& fogbuf[(bldloc[i][2] - y_off) * 128 + x1], 0, x2 - x1);
			}
		}

		// center line
		for (int i = 0; i < 15; i ++) {
			if (bldloc[i][0] || i < 4) {
				int x1 = bldloc[i][1] - r;

				if (x1 < 0) x1 = 0;

				int x2 = bldloc[i][1] + r;

				if (x2 > 99) x2 = 99;

				memset(& fogbuf[bldloc[i][2] * 128 + x1], 0, x2 - x1);
			}
		}
	}

	// upload the new tex
	glBindTexture(GL_TEXTURE_2D, tex_fog);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, 128, 128, 0, GL_ALPHA, GL_UNSIGNED_BYTE, fogbuf);
	free(fogbuf);
}

// Helper function: creates the minimap display list.
static void draw_minimap()
{
	unsigned int i, j;
	Uint32 * ts_pix; //, pcolor;
	// Create a temp surface, where we will draw the minimap.
	SDL_Surface * ts = SDL_CreateRGBSurface(SDL_SWSURFACE, 128, 128, 32,
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
	clr_tile[0] = SDL_MapRGB(ts->format, 0, 0, 255);
	clr_tile[1] = SDL_MapRGB(ts->format, 255, 255, 0);
	clr_tile[2] = SDL_MapRGB(ts->format, 128, 64, 0);
	clr_tile[3] = SDL_MapRGB(ts->format, 128, 255, 128);
	Uint32 clr_tree = SDL_MapRGB(ts->format, 0, 255, 0);
	// Lock the surface and get access to its pixel structure.
	SDL_LockSurface(ts);
	ts_pix = (Uint32 *)ts->pixels;

	// Put colors down on the minimap.
	for (j = 0; j < MAP_Y; j++) {
		for (i = 0; i < MAP_X; i++) {
			if (trees[i][j]) ts_pix[j * 128 + i] = clr_tree;
			else ts_pix[j * 128 + i] = clr_tile[map[i][j]];
		}
	}

	SDL_UnlockSurface(ts);
//    SDL_SaveBMP(ts, "minimap.bmp");
	glBindTexture(GL_TEXTURE_2D, tex_minimap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, ts->w, ts->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, ts->pixels);
	SDL_FreeSurface(ts);
	glNewList(list_minimap, GL_COMPILE);
	{
		// Disable simple Alpha test
		glDisable(GL_ALPHA_TEST);
		// enable blending for detailed transparency
		glEnable(GL_BLEND);
		// bind minimap texture
		glBindTexture(GL_TEXTURE_2D, tex_minimap);
		glColor4f(1.0f, 1.0f, 1.0f, 0.75f);
		// draw a quad at top-left.
		glBegin(GL_QUADS);
		glTexCoord2f(0, 0);
		glVertex2i(SCREEN_X - 100, 0);
		glTexCoord2f(0.78125, 0);
		glVertex2i(SCREEN_X, 0);
		glTexCoord2f(0.78125, 0.78125);
		glVertex2i(SCREEN_X, 100);
		glTexCoord2f(0, 0.78125);
		glVertex2i(SCREEN_X - 100, 100);
		glEnd();
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		// Turn blending back off, we don't need it any more, probably.
		glDisable(GL_BLEND);
	}

	glEndList();
	// Got it, see ya later bye.
}

// Client-side map loading function.
static char load_map(const char * name)
{
	int i, j;
	FILE * fp = fopen(name, "rb");

	if (fp == NULL) {
		fprintf(stderr, "Error opening map %s!\n", name);
		return 0;
	}

	for (j = 0; j < MAP_Y; j++) {
		for (i = 0; i < MAP_X; i += 2) {
			// Read next (binary) character from file.
			unsigned char incoming = fgetc(fp);
			// Maps are 100x100 with up to 16 choices for tile.
			//  Pack two tiles in a nibble.  Because, you know... space is valuable?
			int low_tile = (incoming & 0xF0) >> 4;
			int high_tile = (incoming & 0x0F);

			// Special purpose for "trees".
			if (low_tile > 3) {
				trees[i][j] = 1;
				map[i][j] = 0;
			} else {
				trees[i][j] = 0;
				map[i][j] = low_tile;
			}

			if (high_tile > 3) {
				trees[i + 1][j] = 1;
				map[i + 1][j] = 0;
			} else {
				trees[i + 1][j] = 0;
				map[i + 1][j] = high_tile;
			}
		}
	}

	for (i = 0; i < 15; i++) {
		for (j = 0; j < 3; j++)
			bldloc[i][j] = (int)fgetc(fp);
	}

	/*		initang[0] = (unsigned char)fgetc(fp);
		initang[1] = (unsigned char)fgetc(fp);*/
	fclose(fp);
	draw_map();
	draw_minimap();
	draw_fog(0);
	return 1;
}

static int game_connect()
{
	int i;
	//char buffer[80];
	char map_name[80] = "maps/x-isle.map";

	if (env.multiplayer) {
		// connect to remote server
		unsigned short int PORT = DEFAULT_PORT;

		for (i = 0; i < (int)strlen(env.HOSTNAME); i++)
			if (env.HOSTNAME[i] == ':' || env.HOSTNAME[i] == ' ') {
				PORT = atoi(&env.HOSTNAME[i + 1]);
				env.HOSTNAME[i] = '\0';
				break;
			}

		if (!(sd = SDLNet_UDP_Open(0))) {
			fprintf(stderr, "SDLNet_UDP_Open: %s\n", SDLNet_GetError());
			return 0;
		}

		if (SDLNet_ResolveHost(&srvadd, env.HOSTNAME, PORT)) {
			fprintf(stderr, "SDLNet_ResolveHost(%s %d): %s\n", env.HOSTNAME, PORT, SDLNet_GetError());
			return 0;
		}

		/* Allocate memory for the packet */
		if (!(p = SDLNet_AllocPacket(512))) {
			fprintf(stderr, "SDLNet_AllocPacket: %s\n", SDLNet_GetError());
			return 0;
		}

		p->address.port = srvadd.port;
		p->address.host = srvadd.host;
		p->data[0] = 'L';
		p->len = 1;
		SDLNet_UDP_Send(sd, -1, p);
	} else {
		// need to start up a local engine.
//		sprintf(map_name,"maps/sp/%d.map",level);
		init_game(map_name, 31, 1);
		// in single player ID is always 0
		slot = connect_player();
	}

	health = 100;
	ammo = 0;
	cash[0] = cash[1] = CASH_START;
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
	if (env.multiplayer) {
		p->data[0] = 'D';
		p->len = 1;
		SDLNet_UDP_Send(sd, -1, p);
		SDLNet_FreePacket(p);
	} else
		shutdown_game();
}

static void game_quit()
{
	int i;
	glDeleteLists(list_tree, 1);
	glDeleteLists(list_map, 1);
	glDeleteLists(list_cappoint, 9);
	glDeleteLists(list_sidebar, 2);
	glDeleteLists(list_fog, 1);
	glDeleteLists(list_minimap, 1);
	glDeleteLists(list_hud, 1);
	glDeleteLists(list_digit, 10);
	glDeleteLists(list_direction, 1);

	for (i = 0; i < 2; i ++)
		glDeleteLists(list_unit[i], NUM_UNITS);

//	glDeleteLists(list_unit_shadow,NUM_UNITS);
	glDeleteLists(list_bullet, NUM_BULLETS);
	glDeleteLists(list_particle, NUM_PARTICLES);
	glDeleteTextures(10, tex_digit);
	glDeleteTextures(4, tex_hud);

	for (i = 0; i < 2; i ++)
		glDeleteTextures(NUM_UNITS, tex_unit[i]);

	glDeleteTextures(NUM_BULLETS, tex_bullet);
	glDeleteTextures(NUM_BLDGS, tex_bldg);
	glDeleteTextures(NUM_TILES + 4, tex_tile);
	glDeleteTextures(NUM_PARTICLES, tex_particle);
	glDeleteTextures(2, tex_sidebar);
	glDeleteTextures(1, &tex_fog);
	glDeleteTextures(1, &tex_minimap);
	glDeleteTextures(1, &tex_dir);

	for (i = 0; i < NUM_SFX; i++) {
		if (sfx[i]) {
			Mix_FreeChunk(sfx[i]);
			sfx[i] = NULL;
		}
	}
}

static void game_draw()
{
	int i;
	/*	int j, drawTrail=0; */
//	int startx, starty, endx, endy;
//	struct human *obj;
	float rate = (float)(SDL_GetTicks() - last_update_tick) / UPDATE_FREQ;
//	printf("rate = %f\n", rate);
	// "rate" is lerp-point, between 0 and 1.
	//float rate=((float)ticks-SDL_GetTicks())/ ((smooth[0]+smooth[1]+smooth[2]+smooth[3]+smooth[4]) / 5);
	// Set up the orthographic perspective.  Either fully zoomed out (in HQ or cheating), or
	//  set by globalZoom parameter.
//	glClear(GL_COLOR_BUFFER_BIT);// | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	if (status == 1 || status == 3 || iSeeAll) {
		glOrtho(0, 3200 * 4 / 3, 3200, 0, -1.0, 1.0);
		glMatrixMode(GL_MODELVIEW);
		glClear(GL_COLOR_BUFFER_BIT);// | GL_DEPTH_BUFFER_BIT);
		glLoadIdentity();
	} else {
		float globalzoom = lerp(oldzoom, newzoom, rate);
		globalzoom = 1.0f;
		float camW = SCREEN_X / 8 * 4 * globalzoom;
		float camH = SCREEN_X / 8 * 3 * globalzoom;
		glOrtho(-camW, camW, camH, -camH, -1.0, 1.0);
		glMatrixMode(GL_MODELVIEW);
		glClear(GL_COLOR_BUFFER_BIT);// | GL_DEPTH_BUFFER_BIT);
		glLoadIdentity();

//		glTranslatef(-1500,-1500,0);
//		if (player_e) {
//			glTranslatef(-player_e->x,-player_e->y,0);
//		} else {
		if (status == 2 && player != NULL) {
			glTranslatef(-(player->x + rate * player->dx) + (shake * ((float)rand() / RAND_MAX - .5)),
				-(player->y + rate * player->dy) + (shake * ((float)rand() / RAND_MAX - .5)), 0);
		} else
			glTranslatef(-1600, -1600, 0);
	}

	// Draw landscapes.
	glCallList(list_map);

	// Ground-based objects
	// Capture points
	for (i = 0; i < 3; i ++)
		glCallList(list_cappoint + (3 * i) + owner[i]);

	// Everything from here on uses alpha test or blend.
	glEnable(GL_ALPHA_TEST);
	// All visible objects
	/*
	struct human * h = human_list;
	while (h != NULL)
	{
		draw_human(h, rate);
		h = h->next;
	}
	*/
	// Ground-based units
	struct unit * v = unit_list;

	while (v != NULL) {
		if (!(v->o->flags & PLANE) || v->speed <= 30) {
//			draw_shadow(obj);
			draw_unit(v, rate);
		}

		v = v->next;
	}

	struct projectile * j = projectile_list;

	while (j != NULL) {
		draw_projectile(j, rate);
		j = j->next;
	}

	// Trees
	glCallList(list_tree);
	// Airborne objects (planes)
	/*
	obj = objects[OBJ_unit];
	while (obj != NULL)
	{
		if (obj->sprite >= 15 && obj->speed > 30) {
			draw_shadow(obj);
			draw_object(obj);
		}
		obj = obj->next;
	}
	*/
	glDisable(GL_ALPHA_TEST);
	glEnable(GL_BLEND);
	// Particles
	struct particle * t = particle_list;

	while (t != NULL) {
		draw_particle(t, rate);
		t = t->next;
	}

	glDisable(GL_BLEND);
	glColor3f(1, 1, 1);

	// Time to draw the UI.
	//gotta go back to the old way to draw the cursor right size, and UI.
	/*
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, SCREEN_X, SCREEN_Y, 0, -1.0, 1.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	*/
	if (status == 1 || status == 3) {
		if (status == 1)
			// draw Redeploy sidebar if not in game yet
			glCallList(list_sidebar);
		else if (status == 3)
			// if player is commander, draw hq menu
			glCallList(list_sidebar + 1);

		// Draw fog of war overlay
		glCallList(list_fog);
	}

	// Draw HUD
	glCallList(list_hud);
	// HUD items (numbers)
	//unsigned int base = 0;
	draw_number(abase, 40, 0);
	draw_number(nbase, 120, 0);
	draw_number(ammo, 40, 40);
	draw_number(health, 40, 80);

	// if player is dead, draw respawn countdown timer
	if (status == 1)
		draw_number(int(respawn_time * UPDATE_FREQ / 1000 + .5), 650, 275);

	// Team cash
	if (status == 1 || status == 3) {
		draw_number(cash[0], 640, 20);
		draw_number(cash[1], 640, 80);
	} else {
		unsigned int x;

		if (cash[0] > 9999) x = 640;
		else if (cash[0] > 999) x = 672;
		else if (cash[0] > 99) x = 704;
		else if (cash[0] > 9) x = 736;
		else x = 768;

		draw_number(cash[0], x, 104);

		if (cash[1] > 9999) x = 640;
		else if (cash[1] > 999) x = 672;
		else if (cash[1] > 99) x = 704;
		else if (cash[1] > 9) x = 736;
		else x = 768;

		draw_number(cash[1], x, 140);
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
	if (bChatting == 1) {
		glColor3f(1, 1, 0);
		glPrint(144, 536, "CHAT:");
		glPrint(160, 536, cMesg);
		glColor3f(1, 1, 1);
	} else if (bChatting == 2) {
		glColor3f(1, 1, 0);
		glPrint(144, 536, "TEAM CHAT:");
		glPrint(160, 536, cMesg);
		glColor3f(1, 1, 1);
	}

	// Draw cursor.
	glDrawCursor(mx, my);
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
	/*
	frames++;
	if (fpsticks + 1000 < SDL_GetTicks()) {
		fpsticks = SDL_GetTicks();
		printf("FPS: %d\n",frames);
		frames=0;
	}
	*/
}

static unsigned int rd_int32(unsigned char * p)
{
	return (p[0] << 24) |
		(p[1] << 16) |
		(p[2] << 8) |
		p[3];
}

static unsigned int rd_int24(unsigned char * p)
{
	return (p[0] << 16) |
		(p[1] << 8) |
		p[2];
}

static unsigned short rd_int16(unsigned char * p)
{
	return	(p[0] << 8) |
		p[1];
}

static struct unit_sub * create_sub(const struct object_sub * o) //, struct unit_sub * parent)
{
	struct unit_sub * c = (struct unit_sub *) malloc(sizeof(struct unit_sub));
	c->o = o;
	c->angle = 0;
	c->flags = 0;
//	c->parent = parent;

	if (o->sub != NULL)
		c->sub = create_sub(o->sub);
	else
		c->sub = NULL;

	// TODO make this iterative?
	if (o->next != NULL)
		c->next = create_sub(o->next);
	else
		c->next = NULL;

	return c;
}

void free_sub(struct unit_sub * c)
{
	while (c != NULL) {
		free_sub(c->sub);
		struct unit_sub * n = c->next;
		free(c);
		c = n;
	}
}

static unsigned char * unserialize_sub(unsigned char * payload, struct unit_sub * v)
{
	do {
		unsigned short occupant_angle = rd_int16(payload);
		payload += 2;
		v->flags = occupant_angle >> 15;
		v->angle = (occupant_angle & 0x7FFF) * M_PI / 16384.0;
//		printf(" unserialized angle = %f\n", v->angle);

//		printf(" . sub: occ=%u ang=%f\n", v->occupied, v->angle);

		if (v->sub)
			payload = unserialize_sub(payload, v->sub);

		v = v->next;
	} while (v != NULL);

	return payload;
}

static void unserialize(unsigned char * payload)
{
	int i;
	struct object * obj;
	// Unserializes the server-response for "game update".
	//  Global info
	// Unserialize capture-points owner
	//  Note that if there's a change in ownership, we need to update the fog-of-war too.
	int owner_change = 0;

	for (i = 0; i < 3; i ++) {
		int prev_owner = owner[i];
		owner[i] = (*payload >> (i * 2)) & 0x3;

		if (prev_owner != owner[i]) owner_change = 1;
	}

	payload ++;

	if (owner_change) draw_fog(0);

	// Cash
	for (i = 0; i < 2; i ++) {
		cash[i] = rd_int16(payload);
		payload += 2;
	}

	// Player status
	status = *payload;
	payload ++;

	switch (status) {
	case 1:
		respawn_time = *payload;
		payload ++;
		break;

	case 2:
		// Player entity ID and other local info
		id = rd_int16(payload);
		payload += 2;
		seat = *payload;
		payload ++;
		health = *payload;
		payload ++;
		ammo = *payload;
		payload ++;
		break;

	case 3:
		// Player entity ID and other local info
		break;
	}

	player = NULL;
	// Unit updates.
	{
		//  A new entity list where we stash everything we already knew about or created
		struct unit * l = NULL;
		unsigned char unit_count = *payload;
		payload ++;

		for (i = 0; i < unit_count; i ++) {
			unsigned short unit_id = rd_int16(payload);
			payload += 2;
			unsigned char team_type = *payload;
			payload ++;
			// Try to locate the object in the local listing.
			struct unit * prev = NULL;
			struct unit * v = unit_list;

			while (v != NULL) {
				if (v->id == unit_id) {
					// matched!  splice object out of prev. entity list
					if (prev)
						prev->next = v->next;
					else
						unit_list = v->next;

					break;
				}

				// not matched, keep looking
				prev = v;
				v = v->next;
			}

			if (v == NULL) {
				// didn't find it, need to clone a fresh copy from the library
				unsigned char type = team_type & 0x7F;
				// search the library for an object of this type
				const struct object_base * o = object_root;

				while (o != NULL && o->type != type)
					o = o->next;

				if (o == NULL) {
					// this is a fatal error!  the server told us about a unit ID we have never heard of
					fprintf(stderr, "ERROR: Server requested unit type %d which is unknown\n", type);
					return;
//					gamestate = gs_exit;
				}

				v = (struct unit *) malloc(sizeof(struct unit));
				// set the ptr to the library object
				v->o = o;
				v->id = unit_id;
				v->sub = create_sub(o->sub);
			}

			// unpack the payload and update the item
			v->team = team_type >> 7;
			v->flags = 0; //team_occupied_type & 0x40;
//			v->type = team_occupied_type & 0x7F;
			// object x, y, angle, and speed
			unsigned int xy = rd_int24(payload);
			payload += 3;
			v->x = xy >> 12;
			v->y = xy & 0xFFF;
			// speed max is +-64 (7 bits)
			// angle in 512 steps (9 bits)
			unsigned short angle_speed = rd_int16(payload);
			payload += 2;
			v->angle = (angle_speed & 0x01FF) * M_PI / 256.0;
			v->speed = ((short)angle_speed) >> 9;
			//printf("Parsed unit: %u, team=%u, type=%u, occupied=%u, x=%u, y=%u, angle=%f, speed=%d\n", v->id, v->team, v->o->type, 0, v->x, v->y, v->angle, v->speed);
			// and now: the subchild
			payload = unserialize_sub(payload, v->sub);
			// place object into New Entity List
			v->next = l;
			l = v;
			// computations
			v->dx = v->speed * cos(v->angle);
			v->dy = v->speed * sin(v->angle);

			// cache the player entity here for camera centering etc
			if (status == 2 && unit_id == id) {
				player = v;
//				camX = h->x;
//				camY = h->y;
			}
		}

		// delete old entity_list
		while (unit_list != NULL) {
			struct unit * v = unit_list->next;
			free_sub(unit_list->sub);
			free(unit_list);
			unit_list = v;
		}

		unit_list = l;
	}

	// Projectile updates.
	{
		//  A new list
		struct projectile * l = NULL;
		unsigned char projectile_count = *payload;
		payload ++;

		for (i = 0; i < projectile_count; i ++) {
			unsigned short projectile_id = rd_int16(payload);
			payload += 2;
			// Try to locate the object in the local listing.
			struct projectile * prev = NULL;
			struct projectile * j = projectile_list;

			while (j != NULL) {
				if (j->id == projectile_id) {
					// matched!  splice object out of prev. entity list
					if (prev)
						prev->next = j->next;
					else
						projectile_list = j->next;

					break;
				}

				// not matched, keep looking
				prev = j;
				j = j->next;
			}

			if (j == NULL) {
				// didn't find it, need to clone a fresh copy from the library
				j = (struct projectile *)malloc(sizeof(struct projectile));
				j->id = projectile_id;
			}

			// unpack the payload and update the item
			unsigned int xy = rd_int24(payload);
			payload += 3;
			j->x = xy >> 12;
			j->y = xy & 0xFFF;
			// 24 bits -
			//  4 bits for Type
			//  1 bit for Fractional Life
			// 11 bits Angle (2048 steps)
			//  8 bits Life
			unsigned char type_float_angle = *payload;
			payload ++;
			j->type = type_float_angle >> 4;
			j->angle = (((type_float_angle & 0x07) << 8) | *payload) * (M_PI / 1024);
			payload ++;

			if (type_float_angle & 0x08)
				j->life = (*payload) / 256.0;
			else
				j->life = *payload;

			payload ++;
			printf("projectile %u: type=%u, xy=%u, %u, angle=%f, life=%f\n", j->id, j->type, j->x, j->y, j->angle, j->life);
			// place object into New Entity List
			j->next = l;
			l = j;
			// computations
			j->dx = projectile_detail[j->type].speed * cos(j->angle);
			j->dy = projectile_detail[j->type].speed * sin(j->angle);
		}

		// delete old entity_list
		while (projectile_list != NULL) {
			struct projectile * j = projectile_list->next;
			free(projectile_list);
			projectile_list = j;
		}

		projectile_list = l;
	}

	// effect updates
	{
		unsigned char effect_count = *payload;
		payload ++;

		for (i = 0; i < effect_count; i ++) {
			// unpack the payload and update the item
			unsigned int xy = rd_int24(payload);
			payload += 3;
			unsigned short x = xy >> 12;
			unsigned short y = xy & 0xFFF;
			unsigned char type = *payload;
			payload ++;
			float delay = *payload / 256.0f;
			payload ++;
			effect * e = (struct effect *)malloc(sizeof(struct effect));
			e->x = x;
			e->y = y;
			e->type = type;
			e->time = delay;
			//printf("effect: type=%u, xy=%u, %u, time=%f\n", e->type, e->x, e->y, e->time);
			e->next = effect_list;
			effect_list = e;
		}
	}

	// /////////////
	// particle updates?
	{
		struct particle * prev = NULL;
		struct particle * t = particle_list;

		while (t != NULL) {
			t->life -= 1;

			if (t->life < 0) {
				if (t == particle_list) {
					particle_list = t->next;
					free(t);
					t = particle_list;
				} else {
					prev->next = t->next;
					free(t);
					t = prev->next;
				}
			} else {
				t->x += t->dx;
				t->dx *= .5;
				t->y += t->dy;
				t->dy *= .5;
				prev = t;
				t = t->next;
			}
		}
	}

//	if (player_e) {
//		printf("Player %d, %d, at %f, %f / %f, %f\n", player_e->type, player_e->team, player_e->x, player_e->y, player_e->angle, player_e->speed);
//	}
}

static char game_update()
{
	static Uint32 next_control_tick = 0;
	static Uint32 next_server_tick = 0;
	char retval = gs_game;
	char buffer[80];
	unsigned int i;
	/* Periodic client-side activity here: things like particle trails, etc. */
	ticks = SDL_GetTicks();
	float rate = (float)(ticks - last_update_tick) / UPDATE_FREQ;
	struct projectile * j = projectile_list, * prev = NULL;

	while (j != NULL) {
		if (j->life < rate) {
//			printf(" - Expire projectile J as life %f < rate %f\n", j->life, rate);
			// stop bullet here - splice it out of list
			if (j == projectile_list) {
				projectile_list = j->next;
				free(j);
				j = projectile_list;
			} else {
				prev->next = j->next;
				free(j);
				j = prev->next;
			}
		} else {
//			printf(" - Continue as J as life %f >= rate %f\n", j->life, rate);
			if ((projectile_detail[j->type].flags & TRAIL) && (rand() % 3) == 0) {
				/* make a particle */
				struct particle * t = (struct particle *)malloc(sizeof(struct particle));
				t->type = 0;
				float rgb = (0.5f * (float)rand() / RAND_MAX) + 0.5f;
				t->r = t->g = t->b = rgb;
				float q = (float)rand() / RAND_MAX;
				t->angle = j->angle + (3 * M_PI_4) + (q * M_PI_2);
				t->x = j->x + rate * j->dx;
				t->y = j->y + rate * j->dy;
				q = (float)rand() / RAND_MAX;
				t->scale = q / 2;
				t->speed = 10 * (1.5 - q);
				t->life = 2 + rate + q;
				t->dx = t->speed * cos(t->angle);
				t->dy = t->speed * sin(t->angle);
				t->next = particle_list;
				particle_list = t;
			}

			prev = j;
			j = j->next;
		}
	}

	// special effects if it's time
	{
		struct effect * e = effect_list, * prev = NULL;

		while (e != NULL) {
			if (e->time < rate) {
				// it is time
				if (e->type == 1) {
					// explosion
					// screen shake
					float dist = sqr(player->x - e->x) + sqr(player->y - e->y);

//				printf("Dist is %d-%d %d-%d %f (%f)\n", player->x, e->x, player->y, e->y, dist, sqrt(dist));
					/*				if (dist < 250000) {
										shake += (500 - sqrt(dist)) / 5;
									}
					*/
					for (int i = 0; i < 128; i ++) {
						struct particle * t = (struct particle *)malloc(sizeof(struct particle));
						t->type = 0;

						if (i < 96) {
							t->r = 1;
							t->g = (float)rand() / RAND_MAX;
							t->b = 0;
						} else
							t->r = t->g = t->b = (float)rand() / RAND_MAX;

						t->x = e->x;
						t->y = e->y;
						float q = (float)rand() / RAND_MAX;
						t->angle = rand() / ((float)RAND_MAX / M_TAU);
//					printf(" . particle created at xy=%u, %u, angle=%f\n", t->x, t->y, t->angle);
						t->scale = q;
						t->speed = 48 * (1.1 - q);
						t->life = 4 + q;
						t->dx = t->speed * cos(t->angle);
						t->dy = t->speed * sin(t->angle);
//					printf(" . . dx=%f dy=%f (dist=%f)\n", t->dx, t->dy, (t->dx * t->dx) + (t->dy * t->dy));
						t->next = particle_list;
						particle_list = t;
					}
				}

				// and delete the effect, it's triggered now
				if (e == effect_list) {
					effect_list = e->next;
					free(e);
					e = effect_list;
				} else {
					prev->next = e->next;
					free(e);
					e = prev->next;
				}
			} else {
				prev = e;
				e = e->next;
			}
		}
	}

	/* The next thing to do is to check for player inputs. */
	SDL_Event event;

	while (SDL_PollEvent(&event)) {
		switch (event.type) {
		case SDL_KEYDOWN:
			if (!bChatting) {
				if (event.key.keysym.sym == SDLK_LEFT)  //|| event.key.keysym.sym == SDLK_A)
					localcontrols[0] = 1;
				else if (event.key.keysym.sym == SDLK_RIGHT) // || event.key.keysym.sym == SDLK_D)
					localcontrols[1] = 1;
				else if (event.key.keysym.sym == SDLK_UP) // || event.key.keysym.sym == SDLK_W)
					localcontrols[2] = 1;
				else if (event.key.keysym.sym == SDLK_DOWN) // || event.key.keysym.sym == SDLK_S)
					localcontrols[3] = 1;
				else if (event.key.keysym.sym == SDLK_TAB)
					localcontrols[6] = 1;
				else if (event.key.keysym.sym == SDLK_RETURN)
					localcontrols[7] = 1;
			} else {
				if (event.key.keysym.sym == SDLK_RETURN) {
					if (env.multiplayer) {
						if (bChatting == 1)
							sprintf((char *)p->data, "C%s", cMesg);
						else
							sprintf((char *)p->data, "T%s", cMesg);

						p->len = strlen(cMesg) + 2;
						p->address.host = srvadd.host;	/* Set the destination host */
						p->address.port = srvadd.port;	/* And destination host */
						SDLNet_UDP_Send(sd, -1, p); /* This sets the p->channel */
					} else {
						if (strcmp(cMesg, "raise the roof") == 0) iSeeAll = !iSeeAll;
						else if (strcmp(cMesg, "victory is mine") == 0) retval = gs_win;
						else if (strcmp(cMesg, "game over man") == 0) retval = gs_lose;
					}

					bChatting = 0;
					SDL_EnableUNICODE(0);
					memset(cMesg, 0, 50);
				} else if (event.key.keysym.sym == SDLK_BACKSPACE && strlen(cMesg) > 0)
					cMesg[strlen(cMesg) - 1] = '\0';
				else if (strlen(cMesg) < 49 && event.key.keysym.unicode < 0x80 && event.key.keysym.unicode >= 0x20)
					cMesg[strlen(cMesg)] = (char)(event.key.keysym.unicode);
			}

			break;

		case SDL_KEYUP:
			if (event.key.keysym.sym == SDLK_ESCAPE) {
				if (bChatting) {
					bChatting = 0;
					SDL_EnableUNICODE(0);
					memset(cMesg, 0, 50);
				} else
					retval = gs_title;
			} else if (event.key.keysym.sym == SDLK_LEFT) // || event.key.keysym.sym == SDLK_A)
				localcontrols[0] = 0;
			else if (event.key.keysym.sym == SDLK_RIGHT) // || event.key.keysym.sym == SDLK_D)
				localcontrols[1] = 0;
			else if (event.key.keysym.sym == SDLK_UP) // || event.key.keysym.sym == SDLK_W)
				localcontrols[2] = 0;
			else if (event.key.keysym.sym == SDLK_DOWN) // || event.key.keysym.sym == SDLK_S)
				localcontrols[3] = 0;
			else if (event.key.keysym.sym == SDLK_TAB)
				localcontrols[6] = 0;
			else if (event.key.keysym.sym == SDLK_RETURN)
				localcontrols[7] = 0;
			else if (event.key.keysym.sym == SDLK_c) {
				bChatting = 1;
				SDL_EnableUNICODE(1);
			} else if (env.multiplayer && (event.key.keysym.sym == SDLK_t)) {
				bChatting = 2;
				SDL_EnableUNICODE(1);
			}

			break;

		case SDL_MOUSEBUTTONDOWN:
			mx = event.motion.x;
			my = event.motion.y;

			if (status == 1) {
				if (event.button.button == SDL_BUTTON_LEFT)
					localcontrols[4] = 1;
			} else if (status == 2) {
				// figure out my new facing.
				// localaim is an absolute (heading)
				localaim = atan2(my - 300.0f, mx - 400.0f);

				if (localaim < 0) localaim += M_TAU;

				// convert this into a bearing, for display
				/*
				float aim = localaim - player->angle;
				// get the range back to -pi to +pi
				if (aim < -M_PI) aim += M_TAU;
				else if (aim > M_PI) aim -= M_TAU;
				*/
				// cap range to -pi/2 to pi/2 for our controlled player
				/*
				if (aim > M_PI/2) player->aim = M_PI/2;
				else if (aim < -M_PI/2) player->aim = -M_PI/2;
				else player->aim = aim;
				*/

				if (event.button.button == SDL_BUTTON_LEFT)
					localcontrols[4] = 1;
				else if (event.button.button == SDL_BUTTON_RIGHT)
					localcontrols[5] = 1;
			}

			/*				if (status == 3) {
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
							*/
			break;

		case SDL_MOUSEBUTTONUP:
			mx = event.motion.x;
			my = event.motion.y;

			if (status == 1) {
				if (event.button.button == SDL_BUTTON_LEFT)
					localcontrols[4] = 0;
			} else if (status == 2) {
				// figure out my new facing.
				// localaim is an absolute (heading)
				localaim = atan2(my - 300.0f, mx - 400.0f);

				if (localaim < 0) localaim += M_TAU;

				// convert this into a bearing, for display
				float aim = localaim - player->angle;

				// get the range back to -pi to +pi
				if (aim < -M_PI) aim += M_TAU;
				else if (aim > M_PI) aim -= M_TAU;

				// cap range to -pi/2 to pi/2 for our controlled player
				/*
				if (aim > M_PI/2) player->aim = M_PI/2;
				else if (aim < -M_PI/2) player->aim = -M_PI/2;
				else player->aim = aim;
				*/

				if (event.button.button == SDL_BUTTON_LEFT)
					localcontrols[4] = 0;
				else if (event.button.button == SDL_BUTTON_RIGHT)
					localcontrols[5] = 0;
			}

			break;

		case SDL_MOUSEMOTION:
			mx = event.motion.x;
			my = event.motion.y;

			if (status == 2) {
				// figure out my new facing.
				// localaim is an absolute (heading)
				localaim = atan2(my - 300.0f, mx - 400.0f);

				if (localaim < 0) localaim += M_TAU;

				// convert this into a bearing, for display
				float aim = localaim - player->angle;

				// get the range back to -pi to +pi
				if (aim < -M_PI) aim += M_TAU;
				else if (aim > M_PI) aim -= M_TAU;

				// cap range to -pi/2 to pi/2 for our controlled player
				/*
				if (aim > M_PI/2) player->aim = M_PI/2;
				else if (aim < -M_PI/2) player->aim = -M_PI/2;
				else player->aim = aim;
				*/
			}

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
	if (env.multiplayer) {
		if (ticks >= next_control_tick) {
			next_control_tick = ticks + CONTROL_FREQ;

			if (status == 1) {
				p->data[0] = localcontrols[4];
				p->data[1] = localcontrols[5];
				p->data[2] = localcontrols[8];
				p->len = 3;
			} else if (status == 2) {
				// A regular control update.
				//  A two-byte packet: bitfields packed in byte 1, and byte 2 holds angle as char.
				p->data[0] = 0;

				for (i = 0; i < 8; i++)
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

		if (SDLNet_UDP_Recv(sd, p) && p->len > 0) {
			unsigned char * payload = p->data + 1;

			switch (p->data[0]) {
			case 'C':	// Chat message received
				message_post(0, (char *)&payload);
				break;

			case 'M':	// Map change message
				sprintf(buffer, "maps/%s", (char *)&payload);
				load_map(buffer);
				break;

			case 'U':	// Game update.
				last_update_tick = ticks;
				unserialize(payload);
				break;

			default:
				break;
			}
		}
	} else {
		// we just control the local game directly
		if (status == 1) {
			control_game_deploy(slot,
				mx / 6,
				min(my / 6, 100),
				localcontrols[4]
			);
		} else if (status == 2) {
			control_game_regular(slot,
				localcontrols[2] ? 1 : (localcontrols[3] ? -1 : 0),
				localcontrols[1] ? 1 : (localcontrols[0] ? -1 : 0),
				localcontrols[4],
				localcontrols[5],
				localcontrols[6],
				localcontrols[7],
				localaim);
		} else if (status == 3) {
			control_game_hq(slot,
				mx / 6,
				min(my / 6, 100),
				0,
				0,
				localcontrols[7]);
		}

		if (next_server_tick <= ticks) {
			shake /= 2;
			last_update_tick = ticks;
			next_server_tick = ticks + UPDATE_FREQ;
			update_game();
			// serialize the game state from the engine, then unpack it again
			unsigned char payload[1500];
			int payload_size = serialize_game(slot, payload);
			unserialize(payload);
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
	if (game_connect()) {
		retval = gs_game;

		// play the game
		while (retval == gs_game) {
			game_draw();
			retval = game_update();
			/* Don't run too fast */
			SDL_Delay(1);
		}

		// disconnect from game brain
		game_disconnect();
	}

	// quit the game
	game_quit();
	return retval;
}
