/* winlose.cpp - win and lose screens */

#include "winlose.h"

// Texture operations, and music_play
#include "texops.h"

// Externs used by this sub-section
extern unsigned char vol_music;

extern int level;

// Shared function for full-screen win/loss display, with music
static char win_lose(const char *tex_name, const char *mus_name, char current_gamestate, char next_gamestate)
{
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

	// MAIN SCREEN LOOP	-- Values used for main loop
	//  dirty flag: redraw screen when set to 1
	unsigned char dirty=1;
	//  retval: when switched away from 0, exit screen
	//    positive value is next gamestate
	//    negative value is gs_exit.
	char retval = current_gamestate;

	while (retval == current_gamestate)
	{
		if (dirty)
		{
			glCallList(list_win);

			//Flip the backbuffer to the primary
			SDL_GL_SwapBuffers();

			dirty = 0;
		}

		/* Check for events */
		SDL_Event event;
		while (SDL_PollEvent (&event))
		{
			switch (event.type)
			{
				case SDL_KEYUP:
				case SDL_MOUSEBUTTONUP:
					retval = next_gamestate;
					break;
				case SDL_QUIT:
					retval = gs_exit;
					break;
				case SDL_VIDEOEXPOSE:
					dirty = 1;
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

char do_gs_win()
{
	return win_lose("img/ui/v.png","audio/win.mod", gs_win, gs_cutscene);
}

char do_gs_lose()
{
	return win_lose("img/ui/d.png","audio/lose.mod", gs_lose, gs_title);
}

