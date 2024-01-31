/* cutscene.cpp - displays cutscenes by number */
#include "cutscene.h"

// messagebox (speech bubble) used on this screen
#include "message.h"
// texture operations, also music_play
#include "function.h"

// how many frames make up a cutscene
#define NUM_FRAMES 3

// how long can the script be
#define MAX_SCRIPT_LEN 20
#define SCRIPT_LINE_LEN 80

// Simply-scripted cutscene action tokens.
#define CS_DEFAULT 0
#define CS_FRAME 1
#define CS_MESSAGE 2
#define CS_DELAY 3
#define CS_END 4

// Pull external "level" value from main.cpp (shared with game)
extern int level;

char do_gs_cutscene()
{
	char buffer[64];

	// cutscene script
	unsigned char script_action[MAX_SCRIPT_LEN];
	char script_param[MAX_SCRIPT_LEN][SCRIPT_LINE_LEN];

	// Cutscene screen init section.
	//  First up, load the cutscene script.
	sprintf(buffer,"cutscene/%d/scene.txt",level);
	FILE *fp = fopen(buffer,"r");
	if (fp == NULL) { return gs_title; }
	int i = 0;
	while (!feof(fp) && i < MAX_SCRIPT_LEN)
	{
		// read first char
		int c = fgetc(fp);
		switch(c)
		{
			case '*':
				script_action[i] = CS_FRAME;
				break;
			case '%':
				script_action[i] = CS_MESSAGE;
				break;
			case '#':
				script_action[i] = CS_DELAY;
				break;
			case '!':
			case -1:
				script_action[i] = CS_END;
				break;
			default:
				script_action[i] = CS_DEFAULT;
		}

		// everything afterwards is "script parameter"
		
		if (fgets(script_param[i],SCRIPT_LINE_LEN-1,fp) != 0) {
			script_param[i][strlen(script_param[i])-1] = '\0';
		}
		i++;
	}
	fclose(fp);

	//  Load textures from disk.
	//  Film border
	GLuint tex_film[2];
	tex_film[0] = load_texture("cutscene/frame-l.png",GL_LINEAR,GL_LINEAR);
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT ) ;

	tex_film[1] = load_texture("cutscene/frame-r.png",GL_LINEAR,GL_LINEAR);
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT ) ;

	// Cutscene frame textures.
	GLuint tex_frame[NUM_FRAMES];
	for (i=0; i<NUM_FRAMES; i++)
	{
		sprintf(buffer,"cutscene/%d/%d.png",level,i);
		tex_frame[i] = load_texture(buffer,GL_LINEAR,GL_LINEAR);
	}

	// Setup display lists for each screen item.
	// Film frames
	GLuint list_film = glGenLists(1);
	glNewList(list_film, GL_COMPILE);
		// DISable alpha test: no transparency for backdrop!
		glDisable(GL_ALPHA_TEST);
		// bind left-film edge texture
		glBindTexture(GL_TEXTURE_2D, tex_film[0]);
	   	glBegin(GL_QUADS);
			glTexCoord2f(0,0);
			glVertex2i(0, 0);
			glTexCoord2f(1,0);
			glVertex2i(128, 0);
			glTexCoord2f(1,2);
			glVertex2i(128, SCREEN_Y * 2);
			glTexCoord2f(0,2);
			glVertex2i(0, SCREEN_Y * 2);
		glEnd();
		// bind right-film edge texture
		glBindTexture(GL_TEXTURE_2D, tex_film[1]);
	   	glBegin(GL_QUADS);
			glTexCoord2f(0,0);
			glVertex2i(SCREEN_X - 128, 0);
			glTexCoord2f(1,0);
			glVertex2i(SCREEN_X, 0);
			glTexCoord2f(1,2);
			glVertex2i(SCREEN_X, SCREEN_Y * 2);
			glTexCoord2f(0,2);
			glVertex2i(SCREEN_X - 128, SCREEN_Y * 2);
		glEnd();
	glEndList();

	// one for the main cutscene (don't bind texture here, save that for later)
	GLuint list_frame = glGenLists(1);
	glNewList(list_frame, GL_COMPILE);
	   	glBegin(GL_QUADS);
	   		glBox(128,0,SCREEN_X-256,SCREEN_Y);
		glEnd();
	glEndList();

	// MUSIC
	/* This is the music to play. */
	sprintf(buffer,"cutscene/%d/bg.xm",level);
	Mix_Music *music = music_play(buffer);

	// GL setup for this screen
	// no blending, yes texturing
	//glDisable(GL_BLEND);
	//glEnable(GL_TEXTURE_2D);

	// get ready to begin the cutscene loop...
	unsigned int curframe = 0;
	int filmoff=0;
	unsigned int scriptpos = 0;

	unsigned int script_ticks;
	unsigned int film_ticks = script_ticks = SDL_GetTicks();

	// clear the message box
	message_clear();

	// dirty (partial screen redraw)
	unsigned char dirty = 1;
	char retval = gs_cutscene;

	while (retval==gs_cutscene)
	{
		// Timed events.
		unsigned int ticks = SDL_GetTicks();

		// Rolling film on sidebars, updates every 1 millisec
		if (film_ticks < ticks)
		{
			filmoff -= (ticks - film_ticks); film_ticks = ticks;
			while (filmoff < -600) filmoff += 600;
			dirty = 1;
		}

		// Script event
		if (script_ticks < ticks)
		{
			switch (script_action[scriptpos]) {
				case CS_FRAME:
					curframe = atoi(script_param[scriptpos]);
					dirty=1;
					break;
				case CS_MESSAGE:
					message_post(script_param[scriptpos][0] - '0', &script_param[scriptpos][1]);
					dirty=1;
					break;
				case CS_DELAY:
					script_ticks = 1000 * atoi(script_param[scriptpos]) + ticks;
					break;
				case CS_END:
					if (level > 3)
						retval=gs_title;
					else
						retval=gs_game;
					break;
				case CS_DEFAULT:
				default:
					break;
			}
			scriptpos++;
		}

		// Redraw screen if needed.
		if (dirty)
		{
			glPushMatrix();
				glTranslatef(0, filmoff, 0);
				glCallList(list_film);
			glPopMatrix();

			glBindTexture(GL_TEXTURE_2D, tex_frame[curframe]);
			glCallList(list_frame);
			message_draw();
			//Flip the backbuffer to the primary
			SDL_GL_SwapBuffers();

			dirty = 0;
		}

		SDL_Event event;

		/* Check for events */
		while (SDL_PollEvent (&event))
		{
			switch (event.type)
			{
				case SDL_KEYUP:
					if (event.key.keysym.sym == SDLK_ESCAPE) {
						if (level > MAX_LEVEL)
							retval=gs_title;
						else
							retval=gs_game;
					}
					break;
				case SDL_MOUSEBUTTONUP:
					if (level > MAX_LEVEL)
						retval=gs_title;
					else
						retval=gs_game;
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
	glDeleteLists(list_frame, 1);
	glDeleteLists(list_film, 1);

	glDeleteTextures( NUM_FRAMES, tex_frame );
	glDeleteTextures( 2, tex_film );

	return retval;
}

