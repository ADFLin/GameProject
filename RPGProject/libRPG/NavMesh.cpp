#include "ProjectPCH.h"
#include "NavMesh.h"

#include <math.h>
#include "DetourNode.h"
#include <fstream>

#ifdef max
#undef max
#endif 
#ifdef min
#undef min
#endif 
#include "DetourCommon.h"

inline void vscale(float* v , float s)
{
	v[0] *= s; v[1] *= s; v[2] *= s;
}

#define SQR(a) ( (a) * (a) )
TStatNavMesh::TStatNavMesh()
{
	mMinPotalLenSqr  = SQR( 150 );
}

int TStatNavMesh::findStraightPath(
	const float* startPos, const float* endPos,
	const dtStatPolyRef* path, const int pathSize,
	float* straightPath, const int maxStraightPathSize)
{
	if (!m_header) 
		return 0;

	if (!maxStraightPathSize)
		return 0;

	if (!path[0])
		return 0;

	int straightPathSize = 0;

	float closestStartPos[3];
	if (!closestPointToPoly(path[0], startPos, closestStartPos))
		return 0;

	// Add start point.
	vcopy(&straightPath[straightPathSize*3], closestStartPos);
	straightPathSize++;
	if (straightPathSize >= maxStraightPathSize)
		return straightPathSize;

	float closestEndPos[3];
	if (!closestPointToPoly(path[pathSize-1], endPos, closestEndPos))
		return 0;

	float portalApex[3], portalLeft[3], portalRight[3];

	if (pathSize > 1)
	{
		vcopy(portalApex, closestStartPos);
		vcopy(portalLeft, portalApex);
		vcopy(portalRight, portalApex);
		int apexIndex = 0;
		int leftIndex = 0;
		int rightIndex = 0;

		for (int i = 0; i < pathSize; ++i)
		{
			float left[3], right[3];
			if (i < pathSize-1)
			{
				// Next portal.
				getPortalPoints(path[i], path[i+1], left, right);
			}
			else
			{
				// End of the path.
				vcopy(left, closestEndPos);
				vcopy(right, closestEndPos);
			}



			// Right vertex.
			if (vequal(portalApex, portalRight))
			{
				vcopy(portalRight, right);
				rightIndex = i;
			}
			else
			{
				if (triArea2D(portalApex, portalRight, right) <= 0.0f)
				{
					if (triArea2D(portalApex, portalLeft, right) > 0.0f)
					{
						vcopy(portalRight, right);
						rightIndex = i;
					}
					else
					{
						//right is not in portal
						vcopy(portalApex, portalLeft);
						apexIndex = leftIndex;

						if (!vequal(&straightPath[(straightPathSize-1)*3], portalApex))
						{
							vcopy(&straightPath[straightPathSize*3], portalApex);
							straightPathSize++;
							if (straightPathSize >= maxStraightPathSize)
								return straightPathSize;
						}

						vcopy(portalLeft, portalApex);
						vcopy(portalRight, portalApex);
						leftIndex = apexIndex;
						rightIndex = apexIndex;

						// Restart
						i = apexIndex;

						continue;
					}
				}
			}

			// Left vertex.
			if (vequal(portalApex, portalLeft))
			{
				vcopy(portalLeft, left);
				leftIndex = i;
			}
			else
			{
				if (triArea2D(portalApex, portalLeft, left) >= 0.0f)
				{
					if (triArea2D(portalApex, portalRight, left) < 0.0f)
					{
						vcopy(portalLeft, left);
						leftIndex = i;
					}
					else
					{
						vcopy(portalApex, portalRight);
						apexIndex = rightIndex;

						if (!vequal(&straightPath[(straightPathSize-1)*3], portalApex))
						{
							vcopy(&straightPath[straightPathSize*3], portalApex);
							straightPathSize++;
							if (straightPathSize >= maxStraightPathSize)
								return straightPathSize;
						}

						vcopy(portalLeft, portalApex);
						vcopy(portalRight, portalApex);
						leftIndex = apexIndex;
						rightIndex = apexIndex;

						// Restart
						i = apexIndex;

						continue;
					}
				}
			}


			float distSqr = vdistSqr( left , right );
			if ( distSqr < mMinPotalLenSqr && i != apexIndex )
			{
				float pos[3];
				vadd( pos , left , right );
				vscale( pos , 0.5 );

				if ( vdistSqr( pos , portalApex ) > 300 * 300 )
				{
					vcopy( portalApex , pos );
					if (!vequal(&straightPath[(straightPathSize-1)*3], portalApex))
					{
						vcopy(&straightPath[straightPathSize*3], portalApex);
						straightPathSize++;
						if (straightPathSize >= maxStraightPathSize)
							return straightPathSize;
					}

					apexIndex = i;
					vcopy(portalLeft, portalApex);
					vcopy(portalRight, portalApex);
					leftIndex = apexIndex;
					rightIndex = apexIndex;

					continue;
				}
			}
		}
	}

	// Add end point.
	vcopy(&straightPath[straightPathSize*3], closestEndPos);
	straightPathSize++;

	return straightPathSize;
}

float TStatNavMesh::triArea2D( const float* a, const float* b, const float* c )
{
	return ((b[1]*a[0] - a[1]*b[0]) + (c[1]*b[0] - b[1]*c[0]) + (a[1]*c[0] - c[1]*a[0])) * 0.5f;
}

bool NavMesh::findPath( NavPath& path , Vec3D const& posStart , Vec3D const& posEnd , Vec3D const& extents )
{
	dtStatPolyRef start = mImpl.findNearestPoly( (float const*)&posStart , (float const*)&extents );
	dtStatPolyRef end  = mImpl.findNearestPoly( (float const*)&posEnd ,(float const*)&extents );

	if ( !start ||  !end  )
		return false;

	dtStatPolyRef pathPoly[256];


	int nPoly = mImpl.findPath( start , end , (float const*)&posStart , (float const*)&posEnd , pathPoly , 256 );

	if ( !nPoly )
		return false;

	Vec3D pathPos[ 256 ];
	int nPathPos = mImpl.findStraightPath( (float const*)&posStart , (float const*)&posEnd , pathPoly , nPoly , (float*)&pathPos[0] , 256 );

	if ( !nPathPos )
		return false;

	path.clear();
	path.mStartPoly = start;
	path.mEndPoly   = end;

	for( int i = 0;i < nPathPos ; ++i )
	{
		Waypoint wp;
		wp.type = WPT_NORMAL;
		wp.pos  = pathPos[i];
		path.addWaypoint( wp );
	}

	return true;
}

bool NavMesh::load( char const* path )
{
	using namespace std;
	std::ifstream fs( path , std::ios::binary );

	if ( !fs )
		return false;

	long begin , end ;
	begin = fs.tellg();
	fs.seekg (0, ios::end);
	end = fs.tellg();

	long size = end - begin; 

	if ( size == 0 )
		return false;


	releaseData();
	m_data = new unsigned char[ size ];

	fs.seekg (0, ios::beg);
	fs.read( (char*)m_data  , size );
	mImpl.init( m_data  , size , false );

	return true;
}

void NavMesh::releaseData()
{
	delete [] m_data;
	m_data = 0;
	//mImpl.init( 0 , 0 , false );
}

NavMesh::NavMesh()
{
	m_data = nullptr;
}

NavMesh::~NavMesh()
{
	releaseData();
}

bool NavMesh::updateGoalPos( NavPath& path , Vec3D const& newGoalPos , Vec3D const& extents )
{
	dtStatPolyRef polyRef = mImpl.findNearestPoly( (float const*)&newGoalPos , (float const*)&extents );
	if ( polyRef == path.mEndPoly )
	{
		path.getGoalWaypoint()->pos = newGoalPos;
		return true;
	}
	return false;
}

void NavPath::clear()
{
	mStartPoly = ERROR_NAV_POLYREF;
	mEndPoly   = ERROR_NAV_POLYREF;
	mWaypoints.clear();
}

bool NavPath::startRoute( Cookie& cookie )
{
	cookie.iter = mWaypoints.begin();

	if ( cookie.iter == mWaypoints.end() )
		return false;

	return true;
}

Waypoint* NavPath::nextRoutePoint( Cookie& cookie )
{
	if ( cookie.iter == mWaypoints.end() )
		return nullptr;

	++cookie.iter;
	return &( *cookie.iter );
}

Waypoint* NavPath::getGoalWaypoint()
{
	if ( mWaypoints.empty() )
		return nullptr;
	return &mWaypoints.back();
}
