/* multimenu.cpp - multiplayer game menu */

#include "multimenu.h"
#include "texops.h"

// used to draw text on screen
#include "message.h"

// used by Overserver
#include "osc.h"

// Externs used by this sub-section
extern unsigned char vol_music;
extern long mx, my;
extern GLuint list_cursor;

// Hostname to connect to
extern char HOSTNAME[80];

extern char OS_LOC[80];
extern unsigned short OS_PORT;

static int min(int a, int b)
{
	if (a < b) return a;
	return b;
}

char do_gs_multimenu()
{
	// Multimenu init section.
	int num=0, i;
	IPaddress *iplist = NULL;
	int picked_server = -1;

	char buffer[80];

	memset(HOSTNAME,0,80);
	sprintf(HOSTNAME,"localhost");

	unsigned char editingIP=0;

	// Go ask OverServer for the top block of IPs.
	num=GetMetaserverBlock (OS_LOC, OS_PORT, 5, 0, &iplist);

	//  Load texture from disk.
	GLuint tex_multimenu = load_texture("img/ui/multimenu.png",GL_LINEAR,GL_LINEAR);
	if (!tex_multimenu)
		return 0;

	// Make a display list in which we draw a full-screen quad.
	GLuint list_multimenu = glGenLists(1);
	glNewList(list_multimenu, GL_COMPILE);
		// DISable alpha test: no transparency for backdrop!
		glDisable(GL_ALPHA_TEST);
		// bind cursor texture
		glBindTexture(GL_TEXTURE_2D, tex_multimenu);
		// draw a quad, top-left corner at 0,0
		glBegin(GL_QUADS);
			glBox(0,0,SCREEN_X,SCREEN_Y);
		glEnd();
	glEndList();

	/* Play music */
	Mix_Music *music = music_play("audio/multimenu.mod");

	// GL setup for this screen: no alpha blending, yes texturing
	glDisable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);

	// dirty flag (needs redraw)
	unsigned char dirty=1;
	char retval = gs_multimenu;

	while (retval == gs_multimenu)
	{
		if (dirty)
		{
			// Draw backdrop.  This is full-screen thus overwrites any prev. color
//			glClear(GL_COLOR_BUFFER_BIT);
			glCallList(list_multimenu);

			// Draw UI texts on the menu.
			// First, the entered IP address.
			if (! editingIP)
				glColor3f(0.0,0.0,0.0);
			else
				glColor3f(1.0,1.0,0.0);
			glPrint(460, 180, HOSTNAME);

			glColor3f(0.0,0.0,0.0);

			// Next, loop for overserver-matched servers.
			for (i = 0; i < num && i < 39; i++)
			{
				if (picked_server == i)
					glColor3f(1.0,1.0,0.0);
				
				sprintf(buffer,"%d.%d.%d.%d:%d",iplist[i].host & 0x000000FF,
						(iplist[i].host & 0x0000FF00) >> 8,
						(iplist[i].host & 0x00FF0000) >> 16,
						(iplist[i].host & 0xFF000000) >> 24, iplist[i].port);
				glPrint(60, 160 + (10 * i), buffer);

				if (picked_server == i)
					glColor3f(0.0,0.0,0.0);
			}

			glColor3f(1.0,1.0,1.0);

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
					if (editingIP &&
						(event.key.keysym.sym == SDLK_RETURN || event.key.keysym.sym == SDLK_ESCAPE)) {
							editingIP=0;
							dirty=1;
					        SDL_EnableUNICODE( 0 );
					}
					else if (!editingIP && event.key.keysym.sym == SDLK_ESCAPE) {
						retval=gs_title;
					}
					break;
				case SDL_KEYDOWN:
					if (editingIP) {
						if (event.key.keysym.sym == SDLK_BACKSPACE && strlen(HOSTNAME) > 0)
							HOSTNAME[strlen(HOSTNAME)-1]='\0';
						else if (strlen(HOSTNAME) < 31 && event.key.keysym.unicode < 0x80 && event.key.keysym.unicode >= 0x20) {
							HOSTNAME[strlen(HOSTNAME)]=(char)(event.key.keysym.unicode);							
						}
					dirty = 1;
					}
					break;
				case SDL_MOUSEBUTTONUP:
					if (editingIP) {
						editingIP=0;
						dirty=1;
				        SDL_EnableUNICODE( 0 );
					} else {
						if (event.button.x > 637 && event.button.x < 749 && event.button.y > 200 && event.button.y < 249) {
						picked_server = -1;
						editingIP=1;
						dirty = 1;

				        /* Enable Unicode translation */
        				SDL_EnableUNICODE( 1 );
					} else if (event.button.x > 451 && event.button.x < 749 && event.button.y > 440 && event.button.y < 488) retval = gs_game;
					else if (event.button.x > 451 && event.button.x < 749 && event.button.y > 500 && event.button.y < 549) retval = gs_title;
					else if (event.button.x > 51 && event.button.x < 399 && event.button.y > 155 && event.button.y < 544 && num>0)
					{
						picked_server = min((event.button.y - 155) / 10, num - 1);
						sprintf(HOSTNAME,"%d.%d.%d.%d:%d",iplist[picked_server].host & 0x000000FF,
								(iplist[picked_server].host & 0x0000FF00) >> 8,
								(iplist[picked_server].host & 0x00FF0000) >> 16,
								(iplist[picked_server].host & 0xFF000000) >> 24, iplist[picked_server].port);
						dirty = 1;
					}
				}
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

	// Delete IP address list, if it had anything in it.
	if (num > 0) free(iplist);

	// Stop music playback, if it was playing
	if (music) Mix_FreeMusic(music);

	// Clean up OpenGL stuff for this screen
	glDeleteLists(list_multimenu, 1);
	glDeleteTextures( 1, &tex_multimenu );

	return retval;
}

/*
extern SDL_Surface *screen;
extern int vol_music, gamestate;
static Mix_Music *music;
extern long mx, my;
extern int multiplayer, OS_PORT;

extern GLuint list_cursor;
GLuint tex1;

extern char HOSTNAME[80], OS_LOC[80];

unsigned char editingIP;

GLuint mmtex[2];
extern SDL_Surface *font;
SDL_Surface *ts, *ts2;
IPaddress *iplist;
int num;

int multiplayer, OS_PORT;

char HOSTNAME[80], OS_LOC[80];
UDPsocket sd;
IPaddress srvadd;
UDPpacket *p;

	glVertex2i(51, 151);

	glVertex2i(451, 151);

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


			case SDL_KEYUP:
				if (editingIP) {
					if (event.key.keysym.sym == SDLK_RETURN || event.key.keysym.sym == SDLK_ESCAPE)
						editingIP=0;
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
				if (editingIP) editingIP=0; else {
					if (event.button.x > 637 && event.button.x < 749 && event.button.y > 200 && event.button.y < 249) editingIP=1;
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
						//					 strcpy(HOSTNAME,iplist[(event.button.y - 151) / 14]);
						mmipup(HOSTNAME);
					}
				}
				break;
			case SDL_MOUSEMOTION:
				mx=event.motion.x;
				my=event.motion.y;
				break;
			case SDL_QUIT:
				retval = 0;
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
}*/
