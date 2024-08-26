/* title.cpp - title and main menu */
#include "title.h"

// Texture operations + music
#include "common_client.h"

// game globals
extern struct env_t env;

int do_gs_title()
{
	// Title init section.
	//  Load texture from disk.
	// TODO: This title image is not sharp.  Needs redrawing at 512x512.
	GLuint tex_title = load_texture("img/ui/title.png", GL_LINEAR, GL_LINEAR);

	if (!tex_title)
		return gs_exit;

	// Make a display list in which we draw a full-screen quad.
	GLuint list_title = glGenLists(1);
	glNewList(list_title, GL_COMPILE);
	// DISable alpha test: no transparency for backdrop!
	glDisable(GL_ALPHA_TEST);
	glBox(tex_title, SCREEN_X, SCREEN_Y);
	glEndList();
	/* Play music */
	Mix_Music * music = music_play("audio/title.xm");
	// GL setup for this screen: no alpha blending, yes texturing
	glDisable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	// MAIN SCREEN LOOP	-- Values used for main loop
	// mouse state
	int mx, my;
	SDL_GetMouseState(&mx, &my);
	//  dirty flag: redraw screen when set to 1
	int dirty = 1;
	//  retval: when switched away from gs_title, exit screen
	char state = gs_title;

	while (state == gs_title) {
		if (dirty) {
			// Draw backdrop.  This is full-screen thus overwrites any prev. color
//			glClear(GL_COLOR_BUFFER_BIT);
			glCallList(list_title);
			// Draw mouse cursor over main menu.
			glDrawCursor(mx, my);
			// Flip the backbuffer to the primary
			SDL_GL_SwapBuffers();
			// reset "dirty" flag
			dirty = 0;
		}

		/* Check for events */
		SDL_Event event;

		while (SDL_PollEvent(&event)) {
			switch (event.type) {
			case SDL_KEYUP:

				// Only key recognized on this screen is ESCAPE, which quits game.
				if (event.key.keysym.sym == SDLK_ESCAPE)
					state = gs_exit;

				break;

			case SDL_MOUSEBUTTONDOWN:
				mx = event.button.x;
				my = event.button.y;
				dirty = 1;
				break;

			case SDL_MOUSEBUTTONUP:
				mx = event.button.x;
				my = event.button.y;
				dirty = 1;

				if (event.button.button == SDL_BUTTON_LEFT) {
					if (mx > 56 && mx < 373) {
						if (my > 255 && my < 356) {
							// Begin a single player game: reset level to 0,
							//  and switch to "cutscene player".
							env.multiplayer = 0;
							env.level = 0;
							state = gs_cutscene;
						} else if (my > 435 && my < 536) {
							// Options menu.
							state = gs_options;
						}
					} else if (mx > 424 && mx < 741) {
						if (my > 255 && my < 356 && env.ok_network) {
							// Multiplayer menu.  Only usable if network is up.
							state = gs_multimenu;
						} else if (my > 435 && my < 536) {
							// EXIT button...
							state = gs_exit;
						}
					}
				}

				break;

			case SDL_MOUSEMOTION:
				mx = event.motion.x;
				my = event.motion.y;
				// Moving the mouse resets the dirty flag (need to redraw cursor...)
				dirty = 1;
				break;

			case SDL_QUIT:
				state = gs_exit;
				break;

			case SDL_VIDEOEXPOSE:
				// WM-initiated redraw event
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
	glDeleteLists(list_title, 1);
	glDeleteTextures(1, &tex_title);
	return state;
}
