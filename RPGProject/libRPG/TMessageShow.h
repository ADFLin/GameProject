#ifndef TMessageShow_h__
#define TMessageShow_h__

#include "common.h"
#include "CFWorld.h"

#include <cstdarg>

class TMessageShow
{
public:
	typedef unsigned char BYTE;

	TMessageShow()
	{
		height = 15;
		setPos( 10 , 10 );
		setColor( 255 , 255 , 0 , 255 );
	}

	void shiftPos( int dx , int dy )
	{
		startX += dx;
		startY += dy;
	}

	void setPos( int x , int y )
	{
		startX = x ; startY = y;
	}

	void setColor( BYTE r, BYTE g , BYTE b , BYTE a = 255 )
	{
		cr = r ; cg = g; cb = b ; ca = a;  
	}
	void start()
	{
		lineNun = 0;
	}


	void pushStr( char const* str )
	{
		//getRenderSystem()->getGraphics2D().setTextColor(
		//	ColorKey3( cr , cg , cb ) );

		mWorld->showMessage( startX , startY + lineNun * height , str , cr , cg , cb );
		++lineNun;
	}
	void push( char const* format, ... )
	{
		va_list argptr;
		va_start( argptr, format );
		vsprintf( string , format , argptr );
		va_end( argptr );

		pushStr( string );
	}

	void finish()
	{

	}

	void setOffsetY( int y )
	{
		height = y;
	}

	CFWorld*  mWorld;

	char     string[512];
	int      height;
	int      lineNun;
	BYTE     cr,cg,cb,ca;
	int      startX , startY;
};

#endif // TMessageShow_h__