#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL/SDL_net.h>

#include "oss.h"

SDL_TimerID timerID;

unsigned char saved_type;
unsigned short int saved_port;
IPaddress srvadd;

Uint32 SendHeartbeat (Uint32 interval, void *param)
{
    UDPsocket sd;
    UDPpacket *p;
    
	/* Open a socket on random port */
	if (!(sd = SDLNet_UDP_Open(0)))
	{
		fprintf(stderr, "OverServer ERROR: SDLNet_UDP_Open: %s\n", SDLNet_GetError());
		return 0;
	}
	/* Allocate memory for the packet */
	if (!(p = SDLNet_AllocPacket(4))) // small packet...
	{
		fprintf(stderr, "OverServer ERROR: SDLNet_AllocPacket: %s\n", SDLNet_GetError());
		return 0;
	}
 
	/* Main loop */
	p->data[0]='H';
	p->data[1]=saved_type;
	SDLNet_Write16(saved_port,&p->data[2]);
	p->len=4;
	p->address.host = srvadd.host;	/* Set the destination host */
	p->address.port = srvadd.port;	/* And destination host */
	SDLNet_UDP_Send(sd, -1, p); /* This sets the p->channel */

	SDLNet_FreePacket(p);
	
	return interval;
}

int ReportToMetaserver (char *ip, int os_port, unsigned short int srv_port, unsigned char type)
{
    UDPsocket sd;
    UDPpacket *p;
    
    unsigned long ticks;
    
    int awaiting_packet;

	/* Open a socket on random port */
	if (!(sd = SDLNet_UDP_Open(0)))
	{
		fprintf(stderr, "OverServer ERROR: SDLNet_UDP_Open: %s\n", SDLNet_GetError());
		return 1;
	}
	/* Resolve server name and overserver port */
	if (SDLNet_ResolveHost(&srvadd, ip, os_port))
	{
		fprintf(stderr, "OverServer ERROR: SDLNet_ResolveHost(%s %d): %s\n", ip, os_port, SDLNet_GetError());
		return 1;
	}
	/* Allocate memory for the packet */
	if (!(p = SDLNet_AllocPacket(2048))) // allow for super-big magic keys
	{
		fprintf(stderr, "OverServer ERROR: SDLNet_AllocPacket: %s\n", SDLNet_GetError());
		return 1;
	}
 
    saved_type = type; // save the game type for other functions.
    saved_port = srv_port; // save OUR server port.

	p->data[0]='R';
	p->data[1]=saved_type;
	SDLNet_Write16(saved_port,&p->data[2]);
	p->len=4;
	p->address.host = srvadd.host;	/* Set the destination host */
	p->address.port = srvadd.port;	/* And destination host */
	SDLNet_UDP_Send(sd, -1, p); /* This sets the p->channel */

	ticks = SDL_GetTicks();
	awaiting_packet=1;
	while ((awaiting_packet == 1) && (ticks + MAX_WAIT > SDL_GetTicks()))
	{
          if (SDLNet_UDP_Recv(sd,p)) // got our magic packet
          {
              SDLNet_UDP_Send(sd, -1, p); // send it back.
              awaiting_packet = 0;
          }
    }

	SDLNet_FreePacket(p);

    /* If successful, start the timer for automatic re-registration. */
	if (awaiting_packet == 0)
    {
        timerID = SDL_AddTimer(HEARTBEAT_DELAY, SendHeartbeat, NULL);
    }

	return awaiting_packet; // 0 = success registering, 1 = failure
}

void UnReportToMetaserver ()
{
    UDPsocket sd;
    UDPpacket *p;
    
    /* First, kill off the timer. */
    SDL_RemoveTimer(timerID);
    
	/* Open a socket on random port */
	if (!(sd = SDLNet_UDP_Open(0)))
	{
		fprintf(stderr, "OverServer ERROR: SDLNet_UDP_Open: %s\n", SDLNet_GetError());
		return;
	}
	/* Allocate memory for the packet */
	if (!(p = SDLNet_AllocPacket(4))) // small packet.
	{
		fprintf(stderr, "OverServer ERROR: SDLNet_AllocPacket: %s\n", SDLNet_GetError());
		return;
	}
 
	/* Main loop */
	p->data[0]='U';
	p->data[1]=saved_type;
	SDLNet_Write16(saved_port,&p->data[2]);
	p->len=4;
	p->address.host = srvadd.host;	/* Set the destination host */
	p->address.port = srvadd.port;	/* And destination host */
	SDLNet_UDP_Send(sd, -1, p); /* This sets the p->channel */

	SDLNet_FreePacket(p);
}
