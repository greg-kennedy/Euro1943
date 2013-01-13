/* multimenu.cpp - multiplayer game menu */

#include "multimenu.h"
#include "texops.h"
#include "osc.h"

extern SDL_Surface *screen;
extern int sfxon, gamestate;
extern Mix_Music *music;
extern long mx, my;
extern int multiplayer, OS_PORT;

extern GLuint cursor, tex1;

extern char HOSTNAME[80], OS_LOC[80];

bool editingIP;

GLuint mmtex[2];
extern SDL_Surface *font;
SDL_Surface *ts, *ts2;
IPaddress *iplist;
int num;

void initmultimenu()
{
	glGenTextures( 1, &tex1 );
	glGenTextures( 2, mmtex );

	ts2 = loadGLStyle("img/ui/mm0.png");
	glBindTexture( GL_TEXTURE_2D, mmtex[0] );
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	ts = loadGLStyle("img/ui/mm1.png");
	glBindTexture( GL_TEXTURE_2D, mmtex[1] );
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	tex1 = load_texture("img/ui/multimenu.png",GL_LINEAR,GL_LINEAR);

	num=0;
	iplist = NULL;
	memset(HOSTNAME,0,80);
	sprintf(HOSTNAME,"localhost");

	multiplayer=1;
	editingIP=false;
	mmipup(HOSTNAME);
	mmosup();

	/* This is the music to play. */
	if (sfxon==1) {
		music = Mix_LoadMUS("audio/multimenu.mod");
		Mix_PlayMusic(music, -1);
	}
}

void destroymultimenu()
{
	if (num > 0) free(iplist);

	glDeleteTextures( 1, &tex1 );
	glDeleteTextures( 2, mmtex );
	SDL_FreeSurface(ts);

	Mix_HaltMusic();
	Mix_FreeMusic(music);
	music = NULL;
}

void drawmultimenu()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	//Enable texturing
	glEnable(GL_TEXTURE_2D);

	glBindTexture(GL_TEXTURE_2D, tex1); 
	glBegin(GL_QUADS);
	glTexCoord2f(0,0);
	glVertex2i(0, 0);
	glTexCoord2f(1,0);
	glVertex2i(800,0);
	glTexCoord2f(1,1);
	glVertex2i(800,600);
	glTexCoord2f(0,1);
	glVertex2i(0,600);
	glEnd();

	glBindTexture(GL_TEXTURE_2D, mmtex[0]);
	glBegin(GL_QUADS);
	glTexCoord2f(0,0);
	glVertex2i(51, 151);
	glTexCoord2f(1,0);
	glVertex2i(399,151);
	glTexCoord2f(1,1);
	glVertex2i(399,549);
	glTexCoord2f(0,1);
	glVertex2i(51,549);
	glEnd();

	glBindTexture(GL_TEXTURE_2D, mmtex[1]);
	glBegin(GL_QUADS);
	glTexCoord2f(0,0);
	glVertex2i(451, 151);
	glTexCoord2f(1,0);
	glVertex2i(749,151);
	glTexCoord2f(1,1);
	glVertex2i(749,199);
	glTexCoord2f(0,1);
	glVertex2i(451,199);
	glEnd();

	if (editingIP) {
		glColor3f(0.7f,0.7f,0.0f);

		//glBindTexture(GL_TEXTURE_2D, 0);
		glDisable(GL_TEXTURE_2D);
		glBegin(GL_QUADS);
		glVertex3i(637,200, 0);
		glVertex3i(749,200, 0);
		glVertex3i(749,249, 0);
		glVertex3i(637,249, 0);
		glEnd();
		glEnable(GL_TEXTURE_2D);
		glColor3f(1.0f,1.0f,1.0f);
	}

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

bool handlemultimenu()
{
	SDL_Event event;
	bool retval=true;

	/* Check for events */
	while (SDL_PollEvent (&event))
	{
		switch (event.type)
		{
			case SDL_KEYUP:
				if (editingIP) {
					if (event.key.keysym.sym == SDLK_RETURN || event.key.keysym.sym == SDLK_ESCAPE)
						editingIP=false;
					else if (event.key.keysym.sym == SDLK_BACKSPACE && strlen(HOSTNAME) > 0)
						HOSTNAME[strlen(HOSTNAME)-1]='\0';
					else if (strlen(HOSTNAME) < 31 && (event.key.keysym.sym == '.' || (event.key.keysym.sym == ':') || (event.key.keysym.sym >= '0' && event.key.keysym.sym <= '9') || (event.key.keysym.sym >= 'a' && event.key.keysym.sym <= 'z') || (event.key.keysym.sym >= 'A' && event.key.keysym.sym <= 'Z')))
						HOSTNAME[strlen(HOSTNAME)]=(char)(event.key.keysym.sym);
					else if (strlen(HOSTNAME) < 31 && (event.key.keysym.sym == ';' || event.key.keysym.sym == ' '))
						HOSTNAME[strlen(HOSTNAME)]=':';
					mmipup(HOSTNAME);
				} else {
					if (event.key.keysym.sym == SDLK_ESCAPE)
						gamestate=0;
				}
				break;
			case SDL_MOUSEBUTTONUP:
				if (editingIP) editingIP=false; else {
					if (event.button.x > 637 && event.button.x < 749 && event.button.y > 200 && event.button.y < 249) editingIP=true;
					else if (event.button.x > 451 && event.button.x < 749 && event.button.y > 440 && event.button.y < 488) gamestate=10;
					else if (event.button.x > 451 && event.button.x < 749 && event.button.y > 500 && event.button.y < 549) gamestate=0;
					else if (event.button.x > 51 && event.button.x < 399 && event.button.y > 151 && event.button.y < 549 && num>0)
					{
						if ((event.button.y - 151) / 14 >= num-1)
							sprintf(HOSTNAME,"%d.%d.%d.%d:%d",iplist[num-1].host & 0x000000FF,
									(iplist[num-1].host & 0x0000FF00) >> 8,
									(iplist[num-1].host & 0x00FF0000) >> 16,
									(iplist[num-1].host & 0xFF000000) >> 24, iplist[num-1].port);
						else
							sprintf(HOSTNAME,"%d.%d.%d.%d:%d",iplist[(event.button.y - 151) / 14].host & 0x000000FF,
									(iplist[(event.button.y - 151) / 14].host & 0x0000FF00) >> 8,
									(iplist[(event.button.y - 151) / 14].host & 0x00FF0000) >> 16,
									(iplist[(event.button.y - 151) / 14].host & 0xFF000000) >> 24, iplist[(event.button.y - 151) / 14].port);
						//                     strcpy(HOSTNAME,iplist[(event.button.y - 151) / 14]);
						mmipup(HOSTNAME);
					}
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

void mmipup (char *message)
{
	unsigned int i;
	SDL_Surface *temp;
	SDL_Rect sr,dr;
	sr.h=8;
	sr.w=8;
	sr.y=0;
	dr.x=2;
	dr.y=12;
	dr.w=8;
	dr.h=8;
	temp = SDL_ConvertSurface(ts,ts->format,ts->flags);

	for (i=0; i<strlen(message); i++)
	{
		sr.x = 8 * (message[i] & 0x7F);
		SDL_BlitSurface(font,&sr,temp,&dr);
		dr.x+=8;
	}
	glBindTexture(GL_TEXTURE_2D,mmtex[1]);
	glTexImage2D(GL_TEXTURE_2D, 0, 3, temp->w, temp->h, 0, GL_RGB, GL_UNSIGNED_BYTE, temp->pixels);
	SDL_FreeSurface(temp);
}

void mmosup ()
{
	unsigned int i;
	int j;
	char buffer[22];

	SDL_Surface *temp;
	SDL_Rect sr,dr;
	sr.h=8;
	sr.w=8;
	sr.y=0;
	dr.x=1;
	dr.y=1;
	dr.w=8;
	dr.h=8;
	temp = SDL_ConvertSurface(ts2,ts2->format,ts2->flags);

	num=GetMetaserverBlock (OS_LOC, OS_PORT, 5, 0, &iplist);

	for (j=0; j<num;j++)
	{
		sprintf(buffer,"%d.%d.%d.%d:%d",iplist[j].host & 0x000000FF,
				(iplist[j].host & 0x0000FF00) >> 8,
				(iplist[j].host & 0x00FF0000) >> 16,
				(iplist[j].host & 0xFF000000) >> 24, iplist[j].port);
		for (i=0; i<strlen(buffer); i++)
		{
			sr.x = 8 * (buffer[i] & 0x7F);
			SDL_BlitSurface(font,&sr,temp,&dr);
			dr.x+=8;
		}
		dr.y+=9;
		dr.x = 1;
	}

	glBindTexture(GL_TEXTURE_2D,mmtex[0]);
	glTexImage2D(GL_TEXTURE_2D, 0, 3, temp->w, temp->h, 0, GL_RGB, GL_UNSIGNED_BYTE, temp->pixels);
	SDL_FreeSurface(temp);
}
