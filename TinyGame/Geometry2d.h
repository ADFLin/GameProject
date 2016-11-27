#ifndef Geometry2d_h__
#define Geometry2d_h__

#include "TVector2.h"

namespace Geom2d
{
	typedef TVector2< float > Vec2f;

	template< class T >
	struct PolyProperty
	{
		typedef T PolyType;
		static void  Setup( PolyType& p, int size );
		static int   Size( PolyType const& p);
		static Vec2f const& Vertex( PolyType const& p, int idx );
		static void  UpdateVertex( PolyType& p, int idx , Vec2f const& value );
	};

	namespace Poly
	{
		template< class T >
		void Setup( T& p , int size ){ PolyProperty< T >::Setup( p , size ); }
		template< class T >
		int  Size( T const& p ){ return PolyProperty< T >::Size( p ); }
		template< class T >
		Vec2f const& Vertex( T const& p , int idx ){ return PolyProperty< T >::Vertex( p , idx ); }
		template< class T >
		void UpdateVertex( T& p , int idx , Vec2f const& value ){ return PolyProperty< T >::UpdateVertex( p , idx , value ); }

	}

	template< class T1 >
	bool TestInSide( T1 const& poly , Vec2f const& pos )
	{
		int num = Poly::Size( poly );
		int idxPrev = num - 1;
		unsigned count = 0;
		for( int i = 0 ; i < num ; idxPrev = i , ++i )
		{
			Vec2f const& prev = Poly::Vertex( poly , idxPrev );
			Vec2f const& cur = Poly::Vertex( poly , i );
			if ( ( cur.y < pos.y && pos.y <= prev.y ) ||
				( prev.y < pos.y && pos.y <= cur.y ) )
			{
				float slopInv = ( cur.x - prev.x ) / ( cur.y - prev.y );
				float x = prev.x + ( pos.y - prev.y ) * slopInv;

				if ( x > pos.x )
				{
					++count;
				}
			}
		}
		return ( count & 0x1 ) != 0 ;
	}

	template< class T1 , class T2 , class T3 >
	void MinkowskiSum( T1 const& polyA , T2 const& polyB , T3& result )
	{
		int numA = Poly::Size( polyA );
		int numB = Poly::Size( polyB );
		assert( numA >= 2 && numB >= 2 );

		std::vector< Vec2f > offsets;
		int num = numA + numB;
		offsets.resize( num );

		{
			int idx = 0;
			for( int i = 0 , prev = numA - 1 ; i < numA ; prev = i++ )
			{
				offsets[idx++] = Poly::Vertex( polyA , i ) - Poly::Vertex( polyA , prev );
			}
			for( int i = 0 , prev = numB - 1 ; i < numB ; prev = i++ )
			{
				offsets[idx++] = Poly::Vertex( polyB , i ) - Poly::Vertex( polyB , prev );
			}
		}

		std::vector< int >   idxVertex;
		idxVertex.resize( num );

		int idxAStart;
		{
			Vec2f const& offset = offsets[ num - 1 ];
			Vec2f const& offsetNext = offsets[ numA ];

			for( idxAStart = 0 ; idxAStart < numA ; ++idxAStart )
			{
				Vec2f const& offsetTest = offsets[idxAStart];
				if ( offset.cross( offsetTest ) * offsetTest.cross( offsetNext ) > 0 )
					break;
			}
		}
		assert( idxAStart < numA );

		int n = 0;
		int idxANext = idxAStart;
		int idxB = numA;
		for( int i = 0 ; i < numA ; ++i )
		{
			int idxA = idxANext;
			idxANext = ( idxA + 1 ) % numA;
			idxVertex[n++] = idxA;

			while( idxB < num )
			{
				if ( offsets[ idxB ].cross( offsets[idxANext]) < 0 )
					break;
				idxVertex[n++] = idxB;
				++idxB;
			}
		}

		while( idxB < num )
		{
			idxVertex[n++] = idxB;
			++idxB;
		}

		assert( n == num );

		Poly::Setup( result , num );
		Vec2f v = Poly::Vertex( polyA , idxAStart );
		for( int i = 0 ; i < num ; ++i )
		{
			v += offsets[ idxVertex[i] ];
			Poly::UpdateVertex( result , i , v );
		}
	}


	template< class T >
	class QHullSolver
	{
	private:
		T*     mPoly;
		int*   mOut;
		Vec2f const& getVertex( int idx ){ return Poly::Vertex( *mPoly , idx ); }
	public:
		int solve( T& poly , int outIndex[] )
		{
			int numV = Poly::Size( poly );
			if ( numV <= 3 )
			{
				for( int i = 0 ; i < numV ; ++i )
					outIndex[i] = i;
				return numV;
			}

			assert( numV > 3 );

			Vec2f v0 = Poly::Vertex( poly , 0 );
			float xP[2] = { v0.x , v0.x };
			float yP[2] = { v0.y , v0.y };
			int xI[2] = { 0 , 0 };
			int yI[2] = { 0 , 0 };
			for( int i = 1 ; i < numV ; ++i )
			{
				Vec2f const v = Poly::Vertex( poly , i );
				float x = v.x;
				float y = v.y;
				if ( xP[0] > x ){ xI[0] = i; xP[0] = x; }
				else if ( xP[1] < x ){ xI[1] = i; xP[1] = x; }
				if ( yP[0] > y ){ yI[0] = i; yP[0] = y; }
				else if ( yP[1] < y ){ yI[1] = i; yP[1] = y; }
			}

			std::vector< int > idxBuf( numV );
			int* pIdx = &idxBuf[0];
			int nV = 0;
			for( int i = 0 ; i < numV ; ++i )
			{
				if ( i != xI[0] && i != xI[1] && i != yI[0] && i != yI[1] )
				{
					pIdx[ nV ] = i;
					++nV;
				}
			}

			mPoly = &poly;
			mOut = outIndex;
			int num;
			int idxQuad[5] = { xI[0] , yI[0] , xI[1] , yI[1] , xI[0] };

			for( int i = 0 ; i < 4 ; ++i )
			{
				int i1 = idxQuad[i];
				int i2 = idxQuad[i+1];
				if ( i1 != i2 )
				{
					*mOut++ = i1;
					if ( nV )
					{
						num = constuct_R( pIdx , nV , i1 , i2 );
						pIdx += num;
						nV -= num;
					}
				}
			}
			return mOut - outIndex;
		}
		int  constuct_R( int pIdx[] , int nV , int i1 , int i2 )
		{
			assert( nV > 0 );

			int num = clip( pIdx , nV , i1 , i2 );

			switch( num )
			{
			case 0: return 0;
			case 1: *mOut++ = pIdx[0]; return 1;
			default:
				{
					int iMax = pIdx[0];
					int n = num - 1;
					int* pTemp = pIdx + 1;
					int temp = constuct_R( pTemp  , n , i1 , iMax );
					n -= temp;
					*mOut++ = iMax;
					if ( n )
					{
						pTemp += temp;
						constuct_R( pTemp  , n , iMax , i2 );
					}
				}
			}
			return num;
		}

		int clip( int pIdx[] , int nV , int i1 , int i2 )
		{
			assert( i1 != i2 );

			Vec2f const& v2 = getVertex(i2); 
			Vec2f d = getVertex(i1) - v2;

			float valMax = 0;
			int*  itMax = 0;

			int*  iter = pIdx;
			int*  itEnd = pIdx + nV;
			while( iter != itEnd )
			{
				int idx = *iter;
				assert( idx != i1 && idx != i2 );
				float val = d.cross( getVertex( idx ) - v2 );

				if ( val <= 0 )
				{
					--itEnd;
					if ( iter != itEnd )
					{
						int temp = *iter;
						*iter = *itEnd;
						*itEnd = temp;
					}
				}
				else 
				{
					if ( val > valMax )
					{
						itMax = iter;
						valMax = val;
					}
					++iter;
				}
			}
			if ( itMax && itMax != pIdx )
			{
				int temp = *itMax;
				*itMax = *pIdx;
				*pIdx = temp;
			}
			return iter - pIdx;
		}

	};

	template< class T >
	inline int QuickHull( T& poly , int outIndex[] )
	{
		QHullSolver< T > solver;
		return solver.solve( poly , outIndex );
	}




}
#endif // Geometry2d_h__
