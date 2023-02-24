#include "BMPCH.h"
#include "BMMode.h"

#include "BMRender.h"

#include <algorithm>
#include "GameGlobal.h"

//DOTO: add block fall feature

namespace BomberMan
{
	void IMapGenerator::generateGapTile( World& world , Vec2i const& start , Vec2i const& size , int id )
	{
		Vec2i end = start + size;

		Vec2i pos;
		for( pos.x = start.x ; pos.x < end.x ; ++pos.x )
		for( pos.y = start.y ; pos.y < end.y ; ++pos.y )
		{
			TileData& tile = world.getTile( pos );

			if (  ( ( pos.x - start.x ) & 0x1 ) && 
				  ( ( pos.y - start.y ) & 0x1 ) )
			{
				tile.obj  = id;
				tile.meta = 0;
			}
		}
	}

	void IMapGenerator::generateRandTile( World& world , Vec2i const& start , Vec2i const& size , int ratio , int id )
	{
		Vec2i end = start + size;

		Vec2i pos;
		for( pos.x = start.x ; pos.x < end.x ; ++pos.x )
		{
			for( pos.y = start.y ; pos.y < end.y ; ++pos.y )
			{
				TileData& tile = world.getTile( pos );

				if ( tile.obj != MO_NULL )
					continue;

				int value = world.getRandom( 100 );

				if ( value <= ratio )
				{
					tile.obj  = id;
					tile.meta = 0;
				}
			}
		}
	}

	void IMapGenerator::generateConRoad( World& world , Vec2i const& pos , int numCon  )
	{
		{
			TileData& tile = world.getTile( pos );
			tile.obj  = MO_NULL;
			tile.meta = 0;
		}

		TArray< Vec2i > conPos;
		conPos.reserve( numCon );
		TArray< Vec2i > stackPos;
		stackPos.reserve( 4 * numCon );

		for( int i = 0 ; i < 4 ; ++i )
		{
			Vec2i tPos = pos + getDirOffset( i );

			if ( 0 <= tPos.x && tPos.x < world.getMapSize().x &&
				 0 <= tPos.y && tPos.y < world.getMapSize().y )
			{
				stackPos.push_back( tPos );
			}
		}
		conPos.push_back( pos );

		while ( !stackPos.empty() )
		{
			int idx = world.getRandom( stackPos.size() );
			Vec2i cPos = stackPos[ idx ];
			stackPos[ idx ] = stackPos.back();
			stackPos.pop_back();

			TileData& tile = world.getTile( cPos );

			bool test = false;
			if ( tile.obj == MO_NULL )
			{
				if ( std::find( conPos.begin() , conPos.end() , cPos ) == conPos.end() )
					test = true;
			}
			else if ( tile.obj == MO_WALL )
			{
				test = true;
			}

			if ( test )
			{
				tile.obj  = MO_NULL;
				tile.meta = 0;
				conPos.push_back( cPos );

				if ( conPos.size() >= numCon )
					break;

				for( int i = 0 ; i < 4 ; ++i )
				{
					Vec2i tPos = cPos + getDirOffset( i );

					if ( 0 <= tPos.x && tPos.x < world.getMapSize().x &&
						 0 <= tPos.y && tPos.y < world.getMapSize().y )
					{
						stackPos.push_back( tPos );
					}
				}
			}
		}
	}

	void IMapGenerator::generateHollowTile( World& world , Vec2i const& start , Vec2i const& size , int depth , int id )
	{
		Vec2i end = start + size;
		Vec2i pos;
		for( pos.x = start.x ; pos.x < end.x ; ++pos.x )
		{
			for( int n = 0 ; n < depth ; ++n )
			{
				{
					pos.y = n;
					TileData& tile = world.getTile( pos );

					tile.obj  = id;
					tile.meta = 0;
				}

				{
					pos.y = end.y - n - 1;
					TileData& tile = world.getTile( pos );
					tile.obj  = id;
					tile.meta = 0;
				}
			}
		}

		for( pos.y = start.y ; pos.y < end.y ; ++pos.y )
		{
			for( int n = 0 ; n < depth ; ++n )
			{
				{
					pos.x = n;
					TileData& tile = world.getTile( pos );
					tile.obj  = id;
					tile.meta = 0;
				}

				{
					pos.x = end.x - n - 1;
					TileData& tile = world.getTile( pos );
					tile.obj  = id;
					tile.meta = 0;
				}
			}
		}
	}

	void IMapGenerator::generateRail( World& world , Vec2i const posStart , Vec2i const posEnd , int numNode )
	{
		int const lineRailOffset[2] = { makeRailOffset( DIR_WEST , DIR_EAST ) , makeRailOffset( DIR_SOUTH , DIR_NORTH ) };
		int idx1 = world.getRandom( 1 );
		int idx2 = ( idx1 + 1 ) % 2;

		//Dir prevDir = ( idx1 ) == 0 ? ;

		int offset = ( posEnd[ idx1 ] > posStart[ idx1 ] ) ?  1 : -1;

		Vec2i pos = posStart;
		{
			TileData& tile = world.getTile( pos );
			assert( !( MT_RAIL < tile.terrain && tile.terrain < MT_RAIL_END ) );
			tile.terrain = MT_RAIL + lineRailOffset[ idx1 ];
		}

		pos[ idx2 ] = posStart[ idx2 ];
		pos[ idx1 ] = posStart[ idx1 ] + offset;
		for(  ; pos[ idx1 ] != posEnd[ idx1 ] ; pos[ idx1 ] += offset )
		{
			TileData& tile = world.getTile( pos );
			assert( !( MT_RAIL < tile.terrain && tile.terrain < MT_RAIL_END ) );
			tile.terrain = MT_RAIL + lineRailOffset[ idx1 ];
		}

		{
			TileData& tile = world.getTile( pos );
			assert( !( MT_RAIL < tile.terrain && tile.terrain < MT_RAIL_END ) );
			tile.terrain = MT_RAIL + makeRailOffset( 
				( posEnd[ idx1 ] > posStart[ idx1 ] ) ? Dir( idx1 + 2 ) : Dir( idx1 )  , 
				( posEnd[ idx2 ] > posStart[ idx2 ] ) ? Dir( idx2 ) : Dir( idx2 + 2 )  );
		}
		offset = ( posEnd[ idx2 ] > posStart[ idx2 ] ) ? 1 : -1;

		pos[ idx1 ] = posEnd[ idx1 ];
		pos[ idx2 ] = posStart[ idx2 ] + offset;
		for( ; pos[ idx2 ] != posEnd[ idx2 ] ; pos[ idx2 ] += offset )
		{
			TileData& tile = world.getTile( pos );
			assert( !( MT_RAIL < tile.terrain && tile.terrain < MT_RAIL_END ) );
			tile.terrain = MT_RAIL + lineRailOffset[ idx2 ];
		}

		{
			TileData& tile = world.getTile( pos );
			assert( !( MT_RAIL < tile.terrain && tile.terrain < MT_RAIL_END ) );
			tile.terrain = MT_RAIL + lineRailOffset[ idx2 ];
		}
	}

	CSimpleMapGen::CSimpleMapGen( Vec2i const& size , uint16 const* map ) 
		:mMapSize( size ) 
		,mMapBase( map )
	{
		assert( size.x > 5 && size.y > 5 );
		mEntry[0] = Vec2i( 1 ,1 );
		mEntry[1] = Vec2i( size.x - 2 , size.y - 2 );
		mEntry[2] = Vec2i( size.x - 2 , 1 );
		mEntry[3] = Vec2i( 1 , size.y - 2 );
	}

	void CSimpleMapGen::generate( World& world , ModeId id , int playeNum , bool beInit )
	{
		world.setupMap( mMapSize , mMapBase );
		generateHollowTile( world , Vec2i(0,0) , mMapSize , 1 , MO_BLOCK );
		generateGapTile( world , Vec2i(1,1) , mMapSize - Vec2i(2,2) , MO_BLOCK );

		Vec2i tPosRailStart( 5 , 5 );
		generateRail( world , tPosRailStart , Vec2i( 11 , 11 ) , 0 );
		
		//world.addEntity( new MineCar( tPosRailStart ) , true );
		//generateRandTile( world , Vec2i(0,0) , mMapSize , 70 , MO_WALL );

		//world.getTile( Vec2i(3,3) ).terrain  = MT_ARROW_CHANGE + DIR_EAST;
		//world.getTile( Vec2i(15,3) ).terrain = MT_ARROW + DIR_SOUTH;

		//world.getTile( Vec2i(15,1) ).obj = MO_WALL;
		//world.getTile( Vec2i(5,1) ).obj  = MO_WALL;
		//world.getTile( Vec2i(6,1) ).obj  = MO_WALL;

		for ( int i = 0 ; i < playeNum ; ++i )
			generateConRoad( world , mEntry[i] , 4 );
	}


	void Mode::restart( bool beInit )
	{
		World& world = getWorld();

		world.restart();
		getMapGenerator().generate( world , getId() , world.getPlayerNum() , beInit );

		for( int i = 0 ; i < world.getPlayerNum() ; ++i )
		{
			Vec2i pos = getMapGenerator().getPlayerEntry( i );
			Player* player = getWorld().getPlayer( i );
			player->changeState( Player::STATE_NORMAL );
			player->setPosWithTile( pos );
		}

		setState( eRUN );
		onRestart( beInit );
		getWorld().getAnimManager()->restartLevel( beInit );
	}

	Mode::Mode( ModeId id , IMapGenerator& mapGen ) 
		:mId( id )
		,mLevel( NULL )
		,mMapGen( &mapGen )
	{
		
	}

	void Mode::tick()
	{
		getWorld().tick();
		onTick();
	}

	void Mode::onBreakTileObject( Vec2i const& pos , int prevId , int prevMeta )
	{
		TileData& tile = getWorld().getTile( pos );
		switch( prevId )
		{
		case MO_WALL:
			{
				int tool;
				if ( produceTool( tool ) )
				{
					tile.obj  = MO_ITEM;
					tile.meta = tool;
				}
			}
			break;
		}
	}

	void Mode::onPlayerEvent( EventId id , Player& player )
	{
		switch( id )
		{
		case EVT_ACTOR_DIE:
			break;
		}
	}

	Player* Mode::addPlayer()
	{
		int idx = getWorld().getPlayerNum();
		Vec2i pos = getMapGenerator().getPlayerEntry( idx );
		return getWorld().addPlayer( pos );
	}

	void Mode::setupWorld( World& world )
	{
		mLevel = &world;
		world.setListener( this );
	}

	BattleMode::BattleMode( IMapGenerator& mapGen ) 
		:Mode( MODE_BATTLE , mapGen )
	{

	}

	void BattleMode::onRestart( bool beInit )
	{
		Info& info = mInfo;

		for( int i = 0 ; i < getWorld().getPlayerNum() ; ++i )
		{
			Player* player = getWorld().getPlayer( i );

			player->spawn( false );

			PlayerStatus& status = player->getStatus();
			status.maxNumBomb = info.numBomb;
			status.power      = info.power;

			if ( beInit )
			{
				mPlayerGameStatus[i].numWin = 0;
			}
		}

		mTime = 0;
	}

	void BattleMode::onTick()
	{

		mTime += gDefaultTickTime;

		int numPlayer = 0;
		World& world = getWorld();

		Player* winPlayer = NULL;
		for( int i = 0 ; i < world.getPlayerNum(); ++i )
		{
			Player* player = world.getPlayer( i );
			if ( player->isAlive() )
			{
				winPlayer = player;
				++numPlayer;
			}
		}
		if ( numPlayer == 0 )
		{
			mLastWinPlayer = NULL;
			setState( Mode::eROUND_END );
		}
		else if ( numPlayer == 1 )
		{
			mLastWinPlayer = winPlayer;

			mLastWinPlayer->setFaceDir( DIR_SOUTH );
			getWorld().getAnimManager()->setPlayerAnim( *mLastWinPlayer , ANIM_VICTORY );
			mPlayerGameStatus[ winPlayer->getId()].numWin += 1;
			if ( mPlayerGameStatus[ winPlayer->getId()].numWin < getInfo().numWinRounds )
				setState( Mode::eROUND_END );
			else
				setState( Mode::eGAME_OVER );
			mbIsOver = true;
		}
		else
		{
			if ( getInfo().decArea )
			{





			}
		}

	}

	bool BattleMode::produceTool( int& tool )
	{
		if ( getWorld().getRandom( Global::RandomNet() % 100 ) > 50 )
			return false;
		tool = getWorld().getRandom( NUM_ITEM_TYPE );
		return true;
	}

	void BattleMode::onPlayerEvent( EventId id , Player& player )
	{
		switch( id )
		{
		case EVT_ACTOR_DIE:

			break;

		}
	}

}//namespace BomberMan