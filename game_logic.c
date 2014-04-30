/* game_logic.cpp
	used by client or server */

#include "game_logic.h"

void simulategame()
{
}

void init_game()
{
}

void shutdown_game()
{
}

/*
void simulategame()
{
	struct gameobj *objptr=topobject, *drill, *nxobj;
	struct human *humptr=tophuman, *h2;
	projectile *projptr=topproj, *pp2;
	int i, vehid,j,landed;
	int oldx, oldy;
	float dist, tangle2, tx, ty, tx2, ty2;
	unsigned char tangle;

	cashmoney++;
	if (cashmoney > 4) {
		cash[0]-=2;  //upkeep
		cash[1]-=2;
		if (bldloc[0][0] == 8) cash[0]+=4; else if (bldloc[0][0] == 9) cash[1]+=4;
		if (bldloc[1][0] == 8) cash[0]+=4; else if (bldloc[1][0] == 9) cash[1]+=4;
		if (bldloc[2][0] == 8) cash[0]+=4; else if (bldloc[2][0] == 9) cash[1]+=4;
		cashmoney = 0;
	}

	if (cash[0] <=0 || cash[1] <= 0)
	{
		for (i=0; i<MAX_PLAYERS; i++)
		{
			if (players[i].status == 3) {
				undriverhuman(hqock[i % 2]);
				players[i].controller_k = hqock[i % 2];
				players[i].status = 1;
				hqock[i % 2] = 0;
			}
		}

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
		killhuman(0);
		destroyvehicle(0);

		if (cash[0] <= 0)
		{
			globalchat("AXIS VICTORY!");
		} else if (cash[1] <= 0) {
			globalchat("ALLIED VICTORY!");
		}
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
	} else {

		humptr=tophuman;
		while (humptr!=NULL)
		{
			oldx = humptr->x;
			oldy = humptr->y;
			humptr->x+=(int)((float)humptr->speed * sintable[humptr->angle]);
			humptr->y+=(int)((float)humptr->speed * -costable[humptr->angle]);
			if (humptr->x<32) humptr->x=32;
			if (humptr->y<32) humptr->y=32;
			if (humptr->x>3200) humptr->x=3200; // values in pixels!
			if (humptr->y>3200) humptr->y=3200;
			if (humptr->cooldown>0) humptr->cooldown--;
			h2 = tophuman;
			while (h2 != NULL)
			{
				if (humptr != h2 && humhumcollide(humptr,h2))
				{
					humptr->x = oldx; // simply prevent humans from passing through one another.
					humptr->y = oldy;
				}
				h2=h2->next;
			}

			for (i=0; i<15; i++)
			{
				if ((i < 5 || bldloc[i][0] != 0) && bldhumcollide(i,humptr))
				{
					if (bldloc[i][0] == 0 || bldloc[i][0] == 8 || bldloc[i][0] == 9)
						bldloc[i][0] = 8 + humptr->align;
					else
					{
						humptr->x = oldx;
						humptr->y = oldy;
					}
				}
			}
			humptr=humptr->next;
		}

		pp2=topproj;
		while (projptr != NULL)
		{
			oldx = projptr->x;
			oldy = projptr->y;
			projptr->x += (int)((float)projptr->speed*sintable[projptr->angle]);
			projptr->y += (int)((float)projptr->speed*-costable[projptr->angle]);

			// let's see if we hit any humans.  Point collision with a circle: use distance formula.
			humptr=tophuman;
			while (humptr!=NULL)
			{
				if (projptr->type < 6 || projptr->type > 12) {
					if ((projptr->align != humptr->align) && bulhumcollide(humptr,oldx,oldy,projptr->x,projptr->y))
					{
						if (projptr->type == 0 || projptr->type == 3 || projptr->type == 14)
						{
							humptr->health-=10;
							projptr->life=1;
						} else if (projptr->type == 2 || projptr->type == 15) {
							humptr->health-=75;
							projptr->life=1;
						} else {
							humptr->health-=100;
							projptr->life=1;
							createfx(projptr->x, projptr->y,1);
						}
					}
				}else if (projptr->type < 8) {
					if (humptr->weapon != 8 && (projptr->align == humptr->align) && bulhumcollide(humptr,oldx,oldy,projptr->x,projptr->y))
					{
						if (projptr->type == 6)
						{
							humptr->health=100;
							projptr->life=1;
						} else { // type = 8
							humptr->ammo=20;
							projptr->life=1;
						}
					}
				} else {
					if (bulhumcollide(humptr,oldx,oldy,projptr->x,projptr->y))
					{
						humptr->carriedarms = projptr->type - 4;
						humptr->weapon = humptr->carriedarms;
						humptr->ammo = 20;
						projptr->life=1;
					}
				}

				humptr=humptr->next;
			}

			objptr=topobject;
			while (objptr!=NULL)
			{
				if (projptr->type < 6 || projptr->type > 12) {
					if ((projptr->align != objptr->align) && bulvehcollide(objptr,oldx,oldy,projptr->x,projptr->y,projptr->type))
					{
						if (projptr->type == 0 || projptr->type == 3 || projptr->type == 2)
						{
							objptr->hp-=3; // pistol or machine gun bullets are not too effective
							projptr->life=1;
						} else if (projptr->type == 14) { // special case for aa gun
							if (objptr->plane == 1) objptr->hp-=15; else objptr->hp-=5;
							projptr->life = 1;
						} else if (projptr->type == 15) {
							objptr->hp-=50; // dgun bullets are pretty tough
							projptr->life=1;
						} else if (projptr->type == 16) {
							if (objptr->type == 21 || objptr->type == 31) objptr->hp-=40; else // bombs must be weakened against ships
								objptr->hp-=100;
							projptr->life=1;
							createfx(projptr->x, projptr->y,1);
						} else { // just blow up.
							objptr->hp-=100;
							projptr->life=1;
							createfx(projptr->x, projptr->y,1);
						}
					}
				}

				objptr=objptr->next;
			}

			projptr->life--;
			if (projptr->x < 0 || projptr->y < 0 || projptr->life <= 0)
			{
				if (projptr->x < 0 || projptr->y < 0)
				{
					projptr->x = 6000; // some huge number.
					projptr->y = 6000;
				}
				if (projptr->type == 1 || projptr->type == 4 || projptr->type == 5 || projptr->type == 13 || projptr->type == 16)
					createfx(projptr->x, projptr->y,1);
				if (pp2==projptr) {
					topproj=projptr->next;
					free(projptr);
					pp2=topproj;
					projptr=pp2;
				} else {
					pp2->next=projptr->next;
					free(projptr);
					projptr=pp2->next;
				}
				//		  printf("expired.");
			} else {
				pp2 = projptr;
				projptr=projptr->next;
			}
		}

		objptr = topobject;
		while (objptr != NULL)
		{
			nxobj = objptr->next;
			oldx = objptr->x;
			oldy = objptr->y;
			objptr->x=(int)(objptr->x+(objptr->speed*sintable[objptr->angle]));
			objptr->y=(int)(objptr->y+(objptr->speed*-costable[objptr->angle]));
			if (objptr->speed > 0) objptr->speed--;
			if (objptr->speed < 0) objptr->speed++;
			if (objptr->x < 0) objptr->x=0;
			if (objptr->x > 3200) objptr->x=3200;
			if (objptr->y < 0) objptr->y=0;
			if (objptr->y > 3200) objptr->y=3200;

			//printf("(%d,%d)\n",objptr->x/32,objptr->y/32);
			if (objptr->plane == 1) {
				if (objptr->speed <= 30) {
					if (map[objptr->x / 32][objptr->y / 32] == 0 ) {
						createfx(objptr->x, objptr->y, 255);
						destroyvehicle(objptr->id);
					} else if (map[objptr->x/32][objptr->y/32] == 4) {
						createfx(objptr->x, objptr->y, 1);
						destroyvehicle(objptr->id);
					}
				}
			} else if (objptr->waterborne == 1) {
				if (map[(int)(objptr->x / 32)][(int)(objptr->y / 32)] != 0)
				{
					objptr->x = oldx;
					objptr->y = oldy;
				} else if (map[min(99,max(0,(objptr->x + objptr->height/2 * sintable[objptr->angle]) / 32))]
						[min(99,max(0,(objptr->y + objptr->height/2 * -costable[objptr->angle]) / 32))] != 0) {
					objptr->x = oldx;
					objptr->y = oldy;
				} else if (map[min(99,max(0,(objptr->x - objptr->height/2 * sintable[objptr->angle]) / 32))]
						[min(99,max(0,(objptr->y - objptr->height/2 * -costable[objptr->angle]) / 32))] != 0) {
					objptr->x = oldx;
					objptr->y = oldy;
				}
			} else if (map[objptr->x / 32][objptr->y / 32] == 0 ) {
				createfx(objptr->x, objptr->y, 255);
				destroyvehicle(objptr->id);
			} else {

				humptr = tophuman;
				while (humptr != NULL)
				{
					if (humvehcollide(humptr,objptr)) {
						if (abs(objptr->speed) > 5)
						{
							humptr->health = 0;
							objptr->speed -=5;
						}
						else objptr->speed = 0;
					}
					//		  createfx(humptr->x, humptr->y, 1);
					humptr=humptr->next;
				}
			}

				drill = topobject;
				while (drill != NULL)
				{
					if (drill != objptr && vehvehcollide(drill,objptr))
					{
						if (3 * (abs(objptr->speed) + abs(drill->speed)) > 0) {
						createfx(drill->x,drill->y,254);
						createfx(objptr->x,objptr->y,254);
						drill->hp -= 3*(abs(objptr->speed) + abs(drill->speed));
						objptr->hp -= 3*(abs(objptr->speed) + abs(drill->speed));

						drill->speed = 0;
						objptr->speed = 0;
						objptr->x = oldx;
						objptr->y = oldy;
						}
					}
					drill=drill->next;
				}

				for (i=0; i<15; i++)
				{
					if (bldloc[i][0] != 0 && bldloc[i][0] != 8 && bldloc[i][0] != 9 && bldvehcollide(i,objptr))
					{
						if (3 * abs(objptr->speed) > 0) {
						createfx(objptr->x,objptr->y,254);
						objptr->hp -= 3*(abs(objptr->speed));
						objptr->speed = 0;
						objptr->x = oldx;
						objptr->y = oldy;
						}
					}
				}
			objptr=nxobj;
		}

		recobjmove(topobject);  // does all the angle updating.

		// last apply all new player input
		for (i=0; i<MAX_PLAYERS; i++)
		{
			if (players[i].connected == 1)
			{
				if (players[i].status==0) {
					printf("Player %d is dead and needs a new body.\n",i);
					humptr=(human *) malloc (sizeof (human));
					if (humptr==NULL) { fprintf(stderr,"Memory error!\n"); exit(1); }
					humptr->next=tophuman;
					tophuman=humptr;
					tophuman->id=idnum;
					tophuman->align=i % 2;
					landed=0;
					while (!landed)
					{
						landed = 1;
						if (tophuman->align == 0) {
							tophuman->x=bldloc[3][1]*32 + rand()%400-200;
							tophuman->y=bldloc[3][2]*32 + rand()%400-200;
						} else {
							tophuman->x=bldloc[4][1]*32 + rand()%400-200;
							tophuman->y=bldloc[4][2]*32 + rand()%400-200;
						}
						for (j = 0; j < 15; j++)
						{
							if (bldhumcollide(j,tophuman)) landed=0;
						}
					}
					tophuman->angle=initang[tophuman->align];
					tophuman->torsoangle=0;
					tophuman->speed=0;
					tophuman->weapon=3;
					tophuman->ammo=20;
					tophuman->health=100;
					tophuman->carriedarms=3; //4 + (rand()%3);
					tophuman->cooldown=0;
					players[i].status=1;
					players[i].controller_k=idnum;
					idnum++;
					cash[tophuman->align] -=15;
				} else if (players[i].status==1)
				{
					humptr = locate_human(tophuman,players[i].controller_k);
					if (humptr!=NULL)
					{
						// first of all: are we dead?
						if (humptr->health <= 0) {humptr->health = 0; players[i].status=0;}
						else {
							if (players[i].controls[0]) humptr->angle-=16;
							if (players[i].controls[1]) humptr->angle+=16;
							if (players[i].controls[2]) humptr->speed=15;
							else if (players[i].controls[3]) humptr->speed=-15;
							else humptr->speed=0;

							players[i].zoomlevel=0;
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
		}

		killhuman(0); // hack: remove all humans with 0 health.
		destroyvehicle(0); // hack: remove all vehicles with 0 health.
	}
}
*/
