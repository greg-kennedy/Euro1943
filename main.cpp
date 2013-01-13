/* main.cpp - game flow goes here! */

#include "common.h"

#include "title.h"
#include "options.h"
#include "multimenu.h"
#include "cutscene.h"
#include "game.h"
#include "winlose.h"
#include "texops.h"

#include "osc.h"

SDL_Surface *screen = NULL, *tempsurf=NULL, *font;
int gamestate, muson, sfxon;
Mix_Music *music;
SDL_Rect mcrect;
long mx=0, my=0;

int multiplayer, level, OS_PORT;

char HOSTNAME[80], OS_LOC[80];
UDPsocket sd;
IPaddress srvadd;
UDPpacket *p;

int GLTexSize;

GLuint cursor, tex1;

int main(int argc, char *argv[])
{
	FILE *inifile;
	bool isRunning = true;

	// Initialize SDL
	if(SDL_Init(SDL_INIT_EVERYTHING) == -1){
		fprintf(stderr, "Failed to initialize SDL: %s\n", SDL_GetError());
		exit(1);
	}
	atexit(SDL_Quit);

    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);      //Use at least 5 bits of Red
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 5);      //Use at least 5 bits of Green
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);      //Use at least 5 bits of Blue
//    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);      //Use at least 16 bits for the depth buffer
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);      //Use at least 16 bits for the depth buffer
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);   //Enable double buffering

	// Initilize the screen
	if (argc > 1 && strcmp(argv[1],"-w") == 0)
		screen = SDL_SetVideoMode(SCREEN_X, SCREEN_Y, 0, SDL_OPENGL ); //| SDL_FULLSCREEN );
	else
		screen = SDL_SetVideoMode(SCREEN_X, SCREEN_Y, 0, SDL_OPENGL | SDL_FULLSCREEN );

	if(screen == NULL){
		fprintf(stderr, "Unable to set video mode: %s\n", SDL_GetError());
		exit(1);
	}

    SDL_WM_SetCaption ("euro1943", NULL);
    SDL_ShowCursor( SDL_DISABLE );

    glViewport(0,0,SCREEN_X,SCREEN_Y);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0f, SCREEN_X, SCREEN_Y, 0.0f, -1.0f, 1.0f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    //Clear the screen to blue and turn a few OpenGL switches on/off.
    glClearColor(0.0f,0.0f,0.0f,1.0f);
//    glClearDepth(1.0f);
    //glDepthFunc(GL_LEQUAL); 
    //glEnable(GL_DEPTH_TEST);

//    glAlphaFunc(GL_GREATER, 0.1f);
//    glEnable(GL_ALPHA_TEST);
glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
glEnable(GL_BLEND);

//    glShadeModel(GL_SMOOTH);
    glDisable(GL_CULL_FACE);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST); 

    printf("OpenGL information:\nVendor: %s\nRenderer: %s\nVersion: %s\nExtensions: %s\n",
      glGetString( GL_VENDOR ),
      glGetString( GL_RENDERER ),
      glGetString( GL_VERSION ),
      glGetString( GL_EXTENSIONS ) );
    glGetIntegerv( GL_MAX_TEXTURE_SIZE, & GLTexSize ) ;
    printf("Max texture size: %d\n",GLTexSize);

    inifile = fopen("config.ini","r");
    if (inifile == NULL)
    {
    	sfxon=1;
    	strcpy(OS_LOC,"nwserver.ath.cx");
    	OS_PORT=5009;
    } else {
        if (fscanf(inifile,"%d\n%s\n%d\n",&sfxon,OS_LOC,&OS_PORT) != 3)
	{
    		sfxon=1;
	    	strcpy(OS_LOC,"nwserver.ath.cx");
	    	OS_PORT=5009;
	}
        fclose(inifile);
    }

    if(Mix_OpenAudio(22050, AUDIO_S16, 2, 4096)) {
      printf("Unable to open audio!\n");
      sfxon=0;
      //exit(1);
    }

	/* Initialize SDL_net */
	if (SDLNet_Init() < 0)
	{
		fprintf(stderr, "SDLNet_Init: %s\n", SDLNet_GetError());
		exit(EXIT_FAILURE);
	}

    cursor = load_texture("img/ui/cursor.png",GL_NEAREST,GL_NEAREST);

    if (cursor == 0) {	fprintf(stderr, "Cannot load cursor.png\n"); exit(1); }

    font=loadGLStyle("img/hud/font.png");

	gamestate = 0;
	music=NULL;
	level=0;
	
	strcpy(HOSTNAME,"localhost");

	while(isRunning == true){
        if(gamestate==0)
        {
            inittitle();
            while (isRunning && gamestate==0)
            {
                isRunning=handletitle();
                drawtitle();
            }
            destroytitle();
        }else if (gamestate==1)
        {
            initmultimenu();
            while (isRunning && gamestate==1)
            {
                isRunning=handlemultimenu();
                drawmultimenu();
            }
            destroymultimenu();
        }else if (gamestate==2)
        {
            initoptions();
            while (isRunning && gamestate==2)
            {
                isRunning=handleoptions();
                drawoptions();
            }
            destroyoptions();
        }else if (gamestate==3)
        {
            initcutscene(level);
            while (isRunning && gamestate==3)
            {
                isRunning=handlecutscene();
                drawcutscene();
            }
            destroycutscene();
        }else if (gamestate==4)
        {
            initwin();
            while (isRunning && gamestate==4)
            {
                isRunning=handlewin();
                drawwin();
            }
            destroywin();
        }else if (gamestate==5)
        {
            initlose();
            while (isRunning && gamestate==5)
            {
                isRunning=handlelose();
                drawlose();
            }
            destroylose();
        }else if (gamestate==10)
        {
/*		printf("Setting HOSTNAME to %s\n",argv[1]);
                if (multiplayer==1)
                if (argc > 1) strcpy(HOSTNAME,argv[1]);
                else strcpy(HOSTNAME,"localhost");*/
            initgame();
            while (isRunning && gamestate==10)
            {
                isRunning=handlegame();
                if (multiplayer==1)
                  networkgame();
                else
                  singlegame();
                drawgame();
            }
            destroygame();
        }else{
             isRunning=false;
             fprintf(stderr, "Error:  Game reached unknown gamestate %d.\n",gamestate);
        }
    }

    inifile = fopen("config.ini","w");
    if (inifile != NULL) {
       fprintf(inifile,"%d\n%s\n%d\n",sfxon,OS_LOC,OS_PORT);
       fclose(inifile);
    }

    glDeleteTextures( 1, &cursor );
    SDL_FreeSurface(font);

    SDL_ShowCursor( SDL_ENABLE );
    SDLNet_Quit();
    Mix_CloseAudio();
    SDL_Quit();

	return 0;
}
