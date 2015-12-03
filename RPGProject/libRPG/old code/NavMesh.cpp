#include "NavMesh.h"


#include <map>

std::vector<int> g_errorNviPoly;
static std::map< unsigned , unsigned > eMap;

class VertexWelding
{
public:

	VertexWelding( int cellSize , float cellLen , btAlignedObjectArray< Vec3D >&  vtxVec)
		: m_cellSize( cellSize )
		, m_cellLen( cellLen )
		, m_vtxVec( vtxVec )
	{
		m_cell = new std::vector<int>[ m_cellSize ];
	}

	~VertexWelding()
	{
		delete [] m_cell;
	}

	int testVertex( int v , Vec3D& vtx )
	{
		int cx = int( vtx.x() / m_cellLen );
		int cy = int( vtx.y() / m_cellLen );
		int cz = int( vtx.z() / m_cellLen );

		for ( int i = -1 ; i <= 1 ; ++i )
		for ( int j = -1 ; j <= 1 ; ++j )
		for ( int k = -1 ; k <= 1 ; ++k )
		{
			unsigned cell = computeHashCellIndex( cx + i , cy + j , cz + j );
			int vT = testVertex( vtx , cell );

			if ( vT != -1)
				return vT;
		}

		unsigned cell = computeHashCellIndex( cx  , cy  , cz  );
		m_cell[cell].push_back( v );

		return -1;
	}

	int testVertex( Vec3D& vtx , unsigned cell)
	{
		std::vector<int>& vVec = m_cell[cell];
		float const threshold2 = NVI_VERTEX_WELDING_THRESHOLD * NVI_VERTEX_WELDING_THRESHOLD;
		for ( int i = 0 ; i < vVec.size() ; ++ i )
		{
			Vec3D& vTest = m_vtxVec[ vVec[i] ];

			if ( (vtx - vTest).length2() < threshold2 )
			{
				vTest =  0.5 * ( vtx + vTest );
				return vVec[i];
			}
		}
		return -1;
	}

	int computeHashCellIndex(int cx ,int cy , int cz )
	{
		const int h1 = 0x8da6b343; // Large multiplicative constants;
		const int h2 = 0xd8163841; // here arbitrarily chosen primes
		const int h3 = 0xcb1ab31f;
		int n = h1 * cx + h2 * cy + h3 * cz;
		n = n % m_cellSize;
		if (n < 0) n += m_cellSize;
		return n;
	}

	static int hashFun(int a, int b, int c)
	{
		a=a-b;  a=a-c;  a=a^(c >> 13);
		b=b-c;  b=b-a;  b=b^(a << 8); 
		c=c-a;  c=c-b;  c=c^(b >> 13);
		a=a-b;  a=a-c;  a=a^(c >> 12);
		b=b-c;  b=b-a;  b=b^(a << 16);
		c=c-a;  c=c-b;  c=c^(b >> 5);
		a=a-b;  a=a-c;  a=a^(c >> 3);
		b=b-c;  b=b-a;  b=b^(a << 10);
		c=c-a;  c=c-b;  c=c^(b >> 15);
		return c;
	}

	
	float m_cellLen;
	int   m_cellSize;

	btAlignedObjectArray< Vec3D >& m_vtxVec;
	std::vector<int>* m_cell;
};

void TerrainManager::load( float* vertex , int NumVertex , int vertexOffset , int* triIndex , int NumTri )
{
	VertexWelding vtxWeld( 40 , 500 , vtxVec );

	std::vector<int> vTerr2Walding;

	int numWelding  = 0;

	vtxVec.reserve( NumVertex );
	for( int i = 0 ; i < NumVertex ; ++ i )
	{
		float* pv = &vertex[ vertexOffset * i ];
		Vec3D vtx( pv[0] , pv[1] , pv[2] );

		int v = vtxWeld.testVertex( vtxVec.size() , vtx );

		if ( v == -1 )
		{
			vTerr2Walding.push_back( vtxVec.size() );
			vtxVec.push_back( vtx );
		}
		else
		{
			vTerr2Walding.push_back( v );
			numWelding++;
		}
	}

	int eID = 0;
	triVec.reserve( NumTri );

	eMap.clear();
	int numCurrentTri = 0;
	int conTri = 0;

	for( int i = 0 ; i < NumTri ; ++ i )
	{
		int* pv = &triIndex[3*i];

		if ( pv[0] == pv[1] || pv[0] == pv[2] || pv[1] == pv[2] )
		{
			++conTri;
			continue;
		}

		triVec.push_back( triangle_t() );
		triangle_t& tri = triVec.back();

		tri.v[0] = vTerr2Walding[ pv[0] ];
		tri.v[1] = vTerr2Walding[ pv[1] ];
		tri.v[2] = vTerr2Walding[ pv[2] ];

		tri.e[0] = buildEdge(  numCurrentTri , tri.v[0] , tri.v[1]  );
		tri.e[1] = buildEdge(  numCurrentTri , tri.v[1] , tri.v[2]  );
		tri.e[2] = buildEdge(  numCurrentTri , tri.v[2] , tri.v[0]  );

		++numCurrentTri;

	}
}

int TerrainManager::buildEdge( int ti , int v1 , int v2 )
{
	struct SortV
	{
		SortV( int _v1, int _v2 )
		{
			if ( _v1 > _v2 ){ v1 = _v1;  v2 = _v2; }
			else{ v1 = _v2;  v2 = _v1; }
		}
		unsigned getVal(){ return (v1 << 16) + v2; }
		unsigned v1,v2;
	};

	SortV e( v1, v2);


	std::map< unsigned , unsigned >::iterator iter = eMap.find( e.getVal() );

	bool needNew = true;
	if ( iter != eMap.end() )
	{
		edge_t& oldEdge = edgeVec[ iter->second ];

		if ( oldEdge.tNum == 1 )
		{
			oldEdge.t[1] = ti;
			++oldEdge.tNum;
			assert ( oldEdge.tNum == 2);
			return iter->second;
		}

	}

	edgeVec.push_back( edge_t() );
	edge_t& edge = edgeVec.back();

	edge.tNum = 1;
	edge.t[0] = ti;
	edge.t[1] = -1;
	edge.v[0] = e.v1;
	edge.v[1] = e.v2;

	int ei = edgeVec.size()-1;

	eMap.insert( std::make_pair( e.getVal() , ei ) );
	return ei;
}

bool NavMeshBuilder::checkPoly( NviPolygon& poly )
{
	int size = poly.eVec.size();
	bool isOk = true;

	for ( int i =0 ; i < size ; ++i )
	{
		int e1 = poly.eVec[i];
		int e2 = poly.eVec[(i+1)%size];

		int v = getShareEV( e1 , e2 );

		if ( v == -1 )
		{
			isOk = false;
			break;
		}
	}

	if ( !isOk )
	{
		std::vector< edge_t > checkEdgeVec;
		for ( int i =0 ; i < size ; ++i )
		{
			edge_t& edge = edgeVec[ poly.eVec[i] ];
			checkEdgeVec.push_back( edge );
		}

		checkEdgeVec.size();
	}
	return isOk;
}
void NavMeshBuilder::build()
{
	pIDVec.resize( getTNum() , -1 );

	g_errorNviPoly.clear();

	for ( int n = 0 ; n < 1 ; ++ n )
	{
		float angle = 15 * (n + 1);
		float threshold = cos( DEG2RAD(angle) );

		for( int i =0; i < getTNum() ; ++i )
		{
			if ( getTPoly( i ) != -1 )
				continue;

			Vec3D normal = computeTNormal(i);
			normal.normalize();

			if ( normal.dot( Vec3D(0,0,1) ) > threshold )
			{
				polyVec.push_back( NviPolygon() );

				NviPolygon& poly = polyVec[polyVec.size() - 1];

				poly.id = polyVec.size() - 1;

				//poly.normal = Vec3D(0,0,1);
				poly.normal = normal;
				poly.normal.normalize();

				poly.tVec.push_back( i );

				poly.eVec.push_back( getTE(i ,0) );
				poly.eVec.push_back( getTE(i ,1) );
				poly.eVec.push_back( getTE(i ,2) );

				setTPoly( i , poly.id );

				for ( int n = 0 ; n < 3 ; ++n )
				{
					int v0 = getTV( i , n );
					float d = getVertex( v0 ).dot( poly.normal );
					poly.dMax = TMax( poly.dMax , d );
					poly.dMin = TMin( poly.dMin , d );
				}

				expandPoly( poly );
				clearIntrinsicEdge( poly );
				computeVertex( poly );
				triangulate( poly );
			}
		}
	}
}

void NavMeshBuilder::expandPoly( NviPolygon& poly )
{
	assert( poly.tVec.size() == 1 );

	addExpandData( poly.tVec[0] );

	while( !exStack.empty() )
	{
		exData_t data = exStack.top();
		exStack.pop();

		if ( getTPoly( data.t ) != -1 )
			continue;

		Vec3D n = computeTNormal( data.t );
		n.normalize();

		bool needAdd = true;

		if ( n.dot( poly.normal ) < NVI_PLANE_NORMAL_THRESHOLD )
			continue;

		float tdMax = getVertex( getTV( data.t , 0 ) ).dot( poly.normal );
	    float tdMin = tdMax;
		for ( int i = 1 ; i < 3 ; ++i )
		{
			int v = getTV( data.t , i );
			float d = getVertex( v ).dot( poly.normal );
			tdMax = TMax( tdMax , d );
			tdMin = TMin( tdMin , d );
		}

		tdMax = TMax( tdMax , poly.dMax );
		tdMin = TMin( tdMin , poly.dMin );

		if ( (tdMax - tdMin ) > NVI_PLANE_MAX_OFFSET )
			continue;

		if ( needAdd )
		{
			poly.dMax = tdMax;
			poly.dMin = tdMin;


			int eiT = getTEIndex( data.t , data.e );
			assert( eiT != -1 );

			bufferInternal.clear();
			bufferInternal.push_back( getTE( data.t , GO_NEXT( eiT , 3 ) ) );
			bufferInternal.push_back( getTE( data.t , GO_NEXT( eiT , 3 ) ) );

			int eiP = poly.getEIndex( data.e );

			
			if ( eiP == -1 )
			{
				checkPoly( poly );

				int t1 = getET( data.e , 0 );
				int t2 = getET( data.e , 1 );

				triangle_t& tri1 = triVec[t1];
				triangle_t& tri2 = triVec[t2];

				assert( eiP != -1 );
			}
			

			int size1 = poly.eVec.size();
			for (int i = 0 ; i < size1 - 1 ;  ++i )
			{
				bufferInternal.push_back( poly.eVec[ GO_NEXT( eiP , size1 )] );
			}

			poly.eVec.swap( bufferInternal );

			setTPoly( data.t , poly.id );
			poly.tVec.push_back( data.t );

			addExpandData( data.t );

		}
	}

	if ( !checkPoly( poly ) )
	{
		if ( !poly.isError )
		{
			g_errorNviPoly.push_back( poly.id );
			poly.errorMsg = "expandPoly Error";
			poly.isError = true;
		}
	}
}

void NavMeshBuilder::clearIntrinsicEdge( NviPolygon& poly )
{
	int size = poly.eVec.size();

	int* prev = new int[ size ];
	int* next = new int[ size ];

	for( int i =0 ; i < size ; ++i )
	{
		prev[i] = i - 1;
		next[i] = i + 1;
	}

	prev[0] = size - 1;
	next[size - 1] = 0;

	bufferInternal.clear();

	int visitNum = 0;
	int i = 0;

	while ( visitNum <= size )
	{
		int e = poly.eVec[i];

		int t1 = getET( e , 0 );
		int t2 = getET( e , 1 );

		bool isConOuter = ( t1 == -1 )|| (t2 == -1 )|| ( getTPoly( t1 ) != getTPoly( t2 ) );

		if ( !isConOuter )
		{
			if ( e == 7 )
			{
				int ePrev =  poly.eVec[ prev[i] ];
				int eNext =  poly.eVec[ next[i] ];
			}
			int eNext = poly.eVec[ next[i] ];

			if ( e == eNext )
			{
				int inext = next[ next[i] ];
				next[ prev[i] ] = inext;
				prev[ inext ] = prev[i];

				i = prev[i];
			}
			else
			{
				i = next[i];
			}
		}
		else
		{
			i = next[i];
		}

		++visitNum;
	}

	bufferInternal.clear();

	int end = prev[i];
	while ( i != end )
	{
		bufferInternal.push_back( poly.eVec[i] );
		i = next[i];
	}

	bufferInternal.push_back( poly.eVec[i] );

	poly.eVec.swap( bufferInternal );

	delete [] next;
	delete [] prev;

	if ( !checkPoly( poly ) )
	{
		if ( !poly.isError )
		{
			g_errorNviPoly.push_back( poly.id );
			poly.errorMsg = "ClearInEdgeError";
			poly.isError = true;
		}
		poly.eVec.swap( bufferInternal );
	}
}

void NavMeshBuilder::computeVertex( NviPolygon& poly )
{
	if ( poly.isError )
		return;

	poly.d = 0.5 * ( poly.dMax + poly.dMin );

	int size = poly.eVec.size();

	btAlignedObjectArray< Vec3D > tempVtxVec;

	for (int i = 0 ; i < size ; ++i )
	{
		int e1 = poly.eVec[i];
		int e2 = poly.eVec[(i+1) % size ];
		int v = getShareEV( e1 , e2 );

		assert( v != -1 );

		Vec3D vtx  =  getVertex( v );

		float d = vtx.dot( poly.normal );

		vtx -= ( d - poly.d ) * poly.normal;

		tempVtxVec.push_back( vtx );
	}



	prev.resize( size );
	next.resize( size );

	for( int i =0 ; i < size ; ++i )
	{
		prev[i] = i - 1;
		next[i] = i + 1;
	}

	prev[0] = size - 1;
	next[size - 1] = 0;

	int visitNum = 0;
	int i = 0;


	Vec3D curDir = tempVtxVec[1] - tempVtxVec[0];
	curDir.normalize();

	while ( visitNum <= size )
	{
		int iPrev = prev[i];
		int iNext = next[i];
		int iNext2 = next[ iNext ];

		Vec3D& vtx = tempVtxVec[i];
		Vec3D dirPrev = tempVtxVec[ iPrev ] - tempVtxVec[i];
		Vec3D dir     = tempVtxVec[ i ] - tempVtxVec[ iNext ];

		dirPrev.normalize();
		dir.normalize();

		if ( dir.dot( dirPrev ) > NVI_POLYGON_EDGE_DIR_THRESHOLD )
		{
			next[ iPrev ] = iNext;
			prev[ iNext ] = iPrev;
		}
		i = next[i];
		++visitNum;
	}

	int end = prev[i];
	while ( i != end )
	{
		Vec3D& vtx = tempVtxVec[i];

		FyVec3D vertex;
		vertex.x = vtx.x();
		vertex.y = vtx.y();
		vertex.z = vtx.z();

		poly.vtxVec.push_back( vertex );

		i = next[i];
	}

	Vec3D& vtx = tempVtxVec[i];

	FyVec3D vertex;
	vertex.x = vtx.x();
	vertex.y = vtx.y();
	vertex.z = vtx.z();

	poly.vtxVec.push_back( vertex );

}

void NavMeshBuilder::addExpandData( int t )
{
	for ( int j =0 ; j < 3 ; ++ j)
	{
		exData_t data;
		data.e = getTE( t , j );
		int tE = getET( data.e , 0 );
		data.t = ( tE  !=  t ) ? tE : getET( data.e , 1 );

		if ( data.t != -1 )
			exStack.push( data );
	}
}
bool TriIsCCW( FyVec3D const& v0 , FyVec3D const& v1 , FyVec3D const& v2 , FyVec3D const& normal )
{
	return normal.dot( (v1 - v0).cross(v2 - v0) ) > 0;
}

float TriArea2D(float x1, float y1 , float x2 , float y2 , float x3 , float y3 )
{
	return (x1-x2)*(y2-y3) - (x2-x3)*(y1-y2);
}

void Barycentric(  FyVec3D& p ,FyVec3D const& a ,FyVec3D const& b , FyVec3D const& c ,
				   float& u , float& v , float& w)
{
	FyVec3D m = ( b - a ).cross( c - a );
	float nu , nv , ood;

	float x = fabs( m.x );
	float y = fabs( m.y );
	float z = fabs( m.z );

	if ( x >= y && x >= z )
	{
		nu = TriArea2D( p.y , p.z , b.y , b.z , c.y , c.z );
		nv = TriArea2D( p.y , p.z , c.y , c.z , a.y , a.z );
		ood = 1.0f / m.x;
	}
	else if ( y >= x && y >= z )
	{
		nu = TriArea2D( p.x , p.z , b.x , b.z , c.x , c.z );
		nv = TriArea2D( p.x , p.z , c.x , c.z , a.x , a.z );
		ood = - 1.0f / m.y;
	}
	else
	{
		nu = TriArea2D( p.x , p.y , b.x , b.y , c.x , c.y );
		nv = TriArea2D( p.x , p.y , c.x , c.y , a.x , a.y );
		ood = 1.0f / m.z;
	}

	u = nu * ood;
	v = nv * ood;
	w = 1.0 - u - v;
}

bool TestPointInTri( FyVec3D& pt ,FyVec3D const& v0 , FyVec3D const& v1 , FyVec3D const& v2 )
{
	float u , v , w;

	Barycentric( pt , v0 , v1 , v2 , u, v, w );
	return v >= 0.0f && w >= 0.0f && ( v + w )<= 1.0f;
}
void NavMeshBuilder::triangulate( NviPolygon& poly)
{
	int n = poly.vtxVec.size();

	FyVec3D nor;
	nor = poly.normal;

	prev.resize( n );
	next.resize( n );

	for( int i =0 ; i < n ; ++i )
	{
		prev[i] = i - 1;
		next[i] = i + 1;
	}

	prev[0] = n - 1;
	next[n-1] = 0;

	int i = 0;
	int MaxIter = 1000000;
	int iter = 0;

	bufferInternal.clear();

	while( n >= 3 )
	{
		++iter;
		if ( iter > MaxIter )
		{
			if ( !poly.isError )
			{
				g_errorNviPoly.push_back( poly.id );
				poly.errorMsg = "triangulate error";
				poly.isError = true;
			}
			break;
		}
		int isEar = 1;
		if ( TriIsCCW( poly.vtxVec[ prev[i] ] , poly.vtxVec[ i ] ,
					   poly.vtxVec[ next[i] ] , nor ) )
		{
			int k = next[ next[i] ];
			do 
			{
				if ( TestPointInTri( 
					   poly.vtxVec[ k ] , poly.vtxVec[ prev[i] ] ,
					   poly.vtxVec[ i ] , poly.vtxVec[ next[i] ] ) )
				{
					isEar = 0;
					break;
				}

				k = next[k];
			}
			while ( k != prev[i] );
		}
		else
		{
			isEar = 0;
		}

		if ( isEar )
		{
			poly.triVec.push_back( NviPolygon::tri_t() );
			NviPolygon::tri_t& tri = poly.triVec.back();

			tri.v[0] = prev[i];
			tri.v[1] = i;
			tri.v[2] = next[i];

			next[ prev[i] ] = next[i];
			prev[ next[i] ] = prev[i];
			n--;
			i = next[ next[i] ];
		}
		else
		{
			i = next[i];
		}
	}

	poly.triVec.push_back( NviPolygon::tri_t() );
	NviPolygon::tri_t& tri = poly.triVec.back();

	tri.v[0] = prev[i];
	tri.v[1] = i;
	tri.v[2] = next[i];
}