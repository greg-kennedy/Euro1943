/* title.cpp - title and main menu */

#include "title.h"

// Texture operations, and music_play
#include "texops.h"

// Externs used by this sub-section
extern unsigned char vol_music, network;
extern long mx, my;
extern GLuint list_cursor;

extern int level;

char do_gs_title()
{
	// Title init section.
	//  Load texture from disk.
	// TODO: This title image is not sharp.  Needs redrawing at 512x512.
	GLuint tex_title = load_texture("img/ui/title.png",GL_LINEAR,GL_LINEAR);
	if (!tex_title)
		return gs_exit;

	// Make a display list in which we draw a full-screen quad.
	GLuint list_title = glGenLists(1);
	glNewList(list_title, GL_COMPILE);
		// DISable alpha test: no transparency for backdrop!
		glDisable(GL_ALPHA_TEST);
		// bind cursor texture
		glBindTexture(GL_TEXTURE_2D, tex_title);
		// draw a quad, top-left corner at 0,0
		glBegin(GL_QUADS);
			glBox(0,0,SCREEN_X,SCREEN_Y);
		glEnd();
	glEndList();

	/* Play music */
	Mix_Music *music = music_play("audio/title.xm");

	// GL setup for this screen: no alpha blending, yes texturing
	glDisable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);

	// MAIN SCREEN LOOP	-- Values used for main loop
	//  dirty flag: redraw screen when set to 1
	unsigned char dirty=1;
	//  retval: when switched away from gs_title, exit screen
	char retval = gs_title;

	while (retval==gs_title)
	{
		if (dirty)
		{
			// Draw backdrop.  This is full-screen thus overwrites any prev. color
//			glClear(GL_COLOR_BUFFER_BIT);
			glCallList(list_title);

			// Draw mouse cursor over main menu.
			glPushMatrix();
				glTranslatef(mx, my, 0);
				glCallList(list_cursor);
			glPopMatrix();

			// Flip the backbuffer to the primary
			SDL_GL_SwapBuffers();
			
			// reset "dirty" flag
			dirty = 0;

		}

		/* Check for events */
		SDL_Event event;
		while (SDL_PollEvent (&event))
		{
			switch (event.type)
			{
				case SDL_KEYUP:
					// Only key recognized on this screen is ESCAPE, which quits game.
					if (event.key.keysym.sym == SDLK_ESCAPE)
						retval=gs_exit;
					break;
				case SDL_MOUSEBUTTONUP:
					if (event.button.x>56 && event.button.x<373)
					{
						if (event.button.y>255 && event.button.y<356)
						{
							// Begin a single player game: reset level to 0,
							//  and switch to "cutscene player".
							level = 0;
							retval = gs_cutscene;
						}
						else if (event.button.y>435 && event.button.y<536)
						{
							// Options menu.
							retval = gs_options;
						}
					}
					else if (event.button.x>424 && event.button.x<741)
					{
						if (event.button.y>255 && event.button.y<356 && network)
						{
							// Multiplayer menu.  Only usable if network is up.
							retval = gs_multimenu;
						}
						else if (event.button.y>435 && event.button.y<536)
						{
							// EXIT button...
							retval = gs_exit;
						}
					}
				break;
			case SDL_MOUSEMOTION:
				mx=event.motion.x;
				my=event.motion.y;
				// Moving the mouse resets the dirty flag (need to redraw cursor...)
				dirty = 1;
				break;
			case SDL_QUIT:
				retval = gs_exit;
				break;
			case SDL_VIDEOEXPOSE:
				// WM-initiated redraw event
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
	glDeleteLists(list_title, 1);
	glDeleteTextures( 1, &tex_title );

	return retval;
}

