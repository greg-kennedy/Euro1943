/* winlose.cpp - win and lose screens */
#include "winlose.h"

// Texture operations + music
#include "common_client.h"

// Externs used by this sub-section
extern unsigned char vol_music;

extern int level;

// Shared function for full-screen win/loss display, with music
static char win_lose(const char * tex_name, const char * mus_name, char current_gamestate, char next_gamestate)
{
	// Screen init section.
	//  Load texture from disk.
	GLuint tex_win = load_texture(tex_name, GL_LINEAR, GL_LINEAR);

	if (!tex_win)
		return 0;

	// Make a display list in which we draw a full-screen quad.
	GLuint list_win = glGenLists(1);
	glNewList(list_win, GL_COMPILE);
	glBox(tex_win, SCREEN_X, SCREEN_Y);
	glEndList();
	/* This is the music to play. */
	Mix_Music * music = music_play(mus_name);
	// GL setup for this screen
	glEnable(GL_TEXTURE_2D);
	// DISable alpha test: no transparency for backdrop!
	glDisable(GL_ALPHA_TEST);
	// bind VICTORY quad texture
//	glBindTexture(GL_TEXTURE_2D, tex_win);
	// MAIN SCREEN LOOP	-- Values used for main loop
	//  dirty flag: redraw screen when set to 1
	int dirty = 1;
	//  state: when switched away from 0, exit screen
	//    positive value is next gamestate
	//    negative value is gs_exit.
	int state = current_gamestate;

	while (state == current_gamestate) {
		if (dirty) {
			glCallList(list_win);
			//Flip the backbuffer to the primary
			SDL_GL_SwapBuffers();
			dirty = 0;
		}

		/* Check for events */
		SDL_Event event;

		while (SDL_PollEvent(&event)) {
			switch (event.type) {
			case SDL_KEYUP:
			case SDL_MOUSEBUTTONUP:
				state = next_gamestate;
				break;

			case SDL_QUIT:
				state = gs_exit;
				break;

			case SDL_VIDEOEXPOSE:
				dirty = 1;
				break;
			}
		}

		/* Don't run too fast */
		SDL_Delay(1);
	}

	// Stop music playback, if it was playing
	Mix_FreeMusic(music);
	// Clean up OpenGL stuff for this screen
	glDeleteLists(list_win, 1);
	glDeleteTextures(1, &tex_win);
	return state;
}

char do_gs_win()
{
	return win_lose("img/ui/v.png", "audio/win.mod", gs_win, gs_cutscene);
}

char do_gs_lose()
{
	return win_lose("img/ui/d.png", "audio/lose.mod", gs_lose, gs_title);
}

