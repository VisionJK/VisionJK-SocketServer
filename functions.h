#pragma once

#include <stdio.h>

#ifdef WIN32
#include <Windows.h>
#endif // WIN32

#define PORT 29075
#define BUF 4096
#define WS ' '
#define CLIENTS 8
#define ACCESS_KEY "dummy"
#define HEARTBEAT 30000
#define HEARTBEAT_KICK 40000

#define ARRAY_LEN( x ) ( sizeof( x ) / sizeof( *(x) ) )

struct visionNetwork_s {
	SOCKET socket;
	struct sockaddr_in remote;

	struct {
		struct sockaddr_in con_info;
		long l_heartbeat;
		boolean h_req;
		int state;
	} clients[CLIENTS];
} visionNetwork;

enum state {
	S_NONE,
	S_CONNECT,
	S_AUTH,
};

void Z_ServerLoop( void );
void Z_CheckHeartBeat( void );
void Z_StartServer_f( void );
int create_socket( int af, int type, int protocol );
void bind_socket( SOCKET * sock, unsigned long adress, unsigned short port );
void UDP_send( SOCKET * sock, char * data, size_t size, char * addr, unsigned short port );
void UDP_recv( SOCKET * sock, char * data, size_t size );
void Z_RunPacketCmd( char *in, struct sockaddr_in *info );
void Z_SendDisconnect( void );
