/* title.cpp - title and main menu */

#include "title.h"

#include "texops.h"

extern SDL_Surface *screen;
extern int sfxon, gamestate, level;
extern Mix_Music *music;
extern long mx, my;

extern GLuint cursor, tex1;

void inittitle()
{
	tex1 = load_texture("img/ui/title.png",GL_LINEAR,GL_LINEAR);

     /* This is the music to play. */
     if (sfxon==1) {
	     music = Mix_LoadMUS("audio/title.xm");
         Mix_PlayMusic(music, -1);
     }

     level=0;
}

void destroytitle()
{
     glDeleteTextures( 1, &tex1 );
     Mix_HaltMusic();
     Mix_FreeMusic(music);
     music = NULL;
}

void drawtitle()
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

    glBindTexture(GL_TEXTURE_2D, cursor); 
    glBegin(GL_QUADS);
      glTexCoord2f(0,0);
      glVertex2i(mx-16, my-16);
      glTexCoord2f(1,0);
      glVertex2i(mx+16, my-16);
      glTexCoord2f(1,1);
      glVertex2i(mx+16, my+16);
      glTexCoord2f(0,1);
      glVertex2i(mx-16, my+16);
    glEnd();

    //Disable texturing
    glDisable(GL_TEXTURE_2D);
   
    //Flip the backbuffer to the primary
    SDL_GL_SwapBuffers();
    /* Don't run too fast */
    SDL_Delay (1);
}

bool handletitle()
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
                retval=false;
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
				retval = false;
		}
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
