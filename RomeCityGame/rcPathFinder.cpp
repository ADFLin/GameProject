#include "rcPCH.h"
#include "rcPathFinder.h"

#include "rcLevelMap.h"
#include "rcBuilding.h"

#include "AStarTile2D.h"

#ifdef USE_BOOST


#include <boost/pool/object_pool.hpp>
template < class T >
class BoostPoolPolicy
{
public:
	T* alloc()
	{ 
		T* ptr = m_pool.malloc();
		return ptr;
	}
	void free(T* ptr)
	{ 
		if( ptr ) m_pool.free( ptr ); 
	}
	boost::object_pool<T> m_pool;
};

#endif

#if 0
#include "TQTPortalAStar.h"
class PortalPathFinder : public rcPathFinder
	                   , public TPortalAStar< PortalPathFinder >
{
public:
	void      processNeighborNode( StarNode& node )
	{
		StateType& state = node.state;

		Region::PortalList pList = state.region->portals;
		for( Region::PortalList::iterator iter = pList.begin();
			iter != pList.end(); ++iter )
		{
			Portal* p = *iter;

			FindState newState;
			newState.region = p->getOtherRegion( *state.region );

			if ( node.parent && newState.region == node.parent->state.region )
				continue;

			if ( newState.region->type == Region::eBlock )
			{
				rcMapData& data = mLevelMap->getData( newState.region->rect.getMin() );
				if ( !data.haveRoad() )
				{
					if ( mType != FT_BUILDING && data.buildingID != mBuildID )
						continue;
				}
			}

			ScoreType dist = processRegion( p , state , newState );

			addSreachNode( newState , node , dist );
		}
	}

	virtual  rcPath*  sreachPath( Vec2i const& from )
	{
		PROFILE_ENTRY( "sreachPath" );

		FindState startState;
		//FIXME
		startState.region = 0;
		startState.pos = from;

		startSreach( startState );
		static int maxSreachStep = 100000;
		int result;
		int step = 0;

		do 
		{

			++step;
			if ( step > maxSreachStep )
				return NULL;

			result = sreachStep();
		}
		while( result == ASTAR_SREACHING );

		if ( result == ASTAR_SREACH_FAIL )
			return NULL;

		//int dir = 0;

		//rcPath* path = new rcPath;

		//StarNode* curNode = getPath();
		//StarNode* prevNode;

		//int num = getPathNodeNum();

		//path->addRoutePos( curNode->state.pos );
		//
		//prevNode = curNode;
		//curNode  = curNode->child;
		//if ( curNode == NULL )
		//	return path;
		//
		//Vec2i offset = curNode->state.pos - prevNode->state.pos;

		//while( 1 )
		//{
		//	prevNode = curNode;
		//	curNode = curNode->child;
		//	if ( curNode == NULL )
		//	{
		//		if ( mType == FT_BUILDING )
		//		{
		//			Vec2i pos = prevNode->state - offset;
		//			if ( pos != path->getGoalPos() )
		//				path->addRoutePos( pos );
		//		}
		//		else
		//		{	
		//			path->addRoutePos( prevNode->state );
		//		}
		//		break;
		//	}

		//	Vec2i nextOffset = curNode->state - prevNode->state;
		//	if ( nextOffset != offset )
		//	{
		//		path->addRoutePos( prevNode->state );
		//		offset = nextOffset;
		//		num = 0;
		//	}
		//}

		//return path;
	}
};
	  
#endif

class CPathFinder : public rcPathFinder
	              , public AStar::AStarTile2DT< CPathFinder >  
{

public:
	CPathFinder( Vec2i const& size )
		:AStarTile2DT< CPathFinder >()
	{
		getMap().resize( size.x , size.y );
	}

	virtual  rcPath*  sreachPath( Vec2i const& from )
	{
		PROFILE_ENTRY( "sreachPath" );
		SreachResult sreachResult;
		startSreach( from , sreachResult );
		static int maxSreachStep = 100000;
		int result;
		int step = 0;

		do 
		{

			++step;
			if ( step > maxSreachStep )
				return NULL;

			result = sreachStep(sreachResult);
		}
		while( result == AStar::eSREACHING );

		if ( result == AStar::eSREACH_FAIL )
			return NULL;

		int dir = 0;

		rcPath* path = new rcPath;
#if 0

		NodeType* curNode = getPath();
		NodeType* prevNode;


		int num = getPathNodeNum();

		path->addRoutePos( curNode->state );
		prevNode = curNode;
		curNode  = curNode->child;
		if ( curNode == NULL )
			return path;
		Vec2i offset = curNode->state - prevNode->state;

		while( 1 )
		{
			prevNode = curNode;
			curNode = curNode->child;
			if ( curNode == NULL )
			{
				if ( mType == FT_BUILDING )
				{
					Vec2i pos = prevNode->state - offset;
					if ( pos != path->getGoalPos() )
						path->addRoutePos( pos );
				}
				else
				{	
					path->addRoutePos( prevNode->state );
				}
				break;
			}

			Vec2i nextOffset = curNode->state - prevNode->state;
			if ( nextOffset != offset )
			{
				path->addRoutePos( prevNode->state );
				offset = nextOffset;
				num = 0;
			}
		}
#endif

		return path;
	}



	ScoreType calcHeuristic( StateType& a )
	{
		return abs( a.x - mEndPos.x ) +  abs( a.y -  mEndPos.y );
	}


	ScoreType calcDistance(StateType& from , StateType& to )
	{
		return abs( from.x - to.x ) +  abs( from.y - to.y );
	}
	
	bool      isEqual(StateType& state1,StateType& state2)
	{   
		return state1 == state2;  
	}
	
	bool      isGoal( StateType& state )
	{
		if ( mType == FT_POSITION )
			return state == mEndPos;  
		else
		{
			return mLevelMap->getData( state ).buildingID == mBuildID;
		}
		return false;
	}

	void   processNeighborNode( NodeType& curNode )
	{
		for( int i = 0 ; i < 4 ; ++ i )
		{
			Vec2i pos = curNode.state + g_OffsetDir[i];

			if ( curNode.parent && curNode.parent->state == pos )
				continue;

			if ( !mLevelMap->isRange( pos.x , pos.y ) )
				continue;

			rcMapData& data = mLevelMap->getData( pos );

			if ( data.layer[ ML_BUILDING ] )
			{
				if ( !data.haveRoad() )
				{
					if ( mType != FT_BUILDING && data.buildingID != mBuildID )
						continue;
				}
			}

			ScoreType dist = data.haveRoad() ? 1 : 2;
			addSreachNode( pos , curNode , dist );
		}
	}


};


rcPathFinder* rcPathFinder::creatPathFinder( Vec2i const& size )
{
	return new CPathFinder( size );
}


void rcPathFinder::setupDestion( rcBuilding* building )
{
	mEndPos = building->getEntryPos();
	mBuildID = building->getManageID();
	mType = FT_BUILDING;
}

void rcPathFinder::setupDestion( Vec2i const& mapPos )
{
	mEndPos = mapPos;
	mType = FT_POSITION;
}


bool rcPath::getNextRoutePos( Cookie& cookie , Vec2i& pos )
{
	if ( mRotePosList.size() == 1 )
	{
		int i = 1;
	}
	++cookie;
	if ( cookie == mRotePosList.end() )
		return false;
	pos = *cookie;
	return true;
}

void rcPath::startRouting( Cookie& cookie )
{
	cookie = mRotePosList.begin();
}
