/* EURO1943 - GREG KENNEDY
	http://greg-kennedy.com */
	
/* common_client.h - Declarations common to Client modules goes here. */

#ifndef COMMON_CLIENT_H_
#define COMMON_CLIENT_H_

/* Global (client and server) includes */
#include "common.h"

/* Client-specific items here */

// since SDL_Mixer didn't see fit to define one
#define MIX_DEFAULT_CHUNKSIZE 1024

#define SCREEN_X 800
#define SCREEN_Y 600

// gamestates
#define	gs_title 0
#define gs_multimenu 1
#define gs_options 2
#define gs_cutscene 3
#define gs_win 4
#define gs_lose 5
#define gs_game 10

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <SDL/SDL.h>
#include <SDL/SDL_opengl.h>
#include <SDL/SDL_mixer.h>
#include <SDL/SDL_image.h>
#include <SDL/SDL_net.h>

#endif
