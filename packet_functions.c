#include "functions.h"

static void PF_UserExit_f( char *in, struct sockaddr_in *info ) {
	//Client aus dem Index löschen
	for ( int i = 0; i < CLIENTS; i++ )
	{
		if ( !memcmp( &visionNetwork.remote, &visionNetwork.clients[i].con_info, sizeof( struct sockaddr_in ) ) && visionNetwork.clients[i].state > S_NONE )
		{
			char buf[BUF] = { 0 };

			snprintf( buf, BUF, "%s ist vom visionNetwork getrennt", visionNetwork.clients[i].uinfo.name );
			Z_CleanUser( &visionNetwork, i, buf, PRINT_UI );

			Z_UserInfos();
			break;
		}
	}
}

/*
connection 1 = new client - waiting for auth
connection 0 = already connected
*/
static void PF_Connection_f( char *in, struct sockaddr_in *info ) {

	for ( int i = 0; i < CLIENTS; i++ )
	{
		if ( !memcmp( &visionNetwork.remote, &visionNetwork.clients[i].con_info, sizeof( struct sockaddr_in ) ) )
		{
			UDP_send( &visionNetwork.socket, "connection 0", 13, inet_ntoa( visionNetwork.clients[i].con_info.sin_addr ), ntohs( visionNetwork.clients[i].con_info.sin_port ) );
			visionNetwork.clients[i].l_heartbeat = runTime();
			return;
		}

		if ( visionNetwork.clients[i].state < S_CONNECT )
		{
			memcpy( &visionNetwork.clients[i], &visionNetwork.remote, sizeof( struct sockaddr_in ) );
			UDP_send( &visionNetwork.socket, "connection 1", 13, inet_ntoa( visionNetwork.clients[i].con_info.sin_addr ), ntohs( visionNetwork.clients[i].con_info.sin_port ) );
			visionNetwork.clients[i].l_heartbeat = runTime();
			visionNetwork.clients[i].state = S_CONNECT;
			return;
		}

	}

	//Client zum Index hinzufügen
}

static void PF_Auth_f( char *in, struct sockaddr_in *info ) {
	//Client nach password fragen
	//Richtig: Erhält Chat, Heartbeat, Gamestate pakete
	//Falsch: Wird aus Index entfernt, verhält sich wie Connection Paket
	for ( int i = 0; i < CLIENTS; i++ )
	{
		if ( !memcmp( &visionNetwork.remote, &visionNetwork.clients[i].con_info, sizeof( struct sockaddr_in ) ) && visionNetwork.clients[i].state == S_CONNECT )
		{
			if ( !strncmp( in, ACCESS_KEY, strlen( ACCESS_KEY ) ) )//dummy
			{
				UDP_send( &visionNetwork.socket, "auth", 5, inet_ntoa( visionNetwork.clients[i].con_info.sin_addr ), ntohs( visionNetwork.clients[i].con_info.sin_port ) );
				visionNetwork.clients[i].l_heartbeat = runTime();
				visionNetwork.clients[i].state = S_AUTH;
				break;
			}
			else
			{
				UDP_send( &visionNetwork.socket, "disconnect", 11, inet_ntoa( visionNetwork.clients[i].con_info.sin_addr ), ntohs( visionNetwork.clients[i].con_info.sin_port ) );

				Z_CleanUser( &visionNetwork, i, NULL, PRINT_NONE );

				Z_UserInfos();
				break;
			}
		}
	}
}

static void PF_Chat_f( char *in, struct sockaddr_in *info ) {
	for ( int i = 0; i < CLIENTS; i++ )
	{
		if ( !memcmp( &visionNetwork.remote, &visionNetwork.clients[i].con_info, sizeof( struct sockaddr_in ) ) && visionNetwork.clients[i].state > S_CONNECT )
		{
			char buf[BUF] = { 0 };
			int len;

			if ( len = snprintf( &buf, BUF, "chat %s: %s", visionNetwork.clients[i].uinfo.name, in ) > 0 )
			{
				for ( int j = 0; j < CLIENTS && visionNetwork.clients[j].state == S_AUTH; j++ )
					UDP_send( &visionNetwork.socket, buf, strlen( buf ) + 1, inet_ntoa( visionNetwork.clients[j].con_info.sin_addr ), ntohs( visionNetwork.clients[j].con_info.sin_port ) );
				visionNetwork.clients[i].l_heartbeat = runTime();
			}
			break;
		}
	}
}

static void PF_Heartbeat_f( char *in, struct sockaddr_in *info ) {
	//Lebst du noch oder bist du schon tot?
	for ( int i = 0; i < CLIENTS; i++ )
	{
		if ( !memcmp( &visionNetwork.remote, &visionNetwork.clients[i].con_info, sizeof( struct sockaddr_in ) ) && visionNetwork.clients[i].state > S_NONE )
		{
			visionNetwork.clients[i].l_heartbeat = runTime();
			visionNetwork.clients[i].h_req = 0;
			break;
		}
	}
}

static void PF_Print_f( char *in, struct sockaddr_in *info )
{
	for ( int i = 0; i < CLIENTS; i++ )
	{
		if ( !memcmp( &visionNetwork.remote, &visionNetwork.clients[i].con_info, sizeof( struct sockaddr_in ) ) && visionNetwork.clients[i].state > S_CONNECT )
		{
			char buf[BUF] = { 0 };

			char *ptr = in;
			int print_i = 0;

			switch ( *ptr )
			{
			case '0':
				ptr += 2;
				print_i = 0;
				break;
			case '1':
				ptr += 2;
				print_i = 1;
				break;
			case '2':
				ptr += 2;
				print_i = 2;
				break;
			case NULL:
				return;
			default:
				ptr += 2;
				print_i = 0;
				break;
			}

			if ( snprintf( &buf, BUF, "print %d %s", print_i, ptr ) > 0 )
			{
				for ( int j = 0; j < CLIENTS && visionNetwork.clients[j].state == S_AUTH; j++ )
					UDP_send( &visionNetwork.socket, buf, strlen( buf ) + 1, inet_ntoa( visionNetwork.clients[j].con_info.sin_addr ), ntohs( visionNetwork.clients[j].con_info.sin_port ) );
				visionNetwork.clients[i].l_heartbeat = runTime();
			}

			break;
		}
	}
}

static void PF_newInfo_f( char *in, struct sockaddr_in *info ) {
	//Zum verschicken einzelner Daten (Hut, HP, Viewangles)
	for ( int i = 0; i < CLIENTS; i++ )
	{
		if ( !memcmp( &visionNetwork.remote, &visionNetwork.clients[i].con_info, sizeof( struct sockaddr_in ) ) && visionNetwork.clients[i].state > S_NONE )
		{
			visionpacket_t packet;

			H_Init_Packet( &packet, in, NULL );

			visionNetwork.clients[i].uinfo.player.playerID = H_Read_Packet( &packet, V_SHORT );
			visionNetwork.clients[i].uinfo.player.hat = H_Read_Packet( &packet, V_SHORT );
			visionNetwork.clients[i].uinfo.player.health = H_Read_Packet( &packet, V_INTEGER32 );
			visionNetwork.clients[i].uinfo.player.armor = H_Read_Packet( &packet, V_INTEGER32 );
			visionNetwork.clients[i].uinfo.serverip.sin_addr.s_addr = (ULONG)H_Read_Packet( &packet, V_INTEGER32 );
			visionNetwork.clients[i].uinfo.serverip.sin_port = (u_short)H_Read_Packet( &packet, V_SHORT );
			for ( int j = 0; j < 64; j++ )
			{
				byte c = H_Read_Packet( &packet, V_CHAR );

				if ( c == NULL )
					break;

				visionNetwork.clients[i].uinfo.servername[j] = c;
			}

			for ( int j = 0; j < 36; j++ )
			{
				byte c = H_Read_Packet( &packet, V_CHAR );

				if ( c == NULL )
					break;

				visionNetwork.clients[i].uinfo.name[j] = c;
			}

			printf( "Userinfo[%d] eingetragen: name: %s, id: %hd, hat: %hd, health: %d, armor: %d, serverip: %s, port: %u Server: %s\n",
				i,
				visionNetwork.clients[i].uinfo.name,
				visionNetwork.clients[i].uinfo.player.playerID,
				visionNetwork.clients[i].uinfo.player.hat,
				visionNetwork.clients[i].uinfo.player.health,
				visionNetwork.clients[i].uinfo.player.armor,
				inet_ntoa( visionNetwork.clients[i].uinfo.serverip.sin_addr ),
				ntohs( visionNetwork.clients[i].uinfo.serverip.sin_port ),
				visionNetwork.clients[i].uinfo.servername );


			char buf[BUF] = { 0 };

			if ( snprintf( &buf, BUF, "%s ist dem visionNetwork beigetreten", visionNetwork.clients[i].uinfo.name ) > 0 )
			{
				Z_Print( &buf, PRINT_UI );
			}

			Z_UserInfos();

			break;
		}
	}
}


static void PF_GameState_f( char *in, struct sockaddr_in *info ) {
	//Zum verschicken einzelner Daten (Hut, HP, Viewangles)
	for ( int i = 0; i < CLIENTS; i++ )
	{
		if ( !memcmp( &visionNetwork.remote, &visionNetwork.clients[i].con_info, sizeof( struct sockaddr_in ) ) && visionNetwork.clients[i].state > S_CONNECT )
		{
			char buf[BUF] = { 0 };

			visionpacket_t packet;

			H_Init_Packet( &packet, in, NULL );

			visionNetwork.clients[i].uinfo.player.health = H_Read_Packet( &packet, V_INTEGER32 );
			visionNetwork.clients[i].uinfo.player.armor = H_Read_Packet( &packet, V_INTEGER32 );
			visionNetwork.clients[i].uinfo.player.hat = H_Read_Packet( &packet, V_SHORT );

			H_Init_Packet( &packet, buf, NULL );

			H_Write_Packet( &packet, 'g', V_CHAR );
			H_Write_Packet( &packet, 's', V_CHAR );
			H_Write_Packet( &packet, ' ', V_CHAR );

			for ( int i = 0; i < CLIENTS; i++ )
			{
				H_Write_Packet( &packet, visionNetwork.clients[i].uinfo.player.health, V_INTEGER32 );
				H_Write_Packet( &packet, visionNetwork.clients[i].uinfo.player.armor, V_INTEGER32 );
				H_Write_Packet( &packet, visionNetwork.clients[i].uinfo.player.hat, V_SHORT );
			}

			UDP_send( &visionNetwork.socket, packet.data, packet.currsize, inet_ntoa( visionNetwork.clients[i].con_info.sin_addr ), ntohs( visionNetwork.clients[i].con_info.sin_port ) );
			break;
		}
	}
}

typedef struct packetfunc_s {
	char	*name;
	void( *func )(char* data, struct sockaddr_in *info);
} packetfunc_t;

static packetfunc_t packetfuncs[] = {
	{ "auth",		PF_Auth_f },		// Authentifikation (Passwort Abfrage)
	{ "connection", PF_Connection_f },	// Client verbindet sich
	{ "chat",		PF_Chat_f },		// Chat Nachricht
	{ "print",		PF_Print_f },
	{ "cinfo",		PF_newInfo_f },
	{ "heartbeat",	PF_Heartbeat_f },	// Überprüfen ob noch aktiv
	{ "quit",		PF_UserExit_f },		// Client hat das Spiel geschlossen
	{ "gs",			PF_GameState_f },
};

static const size_t max_packetfuncs = ARRAY_LEN( packetfuncs );

void PF_RunPacketCmd( char *in, struct sockaddr_in *info ) {

	if ( in == NULL )
		return;

	char *cmd = strtok( in, " " );

	for ( int i = 0; i < max_packetfuncs; i++ )
	{
		if ( !strncmp( cmd, packetfuncs[i].name, strlen( packetfuncs[i].name ) + 1 ) )
		{
			packetfuncs[i].func( in + strlen( packetfuncs[i].name ) + 1, &info );
		}
	}
}