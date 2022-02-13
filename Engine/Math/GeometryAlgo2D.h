#ifndef Geometry2d_h__
#define Geometry2d_h__

#include "Math/Vector2.h"
#include "Math/PrimitiveTest.h"

#include <vector>

namespace Geom2D
{
	using Vector2 = Math::Vector2;

	template< class T >
	struct PolyProperty
	{
		using PolyType = T;
		static void  Setup( PolyType& p, int size );
		static int   Size( PolyType const& p);
		static Vector2 const& Vertex( PolyType const& p, int idx );
		static void  UpdateVertex( PolyType& p, int idx , Vector2 const& value );
	};

	namespace Poly
	{
		template< class TPoly >
		void Setup(TPoly& p , int size ){ PolyProperty< TPoly >::Setup( p , size ); }
		template< class TPoly >
		int  Size(TPoly const& p ){ return PolyProperty< TPoly >::Size( p ); }
		template< class TPoly >
		Vector2 const& Vertex(TPoly const& p , int idx ){ return PolyProperty< TPoly >::Vertex( p , idx ); }
		template< class TPoly >
		void UpdateVertex(TPoly& p , int idx , Vector2 const& value ){ return PolyProperty< TPoly >::UpdateVertex( p , idx , value ); }
	}

	template< class TPoly >
	bool TestInSide(TPoly const& poly , Vector2 const& pos )
	{
		int num = Poly::Size( poly );
		int idxPrev = num - 1;
		unsigned count = 0;
		for( int i = 0 ; i < num ; idxPrev = i , ++i )
		{
			Vector2 const& prev = Poly::Vertex( poly , idxPrev );
			Vector2 const& cur = Poly::Vertex( poly , i );
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


	template< class TPoly >
	bool IsConvex(TPoly const& poly)
	{
		int num = Poly::Size(poly);
		if (num <= 2)
		{
			return false;
		}

		int idxPrev = num - 1;
		unsigned count = 0;
		Vector2 dirPrev = Poly::Vertex(poly, 0) - Poly::Vertex(poly, num - 1);
		Vector2 dirCur = Poly::Vertex(poly, 1) - Poly::Vertex(poly, 0);
		float ang = dirPrev.cross(dirCur);

		for( int i = 1; i < num; idxPrev = i, ++i )
		{
			dirPrev = dirCur;
			dirCur = Poly::Vertex(poly, (i + 1) % num) - Poly::Vertex(poly, i);

			if (ang * dirPrev.cross(dirCur) < 0)
			{
				return false;
			}
		}

		return true;
	}

	template< class T1 , class T2 , class T3 >
	void MinkowskiSum( T1 const& polyA , T2 const& polyB , T3& result )
	{
		int numA = Poly::Size( polyA );
		int numB = Poly::Size( polyB );
		assert( numA >= 2 && numB >= 2 );

		std::vector< Vector2 > offsets;
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
			Vector2 const& offset = offsets[ num - 1 ];
			Vector2 const& offsetNext = offsets[ numA ];

			for( idxAStart = 0 ; idxAStart < numA ; ++idxAStart )
			{
				Vector2 const& offsetTest = offsets[idxAStart];
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
		Vector2 v = Poly::Vertex( polyA , idxAStart );
		for( int i = 0 ; i < num ; ++i )
		{
			v += offsets[ idxVertex[i] ];
			Poly::UpdateVertex( result , i , v );
		}
	}

	template< class TPoly >
	class QHullSolver
	{
	private:
		TPoly const* mPoly;
		int*   mOutIndices;
		Vector2 const& getVertex( int idx ){ return Poly::Vertex( *mPoly , idx ); }
	public:

		int solve(TPoly const& poly, int outHullIndices[])
		{
			int numV = Poly::Size( poly );
			if ( numV <= 3 )
			{
				for( int i = 0 ; i < numV ; ++i )
					outHullIndices[i] = i;
				return numV;
			}

			assert( numV > 3 );

			Vector2 v0 = Poly::Vertex( poly , 0 );
			float xP[2] = { v0.x , v0.x };
			float yP[2] = { v0.y , v0.y };
			int xI[2] = { 0 , 0 };
			int yI[2] = { 0 , 0 };
			for( int i = 1 ; i < numV ; ++i )
			{
				Vector2 const v = Poly::Vertex( poly , i );
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
			mOutIndices = outHullIndices;
			int num;
			int idxQuad[5] = { xI[0] , yI[0] , xI[1] , yI[1] , xI[0] };

			for( int i = 0 ; i < 4 ; ++i )
			{
				int i1 = idxQuad[i];
				int i2 = idxQuad[i+1];
				if ( i1 != i2 )
				{
					*mOutIndices++ = i1;
					if ( nV )
					{
						num = constuct_R( pIdx , nV , i1 , i2 );
						pIdx += num;
						nV -= num;
					}
				}
			}
			return mOutIndices - outHullIndices;
		}
		int  constuct_R( int pIdx[] , int nV , int i1 , int i2 )
		{
			assert( nV > 0 );

			int num = clip( pIdx , nV , i1 , i2 );

			switch( num )
			{
			case 0: return 0;
			case 1: *mOutIndices++ = pIdx[0]; return 1;
			default:
				{
					int iMax = pIdx[0];
					int n = num - 1;
					int* pTemp = pIdx + 1;
					int temp = constuct_R( pTemp  , n , i1 , iMax );
					n -= temp;
					*mOutIndices++ = iMax;
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

			Vector2 const& v2 = getVertex(i2); 
			Vector2 d = getVertex(i1) - v2;

			float valMax = 0;
			int*  itMax = nullptr;

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

	template< class TPoly >
	inline int QuickHull(TPoly const& poly , int outHullIndices[] )
	{
		QHullSolver< TPoly > solver;
		return solver.solve( poly , outHullIndices );
	}


	template< class TConvexVertexProvider >
	inline void CalcMinimumBoundingRectangleImpl(TConvexVertexProvider&& provider, int numHullVertex , Vector2& outCenter, Vector2& outAxisX, Vector2& outSize)
	{
		// rotating calipers algorithm
		if( numHullVertex <= 3 )
		{
			switch( numHullVertex )
			{
			case 0:
				outCenter = Vector2::Zero();
				outSize = Vector2::Zero();		
				outAxisX = Vector2::Zero();
				break;
			case 1:
				outCenter = provider(0);
				outSize = Vector2::Zero();
				outAxisX = Vector2::Zero();
				break;
			case 2:
				{
					Vector2 const& v0 = provider(0);
					Vector2 const& v1 = provider(1);
					outCenter = 0.5 * (v0 + v1);
					Vector2 dir = v1 - v0;
					float dist = dir.normalize();
					outSize = Vector2(dist, 0);
					outAxisX = dir;
				}
				break;
			case 3:
				{
					float maxDot = 0;
					int indexStart = -1;
					for( int i = 0; i < 3; ++i )
					{
						Vector2 const& v0 = provider(i);
						Vector2 const& v1 = provider((i+1)%3);
						
						Vector2 dir = v1 - v0;
						float len = dir.normalize();
						float dotAbs = Math::Abs( dir.dot(Vector2(1, 0)) );
						if ( dotAbs > maxDot )
						{
							Vector2 const& v2 = provider((i + 2) % 3);
							if ( dir.dot( v2 - v0 ) >= 0 &&
								 dir.dot( v1 - v2 ) >= 0 )
							{
								outAxisX = dir;
								outSize.x = len;
								indexStart = i;
								maxDot = dotAbs;
							}
						}
					}

					assert(indexStart != -1);

					Vector2 const& v0 = provider(indexStart);
					Vector2 const& v2 = provider((indexStart + 2) % 3);
					Vector2 dir = v2 - v0;
					outSize.y = outAxisX.cross(dir);
					outCenter = v0 + 0.5 * (outSize.x * outAxisX + outSize.y * Math::Perp(outAxisX));
				}
				break;
			}
			return;
		}
		enum ERectSide
		{
			LEFT, BOTTOM, RIGHT,TOP,
		};
		ERectSide const NextSide[] = { BOTTOM , RIGHT , TOP , LEFT };
		ERectSide const PairSide[] = { RIGHT , TOP , LEFT , BOTTOM };
		int boundIndices[4];
		{
			Vector2 minValue = Vector2(Math::MaxFloat, Math::MaxFloat);
			Vector2 maxValue = Vector2(-Math::MaxFloat, -Math::MaxFloat);
			for( int i = 0; i < numHullVertex; ++i )
			{
				Vector2 const& v = provider(i);

				if( v.x < minValue.x )
				{
					boundIndices[LEFT] = i;
					minValue.x = v.x;
				}
				if( v.x > maxValue.x )
				{
					boundIndices[RIGHT] = i;
					maxValue.x = v.x;
				}
				if( v.y < minValue.y )
				{
					boundIndices[BOTTOM] = i;
					minValue.y = v.y;
				}
				if( v.y > maxValue.y )
				{
					boundIndices[TOP] = i;
					maxValue.y = v.y;
				}
			}
		}
		Vector2 sideDirs[4];
		sideDirs[LEFT]   = Vector2(0,-1);
		sideDirs[RIGHT]  = Vector2(0, 1);
		sideDirs[BOTTOM] = Vector2(1, 0);
		sideDirs[TOP]    = Vector2(-1, 0);

		auto GetVertex = [&](ERectSide side) -> Vector2
		{
			return provider(boundIndices[side]);
		};
		auto GetNextVertex = [&](ERectSide side) -> Vector2
		{
			return provider((boundIndices[side] + 1) % numHullVertex );
		};

		auto GetInterection = [&](ERectSide s1, ERectSide s2) -> Vector2
		{
			Vector2 result;
			bool bHad = Math::LineLineTest(GetVertex(s1), sideDirs[s1], GetVertex(s2), sideDirs[s2], result);
			assert(bHad);
			return result;
		};


		float bestArea = Math::MaxFloat;
		for( int n = 0 ; n < numHullVertex ; ++n )
		{
			float maxDotAngle = -Math::MaxFloat;
			int sideSelected = -1;
			Vector2 dirSelected;
			for( int i = 0; i < 4; ++i )
			{
				Vector2 const& v0 = GetVertex(ERectSide(i));
				Vector2 const& v1 = GetNextVertex(ERectSide(i));
				Vector2 edgeDir = Math::GetNormal(v1 - v0);
				float dotAngle = edgeDir.dot(sideDirs[i]);
				if( dotAngle > maxDotAngle )
				{
					maxDotAngle = dotAngle;
					sideSelected = i;
					dirSelected = edgeDir;
				}
			}

			assert(sideSelected != -1);
			sideDirs[sideSelected] = dirSelected;
			sideDirs[PairSide[sideSelected]] = -dirSelected;
			ERectSide nextSide = NextSide[sideSelected];
			sideDirs[nextSide] = Math::Perp(dirSelected);
			sideDirs[PairSide[nextSide]] = -sideDirs[nextSide];

			boundIndices[sideSelected] = (boundIndices[sideSelected] + 1) % numHullVertex;

#if 1
			float width = sideDirs[BOTTOM].dot(GetVertex(RIGHT) - GetVertex(LEFT));
			float height = sideDirs[RIGHT].dot(GetVertex(TOP) - GetVertex(BOTTOM));
			float area = width * height;
			if( area < bestArea )
			{
				outSize = Vector2(width, height);
				outCenter = GetInterection(LEFT, BOTTOM) + 0.5 * (width * sideDirs[BOTTOM] + height * sideDirs[RIGHT]);
				outAxisX = sideDirs[BOTTOM];
				bestArea = area;
			}
#else
			Vector2 LT = GetInterection(LEFT, TOP);
			Vector2 RT = GetInterection(RIGHT, TOP);
			Vector2 LB = GetInterection(LEFT, BOTTOM);

			float width = Math::Distance(LT, RT);
			float height = Math::Distance(LT, LB);
			float area = width * height;
			if( area < bestArea )
			{
				outCenter = 0.5 * (RT + LB);
				outSize = Vector2(width, height);
				outAxisX = Math::GetNormal(RT - LT);
				bestArea = area;
			}
#endif
		} 
	}

	template< class TPoly >
	inline void CalcMinimumBoundingRectangle(TPoly const& poly, int hullIndices[], int numHullVertex, Vector2& outCenter, Vector2& outAxisX, Vector2& outSize)
	{
		auto Provider = [&](int index) -> Vector2 const&
		{
			return Poly::Vertex(poly, hullIndices[index]);
		};
		CalcMinimumBoundingRectangleImpl(Provider, numHullVertex, outCenter, outAxisX, outSize);
	}

	template< class TPoly >
	inline void CalcMinimumBoundingRectangle(TPoly const& poly, Vector2& outCenter, Vector2& outAxisX, Vector2& outSize)
	{
		std::vector< int > hullIndices(Poly::Size(poly));
		QHullSolver< TPoly > solver;
		int numHullVertex = solver.solve(poly, hullIndices.data());

		auto Provider = [&](int index) -> Vector2 const&
		{
			return Poly::Vertex(poly, hullIndices[index]);
		};
		CalcMinimumBoundingRectangleImpl(Provider, numHullVertex, outCenter, outAxisX, outSize);
	}


	template< class TPoly >
	inline void CalcMinimumBoundingRectangleForConvex(TPoly const& poly, Vector2& outCenter, Vector2& outAxisX, Vector2& outSize)
	{
		assert(IsConvex(poly));
		auto Provider = [&](int index) -> Vector2 const&
		{
			return Poly::Vertex(poly, index);
		};
		CalcMinimumBoundingRectangleImpl(Provider, Poly::Size(poly), outCenter, outAxisX, outSize);
	}

}
#endif // Geometry2d_h__
