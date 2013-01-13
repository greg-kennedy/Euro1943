/***********************************/
/* The OverServer - Greg Kennedy   */
/***********************************/

/* Usage:  Call ReportToMetaserver during your game init. */
/*   (This function returns a 0 if it was successful, or a 1 otherwise) */
/* Call UnReportToMetaserver when shutting down your server. */
/*   (These functions offer no return values) */

/* ip is the location of the OverServer.  os_port is the OverServer port. */
/* srv_port is YOUR game's port number.  type is the gametype. */

// How many milliseconds to wait for the magic key packet
#define MAX_WAIT 3000
// How often to re-report with master server, in milliseconds
#define HEARTBEAT_DELAY 240000

int ReportToMetaserver (char *ip, int os_port, unsigned short int srv_port, unsigned char type);
void UnReportToMetaserver();
