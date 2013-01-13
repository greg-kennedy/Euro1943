/* winlose.cpp - win and lose screens */

#include "winlose.h"
#include "texops.h"

extern SDL_Surface *screen;
extern int sfxon, gamestate, level;
extern Mix_Music *music;

extern GLuint tex1;

void initwin()
{
	tex1 = load_texture("img/ui/v.png",GL_LINEAR,GL_LINEAR);

	/* This is the music to play. */
	if (sfxon==1) {
		music = Mix_LoadMUS("audio/win.mod");
		Mix_PlayMusic(music, -1);
	}
}

void destroywin()
{
	glDeleteTextures( 1, &tex1 );
	Mix_HaltMusic();
	Mix_FreeMusic(music);
	music = NULL;

	level++;
}

void drawwin()
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

	//Disable texturing
	glDisable(GL_TEXTURE_2D);

	//Flip the backbuffer to the primary
	SDL_GL_SwapBuffers();

	/* Don't run too fast */
	SDL_Delay (1);
}

bool handlewin()
{
	SDL_Event event;
	bool retval=true;

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
				retval = false;
				break;
			default:
				break;
		}
	}

	return retval;
}

void initlose()
{
	tex1 = load_texture("img/ui/d.png",GL_LINEAR,GL_LINEAR);

	/* This is the music to play. */
	if (sfxon==1) {
		music = Mix_LoadMUS("audio/lose.mod");
		Mix_PlayMusic(music, -1);
	}
}

void destroylose()
{
	glDeleteTextures( 1, &tex1 );
	Mix_HaltMusic();
	Mix_FreeMusic(music);
	music = NULL;
}

void drawlose()
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

	//Disable texturing
	glDisable(GL_TEXTURE_2D);

	//Flip the backbuffer to the primary
	SDL_GL_SwapBuffers();

	/* Don't run too fast */
	SDL_Delay (1);
}

bool handlelose()
{
	SDL_Event event;
	bool retval=true;

	/* Check for events */
	while (SDL_PollEvent (&event))
	{
		switch (event.type)
		{
			case SDL_KEYUP:
			case SDL_MOUSEBUTTONUP:
				gamestate=0;
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
