#ifndef HMAlgo_h__
#define HMAlgo_h__

#include "common.h"
#include "TPathFinding.h"
#include "FlyModelFileLoader.h"

#include <iostream>
#include <cstdlib>
using namespace std;


struct edge_t
{
	int v[2];
	int tID[2];

	bool isInclued( int vt )
	{
		return v[0] == vt || v[1] == vt;
	}

	int getOtherV( int vt )
	{
		assert( isInclued(vt) );
		if ( v[0] == vt ) 
			return v[1];
		else 
			return v[0];
	}
	int getOtherT( int t )
	{
		if ( tID[0] == t )
			return tID[1];
		else
			return tID[0];
	}

	int getOtherV( edge_t& conEdge )
	{
		if ( conEdge.v[0] == v[0] || 
			 conEdge.v[1] == v[0] )
			return v[1];
		else
		{
			assert( conEdge.v[0] == v[1] || 
			        conEdge.v[1] == v[1] );
			return v[0];
		}
	}
};

struct triangle_t
{
	int e[3];
	int v[3];
	int polyID;



	int getEdgeIndex( int et )
	{
		for( int i =0;i < 3 ; ++i )
		{
			if ( e[i] == et )
				return i;
		}
		return -1;
	}

	int getNV( edge_t& edge )
	{
		if ( !edge.isInclued(v[0]) )
			return v[0];

		if ( !edge.isInclued(v[1]) )
			return v[1];

		return v[2];
	}
};



#include <vector>
struct polygon_t
{
	bool needUpdate;
	std::vector< int > triVec;
	std::vector< int > e;
	std::vector< int > v;

	int getEdgeIndex( int ei )
	{
		for( int i =0;i < e.size() ; ++i )
		{
			if ( e[i] == ei )
				return i;
		}
		return -1;
	}

};



#define CROSS( v1 , v2 ) ( v1.x * v2.y - v1.y * v2.x )
#define NEXT_VAL( val , size ) ( ( val + 1 ) % size )
#define BEFORE_VAL( val , size ) ( ( val + size - 1 ) % size )
#define GO_NEXT( val , size ) ( val = ( val + 1 ) % size )
#define GO_BEFORE( val , size ) ( val = ( val + size - 1 ) % size )


class HMAlgo
{
public:

	struct SortV
	{
		SortV( int p1, int p2 )
		{
			if ( p1 > p2 )
			{
				v1 = p1;  v2 = p2;
			}
			else
			{
				v1 = p2;  v2 = p1;
			}
		}
		int v1,v2;
	}; 

	typedef std::vector< SortV > EMMap;

	void load( float* vertex , int NumVertex , int vertexOffset , int* triIndex , int NumTri )
	{
		vtxVec.reserve( NumVertex );
		for( int i = 0 ; i < NumVertex ; ++ i )
		{
			vtxVec.push_back( vertex_t() );
			vertex_t& vtx = vtxVec.back();

			float* pv = &vertex[ vertexOffset * i ];
			vtx.x = pv[0];
			vtx.y = pv[1];
			vtx.z = pv[2];
		}

		int eID = 0;

		EMMap eMap;
		triVec.reserve( NumTri );

		int numCurrentTri = 0;

		for( int i = 0 ; i < NumTri ; ++ i )
		{
			int* pv = &triIndex[3*i];

			FyVec3D v1 = makeVec( pv[0] , pv[1] );
			FyVec3D v2 = makeVec( pv[0] , pv[2] );

			if ( CROSS( v1, v2 ) < 0)
				continue;

			triVec.push_back( triangle_t() );
			triangle_t& tri = triVec.back();

			tri.polyID = -1;
			
			tri.v[0] = pv[0];
			tri.v[1] = pv[1];
			tri.v[2] = pv[2];

			//FyVec3D v1 = makeVec( tri.v[0] , tri.v[1] );
			//FyVec3D v2 = makeVec( tri.v[0] , tri.v[2] );

			//if ( CROSS( v1, v2 ) < 0)
			//{
			//	std::swap( tri.v[1] , tri.v[2] );
			//}

			//tri.e[0] = buildEdge( eMap, i , SortV( tri.v[0] , tri.v[1] ) );
			//tri.e[1] = buildEdge( eMap, i , SortV( tri.v[1] , tri.v[2] ) );
			//tri.e[2] = buildEdge( eMap, i , SortV( tri.v[2] , tri.v[0] ) );


			tri.e[0] = buildEdge( eMap, numCurrentTri , SortV( tri.v[0] , tri.v[1] ) );
			tri.e[1] = buildEdge( eMap, numCurrentTri , SortV( tri.v[1] , tri.v[2] ) );
			tri.e[2] = buildEdge( eMap, numCurrentTri , SortV( tri.v[2] , tri.v[0] ) );

			++numCurrentTri;

		}

		checkLoad();
	}
	void load( TerrainInfo& info )
	{
		load( info.vertex , info.numVertex , info.vertexLen ,
			  info.face , info.numFace );
	}

	void load( FlyModelDataInfo& info )
	{
		load( (Float*)info.vertex , info.NumVertex , sizeof( Vec3D )/sizeof(Float) , 
			   info.triIndex , info.NumTri );
	}

	int getBNVertex( polygon_t& poly , int e , int* t )
	{
		int size = poly.e.size();
		int index = poly.getEdgeIndex( e );

		edge_t& edge = edgeVec[e];

		int en = poly.e[ NEXT_VAL( index , size) ];
		int eb = poly.e[ BEFORE_VAL( index , size) ];

		t[0] = edgeVec[eb].getOtherV( edge );
		t[1] = edgeVec[en].getOtherV( edge );

		return index;
	}


	int buildEdge( EMMap& eMap, int ti , SortV& e )
	{

		for ( int i = 0 ; i < edgeVec.size() ; ++ i)
		{
			edge_t& edge = edgeVec[i];
			if ( e.v1 == edge.v[0] && e.v2 == edge.v[1] )
			{
				edge_t& edge = edgeVec[i];
				edge.tID[1] = ti;
				return i;
			}
		}
	
		edgeVec.push_back( edge_t() );
		edge_t& edge = edgeVec.back();

		edge.tID[0] = ti;
		edge.tID[1] = -1;
		edge.v[0] = e.v1;
		edge.v[1] = e.v2;

		//eMap[e.v1][e.v2] =  edgeVec.size()-1;

		return edgeVec.size()-1;
	}

	void build()
	{
		int manPolyTriNum = 2;

		for(int i = 0; i < edgeVec.size(); ++i )
		{
			edge_t& edge = edgeVec[i];

			int index = mergeEdge( i );

			while ( index != -1 )
			{
				polygon_t& poly = polyVec[ index ];

				if ( poly.triVec.size() > manPolyTriNum )
					break;
				
				if ( poly.e.size() == 0 )
					break;

				int pes = rand() % poly.e.size();
				for ( int j = 0 ; j < poly.e.size(); ++j )
				{
					index = mergeEdge( poly.e[ (j + pes ) % poly.e.size() ] );
					if ( index != -1)
						break;
				}
			}
		}

		for ( int time = 0 ; time < 10 ; ++ time )
		{
			manPolyTriNum *= 2;

			for(int i = 0; i < polyVec.size(); ++i )
			{
				polygon_t& poly = polyVec[i];

				int index;

	
					if ( poly.triVec.size() > manPolyTriNum )
						break;

					for ( int j = 0 ; j < poly.e.size(); ++j )
					{
						index = mergeEdge( poly.e[j] );
	/*					if ( index != -1)
							break;*/
					}
			}
		}


	}

	int mergeEdge( int eID )
	{
		edge_t& edge = edgeVec[eID];

		if ( edge.tID[0] == -1 || edge.tID[1] == -1 )
			return -1;

		triangle_t& tr0 = triVec[ edge.tID[0] ];
		triangle_t& tr1 = triVec[ edge.tID[1] ];

		int lv[2],rv[2];

		if ( tr0.polyID < 0 && tr1.polyID < 0 )
		{
			return mergeTriTri( edge.tID[0] , edge.tID[1] , eID );
		}

		if ( tr0.polyID >= 0 && tr1.polyID < 0 )
		{
			return mergePolyTri( tr0.polyID , edge.tID[1] , eID );
		}

		if ( tr0.polyID < 0 && tr1.polyID >= 0 )
		{
			return mergePolyTri( tr1.polyID , edge.tID[0] , eID );
		}

		if ( tr0.polyID >= 0 && tr1.polyID >= 0 )
		{
			return mergePolyPoly( tr0.polyID  , tr1.polyID , eID );
		}
		return -1;
	}

	int mergePolyPoly( int p1 , int p2 , int em )
	{
		if ( p1 == p2 )
			return -1;

		edge_t& edge = edgeVec[em];
		polygon_t& poly1 = polyVec[p1];
		polygon_t& poly2 = polyVec[p2];



		//showFullInfo( poly1 );
		//showFullInfo( poly2 );

		int vL[2],vR[2],ve[2];

		int size1 = poly1.e.size();
		int size2 = poly2.e.size();

		int peL = getBNVertex( poly1 , em , vL );
		int peR = getBNVertex( poly2 , em , vR );

		int enl = poly1.e[ NEXT_VAL( peL , size1 ) ];
		getVe( ve , em , enl );


		bool isTopCon  =  ( poly1.e[ NEXT_VAL( peL , size1 ) ] == 
			                poly2.e[ BEFORE_VAL( peR , size2 ) ] );
		bool isDownCon =  ( poly1.e[ BEFORE_VAL( peL , size1 ) ] == 
			                poly2.e[ NEXT_VAL( peR , size2 ) ] );

		if ( isTopCon && isDownCon )
			return -1;

		int len = 1;

		if ( isTopCon )
		{
			int pe1 = peL;
			int pe2 = peR;

			int e1,e2;
			while (1)
			{
				e1 =  poly1.e[ GO_NEXT( pe1 , size1 ) ];
				e2 =  poly2.e[ GO_BEFORE( pe2 , size2 ) ];

				if ( e1 != e2 )
					break;

				++len;
			}

			int v1 = edgeVec[ e1 ].getOtherV( edgeVec[ e2 ] );
			int v2 = edgeVec[ e2 ].getOtherV( edgeVec[ e1 ] );

			peL = BEFORE_VAL( pe1 , size1 );

			vL[1] =  v1;
			vR[0] =  v2;

			ve[1] =   edgeVec[e1].getOtherV( v1 );

		}


		if ( isDownCon )
		{
			int pe1 = peL;
			int pe2 = peR;

			int e1,e2;
			while (1)
			{
				e1 =  poly1.e[ GO_BEFORE( pe1 , size1 ) ];
				e2 =  poly2.e[ GO_NEXT( pe2 , size2 ) ];

				if ( e1 != e2 )
					break;

				++len;
			}

			int v1 = edgeVec[ e1 ].getOtherV( edgeVec[ e2 ] );
			int v2 = edgeVec[ e2 ].getOtherV( edgeVec[ e1 ] );

			peR = BEFORE_VAL( pe2 , size2 );

			vL[0] =  v1;
			vR[1] =  v2;

			ve[0] =   edgeVec[e1].getOtherV( v1 );

		}

		if ( len == 1 && poly1.triVec.size() > 2 || poly2.triVec.size() > 2 )
			return -1;

		if ( testCanMerge( vL , vR , ve ) )
		{
			//checkPoly( poly1 );
			//checkPoly( poly2 );

			//showFullInfo( poly1 );
			//showFullInfo( poly2 );

			for( int i =0 ; i < poly2.triVec.size() ; ++ i)
			{
				triangle_t& tri = triVec[ poly2.triVec[i] ];
				tri.polyID = p1;
			}

			poly1.triVec.insert( poly1.triVec.end() , poly2.triVec.begin() , 
				                                      poly2.triVec.end() );


			bufferInternal.clear();

			for (int i = 0 ; i < size2 - len ;  ++ i )
			{
				bufferInternal.push_back( poly2.e[ GO_NEXT( peR , size2 ) ] );
			}

			for (int i = 0 ; i < size1 - len ;  ++ i )
			{
				bufferInternal.push_back( poly1.e[ GO_NEXT( peL , size1 ) ] );
			}

			poly1.e.swap( bufferInternal );

			freePoly( p2 );

			checkPoly( poly1 );
			return p1;
		}

		return -1;
	}

	int mergePolyTri( int p1 , int t2 , int em )
	{
		edge_t& edge = edgeVec[em];
		polygon_t&  poly1 = polyVec[p1];
		triangle_t& tri2 =  triVec[t2];

		if ( poly1.e.size() >= 4 )
			return -1;

		int vL[2],vR[2],ve[2];

		int size1 = poly1.e.size();
		int size2 = 3;

		int peL = getBNVertex( poly1 , em , vL );
		
		vR[0] = tri2.getNV( edge );
		vR[1] = vR[0];

		int enl = poly1.e[ NEXT_VAL( peL , size1 ) ];
		getVe( ve , em , enl );

		if ( testCanMerge( vL , vR , ve ) )
		{
			tri2.polyID = p1;

			poly1.triVec.insert( poly1.triVec.end() , 1 , t2 );

			int re = tri2.getEdgeIndex( em );
			bufferInternal.clear();

			bufferInternal.push_back( tri2.e[ GO_NEXT( re , 3 )] );
			bufferInternal.push_back( tri2.e[ GO_NEXT( re , 3 )] );

			int size1 = poly1.e.size();
			for (int i = 0 ; i < size1 - 1 ;  ++ i )
			{
				bufferInternal.push_back( poly1.e[ GO_NEXT( peL , size1 )] );
			}

			poly1.e.swap( bufferInternal );

			checkPoly( poly1 );

			return p1;
		}

		return -1;
	}


	int mergeTriTri( int t1 , int t2 , int em )
	{
		edge_t& edge = edgeVec[em];
		triangle_t& tri1 =  triVec[t1];
		triangle_t& tri2 =  triVec[t2];

		int vL[2],vR[2],ve[2];

		vL[0] = tri1.getNV( edge );
		vL[1] = vL[0];

		vR[0] = tri2.getNV( edge );
		vR[1] = vR[0];

		int etL = tri1.getEdgeIndex( em );
		int enl = tri1.e[ NEXT_VAL( etL , 3 ) ];
		getVe( ve , em , enl );

		if ( testCanMerge( vL , vR , ve ) )
		{
			int p = allocPoly();

			polygon_t& poly = polyVec[p];

			tri1.polyID = p;
			tri2.polyID = p;

			poly.triVec.push_back( t1 );
			poly.triVec.push_back( t2 );

			int teL = tri1.getEdgeIndex( em );
			int teR = tri2.getEdgeIndex( em );

			poly.e.push_back( tri2.e[ GO_NEXT( teR , 3 )] );
			poly.e.push_back( tri2.e[ GO_NEXT( teR , 3 )] );

			poly.e.push_back( tri1.e[ GO_NEXT( teL , 3 )] );
			poly.e.push_back( tri1.e[ GO_NEXT( teL , 3 )] );

			if ( !checkPoly( poly ) )
			{

			}

			return p;
		}

		return -1;
	}

	void getVe( int* ve , int e , int enl )
	{
		edge_t& edge = edgeVec[ e ];
		edge_t& edgeNL = edgeVec[ enl ];

		if ( edgeNL.isInclued( edge.v[0] ) )
		{
			ve[0] = edge.v[1];
			ve[1] = edge.v[0];
		}
		else
		{
			ve[0] = edge.v[0];
			ve[1] = edge.v[1];
		}

	}


	void showFullInfo(polygon_t& poly )
	{
		std::vector<edge_t> eVec;
		cout << "edge =";
		for(int i=0 ; i< poly.e.size(); ++i )
		{
			edge_t& tEdge = edgeVec[ poly.e[i] ];

			cout << "(" << tEdge.v[0] << " " << tEdge.v[1] << ")";
			eVec.push_back( tEdge );
		}
		cout <<endl;

		std::vector<triangle_t> tVec;
		for(int i=0 ; i< poly.triVec.size(); ++i )
		{
			triangle_t& tri = triVec[ poly.triVec[i] ];
			tVec.push_back( tri );
		}

	}

	bool testCanMerge( int* vL , int* vR , int* ve )
	{
		bool t1 = testDir( makeVec( vL[0] , ve[0] ) , 
			               makeVec( ve[0] , ve[1] ) , 
			               makeVec( ve[0] , vR[1] ) );

		bool t2 = testDir( makeVec( vR[0] , ve[1] ) , 
			               makeVec( ve[1] , ve[0] ) , 
			               makeVec( ve[1] , vL[1] ) );

		return t1 && t2;
	}


	bool testCanMerge( int* lv , int* rv , edge_t& edge )
	{
		bool t1 = testDir( makeVec( lv[0] , edge.v[0] ) , 
			               makeVec( edge.v[0] , edge.v[1] ) , 
			               makeVec( edge.v[0] , rv[0]  ) );

		bool t2 = testDir( makeVec( rv[1] , edge.v[1] ) , 
			               makeVec( edge.v[1] , edge.v[0] ) , 
			               makeVec( edge.v[1] , lv[1] ) );

		return t1 && t2;
	}

	bool checkPoly( polygon_t& poly )
	{
		int size = poly.e.size();
		bool isOk = true;

		for ( int i =0 ; i < size ; ++i )
		{
			edge_t& edge1 = edgeVec[ poly.e[i] ];
			edge_t& edge2 = edgeVec[ poly.e[(i+1)%size] ];

			if ( edge1.v[0] != edge2.v[0] && 
				 edge1.v[0] != edge2.v[1] &&
			     edge1.v[1] != edge2.v[0] &&
				 edge1.v[1] != edge2.v[1] )
			{
				isOk = false;
				break;
			}
		}

		if ( !isOk )
		{
			showFullInfo( poly );
		}

		return isOk;
	}


	void freePoly(int i)
	{
		polygon_t& poly = polyVec[i];

		poly.e.clear();
		poly.triVec.clear();

		noUsePoly.push_back( i );
	}

	int allocPoly()
	{
		if ( noUsePoly.empty() )
		{
			polyVec.push_back( polygon_t() );
			return polyVec.size() - 1;
		}

		int index = noUsePoly.back();
		noUsePoly.pop_back();

		return index;
	}


	void mergeTri( int t0 , int t1 )
	{
		triangle_t& tr0 = triVec[ t0 ];
		triangle_t& tr1 = triVec[ t1 ];
	}

	FyVec3D makeVec( int from ,int to )
	{
		FyVec3D out;
		out.x = vtxVec[to].x - vtxVec[from].x;
		out.y = vtxVec[to].y - vtxVec[from].y;

		return out;
	}

	bool testDir( FyVec3D& v1 , FyVec3D& v2 , FyVec3D& v3 )
	{
		int k1 = CROSS( v1 , v2 );
		int k2 = CROSS( v1 , v3 );

		return ( k1 > 0 && k2 >= 0) || ( k1 < 0 && k2 <= 0 );
	}


	int getShareEdgeTri( int triID , int edgeIdex )
	{
		triangle_t& tri = triVec[triID];
		edge_t& edge =  edgeVec[ tri.e[ edgeIdex ] ];

		if ( edge.tID[0] != triID )
			return edge.tID[0];
		else
			return edge.tID[1];
	}

	bool checkLoad()
	{
		bool isOk = true;
		for(int i=0;i< edgeVec.size();++i)
		{
			edge_t& edge = edgeVec[i];
			if ( edge.tID[0] != -1 )
			{
				triangle_t tri = triVec[ edge.tID[0] ];
				if ( tri.e[0] != i &&
					 tri.e[1] != i &&
					 tri.e[2] != i )
				{
					isOk = false;
					break;
				}
			}

			if ( edge.tID[1] != -1 )
			{
				triangle_t tri = triVec[ edge.tID[1] ];
				if ( tri.e[0] != i &&
					 tri.e[1] != i &&
					 tri.e[2] != i )
				{
					isOk = false;
					break;
				}
			}
		}


		if ( !isOk )
		{
			vtxVec.size();

		}

		for ( int i = 0 ; i < triVec.size() ; ++ i )
		{
			triangle_t& tri = triVec[i];

			for( int j = 0 ; j < 3 ; ++ j)
			{
				edge_t& edge = edgeVec[ tri.e[j] ];
				if ( edge.tID[0] != i && 
					 edge.tID[1] != i )
				{
					isOk = false;
					break;
				}

				if ( edge.v[0] != tri.v[0] &&
					 edge.v[0] != tri.v[1] &&
					 edge.v[0] != tri.v[2] )
				{
					isOk = false;
					break;
				}

				if ( edge.v[1] != tri.v[0] &&
					 edge.v[1] != tri.v[1] &&
					 edge.v[1] != tri.v[2] )
				{
					isOk = false;
					break;
				}
			}
		}

		if ( !isOk )
		{
			vtxVec.size();
		}

		return isOk;
	}


	std::vector< int >        bufferInternal;

	std::vector< int >        noUsePoly;
	std::vector< int >        noUseVertex;
	std::vector< int >        noUseEdge;

	std::vector< polygon_t >  polyVec;
	std::vector< edge_t >     edgeVec;
	std::vector< vertex_t >   vtxVec;
	std::vector< triangle_t > triVec;
};




#endif // HMAlgo_h__