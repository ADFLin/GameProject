#ifndef NavMesh_h__
#define NavMesh_h__

#include "common.h"
#include "DetourStatNavMesh.h"

#define ERROR_NAV_POLYREF 0

enum WaypointType
{
	WPT_NORMAL ,
	/////////
	WPT_DUCK   ,
	WPT_JUMP   ,

};

struct Waypoint
{
	WaypointType type;
	Vec3D        pos;
};

class NavPath
{
	typedef std::list< Waypoint > WaypointList;

public:
	class Cookie
	{
	private:
		WaypointList::iterator iter;
		friend class NavPath;
	};
	void      clear();
	bool      startRoute( Cookie& cookie );
	Waypoint* nextRoutePoint( Cookie& cookie );
	Waypoint* getGoalWaypoint();
	void      addWaypoint( Waypoint const& wp ){ mWaypoints.push_back(wp); }
	size_t    getWaypointNum() const { return mWaypoints.size(); }


	Waypoint const& getWayPoint( int index );

private:
	dtStatPolyRef mStartPoly;
	dtStatPolyRef mEndPoly;
	WaypointList  mWaypoints;

	friend class NavMesh;
};


class TStatNavMesh : public dtStatNavMesh
{
public:
	TStatNavMesh();
	int findStraightPath(const float* startPos, const float* endPos, 
		const dtStatPolyRef* path, const int pathSize, 
		float* straightPath, const int maxStraightPathSize);
protected:

	float  mMinPotalLenSqr;
	static float triArea2D(const float* a, const float* b, const float* c);
};



class NavMesh
{
public:
	NavMesh();
	~NavMesh();

	bool load( char const* path );
	void releaseData();
	bool findPath( NavPath& path , Vec3D const& posStart , Vec3D const& posEnd , Vec3D const& extents);
	bool updateGoalPos( NavPath& path , Vec3D const& newGoalPos , Vec3D const& extents );
private:
	unsigned char* m_data;
	TStatNavMesh mImpl;
};


#endif // NavMesh_h__

