#include "TinyGamePCH.h"

#include "TinyGameApp.h"

TinyGameApp game;

class V3
{
public:
	operator       int* ()       { return &x; }
	operator const int* () const { return &x; }
public:
	int x,y,z;
};



int main( int argc , char* argv[] )
{    
	game.run();
	return 0;
}




