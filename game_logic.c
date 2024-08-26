/* game_logic.cpp
	used by client or server */

#include "game_logic.h"
#include <stddef.h>

#include <stdlib.h>
#include <stdio.h>

#define _USE_MATH_DEFINES
#include <math.h>
#ifndef M_TAU
#define M_TAU (M_PI * 2.0)
#endif
#ifndef signbit
#define signbit(x) (_copysign(1.0, x) < 0)
#endif

#include <float.h>

#include "data.h"

// /////////////////////////////////////////////////////////////////
// rolling values
//  current server frame
static unsigned long frame;
//  last assigned object ID
static unsigned short id;

// /////////////////////////////////////////////////////////////////
// Money settings
//  cash per team
static short cash[2];
//  counter until next cash update
static unsigned char cash_frame;

// /////////////////////////////////////////////////////////////////
// All map info
//  Tile map
static unsigned char tile[MAP_Y][MAP_X];

// HQ center point and occupant
static struct {
	unsigned short x, y;
	struct unit * occupant;
} hq[2];

// Capture point center and owner
static struct {
	unsigned short x, y;
	unsigned char owner;
} cp[3];

// Solid buildings (includes HQ but not cap points)
static struct {
	unsigned short x, y;
	unsigned char type;
} bldg[12];

// /////////////////////////////////////////////////////////////////
// This "player" struct links controls to an entity and status
static struct {
	// 0 = not connected
	// 1 = dead
	// 2 = controlling unit (human, vehicle)
	// 3 = in HQ
	unsigned char status;

	// controls - as read from packet
	union {
		struct {
			// dead player respawn selection
			//  a desired x/y location and a click to deploy
			unsigned char fire;
			unsigned char x, y;
		} d;

		struct {
			// human/unit controls
			//  momentary triggers
			unsigned char fire, fire_secondary,
				 weaponswap, enter;
			// acceleration and rotation change
			char accel, rotate;
			// desired aim
			float aim;
		} u;

		struct {
			// hq only controls
			//  enter/exit bldg, place item, selected item type
			unsigned char fire, enter;
			unsigned char x, y, type;
		} q;
	} controls;

	// timer to cooldown
	unsigned char respawn;

	// ptr to the unit being controlled, and seat number
	struct unit * u;
	unsigned char seat;
} players[MAX_PLAYERS];

/*
static struct
{
	// AI brain storage
} brain[MAX_PLAYERS];
*/

// /////////////////////////////////////////////////////////////////
// GAME OBJECTS

// Vehicles etc, these come in multiple parts
struct unit_sub {
	// ptr to part dictionary
	const struct object_sub * o;

	// local settings
	float angle; // 16384 step
	unsigned short ammo;
	unsigned char cooldown;

	struct unit * occupant;

	const struct unit * base;
	struct unit_sub * parent;

	struct unit_sub * sub;
	struct unit_sub * next;
};

// the base of a unit, which is invisible, takes damage, moves etc
//  vehicle models are built of a tree of "parts" (above)
static struct unit {
	// ptr to associated obj dictionary
	const struct object_base * o;

	unsigned short id;
	unsigned char team;

	// for human-type, these select a variant torso
	unsigned char equip_active, equip_avail;

	// delta values
	char speed; // -64 to +63
	char rotation;

	// position
	float prev_x, prev_y;
	float prev_angle;

	float x, y;
	float angle;

	unsigned short hp;

	struct unit_sub * sub;
	struct unit * next;
} * unit_list = NULL;

// Projectiles
static struct projectile {
	unsigned short id;
	unsigned char type, team;

	float prev_x, prev_y;

	float x, y;
	float angle;	// 2048 step
	float life;

	struct projectile * next;
} * projectile_list = NULL;

// Special effects
static struct effect {
	unsigned char type;

	float x, y;

	float delay;

	struct effect * next;
} * effect_list = NULL;

// /////////////////////////////////////////////////////////////////
// GAME OBJECT MANIP FUNCTIONS
// create a unit and add it to the list
//  this is the recursive function for sub-units in the tree
static unit_sub * create_unit_sub(const struct object_sub * o, struct unit_sub * parent, struct unit * base)
{
	struct unit_sub * c = (struct unit_sub *) malloc(sizeof(struct unit_sub));
	// set the ptr to the library object
	c->o = o;
	// initial settings
	c->angle = 0;
	c->ammo = 0;
	c->cooldown = o->cooldown;
	c->occupant = NULL;
	c->base = base;
	c->parent = parent;

	// recurse down
	if (o->sub)
		c->sub = create_unit_sub(o->sub, c, base);
	else
		c->sub = NULL;

	// recurse across
	if (o->next)
		c->next = create_unit_sub(o->next, parent, base);
	else
		c->next = NULL;

	return c;
}

static void create_unit(unsigned char type, unsigned char team, float x, float y, float angle)
{
	// search the library for an object of this type
	const struct object_base * o = object_root;

	while (o != NULL && o->type != type)
		o = o->next;

	if (o == NULL) {
		fprintf(stderr, "SERVER ERROR: Failed to find object of type %d\n", type);
		return;
	}

	struct unit * v = (struct unit *) malloc(sizeof(struct unit));

	// set the ptr to the library object
	v->o = o;
	// initial settings
	v->id = id;
	id ++;
	v->team = team;
	v->x = x;
	v->y = y;
	v->speed = 0;
	v->angle = angle; //snap(angle, 512);
	v->hp = o->hp;
	// set up sub objects
	v->sub = create_unit_sub(o->sub, NULL, v);
	v->next = unit_list;
	unit_list = v;
}

// create a projectile and add it to the list
static void create_projectile(unsigned char type, unsigned char team, float x, float y, float angle)
{
	struct projectile * j = (struct projectile *) malloc(sizeof(struct projectile));
	j->id = id;
	id ++;
	j->type = type;
	j->team = team;
	j->x = j->prev_x = x;
	j->y = j->prev_y = y;
	j->angle = angle; //snap(angle, 2048);
	j->life = projectile_detail[type].life;
	j->next = projectile_list;
	projectile_list = j;
}

// create a special effect for the list
static void create_effect(unsigned char type, float x, float y, float delay)
{
	effect * e = (struct effect *)malloc(sizeof(struct effect));
	e->x = x;
	e->y = y;
	e->type = type;
	e->delay = delay;
	e->next = effect_list;
	effect_list = e;
}

// /////////////////////////////////////////////////////////////////
// PHYSICS
static float min(float x, float y)
{
	return x < y ? x : y;
}

static float max(float x, float y)
{
	return x > y ? x : y;
}

// "fast" float square
static float sqr(float x)
{
	return x * x;
}

// normalize an angle to [0, 2*PI)
static float norm(float x)
{
	while (x >= M_TAU) x -= M_TAU;

	while (x < 0) x += M_TAU;

	return x;
}

// expand a step fraction into an angle (inverse of above)
//static float unsnap(int step, unsigned int steps) {
//	return (2 * M_PI) * step / steps ;
//}

// Point to AABB collision
//  Returns distance from point to aabb
//  so, 0 if point in AABB
static int point_aabb(float x, float y,
	float ox, float oy, float ow2, float oh2)
{
	// translate point to center space
	// all quadrants are mirror-symmetrical, so it is OK to do everything in positive-quadrant space
	float dx = fabs(x - ox) - ow2;
	float dy = fabs(y - oy) - oh2;

	if (dx <= 0) {
		if (dy <= 0) {
			// point within bounds on both sides
			return 0;
		} else {
			// point is within horizontal range so return distance from vertical bounds
			return dy;
		}
	} else if (dy <= 0) {
		// point is within vertical range so return distance from horizontal bounds
		return dx;
	} else {
		// point is nearest to the corner
		return sqrt(sqr(dx) + sqr(dy));
	}
}

// Point to OBB collision
//  Returns distance from point to rotated rect
static int point_obb(float x, float y,
	float ox, float oy, float ow2, float oh2, float a)
{
	// rotate the point about the center
	float tx = x - ox;
	float ty = y - oy;
	float rx = tx * cos(-a) - ty * sin(-a) + ox;
	float ry = tx * sin(-a) + ty * cos(-a) + oy;
	return point_aabb(rx, ry, ox, oy, ow2, oh2);
}

// Ray to AABB collision
//  Return a distance at which the collision happens (or FLT_MAX if never)
static float ray_aabb(float x, float y, float t, float a,
	float ox, float oy, short ow2, short oh2)
{
	// translate the x/y to center space
	float tx = x - ox;
	float ty = y - oy;
	float dx = t * cos(a);
	float dy = t * sin(a);

	// slab test: https://noonat.github.io/intersect/

	// swap rectangle direction based on the incoming angle
	if (signbit(dx)) ow2 = -ow2;

	if (signbit(dy)) oh2 = -oh2;

	//
	const float near_x = (-ow2 - tx) / dx;
	const float far_y = (oh2 - ty) / dy;

	if (near_x > far_y) return FLT_MAX;

	const float near_y = (-oh2 - ty) / dy;
	const float far_x = (ow2 - tx) / dx;

	if (near_y > far_x) return FLT_MAX;

	// determine near and far within 0 and 1
	const float near = max(near_x, near_y);

	if (near >= 1) return FLT_MAX;

	const float far = min(far_x, far_y);

	if (far <= 0) return FLT_MAX;

	// clamp hit
	if (near <= 0) return 0;

	return near;
}

static float ray_obb(float x, float y, float t, float a,
	float ox, float oy, short ow2, short oh2, float oa)
{
	// rotate the ray about the center
	float tx = x - ox;
	float ty = y - oy;
	float rx = tx * cos(-oa) - ty * sin(-oa) + ox;
	float ry = tx * sin(-oa) + ty * cos(-oa) + oy;
	return ray_aabb(rx, ry, t, a - oa, ox, oy, ow2, oh2);
}

// Check an OBB against an AABB
//  Returns the distance from -h2 to h2 at which intersection occurred
//  or FLT_MAX if no collision.
static float obb_aabb(float x, float y, short w2, short h2, float a,
	float ox, float oy, short ow2, short oh2)
{
	// translate the x/y to center space
	float tx = x - ox;
	float ty = y - oy;
	// first check the AABB projected onto the OBB
	//  this means determining extents in X and Y direction based on rotation (a)
	// "extent" being the actual, rotated distance from ox
	float extent_x = fabs(w2 * cos(a)) + fabs(h2 * sin(a));

	// overlap X
	if (extent_x < tx - ow2 || - extent_x > tx + ow2) return FLT_MAX;

	float extent_y = fabs(w2 * sin(a)) + fabs(h2 * cos(a));

	// overlap Y
	if (extent_y < ty - oh2 || - extent_y > ty + oh2) return FLT_MAX;

	// NOW, swap who is who
	//  this means counter-rotating x/y to be axis-aligned instead
	tx = ox - x;
	ty = oy - y;
	float rx = tx * cos(-a) - ty * sin(-a);
	float ry = tx * sin(-a) + ty * cos(-a);
	extent_x = fabs(ow2 * cos(-a)) + fabs(oh2 * sin(-a));

	// overlap X
	if (extent_x < rx - w2 || - extent_x > rx + w2) return FLT_MAX;

	extent_y = fabs(ow2 * sin(-a)) + fabs(oh2 * cos(-a));

	// overlap Y
	if (extent_y < ry - h2 || - extent_y > ry + h2) return FLT_MAX;

	return 0;
}

static float obb_obb(float x, float y, short w2, short h2, float a,
	float ox, float oy, short ow2, short oh2, float oa)
{
	// rotate the second obb about the first so it becomes axis-aligned
	float tx = x - ox;
	float ty = y - oy;
	float rx = tx * cos(-oa) - ty * sin(-oa) + ox;
	float ry = tx * sin(-oa) + ty * cos(-oa) + oy;
	return obb_aabb(rx, ry, w2, h2, a - oa, ox, oy, ow2, oh2);
}

//
// /////////////////////////////////////////////////////////////////
// GAME UPDATE

// helper functions - check for seat in sub objects
static const struct unit_sub * check_seat_sub(const struct unit_sub * v)
{
	do {
		if (v->o->seat_id > -1 && v->occupant == NULL) {
			// this seat was empty, return it
			return v;
		}

		if (v->sub) {
			// check the sub tree
			const struct unit_sub * c = check_seat_sub(v->sub);

			// found something there, bubble it up
			if (c != NULL) return c;
		}

		v = v->next;
	} while (v != NULL);

	return NULL;
}

// helper function - find numbered seat in subobjects
static struct unit_sub * find_mouse_sub(struct unit_sub * v, const char seat)
{
	do {
		if (v->o->controller == seat) {
			// found, return it
			return v;
		}

		if (v->sub) {
			// check the sub tree
			struct unit_sub * c = find_mouse_sub(v->sub, seat);

			// found something there, bubble it up
			if (c != NULL) return c;
		}

		v = v->next;
	} while (v != NULL);

	return NULL;
}

// helper function - find numbered seat in subobjects
static const struct unit_sub * find_sub(const struct unit_sub * v, const char seat)
{
	do {
		if (v->o->seat_id == seat) {
			// found, return it
			return v;
		}

		if (v->sub) {
			// check the sub tree
			const struct unit_sub * c = find_sub(v->sub, seat);

			// found something there, bubble it up
			if (c != NULL) return c;
		}

		v = v->next;
	} while (v != NULL);

	return NULL;
}

// helper function - recursive update of unit sub-parts
static void update_unit_sub(struct unit_sub * v)
{
	do {
		if (v->cooldown > 0) v->cooldown --;

		// check the sub tree
		if (v->sub)
			update_unit_sub(v->sub);

		v = v->next;
	} while (v != NULL);
}


// Performs all game simulation.
void update_game()
{
	// Increment global frame number.
	frame ++;
	// Clear the Effect List.
	struct effect * e = effect_list;

	while (e != NULL) {
		effect_list = e->next;
		free(e);
		e = effect_list;
	}

	// Money handling.  Cash frame increments each tick.  Every five frames, money is incremented.
	cash_frame++;

	while (cash_frame >= CASH_FREQ) {
		cash[0] += CASH_UPKEEP;  //upkeep
		cash[1] += CASH_UPKEEP;

		// money earned for each capture point owned
		for (int i = 0; i < 3; i++) {
			if (cp[i].owner == 1) cash[0] += CASH_POINT;
			else if (cp[i].owner == 2) cash[1] += CASH_POINT;
		}

		cash_frame -= CASH_FREQ;
	}

	// Check for game over here.
	if (cash[0] <= 0 || cash[1] <= 0) {
		// restore everyone to Dead status
		//  todo: interlude status idk
		for (int i = 0; i < MAX_PLAYERS; i++) {
			if (players[i].status)
				players[i].status = 1;
		}

		// TODO: all game over cleanup!!
	} else {
		// apply all new player input
		for (int i = 0; i < MAX_PLAYERS; i++) {
			unsigned char team = i % 2;

			if (players[i].status == 1) {
				// Player is waiting for respawn.
				//  Count down their respawn timer.
				if (players[i].respawn > 0)
					players[i].respawn --;

				else {
					if (players[i].controls.d.fire && cash[team] > CASH_RESPAWN) {
						// asking for a respawn
						// Create a brand new Human
						float x = players[i].controls.d.x * 32 + 16;
						float y = players[i].controls.d.y * 32 + 16;
						create_unit(0, team, x, y, atan2f(hq[! team].y - y, hq[! team].x - x));
//						create_unit(0, team, x, y, 0);
						// put the player into the driver's seat of their body
						players[i].u = unit_list;
						const unit_sub * s = check_seat_sub(unit_list->sub);
						players[i].seat = s->o->seat_id;
						//						players[i].u = unit_list->sub;
						//				players[i].id = h->id;
						// set other values
						/*
						h->weapon_active = 1;	// pistol

						// TODO
						h->weapon_carried = 2;
						h->ammo = 20;
						*/
						// Pay the respawn fee
						cash[team] -= CASH_RESPAWN;
						// ready to rock
						players[i].status = 2;
					}
				}
			} else if (players[i].status == 2) {
				// Player is active and controlling some unit
				// keyboard movement controlled by this player
				struct unit * base = players[i].u;

				if (base->o->controller == players[i].seat) {
					base->speed += base->o->accel * players[i].controls.u.accel;

					if (base->speed > base->o->speed)
						base->speed = base->o->speed;
					else if (base->speed < -base->o->speed)
						base->speed = -base->o->speed;

					base->angle = norm(base->angle + base->o->rotation * players[i].controls.u.rotate);
				}

				// locate the sub-unit under this mouse control
				struct unit_sub * sub = find_mouse_sub(base->sub, players[i].seat);

				if (sub) {
					// calculate absolute position and angles by summing up all parts
					//  between the base, and here
					float angle = base->angle;
					float x = base->x;
					float y = base->y;
					struct unit_sub * parent = sub;

					do {
						angle += parent->angle;
						// x
						// y
						parent = parent->parent;
					} while (parent != NULL);

					// change angle by allowable rotation
					float theta = norm(players[i].controls.u.aim - angle);

					//printf("Aim = %f, theta = %f\n", players[i].controls.u.aim, angle);
					// get the range back to -pi to +pi
					if (theta < -M_PI) theta += M_TAU;
					else if (theta > M_PI) theta -= M_TAU;

					//printf(" (norm => %f)\n", theta);

					// lock maximum angle change to part rotation
					if (theta < 0) {
						if (theta < - sub->o->rotation) theta = - -sub->o->rotation;
					} else if (theta > 0) {
						if (theta > sub->o->rotation) theta = sub->o->rotation;
					}

					//printf(" Calc. rotation = %f\n", theta);
					sub->angle = norm(sub->angle + theta);
					//printf("New angle %f\n", sub->angle);

					//  convert player aim into a bearing and make sure it's within their foreward 180 degrees

					// shoot the gun
					if (sub->o->projectile >= 0 &&
						players[i].controls.u.fire &&
						sub->cooldown == 0) {
						create_projectile(sub->o->projectile, team, x, y, angle);
						sub->cooldown += sub->o->cooldown;
					}
				}

				// weapon swap
				/*
				if (players[i].controls.u.weaponswap && players[i].u->weapon_carried)
				{
					if (players[i].u->weapon_carried == players[i].u->weapon_active)
						players[i].u->weapon_active = 1;
					else
						players[i].u->weapon_active = players[i].u->weapon_carried;
				}
				*/
				// enter / exit unit attempt
				/*
				if (players[i].controls.u.enter) {
					// check for HQ entry
					if (hq[i % 2].occupant == NULL) {
						// building is available...
						if (circle_aabb(&(players[i].u->x), &(players[i].u->y), 64,
							bldg[i % 2].x0, bldg[i % 2].y0, bldg[i % 2].x1, bldg[i % 2].y1))
						{
							players[i].status = 4;
							hq[i % 2].occupant = players[i].u;

							// splice the human out of the unit_list
							if (players[i].u == unit_list) {
								unit_list = players[i].u->next;
							} else {
								unit * h = unit_list;
								while (players[i].u != h->next)
								{
									h = h->next;
								}
								h->next = players[i].u->next;
							}
							players[i].u->next = NULL;

							continue;
						}
					}

					// check for unit entry
					struct unit * v = unit_list;
					while (v != NULL) {
						if (v->team == i % 2) {
							int seat_id = -1;
							if (v->o->seat_id > -1 && v->occupant == NULL) {
								// found a seat
								seat_id = v->o->seat_id;

								// stash human in there
								v->occupant = players[i].u;
							} else if (v->sub) {
								// check sub for available seat
								struct unit_sub * c = check_seat_sub(v->sub);
								if (c != NULL) {
									// found a seat
									seat_id = c->o->seat_id;

									// stash human in there
									c->occupant = players[i].u;
								}
							}

							if (seat_id > -1) {
								// ok record our occupancy
				//								players[i].status = 2;

								players[i].u = v;
				//								players[i].seat = seat_id;

								// splice the human out of the unit_list
								if (players[i].u == unit_list) {
									unit_list = players[i].u->next;
								} else {
									unit * h = unit_list;
									while (players[i].u != h->next)
									{
										h = h->next;
									}
									h->next = players[i].u->next;
								}
								players[i].u->next = NULL;

								// reset all the other controls
								players[i].controls.u.accel =
									players[i].controls.u.rotate =
									players[i].controls.u.weaponswap =
									players[i].controls.u.fire =
									players[i].controls.u.fire_secondary =
									players[i].controls.u.enter =
									players[i].controls.u.aim = 0;

								break;
							}
						}
						v = v->next;
					}
				}
				*/
				/*			} else if (players[i].status == 3) {
								// Player inside a unit somewhere
								struct unit * v = players[i].v;
								if (players[i].seat == 0)
								{
									// keyboard movement controlled by this player
									v->speed += v->o->accel * players[i].controls.u.accel;

									if (v->speed > v->o->speed)
										v->speed = v->o->speed;
									else if (v->speed < -v->o->accel)
										v->speed = -v->o->accel;

									v->angle += v->o->rotation * players[i].controls.u.rotate;
								}

								// exit unit
								if (players[i].controls.u.enter) {
									// search unit for correct seat ID
									if (v->o->seat_id == players[i].seat)
									{
										v->occupant = NULL;
										players[i].u->next = unit_list;
										unit_list = players[i].u;

										players[i].status = 2;
										players[i].seat = -1;

										players[i].u->x = v->x;
										players[i].u->y = v->y;
									} else {
										// sub
									}
								}
								*/
			} else if (players[i].status == 3) {
				// Player is in the HQ
				// exit HQ
				/*
				if (players[i].controls.q.enter) {
					hq[i % 2].occupant = NULL;

					players[i].u->next = unit_list;
					unit_list = players[i].u;

					players[i].status = 2;
				}
				*/
			}
		}

		/*
				if (players[i].controls[5])
				{
					if (humptr->weapon == 5) // sniper rifle zoom!
					{
						humptr->speed=0;
						players[i].zoomlevel=100;
					}
					if (humptr->cooldown == 0)
					{
						if (humptr->weapon == 7 && humptr->ammo>0)
						{
							humptr->ammo--;
							humptr->cooldown=8;
							createbullet(humptr->align,humptr->x,humptr->y,humptr->torsoangle + humptr->angle, 0, 4, 255);
						}
						else if (humptr->weapon == 8 && humptr->ammo>0)
						{
							humptr->ammo--;
							humptr->cooldown=5;
							createbullet(humptr->align,humptr->x,humptr->y,humptr->torsoangle + humptr->angle, 10, 7, 10);
						}
					}

				}
				if (players[i].controls[6])	// unit entrance!
				{
					//		   printf("Player tried to enter unit.\n");
					if (hqock[humptr->align] == 0 &&
							sqrt(pow(humptr->x - (bldloc[3 + humptr->align][1] * 32),2) +
								pow(humptr->y - (bldloc[3 + humptr->align][2] * 32),2)) < 128)
					{
						hqock[humptr->align] = humptr->id; // save my ID
						players[i].status = 3;
						driverhuman(humptr->id); // store my body.
						//												  printf("%d entered HQ!\n",i);
					} else {
						vehid = enterveh(humptr);
						if (vehid)
						{
							players[i].status = 2;
							players[i].vehid = vehid;
							players[i].controller_k = checkcontrol(vehid,'K');
							players[i].controller_m = checkcontrol(vehid,'M');
						}
					}
					players[i].controls[0]=0;
					players[i].controls[6]=0;
				}
				if (players[i].controls[7])
				{
					players[i].controls[7]=0;
					if (humptr->weapon!=3)
						humptr->weapon=3;
					else
						humptr->weapon=humptr->carriedarms;
				}
				humptr->torsoangle=players[i].controls[8]-humptr->angle;
				if (humptr->torsoangle > 127 && humptr->torsoangle < 192)
					humptr->torsoangle=192;
				else if (humptr->torsoangle > 63 && humptr->torsoangle < 128)
					humptr->torsoangle=63;
				//		   humptr->anglediff=players[i].controls[9]-humptr->angle;
			}
		} else {
			printf("Got a player %d with no body: setting status to DEAD\n",i);
			players[i].status=0;
		}
		} else if (players[i].status==2) {
		if (players[i].controls[6])	// unit exit!
		{
			objptr = locate_object(topobject, players[i].vehid);
			players[i].status=1;
			undriverhuman(objptr->occupied);
			players[i].controller_m = objptr->occupied;
			players[i].controller_k = objptr->occupied;
			humptr = locate_human(tophuman,players[i].controller_k);
			objptr->occupied=0;
			while (objptr->parent != NULL) objptr=objptr->parent;
			humptr->x = objptr->x - (int)(((double)objptr->width/2 + 32) * costable[objptr->angle]);
			humptr->y = objptr->y - (int)(((double)objptr->width/2 + 32) * sintable[objptr->angle]);
			players[i].controls[6] = 0;
			//			  printf("There is NO ESCAPE.\n");
		} else {
			objptr = locate_object(topobject, players[i].controller_k);
			if (objptr != NULL) {
				if (players[i].controls[0]) objptr->anglediff = -objptr->maxang;
				if (players[i].controls[1]) objptr->anglediff = objptr->maxang;
				if (players[i].controls[2]) objptr->speed+=objptr->accel;
				else if (players[i].controls[3]) objptr->speed-=objptr->accel;
				if (objptr->speed > objptr->topspeed) objptr->speed = objptr->topspeed;
				if (objptr->speed < -objptr->topspeed) objptr->speed = -objptr->topspeed;

				players[i].controls[6]=0;
				players[i].controls[7]=0;
			}
			// let's do mouse control now.
			objptr = locate_object(topobject, players[i].controller_m);

			if (objptr != NULL) {
				tangle = 0;
				drill = objptr;
				tx2 = 0;
				ty2 = 0;

				while (drill!=NULL)
				{
					tangle2 = atan2(ty2,tx2) + ((double)(drill->angle + drill->anglediff) * 0.02463994235294);
					dist = sqrt(tx2*tx2+ty2*ty2);

					tx = dist * cos(tangle2);
					ty = dist * sin(tangle2);
					if (drill->parent != NULL)
					{
						tx += drill->parent->xoff;
						ty += drill->parent->yoff;
					}
					tx2 = (drill->zoom)*tx + (float)drill->x;
					ty2 = (drill->zoom)*ty + (float)drill->y;

					tangle+=(drill->angle + drill->anglediff);
					drill=drill->parent;
				}
				oldx = (int)tx2;
				oldy = (int)ty2;
				objptr->anglediff=players[i].controls[8]-tangle;
				if (objptr->anglediff > objptr->maxang) objptr->anglediff = objptr->maxang;
				if (objptr->anglediff < -objptr->maxang) objptr->anglediff = -objptr->maxang;

				if (players[i].controls[4]) // oh my, here come bullets.
				{
					if (objptr->coolleft == 0)
					{
						objptr->coolleft = objptr->cool;
						if (objptr->proj == 4)
							createbullet(objptr->align,oldx,oldy,(256 + tangle + objptr->anglediff) % 256, 0, objptr->proj, 255);
						else if (objptr->proj == 14)
							createbullet(objptr->align,oldx,oldy,(256 + tangle + objptr->anglediff) % 256, 100, objptr->proj, 6);
						else if (objptr->proj == 16)
							createbullet(objptr->align,oldx,oldy,(256 + tangle + objptr->anglediff) % 256, 0, objptr->proj, 4);
						else if (objptr->proj == 17)
							createbullet(objptr->align,oldx,oldy,(256 + tangle + objptr->anglediff) % 256, 100, 13, 6);
						else
							createbullet(objptr->align,oldx,oldy,(256 + tangle + objptr->anglediff) % 256, 100, objptr->proj, 5);
					}
				}
			}
		}
		} else if (players[i].status == 3) {
		if (players[i].controls[4]) {
			if (cash [i % 2] > prices[players[i].controls[0]] && players[i].controls[0] < 17) {

				if (players[i].controls[0] < 7)
				{
					createbullet(i % 2,(int)players[i].controls[5]*32,(int)players[i].controls[8]*32,0,0,players[i].controls[0] + 6,255);
				} else if (players[i].controls[0] < 17) {
					createobject(hqobjects[players[i].controls[0] - 7],(int)players[i].controls[5]*32,(int)players[i].controls[8]*32, i%2);
				}
				cash [i % 2] -= prices[players[i].controls[0]];
			}
			players[i].controls[4]=0;
		}
		if (players[i].controls[6]) { // leaving HQ
			players[i].status=1;
			undriverhuman(hqock[i % 2]);
			players[i].controller_m = hqock[i % 2];
			players[i].controller_k = hqock[i % 2];
			hqock[i % 2] = 0;
			players[i].controls[8] = 0;
			players[i].controls[6] = 0;
			players[i].controls[5] = 0;
			players[i].controls[4] = 0;
			players[i].controls[0]=0;
		}
		}
		}
		}*/
		// Now iterate through each object, and update its settings.
		// Units
		struct unit * u = unit_list;

		while (u != NULL) {
			// copy x/y/angle to prev. values
			u->prev_x = u->x;
			u->prev_y = u->y;
			u->prev_angle = u->angle;
			// reduce weapon cooldown, if any
			update_unit_sub(u->sub);

			if (u->speed) {
				// possible new movement for object
				u->x += u->speed * cos(u->angle);
				u->y += u->speed * sin(u->angle);

				// boundaries
				//  todo: use actual bounding box
				if (u->x < 16) u->x = 16;
				else if (u->x > 3184) u->x = 3184;

				if (u->y < 16) u->y = 16;
				else if (u->y > 3184) u->y = 3184;

				// check unit vs building collision
				//  if colliding, push them out of the building (closest edge)
				// damage dealt is based on distance pushed
				for (int i = 0; i < 12; i ++) {
					if (0 ==
						obb_aabb(u->x, u->y, u->o->width / 2, u->o->height / 2, u->angle,
						bldg[i].x, bldg[i].y, building_size[bldg[i].type].w / 2, building_size[bldg[i].type].h / 2))
						create_effect(1, u->x, u->y, 0);
				}

				// check unit vs unit collision
				const struct unit * o = unit_list;

				while (o != NULL) {
					if (o != u) {
						if (0 ==
							obb_obb(u->x, u->y, u->o->width / 2, u->o->height / 2, u->angle,
							o->x, o->y, o->o->width / 2, o->o->height / 2, o->angle))
							create_effect(1, u->x, u->y, 0);
					}

					o = o->next;
				}
			}

			u = u->next;
		}

		// Projectiles
		struct projectile * j = projectile_list, * j_prev = NULL;

		while (j != NULL) {
			// reduce life, destroy self if 0
			if (j->life < 1) {
				/*
				if (projectile_detail[j->type].flags & EXPLOSIVE) {
					// move projectile to its dest
					j->x += projectile_detail[j->type].speed * j->life * cos(j->angle);
					j->y += projectile_detail[j->type].speed * j->life * sin(j->angle);

					// explode rocket
					create_effect(1, j->x, j->y, j->life);
				}
				*/
				if (j == projectile_list) {
					projectile_list = j->next;
					free(j);
					j = projectile_list;
				} else {
					j_prev->next = j->next;
					free(j);
					j = j_prev->next;
				}
			} else {
				j->life --;

				if (projectile_detail[j->type].speed > 0) {
					// possible new movement for object
					j->prev_x = j->x;
					j->prev_y = j->y;
					j->x += projectile_detail[j->type].speed * cos(j->angle);

					if (j->x < 0 || j->x >= 3200) {
						// todo delete it
						j->life = 1;
						j->x = 4000;
					} else {
						j->y += projectile_detail[j->type].speed * sin(j->angle);

						// boundaries
						if (j->y < 0 || j->y >= 3200) {
							// flew out of bounds
							j->life = 1;
							j->y = 4000;
						} else {
							// check vs building collision
							float t = FLT_MAX;
							int hit_bldg = -1;

							for (int i = 0; i < 12; i ++) {
								//							printf("Testing ray_aabb vs bldg %d\n", i);
								float new_t = ray_aabb(j->prev_x, j->prev_y, projectile_detail[j->type].speed, j->angle,
									bldg[i].x, bldg[i].y, building_size[bldg[i].type].w / 2, building_size[bldg[i].type].h / 2);

								if (new_t >= 0 && new_t < t) {
									//								printf("Hit at t = %f\n", new_t);
									t = new_t;
									hit_bldg = 1;
									//							} else {
									//								printf("No hit, t remains %f\n", t);
								}
							}

							// check vs unit collision
							struct unit * o = unit_list, * hit_unit = NULL;

							while (o != NULL) {
								if (j->team != o->team) {
									float new_t = ray_obb(j->prev_x, j->prev_y, projectile_detail[j->type].speed, j->angle,
										o->x, o->y, o->o->width / 2, o->o->height / 2, o->angle);

									if (new_t >= 0 && new_t < t) {
										//								printf("Hit at t = %f\n", new_t);
										t = new_t;
										hit_bldg = -1;
										hit_unit = o;
										//							} else {
										//								printf("No hit, t remains %f\n", t);
									}
								}

								o = o->next;
							}

							if (t < 1) {
								j->life = t;
								create_effect(1, j->prev_x + t * (j->x - j->prev_x),
									j->prev_y + t * (j->y - j->prev_y), t);
							}
						}
					}
				} else {
					// stationary "projectile", these don't move but we do need to point-test them vs buildings and objects
					int hit_bldg = -1;

					for (int i = 0; i < 12; i ++) {
//							printf("Testing ray_aabb vs bldg %d\n", i);
						if (point_aabb(j->x, j->y, bldg[i].x, bldg[i].y, building_size[bldg[i].type].w / 2, building_size[bldg[i].type].h / 2) == 0) {
//								printf("Hit at t = %f\n", new_t);
							hit_bldg = 1;
//							} else {
//								printf("No hit, t remains %f\n", t);
						}
					}

					// check vs unit collision
					struct unit * o = unit_list, * hit_unit = NULL;

					while (o != NULL) {
						if (j->team != o->team) {
							if (point_obb(j->x, j->y,
								o->x, o->y, o->o->width / 2, o->o->height / 2, o->angle) == 0) {
								hit_bldg = -1;
								hit_unit = o;
							}
						}

						o = o->next;
					}

					if (hit_bldg >= 0 || hit_unit != NULL) {
						j->life = 0;
						create_effect(1, j->x, j->y, 0);
					}
				}

				j_prev = j;
				j = j->next;
			}
		}

		// check for capture points changing hands
		//  to take a CP you must be the only team of humans within 128px
		for (int i = 0; i < 3; i ++) {
			unsigned char new_owner = 0;
			struct unit * h = unit_list;

			while (h != NULL) {
				unsigned char team = 1 + h->team;

				if ((h->o->flags && HUMAN == HUMAN) && (new_owner == 0 || new_owner != team)) {
					// check for proximity only if another teammate isn't already there
//					printf("Player is at %f, %f and CP %d at %f, %f.  Distance: %f", h->x, h->y, i, cp[i].x, cp[i].y, sqr(h->x - cp[i].x) + sqr(h->y - cp[i].y));
					if (sqr(h->x - cp[i].x) + sqr(h->y - cp[i].y) < sqr(64)) {
						new_owner += team;

						// seems contested, nobody can have it.
						if (new_owner > 2) goto no_owner;
					}
				}

				h = h->next;
			}

			if (new_owner) cp[i].owner = new_owner;

no_owner:
			;
		}
	}
}

// Server-side map loading function.
static char load_map(const char * name)
{
	int i, j, c;
	FILE * fp = fopen(name, "rb");

	if (fp == NULL) {
		fprintf(stderr, "Error opening map %s!\n", name);
		return 0;
	}

	for (j = 0; j < MAP_Y; j++) {
		for (i = 0; i < MAP_X; i += 2) {
			// Read next (binary) character from file.
			unsigned char incoming = fgetc(fp);
			// Maps are 100x100 with up to 16 choices for tile.
			tile[i][j] = (incoming & 0xF0) >> 4;
			tile[i + 1][j] = (incoming & 0x0F);
		}
	}

	// all buildings
	c = 0;
	j = 0;

	for (i = 0; i < 15; i ++) {
		unsigned char type = fgetc(fp);
		unsigned char x = fgetc(fp);
		unsigned char y = fgetc(fp);

//		printf("bldg %d: %d, %d, %d\n", i, type, x, y);

		if (type == 0 || type == 8 || type == 9) {
			if (c < 3) {
				// cap points
				if (type == 8) cp[c].owner = 1;
				else if (type == 9) cp[c].owner = 2;
				else cp[c].owner = 0;

				cp[c].x = 16 + x * 32;
				cp[c].y = 16 + y * 32;
				c ++;
			}
		} else {
			if (type == 1 || type == 2) {
				// hq
				hq[type - 1].occupant = NULL;
				hq[type - 1].x = 16 + x * 32;
				hq[type - 1].y = 16 + y * 32;
			}

			// solid building
			bldg[j].x = 16 + x * 32;
			bldg[j].y = 16 + y * 32;
			bldg[j].type = type;
			j ++;
		}
	}

	// initial spawn angles
//	initang[0] = (unsigned char)fgetc(fp);
//	initang[1] = (unsigned char)fgetc(fp);
	fclose(fp);
	return 1;
}

void init_game(const char * map, int ai_players, int ai_hq)
{
	// Reset everything from scratch.
	frame = 0;
	id = 0;
	cash[0] = cash[1] = CASH_START;
	cp[0].owner = cp[1].owner = cp[2].owner = 0;
	unit_list = NULL;
	projectile_list = NULL;
	effect_list = NULL;
	load_map(map);

	// add bot controllers for AI at the bottom of the player list
	for (int i = MAX_PLAYERS - ai_players; i < MAX_PLAYERS; i ++)
		players[i].status = 1;

	// add AI HQ players
	/*
	for (int i = ai_players; i < MAX_PLAYERS; i ++) {
		players[i].status = 1;
	}
	*/
	// stick a jeep in there lol
//	unit_list =
//	create_unit(1, 0, hq[0].x + 150, hq[0].y - 50, 1);
//	unit_list->sub->occupant = (struct human *)1;
//	unit_list->sub->next->occupant = (struct human *)1;
}

void shutdown_game()
{
	// destroy everything!
	for (int i = 0; i < MAX_PLAYERS; i ++)
		players[i].status = 0;
}

// Connect a player to the game
//  Returns -1 if failure (game full)
int connect_player()
{
	for (int i = 0; i < MAX_PLAYERS; i ++) {
		if (players[i].status == 0) {
			players[i].status = 1;
			players[i].respawn = 1000 / UPDATE_FREQ;
			return i;
		}
	}

	return -1;
}

// Drop player from the game
void disconnect_player(int slot)
{
	/* todo: kill human or unman unit etc
	if (players[id].status != 0)
	{
	} */
	players[slot].status = 0;
}


// Converts / stores a controls buffer into player struct
//
void control_game_regular(int slot, int speed, int turn, int fire, int fire_secondary,
	int weaponswap, int enter, float aim)
{
	// only accept player control if they are in Entity Mode
	if (players[slot].status == 2) {
		players[slot].controls.u.accel = speed;
		players[slot].controls.u.rotate = turn;
		players[slot].controls.u.fire = fire;
		players[slot].controls.u.fire_secondary = fire_secondary;
		players[slot].controls.u.weaponswap = weaponswap;
		players[slot].controls.u.enter = enter;
		players[slot].controls.u.aim = aim;
	}
}

void control_game_hq(int slot, int x, int y, int type, int fire, int enter)
{
	// accept HQ controls in HQ mode
	if (players[slot].status == 3) {
		players[slot].controls.q.x = x;
		players[slot].controls.q.y = y;
		players[slot].controls.q.type = type;
		players[slot].controls.q.fire = fire;
		players[slot].controls.q.enter = enter;
	}
}

void control_game_deploy(int slot, int x, int y, int fire)
{
	// accept Deploy controls in d mode
	if (players[slot].status == 1) {
		players[slot].controls.d.x = x;
		players[slot].controls.d.y = y;
		players[slot].controls.d.fire = fire;
	}
}

// snap floating point angle to one of N segments
static unsigned int snap(float theta, unsigned int steps)
{
	// normalize angle - is this needed?  does unsigned + modulus handle it?
	if (theta < 0) theta += M_TAU;

	// convert to steps, round and modulus
	return (unsigned int)(0.5 + (theta / M_TAU) * steps) % steps;
}

static void w_int32(unsigned char * p, const unsigned int i)
{
	p[0] = (i >> 24) & 0xFF;
	p[1] = (i >> 16) & 0xFF;
	p[2] = (i >> 8) & 0xFF;
	p[3] = i & 0xFF;
}

static void w_int24(unsigned char * p, const unsigned int i)
{
	p[0] = (i >> 16) & 0xFF;
	p[1] = (i >> 8) & 0xFF;
	p[2] = i & 0xFF;
}

static void w_int16(unsigned char * p, const unsigned short w)
{
	p[0] = (w >> 8) & 0xFF;
	p[1] = w & 0xFF;
}

// sub-objects
static unsigned char * serialize_sub(unsigned char * p, const struct unit_sub * v)
{
	do {
//		printf("angle = %f (ser. %x)\n", v->angle, snap(v->angle, 0x8000));
		w_int16(p, (v->occupant != NULL ? 0x8000 : 0) |
			snap(v->angle, 0x8000));
		p += 2;

		if (v->sub)
			p = serialize_sub(p, v->sub);

		v = v->next;
	} while (v != NULL);

	return p;
}

int serialize_game(int slot, unsigned char * payload)
{
	unsigned char * p = payload;
//	printf("Serializing for player %u\n", slot);
	// Global info
	// capture points owners
	*p = 0;

	for (int i = 0; i < 3; i ++)
		*p |= (cp[i].owner << (i * 2));

	p ++;

	// Cash
	for (int i = 0; i < 2; i ++) {
		w_int16(p, cash[i]);
		p += 2;
	}

	// Player Status (alive, dead, in HQ, etc)
	*p = players[slot].status;
	p++;

	if (players[slot].status == 1) {
		// send respawn timeout
		*p = players[slot].respawn;
		p ++;
	} else if (players[slot].status == 2) {
		// Player ID and other local info
		//	printf("Writing ID as %u, status=%u\n", players[slot].id, players[slot].status);
		w_int16(p, players[slot].u->id);
		p += 2;
		// seat
		*p = players[slot].seat;
		p ++;
		// health
		*p = players[slot].u->hp;
		p ++;
		// ammo
		*p = players[slot].u->hp;
		p ++;
	} else if (players[slot].status == 3) {
		/*
				w_int16(p, 0); p += 2;
				// health
				*p = 0; p ++;
				// ammo
				*p = 0; p ++;
		*/
	}

	// Unit updates.
	//  Since we don't know how many objects we're reporting about here (depends on vision),
	//  bookmark this point
	unsigned char * o = p;
	p ++;
	*o = 0;
	const struct unit * u = unit_list;

	while (u != NULL) {
		// TODO: determine what's near player
		//  for now just stuff everything in the packet
		// increment the object count
		(*o) ++;
		// unit id
		w_int16(p, u->id);
		p += 2;
		// team and type
		*p = (u->team << 7) | u->o->type;
		p ++;
		// object x, y, angle, and speed
		// x and y fit into 12 bits each so they only take 3 bytes
		w_int24(p, (((unsigned int)u->prev_x << 12) | ((unsigned int)u->prev_y) & 0xFFF));
		p += 3;
		// speed max is +-64 (7 bits)
		// angle in 512 steps (9 bits)
		w_int16(p, (u->speed << 9) |
			snap(u->angle, 512));
		p += 2;
		// and now: the sub-tree
		p = serialize_sub(p, u->sub);
		u = u->next;
	}

	// Projectile updates.
	//  Same deal as before: one byte to hold projectile count
	o = p;
	p ++;
	*o = 0;
	struct projectile * j = projectile_list;

	while (j != NULL) {
		// TODO: determine what's near player
		//  for now just stuff everything in the packet
		// increment the object count
		(*o) ++;
		// projectile id
		w_int16(p, j->id);
		p += 2;
		// object x, y
		w_int24(p, (((unsigned int)j->prev_x << 12) | ((unsigned int)j->prev_y) & 0xFFF));
		p += 3;
		// 24 bits -
		//  4 bits for Type
		//  1 bit for Fractional Life (set the bit if projectile terminates early)
		// 11 bits Angle (2048 steps)
		unsigned short proj_angle = snap(j->angle, 2048);
		*p = (j->type << 4) | ((j->life < 1) << 3) | ((proj_angle >> 8) & 0x7);
		p ++;
		*p = (proj_angle & 0xFF);
		p ++;

		if (j->life < 1)
			*p = (unsigned char)(j->life * 256.0);
		else
			*p = (unsigned char)(j->life);

		p ++;
		j = j->next;
	}

	// FX updates.
	//  The FX list is one-shot entries
	o = p;
	p ++;
	*o = 0;
	struct effect * f = effect_list;

	while (f != NULL) {
		// TODO: determine what's near player
		//  for now just stuff everything in the packet
		// increment the object count
		(*o) ++;
		// object x, y
		w_int24(p, (((unsigned int)f->x << 12) | ((unsigned int)f->y) & 0xFFF));
		p += 3;
		*p = f->type;
		p ++;
		*p = (unsigned char)(f->delay * 256.0);
		p ++;
		f = f->next;
	}

	return p - payload;
}
