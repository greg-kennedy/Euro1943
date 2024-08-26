/* options.cpp - set some game options */
#include "options.h"

// Texture operations + music
#include "common_client.h"

// game globals
extern struct env_t env;

int do_gs_options()
{
	// options init section.
	//  Load texture from disk.
	GLuint tex_options = load_texture("img/ui/options.png", GL_LINEAR, GL_LINEAR);

	if (!tex_options)
		return gs_exit;

	// Make a display list in which we draw a full-screen quad.
	GLuint list_options = glGenLists(1);
	glNewList(list_options, GL_COMPILE);
	// DISable alpha test: no transparency for backdrop!
	glDisable(GL_ALPHA_TEST);
	// bind cursor texture
	glBox(tex_options, SCREEN_X, SCREEN_Y);
	glEndList();
	// Another display list, this one just a black quad, for checked options
	GLuint list_option_checked = glGenLists(1);
	glNewList(list_option_checked, GL_COMPILE);

	if (! env.ok_audio)
		glColor3f(0.5f, 0.25f, 0.25f);
	else
		glColor3f(0.0f, 0.0f, 0.0f);

	glDisable(GL_TEXTURE_2D);
	glBegin(GL_QUADS);
	glVertex2s(-108, -55);
	glVertex2s(-75, -55);
	glVertex2s(-75, -28);
	glVertex2s(-108, -28);
	glEnd();
	glEnable(GL_TEXTURE_2D);
	glColor3f(1.0f, 1.0f, 1.0f);
	glEndList();
	/* Play music */
	Mix_Music * music = music_play("audio/options.mod");
	// GL setup for this screen: no alpha blending, yes texturing
	//glDisable(GL_BLEND);
	//glEnable(GL_TEXTURE_2D);
	// mouse state
	int mx, my;
	SDL_GetMouseState(&mx, &my);
	// dirty flag: redraw screen
	int dirty = 1;
	int state = gs_options;

	while (state == gs_options) {
		if (dirty) {
			// Draw backdrop.  This is full-screen thus overwrites any prev. color
//			glClear(GL_COLOR_BUFFER_BIT);
			glCallList(list_options);

			if (env.volume || !env.ok_audio) glCallList(list_option_checked);

			// Draw mouse cursor over menu.
			glDrawCursor(mx, my);
			// Flip the backbuffer to the primary
			SDL_GL_SwapBuffers();
			// reset "dirty" flag
			dirty = 0;
		}

		SDL_Event event;

		/* Check for events */
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
			case SDL_KEYUP:
				if (event.key.keysym.sym == SDLK_ESCAPE)
					state = gs_title;

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
					if (event.button.x > 286 && event.button.x < 522 && event.button.y > 239 && event.button.y < 277 && env.ok_audio) {
						if (env.volume) {
							Mix_FreeMusic(music);
							env.volume = 0;
						} else {
							music = music_play("audio/options.mod");
							env.volume = MIX_MAX_VOLUME;
						}
					} else if (event.button.x > 291 && event.button.x < 512 && event.button.y > 511 && event.button.y < 591)
						state = gs_title;
				}

				break;

			case SDL_MOUSEMOTION:
				mx = event.motion.x;
				my = event.motion.y;
				dirty = 1;
				break;

			case SDL_QUIT:
				state = gs_exit;
				break;

			case SDL_VIDEOEXPOSE:
				dirty = 1;
				break;

			default:
				break;
			}
		}

		/* Don't run too fast */
		SDL_Delay(1);
	}

	// Stop music playback, if it was playing
	Mix_FreeMusic(music);
	// Clean up OpenGL stuff for this screen
	glDeleteLists(list_option_checked, 1);
	glDeleteLists(list_options, 1);
	glDeleteTextures(1, &tex_options);
	return state;
}

