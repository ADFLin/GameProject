#ifndef Graphics2DBase_h__
#define Graphics2DBase_h__

#include "TVector2.h"
typedef unsigned char uint8;

typedef TVector2< int >   Vec2i;

struct ColorKey3
{
	ColorKey3(){}
	ColorKey3( uint8 r_ , uint8 g_ , uint8 b_ ) : r(r_),g(g_),b(b_){}
	uint8 r , g , b ;
	operator COLORREF() const { return RGB( r , g , b ); }
};

#endif // Graphics2DBase_h__
