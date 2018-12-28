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

void H_Ready_Packet( visionpacket_t *packet, byte *buffer, int readSize ) {
	packet->data = (byte *)buffer;
	packet->currsize = 0;
	packet->maxsize = readSize == NULL ? BUF : readSize;
	packet->read = 0;
}

void H_Write_Packet( visionpacket_t *packet, long long data, int datatype ) {

	switch ( datatype )
	{
	case V_CHAR:
		packet->data[packet->currsize] = (byte)data;
		packet->currsize += 1;
		break;
	case V_SHORT:
		packet->data[packet->currsize] = data & 0xFF;
		packet->data[packet->currsize + 1] = (data >> 8) & 0xFF;
		packet->currsize += 2;
		break;
	case V_INTEGER32:
		packet->data[packet->currsize] = data & 0xFF;
		packet->data[packet->currsize + 1] = (data >> 8) & 0xFF;
		packet->data[packet->currsize + 2] = (data >> 16) & 0xFF;
		packet->data[packet->currsize + 3] = (data >> 24) & 0xFF;
		packet->currsize += 4;
		break;
	case V_INTEGER64:
		packet->data[packet->currsize] = data & 0xFF;
		packet->data[packet->currsize + 1] = (data >> 8) & 0xFF;
		packet->data[packet->currsize + 2] = (data >> 16) & 0xFF;
		packet->data[packet->currsize + 3] = (data >> 24) & 0xFF;
		packet->data[packet->currsize + 4] = (data >> 32) & 0xFF;
		packet->data[packet->currsize + 5] = (data >> 40) & 0xFF;
		packet->data[packet->currsize + 6] = (data >> 48) & 0xFF;
		packet->data[packet->currsize + 7] = (data >> 56) & 0xFF;
		packet->currsize += 8;
		break;
	default:
		break;
	}

}

unsigned long long H_Read_Packet( visionpacket_t *packet, int datatype )
{
	switch ( datatype )
	{
	case V_CHAR:
	{
		byte c_byte = packet->data[packet->read];
		packet->read += 1;
		return (byte)c_byte;
	}
	case V_SHORT:
	{
		unsigned short i_short;

		i_short = packet->data[packet->read] | packet->data[packet->read + 1] << 8;
		packet->read += 2;
		return (unsigned short)i_short;
	}
	case V_INTEGER32:
	{
		unsigned int i_32;

		i_32 = packet->data[packet->read];
		i_32 |= packet->data[packet->read + 1] << 8;
		i_32 |= packet->data[packet->read + 2] << 16;
		i_32 |= packet->data[packet->read + 3] << 24;
		packet->read += 4;
		return (unsigned int)i_32;
	}
	case V_INTEGER64:
	{
		unsigned long long i_64;

		i_64 = packet->data[packet->read];
		i_64 |= (long long)packet->data[packet->read + 1] << 8;
		i_64 |= (long long)packet->data[packet->read + 2] << 16;
		i_64 |= (long long)packet->data[packet->read + 3] << 24;
		i_64 |= (long long)packet->data[packet->read + 4] << 32;
		i_64 |= (long long)packet->data[packet->read + 5] << 40;
		i_64 |= (long long)packet->data[packet->read + 6] << 48;
		i_64 |= (long long)packet->data[packet->read + 7] << 56;
		packet->read += 8;
		return (unsigned long long)i_64;
	}
	default:
		return -1;
	}
}