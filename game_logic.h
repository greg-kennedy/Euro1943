/* game_logic.h
	shared game logic used by either client or server */

#include "common.h"

// Start up, tick, and shutdown game engine
void init_game();
void update_game();
void shutdown_game();

// Sets a script to playback over chat
void set_script(const char *);

// Supply user controls to an ID
void control_game_regular(int, unsigned char controls[]);
void control_game_hq(int, unsigned char controls[]);

// Serialise game status, customized to a player
unsigned char *serialize_game(int);
