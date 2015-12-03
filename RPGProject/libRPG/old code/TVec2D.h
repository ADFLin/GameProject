#ifndef TVec2D_h__
#define TVec2D_h__

class TVec2D
{
public:
	TVec2D(){}
	TVec2D(int _x, int _y )
		:x(_x),y(_y){}

	int x , y;

	TVec2D operator-( TVec2D const& rh )  const
	{ return TVec2D( x - rh.x , y - rh.y ); }

	TVec2D operator+( TVec2D const& rh )  const
	{ return TVec2D( x + rh.x , y + rh.y ); }

	TVec2D& operator+=( TVec2D const& rh )
	{ x += rh.x ; y += rh.y ; return *this; }

	TVec2D& operator-=( TVec2D const& rh )
	{ x -= rh.x ; y -= rh.y ; return *this; }

};

template < class T >
TVec2D operator * ( T s , TVec2D const& v )
{
	return TVec2D( s * v.x , s * v.y );
}

template < class T >
TVec2D operator * ( TVec2D const& v , T s )
{
	return TVec2D( s * v.x , s * v.y );
}
#endif // TVec2D_h__