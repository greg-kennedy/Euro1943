/* This is the server code for Euro1943. */

#include "main.h"

/* The screen surface */
SDL_Surface *screen = NULL;
player players[MAX_PLAYERS];
int connectedplayers;
unsigned char map[MAP_X][MAP_Y];

gameobj *objlib=NULL;

gameobj *topobject=NULL;
human *tophuman = NULL;
human *corpses = NULL;
human *drivers = NULL;
projectile *topproj = NULL;
projectile *newproj = NULL;
fx *topfx = NULL;

UDPsocket sd;       /* Socket descriptor */
UDPpacket *p;       /* Pointer to packet memory */

unsigned long ticks;

int idnum=1;
int objidnum=1;

int bldloc[15][3];
int bldh[12];
int bldw[12];

int cash[2]={500,500};
int cashmoney=0;
unsigned char initang[2];
int mapnum, maxmapnum, mapplays, maxmapplays;

char maps[30][20];

int hqock[2] = {0,0};
int hqobjects[] = {0,3,6,10,13,16,21,31,44,45};
int prices[] = {5,5,25,25,15,20,15,75,175,250,50,50,75,200,400,150,250};

int min (int a, int b)
{
	if (a < b) return a; return b;
}
int max (int a, double b)
{
	if ((double)a > b) return a; return (int)b;
}

void rectreeprint(gameobj *hi, int depth)
{
	while (hi!=NULL)
	{
		printf("%d: %d\n",depth,hi->id);
		if (hi->subobj!=NULL)
			rectreeprint(hi->subobj,depth+1);
		hi=hi->next;
	}
}


void draw ()
{
	SDL_FillRect (screen, NULL, SDL_MapRGB(screen->format,0,0,0));
	SDL_Flip (screen);
}

gameobj * recobjload(FILE *fp)
{
	gameobj *temp, *temp2;
	char name[40];
	int kids=0,i;

	temp = (gameobj*) malloc (sizeof(gameobj));

	fscanf(fp,"%d %s %d %d %d %d %d %d %d %d %d %d %d %c %c %d %d %d %d %f %d %d %d\n",
			&(temp->id),name,&(temp->type), &(temp->x), &(temp->y), &(temp->xoff), &(temp->yoff),
			&(temp->topspeed), &(temp->accel), &(temp->maxang), &(temp->hp),
			&(temp->proj),&(temp->cool),
			&(temp->mtoken), &(temp->ktoken),&(temp->seat),
			&(temp->seatedimg),&(temp->waterborne),&(temp->plane),
			&(temp->zoom),&kids,&(temp->width),&(temp->height));


	temp->subobj=NULL;
	temp->occupied = 0;

	for (i=0;i<kids;i++)
	{
		temp2=temp->subobj;
		temp->subobj=recobjload(fp);
		temp->subobj->parent=temp;
		temp->subobj->next=temp2;
	}
	printf("Successfully loaded object %s.\n",name);

	temp->next=NULL;
	temp->parent=NULL;

	return temp;
}

void loadobjects()
{
	FILE* fp;
	gameobj *to=NULL;
	int i;

	fp=fopen("obj.txt","r");
	if (fp==NULL)
	{
		fprintf(stderr,"Couldn't open object library!\n");
		exit(1);
	}

	while(!feof(fp))
	{
		if (objlib!=NULL)
		{
			to=objlib;
			objlib=recobjload(fp);
			objlib->next=to;
		} else {
			objlib=recobjload(fp);
		}
	}
	fclose(fp);

	fp=fopen("bldg.txt","r");
	if (fp==NULL)
	{
		fprintf(stderr,"Couldn't open bldg-size library!\n");
		exit(1);
	}
	for (i=0; i<12; i++)
	{
		fscanf(fp,"%d %d\n", &bldw[i], &bldh[i]);
	}
	fclose(fp);

	//rectreeprint(objlib,0);
}

int main (int argc, char *argv[])
{
	int done, i;
	FILE *fp;
	gameobj *objptr;
	char msg[50];

	int OS_PORT;
	char OS_LOC[80];

#ifdef DO_GUI
	if (SDL_Init (SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0)
#else
		if (SDL_Init (SDL_INIT_TIMER) < 0)
#endif
		{
			fprintf (stderr, "Couldn't initialize SDL: %s\n", SDL_GetError ());
			exit (1);
		}
	atexit (SDL_Quit);

#ifdef DO_GUI
	screen = SDL_SetVideoMode (150, 150, 0, SDL_SWSURFACE);
	if (screen == NULL)
	{
		fprintf (stderr, "Couldn't set 150x150 video mode: %s\n", SDL_GetError ());
		exit (2);
	}
	SDL_WM_SetCaption ("Euro1943 server", NULL);
#endif

	if (SDLNet_Init() < 0)
	{
		fprintf(stderr, "SDLNet_Init: %s\n", SDLNet_GetError());
		exit(EXIT_FAILURE);
	}

	/* Open a socket */
	if (!(sd = SDLNet_UDP_Open(PORT)))
	{
		fprintf(stderr, "SDLNet_UDP_Open: %s\n", SDLNet_GetError());
		exit(EXIT_FAILURE);
	}

	if (!(p = SDLNet_AllocPacket(512)))
	{
		fprintf(stderr, "SDLNet_AllocPacket: %s\n", SDLNet_GetError());
		exit(EXIT_FAILURE);
	}

	fp = fopen("server.ini","r");
	if (fp == NULL)
	{
		strcpy(OS_LOC,"nwserver.ath.cx");
		OS_PORT=5009;
		strcpy(maps[0],"x-isle.map");
		maxmapnum=1;

		fp = fopen("server.ini","w");
		if (fp != NULL)
		{
			fprintf(fp,"%s\n%d\n%s\n",OS_LOC,OS_PORT,maps[0]);
			fclose(fp);
		}
	} else {
		fscanf(fp,"%s\n%d\n",OS_LOC,&OS_PORT);
		maxmapnum=0;
		while (!feof(fp))
		{
            fscanf(fp,"%s",maps[maxmapnum]);
            maxmapnum++;
        }
		fclose(fp);
	}

	initgame();
	ReportToMetaserver(OS_LOC,OS_PORT,PORT,5);

	done = 0;
	ticks = SDL_GetTicks() + UPDATEFREQ ;
	while (!done)
	{
		while (!done && ticks > SDL_GetTicks())
		{
			done = checkforquit();
			checkforpacket();
#ifdef DO_GUI
			draw ();
#endif
			SDL_Delay (1);
		}
		ticks = SDL_GetTicks() + UPDATEFREQ ;
		//updateplayers();
		simulategame();
		updateplayers();

		for (i=0; i<MAX_PLAYERS; i++)
		{
			if (players[i].connected == 1 && ticks - players[i].lastup > 3000)
			{
				printf("Player in slot %d dropped (TIMEOUT).\n",i);
				if (players[i].status == 1)
					killhuman(players[i].controller_k);
				else if (players[i].status == 2)
				{
					objptr = locate_object(topobject, players[i].vehid);
					undriverhuman(objptr->occupied);
					players[i].controller_k = objptr->occupied;
					objptr->occupied=0;
					killhuman(players[i].controller_k);
				} else if (players[i].status == 3) {
					undriverhuman(hqock[i%2]);
					players[i].controller_k = hqock[i%2];
					killhuman(players[i].controller_k);
					hqock[i%2]=0;
				}
				sprintf(msg,"Player %d (%s) dropped.",i,(i%2?"axis":"allies"));
				globalchat(msg);
				connectedplayers--;
				players[i].connected=0;
				players[i].status=0;
				i=MAX_PLAYERS;
			}
		}

		/* Draw to screen */
	}

	UnReportToMetaserver();

	while (topobject != NULL)
	{
		objptr = topobject->next;
		recdv(topobject);
		topobject = objptr;
	}
	while (objlib != NULL)
	{
		objptr = objlib->next;
		recdv(objlib);
		objlib = objptr;
	}
	human *thum;
	while (tophuman != NULL)
	{
		thum = tophuman->next;
		free(tophuman);
		tophuman=thum;
	}
	while (drivers != NULL)
	{
		thum = drivers->next;
		free(drivers);
		drivers=thum;
	}
	projectile *tpro;
	while (topproj != NULL)
	{
		tpro = topproj->next;
		free(topproj);
		topproj=tpro;
	}


	SDLNet_FreePacket(p);
	SDLNet_Quit();
	SDL_Quit();
	return 0;
}

int checkforquit()
{
	SDL_Event event;

	int done=0;

	/* Check for events */
	while (SDL_PollEvent (&event))
	{
		switch (event.type)
		{
			case SDL_QUIT:
				done = 1;
				break;
			default:
				break;
		}
	}
	return done;
}

void initgame()
{
	int i;

	connectedplayers=0;
	for (i=0; i<MAX_PLAYERS; i++)
	{
		players[i].connected=0;
		players[i].status=0;
	}

	if (loadmap(maps[0]))
	{
		printf("Error loading map %s!\n",maps[0]);
		exit(1);
	}
	loadobjects();
	
	mapnum = 0;
	mapplays = 0;
	maxmapplays = 4;
}

int loadmap(const char *name)
{
	int x,y;
	unsigned char in;
	FILE *fp;

	char full_map_name[40];
	sprintf(full_map_name,"maps/%s",name);

	fp = fopen(full_map_name,"r");
	if (fp != NULL)
	{
		for (y=0; y<MAP_Y; y++)
			for (x=0; x<MAP_X; x+=2)
			{
				in = fgetc(fp);
				map[x][y] = in >> 4;
				map[x+1][y] = in && 0x0F;
			}

		for (y=0; y<15; y++)
			for (x=0; x<3; x++)
				bldloc[y][x]=(int)fgetc(fp);
				
		initang[0] = (unsigned char)fgetc(fp);
		initang[1] = (unsigned char)fgetc(fp);


		fclose (fp);
		return 0;
	} else {
		return 1;
	}
}

void checkforpacket()
{
	int i, j;
	gameobj *objptr;
	char msg[50];

	while (SDLNet_UDP_Recv(sd, p))
	{
		/*	   printf("Got a packet, length %d:\n",p->len);
			   for (i=0; i<p->len; i++)
			   {
			   printf("%03d ",p->data[i]);
			   }
			   printf("\n");*/
		if (p->len == 1) // queries
		{
			/*			if (p->data[0]=='R') {// overserver request...
						strncpy((char *)p->data,"Euro1943 Server",15);
						p->len=16;
						SDLNet_UDP_Send(sd,-1,p);
						} else */ if (p->data[0]=='L') {// login request...
							if (connectedplayers>=MAX_PLAYERS) { // server full!
								printf("Player tried to connect, but we're full!\n");
								p->data[0]='F';
								SDLNet_UDP_Send(sd,-1,p);
							} else {
								for (i=0; i<MAX_PLAYERS; i++)
								{
									if (players[i].connected==0)
									{
										connectedplayers++;
										players[i].connected=1;
										players[i].addr.host = p->address.host;
										players[i].addr.port = p->address.port;
										players[i].status=0;
										printf("Player connected to slot %d.\n",i);

                                        sprintf((char *)p->data,"M%s",maps[mapnum]);
                                        p->len = 2+strlen(maps[mapnum]);
			                            p->address.host=players[i].addr.host;
                                        p->address.port=players[i].addr.port;
    			                        SDLNet_UDP_Send(sd,-1,p);
					                    players[i].lastup = ticks;

//                                        for(j=0;j<MAX_PLAYERS;j++)
    									sprintf(msg,"Player %d (%s) joined.",i,(i%2?"axis":"allies"));
										globalchat(msg);
										i=MAX_PLAYERS;
									}
								}
							}
						} else if (p->data[0]=='D') {// drop request...
							for (i=0; i<MAX_PLAYERS; i++)
							{
								if (players[i].addr.host == p->address.host &&
										players[i].addr.port == p->address.port)
								{
									printf("Player in slot %d quit (nicely).\n",i);
									if (players[i].status == 1)
										killhuman(players[i].controller_k);
									else if (players[i].status == 2)
									{
										objptr = locate_object(topobject, players[i].vehid);
										undriverhuman(objptr->occupied);
										players[i].controller_k = objptr->occupied;
										objptr->occupied=0;
										killhuman(players[i].controller_k);
									} else if (players[i].status == 3) {
										undriverhuman(hqock[i%2]);
										players[i].controller_k = hqock[i%2];
										killhuman(players[i].controller_k);
										hqock[i%2]=0;
									}
									sprintf(msg,"Player %d (%s) quit.",i,(i%2?"axis":"allies"));
									globalchat(msg);
									connectedplayers--;
									players[i].connected=0;
									players[i].status=0;
									i=MAX_PLAYERS;
								}
							}
						} else {
							printf("I don't know what to do with this length 1 packet 0x%x.\n",p->data[0]);
						}
		} else if (p->len==3) {
			// First, is this player even connected?
			for (i=0; i<MAX_PLAYERS; i++)
			{
				if (players[i].connected==1 &&
						players[i].addr.host == p->address.host &&
						players[i].addr.port == p->address.port)
				{
					//             printf("Player %d is changing %d to %d.\n",i,p->data[0],p->data[1]);
					players[i].controls[0] = p->data[0] & 63;
					players[i].controls[4] = (p->data[0] & 64) != 0;
					players[i].controls[6] = (p->data[0] & 128) != 0;
					players[i].controls[5] = p->data[1];
					players[i].controls[8] = p->data[2];
					players[i].lastup = ticks;
					i=MAX_PLAYERS;
				}
			}
			//       printf("Got a 2 byte packet.\n");
		} else if (p->len==2) {
			// First, is this player even connected?
			for (i=0; i<MAX_PLAYERS; i++)
			{
				if (players[i].connected==1 &&
						players[i].addr.host == p->address.host &&
						players[i].addr.port == p->address.port)
				{
					//             printf("Player %d is changing %d to %d.\n",i,p->data[0],p->data[1]);
					for (j=0; j<8; j++)
						players[i].controls[j] = (p->data[0] >> j) & 1;
					players[i].controls[8] = p->data[1];

					players[i].lastup = ticks;

					i=MAX_PLAYERS;
				}
			}
			//       printf("Got a 2 byte packet.\n");
		} else {
			for (i=0; i<MAX_PLAYERS; i++)
			{
				if (players[i].connected==1 &&
						players[i].addr.host == p->address.host &&
						players[i].addr.port == p->address.port)
				{
					if (p->data[0] == 'C')
					{
						snprintf(msg,48,"G(%d) %s",i,(char *)&p->data[1]);
						globalchat(msg);
					}else if (p->data[0] == 'T'){
						snprintf(msg,48,"T(%d) %s",i,(char *)&p->data[1]);
						teamchat(msg,i % 2);
					}else
						printf("I didn't understand this %d length packet from %d.\n",p->len,i);

					i=MAX_PLAYERS;
				}
			}
			printf("Got a packet size %d, what now?\n",p->len);
		}
	}
}

void recobjmove(gameobj * temp)
{
	while (temp!=NULL)
	{
		temp->angle=temp->angle+temp->anglediff;
		temp->anglediff = 0;

		if (temp->coolleft>0) temp->coolleft--;

		recobjmove(temp->subobj);

		temp=temp->next;
	}
}

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
				//	      printf("expired.");
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
					//	      createfx(humptr->x, humptr->y, 1);
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
									//                                                  printf("%d entered HQ!\n",i);
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
							//           humptr->anglediff=players[i].controls[9]-humptr->angle;
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
						//		      printf("There is NO ESCAPE.\n");
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

void updateplayers()
{
	int i, pindex;
	human *humptr;
	projectile *pptr;
	fx *tfx;
	gameobj *go;

	for(i=0;i<MAX_PLAYERS;i++)
	{
		if (players[i].connected==1)
		{
			p->len=13;

			p->address.host=players[i].addr.host;
			p->address.port=players[i].addr.port;

			humptr = locate_human(tophuman,players[i].controller_k);

			/* here we stuff important info about the player. */
			p->data[0] = players[i].status;
			if (players[i].status == 2)
			{
				//                printf("players[%d].controller_m = %d",i,players[i].controller_m);
				if (players[i].controller_m != 0) {
					p->data[1] = players[i].controller_m / 256;
					p->data[2] = players[i].controller_m % 256;
				} else {
					p->data[1] = players[i].vehid / 256;
					p->data[2] = players[i].vehid % 256;
				}
				go = locate_object(topobject, players[i].vehid);
				while (go->parent != NULL) go = go->parent;
				p->data[4] = (unsigned char) go->hp;
				p->data[5] = 0;
				if (go->type == 21 || go->type == 31) players[i].zoomlevel = 100 + abs(go->speed); else
					players[i].zoomlevel = abs(go->speed);
				p->data[3] = players[i].zoomlevel;
			} else if (players[i].status == 1) {
				p->data[1] = players[i].controller_k / 256;
				p->data[2] = players[i].controller_k % 256;
				if (humptr!=NULL)
				{
					p->data[4] = (unsigned char) humptr->health;
					p->data[5] = humptr->ammo;
				}
				p->data[3] = players[i].zoomlevel;
			} //else if (players[i].status == 3) {
				p->data[9] = cash[0] / 256;
				p->data[10] = cash[0] % 256;
				p->data[11] = cash[1] / 256;
				p->data[12] = cash[1] % 256;
//				p->data[5] = 0;
			//}

			p->data[6] = (unsigned char) bldloc[0][0];
			p->data[7] = (unsigned char) bldloc[1][0];
			p->data[8] = (unsigned char) bldloc[2][0];

			pindex = p->len;
			p->data[pindex]=0;
			p->len++;

			humptr = tophuman;
			while (humptr != NULL)
			{
				p->data[p->len] = humptr->id / 256;
				p->data[p->len+1] = humptr->id % 256;
				p->data[p->len+2] = (int)humptr->x / 256;
				p->data[p->len+3] = (int)humptr->x % 256;
				p->data[p->len+4] = (int)humptr->y / 256;
				p->data[p->len+5] = (int)humptr->y % 256;
				p->data[p->len+6] = (humptr->align << 7 ) | ((humptr->speed == 15) << 5) | ((humptr->speed == -15) << 4) | (humptr->weapon);
				p->data[p->len+7] = humptr->torsoangle;
				p->data[p->len+8] = humptr->angle;
				humptr=humptr->next;
				p->data[pindex]++;
				p->len += 9;
			}

			pindex = p->len;
			p->data[pindex]=0;
			p->len++;

			pptr = newproj;
			while (pptr != NULL)
			{
				p->data[p->len] = pptr->x / 256;
				p->data[p->len+1] = pptr->x % 256;
				p->data[p->len+2] = pptr->y / 256;
				p->data[p->len+3] = pptr->y % 256;
				p->data[p->len+4] = pptr->angle;
				p->data[p->len+5] = pptr->speed / 10;
				p->data[p->len+6] = pptr->life;
				p->data[p->len+7] = (pptr->align << 7) | (pptr->type & 0x7F);
				pptr=pptr->next;
				p->data[pindex]++;
				p->len += 8;
				//		   printf("I'm telling player about new projectile.\n");
			}

			pindex = p->len;
			p->data[pindex]=0;
			p->len++;

			humptr = corpses;
			while (humptr != NULL)
			{
				p->data[p->len] = humptr->id / 256;
				p->data[p->len+1] = humptr->id % 256;
				humptr=humptr->next;
				p->data[pindex]++;
				p->len += 2;
			}

			pindex = p->len;
			p->data[pindex]=0;
			p->len++;

			tfx = topfx;
			while (tfx != NULL)
			{
				p->data[p->len] = tfx->x / 256;
				p->data[p->len+1] = tfx->x % 256;
				p->data[p->len+2] = tfx->y / 256;
				p->data[p->len+3] = tfx->y % 256;
				p->data[p->len+4] = tfx->type;
				tfx=tfx->next;
				p->data[pindex]++;
				p->len += 5;
			}

			// do the object updating here!
			pindex = p->len;
			p->data[pindex]=0;
			p->len++;

			go = topobject;
			while (go != NULL)
			{
				p->data[p->len] = go->id / 256;
				p->data[p->len+1] = go->id % 256;
				p->data[p->len+2] = go->x / 256;
				p->data[p->len+3] = go->x % 256;
				p->data[p->len+4] = go->y / 256;
				p->data[p->len+5] = go->y % 256;
				p->data[p->len+6] = go->speed;
				p->data[p->len+7] = (go->type & 0x3F);
				p->len += 8;
				appendangles(go);
				go=go->next;
				p->data[pindex]++;
			}

			SDLNet_UDP_Send(sd,-1,p);
		}
	}

	//move everything from new to existing. 
	while (newproj != NULL)
	{
		pptr = newproj;
		newproj=newproj->next;
		pptr->next=topproj;
		topproj=pptr;
	}
	newproj = NULL;

	while (corpses != NULL)
	{
		humptr = corpses->next;
		free(corpses);
		corpses=humptr;
	}
	corpses=NULL;

	while (topfx != NULL)
	{
		tfx = topfx->next;
		free(topfx);
		topfx=tfx;
	}
	topfx=NULL;
}

gameobj *locate_object(gameobj *start, unsigned int target)
{
	gameobj *t, *t2=NULL;

	if (start==NULL) return NULL;
	else if (target == start->id) return start;
	else
	{
		t=start->subobj;
		while (t!=NULL && t2==NULL)
		{
			t2=locate_object(t, target);
			if (t2==NULL) t=t->next;
		}
		if (t2!=NULL) return t2;
		else return locate_object(start->next, target);
	}
}

human *locate_human(human *start, unsigned int target)
{
	while (start!=NULL)
	{
		if (start->id == target) return start;
		start=start->next;
	}
	return NULL;
}

void createbullet(unsigned char align,int x,int y,unsigned char angle, unsigned int speed, unsigned char type, unsigned char life)
{
	projectile *np;
	np = (projectile *)malloc(sizeof(projectile));
	np->next=newproj;
	newproj=np;
	np->align=align;
	np->x=x;
	np->y=y;
	np->angle=angle;
	np->speed=speed;
	np->type=type;
	np->life=life;
}

void killhuman(unsigned int id) // sorts corpses and humans
{
	int i;
	human *target = tophuman, *temp;
	tophuman = NULL;
	char msg[50];

	while (target != NULL)
	{
		if (target->id == id || target->health == 0)
		{
			if (target->health == 0)
			{
				for (i=0;i<MAX_PLAYERS;i++)
					if (players[i].controller_k == target->id) break;
				sprintf(msg,"Player %d (%s) died!",i,(target->align?"axis":"allies"));
				globalchat(msg);
			}
			//               if (target->health == 0)
			//fprintf (stderr, "Got a guy with 0 health (%d).  KILL\n",target->id);
			temp = target->next;
			target->next = corpses;
			corpses = target;
			target = temp;
		} else {
			temp = target->next;
			target->next = tophuman;
			tophuman = target;
			target = temp;
		}
	}
}

void createfx (int x, int y, unsigned char type)
{
	fx *nf;
	nf = (fx *)malloc(sizeof(struct fx));
	nf->x=x;
	nf->y=y;
	nf->type=type;
	nf->next=topfx;
	topfx=nf;
}

void globalchat(const char *message)
{
	int i;
	sprintf((char *)p->data,"C%s",message);
	p->len = 2+strlen(message);
	for(i=0;i<MAX_PLAYERS;i++)
	{
		if (players[i].connected==1)
		{
			p->address.host=players[i].addr.host;
			p->address.port=players[i].addr.port;
			SDLNet_UDP_Send(sd,-1,p);
		}
	}
}

void teamchat(const char *message, int team)
{
	int i;
	sprintf((char *)p->data,"C%s",message);
	p->len = 2+strlen(message);
	for(i=team;i<MAX_PLAYERS;i+=2)
	{
		if (players[i].connected==1)
		{
			p->address.host=players[i].addr.host;
			p->address.port=players[i].addr.port;
			SDLNet_UDP_Send(sd,-1,p);
		}
	}
}

void createobject (int type, int x, int y, unsigned char align)
{
	gameobj *temp;
	gameobj *src = locate_object(objlib,type);
	//	fprintf(stderr,"I located type %d.\n",type);
	if (src != NULL)
	{
		temp = createobjfromlib(src,NULL, align);
		//		fprintf(stderr,"I created...\n");
		temp->x = x;
		temp->y = y;
		temp->align = align;
		temp->next = topobject;
		topobject = temp;
		printf("Spawned a new type %d at %d, %d (id %d).\n",type,x,y,temp->id);
	} else {
		printf("Warning: object %d not found in library!\n",type);
	}
}

gameobj *createobjfromlib (gameobj *src, gameobj *parent, unsigned char align)
{
	gameobj *dest = NULL;
	gameobj *kids = NULL, *temp;

	dest = (gameobj *) malloc (sizeof (struct gameobj));
	//	fprintf(stderr,"I malloc'd...\n");
	//	strcpy(dest->name,src->name);
	dest->type=src->id;
	//	fprintf(stderr,"I set type...\n");

	dest->id=objidnum;
	objidnum++;

	//	fprintf(stderr,"I set ID num...\n");
	dest->x=src->x;
	dest->y=src->y;
	dest->xoff=src->xoff;
	dest->yoff=src->yoff;
	if (parent == NULL) dest->angle=initang[align]; else dest->angle = 0;
	dest->topspeed=src->topspeed;
	dest->accel=src->accel;
	dest->maxang=src->maxang;
	dest->hp=src->hp;
	dest->proj=src->proj;
	dest->cool=src->cool;
	dest->mtoken=src->mtoken;
	dest->ktoken=src->ktoken;
	dest->seat=src->seat;
	dest->seatedimg=src->seatedimg;
	dest->waterborne=src->waterborne;
	dest->plane=src->plane;
	dest->zoom=src->zoom;
	dest->width=src->width;
	dest->height=src->height;
	kids = src->subobj;

	dest->coolleft=0;
	dest->align=align;
	dest->speed=0;
	dest->anglediff=0;
	dest->occupied=0;
	dest->subobj=NULL;
	dest->parent=parent;

	while (kids!=NULL)
	{
		temp = createobjfromlib(kids,dest,align);
		temp->next = dest->subobj;
		dest->subobj=temp;
		kids=kids->next;
	}
	return dest;
}

void appendangles(gameobj *go)
{
	gameobj *kid;
	if (go != NULL)
	{
		p->data[p->len]=go->angle;
		if (go->parent == NULL)
			p->data[p->len+1]=(go->align << 7) | (go->anglediff & 0x7F);
		else
			p->data[p->len+1]=(go->occupied>0 ? 0x80 : 0) | (go->anglediff & 0x7F);
		p->len+=2;
		kid = go->subobj;
		while (kid != NULL)
		{
			appendangles(kid);
			kid=kid->next;
		}
	}
}

int enterveh(human *humptr)
{
	int i;
	gameobj *start = topobject;

	while (start != NULL)
	{
		if (humptr->align == start->align && sqrt( pow(humptr->x - start->x,2) + pow(humptr->y - start->y,2)) < 128) //set some appropriate dist.
		{
			//               		fprintf (stderr, "There's a thing in range, id %d\n",start->id);
			i = seatsearch(humptr,start);
			if (i !=0) return i;
		}
		start = start->next;
	}
	//        fprintf (stderr, "There's nothing in range!\n");
	return 0;
}

int seatsearch (human *humptr, gameobj *start)
{
	int i;
	gameobj *temp;
	if (start==NULL)
	{
		//               fprintf (stderr, "No seats here.\n");
		return 0;
	} else if (start->seat && !start->occupied) {
		//               fprintf (stderr, "Got an empty seat for %d, id is %d.\n",humptr->id, start->id);
		start->occupied = humptr->id;
		driverhuman(humptr->id);
		return start->id;
	} else {
		//               fprintf (stderr, "No seats here, checking the subobjects.\n");
		temp = start->subobj;
		while (temp != NULL)
		{
			i=seatsearch(humptr,temp);
			if (i != 0) return i;
			temp=temp->next;
		}
		return 0;
	}
}

void undriverhuman(unsigned int id) // re-sorts drivers and humans
{
	human *target = drivers, *temp;
	drivers = NULL;
	//     fprintf (stderr, "undriving (%d).\n",target->id);

	while (target != NULL)
	{
		if (target->id == id)
		{
			//               fprintf (stderr, "Got a driver, undriving (%d).\n",target->id);
			temp = target->next;
			target->next = tophuman;
			tophuman = target;
			target = temp;
		} else {
			//               fprintf(stderr,"%d isn't %d.\n",target->id,id);
			temp = target->next;
			target->next = drivers;
			drivers = target;
			target = temp;
		}
	}
}

void driverhuman(unsigned int id) // sorts drivers and humans
{
	human *target = tophuman, *temp;
	tophuman = NULL;

	while (target != NULL)
	{
		if (target->id == id)
		{
			//               fprintf (stderr, "Got a driver, sorting (%d).\n",target->id);
			temp = target->next;
			target->next = drivers;
			drivers = target;
			target = temp;
		} else {
			temp = target->next;
			target->next = tophuman;
			tophuman = target;
			target = temp;
		}
	}
}

int checkcontrol (unsigned int number, char token)
{
	gameobj *obj;
	obj = locate_object(topobject, number);
	//printf("Player got in %d, controller type %c is ", number, token);
	if (token == 'M') token = obj->mtoken;
	else if (token == 'K') token = obj->ktoken;

	//printf("%c\n",token);
	if (token == 'N') return 0;
	if (token == 'B') return obj->id;
	if (token == 'P') return obj->parent->id;
	if (token == 'Q') return obj->parent->parent->parent->id;
	return 0;
}

void destroyvehicle(unsigned int id)
{
	// this destroys a vehicle, kills players and drivers, fixes status, etc.
	gameobj *target = topobject, *temp;
	topobject = NULL;

	while (target != NULL)
	{
		if (target->hp <=0)
			createfx(target->x,target->y,1);
		if (target->id == id || target->hp <= 0)
		{

			//               fprintf (stderr, "Got a driver, sorting (%d).\n",target->id);
			temp = target;
			target=target->next;
			recdv(temp);
		} else {
			temp = target->next;
			target->next = topobject;
			topobject = target;
			target = temp;
		}
	}
}

int playersearch(unsigned int id)
{
	int i;
	for (i=0; i<MAX_PLAYERS; i++)
		if (players[i].vehid == id) return i;
	return MAX_PLAYERS;
}

void recdv (gameobj *veh)
{
	gameobj *sub = veh->subobj, *t;
	while (sub != NULL)
	{
		t = sub->next;
		recdv(sub);
		sub=t;
	}
	if (veh->occupied != 0) {
		players[playersearch(veh->id)].status=0;
		undriverhuman(veh->occupied);
		killhuman(veh->occupied);
	}
	free(veh);
}

int humhumcollide (human *v1, human *v2)
{
	POLYGON p1;
	POLYGON p2;
	p1.NumSides = 4;
	p2.NumSides = 4;
	p1.Center.x = (double)v1->x;
	p1.Center.y = (double)v1->y;
	p2.Center.x = (double)v2->x;
	p2.Center.y = (double)v2->y;

	p1.Points[0].x = -16;
	p1.Points[0].y = 16;
	p1.Points[1].x = 16;
	p1.Points[1].y = 16;
	p1.Points[2].x = 16;
	p1.Points[2].y = -16;
	p1.Points[3].x = -16;
	p1.Points[3].y = -16;

	p2.Points[0].x = -16;
	p2.Points[0].y = 16;
	p2.Points[1].x = 16;
	p2.Points[1].y = 16;
	p2.Points[2].x = 16;
	p2.Points[2].y = -16;
	p2.Points[3].x = -16;
	p2.Points[3].y = -16;

	return polypolyCollision(&p1,&p2);
}

int humvehcollide (human *v1, gameobj *v2)
{
	POLYGON p1;
	POLYGON p2;
	double dist, w, h;
	double newangle, addangle;

	if (v2->plane == 1 && v2->speed > 30) return 0;
	p1.NumSides = 4;
	p2.NumSides = 4;

	p1.Center.x = v1->x;
	p1.Center.y = v1->y;
	p2.Center.x = v2->x;
	p2.Center.y = v2->y;

	p1.Points[0].x = -16;
	p1.Points[0].y = 16;
	p1.Points[1].x = 16;
	p1.Points[1].y = 16;
	p1.Points[2].x = 16;
	p1.Points[2].y = -16;
	p1.Points[3].x = -16;
	p1.Points[3].y = -16;

	w = (double) v2->width / 2;
	h = (double) v2->height / 2;
	dist = sqrt(w*w+h*h);
	if (v2->plane == 1)
		addangle = (double)(v2->angle + 32) * 0.02463994235294;
	else
		addangle = (double)v2->angle * 0.02463994235294;

	newangle = atan2(h,-w) + addangle;
	p2.Points[0].x = dist * cos(newangle);
	p2.Points[0].y = dist * sin(newangle);
	newangle = atan2(h,w) + addangle;
	p2.Points[1].x = dist * cos(newangle);
	p2.Points[1].y = dist * sin(newangle);
	newangle = atan2(-h,w) + addangle;
	p2.Points[2].x = dist * cos(newangle);
	p2.Points[2].y = dist * sin(newangle);
	newangle = atan2(-h,-w) + addangle;
	p2.Points[3].x = dist * cos(newangle);
	p2.Points[3].y = dist * sin(newangle);

	return polypolyCollision(&p1,&p2);
}

int bldhumcollide (int id, human *v2)
{
	POLYGON p1;
	POLYGON p2;
	p1.NumSides = 4;
	p2.NumSides = 4;
	p1.Center.x = bldloc[id][1] * 32;
	p1.Center.y = bldloc[id][2] * 32;
	p2.Center.x = (double)v2->x;
	p2.Center.y = (double)v2->y;

	p1.Points[0].x = -bldw[bldloc[id][0]];
	p1.Points[0].y = bldh[bldloc[id][0]];
	p1.Points[1].x = bldw[bldloc[id][0]];
	p1.Points[1].y = bldh[bldloc[id][0]];
	p1.Points[2].x = bldw[bldloc[id][0]];
	p1.Points[2].y = -bldh[bldloc[id][0]];
	p1.Points[3].x = -bldw[bldloc[id][0]];
	p1.Points[3].y = -bldh[bldloc[id][0]];

	p2.Points[0].x = -16;
	p2.Points[0].y = 16;
	p2.Points[1].x = 16;
	p2.Points[1].y = 16;
	p2.Points[2].x = 16;
	p2.Points[2].y = -16;
	p2.Points[3].x = -16;
	p2.Points[3].y = -16;

	return polypolyCollision(&p1,&p2);
}

int bldvehcollide (int id, gameobj *v2)
{
	POLYGON p1;
	POLYGON p2;
	double dist, w, h;
	double newangle, addangle;
	if (v2->plane == 1 && v2->speed > 30) return 0;

	p1.NumSides = 4;
	p2.NumSides = 4;

	p1.Center.x = bldloc[id][1] * 32;
	p1.Center.y = bldloc[id][2] * 32;
	p2.Center.x = v2->x;
	p2.Center.y = v2->y;

	p1.Points[0].x = -bldw[bldloc[id][0]];
	p1.Points[0].y = bldh[bldloc[id][0]];
	p1.Points[1].x = bldw[bldloc[id][0]];
	p1.Points[1].y = bldh[bldloc[id][0]];
	p1.Points[2].x = bldw[bldloc[id][0]];
	p1.Points[2].y = -bldh[bldloc[id][0]];
	p1.Points[3].x = -bldw[bldloc[id][0]];
	p1.Points[3].y = -bldh[bldloc[id][0]];

	w = (double) v2->width / 2;
	h = (double) v2->height / 2;
	dist = sqrt(w*w+h*h);
	if (v2->plane == 1)
		addangle = (double)(v2->angle + 32) * 0.02463994235294;
	else
		addangle = (double)v2->angle * 0.02463994235294;

	newangle = atan2(h,-w) + addangle;
	p2.Points[0].x = dist * cos(newangle);
	p2.Points[0].y = dist * sin(newangle);
	newangle = atan2(h,w) + addangle;
	p2.Points[1].x = dist * cos(newangle);
	p2.Points[1].y = dist * sin(newangle);
	newangle = atan2(-h,w) + addangle;
	p2.Points[2].x = dist * cos(newangle);
	p2.Points[2].y = dist * sin(newangle);
	newangle = atan2(-h,-w) + addangle;
	p2.Points[3].x = dist * cos(newangle);
	p2.Points[3].y = dist * sin(newangle);

	return polypolyCollision(&p1,&p2);
}

int vehvehcollide (gameobj *v1, gameobj *v2)
{
	POLYGON p1;
	POLYGON p2;
	double dist, w, h;
	double newangle, addangle;

	if (v1->plane == 1 && v1->speed > 30)
		if (v2->plane == 0 || v2->speed <= 30) return 0;
	if (v2->plane == 1 && v2->speed > 30)
		if (v1->plane == 0 || v1->speed <= 30) return 0;

	p1.NumSides = 4;
	p2.NumSides = 4;

	p1.Center.x = v1->x;
	p1.Center.y = v1->y;
	p2.Center.x = v2->x;
	p2.Center.y = v2->y;

	w = (double) v1->width / 2;
	h = (double) v1->height / 2;
	dist = sqrt(w*w+h*h);
	if (v1->plane == 1)
		addangle = (double)(v1->angle + 32) * 0.02463994235294;
	else
		addangle = (double)v1->angle * 0.02463994235294;

	newangle = atan2(h,-w) + addangle;
	p1.Points[0].x = dist * cos(newangle);
	p1.Points[0].y = dist * sin(newangle);
	newangle = atan2(h,w) + addangle;
	p1.Points[1].x = dist * cos(newangle);
	p1.Points[1].y = dist * sin(newangle);
	newangle = atan2(-h,w) + addangle;
	p1.Points[2].x = dist * cos(newangle);
	p1.Points[2].y = dist * sin(newangle);
	newangle = atan2(-h,-w) + addangle;
	p1.Points[3].x = dist * cos(newangle);
	p1.Points[3].y = dist * sin(newangle);

	w = (double) v2->width / 2;
	h = (double) v2->height / 2;
	dist = sqrt(w*w+h*h);
	if (v2->plane == 1)
		addangle = (double)(v2->angle + 32) * 0.02463994235294;
	else
		addangle = (double)v2->angle * 0.02463994235294;

	newangle = atan2(h,-w) + addangle;
	p2.Points[0].x = dist * cos(newangle);
	p2.Points[0].y = dist * sin(newangle);
	newangle = atan2(h,w) + addangle;
	p2.Points[1].x = dist * cos(newangle);
	p2.Points[1].y = dist * sin(newangle);
	newangle = atan2(-h,w) + addangle;
	p2.Points[2].x = dist * cos(newangle);
	p2.Points[2].y = dist * sin(newangle);
	newangle = atan2(-h,-w) + addangle;
	p2.Points[3].x = dist * cos(newangle);
	p2.Points[3].y = dist * sin(newangle);

	return polypolyCollision(&p1,&p2);
}

int bulvehcollide (gameobj *v1, int oldx, int oldy, int newx, int newy, int type)
{
	POLYGON p1;
	POLYGON p2;
	double dist, w, h;
	double newangle, addangle;

	if (v1->plane == 1 && v1->speed > 30 && (type == 1 || type == 2 || type == 3 || type == 4 ||
				type == 5 || type == 13 || type == 15 || type == 16)) return 0;
//	if (v1->plane == 1 && v1->speed > 30) return 0;

	p1.NumSides = 4;
	p2.NumSides = 2;

	p1.Center.x = v1->x;
	p1.Center.y = v1->y;
	p2.Center.x = oldx;
	p2.Center.y = oldy;

	w = (double) v1->width / 2;
	h = (double) v1->height / 2;
	dist = sqrt(w*w+h*h);
	if (v1->plane == 1)
		addangle = (double)(v1->angle + 32) * 0.02463994235294;
	else
		addangle = (double)v1->angle * 0.02463994235294;

	newangle = atan2(h,-w) + addangle;
	p1.Points[0].x = dist * cos(newangle);
	p1.Points[0].y = dist * sin(newangle);
	newangle = atan2(h,w) + addangle;
	p1.Points[1].x = dist * cos(newangle);
	p1.Points[1].y = dist * sin(newangle);
	newangle = atan2(-h,w) + addangle;
	p1.Points[2].x = dist * cos(newangle);
	p1.Points[2].y = dist * sin(newangle);
	newangle = atan2(-h,-w) + addangle;
	p1.Points[3].x = dist * cos(newangle);
	p1.Points[3].y = dist * sin(newangle);

	p2.Points[0].x = 0;
	p2.Points[0].y = 0;
	p2.Points[1].x = newx-oldx;
	p2.Points[1].y = newy-oldy;

	return polypolyCollision(&p1,&p2);
}

int bulhumcollide (human *v1, int oldx, int oldy, int newx, int newy)
{
	POLYGON p1;
	POLYGON p2;
	p1.NumSides = 4;
	p2.NumSides = 2;

	p1.Center.x = v1->x;
	p1.Center.y = v1->y;
	p2.Center.x = oldx;
	p2.Center.y = oldy;

	p1.Points[0].x = -16;
	p1.Points[0].y = 16;
	p1.Points[1].x = 16;
	p1.Points[1].y = 16;
	p1.Points[2].x = 16;
	p1.Points[2].y = -16;
	p1.Points[3].x = -16;
	p1.Points[3].y = -16;

	p2.Points[0].x = 0;
	p2.Points[0].y = 0;
	p2.Points[1].x = newx-oldx;
	p2.Points[1].y = newy-oldy;

	return polypolyCollision(&p1,&p2);
}


/* Tests for a collision between any two convex polygons.
 * Returns 0 if they do not intersect
 * Returns 1 if they intersect
 */
int polypolyCollision(POLYGON *a, POLYGON *b)
{
	POINT axis;
	double tmp, minA, maxA, minB, maxB;
	int side, i;

	/* test polygon A's sides */
	for (side = 0; side < a->NumSides; side++)
	{
		/* get the axis that we will project onto */
		if (side == 0)
		{
			axis.x = a->Points[a->NumSides - 1].y - a->Points[0].y;
			axis.y = a->Points[0].x - a->Points[a->NumSides - 1].x;
		}
		else
		{
			axis.x = a->Points[side - 1].y - a->Points[side].y;
			axis.y = a->Points[side].x - a->Points[side - 1].x;
		}

		/* normalize the axis */
		tmp = sqrt(axis.x * axis.x + axis.y * axis.y);
		axis.x /= tmp;
		axis.y /= tmp;

		/* project polygon A onto axis to determine the min/max */
		minA = maxA = a->Points[0].x * axis.x + a->Points[0].y * axis.y;
		for (i = 1; i < a->NumSides; i++)
		{
			tmp = a->Points[i].x * axis.x + a->Points[i].y * axis.y;
			if (tmp > maxA)
				maxA = tmp;
			else if (tmp < minA)
				minA = tmp;
		}
		/* correct for offset */
		tmp = a->Center.x * axis.x + a->Center.y * axis.y;
		minA += tmp;
		maxA += tmp;

		/* project polygon B onto axis to determine the min/max */
		minB = maxB = b->Points[0].x * axis.x + b->Points[0].y * axis.y;
		for (i = 1; i < b->NumSides; i++)
		{
			tmp = b->Points[i].x * axis.x + b->Points[i].y * axis.y;
			if (tmp > maxB)
				maxB = tmp;
			else if (tmp < minB)
				minB = tmp;
		}
		/* correct for offset */
		tmp = b->Center.x * axis.x + b->Center.y * axis.y;
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
			axis.x = b->Points[b->NumSides - 1].y - b->Points[0].y;
			axis.y = b->Points[0].x - b->Points[b->NumSides - 1].x;
		}
		else
		{
			axis.x = b->Points[side - 1].y - b->Points[side].y;
			axis.y = b->Points[side].x - b->Points[side - 1].x;
		}

		/* normalize the axis */
		tmp = sqrt(axis.x * axis.x + axis.y * axis.y);
		axis.x /= tmp;
		axis.y /= tmp;

		/* project polygon A onto axis to determine the min/max */
		minA = maxA = a->Points[0].x * axis.x + a->Points[0].y * axis.y;
		for (i = 1; i < a->NumSides; i++)
		{
			tmp = a->Points[i].x * axis.x + a->Points[i].y * axis.y;
			if (tmp > maxA)
				maxA = tmp;
			else if (tmp < minA)
				minA = tmp;
		}
		/* correct for offset */
		tmp = a->Center.x * axis.x + a->Center.y * axis.y;
		minA += tmp;
		maxA += tmp;

		/* project polygon B onto axis to determine the min/max */
		minB = maxB = b->Points[0].x * axis.x + b->Points[0].y * axis.y;
		for (i = 1; i < b->NumSides; i++)
		{
			tmp = b->Points[i].x * axis.x + b->Points[i].y * axis.y;
			if (tmp > maxB)
				maxB = tmp;
			else if (tmp < minB)
				minB = tmp;
		}
		/* correct for offset */
		tmp = b->Center.x * axis.x + b->Center.y * axis.y;
		minB += tmp;
		maxB += tmp;

		/* test if lines intersect, if not, return false */
		if (maxA < minB || minA > maxB)
			return 0;
	}

	return 1;
}

void mapchange (int mapnum)
{
     if (loadmap(maps[mapnum]))
     {
         fprintf(stderr,"Error loading map %s!",maps[mapnum]);
         exit(1);
     }

	int i;
	sprintf((char *)p->data,"M%s",maps[mapnum]);
	p->len = 2+strlen(maps[mapnum]);
	for(i=0;i<MAX_PLAYERS;i++)
	{
		if (players[i].connected==1)
		{
			p->address.host=players[i].addr.host;
			p->address.port=players[i].addr.port;
			SDLNet_UDP_Send(sd,-1,p);
		   }
	}
}
