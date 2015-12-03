#include "common.h"

struct Segment_t;

struct Joint_t
{
	Segment_t* child;
	Vec3D pos;
};

struct Segment_t
{
	Segment_t( char const* _name , Segment_t* _parent )
		:name( _name ) , parent( _parent )
	{
		index = -1;
	}

	short index;
	Segment_t* parent;
	TString name;
	Vec3D piovt;
	std::list< Joint_t* > jointList;
};

struct MQPData
{
	Quat  rote;
	Vec3D translate;
};

struct MAAPData
{
	Vec3D axis;
	Vec3D translate;
};


class TSkeleton
{
public:
	TSkeleton();

	bool load_cwk( char const* path );


	Segment_t* createSegment( char const* name  , char const* parentName );
	Segment_t* createSegment( char const* name , Segment_t* parent );
	Segment_t* getSegment( char const* name );

	Segment_t* m_base;

	OBJECTid debugDraw( FnScene& scene );
	OBJECTid debugDraw( FnScene& scene , Segment_t* segment , OBJECTid parentID );
	struct StrCmp
	{
		bool operator()(const char* s1, const char* s2) const
		{
			return strcmp(s1, s2) < 0;
		}
	};

	typedef std::map< char const* , Segment_t* , StrCmp > SegmentMap;
	SegmentMap  m_segmentMap;
};


