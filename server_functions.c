#include "functions.h"

long runTime( void ) 
{
	clock_t t = clock();

	return t;
}

void Z_SendPacket( visionpacket_t *packet, int client ) {

	if ( client == CLIENT_ALL )
	{
		for ( int i = 0; i < CLIENTS && visionNetwork.clients[i].state > S_NONE; j++ )
		{
			UDP_send( &visionNetwork.socket, packet->data, packet->currsize, inet_ntoa( visionNetwork.clients[i].con_info.sin_addr ), htons( visionNetwork.clients[i].con_info.sin_port ) );
		}

	}
	else
	{
		UDP_send( &visionNetwork.socket, packet->data, packet->currsize, inet_ntoa( visionNetwork.clients[client].con_info.sin_addr ), htons( visionNetwork.clients[client].con_info.sin_port ) );
	}

}

void Z_SendRaw( char *msg, int client ) {

	if ( client == CLIENT_ALL )
	{
		for ( int i = 0; i < CLIENTS && visionNetwork.clients[i].state > S_NONE; i++ )
		{
			UDP_send( &visionNetwork.socket, msg, strlen( msg ) + 1, inet_ntoa( visionNetwork.clients[i].con_info.sin_addr ), htons( visionNetwork.clients[i].con_info.sin_port ) );
		}

	}
	else
	{
		UDP_send( &visionNetwork.socket, msg, strlen( msg ) + 1, inet_ntoa( visionNetwork.clients[client].con_info.sin_addr ), htons( visionNetwork.clients[client].con_info.sin_port ) );
	}

}

int SV_SenderRegistered( void ) {
	
	for ( int i = 0; i < CLIENTS; i++ )
	{
		if ( !memcmp( &visionNetwork.remote, &visionNetwork.clients[i].con_info, sizeof( struct sockaddr_in ) ) )
		{
			//printf( "Already registered\n" );
			visionNetwork.remote_id = i;
			return S_CONNECT;
		}
			
	}

	//printf( "Not registered\n" );
	return S_NONE;
}

int SV_AcceptConnection( visionpacket_t *packet ) {
	byte readcmd[] = "connection";
	
	for ( int i = 0; i < strlen( readcmd ); i++ )
	{
		if ( readcmd[i] != H_Read_Packet( packet, V_CHAR ) )
			return C_REFUSED;
	}

	//read whitespace
	if ( H_Read_Packet( packet, V_CHAR ) != WS )
		return C_REFUSED;

	//check password
	
	byte c = 0;
	byte authPass[] = ACCESS_KEY;
	int i = 0;

	while ( c = H_Read_Packet( packet, V_CHAR ) )
	{
		if ( authPass[i] != c )
			return C_REFUSED;
		i++;
	}

	return C_ACCEPTED;
}

int SV_AssignSlot( visionpacket_t *packet ) {
	for ( int i = 0; i < CLIENTS; i++ )
	{
		if ( visionNetwork.clients[i].state < S_CONNECT )
		{
			memcpy( &visionNetwork.clients[i].con_info, &visionNetwork.remote, sizeof( struct sockaddr_in ) );
			visionNetwork.clients[i].state = S_CONNECT;
			visionNetwork.remote_id = i;

			visionpacket_t packet;
			char buf[16] = { 0 };

			H_Init_Packet( &packet, buf, NULL );

			H_Write_Packet( &packet, 'a', V_CHAR );
			H_Write_Packet( &packet, 'c', V_CHAR );
			H_Write_Packet( &packet, 'c', V_CHAR );
			H_Write_Packet( &packet, ' ', V_CHAR );
			H_Write_Packet( &packet, visionNetwork.remote_id, V_SHORT );
			
			Z_SendPacket( &packet, visionNetwork.remote_id);

			//printf( "Assigning\n" );
			return C_ACCEPTED;
		}
	}

	//printf( "Can't assign\n" );
	return C_REFUSED;
}

int SV_ReturnID() {
	return visionNetwork.remote_id;
}

int SV_ParsePacket( void ) {
	visionpacket_t *packet = &visionNetwork.msg_packet;
	visionNetwork_t *net = &visionNetwork;


	if ( SV_SenderRegistered() == S_NONE )
	{
		//Connection code

		if ( SV_AcceptConnection( packet ) == C_REFUSED )
		{
			//printf( "Refused\n" );
			return C_REFUSED;
		}
		else
			return SV_AssignSlot( packet );
	}
	else 
	{
		int id = SV_ReturnID();

		old_clients[id] = net->clients[id];

		int seq = H_Read_Packet( packet, V_INTEGER64 );
		net->clients[id].uinfo.player.health = H_Read_Packet( packet, V_INTEGER32 );
		net->clients[id].uinfo.player.armor = H_Read_Packet( packet, V_INTEGER32 );
		net->clients[id].uinfo.player.hat = H_Read_Packet( packet, V_SHORT );
		
		//printf( "Parsing Gamestate\n" );
		return C_ACCEPTED;
	}

}

static long long staticinteger;

byte delta_states( client_t client[], client_t old_client[] ) {
	byte state = 0;

	for ( int i = 0; i < CLIENTS; i++ )
	{
		if ( (client[i].uinfo.player.health - old_client[i].uinfo.player.health ||
			client[i].uinfo.player.armor - old_client[i].uinfo.player.armor ||
			client[i].uinfo.player.hat - old_client[i].uinfo.player.hat) != 0 )
		{
			if ( !state )
				state = 1 << i;
			else
				state |= 1 << i;
		}
	}

	return (byte)state;
}

boolean check_state( byte state, int offset ) {
	if ( (state >> offset) & 1 )
		return 1;
	return 0;
}

void Z_SendStates( void ) {
	visionpacket_t *packet = &visionNetwork.msg_packet;
	visionNetwork_t *net = &visionNetwork;

	char buffer[BUF] = { 0 };

	H_Init_Packet( packet, buffer, NULL );

	H_Write_Packet( packet, staticinteger++, V_INTEGER64 ); // Sequence

	byte delta = delta_states( &net->clients, &old_clients );

	//H_Write_Packet( packet, delta, V_CHAR );

	for ( int i = 0; i < CLIENTS; i++ )
	{
		if ( check_state( delta, i ) ) 
		{
			H_Write_Packet( packet, net->clients[i].uinfo.player.health, V_INTEGER32 );
			H_Write_Packet( packet, net->clients[i].uinfo.player.armor, V_INTEGER32 );
			H_Write_Packet( packet, net->clients[i].uinfo.player.hat, V_SHORT );
		}		
	}
	
	Z_SendPacket( packet, CLIENT_ALL );
}

void Z_ServerLoop( void ) {
	char buffer[BUF];

	fd_set fdset;
	fd_set fdwrite;
	FD_ZERO( &fdset );
	FD_ZERO( &fdwrite );
	FD_SET( visionNetwork.socket, &fdset );
	FD_SET( visionNetwork.socket, &fdwrite );

	struct timeval tv_timeout = { 0, (1000 * 1000) / 60 };
	
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
		//printf( "%s:%d: %s\n", inet_ntoa( visionNetwork.remote.sin_addr ), ntohs( visionNetwork.remote.sin_port ), buffer );

		//UDP_send( &visionNetwork.socket, "Empfangen", 10, inet_ntoa( visionNetwork.remote.sin_addr ), ntohs( visionNetwork.remote.sin_port ) );
		printf( "%s", buffer );

		visionNetwork.remote_id = -1,
		H_Init_Packet( &visionNetwork.msg_packet, &buffer, NULL );
		SV_ParsePacket();
		//PF_RunPacketCmd( &buffer, &visionNetwork.remote );
	}

	if ( FD_ISSET( visionNetwork.socket, &fdwrite ) && Sys_Clock() - l_clock > MSEC_FRAMERATE )
	{
		Z_SendStates();
		l_clock = clock();
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
	memset( &old_clients, 0, sizeof( old_clients ) );
	memset( visionNetwork.clients, 0, sizeof(visionNetwork.clients) );
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

	H_Init_Packet( &packet, buf, NULL );

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