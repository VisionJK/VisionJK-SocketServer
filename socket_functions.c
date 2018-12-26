#include "functions.h"

int create_socket( int af, int type, int protocol ) {
	SOCKET sock;
	const int y = 1;
	/* Erzeuge das Socket. */
	sock = socket( af, type, protocol );
	if ( sock < 0 )
	{
		printf( "Fehler beim Anlegen eines Sockets\n" );
		return;
	}

	/* Mehr dazu siehe Anmerkung am Ende des Listings ... */
	setsockopt( sock, SOL_SOCKET, SO_REUSEADDR, &y, sizeof( int ) );
	return sock;
}

/* Erzeugt die Bindung an die Serveradresse,
 * (genauer gesagt an einen bestimmten Port). */
void bind_socket( SOCKET *sock, unsigned long adress,
	unsigned short port ) {
	struct sockaddr_in server;
	memset( &server, 0, sizeof( server ) );
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = htonl( adress );
	server.sin_port = htons( port );
	if ( bind( *sock, (struct sockaddr*)&server, sizeof( server ) ) < 0 )
	{
		printf( "Kann das Socket nicht \"binden\" %d\n", WSAGetLastError() );
		return;
	}
}

/* Daten senden via UDP */
void UDP_send( SOCKET *sock, char *data, size_t size,
	char *addr, unsigned short port ) {
	struct sockaddr_in addr_sento;
	struct hostent *h;
	int rc;

	/* IP-Adresse des Servers überprüfen */
	h = gethostbyname( addr );
	if ( h == NULL )
	{
		printf( "Unbekannter Host?" );
		return;
	}


	addr_sento.sin_family = h->h_addrtype;
	memcpy( (char *)&addr_sento.sin_addr.s_addr,
		h->h_addr_list[0], h->h_length );
	addr_sento.sin_port = htons( port );

	rc = sendto( *sock, data, size, 0,
		(struct sockaddr *) &addr_sento,
		sizeof( addr_sento ) );
	if ( rc < 0 )
	{
		printf( "Konnte Daten nicht senden - sendto()" );
		return;
	}
}

/* Daten empfangen via UDP */
void UDP_recv( SOCKET *sock, char *data, size_t size ) {
	struct sockaddr_in addr_recvfrom;
	unsigned int len;
	int n;

	len = sizeof( addr_recvfrom );
	n = recvfrom( sock, data, size, 0, (struct sockaddr *) &addr_recvfrom, &len );
	visionNetwork.remote = addr_recvfrom;
	if ( n < 0 ) {
		printf( "Keine Daten empfangen ...\n" );
		return;
	}
}