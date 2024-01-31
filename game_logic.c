/* game_logic.cpp
	used by client or server */

#include "game_logic.h"
#include <stddef.h>

#include <stdlib.h>
#include <stdio.h>

#define _USE_MATH_DEFINES
#include <math.h>
#ifndef M_2_PI
  #define M_2_PI (M_PI * 2.0)
#endif
#ifndef M_3_PI_2
  #define M_3_PI_2 (M_PI_2 * 3.0)
#endif
#include <float.h>

#include "data.h"

// /////////////////////////////////////////////////////////////////
// rolling values
static unsigned long frame;
static unsigned short id;

// /////////////////////////////////////////////////////////////////
// Money settings
static short cash[2];

static unsigned char cash_frame;

// /////////////////////////////////////////////////////////////////
// All map info
static unsigned char tile[MAP_Y][MAP_X];

// HQ center point and occupant
static struct {
	unsigned short x, y;
	struct human * occupant;
} hq[2];

// Capture point center and owner
static struct {
	unsigned short x, y;
	unsigned char owner;
} cp[3];

// Solid buildings
static struct {
	unsigned short x0, y0, x1, y1;
} bldg[12];

// /////////////////////////////////////////////////////////////////
// This "player" struct links controls to an entity and status
static struct
{
	// 0 = not connected
	// 1 = dead
	// 2 = controlling human
	// 3 = in vehicle
	// 4 = in HQ
	unsigned char status;

	// controls - as read from packet
	//  fire (mouse button) and enter/exit shared
	union {
		struct {
			// human only controls
			unsigned char fire, enter;
			char accel, rotate;
			float aim;
			unsigned char weaponswap, fire_secondary;
		} h;
		struct {
			// vehicle only controls
			unsigned char fire, enter;
			char accel, rotate;
			float aim;
		} v;
		struct {
			// hq only controls
			unsigned char fire, enter;
			unsigned char x, y, type;
		} q;
	} controls;

	// ptr to the human they control
	struct human * h;

	// if in a vehicle, which one and which seat number
	struct vehicle * v;
	char seat;
} players[MAX_PLAYERS];

/*
static struct
{
	// AI brain storage
} brain[MAX_PLAYERS];
*/

// /////////////////////////////////////////////////////////////////
// GAME OBJECTS

// Humans
static struct human
{
	unsigned short id;

	unsigned char team;

	// movement
	char speed; // -15 to 15
	char rotation;

	float x, y; // 0 to 3200
	float angle; // 16 step

	float aim;  // 4096 step

	unsigned char hp;
	unsigned char weapon_active, weapon_carried, cooldown, ammo;

	struct human * next;
} *human_list = NULL;

// Vehicles etc, these come in multiple parts
struct vehicle_part
{
	// ptr to part dictionary
	const struct object_child * o;

	// local settings
	float angle; // 16384 step
	unsigned char cooldown;

	struct human * occupant;

	struct vehicle_part * child;
	struct vehicle_part * next;
};

static struct vehicle
{
	// ptr to obj dictionary
	const struct object_base * o;

	unsigned short id;

	unsigned char type, team;

	char speed; // -64 to +63
	char rotation;

	float x, y;
	float angle;	// 512 step

	unsigned short hp;
	unsigned char cooldown;

	struct human * occupant;

	struct vehicle_part * child;
	struct vehicle * next;
} *vehicle_list = NULL;

// Projectiles
static struct projectile
{
	unsigned short id;
	unsigned char type, team;

	float x, y;
	float angle;	// 2048 step
	float life;

	struct projectile * next;
} * projectile_list = NULL;

// Special effects
static struct effect
{
	unsigned char type;

	float x, y;

	float delay;

	struct effect * next;
} * effect_list = NULL;

// /////////////////////////////////////////////////////////////////
// GAME OBJECT MANIP FUNCTIONS
// create a vehicle and add it to the list
//  this is the recursive function for sub-parts
static vehicle_part * create_vehicle_part(const struct object_child * o)
{
	struct vehicle_part * c = (struct vehicle_part *) malloc(sizeof(struct vehicle_part));
	c->o = o;

	// initial settings
	c->angle = 0;
	c->cooldown = o->cooldown;
	c->occupant = NULL;

	if (o->child)
		c->child = create_vehicle_part(o->child);
	else
		c->child = NULL;

	if (o->next)
		c->next = create_vehicle_part(o->next);
	else
		c->next = NULL;

	return c;
}

static void create_vehicle(unsigned char type, unsigned char team, float x, float y, float angle)
{
	struct vehicle * v = (struct vehicle *) malloc(sizeof(struct vehicle));
	v->id = id; id ++;
	v->type = type;

	// set the ptr to the library object
	v->o = vehicle_detail[type];

	// initial settings
	v->team = team;
	v->x = x;
	v->y = y;
	v->speed = 0;
	v->angle = angle; //snap(angle, 512);

	v->hp = v->o->hp;
	v->cooldown = v->o->cooldown;

	v->occupant = NULL;

	// set up child objects
	if (v->o->child)
		v->child = create_vehicle_part(v->o->child);
	else
		v->child = NULL;

	v->next = vehicle_list;
	vehicle_list = v;
}

// create a projectile and add it to the list
static void create_projectile(unsigned char type, unsigned char team, float x, float y, float angle)
{
	struct projectile * j = (struct projectile *) malloc(sizeof(struct projectile));
	j->id = id; id ++;
	j->type = type;
	j->team = team;
	j->x = x;
	j->y = y;
	j->angle = angle; //snap(angle, 2048);

	j->life = projectile_detail[type].life;

	j->next = projectile_list;
	projectile_list = j;
}

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
static float min(float x, float y) { return x < y ? x : y; }
static float max(float x, float y) { return x > y ? x : y; }

// "fast" float square
static float sqr (float x) { return x * x ; }

// normalize an angle to [0, 2*PI)
static float norm (float x) {
	while (x >= M_2_PI) x -= M_2_PI;
	while (x < 0) x += M_2_PI;
	return x;
}

// expand a step fraction into an angle (inverse of above)
//static float unsnap(int step, unsigned int steps) {
//	return (2 * M_PI) * step / steps ;
//}

// AABB to AABB collision
//  In case of a hit, the first object is pushed out
/*
static int aabb_aabb(short *x, short *y, unsigned short w, unsigned short h,
			   short ox, short oy, unsigned short ow, unsigned short oh)
{
	// find protrusion into each side
	int p_l = *x + w - ox; if (p_l <= 0) return 0;
	int p_r = ox + ow - *x; if (p_r <= 0) return 0;
	int p_u = *y + h - oy; if (p_u <= 0) return 0;
	int p_d = oy + oh - *y; if (p_d <= 0) return 0;

	// collision.  use minimum dist (above) to figure out which edge to lock to
	if (p_l <= p_r && p_l <= p_u && p_l <= p_d)
		*x = ox - w;
	else if (p_r <= p_u && p_r <= p_d)
		*x = ox + ow;
	else if (p_u <= p_d)
		*y = oy - h;
	else
		*y = oy + oh;

	return 1;
}
*/

// Point to Circle collision
//  In case of a hit, the first object is pushed out
static int point_circle(float *x, float *y,
					  float ox, float oy, unsigned short or)
{
	// check distance
	float d_sqr = sqr(ox - *x) + sqr(oy - *y);

	// too close?
	if (d_sqr >= sqr(or)) return 0;

	// determine angle of intrusion
	float theta = atan2f(oy - *y, ox - *x);
	// push first object away
	*x = ox - or * cos(theta);
	*y = oy - or * sin(theta);

	return 1;
}

// Circle to Circle collision
//  In case of a hit, the first object is pushed out
static int circle_circle(float *x, float *y, unsigned short r,
					  float ox, float oy, unsigned short or)
{
	return point_circle(x, y, ox, oy, r + or);
}

// Circle to AABB collision
//  In case of a hit, the first object is pushed out
static int circle_aabb(float *x, float *y, unsigned short r,
						float ox0, float oy0, float ox1, float oy1)
{
	// find protrusion into each side of an expanded square
	float p_l = *x + r - ox0; if (p_l <= 0) return 0;
	float p_r = ox1 + r - *x; if (p_r <= 0) return 0;
	float p_u = *y + r - oy0; if (p_u <= 0) return 0;
	float p_d = oy1 + r - *y; if (p_d <= 0) return 0;

	// it's in the bounding square.
	//  based on the closest edges, test a corner, or don't
	// collision.  use minimum dist (above) to figure out which edge to lock to
	if (p_l <= p_r && p_l <= p_u && p_l <= p_d)
	{
		// left edge
		if (*y < oy0) return point_circle(x, y, ox0, oy0, r); 
		else if (*y > oy1) return point_circle(x, y, ox0, oy1, r); 
		else *x = ox0 - r;
	} else if (p_r <= p_u && p_r <= p_d) {
		// right edge
		if (*y < oy0) return point_circle(x, y, ox1, oy0, r); 
		else if (*y > oy1) return point_circle(x, y, ox1, oy1, r); 
		else *x = ox1 + r;
	} else if (p_u <= p_d) {
		// top edge
		if (*x < ox0) return point_circle(x, y, ox0, oy0, r); 
		else if (*x > ox1) return point_circle(x, y, ox1, oy0, r); 
		else *y = oy0 - r;
	} else {
		// bottom edge
		// top edge
		if (*x < ox0) return point_circle(x, y, ox0, oy1, r); 
		else if (*x > ox1) return point_circle(x, y, ox1, oy1, r); 
		else *y = oy1 + r;
	}

	return 1;
}

// Ray to AABB collision
//  Return a distance at which the collision happens (or t_start if never)
static float ray_aabb(float x0, float y0, float dx, float dy,
						float ox0, float oy0, float ox1, float oy1)
{
    float tmin = -FLT_MAX; float tmax = FLT_MAX;

//	float ray_dx = x1 - x0;
	if (dx != 0.0) {
        double tx1 = (ox0 - x0)/dx;
        double tx2 = (ox1 - x0)/dx;

        tmin = max(tmin, min(tx1, tx2));
        tmax = min(tmax, max(tx1, tx2));
    }

//	float ray_dy = y1 - y0;
    if (dy != 0.0) {
        double ty1 = (oy0 - y0)/dy;
        double ty2 = (oy1 - y0)/dy;

        tmin = max(tmin, min(ty1, ty2));
        tmax = min(tmax, max(ty1, ty2));
    }

	if (tmax >= tmin) return tmin;
	else return FLT_MAX;
}

// Ray to Circle collision
//  Return a distance at which the collision happens (or t_start if never)
/*
static float ray_circle(float x0, float y0, float dx, float dy,
						float ox, float oy, float or)
{
	// determine the 
    float tmin = -FLT_MAX; float tmax = FLT_MAX;

//	float ray_dx = x1 - x0;
	if (dx != 0.0) {
        double tx1 = (ox0 - x0)/dx;
        double tx2 = (ox1 - x0)/dx;

        tmin = max(tmin, min(tx1, tx2));
        tmax = min(tmax, max(tx1, tx2));
    }

//	float ray_dy = y1 - y0;
    if (dy != 0.0) {
        double ty1 = (oy0 - y0)/dy;
        double ty2 = (oy1 - y0)/dy;

        tmin = max(tmin, min(ty1, ty2));
        tmax = min(tmax, max(ty1, ty2));
    }

	if (tmax >= tmin) return tmin;
	else return FLT_MAX;
}
*/


/* Tests for a collision between any two convex polygons.
 * Returns 0 if they do not intersect
 * Returns 1 if they intersect
 */
#if 0
int boxboxCollision(double a_x[4], double a_y[4], double b_x[4], double b_y[4])
{
	double axis_x, axis_y, tmp, minA, maxA, minB, maxB;
	int side, i;

	/* test polygon A's sides */
	for (side = 0; side < 4; side++)
	{
		/* get the axis that we will project onto */
		axis_x = a->Points[side].y - a->Points[(side + 1) % 4].y;
		axis_y = a->Points[(side + 1) % 4].x - a->Points[side].x;

		/* normalize the axis */
		tmp = sqrt(axis_x * axis_x + axis_y * axis_y);
		axis_x /= tmp;
		axis_y /= tmp;

		/* project polygon A onto axis to determine the min/max */
		minA = maxA = a->Points[0].x * axis_x + a->Points[0].y * axis_y;
		for (i = 1; i < 4; i++)
		{
			tmp = a->Points[i].x * axis_x + a->Points[i].y * axis_y;
			if (tmp > maxA)
				maxA = tmp;
			else if (tmp < minA)
				minA = tmp;
		}
		/* correct for offset */
		tmp = a->Center.x * axis_x + a->Center.y * axis_y;
		minA += tmp;
		maxA += tmp;

		/* project polygon B onto axis to determine the min/max */
		minB = maxB = b->Points[0].x * axis_x + b->Points[0].y * axis_y;
		for (i = 1; i < b->NumSides; i++)
		{
			tmp = b->Points[i].x * axis_x + b->Points[i].y * axis_y;
			if (tmp > maxB)
				maxB = tmp;
			else if (tmp < minB)
				minB = tmp;
		}
		/* correct for offset */
		tmp = b->Center.x * axis_x + b->Center.y * axis_y;
		minB += tmp;
		maxB += tmp;

		/* test if lines intersect, if not, return false */
		if (maxA < minB || minA > maxB)
			return 0;
	}

	/* test polygon B's sides */
	for (side = 0; side < b->NumSides; side++)
	{
		/* get the axis that we will project onto */
		if (side == 0)
		{
			axis_x = b->Points[b->NumSides - 1].y - b->Points[0].y;
			axis_y = b->Points[0].x - b->Points[b->NumSides - 1].x;
		}
		else
		{
			axis_x = b->Points[side - 1].y - b->Points[side].y;
			axis_y = b->Points[side].x - b->Points[side - 1].x;
		}

		/* normalize the axis */
		tmp = sqrt(axis_x * axis_x + axis_y * axis_y);
		axis_x /= tmp;
		axis_y /= tmp;

		/* project polygon A onto axis to determine the min/max */
		minA = maxA = a->Points[0].x * axis_x + a->Points[0].y * axis_y;
		for (i = 1; i < a->NumSides; i++)
		{
			tmp = a->Points[i].x * axis_x + a->Points[i].y * axis_y;
			if (tmp > maxA)
				maxA = tmp;
			else if (tmp < minA)
				minA = tmp;
		}
		/* correct for offset */
		tmp = a->Center.x * axis_x + a->Center.y * axis_y;
		minA += tmp;
		maxA += tmp;

		/* project polygon B onto axis to determine the min/max */
		minB = maxB = b->Points[0].x * axis_x + b->Points[0].y * axis_y;
		for (i = 1; i < b->NumSides; i++)
		{
			tmp = b->Points[i].x * axis_x + b->Points[i].y * axis_y;
			if (tmp > maxB)
				maxB = tmp;
			else if (tmp < minB)
				minB = tmp;
		}
		/* correct for offset */
		tmp = b->Center.x * axis_x + b->Center.y * axis_y;
		minB += tmp;
		maxB += tmp;

		/* test if lines intersect, if not, return false */
		if (maxA < minB || minA > maxB)
			return 0;
	}

	return 1;
}
#endif

// /////////////////////////////////////////////////////////////////
// GAME UPDATE

// helper functions - check for seat in child objects
static struct vehicle_part * check_seat_child(struct vehicle_part * v)
{
	do {
		if (v->o->seat_id > -1 && v->occupant == NULL) {
			// this seat was empty, return it
			return v;
		}

		if (v->child) {
			// check the child tree
			struct vehicle_part * c = check_seat_child(v->child);
			// found something there, bubble it up
			if (c != NULL) return c;
		}

		v = v->next;
	} while (v != NULL);

	return NULL;
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
	while (cash_frame >= CASH_FREQ)
	{
		cash[0] += CASH_UPKEEP;  //upkeep
		cash[1] += CASH_UPKEEP;

		// money earned for each capture point owned
		for (int i = 0; i < 3; i++)
		{
			if (cp[i].owner == 1) cash[0] += CASH_POINT;
			else if (cp[i].owner == 2) cash[1] += CASH_POINT;
		}
		cash_frame -= CASH_FREQ;
	}

	// Check for game over here.
	if (cash[0] <=0 || cash[1] <= 0)
	{
		// restore everyone to Dead status
		//  todo: interlude status idk
		for (int i=0; i<MAX_PLAYERS; i++)
		{
			if (players[i].status) {
				players[i].status = 1;
			}
		}

		// destroy all humans!
		/*
		humptr = tophuman;
		while (humptr != NULL)
		{
			humptr->health = 0;
			humptr=humptr->next;
		}
		while (objptr != NULL)
		{
			objptr->hp = 0;
			objptr=objptr->next;
		}
		*/

		/*
		if (cash[0] <= 0)
		{
			globalchat("AXIS VICTORY!");
		} else if (cash[1] <= 0) {
			globalchat("ALLIED VICTORY!");
		}
		*/

		/*
		bldloc[0][0] = 0;
		bldloc[1][0] = 0;
		bldloc[2][0] = 0;
		cash[0] = 500;
		cash[1] = 500;
		cashmoney=0;
		mapplays++;
		if (mapplays > maxmapplays)
		{
		   mapplays = 0;
		   mapnum++;
		   if (mapnum >= maxmapnum) mapnum = 0;
		   mapchange(mapnum);
		}
		*/
	} else {
		// apply all new player input
		for (int i=0; i<MAX_PLAYERS; i++)
		{
			if (players[i].status == 1) {
				// Player is waiting for respawn.
				//  TODO: cooldown before respawn?

				// Create a brand new Human
				human * h = (struct human *)malloc(sizeof(struct human));
				h->id = id; id ++;

				// add unit to big list
				h->next = human_list;
				human_list = h;

				// put the player into the driver's seat of their body
				players[i].h = h;
//				players[i].id = h->id;

				// set other values
				h->team = i % 2;
				h->hp = 100;
				h->weapon_active = 1;	// pistol

				// TODO
				h->weapon_carried = 2;
				h->ammo = 20;

				h->speed = 0;
				h->cooldown = 5;

				// Attempt to place the entity somewhere on the map
				//  It should be within some radius of the HQ, but not colliding with anything
				do {
					float q = (float)rand() / RAND_MAX;
					float r = 256.0f * sqrt(q);
					h->x = hq[h->team].x + r * cos(q * M_2_PI);
					h->y = hq[h->team].y + r * sin(q * M_2_PI);
//					printf("Spawning player at theta=%f, r=%f, h->x=%f, h->y=%f (hq at %f, %f)\n", theta, r, h->x, h->y, hq[h->team].x, hq[h->team].y);

					// todo: test collision
					break;
				} while(true);

				// point player at enemy
				h->angle = atan2f(hq[! h->team].y - h->y, hq[! h->team].x - h->x);
				h->aim = 0;

				// Pay the respawn fee
				cash[h->team] -= CASH_RESPAWN;

				// ready to rock
				players[i].status = 2;
			} else if (players[i].status == 2) {
				// Player is active and controlling some human

				// movement
				players[i].h->speed = 15 * players[i].controls.h.accel;
				players[i].h->angle = norm(players[i].h->angle + players[i].controls.h.rotate / (8 * M_PI));

				// aim
				//  convert player aim into a bearing and make sure it's within their forward 180 degrees
				float aim = norm(players[i].controls.h.aim - players[i].h->angle);
//				unsigned short aim = (snap(players[i].controls.h.aim, 4096) - players[i].h->angle * 256) & 0xFFF;
				if (aim > M_PI_2 && aim < M_PI) aim = M_PI_2;
				else if (aim >= M_PI && aim < M_3_PI_2) aim = M_3_PI_2;
//				if (aim > 1024 && aim < 2048) aim = 1024;
//				else if (aim >= 2048 && aim < 3096) aim = 3096;
//				players[i].h->aim = aim & 0xFFF;

				// weapon swap
				if (players[i].controls.h.weaponswap && players[i].h->weapon_carried)
				{
					if (players[i].h->weapon_carried == players[i].h->weapon_active)
						players[i].h->weapon_active = 1;
					else
						players[i].h->weapon_active = players[i].h->weapon_carried;
				}

				// shoot the gun
				if (players[i].controls.h.fire && players[i].h->cooldown == 0) {
					if (players[i].h->weapon_active == 1) {
						// pistol
						create_projectile(0, players[i].h->team, players[i].h->x, players[i].h->y, 
							players[i].h->aim + players[i].h->angle);
						players[i].h->cooldown += 3;
					}
					else if (players[i].h->weapon_active == 2 && players[i].h->ammo > 0) {
						// rocket
						create_projectile(1, players[i].h->team, players[i].h->x, players[i].h->y, 
							players[i].h->aim + players[i].h->angle);
						players[i].h->ammo --;
						players[i].h->cooldown += 10;
					}
				}

				// enter vehicle attempt
				if (players[i].controls.h.enter) {
					// check for HQ entry
					if (hq[i % 2].occupant == NULL) {
						// building is available...
						if (circle_aabb(&(players[i].h->x), &(players[i].h->y), 64,
							bldg[i % 2].x0, bldg[i % 2].y0, bldg[i % 2].x1, bldg[i % 2].y1))
						{
							players[i].status = 4;
							hq[i % 2].occupant = players[i].h;

							// splice the human out of the human_list
							if (players[i].h == human_list) {
								human_list = players[i].h->next;
							} else {
								human * h = human_list;
								while (players[i].h != h->next)
								{
									h = h->next;
								}
								h->next = players[i].h->next;
							}
							players[i].h->next = NULL;

							continue;
						}
					}

					// check for vehicle entry
					struct vehicle * v = vehicle_list;
					while (v != NULL) {
						if (v->team == i % 2) {
							int seat_id = -1;
							if (v->o->seat_id > -1 && v->occupant == NULL) {
								// found a seat
								seat_id = v->o->seat_id;

								// stash human in there
								v->occupant = players[i].h;
							} else if (v->child) {
								// check child for available seat
								struct vehicle_part * c = check_seat_child(v->child);
								if (c != NULL) {
									// found a seat
									seat_id = c->o->seat_id;

									// stash human in there
									c->occupant = players[i].h;
								}
							}

							if (seat_id > -1) {
								// ok record our occupancy
								players[i].status = 3;

								players[i].v = v;
								players[i].seat = seat_id;

								// splice the human out of the human_list
								if (players[i].h == human_list) {
									human_list = players[i].h->next;
								} else {
									human * h = human_list;
									while (players[i].h != h->next)
									{
										h = h->next;
									}
									h->next = players[i].h->next;
								}
								players[i].h->next = NULL;

								// reset all the other controls
								players[i].controls.h.accel =
									players[i].controls.h.rotate =
									players[i].controls.h.weaponswap =
									players[i].controls.h.fire =
									players[i].controls.h.fire_secondary =
									players[i].controls.h.enter =
									players[i].controls.h.aim = 0;

								break;
							}
						}
						v = v->next;
					}
				}
			} else if (players[i].status == 3) {
				// Player inside a vehicle somewhere
				struct vehicle * v = players[i].v;
				if (players[i].seat == 0)
				{
					// keyboard movement controlled by this player
					v->speed += v->o->accel * players[i].controls.h.accel;

					if (v->speed > v->o->speed)
						v->speed = v->o->speed;
					else if (v->speed < -v->o->accel)
						v->speed = -v->o->accel;

					v->angle += v->o->rotation * players[i].controls.h.rotate;
				}

//				if (v->o->mouse == players[i].seat)
//				{
					// mouse?
					//  convert player aim into a bearing and make sure it's within their foreward 180 degrees
/*					unsigned short aim = (snap(players[i].controls.h.aim, 4096) - players[i].h->angle * 256) & 0xFFF;
					if (aim > 1024 && aim < 2048) aim = 1024;
					else if (aim >= 2048 && aim < 3096) aim = 3096;
					players[i].h->aim = aim & 0xFFF; */

					// shoot the gun
/*					if (players[i].controls.h.fire && players[i].h->cooldown == 0) {
						if (players[i].h->weapon_active == 1) {
							// pistol
							create_projectile(0, players[i].h->team, players[i].h->x, players[i].h->y, 
								unsnap(players[i].h->aim, 4096) + unsnap(players[i].h->angle, 16));
							players[i].h->cooldown += 3;
						}
					} */
//				}

				// exit vehicle
				if (players[i].controls.h.enter) {
					// search vehicle for correct seat ID
					if (v->o->seat_id == players[i].seat)
					{
						v->occupant = NULL;
						players[i].h->next = human_list;
						human_list = players[i].h;

						players[i].status = 2;
						players[i].seat = -1;

						players[i].h->x = v->x;
						players[i].h->y = v->y;
					} else {
						// child
					}
				}
			} else if (players[i].status == 4) {
				// Player is in the HQ

				// exit HQ
				if (players[i].controls.q.enter) {
					hq[i % 2].occupant = NULL;

					players[i].h->next = human_list;
					human_list = players[i].h;

					players[i].status = 2;
				}
			}
		}
					/*
							if (players[i].controls[4])
							{
								if (humptr->cooldown == 0 && map[humptr->x/32][humptr->y/32] != 0)
								{
									if (humptr->weapon == 3)
									{
										humptr->cooldown=2;
										createbullet(humptr->align,humptr->x,humptr->y,humptr->torsoangle + humptr->angle, 150, 0, 3);
									}
									else if (humptr->weapon == 4 && humptr->ammo>0)
									{
										humptr->ammo--;
										humptr->cooldown=15;
										createbullet(humptr->align,humptr->x,humptr->y,humptr->torsoangle + humptr->angle, 100, 1, 4);
									}
									else if (humptr->weapon == 5 && humptr->ammo>0)
									{
										humptr->ammo--;
										humptr->cooldown=10;
										createbullet(humptr->align,humptr->x,humptr->y,humptr->torsoangle + humptr->angle, 200, 2, 3);
									}
									else if (humptr->weapon == 6 && humptr->ammo>0)
									{
										humptr->ammo--;
										createbullet(humptr->align,humptr->x,humptr->y,humptr->torsoangle + humptr->angle, 100, 3, 4);
									}
									else if (humptr->weapon == 7 && humptr->ammo>0)
									{
										humptr->ammo--;
										humptr->cooldown=5;
										createbullet(humptr->align,humptr->x,humptr->y,humptr->torsoangle + humptr->angle, 20, 5, 10);
									}
									else if (humptr->weapon == 8 && humptr->ammo>0)
									{
										humptr->ammo--;
										humptr->cooldown=5;
										createbullet(humptr->align,humptr->x,humptr->y,humptr->torsoangle + humptr->angle, 10, 6, 10);
									}
								}
							}
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
							if (players[i].controls[6])	// vehicle entrance!
							{
								//		   printf("Player tried to enter vehicle.\n");
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
					if (players[i].controls[6])	// vehicle exit!
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

		//  Humans first
		struct human * h = human_list;
		while (h != NULL)
		{
			// reduce weapon cooldown, if any
			if (h->cooldown) h->cooldown --;

			if (h->speed) {
				// possible new movement for object
				h->x += h->speed * cos(h->angle);
				h->y += h->speed * sin(h->angle);

				// boundaries
				if (h->x < 16) h->x = 16;
				else if (h->x > 3184) h->x = 3184;
				if (h->y < 16) h->y = 16;
				else if (h->y > 3184) h->y = 3184;

				// check human vs building collision
				//  if colliding, push them out of the building (closest edge)
				for (int i = 0; i < 12; i ++)
				{
					circle_aabb(&(h->x), &(h->y), 16, bldg[i].x0, bldg[i].y0, bldg[i].x1, bldg[i].y1);
				}

				// check human vs other human collision
				struct human * o = human_list;
				while (o != NULL)
				{
					if (o != h) {
						circle_circle(&(h->x), &(h->y), 16, o->x, o->y, 16);
					}
					o = o->next;
				}
			}

			h = h->next;
		}

		// Vehicles
		struct vehicle * v = vehicle_list;
		while (v != NULL)
		{
			// reduce weapon cooldown, if any
			if (v->cooldown) v->cooldown --;

			if (v->speed) {
				// possible new movement for object
				v->x += v->speed * cos(v->angle);
				v->y += v->speed * sin(v->angle);

				// boundaries
				//  todo: use actual bounding box
				if (v->x < 16) v->x = 16;
				else if (v->x > 3184) v->x = 3184;
				if (v->y < 16) v->y = 16;
				else if (v->y > 3184) v->y = 3184;

				// check vehicle vs building collision
				//  if colliding, push them out of the building (closest edge)
				// damage dealt is based on distance pushed
				for (int i = 0; i < 12; i ++)
				{
//					obb_aabb(&(h->x), &(h->y), 16, bldg[i].x0, bldg[i].y0, bldg[i].x1, bldg[i].y1);
				}

				// check vehicle vs human collision
				struct human * h = human_list;
				while (h != NULL)
				{
//					obb_circle(&(h->x), &(h->y), 16, o->x, o->y, 16);
					h = h->next;
				}

				// check vehicle vs vehicle collision
				struct vehicle * o = vehicle_list;
				while (o != NULL)
				{
					if (o != v) {
//						obb_obb(&(h->x), &(h->y), 16, o->x, o->y, 16);
					}
					o = o->next;
				}
			}

			v = v->next;
		}

		// Projectiles
		struct projectile * j = projectile_list, * j_prev = NULL;
		while (j != NULL)
		{
			// reduce life, destroy self if 0
			if (j->life < 1) {
				if (j->type == 1) {
					// move projectile to its dest
					j->x += projectile_detail[j->type].speed * j->life * cos(j->angle);
					j->y += projectile_detail[j->type].speed * j->life * sin(j->angle);

					// explode rocket
					create_effect(1, j->x, j->y, j->life);
				}

				if (j == projectile_list)
				{
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

				// possible new movement for object
				float prev_x = j->x;
				j->x += projectile_detail[j->type].speed * cos(j->angle);
				if (j->x < 0 || j->x >= 3200) {
					// todo delete it
					j->life = 1;
					j->x = 4000;
				} else {
					float prev_y = j->y;
					j->y += projectile_detail[j->type].speed * sin(j->angle);
					// boundaries
					if (j->y < 0 || j->y >= 3200)
					{
						// flew out of bounds
						j->life = 1;
						j->y = 4000;
					} else {
						// check vs building collision
						float t = FLT_MAX;
						int hit_bldg = -1;
						for (int i = 0; i < 12; i ++)
						{
//							printf("Testing ray_aabb vs bldg %d\n", i);
							float new_t = ray_aabb(prev_x, prev_y, j->x - prev_x, j->y - prev_y, bldg[i].x0, bldg[i].y0, bldg[i].x1, bldg[i].y1);
							if (new_t > 0 && new_t < t) {
								printf("Hit at t = %f\n", new_t);
								t = new_t;
								hit_bldg = 1;
							} else {
								printf("No hit, t remains %f\n", t);
							}
						}

						// check vs human collision
						struct human * o = human_list;
						while (o != NULL)
						{
		//					if (o != h) {
		//						aabb_aabb(&(h->x), &(h->y), 32, 32, o->x, o->y, 32, 32);
		//					}
							o = o->next;
						}

						if (t < 1)
							j->life = t;
						// check vs vehicle collision
					}
				}

				j_prev = j;
				j = j->next;
			}
		}

		// check for capture points changing hands
		//  to take a CP you must be the only team of humans within 128px
		for (int i = 0; i < 3; i ++)
		{
			unsigned char new_owner = 0;
			struct human * h = human_list;
			while (h != NULL)
			{
				unsigned char team = 1 + h->team;
				if (new_owner == 0 || new_owner != team) {
					// check for proximity only if another teammate isn't already there
//					printf("Player is at %f, %f and CP %d at %f, %f.  Distance: %f", h->x, h->y, i, cp[i].x, cp[i].y, sqr(h->x - cp[i].x) + sqr(h->y - cp[i].y));
					if (sqr(h->x - cp[i].x) + sqr(h->y - cp[i].y) < 64 * 64) {
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
static char load_map(const char* name)
{
	int i, j, c;

	FILE *fp = fopen (name,"rb");
	if (fp == NULL)
	{
		fprintf(stderr,"Error opening map %s!\n",name);
		return 0;
	}

	for (j=0; j<MAP_Y; j++)
	{
		for (i=0; i<MAP_X; i+=2)
		{
			// Read next (binary) character from file.
			unsigned char incoming = fgetc(fp);

			// Maps are 100x100 with up to 16 choices for tile.
			tile[i][j] = (incoming & 0xF0) >> 4;
			tile[i+1][j] = (incoming & 0x0F);
		}
	}

	// all buildings
	c = 0;
	j = 0;
	for (i = 0; i < 15; i ++)
	{
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
			// define two points of the building
			bldg[j].x0 = 16 + x * 32 - (building_size[type].w / 2);
			bldg[j].x1 = 16 + x * 32 + (building_size[type].w / 2);
			bldg[j].y0 = 16 + y * 32 - (building_size[type].h / 2);
			bldg[j].y1 = 16 + y * 32 + (building_size[type].h / 2);

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

	human_list = NULL;
	vehicle_list = NULL;
	projectile_list = NULL;
	effect_list = NULL;

	load_map(map);

	// add bot controllers for AI at the bottom of the player list
	for (int i = MAX_PLAYERS - ai_players; i < MAX_PLAYERS; i ++)
	{
		players[i].status = 1;
	}
	// add AI HQ players

	// stick a jeep in there lol
//	vehicle_list =
	create_vehicle(0, 0, hq[0].x + 150, hq[0].y - 50, 0);
//	vehicle_list->child->occupant = (struct human *)1;
//	vehicle_list->child->next->occupant = (struct human *)1;
}

void shutdown_game()
{
	// destroy everything!
	for (int i = 0; i < MAX_PLAYERS; i ++)
	{
		players[i].status = 0;
	}
}

// Connect a player to the game
//  Returns -1 if failure (game full)
int connect_player()
{
	for (int i = 0; i < MAX_PLAYERS; i ++)
	{
		if (players[i].status == 0) {
			players[i].status = 1;
			return i;
		}
	}

	return -1;
}

// Drop player from the game
void disconnect_player(int slot)
{
	/* todo: kill human or unman vehicle etc
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
	if (players[slot].status == 2 ||
		players[slot].status == 3) {
		players[slot].controls.h.accel = speed;
		players[slot].controls.h.rotate = turn;
		players[slot].controls.h.fire = fire;
		players[slot].controls.h.fire_secondary = fire_secondary;
		players[slot].controls.h.weaponswap = weaponswap;
		players[slot].controls.h.enter = enter;

		players[slot].controls.h.aim = aim;
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

// snap floating point angle to one of N segments
static unsigned int snap(float theta, unsigned int steps) {
	// normalize angle - is this needed?  does unsigned + modulus handle it?
	if (theta < 0) theta += M_2_PI;
	// convert to steps, round and modulus
	return (unsigned int)(0.5 + (theta / M_2_PI) * steps) % steps;
}

static void w_int32(unsigned char *p, const unsigned int i)
{
	p[0] = (i >> 24) & 0xFF;
	p[1] = (i >> 16) & 0xFF;
	p[2] = (i >> 8) & 0xFF;
	p[3] = i & 0xFF;
}

static void w_int24(unsigned char *p, const unsigned int i)
{
	p[0] = (i >> 16) & 0xFF;
	p[1] = (i >> 8) & 0xFF;
	p[2] = i & 0xFF;
}

static void w_int16(unsigned char *p, const unsigned short w)
{
	p[0] = (w >> 8) & 0xFF;
	p[1] = w & 0xFF;
}

// sub-objects
static unsigned char *serialize_child(unsigned char *p, struct vehicle_part *v)
{
	do
	{
		w_int16(p, (v->occupant != NULL ? 0x8000 : 0) |
			       snap(v->angle, 0x8000)); p += 2;

		if (v->child)
			p = serialize_child(p, v->child);

		v = v->next;
	}
	while (v != NULL);

	return p;
}

int serialize_game(int slot, unsigned char * payload)
{
	unsigned char *p = payload;

//	printf("Serializing for player %u\n", slot);

	// Global info
	// capture points owners
	*p = 0;
	for (int i = 0; i < 3; i ++) {
		*p |= (cp[i].owner << (i * 2));
	}
	p ++;

	// Cash
	for (int i = 0; i < 2; i ++) {
		w_int16(p, cash[i]); p += 2;
	}

	// Player Status (alive, dead, in HQ, etc)
	*p = players[slot].status; p++;

	if (players[slot].status == 2) {
		// Player ID and other local info
	//	printf("Writing ID as %u, status=%u\n", players[slot].id, players[slot].status);
		w_int16(p, players[slot].h->id); p += 2;
		// health
		*p = players[slot].h->hp; p ++;
		// ammo
		*p = players[slot].h->ammo; p ++;
	} else if (players[slot].status == 3) {
		w_int16(p, players[slot].v->id); p += 2;
		// health
		*p = players[slot].v->hp; p ++;
		// seat
		*p = players[slot].seat; p ++;
	} else {
		w_int16(p, 0); p += 2;
		// health
		*p = 0; p ++;
		// ammo
		*p = 0; p ++;
	}

	// Object updates.
	//  Since we don't know how many objects we're reporting about here (depends on vision),
	//  bookmark this point
	unsigned char *o = p; p ++;
	*o = 0;

	struct human * h = human_list;
	while (h != NULL) {
		// TODO: determine what's near player
		//  for now just stuff everything in the packet

		// increment the object count
		(*o) ++;

		// object id
		w_int16(p, h->id); p += 2;

		// team and weapon and movement speed
		// T00W WWSS
		//  this all fits in one byte with 2 bits to spare
		// TODO consider "is turning" in 2 bits here?  for prediction?
		*p = (h->team << 7) | (h->weapon_active << 2) | (h->speed > 0 ? 2 : (h->speed < 0 ? 1 : 0));
		p ++;

		// object x, y, aim, angle, and speed
		// x and y fit into 12 bits each so they only take 3 bytes
		w_int24(p, (((unsigned int)h->x << 12) | ((unsigned int)h->y) & 0xFFF)); p += 3;
		// angle locked to 1 of 16 directions (4 bits)
		//  aim is freeform so fit it into 4096 steps (12 bits)
		w_int16(p, (snap(h->angle, 16) << 12) |
			       (snap(h->aim, 4096))); p += 2;

		h = h->next;
	}

	// Vehicle (object) updates.
	//  Same deal as before: one byte to hold projectile count
	o = p; p ++;
	*o = 0;

	struct vehicle * v = vehicle_list;
	while (v != NULL) {
		// TODO: determine what's near player
		//  for now just stuff everything in the packet

		// increment the object count
		(*o) ++;

		// vehicle id
		w_int16(p, v->id); p += 2;

		// team and type and occupied
		*p = (v->team << 7) | (v->occupant != NULL ? 0x40 : 0) | v->type;
		p ++;

		// object x, y, angle, and speed
		// x and y fit into 12 bits each so they only take 3 bytes
		w_int24(p, (((unsigned int)v->x << 12) | ((unsigned int)v->y) & 0xFFF)); p += 3;

		// speed max is +-64 (7 bits)
		// angle in 512 steps (9 bits)
		w_int16(p, (snap(v->angle, 512) << 7) |
			       (v->speed & 0x7F)); p += 2;

		// and now: the children
		if (v->child)
			p = serialize_child(p, v->child);

		v = v->next;
	}

	// Projectile updates.
	//  Same deal as before: one byte to hold projectile count
	o = p; p ++;
	*o = 0;

	struct projectile * j = projectile_list;
	while (j != NULL) {
		// TODO: determine what's near player
		//  for now just stuff everything in the packet

		// increment the object count
		(*o) ++;

		// projectile id
		w_int16(p, j->id); p += 2;

		// object x, y
		w_int24(p, (((unsigned int)j->x << 12) | ((unsigned int)j->y) & 0xFFF)); p += 3;

		// 24 bits -
		//  4 bits for Type
		//  1 bit for Fractional Life (set the bit if projectile terminates early)
		// 11 bits Angle (2048 steps)
		unsigned short proj_angle = snap(j->angle, 2048);
		*p = (j->type << 4) | ((j->life < 1) << 3) | ((proj_angle >> 8) & 0x7); p ++;
		*p = (proj_angle & 0xFF); p ++;

		if (j->life < 1)
			*p = (unsigned char)(j->life * 256.0);
		else
			*p = (unsigned char)(j->life);
		p ++;

		j = j->next;
	}

	// FX updates.
	//  The FX list is one-shot entries
	o = p; p ++;
	*o = 0;

	struct effect * f = effect_list;
	while (f != NULL) {
		// TODO: determine what's near player
		//  for now just stuff everything in the packet

		// increment the object count
		(*o) ++;

		// object x, y
		w_int24(p, (((unsigned int)f->x << 12) | ((unsigned int)f->y) & 0xFFF)); p += 3;

		*p = f->type; p ++;

		f = f->next;
	}

	return p - payload;
}
