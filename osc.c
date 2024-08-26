#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL/SDL_net.h>

#include "osc.h"

unsigned int GetMetaserverBlock(char * ip, int port, unsigned char type, int blocknum,
	IPaddress ** iplist)
{
	UDPsocket sd;
	IPaddress srvadd;
	UDPpacket * p;
	unsigned long ticks;
	long i;
	unsigned char * packptr;
	int awaiting_packet;
	int numRetrieved;

	/* Open a socket on random port */
	if (!(sd = SDLNet_UDP_Open(0))) {
		fprintf(stderr, "OverServer ERROR: SDLNet_UDP_Open: %s\n", SDLNet_GetError());
		return 0;
	}

	/* Resolve server name  */
	if (SDLNet_ResolveHost(&srvadd, ip, port)) {
		fprintf(stderr, "OverServer ERROR: SDLNet_ResolveHost(%s %d): %s\n", ip, port, SDLNet_GetError());
		return 0;
	}

	/* Allocate memory for the packet */
	if (!(p = SDLNet_AllocPacket(2048))) { // allow for super-big magic keys
		fprintf(stderr, "OverServer ERROR: SDLNet_AllocPacket: %s\n", SDLNet_GetError());
		return 0;
	}

	p->data[0] = (unsigned char)(blocknum >> 8);
	p->data[1] = (unsigned char)(blocknum);
	p->data[2] = type;
	p->len = 3;
	p->address.host = srvadd.host;	/* Set the destination host */
	p->address.port = srvadd.port;	/* And destination host */
	SDLNet_UDP_Send(sd, -1, p); /* This sets the p->channel */
	// This first loop is for the magic key
	ticks = SDL_GetTicks();
	awaiting_packet = 1;
	numRetrieved = 0;

	while ((awaiting_packet == 1) && (ticks + MAX_WAIT > SDL_GetTicks())) {
		if (SDLNet_UDP_Recv(sd, p)) {
			SDLNet_UDP_Send(sd, -1, p); // send it back.
			awaiting_packet = 0;
		}
	}

	// This next loop gets the block.
	// Obviously if the magic key never arrived neither will the block
	//  so just skip it.
	if (awaiting_packet == 0) {
		ticks = SDL_GetTicks();
		awaiting_packet = 1;

		while ((awaiting_packet == 1) && (ticks + MAX_WAIT > SDL_GetTicks())) {
			if (SDLNet_UDP_Recv(sd, p)) {
				numRetrieved = p->len / 6;
				packptr = p->data;
				*(iplist) = (IPaddress *)malloc(numRetrieved * sizeof(IPaddress));

				for (i = 0; i < numRetrieved; i++) {
					(*(iplist))[i].host = SDLNet_Read32(packptr);
					packptr += 4;
					(*(iplist))[i].port = SDLNet_Read16(packptr);
					packptr += 2;
				}

				awaiting_packet = 0;
			}
		}
	} else
		fprintf(stderr, "OverServer ERROR: Never heard back from remote server\n");

	SDLNet_FreePacket(p);
// test block, makes up 60 servers.
	/*numRetrieved = 60;
					  *(iplist) = (IPaddress *)malloc(numRetrieved * sizeof(IPaddress));
		for (i = 0; i < numRetrieved; i++)
	{
	  (*(iplist))[i].host = i << 24; packptr+=4;
	  (*(iplist))[i].port = i; packptr+=2;
	}*/
	return numRetrieved;
}
