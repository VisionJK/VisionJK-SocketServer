#include "functions.h"
#include <time.h>

long runTime( void ) 
{
	clock_t t = clock();

	return t;
}

void Z_ServerLoop( void ) {
	char buffer[BUF];

	fd_set fdset;
	FD_ZERO( &fdset );
	FD_SET( visionNetwork.socket, &fdset );

	struct timeval tv_timeout = { 0, 0 };

	int select_retval = select( visionNetwork.socket + 1, &fdset, NULL, NULL, &tv_timeout );

	if ( select_retval == -1 )
	{
		return;
	}

	if ( FD_ISSET( visionNetwork.socket, &fdset ) )
	{
		memset( buffer, 0, BUF );
		UDP_recv( visionNetwork.socket, &buffer, BUF );

		/* erhaltene Nachricht ausgeben */
		//printf( "%s:%d Daten erhalten: %s\n", inet_ntoa( visionNetwork.remote.sin_addr ), ntohs( visionNetwork.remote.sin_port ), buffer );

		//UDP_send( &visionNetwork.socket, "Empfangen", 10, inet_ntoa( visionNetwork.remote.sin_addr ), ntohs( visionNetwork.remote.sin_port ) );

		
		PF_RunPacketCmd( &buffer, &visionNetwork.remote );
	}
	else
	{
		return;
	}

}

void Z_StartServer_f( void ) {
#ifdef _WIN32
	/* Initialisiere TCP für Windows ("winsock"). */
	WORD wVersionRequested;
	WSADATA wsaData;
	wVersionRequested = MAKEWORD( 1, 1 );
	if ( WSAStartup( wVersionRequested, &wsaData ) != 0 )
		exit( EXIT_FAILURE );
	else
		printf( "Winsock initialisiert\n" );
#endif

	visionNetwork.socket == SOCKET_ERROR;
	for ( int i = 0; i < CLIENTS; i++ )
		visionNetwork.clients[i].l_heartbeat = -1;

	/* Socket erzeugen */
	visionNetwork.socket = create_socket( AF_INET, SOCK_DGRAM, 0 );

	bind_socket( &visionNetwork.socket, INADDR_ANY, PORT );
	printf( "Warte auf Daten am Port (UDP) %u\n", PORT );
}

void Z_UserInfos( void )
{
	visionpacket_t packet;
	byte buf[BUF] = { 0 };

	H_Ready_Packet( &packet, buf, NULL );

	H_Write_Packet( &packet, 'n', V_CHAR );
	H_Write_Packet( &packet, 'i', V_CHAR );
	H_Write_Packet( &packet, ' ', V_CHAR );


	for ( int i = 0; i < CLIENTS; i++ )
	{
		H_Write_Packet( &packet, i, V_SHORT );
		H_Write_Packet( &packet, visionNetwork.clients[i].state, V_SHORT );
		H_Write_Packet( &packet, visionNetwork.clients[i].uinfo.player.playerID, V_SHORT );
		H_Write_Packet( &packet, visionNetwork.clients[i].uinfo.player.hat, V_SHORT );
		H_Write_Packet( &packet, visionNetwork.clients[i].uinfo.player.health, V_INTEGER32 );
		H_Write_Packet( &packet, visionNetwork.clients[i].uinfo.player.armor, V_INTEGER32 );

		H_Write_Packet( &packet, visionNetwork.clients[i].uinfo.serverip.sin_addr.s_addr, V_INTEGER32 );
		H_Write_Packet( &packet, visionNetwork.clients[i].uinfo.serverip.sin_port, V_SHORT );
		for ( int j = 0; j < 64; j++ )
		{
			if ( visionNetwork.clients[i].uinfo.servername[j] == NULL )
				break;

			H_Write_Packet( &packet, visionNetwork.clients[i].uinfo.servername[j], V_CHAR );
		}
		H_Write_Packet( &packet, '\0', V_CHAR );
		for ( int j = 0; j < 36; j++ )
		{
			if ( visionNetwork.clients[i].uinfo.name[j] == NULL )
				break;

			H_Write_Packet( &packet, visionNetwork.clients[i].uinfo.name[j], V_CHAR );
		}
		H_Write_Packet( &packet, '\0', V_CHAR );
	}

	for ( int i = 0; i < CLIENTS; i++ )
	{
		if ( visionNetwork.clients[i].state < S_AUTH )
			continue;

		UDP_send( &visionNetwork.socket, packet.data, packet.currsize, inet_ntoa( visionNetwork.clients[i].con_info.sin_addr ), ntohs( visionNetwork.clients[i].con_info.sin_port ) );
		visionNetwork.clients[i].l_heartbeat = runTime();
	}
}

void Z_CheckHeartBeat( void ) 
{
	for ( int i = 0; i < CLIENTS; i++ )
	{
		if ( ( visionNetwork.clients[i].l_heartbeat + HEARTBEAT < runTime() && visionNetwork.clients[i].state > S_NONE ) && visionNetwork.clients[i].h_req == 0 )
		{
			char buf[BUF] = { 0 };
			int len;

			if ( len = snprintf( &buf, BUF, "hb %d", runTime() ) > 0 )
			{
				UDP_send( &visionNetwork.socket, buf, strlen( buf ) + 1, inet_ntoa( visionNetwork.clients[i].con_info.sin_addr ), ntohs( visionNetwork.clients[i].con_info.sin_port ) );
				visionNetwork.clients[i].h_req = 1;
			}
		}

		if ( (visionNetwork.clients[i].l_heartbeat + HEARTBEAT_KICK < runTime() && visionNetwork.clients[i].state > S_NONE) && visionNetwork.clients[i].h_req == 1 )
		{
			printf( "Entferne %s:%d[%d] aus dem Index (Kein Heartbeat)\n", inet_ntoa( visionNetwork.clients[i].con_info.sin_addr ), ntohs( visionNetwork.clients[i].con_info.sin_port), i );
			UDP_send( &visionNetwork.socket, "disconnect", 11, inet_ntoa( visionNetwork.clients[i].con_info.sin_addr ), ntohs( visionNetwork.clients[i].con_info.sin_port ) );
			char buf[BUF] = { 0 };

			if ( snprintf( &buf, BUF, "%s wurde aus dem visionNetwork entfernt. (Kein Heartbeat)", visionNetwork.clients[i].uinfo.name ) > 0 )
			{
				Z_CleanUser( &visionNetwork, i, buf, PRINT_UI );
			}

			Z_UserInfos();
		}
	}
}

void Z_SendDisconnect( void ) {
	for ( int i = 0; i < CLIENTS; i++ )
	{
		if( visionNetwork.clients[i].state > S_NONE )
			UDP_send( &visionNetwork.socket, "disconnect", 11, inet_ntoa( visionNetwork.clients[i].con_info.sin_addr ), ntohs( visionNetwork.clients[i].con_info.sin_port ) );
	}
}

void Z_CleanUser( visionNetwork_t *client, int id, char *msg, short type )
{
	if ( type > PRINT_NONE )
		Z_Print( msg, type );

	memset( &client->clients[id].uinfo, 0, sizeof( userinfo_t ) );
	memset( &client->clients[id], 0, sizeof( client_t ) );
	client->clients[id].l_heartbeat = -1;
	client->clients[id].state = S_NONE;
}

void Z_Print( char *msg, short type ) {
	char buf[BUF] = { 0 };

	if ( snprintf( &buf, BUF, "print %d %s", type, msg ) > 0 )
	{
		for ( int j = 0; j < CLIENTS && visionNetwork.clients[j].state == S_AUTH; j++ )
			UDP_send( &visionNetwork.socket, buf, strlen( buf ) + 1, inet_ntoa( visionNetwork.clients[j].con_info.sin_addr ), ntohs( visionNetwork.clients[j].con_info.sin_port ) );
	}
}