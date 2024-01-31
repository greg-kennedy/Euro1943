/* EURO1943 - GREG KENNEDY
	https://greg-kennedy.com */

/* main.cpp - game flow goes here! */

// common file includes
#include "common_client.h"

// config-file parser
#include "cfg_parse.h"

// texture operations
#include "function.h"
// message-box (speech bubble)
#include "message.h"

// one include file per game section
//  title screen
#include "title.h"
//  options screen
#include "options.h"
//  multiplayer (client) menu
#include "multimenu.h"
//  cutscene-playback
#include "cutscene.h"
//  in-game mode
#include "game.h"
//  win or loss screen
#include "winlose.h"

// Other system-wide includes
#include <time.h>

// Global variables, shared across multiple subsystems.
// Holds music and sound effect volume.  Set to 0 to disable.
unsigned char vol_music, vol_sfx;
// Is the network OK?
unsigned char network;
// Location of remote OverServer host, port
char OS_LOC[80];
unsigned short OS_PORT;

// Is this a multiplayer game?
int multiplayer;
// Which level are we on?
int level;
// Who are we connected to?
char HOSTNAME[80];

// Mouse X / Y position.
long mx=0, my=0;
// Mouse cursor display list
GLuint list_cursor;

// Static vars go here - used in multiple main.c functions, but nowhere else
// Mouse cursor texture
static GLuint tex_cursor;

///////////////////////
// Functions here.  Set up cfg_ structure.
static struct cfg_struct *cfg_setup()
{
	// defaults for numeric-types need an sprintf
	char cfg_val[12] = "";

	// Initialize config struct
	struct cfg_struct *cfg = cfg_init();

	// Specifying some defaults
	cfg_set(cfg,"VIDEO_WIDTH","800");
	cfg_set(cfg,"VIDEO_HEIGHT","600");
	cfg_set(cfg,"VIDEO_DEPTH","16");
	cfg_set(cfg,"VIDEO_FULLSCREEN","1");

	sprintf(cfg_val,"%d",MIX_MAX_VOLUME);
	cfg_set(cfg,"AUDIO_VOLUME_MUSIC",cfg_val);
	cfg_set(cfg,"AUDIO_VOLUME_SFX",cfg_val);
	sprintf(cfg_val,"%d",MIX_DEFAULT_FREQUENCY);
	cfg_set(cfg,"AUDIO_FREQUENCY",cfg_val);
	sprintf(cfg_val,"%d",MIX_DEFAULT_FORMAT);
	cfg_set(cfg,"AUDIO_FORMAT",cfg_val);
	sprintf(cfg_val,"%d",MIX_DEFAULT_CHANNELS);
	cfg_set(cfg,"AUDIO_OUTPUT_CHANNELS",cfg_val);
	sprintf(cfg_val,"%d",MIX_DEFAULT_CHUNKSIZE);
	cfg_set(cfg,"AUDIO_CHUNKSIZE",cfg_val);
	sprintf(cfg_val,"%d",MIX_CHANNELS);
	cfg_set(cfg,"AUDIO_MIXER_CHANNELS",cfg_val);

	// overserver port
	sprintf(cfg_val,"%d",DEFAULT_OS_PORT);
	cfg_set(cfg,"OVERSERVER_PORT",cfg_val);
	cfg_set(cfg,"OVERSERVER_HOST",DEFAULT_OS_HOST);

	// Replace default values with those sourced from real .ini file
	cfg_load(cfg,"config.ini");

	return cfg;
}

////////////
// Init core items.
static int core_init(struct cfg_struct *cfg)
{
	// Initialize SDL
	if(SDL_Init(SDL_INIT_TIMER | SDL_INIT_AUDIO | SDL_INIT_VIDEO) == -1) {
		fprintf(stderr, "Failed to initialize SDL: %s\n", SDL_GetError());
		return EXIT_FAILURE;
	}
	atexit(SDL_Quit);

	// Initialize SDL_Image
	// load support for the PNG image formats - required for the game to work
	//  return failure if we can't open these at all.
	if(IMG_Init(IMG_INIT_PNG) != IMG_INIT_PNG) {
		fprintf(stderr,"IMG_Init: Failed to init required jpg and png support!\n");
		fprintf(stderr,"IMG_Init: %s\n", IMG_GetError());
		SDL_Quit();
		return EXIT_FAILURE;
	}

	/* Initialize SDL_Mixer */
	// Load support for MOD and XM formats.
	//  If that fails, just turn off music volume and carry on.  We may still get WAV support anyway.
	if(Mix_Init(MIX_INIT_MOD) != MIX_INIT_MOD) {
		fprintf(stderr,"Mix_Init: Failed to init required mod support!\n");
		fprintf(stderr,"Mix_Init: %s\n", Mix_GetError());
		vol_music = 0;
	}

	/* Initialize SDL_net */
	// Network support is needed for multiplayer games.
	//  If for some reason that doesn't work, still allow single player.
	if (SDLNet_Init() < 0)
	{
		fprintf(stderr, "SDLNet_Init: %s\n", SDLNet_GetError());
		network = 0;
	} else {
		network = 1;
	}

	// Read OverServer values from cfg file
	strncpy(OS_LOC,cfg_get(cfg,"OVERSERVER_HOST"),80);
	OS_PORT = atoi(cfg_get(cfg,"OVERSERVER_PORT"));

	// Mess with SDL_Event filtering.
	SDL_EventState(SDL_ACTIVEEVENT, SDL_IGNORE);
	SDL_EventState(SDL_JOYAXISMOTION, SDL_IGNORE);
	SDL_EventState(SDL_JOYBALLMOTION, SDL_IGNORE);
	SDL_EventState(SDL_JOYHATMOTION, SDL_IGNORE);
	SDL_EventState(SDL_JOYBUTTONDOWN, SDL_IGNORE);
	SDL_EventState(SDL_JOYBUTTONUP, SDL_IGNORE);
	SDL_EventState(SDL_VIDEORESIZE, SDL_IGNORE);
	//SDL_EventState(SDL_SYSWMEVENT, SDL_IGNORE);
	SDL_EventState(SDL_USEREVENT, SDL_IGNORE);

	return EXIT_SUCCESS;
}

static int video_init(struct cfg_struct *cfg)
{
	int bpp = atoi(cfg_get(cfg,"VIDEO_DEPTH"));
	int w = atoi(cfg_get(cfg,"VIDEO_WIDTH"));
	int h = atoi(cfg_get(cfg,"VIDEO_HEIGHT"));
	int f = atoi(cfg_get(cfg,"VIDEO_FULLSCREEN"));

	// OpenGL attribute list.  Defaults are commented out.
	// OpenGL attributes for 8-, 16- and 32-bit color depth
	if (bpp < 9)
	{
		// SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 3);	  //Use 3 bits of Red
		// SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 3);	  //Use 3 bits of Green
		// SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 2);	  //Use 2 bits of Blue
	} else if (bpp < 16)
	{
		// 16-bit: 555
		SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);	  //Use at least 5 bits of Red
		SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 5);	  //Use at least 5 bits of Green
		SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);	  //Use at least 5 bits of Blue
	} else if (bpp < 17)
	{
		// 16-bit: 565
		SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);	  //Use at least 5 bits of Red
		SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 6);	  //Use at least 5 bits of Green
		SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);	  //Use at least 5 bits of Blue
	} else {
		// 24, 32-bit: 888
		SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);	  //Use at least 8 bits of Red
		SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);	  //Use at least 8 bits of Green
		SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);	  //Use at least 8 bits of Blue
	}

	// other GL attributes follow
	// SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 0);
	// SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE, 0);
	// SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);   //Enable double buffering
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);	  // all 2d game: no depth buffer needed
	// SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 0);
	// SDL_GL_SetAttribute(SDL_GL_ACCUM_RED_SIZE, 0);
	// SDL_GL_SetAttribute(SDL_GL_ACCUM_GREEN_SIZE, 0);
	// SDL_GL_SetAttribute(SDL_GL_ACCUM_BLUE_SIZE, 0);
	// SDL_GL_SetAttribute(SDL_GL_ACCUM_ALPHA_SIZE, 0);
	// SDL_GL_SetAttribute(SDL_GL_STEREO, 0);
	// SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);
	// SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 0);
	// SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
	// the following work only in SDL 1.3+
	//SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 1);
	//SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
	//SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
	//SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
	//SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 0);
	//SDL_GL_SetAttribute(SDL_GL_FRAMEBUFFER_SRGB_CAPABLE, 0);

	// Create the SDL surface window.
	if (f)
		SDL_SetVideoMode(w, h, bpp, SDL_OPENGL | SDL_FULLSCREEN);
	else
		SDL_SetVideoMode(w, h, bpp, SDL_OPENGL);

	if (!SDL_GetVideoSurface())
	{
		fprintf(stderr, "Unable to set video mode: %s\n", SDL_GetError());
		return EXIT_FAILURE;
	}

	// Set window caption and turn off display cursor.
	SDL_WM_SetCaption ("Euro1943", NULL);
	SDL_ShowCursor( SDL_DISABLE );

	// Set up orthographic viewport.  Lock ourselves at 800x600.
	glViewport(0,0,w,h);
	glMatrixMode(GL_PROJECTION);
//	glLoadIdentity();
	glOrtho(0.0f, SCREEN_X, SCREEN_Y, 0.0f, -1.0f, 1.0f);
	glMatrixMode(GL_MODELVIEW);
//	glLoadIdentity();

	// Set the clear color to all black (0, 0, 0)
	glClearColor(0.0f,0.0f,0.0f,1.0f);

	// Two ways to do transparency: simple alpha-test, and blend.
	//  Blend is used for special effects, alpha func otherwise.
	glAlphaFunc(GL_NOTEQUAL,0.0f);
	//glEnable(GL_ALPHA_TEST);
	// GL blending is needed for more complex transparency
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	//glEnable(GL_BLEND);

	//glShadeModel(GL_SMOOTH);
	//glDisable(GL_CULL_FACE);
//	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

	glPixelStorei (GL_UNPACK_ALIGNMENT, 1);

	int GLTexSize;
	printf("OpenGL information:\nVendor: %s\nRenderer: %s\nVersion: %s\nExtensions: %s\n",
		glGetString( GL_VENDOR ),
		glGetString( GL_RENDERER ),
		glGetString( GL_VERSION ),
		glGetString( GL_EXTENSIONS ) );

	glGetIntegerv( GL_MAX_TEXTURE_SIZE, & GLTexSize ) ;
	printf("Max texture size: %d\n",GLTexSize);

	return EXIT_SUCCESS;
}

static void audio_init(struct cfg_struct *cfg)
{
	int frequency = atoi(cfg_get(cfg,"AUDIO_FREQUENCY"));
	unsigned short format = (unsigned short)atoi(cfg_get(cfg,"AUDIO_FORMAT"));
	int output_channels = atoi(cfg_get(cfg,"AUDIO_OUTPUT_CHANNELS"));
	int chunksize = atoi(cfg_get(cfg,"AUDIO_CHUNKSIZE"));

	int mixer_channels = atoi(cfg_get(cfg,"AUDIO_MIXER_CHANNELS"));

	if(Mix_OpenAudio(frequency, format, output_channels, chunksize))
	{
		fprintf(stderr,"Unable to open audio!\n");
		vol_music=0;
		vol_sfx=0;
	} else {
		Mix_AllocateChannels(mixer_channels);
		// Set initial volume for all items.
		vol_sfx = atoi(cfg_get(cfg,"AUDIO_VOLUME_SFX"));
		vol_music = atoi(cfg_get(cfg,"AUDIO_VOLUME_MUSIC"));

		Mix_Volume(-1, vol_sfx);
		Mix_VolumeMusic(vol_music);
	}

	// get and print the audio format in use
	int numtimesopened;
	numtimesopened=Mix_QuerySpec(&frequency, &format, &output_channels);
	if (!numtimesopened) {
		fprintf(stderr,"Mix_QuerySpec: %s\n",Mix_GetError());
	} else {
		mixer_channels = Mix_AllocateChannels(-1);
		const char *format_str="Unknown";
		switch(format) {
			case AUDIO_U8: format_str="U8"; break;
			case AUDIO_S8: format_str="S8"; break;
			case AUDIO_U16LSB: format_str="U16LSB"; break;
			case AUDIO_S16LSB: format_str="S16LSB"; break;
			case AUDIO_U16MSB: format_str="U16MSB"; break;
			case AUDIO_S16MSB: format_str="S16MSB"; break;
		}
		printf("Audio opened=%d times  frequency=%dHz  format=%s  channels=%d mixer_channels=%d\n",
			numtimesopened, frequency, format_str, output_channels,mixer_channels);
	}
}

static int shared_resource_init()
{
	// Set up mouse cursor: load cursor texture first.
	tex_cursor = load_texture("img/ui/cursor.png",GL_NEAREST,GL_NEAREST);
	if (!tex_cursor)
		return EXIT_FAILURE;

	// Make a display list for the mouse cursor.
	list_cursor = glGenLists(1);
	glNewList(list_cursor, GL_COMPILE);
		// enable alpha test for simple transparency
		glEnable(GL_ALPHA_TEST);
		// bind cursor texture
		glBindTexture(GL_TEXTURE_2D, tex_cursor);
		// draw a quad centered around 0,0
		glBegin(GL_QUADS);
			glTexCoord2f(0,0);
			glVertex2i(-16, -16);
			glTexCoord2f(1,0);
			glVertex2i(16, -16);
			glTexCoord2f(1,1);
			glVertex2i(16, 16);
			glTexCoord2f(0,1);
			glVertex2i(-16, 16);
		glEnd();
	glEndList();

	// Set up message box.
	glFontInit();
	if (message_init()) return EXIT_FAILURE;

	return EXIT_SUCCESS;
}

static void shared_resource_quit()
{
	message_quit();
	glFontQuit();

	glDeleteLists(list_cursor, 1);
	glDeleteTextures( 1, &tex_cursor );
}

static void audio_quit()
{
	Mix_CloseAudio();
}

static void video_quit()
{
	SDL_ShowCursor( SDL_ENABLE );
}

static void core_quit()
{
	SDLNet_Quit();
	Mix_Quit();
	IMG_Quit();
	SDL_Quit();
}

// Defines - help to prettify the main loop.
//#define state_func(X) isDone = do_ ## X ();
#define switch_state(X) case X: \
	gamestate = do_ ## X (); \
	break;

int main(int argc, char *argv[])
{
	// boilerplate startup message
	printf("Euro1943 - v%s\nGreg Kennedy 2013\n\n", VERSION);

	srand((unsigned int)time(NULL));

	// .ini file configuration
	// Pointer to a cfg_struct structure
	struct cfg_struct *cfg = cfg_setup();

	// Initialize SDL subsystems.
	if (!core_init(cfg))
	{
		// Go set up video.
		if (!video_init(cfg))
		{
			// Go set up audio.
			audio_init(cfg);

			// Load basic (globally shared) resources.
			if (!shared_resource_init())
			{
				// Holds current gamestate.  Change this from within a subsystem, and
				//  the current sub-state will shut down, while the next will start.
				char gamestate = gs_title;
				
				while(gamestate != gs_exit)
				{
					switch(gamestate)
					{
						switch_state(gs_title);
						switch_state(gs_multimenu);
						switch_state(gs_options);
						switch_state(gs_cutscene);
						switch_state(gs_win);
						switch_state(gs_lose);
						switch_state(gs_game);
						default:
							gamestate = gs_exit;
							fprintf(stderr, "Error:  Game reached unknown gamestate %d.\n",gamestate);
					}
				}

				shared_resource_quit();
			}
				
			audio_quit();

			video_quit();
		}
		core_quit();
	}

	// Dump cfg-struct to disk.
	cfg_save(cfg,"config.ini");
	// All done, clean up.
	cfg_free(cfg);

	return 0;
}
