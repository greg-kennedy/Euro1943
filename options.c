/* options.cpp - set some game options */

#include "options.h"
#include "texops.h"

// Externs used by this sub-section
extern unsigned char vol_music, vol_sfx;
extern long mx, my;
extern GLuint list_cursor;

char do_gs_options()
{
	// options init section.
	//  Load texture from disk.
	GLuint tex_options = load_texture("img/ui/options.png",GL_LINEAR,GL_LINEAR);
	if (!tex_options)
		return 0;

	// Make a display list in which we draw a full-screen quad.
	GLuint list_options = glGenLists(1);
	glNewList(list_options, GL_COMPILE);
		// DISable alpha test: no transparency for backdrop!
		glDisable(GL_ALPHA_TEST);
		// bind cursor texture
		glBindTexture(GL_TEXTURE_2D, tex_options);
		// draw a quad, top-left corner at 0,0
		glBegin(GL_QUADS);
			glBox(0,0,SCREEN_X,SCREEN_Y);
		glEnd();
	glEndList();

	// Another display list, this one just a black quad, for checked options
	GLuint list_option_checked = glGenLists(1);
	glNewList(list_option_checked, GL_COMPILE);
		glColor3f(0.0f,0.0f,0.0f);

		glDisable(GL_TEXTURE_2D);
		glBegin(GL_QUADS);
			glVertex2i(292,245);
			glVertex2i(325,245);
			glVertex2i(325,272);
			glVertex2i(292,272);
		glEnd();
		glEnable(GL_TEXTURE_2D);

		glColor3f(1.0f,1.0f,1.0f);
	glEndList();

	/* Play music */
	Mix_Music *music = music_play("audio/options.mod");

	// GL setup for this screen: no alpha blending, yes texturing
	glDisable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);

	// dirty flag: redraw screen
	unsigned char dirty=1;
	char retval = gs_options;

	while (retval == gs_options)
	{
		if (dirty)
		{
			// Draw backdrop.  This is full-screen thus overwrites any prev. color
//			glClear(GL_COLOR_BUFFER_BIT);
			glCallList(list_options);

			if (vol_sfx) glCallList(list_option_checked);

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
		SDL_Event event;

		/* Check for events */
		while (SDL_PollEvent (&event))
		{
			switch (event.type)
			{
			case SDL_KEYUP:
				if (event.key.keysym.sym == SDLK_ESCAPE)
					retval = gs_title;
				break;
			case SDL_MOUSEBUTTONUP:
				if (event.button.x>286 && event.button.x<522 && event.button.y > 239 && event.button.y < 277)
				{
					if (vol_sfx)
					{
						Mix_HaltMusic();
						Mix_FreeMusic(music);
						vol_sfx=0;
					} else {
						music = music_play("audio/options.mod");
						vol_sfx=MIX_MAX_VOLUME;
					}
					dirty = 1;
				}
				else if (event.button.x>291 && event.button.x<512 && event.button.y > 511 && event.button.y < 591)
					retval = gs_title;
				break;
			case SDL_MOUSEMOTION:
				 mx=event.motion.x;
				 my=event.motion.y;
				 dirty = 1;
				 break;
			case SDL_QUIT:
				retval = gs_exit;
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
	glDeleteLists(list_option_checked, 1);
	glDeleteLists(list_options, 1);
	glDeleteTextures( 1, &tex_options );

	return retval;
}

