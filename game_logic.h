/* game_logic.h
	shared game logic used by either client or server */
#ifndef GAME_LOGIC_H_
#define GAME_LOGIC_H_

#include "common.h"

// Start up, tick, and shutdown game engine
void init_game(const char * map, int ai_players, int ai_hq);
void update_game();
void shutdown_game();

// Sets a script to playback over chat
void set_script(const char *);

// Connect or disconnect players
int connect_player();
void disconnect_player(int);

// Supply user controls to an ID
void control_game_regular(int id, int speed, int turn, int fire_primary, int fire_secondary,
	int weaponswap, int enter, float aim);
void control_game_hq(int slot, int x, int y, int type, int fire, int enter);
void control_game_deploy(int slot, int x, int y, int fire);

// Serialise game status, customized to a player
int serialize_game(int, unsigned char *);

#endif
