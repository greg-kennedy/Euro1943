/* message.cpp */
#include "message.h"

// Texture operations and fonts
#include "texops.h"

// Static, global values
#define NUM_SPEAKERS 3

#define MSG_LINE_LEN 80

#include <string.h>

// Textures
//  Speech bubble
static GLuint tex_bubble;
static GLuint tex_speaker[NUM_SPEAKERS];

// display-lists
static GLuint list_bubble;
static GLuint list_speaker;

// current state
static char msg_line[3][MSG_LINE_LEN];
static int msg_speaker;

int message_init()
{
	char buffer[64];
	int i;

	tex_bubble = load_texture("img/hud/bubble.png",GL_NEAREST,GL_NEAREST);

	// Speaker icons
	for (i=0; i<NUM_SPEAKERS; i++)
	{
		sprintf(buffer,"img/hud/speakers/%d.png",i);
		tex_speaker[i] = load_texture(buffer,GL_LINEAR, GL_LINEAR);
	}

	// one for the speech bubble
	list_bubble = glGenLists(1);
	glNewList(list_bubble, GL_COMPILE);
		// ENable alpha test: yes to transparency for speech bubble / speaker icon
		glEnable(GL_ALPHA_TEST);
		glBindTexture(GL_TEXTURE_2D, tex_bubble);
		glBegin(GL_QUADS);
			glBox(144,SCREEN_Y-32,SCREEN_X-288,32);
		glEnd();
	glEndList();

	// one for the current speaker (bind texture first...)
	list_speaker = glGenLists(1);
	glNewList(list_speaker, GL_COMPILE);
		glBegin(GL_QUADS);
			glBox(144,SCREEN_Y-32,32,32);
		glEnd();
	glEndList();

	return EXIT_SUCCESS;
}

void message_clear()
{
	msg_speaker = -1;
	memset(msg_line,'\0',MSG_LINE_LEN*3);
}

void message_post(int speaker, const char *message)
{
	memcpy(msg_line[2],msg_line[1],MSG_LINE_LEN);
	memcpy(msg_line[1],msg_line[0],MSG_LINE_LEN);
	memcpy(msg_line[0],message,MSG_LINE_LEN);
	msg_speaker = speaker;
}

void message_draw()
{
	glCallList(list_bubble);
	// if there is a speaker...
	if (msg_speaker >= 0)
	{
		glBindTexture(GL_TEXTURE_2D, tex_speaker[msg_speaker]);
		glCallList(list_speaker);
	}

	glDisable(GL_ALPHA_TEST);
	glDisable(GL_TEXTURE_2D);

	// draw both lines.
	glColor3f(0,0,0);
	glPrint(200,SCREEN_Y-5,msg_line[0]);
	glPrint(200,SCREEN_Y-13,msg_line[1]);
	glPrint(200,SCREEN_Y-21,msg_line[2]);
	glColor3f(1,1,1);
	glEnable(GL_TEXTURE_2D);
}

void message_quit()
{
	glDeleteLists(list_speaker, 1);
	glDeleteLists(list_bubble, 1);

	glDeleteTextures( NUM_SPEAKERS, tex_speaker );
	glDeleteTextures( 1, &tex_bubble );
}

