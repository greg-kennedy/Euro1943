#ifndef _TEXOPS_H
#define _TEXOPS_H

#include "common_client.h"

int powerOfTwo( int  );
GLuint load_texture (const char *, GLuint, GLuint);

// music handler functions
Mix_Music *music_play(const char *filename);

#endif
