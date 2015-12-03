#ifndef TPathFinding_h__
#define TPathFinding_h__

#include "common.h"
#include "AStar/AStar.h"
#include "AStar/axUIntHash.h"
#include "UtilityFlyFun.h"

typedef int TriID; 

struct TriData
{
	TriID id;
	Vec3D pos;
};

struct TerrainInfo
{
	void init( OBJECTid id )
	{
		terr.Object( id );

		vertexLen = * (int*) terr.GetTriangleData( TRIANGLE_VERTEX_LENGTH );
		numFace =  * (int*) terr.GetTriangleData( TRIANGLE_FACE_NUMBER );
		face = (int*) terr.GetTriangleData( TRIANGLE_FACE );
		numVertex = *(int*) terr.GetTriangleData( TRIANGLE_VERTEX_NUMBER );
		vertex = (float*) terr.GetTriangleData( TRIANGLE_VERTEX );
	}

	int       vertexLen;
	int       numFace;
	int*      face;
	int       numVertex;
	float*    vertex;
	FnTerrain terr;

};

class FlyMapPolicy : public TerrainInfo
{
public:

	typedef Float ScoreType;
	typedef TriData StateType;


	ScoreType computeH(StateType& a, StateType& b)
	{
		return a.pos.distance( b.pos );
	}
	ScoreType computeDistance(StateType& a, StateType& b)
	{
		return a.pos.distance( b.pos );
	}

	bool isEqual(StateType const& node1,StateType const& node2)
	{
		return  node1.id == node2.id;
	}

	Vec3D getCenterPosOfTri( TriID id  )
	{
		Vec3D result(0,0,0);
		int* pvi = &face[3*id];
		for (int i = 0 ; i < 3 ; ++i )
		{
			int vIndex = pvi[i];
			float* pv = &vertex[ vertexLen*vIndex ];
			result += Vec3D( pv[0] , pv[1] , pv[2] );
		}
		return result *= 1.0/3;
	}

	int getAllEdgeTri( TriID id )
	{
		int num = 0;
		for( int i = 0 ; i < 3 ; ++i )
		{
			TriID nId = terr.GetEdgeNeighborTriangle( id , i );
			if ( nId >= 0 )
			{
				triIndex[num] = nId;
				++num;
			}
		}
		return num;
	}

	template< class FunType >
	void processNeighborNode( StateType& node, StateType* parent, FunType& fun)
	{
		int numNTri = getAllEdgeTri( node.id );
		if ( numNTri == 3)
		{
			numNTri = terr.GetAllVertexNeighborTriangles( 
					node.id , triIndex , MAX_TRI_INDEX_NUM );
		}

		for( int i = 0 ; i < numNTri ; ++ i )
		{
			TriID nTriID = triIndex[i];
			if ( parent!= NULL && ( nTriID == parent->id || nTriID == -1 ) )
				continue;

			TriData data;
			data.id = nTriID;
			data.pos = getCenterPosOfTri( data.id );
			fun( data );
		}
	}
	static int const MAX_TRI_INDEX_NUM = 24;

	int triIndex[ MAX_TRI_INDEX_NUM ];
};


template< class SNode >
class FyDebugPolicy
{
public:

	FnWorld world;
	FnScene scene;
	MATERIALid m_red;
	MATERIALid m_green;
	MATERIALid m_blue;

	FnObject pathObj;

	std::vector<FnObject> objVec;
	void clearLine()
	{
		pathObj.RemoveAllGeometry();

		Vec3D color;
		color.setValue( 1 , 0, 0 );
		m_red   =  world.CreateMaterial( &color[0] , &color[0] , &color[0], 1 , &color[0] );
		color.setValue( 0 , 1, 0 );
		m_green =  world.CreateMaterial( &color[0] , &color[0] , &color[0], 1 , &color[0] );
		color.setValue( 0 , 0, 1 );
		m_blue =  world.CreateMaterial( &color[0] , &color[0] , &color[0], 1 , &color[0] );
	}
	void initDebug( FnScene& s )
	{
		scene.Object( s.Object() );
		world.Object( s.GetWorld() );
		pathObj.Object( scene.CreateObject() );

		Vec3D color;
		color.setValue( 1 , 0, 0 );
		m_red   =  world.CreateMaterial( &color[0] , &color[0] , &color[0], 1 , &color[0] );
		color.setValue( 0 , 1, 0 );
		m_green =  world.CreateMaterial( &color[0] , &color[0] , &color[0], 1 , &color[0] );
		color.setValue( 0 , 0, 1 );
		m_blue =  world.CreateMaterial( &color[0] , &color[0] , &color[0], 1 , &color[0] );
	}

	void getCloseInfo(SNode* sNode)
	{
		if ( sNode->parent == NULL)
			return;

		Vec3D offset( 0,0,10);
		UF_CreateLine( &pathObj , sNode->parent->state.pos + offset ,
			                       sNode->state.pos + offset, m_red );

	}
	void getOpenInfo(SNode* sNode)
	{
		if ( sNode->parent == NULL)
			return;

		Vec3D offset( 0,0,10);
		UF_CreateLine( &pathObj , sNode->parent->state.pos + offset, 
			                       sNode->state.pos + offset , m_green );

	}

	void getCurrectStateInfo( SNode* sNode )
	{

	}
};
//unsigned hash_value(FlyMapPolicy::TriData& data){ return data.id; }
inline
unsigned hash_value( TriData& state)
{
	return state.id;
}
typedef AStar< FlyMapPolicy 
			 , NewPolicy
			 , StlVectorClosePolicy 
			 //, axUIntHash
			 , StlHeapOpenPolicy 
			 , NoExtraData 
			 , FyDebugPolicy > FlyAstar;


struct WayPoint
{
	Vec3D pos;
};

typedef btAlignedObjectArray< WayPoint > TWayPath;

class TPathFinding
{

	
	
	WayPoint& getWayPoint( int index ){	return m_path[index]; }


	TWayPath  m_path;
	FlyAstar* m_aStar;
};




#endif // TPathFinding_h__