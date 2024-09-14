#ifndef Rect_h__028D948C_C13C_4C3B_BADB_C6A4B8405647
#define Rect_h__028D948C_C13C_4C3B_BADB_C6A4B8405647

#include "Math/TVector2.h"

template< class T >
struct TRect
{
	typedef TVector2< T > CoordType;
	CoordType pos;
	CoordType size;

	CoordType const& getSize() const { return size; }

	TRect() = default;

	bool isValid() const
	{
		return size.x > 0 && size.y > 0;
	}

	static TRect Intersect(TRect const& r1, TRect const& r2)
	{
		CoordType min = r1.pos.max(r2.pos);
		CoordType max = (r1.pos + r1.size).min(r2.pos + r2.size);
		return { min , max - min };
	}

	bool operator != (TRect const& rhs) const
	{
		return pos != rhs.pos || size != rhs.size;
	}

#if 0
	TRect( CoordType const& min , CoordType const& max )
		:min(min),max(max){}

	bool isIntersect( TRect const& a ) const
	{
		if( max.x < a.min.x || max.y < a.min.y ||	
			min.x > a.max.x || min.y > a.max.y ) 
			return false;		
		return true;
	}

	bool isIntersect( TRect const& a , CoordType const& offset ) const
	{
		if( max.x < a.min.x + offset.x || max.y < a.min.y + offset.y ||	
			min.x > a.max.x + offset.x || min.y > a.max.y + offset.y ) 
			return false;

		return true;
	}

	bool isInside( CoordType const& p ) const
	{
		return min.x < p.x && p.x < max.x &&
			   min.y < p.y && p.y < max.y ;
	}

	bool  overlap( TRect const& other )
	{
		T xMin = std::max( min.x, other.min.x );
		T yMin = std::max( min.y, other.min.y );
		T xMax = std::min( max.x , other.max.x );
		T yMax = std::min( max.y , other.max.y );

		if ( xMax  >= xMin && yMax >= yMin )
		{
			min.x = xMin;
			min.y = yMin;
			max.x = xMax;
			max.y = yMax;
			return true;
		}
		return false;
	}
#endif
};

#endif // Rect_h__028D948C_C13C_4C3B_BADB_C6A4B8405647
