#ifndef NavMesh_h__
#define NavMesh_h__

//#include "HMAlgo.h"
#include "common.h"
#include "TTerrainData.h"
#include "UtilityMath.h"
#include "FyVec3D.h"
#include <vector>
#include <stack>
#include <map>

#define  NV_DIV_ESPISON  1e-32
#define  NV_PLANE_ESPION 1e-32
#define  NV_FLOAT_MAX    1e32

#define  NVI_VERTEX_WELDING_THRESHOLD 4
#define  NVI_MIN_STEP_HEIGHT
#define  NVI_PLANE_NORMAL_THRESHOLD ( cos( DEG2RAD(10) ) )
#define  NVI_PLANE_MAX_OFFSET  40
#define  NVI_POLYGON_EDGE_DIR_THRESHOLD ( cos( DEG2RAD(2) ) )
#define  NVI_POLYGON_EDGE_MIN_LEN 

#define NEXT_VAL( val , size ) ( ( val + 1 ) % size )
#define BEFORE_VAL( val , size ) ( ( val + size - 1 ) % size )
#define GO_NEXT( val , size ) ( val = ( val + 1 ) % size )
#define GO_BEFORE( val , size ) ( val = ( val + size - 1 ) % size )

extern std::vector<int> g_errorNviPoly;


enum
{
	GROUND = BIT(1) ,
	WALL   = BIT(2) ,
};

class MeshManager
{
	int  getEV( int e , int index );
	int  getET( int e , int index );
	int  getTV( int t , int index );
	int  getTE( int t , int index );

	int  getTEIndex( int t , int e );

	// return Num;
	int  getVE( int v , int* eVec ); 
	int  getVT( int v , int* tVec );

	int  getENum();
	int  getVNum();
	int  getTNum();

	int  getShareEV( int e1 , int e2 );
	int  getShareET( int e1 , int e2 );

	Vec3D  computeTNormal( int t );
	Vec3D& getVertex( int v );




};

class TerrainManager
{
public:
	struct edge_t
	{
		int tNum;
		int v[2];
		int t[8];
	};

	struct triangle_t
	{
		int e[3];
		int v[3];

		int getEdgeIndex( int et )
		{
			for( int i =0;i < 3 ; ++i )
			{
				if ( e[i] == et )
					return i;
			}
			return -1;
		}
	};

	std::vector< edge_t >              edgeVec;
	btAlignedObjectArray< Vec3D >      vtxVec;
	std::vector< triangle_t >          triVec;



	int buildEdge( int ti , int v1 , int v2 );
	void load( TTerrainData& data )
	{
		load( &data.m_vertexVec[0].x , data.m_vertexVec.size() , 3 ,
			  &data.m_triVec[0].v[0] , data.m_triVec.size() );
	}

	void load( float* vertex , int NumVertex , int vertexOffset , int* triIndex , int NumTri );


	int  getEV( int e , int index )
	{	
		return edgeVec[e].v[index];	
	}
	int  getET( int e , int index )
	{
		return edgeVec[e].t[index];
	}
	int  getTV( int t , int index )
	{
		return triVec[t].v[index];
	}
	int  getTE( int t , int index )
	{
		return triVec[t].e[index];
	}

	int  getTEIndex( int t , int e )
	{
		return triVec[t].getEdgeIndex( e );
	}

	// return Num;
	int  getVE( int v , int* eVec ); 
	int  getVT( int v , int* tVec );

	int  getENum(){ return edgeVec.size(); }
	int  getVNum();
	int  getTNum(){ return triVec.size(); }

	int  getShareEV( int e1 , int e2 )
	{
		edge_t& edge1 = edgeVec[e1];
		edge_t& edge2 = edgeVec[e2];

		if ( edge1.v[0] == edge2.v[0] || 
			 edge1.v[0] == edge2.v[1] )
			 return edge1.v[0];
		if ( edge1.v[1] == edge2.v[0] || 
			 edge1.v[1] == edge2.v[1] )
			return edge1.v[1];

		return -1;
	}
	int  getShareET( int e1 , int e2 );

	Vec3D  computeTNormal( int t )
	{
		Vec3D& v0 = getVertex( getTV(t,0) );
		Vec3D& v1 = getVertex( getTV(t,1) );
		Vec3D& v2 = getVertex( getTV(t,2) );

		return ( v1 - v0 ).cross( v2 - v0 );
	}
	Vec3D& getVertex( int v )
	{
		return vtxVec[v];
	}


	
};




class NviPolygon
{
public:

	struct tri_t
	{
		int v[3];
	};

	NviPolygon()
	{
		isError  = false;
		errorMsg = NULL;
		dMax = -1e24;
		dMin =  1e24;
	}

	bool isError;
	char const* errorMsg;

	int  id;
	Vec3D normal;
	float d;

	float dMax;
	float dMin;

	std::vector< tri_t >   triVec;
	std::vector< FyVec3D > vtxVec;

	std::vector< int >     tVec;
	std::vector< int >     eVec;

	int getE( int index ){ return tVec[index]; }
	int getT( int index ){ return eVec[index]; }


	

	int getEIndex( int e )
	{
		for( int i = 0 ; i < eVec.size() ; ++ i )
		{
			if ( eVec[i] == e )
				return i;
		}

		return -1;
	}
};


class NavMeshBuilder : public TerrainManager
{
public:

	void setTPoly( int t , int p )
	{
		pIDVec[t] = p;
	}
	int  getTPoly( int t )
	{
		if ( t == -1 )
			return -1;
		return pIDVec[t];
	}

	struct exData_t
	{
		int t;
		int e;
	};

	bool checkPoly( NviPolygon& poly );

	std::vector<int>  pIDVec;

	std::stack<exData_t> exStack;

	btAlignedObjectArray< NviPolygon > polyVec;

	void build();

	void addExpandData( int t );

	void clearIntrinsicEdge(  NviPolygon& poly  );
	void computeVertex( NviPolygon& poly );
	void expandPoly( NviPolygon& poly );
	void triangulate( NviPolygon& poly);

	std::vector< int >   bufferInternal;
	std::vector< int >   prev;
	std::vector< int >   next;

};

#endif // NavMesh_h__