/* cutscene.cpp - displays cutscenes by number */

#include "cutscene.h"
#include "texops.h"

extern SDL_Surface *screen, *tempsurf;
extern int sfxon, gamestate;
extern Mix_Music *music;
extern int multiplayer;

int filmoff=0;

GLuint film[3];
GLuint csframes[NUM_FRAMES];

char script[40][80];
extern char oldmessage[70];
extern SDL_Surface *font, *message, *bubble, *speakers[NUM_SPEAKERS];
long scriptpos,curframe;
unsigned long csticks;

int glevel;

void initcutscene(int level)
{
	int i;
	SDL_Surface *surf;
	char buffer[80];
	FILE *fp;

	glevel=level;

	//     glGenTextures( NUM_FRAMES, csframes );

	bubble = loadGLStyle("img/hud/bubble.png");
	glGenTextures( 1, &film[2] );
	glBindTexture( GL_TEXTURE_2D, film[2] );
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	for (i=0; i<NUM_SPEAKERS; i++)
	{
		sprintf(buffer,"img/hud/speakers/%d.png",i);
		surf=IMG_Load(buffer);
		speakers[i] = SDL_ConvertSurface(surf,bubble->format,bubble->flags);
		SDL_FreeSurface(surf);
	}

	for (i=0; i<NUM_FRAMES; i++)
	{
		sprintf(buffer,"cutscene/%d/%d.png",level,i);
		csframes[i] = load_texture(buffer,GL_LINEAR,GL_LINEAR);
	}

	film[0] = load_texture("cutscene/frame-l.png",GL_NEAREST,GL_NEAREST);
	film[1] = load_texture("cutscene/frame-r.png",GL_NEAREST,GL_NEAREST);

	/* Script. */
	sprintf(buffer,"cutscene/%d/scene.txt",level);
	fp = fopen(buffer,"r");
	if (fp == NULL) gamestate=10; else {
		i=0;
		while (!feof(fp))
		{
			if (fgets(script[i],79,fp) != 0) {
				script[i][strlen(script[i])-1] = '\0';
				i++;
			}
		}
	}
	fclose(fp);

	/* This is the music to play. */
	sprintf(buffer,"cutscene/%d/bg.xm",level);
	if (sfxon==1) {
		music = Mix_LoadMUS(buffer);
		Mix_PlayMusic(music, -1);
	}

	strcpy(oldmessage,"");
	chatcutscene("",0);

	multiplayer=0;
	csticks = 0;
	scriptpos=0;
	curframe=0;
}

void destroycutscene()
{
	int i;

	glDeleteTextures( NUM_FRAMES, csframes );
	glDeleteTextures( 3, film );
	if (sfxon == 1) {
		Mix_HaltMusic();
		Mix_FreeMusic(music);
		music = NULL;
	}
	SDL_FreeSurface(bubble);
	for (i=0; i<NUM_SPEAKERS; i++)
		SDL_FreeSurface(speakers[i]);
}

void drawcutscene()
{

	filmoff+=2;
	if (filmoff>=600) filmoff = 0;

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, film[0]);
	glBegin(GL_QUADS);
	glTexCoord2f(0,0);
	glVertex2i(0, filmoff);
	glTexCoord2f(1,0);
	glVertex2i(128, filmoff);
	glTexCoord2f(1,1);
	glVertex2i(128, 600+filmoff);
	glTexCoord2f(0,1);
	glVertex2i(0, 600+filmoff);
	glTexCoord2f(0,0);
	glVertex2i(0, filmoff-600);
	glTexCoord2f(1,0);
	glVertex2i(128, filmoff-600);
	glTexCoord2f(1,1);
	glVertex2i(128, filmoff);
	glTexCoord2f(0,1);
	glVertex2i(0, filmoff);
	glEnd();

	glBindTexture(GL_TEXTURE_2D, film[1]);
	glBegin(GL_QUADS);
	glTexCoord2f(0,0);
	glVertex2i(672, filmoff);
	glTexCoord2f(1,0);
	glVertex2i(800, filmoff);
	glTexCoord2f(1,1);
	glVertex2i(800, 600+filmoff);
	glTexCoord2f(0,1);
	glVertex2i(672, 600+filmoff);
	glTexCoord2f(0,0);
	glVertex2i(672, filmoff-600);
	glTexCoord2f(1,0);
	glVertex2i(800, filmoff-600);
	glTexCoord2f(1,1);
	glVertex2i(800, filmoff);
	glTexCoord2f(0,1);
	glVertex2i(672, filmoff);
	glEnd();

	glBindTexture(GL_TEXTURE_2D, csframes[curframe]);
	glBegin(GL_QUADS);
	glTexCoord2f(0,0);
	glVertex2i(128, 0);
	glTexCoord2f(1,0);
	glVertex2i(672, 0);
	glTexCoord2f(1,1);
	glVertex2i(672, 600);
	glTexCoord2f(0,1);
	glVertex2i(128, 600);
	glEnd();

	glBindTexture(GL_TEXTURE_2D, film[2]);
	glBegin(GL_QUADS);
	glTexCoord2f(0,0);
	glVertex2i(144, 568);
	glTexCoord2f(1,0);
	glVertex2i(656, 568);
	glTexCoord2f(1,1);
	glVertex2i(656, 600);
	glTexCoord2f(0,1);
	glVertex2i(144, 600);
	glEnd();

	glDisable(GL_TEXTURE_2D);

	//Flip the backbuffer to the primary
	SDL_GL_SwapBuffers();

	SDL_Delay (1);
}

bool handlecutscene()
{
	SDL_Event event;
	bool retval=true;

	if (SDL_GetTicks() > csticks) {
		switch (script[scriptpos][0]) {
			case '*':
				curframe = script[scriptpos][1]-48;
				break;
			case '%':
				chatcutscene(&script[scriptpos][2],script[scriptpos][1]-48);
				break;
			case '#':
				csticks = 1000 * (script[scriptpos][1]-48) + SDL_GetTicks();
				break;
			case '!':
				if (glevel > 3) gamestate=0; else
					gamestate=10;
				break;
			default:
				break;
		}
		scriptpos++;
	}

	/* Check for events */
	while (SDL_PollEvent (&event))
	{
		switch (event.type)
		{
			case SDL_KEYUP:
				if (event.key.keysym.sym == SDLK_ESCAPE) {
					if (glevel > 3) {
					gamestate=0; } else { gamestate=10; } }
				break;
			case SDL_MOUSEBUTTONUP:
				if (glevel > 3) gamestate=0; else
					gamestate=10;
				break;
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

void chatcutscene (const char *message, int speaker)
{
	unsigned int i;
	SDL_Surface *temp;
	SDL_Rect sr,dr;
	sr.h=8;
	sr.w=8;
	sr.y=0;
	dr.x=64;
	dr.y=6;
	dr.w=8;
	dr.h=8;

	// makes temp into a copy of bubble
	temp = SDL_ConvertSurface(bubble, bubble->format, bubble->flags);

	// put the little speaker image down on temp
	SDL_BlitSurface(speakers[speaker],NULL,temp,NULL);

	for (i=0; i<strlen(oldmessage); i++)
	{
		sr.x = 8 * (oldmessage[i] & 0x7F);
		SDL_BlitSurface(font,&sr,temp,&dr);
		dr.x+=8;
	}

	dr.x = 64;
	dr.y = 16;
	for (i=0; i<strlen(message); i++)
	{
		sr.x = 8 * (message[i] & 0x7F);
		SDL_BlitSurface(font,&sr,temp,&dr);
		dr.x+=8;
	}
	strncpy(oldmessage,message,69);
	glBindTexture(GL_TEXTURE_2D,film[2]);
	glTexImage2D(GL_TEXTURE_2D, 0, 4, temp->w, temp->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, temp->pixels);
	SDL_FreeSurface(temp);
}
