/* winlose.cpp - win and lose screens */

#include "winlose.h"

#include "texops.h"

// Externs used by this sub-section
extern unsigned char vol_music, gamestate;

// Shared function for full-screen win/loss display, with music
static unsigned char win_lose(const char *tex_name, const char *mus_name)
{
	unsigned char retval=1, dirty=1;

	// track current gamestate
	int gs = gamestate;

	// Screen init section.
	//  Load texture from disk.
	GLuint tex_win = load_texture(tex_name,GL_LINEAR,GL_LINEAR);
	if (!tex_win)
		return 0;

	// Make a display list in which we draw a full-screen quad.
	GLuint list_win = glGenLists(1);
	glNewList(list_win, GL_COMPILE);
	   	glBegin(GL_QUADS);
			glTexCoord2f(0,0);
			glVertex2i(0, 0);
			glTexCoord2f(1,0);
			glVertex2i(SCREEN_X, 0);
			glTexCoord2f(1,1);
			glVertex2i(SCREEN_X, SCREEN_Y);
			glTexCoord2f(0,1);
			glVertex2i(0, SCREEN_Y);
		glEnd();
	glEndList();

	/* This is the music to play. */
	Mix_Music *music = music_play(mus_name);

	// GL setup for this screen
	glEnable(GL_TEXTURE_2D);
	// DISable alpha test: no transparency for backdrop!
	glDisable(GL_ALPHA_TEST);
	// bind VICTORY quad texture
	glBindTexture(GL_TEXTURE_2D, tex_win);

	while (retval && gamestate==gs)
	{
		if (dirty)
		{
			glCallList(list_win);

			//Flip the backbuffer to the primary
			SDL_GL_SwapBuffers();

			dirty = 0;
		}

		SDL_Event event;

		/* Check for events */
		while (SDL_PollEvent (&event))
		{
			switch (event.type)
			{
				case SDL_KEYUP:
				case SDL_MOUSEBUTTONUP:
					gamestate=3;
					break;
				case SDL_QUIT:
					retval = 0;
					break;
				case SDL_VIDEOEXPOSE:
					dirty = 1;
					break;
				default:
					break;
			}
		}

		/* Don't run too fast */
		SDL_Delay (1);
	}

	// Stop music playback, if it was playing
	if (music) Mix_FreeMusic(music);

	// Clean up OpenGL stuff for this screen
	glDeleteLists(list_win, 1);
	glDeleteTextures( 1, &tex_win );

	return retval;
}

unsigned char do_gs_win()
{
	return win_lose("img/ui/v.png","audio/win.mod");
}

unsigned char do_gs_lose()
{
	return win_lose("img/ui/d.png","audio/lose.mod");
}

