/* EURO1943 - GREG KENNEDY
	https://greg-kennedy.com */

/* main.cpp - game flow goes here! */

// common file includes
#include "common_client.h"

// config-file parser
#include "cfg_parse.h"

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
#include <SDL/SDL_image.h>
#include <SDL/SDL_net.h>
#include <time.h>

extern struct env_t env;

// since SDL_Mixer didn't see fit to define one
#define MIX_DEFAULT_CHUNKSIZE 1024

/* ************************************************************************ */
// helper int-to-string function
static const char * to_string(int i)
{
	static char buffer[12] = { 0 };
	sprintf(buffer, "%d", i);
	return buffer;
}

/* ************************************************************************ */
static struct cfg_struct * cfg_setup(const char * filename)
{
	struct cfg_struct * c = cfg_init();
	// Specifying some defaults
	cfg_set(c, "VIDEO_WIDTH", "800");
	cfg_set(c, "VIDEO_HEIGHT", "600");
	cfg_set(c, "VIDEO_DEPTH", "16");
	cfg_set(c, "VIDEO_FULLSCREEN", "1");
	cfg_set(c, "VIDEO_VSYNC", "1");
	cfg_set(c, "VIDEO_MSAA", "0");
	cfg_set(c, "AUDIO_VOLUME", to_string(MIX_MAX_VOLUME));
	cfg_set(c, "AUDIO_FREQUENCY", to_string(MIX_DEFAULT_FREQUENCY));
	cfg_set(c, "AUDIO_FORMAT", to_string(MIX_DEFAULT_FORMAT));
	cfg_set(c, "AUDIO_OUTPUT_CHANNELS", to_string(MIX_DEFAULT_CHANNELS));
	cfg_set(c, "AUDIO_CHUNKSIZE", to_string(MIX_DEFAULT_CHUNKSIZE));
	cfg_set(c, "AUDIO_MIXER_CHANNELS", to_string(MIX_CHANNELS));
	// overserver port
	cfg_set(c, "OVERSERVER_PORT", to_string(DEFAULT_OS_PORT));
	cfg_set(c, "OVERSERVER_HOST", DEFAULT_OS_HOST);
	// Replace default values with those sourced from real .ini file
	cfg_load(c, filename);
	return c;
}

/* ************************************************************************ */
// Initialize SDL
static int init_sdl()
{
	// you need AT LEAST the timer and video working to play
	//  alas, video needs to happen here, it doesn't want to be initialized later
	if (SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO) == -1) {
		fprintf(stderr, "Failed to initialize SDL: %s\n", SDL_GetError());
		return 0;
	}

	// Mess with SDL_Event filtering.  ignore any event type we don't care about,
	//  otherwise it gets pushed in the queue and we would just ignore it there
	SDL_EventState(SDL_ACTIVEEVENT, SDL_IGNORE);
	SDL_EventState(SDL_JOYAXISMOTION, SDL_IGNORE);
	SDL_EventState(SDL_JOYBALLMOTION, SDL_IGNORE);
	SDL_EventState(SDL_JOYHATMOTION, SDL_IGNORE);
	SDL_EventState(SDL_JOYBUTTONDOWN, SDL_IGNORE);
	SDL_EventState(SDL_JOYBUTTONUP, SDL_IGNORE);
	SDL_EventState(SDL_VIDEORESIZE, SDL_IGNORE);
	SDL_EventState(SDL_SYSWMEVENT, SDL_IGNORE);
	SDL_EventState(SDL_USEREVENT, SDL_IGNORE);
	return 1;
}

/* ************************************************************************ */
// Initialize Video
static int init_video(const struct cfg_struct * cfg)
{
	// Go set up video.
	int bpp = atoi(cfg_get(cfg, "VIDEO_DEPTH"));
	int w = atoi(cfg_get(cfg, "VIDEO_WIDTH"));
	int h = atoi(cfg_get(cfg, "VIDEO_HEIGHT"));
	int f = atoi(cfg_get(cfg, "VIDEO_FULLSCREEN")) != 0;
	int vsync = atoi(cfg_get(cfg, "VIDEO_VSYNC")) != 0;
	int msaa = atoi(cfg_get(cfg, "VIDEO_MSAA"));

	if (SDL_InitSubSystem(SDL_INIT_VIDEO) == -1) {
		fprintf(stderr, "Failed to initialize SDL video subsystem: %s\n", SDL_GetError());
		return 0;
	}

	// OpenGL attribute list.  Defaults are commented out.
	// OpenGL attributes for 8-, 16- and 32-bit color depth
	if (bpp < 9) {
		// SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 3);	  //Use 3 bits of Red
		// SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 3);	  //Use 3 bits of Green
		// SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 2);	  //Use 2 bits of Blue
	} else if (bpp < 16) {
		// 16-bit: 555
		SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);	  //Use at least 5 bits of Red
		SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 5);	  //Use at least 5 bits of Green
		SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);	  //Use at least 5 bits of Blue
	} else if (bpp < 17) {
		// 16-bit: 565
		SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);	  //Use at least 5 bits of Red
		SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 6);	  //Use at least 6 bits of Green
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
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, (msaa > 0));
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, msaa);
	// SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
	SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, vsync);	// vsync

	// Create the SDL surface window.
	if (! SDL_SetVideoMode(w, h, bpp, SDL_OPENGL | (f ? SDL_FULLSCREEN : 0))) {
		fprintf(stderr, "Unable to set video mode: %s\n", SDL_GetError());
		return 0;
	}

	// Set window caption and turn off display cursor.
	SDL_WM_SetCaption("Euro1943", NULL);
	SDL_ShowCursor(SDL_DISABLE);
	// Set up orthographic viewport.  Lock ourselves at 800x600.
	glViewport(0, 0, w, h);
	glMatrixMode(GL_PROJECTION);
//	glLoadIdentity();
	glOrtho(-SCREEN_X / 2.0f, SCREEN_X / 2.0f, SCREEN_Y / 2.0f, -SCREEN_Y / 2.0f, -1.0f, 1.0f);
	glMatrixMode(GL_MODELVIEW);
//	glLoadIdentity();
	// Set the clear color to all black (0, 0, 0)
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	// Two ways to do transparency: simple alpha-test, and blend.
	//  Blend is used for special effects, alpha func otherwise.
	glAlphaFunc(GL_NOTEQUAL, 0.0f);
	//glEnable(GL_ALPHA_TEST);
	// GL blending is needed for more complex transparency
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//glEnable(GL_BLEND);
	//glShadeModel(GL_SMOOTH);
	//glDisable(GL_CULL_FACE);
//	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	// ///////////////////
	// print some OpenGL info
	int GLTexSize;
	printf("OpenGL information:\nVendor: %s\nRenderer: %s\nVersion: %s\nExtensions: %s\n",
		glGetString(GL_VENDOR),
		glGetString(GL_RENDERER),
		glGetString(GL_VERSION),
		glGetString(GL_EXTENSIONS));
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, & GLTexSize) ;
	printf("Max texture size: %d\n", GLTexSize);

	// Initialize SDL_Image
	// load support for the PNG image formats - required for the game to work
	//  return failure if we can't open these at all.
	if (IMG_Init(IMG_INIT_PNG) != IMG_INIT_PNG) {
		fprintf(stderr, "IMG_Init: Failed to init required png support: %s\n", IMG_GetError());
		return 0;
	}

	return 1;
}

static void init_audio(const struct cfg_struct * cfg)
{
	if (SDL_InitSubSystem(SDL_INIT_AUDIO) == -1) {
		fprintf(stderr, "Failed to initialize SDL audio subsystem: %s\n", SDL_GetError());
		env.ok_audio = env.ok_music = 0;
	} else {
		int frequency = atoi(cfg_get(cfg, "AUDIO_FREQUENCY"));
		unsigned short format = (unsigned short)atoi(cfg_get(cfg, "AUDIO_FORMAT"));
		int output_channels = atoi(cfg_get(cfg, "AUDIO_OUTPUT_CHANNELS"));
		int chunksize = atoi(cfg_get(cfg, "AUDIO_CHUNKSIZE"));
		int mixer_channels = atoi(cfg_get(cfg, "AUDIO_MIXER_CHANNELS"));

		/* Initialize SDL_Mixer */
		// Load support for MOD and XM formats.
		if (Mix_Init(MIX_INIT_MOD) != MIX_INIT_MOD) {
			fprintf(stderr, "Mix_Init: Failed to init required mod support: %s\n", Mix_GetError());
			env.ok_music = 0;
		} else
			env.ok_music = 1;

		// try getting audio at all...
		if (Mix_OpenAudio(frequency, format, output_channels, chunksize)) {
			fprintf(stderr, "Mix_OpenAudio: Unable to open audio: %s\n", Mix_GetError());
			env.ok_audio = env.ok_music = 0;
		} else {
			Mix_AllocateChannels(mixer_channels);
			// Set initial volume for all items.
			env.volume = atoi(cfg_get(cfg, "AUDIO_VOLUME"));
			Mix_Volume(-1, env.volume);
			Mix_VolumeMusic(env.volume);
			// we have WAV support at least
			env.ok_audio = 1;
			// get and print the audio format in use
			int numtimesopened;
			numtimesopened = Mix_QuerySpec(&frequency, &format, &output_channels);

			if (!numtimesopened)
				fprintf(stderr, "Mix_QuerySpec: %s\n", Mix_GetError());
			else {
				mixer_channels = Mix_AllocateChannels(-1);
				const char * format_str = "Unknown";

				switch (format) {
				case AUDIO_U8:
					format_str = "U8";
					break;

				case AUDIO_S8:
					format_str = "S8";
					break;

				case AUDIO_U16LSB:
					format_str = "U16LSB";
					break;

				case AUDIO_S16LSB:
					format_str = "S16LSB";
					break;

				case AUDIO_U16MSB:
					format_str = "U16MSB";
					break;

				case AUDIO_S16MSB:
					format_str = "S16MSB";
					break;
				}

				printf("Audio opened=%d times  frequency=%dHz  format=%s  channels=%d mixer_channels=%d\n",
					numtimesopened, frequency, format_str, output_channels, mixer_channels);
			}
		}
	}
}

static void init_network(const struct cfg_struct * cfg)
{
	// Network support is needed for multiplayer games.
	//  If for some reason that doesn't work, still allow single player.
	/* Initialize SDL_net */
	if (SDLNet_Init() < 0) {
		fprintf(stderr, "SDLNet_Init: %s\n", SDLNet_GetError());
		env.ok_network = 0;
		// Read OverServer values from cfg file
		strncpy(env.OS_LOC, cfg_get(cfg, "OVERSERVER_HOST"), 80);
		env.OS_PORT = atoi(cfg_get(cfg, "OVERSERVER_PORT"));
	} else
		env.ok_network = 1;
}

int main(int argc, char * argv[])
{
	// boilerplate startup message
	printf("Euro1943 - v%s\nGreg Kennedy 2013\n\n", VERSION);
	///////////////////////
	// .ini file configuration
	struct cfg_struct * cfg = cfg_setup("config.ini");

	////////////
	// Init core items.
	if (init_sdl()) {
		if (init_video(cfg)) {
			////////////
			// Init optional items.  If these fail, we can still play the game.
			init_audio(cfg);
			init_network(cfg);
			init_common();
			srand((unsigned int)time(NULL));
			// Defines - help to prettify the main loop.
			//#define state_func(X) isDone = do_ ## X ();
#define switch_state(X) case X: \
			gamestate = do_ ## X (); \
			break;
			// Holds current gamestate.  Change this from within a subsystem, and
			//  the current sub-state will shut down, while the next will start.
			int gamestate = gs_title;

			while (gamestate != gs_exit) {
				switch (gamestate) {
					switch_state(gs_title);
					switch_state(gs_multimenu);
					switch_state(gs_options);
					switch_state(gs_cutscene);
					switch_state(gs_win);
					switch_state(gs_lose);
					switch_state(gs_game);

				default:
					gamestate = gs_exit;
					fprintf(stderr, "Error:  Game reached unknown gamestate %d.\n", gamestate);
				}
			}

			quit_common();

			if (env.ok_network) SDLNet_Quit();

			if (env.ok_audio) Mix_CloseAudio();

			if (env.ok_music) Mix_Quit();
		}

		SDL_ShowCursor(SDL_ENABLE);
		IMG_Quit();
		SDL_Quit();
	}

	// Dump cfg-struct to disk.
	cfg_save(cfg, "config.ini");
	// All done, clean up.
	cfg_free(cfg);
	return 0;
}
