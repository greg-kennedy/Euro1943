/* EURO1943 - GREG KENNEDY
	http://greg-kennedy.com */
	
/* common_client.h - Declarations common to Client modules goes here. */

#ifndef COMMON_CLIENT_H_
#define COMMON_CLIENT_H_

// System includes
#include <SDL/SDL.h>
#include <SDL/SDL_opengl.h>
#include <SDL/SDL_mixer.h>

/* Global (client and server) includes */
#include "common.h"

// gamestates
#define	gs_exit -1

#define	gs_title 0
#define gs_multimenu 1
#define gs_options 2
#define gs_cutscene 3
#define gs_win 4
#define gs_lose 5
#define gs_game 10

// How many levels in game?
#define MAX_LEVEL 3

// Size of virtual canvas
#define SCREEN_X 800
#define SCREEN_Y 600

/* Struct declaration of common game objects
    These are shared across multiple major states at once */
struct env_t {

	// Global variables, shared across multiple subsystems.
	// Does audio work OK?
	unsigned char ok_audio, ok_music;
	// Holds music and sound effect volume.  Set to 0 to disable.
	unsigned char volume;

	// Is the network OK?
	unsigned char ok_network;
	// Location of remote OverServer host, port
	char OS_LOC[80];
	unsigned short OS_PORT;

	// Is this a multiplayer game?
	unsigned char multiplayer;
	union {
		// Which level are we on, if not?
		unsigned char level;
		// but if so Who are we connected to?
		char HOSTNAME[80];
	};

	// Key bindings
//	SDLKey keys[10];
};

// set up and tear down common shared objects
void init_common();
void quit_common();

// load texture, return opengl texture id
GLuint load_texture(const char *fname, GLuint min_filt, GLuint max_filt);
GLuint load_texture_extra(const char *fname, GLuint min_filt, GLuint max_filt, int *orig_w, int *orig_h);

// GL wrapper functions to do some common actions
void glPrint(GLshort x, GLshort y, const char *text);
void glDrawCursor(GLfloat x, GLfloat y);
void glBox(GLuint texture, GLushort w, GLushort h);
void glBoxPos(GLuint texture, GLushort w, GLushort h, GLshort x, GLshort y);

// message-box (speech bubble) functions
void message_clear();
void message_post(int speaker, const char *message);
void message_draw();

// music handler functions
Mix_Music *music_play(const char *filename);

#endif
