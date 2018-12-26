// A simple program that computes the square root of a number
//#include <stdlib.h>

#include "functions.h"



int main( int argc, char *argv[] )
{

	Z_StartServer_f();

	//if ( getc( stdin ) == '\n' )
	//{
		for ( ;; )
		{
			onexit( Z_SendDisconnect );
			Z_ServerLoop();
			Z_CheckHeartBeat();
		}
	//}
}


	