// A simple program that computes the square root of a number
//#include <stdlib.h>

#include "functions.h"
#include <time.h>

clock_t counter;
clock_t eta;

void Z_SendGSRequest( void ) {

	if ( eta > counter )
		return;

	char buf[BUF] = { 0 };
	visionpacket_t packet;

	H_Ready_Packet( &packet, buf, NULL );

	H_Write_Packet( &packet, 'g', V_CHAR );
	H_Write_Packet( &packet, 's', V_CHAR );
	H_Write_Packet( &packet, 'r', V_CHAR );
	
	for ( int i = 0; i < CLIENTS; i++ )
	{
		if ( visionNetwork.clients[i].state > S_CONNECT )
			UDP_send( &visionNetwork.socket, packet.data, packet.currsize, inet_ntoa( visionNetwork.clients[i].con_info.sin_addr ), ntohs( visionNetwork.clients[i].con_info.sin_port ) );
	}
	
	eta = counter + 50;
}


int main( int argc, char *argv[] )
{
	Z_StartServer_f();

	for ( ;; )
	{
		counter = clock();

		atexit( Z_SendDisconnect );
		Z_ServerLoop();
		Z_CheckHeartBeat();
		Z_SendGSRequest();
		
		
	}
}


	