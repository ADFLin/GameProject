#include "TSkeleton.h"

#include <fstream>
#include <limits>
#include "UtilityFlyFun.h"

std::istream& operator >> ( std::istream& is , Vec3D& vec )
{
	is >> vec[0] >> vec[1] >> vec[2];
	return is;
}

std::istream& operator >> ( std::istream& is , Quat& q )
{
	is >> q[3] >> q[0] >> q[1] >> q[2];
	return is;
}



TSkeleton::TSkeleton()
{
	m_base = createSegment( "base" , (Segment_t*)NULL );
	m_base->piovt = Vec3D(0,0,0);
}

Segment_t* TSkeleton::createSegment( char const* name  , Segment_t* parent )
{
	Segment_t* segment = new Segment_t( name , parent );
	m_segmentMap.insert( std::make_pair( segment->name.c_str() , segment ) );

	return segment;
}

Segment_t* TSkeleton::createSegment( char const* name , char const* parentName )
{
	Segment_t* parent = getSegment( parentName );
	assert ( parent );

	Segment_t* segment = new Segment_t( name , parent );
	m_segmentMap.insert( std::make_pair( segment->name.c_str() , segment ) );

	return segment;
}

Segment_t* TSkeleton::getSegment( char const* name )
{
	SegmentMap::iterator iter = m_segmentMap.find( name );
	if ( iter != m_segmentMap.end() )
		return iter->second;
	return NULL;
}

bool TSkeleton::load_cwk( char const* path )
{
	std::ifstream steam( path );

	TString token;
	TString boneName;
	TString MDType;

	TString parentName;
	Vec3D   pivot;
	Vec3D   jointPos;
	TString children;

	while ( steam.good() )
	{
		steam >> token;

		int numBones = 0;

		if ( token == "#" )
		{
			steam.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
		}
		else if ( token == "Skeleton" )
		{
			steam.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
		}
		else if ( token == "Bones")
		{
			steam >> numBones;
		}
		else if ( token == "Segment" )
		{
			steam >> boneName;
			steam >> MDType;
			steam >> token;
			assert ( token[0] == '{' );

			int jointNum;

			steam >> token;
			assert( token == "Parent" );
			steam >> parentName;



			Segment_t* curSegment = getSegment( boneName.c_str() );
			if ( !curSegment )
				curSegment = createSegment( boneName.c_str() , parentName.c_str() );


			steam >> token;
			assert( token == "Pivot" );
			steam >> curSegment->piovt;

			if ( parentName == "base" )
			{
				Joint_t* joint = new Joint_t;
				joint->child = curSegment;
				joint->pos   = pivot;
				m_base->jointList.push_back( joint );
			}


			steam >> token;
			assert( token == "Joint" );

			steam >> jointNum;

			for ( int i = 0 ; i < jointNum ; ++ i )
			{
				steam >> token;
				Segment_t* child = getSegment( token.c_str() );
				if ( child == NULL )
				{
					child = createSegment( token.c_str() , boneName.c_str() );
				}
				Joint_t* joint = new Joint_t;
				joint->child = child;
				steam >> joint->pos;

				curSegment->jointList.push_back( joint );
			}

			steam >> token;
			assert( token == "}" );
		}
	}


	return true;
}

OBJECTid TSkeleton::debugDraw( FnScene& scene , Segment_t* segment  , OBJECTid parentID )
{
	Segment_t* curSegment = segment;

	Vec3D& pos = curSegment->piovt;
	FnObject obj;

	obj.Object( scene.CreateObject(  ) );
	obj.SetParent( parentID );

	FnMaterial mat = UF_CreateMaterial();
	mat.SetEmissive( Vec3D(1,0,0) );
	UF_CreateBoxLine( &obj ,1,1,1 , mat.Object() );
	//obj.Translate( pos.x() , pos.y() , pos.z() , GLOBAL );
	obj.SetPosition( &pos[0] );

	for ( std::list< Joint_t* >::iterator iter = curSegment->jointList.begin();
		iter != curSegment->jointList.end() ; ++iter )
	{
		Joint_t* childJoint = *iter;
		debugDraw( scene , childJoint->child , obj.Object() ); 
	}

	return obj.Object();
}

OBJECTid TSkeleton::debugDraw( FnScene& scene )
{
	return debugDraw( scene , m_base , ROOT );
}
