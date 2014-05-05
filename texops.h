#ifndef _TEXOPS_H
#define _TEXOPS_H

#include "common_client.h"

#include <SDL/SDL_opengl.h>
#include <SDL/SDL_mixer.h>

void glBox(int x, int y, int w, int h);

// int powerOfTwo( int  );
GLuint load_texture (const char *, GLuint, GLuint);
GLuint load_texture_extra (const char *, GLuint, GLuint, int *, int *);

// music handler functions
Mix_Music *music_play(const char *filename);

#endif
