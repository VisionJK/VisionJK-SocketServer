#pragma once

#include <stdio.h>
#include <time.h>
#include <math.h>

#ifdef WIN32
#include <Windows.h>
#endif // WIN32

#define PORT 29000
#define BUF 1024
#define WS ' '
#define CLIENTS 8
#define ACCESS_KEY "dummy"
#define HEARTBEAT 30000
#define HEARTBEAT_KICK 40000

#define ARRAY_LEN( x ) ( sizeof( x ) / sizeof( *(x) ) )

typedef struct userinfo_s {
	char name[36]; //copied from name cvar

	struct
	{
		short playerID;
		int health;
		int armor;
		short hat;
	} player;

	char	servername[64];
	struct sockaddr_in	serverip;

} userinfo_t;

typedef struct clients_s {
	//Connection Info
	struct sockaddr_in con_info;
	short state;
	//Heartbeat
	long l_heartbeat;
	boolean h_req;

	userinfo_t uinfo;
} client_t;

typedef struct visionpacket_s {
	byte	*data;
	int		currsize;
	int		currbits;
	int		maxsize;
	int		read;
}visionpacket_t;

typedef struct visionNetwork_s {
	SOCKET socket;
	struct sockaddr_in remote;
	short remote_id;
	client_t clients[CLIENTS];
	visionpacket_t msg_packet;
} visionNetwork_t;

visionNetwork_t visionNetwork;

enum state {
	S_NONE,
	S_CONNECT,
	S_AUTH,
};

enum connection {
	C_REFUSED,
	C_ACCEPTED,
};

enum {
	V_CHAR,
	V_SHORT,
	V_INTEGER32,
	V_INTEGER64,
};

enum {
	PRINT_NONE = -1,
	PRINT_CON,
	PRINT_UI,
};

enum send_e {
	CLIENT_ALL = -1,
};


//server_functions.c
void Z_ServerLoop( void );
void Z_SendStates( void );
void Z_CheckHeartBeat( void );
void Z_StartServer_f( void );
void Z_SendDisconnect( void );
void Z_CleanUser( visionNetwork_t *client, int id, char *msg, short type );
void Z_Print( char *msg, short type );


 //socket_functions.c
int create_socket( int af, int type, int protocol );
void bind_socket( SOCKET * sock, unsigned long adress, unsigned short port );
void UDP_send( SOCKET * sock, char * data, size_t size, char * addr, unsigned short port );
void UDP_recv( SOCKET * sock, char * data, size_t size );

//packet_functions.c
void H_Init_Packet( visionpacket_t *packet, byte *buffer, int readSize );
void H_Write_Packet( visionpacket_t *packet, long long data, int datatype );
unsigned long long H_Read_Packet( visionpacket_t *packet, int datatype );
void PF_RunPacketCmd( char *in, struct sockaddr_in *info );

//globals

#define FRAMERATE 60
#define MSEC_FRAMERATE 1000/FRAMERATE

client_t old_clients[CLIENTS];
clock_t Sys_Clock();
static clock_t sys_timeBase; //Startzeit programm
clock_t sys_curtime, l_clock;
