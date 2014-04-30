/* title.cpp - title and main menu */

#include "title.h"

//#include "message.h"
#include "texops.h"

// Externs used by this sub-section
extern unsigned char vol_music, gamestate, network;
extern long mx, my;
extern GLuint list_cursor;

unsigned char do_gs_title()
{
	unsigned char retval=1, dirty=1;

	// Title init section.
	//  Load texture from disk.
	GLuint tex_title = load_texture("img/ui/title.png",GL_LINEAR,GL_LINEAR);
	if (!tex_title)
		return 0;

	// Make a display list in which we draw a full-screen quad.
	GLuint list_title = glGenLists(1);
	glNewList(list_title, GL_COMPILE);
		// DISable alpha test: no transparency for backdrop!
		glDisable(GL_ALPHA_TEST);
		// bind cursor texture
		glBindTexture(GL_TEXTURE_2D, tex_title);
		// draw a quad, top-left corner at 0,0
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

	/* Play music */
	Mix_Music *music = music_play("audio/title.xm");

	// GL setup for this screen: no alpha blending, yes texturing
	glDisable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);

	while (retval && gamestate==gs_title)
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
		SDL_Event event;

		/* Check for events */
		while (SDL_PollEvent (&event))
		{
			switch (event.type)
			{
			case SDL_KEYUP:
				if (event.key.keysym.sym == SDLK_ESCAPE)
					retval=0;
				break;
			case SDL_MOUSEBUTTONUP:
				if (event.button.x>56 && event.button.x<373)
				{
					if (event.button.y>255 && event.button.y<356)
						gamestate=3; //mission=1;
					if (event.button.y>435 && event.button.y<536)
						gamestate=2;
				}
				if (event.button.x>424 && event.button.x<741)
				{
					if (event.button.y>255 && event.button.y<356)
						gamestate=1;
					if (event.button.y>435 && event.button.y<536)
						retval = 0;
				}
				 break;
			case SDL_MOUSEMOTION:
				 mx=event.motion.x;
				 my=event.motion.y;
				 dirty = 1;
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
	glDeleteLists(list_title, 1);
	glDeleteTextures( 1, &tex_title );

	return retval;
}

