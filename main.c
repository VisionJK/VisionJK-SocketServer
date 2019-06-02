#include "functions.h"
#include <climits>


void Sys_BaseClock() {
	sys_timeBase = clock();
}

clock_t Sys_Clock() {
	sys_curtime = clock();

	sys_curtime -= sys_timeBase;

	return sys_curtime;
}

#if 0
void Z_SendGSRequest( void ) {

	if ( Sys_Clock() - sys_timeBase > MSEC_FRAMERATE )
		return;

	char buf[BUF] = { 0 };
	visionpacket_t packet;

	H_Init_Packet( &packet, buf, NULL );

	H_Write_Packet( &packet, 'g', V_CHAR );
	H_Write_Packet( &packet, 's', V_CHAR );
	H_Write_Packet( &packet, 'r', V_CHAR );
	
	for ( int i = 0; i < CLIENTS; i++ )
	{
		if ( visionNetwork.clients[i].state > S_CONNECT )
			UDP_send( &visionNetwork.socket, packet.data, packet.currsize, inet_ntoa( visionNetwork.clients[i].con_info.sin_addr ), ntohs( visionNetwork.clients[i].con_info.sin_port ) );
	}
	
	sys_timeBase = clock();
}
#endif

int main( int argc, char *argv[] )
{
	Z_StartServer_f();
	Sys_BaseClock();

	if ( getc( stdin ) == '\n' )
	{
		for ( ;; )
		{
			Z_ServerLoop();
			//Z_CheckHeartBeat();
			//Z_SendGSRequest();


		}
	}

	
}


	