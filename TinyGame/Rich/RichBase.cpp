#include "RichPCH.h"
#include "RichBase.h"

#include "RichWorld.h"

namespace Rich
{
	void ActorComp::move( Tile& tile , bool beStay )
	{
		mPosPrev = mPos;

		mPos = tile.pos;

		if ( tileHook.isLinked() )
			tileHook.unlink();

		tile.actors.push_front( *this );

		Tile::ActorList::iterator iter = tile.actors.begin();
		++iter;

		for ( Tile::ActorList::iterator end = tile.actors.end();
			iter != end ; ++iter )
		{
			iter->onMeet( *this , beStay );
		}
	}

	Area::Area() 
		:mId( ERROR_AREA_ID )
	{

	}

}
