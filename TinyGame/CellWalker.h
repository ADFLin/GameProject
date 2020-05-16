#ifndef CellWalker_h__
#define CellWalker_h__

#include "TVector2.h"
#include "TVector3.h"

//#TODO
class CellWalker
{
public:
	typedef TVector3< float > Vec3f;
	typedef TVector3< int > Vec3i;


	int init( Vec3f const& from , Vec3f const& to )
	{
		tpFrom = Vec2i( Math::Floor( from.x ) , Math::Floor( from.y ) , Math::Floor( from.z ) );
		tpCur  = tpFrom;
		tpTo   = Vec3i( Math::Floor( to.x ) , Math::Floor( to.y ), Math::Floor( to.z ) );
		tpDif  = tpTo - tpFrom;

		frac = from  - Vec3f( tpFrom );
		Vec3f dif = to - from;

		int zeroCount = 0;
		zeroCount += ( dif.x == 0 ) ? 1 : 0;
		zeroCount += ( dif.y == 0 ) ? 1 : 0;
		zeroCount += ( dif.z == 0 ) ? 1 : 0;

		switch( zeroCount )
		{
		case 1: 
			{
				funcNext = &CellWalker::getNext1;
				idxWork[0] = ( dif.x != 0 ) ? 0 : ( ( dif.y != 0 ) ? 1 : 2 );
			}
			break;
		case 2: 
			{
				funcNext = &CellWalker::getNext2;
				int idx = ( dif.x == 0 ) ? 0 : ( ( dif.y == 0 ) ? 1 : 2 );
				idxWork[0] = ( idx + 1 ) % 3;
				idxWork[1] = ( idx + 2 ) % 3;
				
			}
			break;
		case 3: 
			{
				funcNext = &CellWalker::getNext3;

				float slopeFactor[0] = dif.x / dif.z;
				if ( slopeFactor[0] < 0 )
					slopeFactor[0] = -slopeFactor[0];
				float slopeFactor[1] = dif.y / dif.z;
				if ( slopeFactor[1] < 0 )
					slopeFactor[1] = -slopeFactor[1];

				for( int i = 0 ; i < 3 ; ++i )
				{
					if ( tpDif[i] > 0 )
					{
						frac[i] = 1 - frac[i];
						delta[i] = 1;
					}
					else
					{
						delta[i] = -1;
					}
				}
			}
			break;
		}

		return zeroCount;
	}
	bool  haveNext(){ return tpCur != tpTo; }
	void  getNext() { (this->*funcNext )(); }


	typedef void ( CellWalker::*FunNext )();
	FunNext funcNext;

	void  getNext1()
	{

	}
	void  getNext2()
	{

	}
	void  getNext3()
	{
		float xOff = frac.x * slopeFactorX;

		int testCase = 0;
		if ( frac.x > xOff )
		{
			frac.y -= yOff;
			frac.x = 1;
			tpCur.x += deltaX;
		}
		else
		{
			float yOff = frac.y * slopeFactorY;
			if ( frac.y > yOff )
			{

			}
			else
			{
				frac.x -= frac.z / slopeFactorX;
				frac.y -= frac.z / slopeFactorY;
				frac.z = 1;
				tpCur.z += delta.z;
			}
		}

		switch( testCase )
		{
		case 0:
		case 1:
		case 2:
		}

	}
	Vec3i const& getPos(){ return tpCur; }

	float slopeFactor[2];
	int   idxWork[2];

	Vec3i tpCur;
	Vec3i tpFrom;
	Vec3i tpTo;
	Vec3i tpDif;

	Vec3i delta;
	Vec3f frac;
};

#endif // CellWalker_h__
