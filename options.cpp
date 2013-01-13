/* options.cpp - set some game options */

#include "options.h"
#include "texops.h"

extern SDL_Surface *screen;
extern int sfxon, gamestate;
extern Mix_Music *music;
extern long mx, my;

extern GLuint cursor, tex1;

void initoptions()
{
	tex1 = load_texture("img/ui/options.png",GL_LINEAR,GL_LINEAR);

	/* This is the music to play. */
	if (sfxon==1) {
		music = Mix_LoadMUS("audio/options.mod");
		Mix_PlayMusic(music, -1);
	}

}

void destroyoptions()
{
	glDeleteTextures( 1, &tex1 );
	Mix_HaltMusic();
	Mix_FreeMusic(music);
	music = NULL;
}

void drawoptions()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	//Enable texturing
	glEnable(GL_TEXTURE_2D);

	glBindTexture(GL_TEXTURE_2D, tex1); 

	glBegin(GL_QUADS);
	glTexCoord2f(0,0);
	glVertex3i(0, 0, 0);
	glTexCoord2f(1,0);
	glVertex3i(800,0, 0);
	glTexCoord2f(1,1);
	glVertex3i(800,600, 0);
	glTexCoord2f(0,1);
	glVertex3i(0,600, 0);
	glEnd();

	if (sfxon) {
		glColor3f(0.0f,0.0f,0.0f);

		//    glBindTexture(GL_TEXTURE_2D, 0);
		glDisable(GL_TEXTURE_2D);
		glBegin(GL_QUADS);
		glVertex3i(292, 245, 0);
		glVertex3i(325,245, 0);
		glVertex3i(325,272, 0);
		glVertex3i(292,272, 0);
		glEnd();
		glEnable(GL_TEXTURE_2D);

		glColor3f(1.0f,1.0f,1.0f);

	}

	glBindTexture(GL_TEXTURE_2D, cursor); 
	glBegin(GL_QUADS);
	glTexCoord2f(0,0);
	glVertex3i(mx-16, my-16, 0);
	glTexCoord2f(1,0);
	glVertex3i(mx+16, my-16, 0);
	glTexCoord2f(1,1);
	glVertex3i(mx+16, my+16, 0);
	glTexCoord2f(0,1);
	glVertex3i(mx-16, my+16, 0);
	glEnd();

	//Disable texturing
	glDisable(GL_TEXTURE_2D);

	//Flip the backbuffer to the primary
	SDL_GL_SwapBuffers();
	/* Don't run too fast */
	SDL_Delay (1);
}

bool handleoptions()
{
	SDL_Event event;
	bool retval=true;

	/* Check for events */
	while (SDL_PollEvent (&event))
	{
		switch (event.type)
		{
			case SDL_KEYUP:
				if (event.key.keysym.sym == SDLK_ESCAPE)
					gamestate=0;
				break;
			case SDL_MOUSEBUTTONUP:
				//             printf("%d, %d\n",event.button.x, event.button.y);
				//             gamestate=0;
				if (event.button.x>286 && event.button.x<522 && event.button.y > 239 && event.button.y < 277)
				{
					if (sfxon)
					{
						Mix_HaltMusic();
						Mix_FreeMusic(music);
						sfxon=0;
					} else {
						music = Mix_LoadMUS("audio/options.mod");
						Mix_PlayMusic(music,-1);
						sfxon=1;
					}
				}
				else if (event.button.x>291 && event.button.x<512 && event.button.y > 511 && event.button.y < 591)
					gamestate = 0;
				break;
			case SDL_MOUSEMOTION:
				mx=event.motion.x;
				my=event.motion.y;
				break;
			case SDL_QUIT:
				retval = false;
				break;
			default:
				break;
		}
	}

	return retval;
}
