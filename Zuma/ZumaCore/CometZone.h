#ifndef CometZone_h__
#define CometZone_h__

#include "CometBase.h"

namespace Comet
{

	static inline CoordType getRandValue( CoordType const& min , CoordType const& max , IRandom* random )
	{
		return CoordType( random->getRangeReal(min.x , max.x) , 
			              random->getRangeReal(min.y , max.y) );
	}

	class RectZone : public Zone
	{
	public:
		RectZone( CoordType const& min , CoordType const& max , IRandom* random );
		virtual CoordType calcPos()
		{
			return getRandValue( minRect , maxRect , mRandom );
		}
		CoordType minRect;
		CoordType maxRect;
		SharePtr< IRandom > mRandom;
	};

	class SphereZone : public Zone
	{
	public:
		SphereZone( float min , float max , IRandom* random );

		virtual CoordType calcPos();

		CoordType center;
		float     minRadius;
		float     maxRadius;

		SharePtr< IRandom > mRandom;
	};

	class SingleZone : public Zone
	{
	public:
		SingleZone( CoordType& pos ): mPos( pos ){}

		void   setPos( CoordType const& pos ){ mPos = pos; }
		virtual CoordType calcPos(){ return mPos; }
	private:
		CoordType mPos;
	};

}//namespace Comet 

#endif // CometZone_h__
