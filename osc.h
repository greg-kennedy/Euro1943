/***********************************/
/* The OverServer - Greg Kennedy   */
/***********************************/

/* Usage:  Call GetMetaserverBlock to get a block of IP addresses. */

/* ip is the location of the OverServer.  port is the OverServer port. */
/* type is the gametype.  blocknum is the block of IPs you want to get. */
/* iplist is a pointer of type IPaddress, you must pass address to it, e.g.
      IPaddress *iplist = NULL;
      GetMetaserverBlock(...,&iplist); */

/* This will fill iplist with the IPs, and return the number retrieved.
   You must free the iplist when you are done with it! */

// How many milliseconds to wait for the response.
#define MAX_WAIT 3000

unsigned int GetMetaserverBlock (char *ip, int port, unsigned char type, int blocknum, IPaddress **iplist);
