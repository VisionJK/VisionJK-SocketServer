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
		printf( "%s:%d Daten erhalten: %s\n", inet_ntoa( visionNetwork.remote.sin_addr ), ntohs( visionNetwork.remote.sin_port ), buffer );

		//UDP_send( &visionNetwork.socket, "Empfangen", 10, inet_ntoa( visionNetwork.remote.sin_addr ), ntohs( visionNetwork.remote.sin_port ) );

		Z_RunPacketCmd( &buffer, &visionNetwork.remote );
	}
	else
	{
		return;
	}

}

void Z_StartServer_f( void ) {
#ifdef _WIN32
	/* Initialisiere TCP f�r Windows ("winsock"). */
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

static void PF_Quit_f( char *in, struct sockaddr_in *info ) {
	//Client aus dem Index l�schen

	char *ptr = in;

	for ( int i = 0; i < CLIENTS; i++ )
	{
		if ( !memcmp( &visionNetwork.remote, &visionNetwork.clients[i].con_info, sizeof( struct sockaddr_in ) ) )
		{
			UDP_send( &visionNetwork.socket, "disconnect", 11, inet_ntoa( visionNetwork.clients[i].con_info.sin_addr ), ntohs( visionNetwork.clients[i].con_info.sin_port ) );
			memset( &visionNetwork.clients[i], 0, sizeof( visionNetwork.clients[i] ) );
			visionNetwork.clients[i].l_heartbeat = -1;
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

		if ( visionNetwork.clients[i].l_heartbeat == -1 )
		{
			memcpy( &visionNetwork.clients[i], &visionNetwork.remote, sizeof( struct sockaddr_in ) );
			UDP_send( &visionNetwork.socket, "connection 1", 13, inet_ntoa( visionNetwork.clients[i].con_info.sin_addr ), ntohs( visionNetwork.clients[i].con_info.sin_port ) );
			visionNetwork.clients[i].l_heartbeat = runTime();
			visionNetwork.clients[i].state = S_CONNECT;
			return;
		}
	}
		
	//Client zum Index hinzuf�gen
}

static void PF_Auth_f( char *in, struct sockaddr_in *info ) {
	//Client nach password fragen
	//Richtig: Erh�lt Chat, Heartbeat, Gamestate pakete
	//Falsch: Wird aus Index entfernt, verh�lt sich wie Connection Paket
	for (int i = 0; i < CLIENTS; i++)
	{
		if (!memcmp(&visionNetwork.remote, &visionNetwork.clients[i].con_info, sizeof(struct sockaddr_in)))
		{
			if (!strncmp(in, ACCESS_KEY, strlen(in)))//dummy
			{
				UDP_send(&visionNetwork.socket, "auth", 5, inet_ntoa(visionNetwork.clients[i].con_info.sin_addr), ntohs(visionNetwork.clients[i].con_info.sin_port));
				visionNetwork.clients[i].l_heartbeat = runTime();
				visionNetwork.clients[i].state = S_AUTH;
			}
			else
			{
				UDP_send(&visionNetwork.socket, "disconnect", 11, inet_ntoa(visionNetwork.clients[i].con_info.sin_addr), ntohs(visionNetwork.clients[i].con_info.sin_port));
				memset(&visionNetwork.clients[i], 0, sizeof(visionNetwork.clients[i]));
				visionNetwork.clients[i].l_heartbeat = -1;
			}
		}
	}
}

static void PF_Chat_f( char *in, struct sockaddr_in *info ) {
	for (int i = 0; i < CLIENTS; i++)
	{
		if ( !memcmp( &visionNetwork.remote, &visionNetwork.clients[i].con_info, sizeof( struct sockaddr_in ) ) && visionNetwork.clients[i].state == S_AUTH )
		{
			char buf[BUF] = { 0 };
			int len;

			if ( len = snprintf( &buf, BUF, "chat %s", in ) > 0 )
			{
				for ( int j = 0; j < CLIENTS && visionNetwork.clients[j].state == S_AUTH; j++ )
					UDP_send( &visionNetwork.socket, &buf, strlen( buf ) + 1, inet_ntoa( visionNetwork.clients[j].con_info.sin_addr ), ntohs( visionNetwork.clients[j].con_info.sin_port ) );
				visionNetwork.clients[i].l_heartbeat = runTime();
			}		
		}
	}
}

static void PF_Heartbeat_f( char *in, struct sockaddr_in *info ) {
	//Lebst du noch oder bist du schon tot?
	for ( int i = 0; i < CLIENTS; i++ )
	{
		if ( !memcmp( &visionNetwork.remote, &visionNetwork.clients[i].con_info, sizeof( struct sockaddr_in ) ) && visionNetwork.clients[i].state == S_AUTH )
		{
			char buf[BUF] = { 0 };
			int len;

			if ( len = snprintf( &buf, BUF, "hb %d", runTime() ) > 0 )
			{
				UDP_send( &visionNetwork.socket, buf, strlen(buf) + 1, inet_ntoa( visionNetwork.clients[i].con_info.sin_addr ), ntohs( visionNetwork.clients[i].con_info.sin_port ) );
				visionNetwork.clients[i].l_heartbeat = runTime();
			}
		}
	}
}

static void PF_GameState_f( char *in, struct sockaddr_in *info ) {
	//Zum verschicken einzelner Daten (Hut, HP, Viewangles)
}


typedef struct packetfunc_s {
	char	*name;
	void	( *func )(char* data, struct sockaddr_in *info);
	boolean	needAuth;
} packetfunc_t;

static packetfunc_t packetfuncs[] = {
	{ "auth",		PF_Auth_f, 0 },		// Authentifikation (Passwort Abfrage)
	{ "connection", PF_Connection_f, 0 },	// Client verbindet sich
	{ "chat",		PF_Chat_f, 1 },		// Chat Nachricht
	{ "gs",			PF_GameState_f, 1 },	// Gamestate
	{ "heartbeat",	PF_Heartbeat_f, 1 },	// �berpr�fen ob noch aktiv
	{ "quit",		PF_Quit_f, 1 },		// Client hat das Spiel geschlossen

};

static const size_t max_packetfuncs = ARRAY_LEN( packetfuncs );

void Z_RunPacketCmd( char *in, struct sockaddr_in *info ) {
	
	if ( in == NULL )
		return;

	char buf[BUF] = { 0 };

	strncpy( &buf, in, BUF );
	buf[BUF - 1] = '\0';

	char *cmd = strtok( in, " " );



	for ( int i = 0; i < max_packetfuncs; i++ )
	{
		if ( !strncmp( cmd, packetfuncs[i].name, strlen( packetfuncs[i].name ) + 1 ) )
		{
			packetfuncs[i].func( buf + strlen( packetfuncs[i].name ) + 1, &info );
		}
	}
}