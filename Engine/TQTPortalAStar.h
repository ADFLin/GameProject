#ifndef TQTPortalAStar_h__
#define TQTPortalAStar_h__

#include "Algo/AStar.h"

#include "TQuadTree.h"
#include "Math/TVector2.h"

#include <algorithm>
#include <cassert>
#include <list>

#include "TileRegion.h"


namespace AStar
{

	struct FindState
	{
		Vec2i   pos;
		Region* region;
		Portal* portal;

		FindState()
		{
			portal = NULL;
			region = NULL;
		}
	};

	struct PortalNode : NodeBaseT< PortalNode , FindState , int >
	{

	};
	class PortalAStar : public AStarT< PortalAStar , PortalNode >
	{

	public:
		typedef FindState StateType;
		ScoreType calcHeuristic( StateType& state )
		{  
			return abs( mEndPos.x - state.pos.x ) + abs( mEndPos.x - state.pos.y );
		}
		ScoreType calcDistance( StateType& a, StateType& b)
		{  
			return abs( a.pos.x - b.pos.x ) + abs( a.pos.y - b.pos.y );
		}
		bool      isEqual(StateType& state1,StateType& state2)
		{ 
			return state1.region == state2.region;
		}
		bool      isGoal (StateType& state )
		{ 
			return state.region->getRect().isInRange( mEndPos );
		}
		//  call addSearchNode for all possible next state


		void      processNeighborNode( NodeType& node )
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
					continue;

				ScoreType dist = processRegion( p , state , newState );

				addSearchNode( newState , node , dist );
			}
		}

		ScoreType  processRegion( Portal* p , FindState const& state , FindState& newState  )
		{
			int conDir = p->getConnectDir( *state.region );
			int idx = conDir % 2;

			ScoreType dist = abs( state.pos[ idx ] - p->value );

			newState.pos[ idx ] = p->value;
			newState.portal     = p;

			if ( conDir / 2 )
			{
				newState.pos[ idx ] -= 1;
				dist += 1;
			}

			int idx2 = ( idx + 1 ) % 2;

			if ( state.pos[ idx2 ] >= p->range.max )
			{
				dist += abs( state.pos[ idx2 ] - ( p->range.max - 1 ) );
				newState.pos[ idx2 ] = p->range.max - 1;
			}
			else if ( state.pos[ idx2 ] < p->range.min )
			{
				dist += abs( state.pos[ idx2 ] - p->range.min );
				newState.pos[ idx2 ] = p->range.min;
			}
			else
			{
				newState.pos[ idx2 ] = state.pos[idx2];
			}

			return dist;
		}


		RegionManager* mManager;
		Vec2i          mEndPos;
	};

}



#endif // TQTPortalAStar_h__

