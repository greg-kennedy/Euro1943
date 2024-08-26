#ifndef DATA_H_
#define DATA_H_

// Money settings and costs
#define CASH_START 500

#define CASH_FREQ 5
#define CASH_UPKEEP -2
#define CASH_POINT 4
#define CASH_RESPAWN 15

// ENTITY DEFINITIONS

// Buildings
static struct {
	int w, h;
} building_size[] = {
	{ 64, 64 },	// capture point
	{ 128, 128 },	// hq
	{ 128, 128 },
	{ 64, 64 },	// tent
	{ 64, 64 },
	{ 128, 128 }, // radar dish
	{ 128, 128 },
	{ 256, 128 }, // factory

	{ 64, 64 },	// capture point (green)
	{ 64, 64 },	// capture point (tan)

	{ 64, 64 }, // small house
	{ 64, 128 } // large house
};

// Projectiles
#define TRAIL 0x01
#define EXPLOSIVE 0x02
static struct {
	unsigned char speed, life, flags;
} projectile_detail[] = {
	{ 150, 3, 0 },		// pistol and smg bullet
	{ 100, 4, TRAIL | EXPLOSIVE },		// rocket
	{ 200, 3, 0 },		// sniper round
	{ 0, 255, EXPLOSIVE },		// land mine
	{ 20, 10, EXPLOSIVE },		// grenade
	{ 10, 10, 0 },		// med kit
	{ 10, 10, 0 },		// ammo pack

	{ 0, 255, 0 },		// rocket launcher
	{ 0, 255, 0 },		// sniper rifle
	{ 0, 255, 0 },		// smg
	{ 0, 255, 0 },		// explosives
	{ 0, 255, 0 },		// backpack

	{ 100, 6, TRAIL },		// tank shell
	{ 150, 5, 0 },		// aa bullet
	{ 100, 6, TRAIL },		// d.gun shell
	{ 0, 4, EXPLOSIVE },		// bomb
};

// Objects
// TYPE flags
#define WATERBORNE 0x01
#define PLANE 0x02
#define HUMAN 0x03

// OTHER flags
#define SEAT_IMG 0x04
#define ANIMATED 0x08

// All objects in the game are made up of a parent object and then a tree of sub nodes.
//  The parent object is a container which can be rotated and has speed / accel values, and HP
//  Child objects are recursive and have x/y offsets, optional sprite, rotation, etc
struct object_sub {
	// sprite number (-1 for "none")
	const char sprite;
	// position of sprite relative to parent
	const short offset_x, offset_y;

	// sprite origin and scale
	const short center_x, center_y;
	const float scale;

	// rotation speed
	const float rotation;

	// projectile type, cooldown between shots
	const char projectile, cooldown;

	// is this a seat?  (can it hold another unit)
	const char seat_id;
	// which seat controls map to this object?
	const char controller;

	// flags (see above)
	const int flags;

	const struct object_sub * const sub;
	const struct object_sub * const next;
};

struct object_base {
	// object type number - must be unique!
	const unsigned char type;

	// sprite number (-1 for "none")
//	const char sprite;

	// sprite origin and scale
//	const short center_x, center_y;
//	const float scale;
	// global zoom imparted by vehicle
	const float zoom;

	// top speed, acceleration, rotation speed
	const unsigned char speed, accel;
	const float rotation;
	// hp before destruction
	const unsigned char hp;

	// projectile type, cooldown between shots
//	const char projectile, cooldown;

	// which seat controls map to this object?
	const char controller;

	// collision bounding box - TODO
	const unsigned short height;
	const unsigned short width;

	const int flags;

	// pointer to tree of sub-objects
	const struct object_sub * const sub;
	const struct object_base * const next;
};

// subobject definitions

// JEEP
//  jeep seats
								// spr, off_x,y, ctr_x,y,scale, rot, proj,cool, seat, m, flags, sub, next
static const struct object_sub jeep_r = { -1, -7, 13, 0,0,1,    0, -1,-1,       1, -1, SEAT_IMG, NULL, NULL };
static const struct object_sub jeep_l = { -1, -7, -13, 0,0,1,   0, -1,-1,       0, -1, SEAT_IMG, NULL, & jeep_r };
static const struct object_sub jeep_body = { 9, 0,0, 0,0,1,     0, -1,-1,      -1, -1, SEAT_IMG, &jeep_l, NULL };
										// type, zoom, spd,accel,rot, hp,  k,  w, h, flags, sub, next
static const struct object_base jeep = { 1, 1, 30,5,M_PI/16,100, 0, 64, 96, 0, & jeep_body, NULL };


// HUMAN
//  torso
//  there is a whole array of them for weapon swaps
													// spr, off_x,y, ctr_x,y,scale, rot, proj,cool, seat, m, flags, sub, next
static const struct object_sub human_torso_unarmed   = { 2, 0,0, 0,0,1.0, M_TAU, -1, 0, 0, 0, 0, NULL, NULL };
static const struct object_sub human_torso_pistol    = { 3, 0,0, 0,0,1.0, M_TAU,  0, 4, 0, 0, 0, NULL, NULL };
static const struct object_sub human_torso_rocket    = { 4, 0,0, 0,0,1.0, M_TAU,  1, 10, 0, 0, 0, NULL, NULL };
static const struct object_sub human_torso_sniper    = { 5, 0,0, 0,0,1.0, M_TAU,  2, 10, 0, 0, 0, NULL, NULL };
static const struct object_sub human_torso_smg       = { 6, 0,0, 0,0,1.0, M_TAU,  0, 1, 0, 0, 0, NULL, NULL };
static const struct object_sub human_torso_explosive = { 7, 0,0, 0,0,1.0, M_TAU,  4, 10, 0, 0, 0, NULL, NULL };
static const struct object_sub human_torso_backpack  = { 8, 0,0, 0,0,1.0, M_TAU,  5, 10, 0, 0, 0, NULL, NULL };

const struct object_sub * const human_torso[] = {
	& human_torso_unarmed,
	& human_torso_pistol,
	& human_torso_rocket,
	& human_torso_sniper,
	& human_torso_smg,
	& human_torso_explosive,
	& human_torso_backpack
};

											// spr, off_x,y, ctr_x,y,scale, rot, proj,cool, seat, m, flags, sub, next
static const struct object_sub human_body = { 0,       0,0,    0,0,1.0,       0, -1,0,       -1, -1, ANIMATED, & human_torso_smg, NULL };

										// type, zoom, spd,accel,rot,    hp,  k, w,  h,  flags, sub, next
static const struct object_base human = {   0,    1,   5,  5,    M_PI/8, 100, 0, 32, 32, HUMAN, & human_body, & jeep };

/*
// LIGHT TANK
//  minigun
static const struct object_sub lt_tank_turret_gun = { 3, -37, -12, 0,0,1, 90, 1, 1, 1, 1, 0, NULL, NULL };
static const struct object_sub lt_tank_turret     = { 2, 8, 0, 30,0,1, 10, 13, 20, 0, 0, 0, & lt_tank_turret_gun, NULL };
										// spr, scale, zoom, spd,accel,rot, hp, proj,cool, seat, w, h, flags, sub
static const struct object_base lt_tank = { 1, 1, 1, 25,5,16,175, -1,0, -1, 64, 96, 0, & lt_tank_turret };

// HEAVY TANK
static const struct object_sub hvy_tank_turret_gun_r = { 3, -34, 18, 0,0,1, 90, 1, 1, 2, 2, 0, NULL, NULL };
static const struct object_sub hvy_tank_turret_gun_l = { 3, -34, -18, 0,0,1, 90, 1, 1, 1, 1, 0, NULL, & hvy_tank_turret_gun_r };
static const struct object_sub hvy_tank_turret       = { 5, 0, 0, 37,0,1, 10, 13, 18, 0, 0, 0, & hvy_tank_turret_gun_l, NULL };
static const struct object_base hvy_tank = { 4, 1, 1, 20,4,16,250, -1,0, -1, 128, 192, 0, & hvy_tank_turret };

// ANTI-TANK GUN
											// spr, off_x,y, ctr_x,y,scale, rot, proj,cool, seat, m, flags, sub, next
static const struct object_sub at_gun_seat = { -1, 0,0, -44,0, 1, 0, -1, 0, 0, 0, SEAT_IMG, NULL, NULL };
static const struct object_sub at_gun_turret = { 7, 16, 0, 0,0,1, 90, 15, 8, -1, 0, 0, & at_gun_seat, NULL };
										// spr, scale, zoom, spd,accel,rot, hp, proj,cool, seat, w, h, flags, sub
static const struct object_base at_gun = { 6, 1, 1, 0,0,0,75, -1,0, -1, 32, 32, 0, & at_gun_turret };

// ANTI-AIR GUN
											// spr, off_x,y, ctr_x,y,scale, rot, proj,cool, seat, m, flags, sub, next
static const struct object_sub aa_gun_seat = { -1, 0,0, -44,0, 1, 0, -1, 0, 0, 0, SEAT_IMG, NULL, NULL };
static const struct object_sub aa_gun_turret = { 8, 16, 0, 0,0,1, 90, 14, 1, -1, 0, 0, & aa_gun_seat, NULL };
											// spr, scale, zoom, spd,accel,rot, hp, proj,cool, seat, w, h, flags, sub
static const struct object_base aa_gun = { 6, 1, 1, 0,0,0,75, -1,0, -1, 32, 32, 0, & aa_gun_turret };

// LANDER BOAT
											// spr, off_x,y, ctr_x,y,scale, rot, proj,cool, seat, m, flags, sub, next
static const struct object_sub lander_seat_4 = { -1, 16, -17, 0,0,1, 0, -1,-1, 3, -1, SEAT_IMG, NULL, NULL };
static const struct object_sub lander_seat_3 = { -1, 16, 17, 0,0,1, 0, -1,-1, 2, -1, SEAT_IMG, NULL, & lander_seat_4 };
static const struct object_sub lander_seat_2 = { 3, -28, -14, 0,0,1, 0, 1,1, 1, 1, 0, NULL, & lander_seat_3 };
static const struct object_sub lander_seat_1 = { -1,-28, 14, 0,0,1, 0, 4,20, 0, 0, SEAT_IMG, NULL, & lander_seat_2 };
											// spr, scale, zoom, spd,accel,rot, hp, proj,cool, seat, w, h, flags, sub
static const struct object_base lander = { 9, 1, 1, 30,5,16,75, -1,0, -1, 64, 96, WATERBORNE, & lander_seat_1 };
*/
// GUNBOAT

// BATTLESHIP

// FIGHTER

// BOMBER

// object array w/ all base-objects in it
const struct object_base * const object_root = & human;
/*
const struct object_base * const unit_detail[] = {
	& human
	& jeep,
	& lt_tank,
	& hvy_tank,
	& at_gun,
	& aa_gun,
	& lander
};
*/

#endif
