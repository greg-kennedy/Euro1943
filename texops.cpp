#include "texops.h"

/**
 * Returns the first power of 2 greater or equal to the specified value.
 *
 * @param value that the  
 *
 *
 */

int powerOfTwo( int value )
{
	int result = 1 ;
	while ( result < value )
		result *= 2;
	return result ;		
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

// Does the same as above, but stops short before uploading the texture
SDL_Surface *loadGLStyle(const char *fname)
{
	SDL_Surface *temp_surf = NULL;
	int w, h;

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
		if (temp_surf == NULL) { printf("Error creating 32bit surface for '%s': %s\n",fname,SDL_GetError()); free(file_surf); return NULL; }
		// clear the dest surface
		SDL_FillRect( temp_surf, 0, SDL_MapRGBA(temp_surf->format,0,0,0,0) ) ;
		SDL_SetAlpha(file_surf,0,0);
		SDL_BlitSurface(file_surf,NULL,temp_surf,NULL);
		SDL_FreeSurface(file_surf);
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
		if (temp_surf == NULL) { printf("Error creating 24bit surface for '%s': %s\n",fname,SDL_GetError()); free(file_surf); return NULL; }
		SDL_BlitSurface(file_surf,NULL,temp_surf,NULL);
		SDL_FreeSurface(file_surf);
	}

	return temp_surf;
}

