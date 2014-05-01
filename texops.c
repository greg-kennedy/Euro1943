#include "texops.h"

extern char vol_music;

/**
 * Returns the first power of 2 greater or equal to the specified value.
 *
 * @param value that the  
 *
 *
 */

static int powerOfTwo( int value )
{
	int result = 1 ;
	while ( result < value )
		result *= 2;
	return result ;		
}

// Draws a textured box.
void glBox (int x, int y, int w, int h)
{
	glTexCoord2f(0,0);
	glVertex2i(x, y);
	glTexCoord2f(1,0);
	glVertex2i(x+w, y);
	glTexCoord2f(1,1);
	glVertex2i(x+w, y+h);
	glTexCoord2f(0,1);
	glVertex2i(x, y+h);
}

GLuint load_texture(const char *fname, GLuint min_filt, GLuint max_filt)
{
	GLuint tex = 0;
	int w, h;

	SDL_Surface *temp_surf = NULL;
	SDL_Surface *file_surf = IMG_Load(fname);
	if (file_surf == NULL) { printf("Error loading texture '%s': %s\n",fname,IMG_GetError()); return 0; }

	w = powerOfTwo(file_surf->w);
	h = powerOfTwo(file_surf->h);
	if (file_surf->flags & (SDL_SRCALPHA | SDL_SRCCOLORKEY)) {
		// Loaded file has a transparent color?
		temp_surf = SDL_CreateRGBSurface( SDL_SWSURFACE, w, h, 32 /* bits */,
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
				) ;
		if (temp_surf == NULL) { printf("Error creating 32bit surface for '%s': %s\n",fname,SDL_GetError()); free(file_surf); return 0; }
		// clear the dest surface
		SDL_FillRect( temp_surf, 0, SDL_MapRGBA(temp_surf->format,0,0,0,0) ) ;
		SDL_SetAlpha(file_surf,0,0);
		SDL_BlitSurface(file_surf,NULL,temp_surf,NULL);
		SDL_FreeSurface(file_surf);

		glGenTextures( 1, &tex );
		glBindTexture( GL_TEXTURE_2D, tex );
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_filt);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, max_filt);
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP ) ;
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP ) ;

		glTexImage2D(GL_TEXTURE_2D, 0, 4, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, temp_surf->pixels);
		SDL_FreeSurface(temp_surf);
	} else {
		// Loaded file does not have transparent areas.
		temp_surf = SDL_CreateRGBSurface( SDL_SWSURFACE, w, h, 24 /* bits */,
#if SDL_BYTEORDER == SDL_LIL_ENDIAN // OpenGL RGBA masks
				0x000000FF,
				0x0000FF00,
				0x00FF0000,
				0x00000000
#else
				0x00FF0000,
				0x0000FF00,
				0x000000FF,
				0x00000000
#endif
				) ;
		if (temp_surf == NULL) { printf("Error creating 24bit surface for '%s': %s\n",fname,SDL_GetError()); free(file_surf); return 0; }
		SDL_BlitSurface(file_surf,NULL,temp_surf,NULL);
		SDL_FreeSurface(file_surf);
		glGenTextures( 1, &tex );
		glBindTexture( GL_TEXTURE_2D, tex );
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_filt);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, max_filt);
		glTexImage2D(GL_TEXTURE_2D, 0, 3, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, temp_surf->pixels);
		SDL_FreeSurface(temp_surf);
	}
	return tex;
}

//////////////////////////
// music handler functions
Mix_Music *music_play(const char *filename)
{
	// Pointer to new music
	Mix_Music *music = NULL;

	// If volume is non-zero...
	if (vol_music)
	{
		// Attempt to load the music from filename.
		music = Mix_LoadMUS(filename);
		// Check for error
		if (!music)
		{
			fprintf(stderr,"ERROR: music_play: Mix_LoadMUS(\"%s\"): %s\n", filename, Mix_GetError());
		} else {
			// Music loaded OK, play it
			if (Mix_PlayMusic(music, -1))
				fprintf(stderr,"ERROR: music_play: Mix_PlayMusic(\"%s\"): %s\n", filename, Mix_GetError());
		}
	}

	// Return pointer so it can be changed later.
	return music;
}

