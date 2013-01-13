#ifndef _TEXOPS_H
#define _TEXOPS_H

#include "common.h"

int powerOfTwo( int  );
GLuint load_texture (const char *, GLuint, GLuint);
SDL_Surface *loadGLStyle (const char *);

#endif
