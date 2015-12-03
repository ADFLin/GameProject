#ifndef QixLevel_h__
#define QixLevel_h__

#include "TVector2.h"

namespace Qix
{

	typedef TVector2< int > Vec2i;



	class CLine
	{
	public:
		CLine()
		{
			mBefore = mNext = 0;
		}
		Vec2i const& getFrom() const { assert( mBefore ); return mBefore->mPos; }
		Vec2i const& getTo()   const { return mPos; }


		//void setFrom( Vec2i const& pt ){ assert( mBefore ); mBefore->mPos = pt; }
		//void setTo  ( Vec2i const& pt ){ mPos = pt; }

		void     conFront( CLine& line , Vec2i const& pos )
		{
			mNext = &line;
			line.mBefore = this;
			mPos = pos;
		}

		void     conBack( CLine& line , Vec2i const& pos )
		{
			mBefore = &line;
			line.mNext = this;
			line.mPos = pos;
		}

		CLine* next(){ return mNext; }
		CLine* before(){ return mBefore; }
	private:

		CLine* mBefore;
		CLine* mNext;
		Vec2i  mPos;
	};



	class Region
	{
	public:

		Region( Vec2i const& from , Vec2i const& to );
		~Region();
		int  getArea() const { return mArea; }
		int  addArea( Vec2i pts[] , int num , CLine& startTouch , CLine& endTouch )
		{
			if ( &startTouch == &endTouch )
			{
				return addAreaSide( pts , num , startTouch );
			}
			return 0;
		}
	private:
		int addAreaSide( Vec2i pts[] , int num , CLine& sideTouch );
	
		int    mArea;
		CLine* mBounds;
	};


	class Level
	{








	};

}//namespace Qix

#endif // QixLevel_h__
