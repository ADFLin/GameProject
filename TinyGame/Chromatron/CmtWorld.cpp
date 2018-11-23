#include "CmtPCH.h"
#include "CmtWorld.h"

#include "CmtLightTrace.h"
#include "CmtDevice.h"

namespace Chromatron
{
	int const MaxTransmitLightNum = 10000;
	int const MaxTransmitStepCount = MaxTransmitLightNum * 10;

	World::World(int sx ,int sy)
	{
		initData( sx , sy );
	}

	World::~World()
	{

	}

	void World::initData( int sx , int sy )
	{
		mTileMap.resize( sx , sy );
	}

	bool World::isValidRange(Vec2i const& pos) const
	{
		return mTileMap.checkRange( pos.x , pos.y );
	}

	Tile const& World::getTile(Vec2i const& pos) const
	{
		return mTileMap.getData( pos.x , pos.y );
	}

	Tile& World::getTile( Vec2i const& pos )
	{
		return mTileMap.getData( pos.x , pos.y );
	}


	static Vec2i const gTestOffset[] = { Vec2i(0,-1) , Vec2i(-1,-1) , Vec2i(-1,0) , Vec2i(0,0) };

	bool World::transmitLightStep( LightTrace& light , Tile** curTile )
	{
		assert( &getTile( light.getEndPos()  ) == *curTile );

		(*curTile)->addLightPathColor( light.getDir(),light.getColor() );

		if ( light.getDir() % 2 == 1 )
		{
			Vec2i testPos = light.getEndPos() + gTestOffset[ light.getDir() / 2 ];
			if ( isValidRange( testPos ) && getTile( testPos ).blockRBConcerLight() )
				return false;
		}

		light.advance();

		if ( !isValidRange( light.getEndPos() ) ) 
		{
			*curTile = NULL;
			return false;
		}

		Tile& tile = getTile( light.getEndPos() );
		*curTile = &tile;

		if ( tile.blockLight() )
			return false;

		tile.addLightPathColor( light.getDir().inverse() , light.getColor() );

		if ( tile.getDevice() )
			return false;

		return true;
	}

	TransmitStatus World::transmitLightSync( WorldUpdateContext& context , LightSyncProcessor& processor , LightList& transmitLights )
	{
		bool prevMode = context.isSyncMode();
		context.setSyncMode( true );

		LightSyncProcessor* prevProvider  = context.mSyncProcessor;
		LightList*     prevLightList = context.mSyncLights;

		context.mSyncProcessor = &processor;
		context.mSyncLights    = &transmitLights;

		typedef std::list< LightList::iterator > IterList;
		IterList   eraseIters;

		int stepCount = 0;

		while( !transmitLights.empty() )
		{
			for ( LightList::iterator iter = transmitLights.begin();
				 iter != transmitLights.end() ;  )
			{

				LightTrace& curLight = (*iter);

				Tile* pTile = &getTile( curLight.getEndPos() );
				bool canKeep = transmitLightStep( curLight , &pTile );

				Device* pDC = ( pTile ) ? ( pTile->getDevice() ) : NULL;

				if ( pDC )
					eraseIters.push_back( iter );

				if ( canKeep || pDC )
					++iter;
				else
					iter = transmitLights.erase( iter );

			}

			if ( !eraseIters.empty() )
			{
				int pass = 0;

				do
				{
					for ( IterList::iterator eIter = eraseIters.begin();
						eIter != eraseIters.end() ; )
					{
						LightList::iterator lightIter = *eIter;
						LightTrace& light = *lightIter;

						Vec2i const& pos = light.getEndPos();

						assert( isValidRange( pos ) );

						Tile& tile = getTile( pos );
						Device* dc = tile.getDevice();
						assert( dc );

						if ( processor.prevEffectDevice( *dc , light , pass ) )
						{
							if ( evalDeviceEffect( context , *dc , light ) != TSS_OK )
								goto SyncEnd;

							transmitLights.erase( lightIter );
							eIter = eraseIters.erase( eIter );
						}
						else
						{
							++eIter;
						}
					}

					++pass;
				}
				while( !eraseIters.empty() );
			}

			++stepCount;
			if ( stepCount > MaxTransmitStepCount )
			{
				context.notifyStatus( TSS_INFINITE_LOOP );
				goto SyncEnd;
			}
		}

SyncEnd:
		context.mSyncLights    = prevLightList;
		context.mSyncProcessor = prevProvider;
		context.setSyncMode( prevMode );

		return context.mStatus;
	}


	TransmitStatus World::transmitLight( WorldUpdateContext& context )
	{
		int loopCount = 0;
		while( !context.mNormalLights.empty() )
		{
			LightTrace& light = context.mNormalLights.front();
			
			Tile* pTile = &getTile( light.getEndPos() );

			for(;;)
			{
				if ( !transmitLightStep( light , &pTile ) )
					break;
			}

			if ( pTile )
			{
				Device* dc = pTile->getDevice();
				if ( dc )
				{
					TransmitStatus status = evalDeviceEffect( context , *dc , light );
					if ( status != TSS_OK )
						return status;
				}
			}

			context.mNormalLights.pop_front();
			++loopCount;

			if ( loopCount > MaxTransmitLightNum )
				return TSS_INFINITE_LOOP;
		}
		return TSS_OK;
	}

	void World::clearLight()
	{
		int length = getMapSizeX() * getMapSizeY();
		for (int i=0;i<length ; ++i )
		{
			mTileMap[i].clearLight();
		}
	}

	int World::countSameLighPathColortStepNum(Vec2i const& pos, Dir dir) const
	{
		assert(isValidRange(pos));
		Color color = getTile(pos).getLightPathColor(dir);
		if ( color == COLOR_NULL )
			return 0;
		int result = 1;
		Dir invDir = dir.inverse();
		Vec2i testPos = pos;
		for (;;)
		{
			testPos += LightTrace::GetDirOffset(dir);
			if (isValidRange(testPos) == false)
				break;

			Tile const& tile = getTile(testPos);
			if ( tile.blockLight() )
				break;

			if (tile.getLightPathColor(invDir) != color )
				break;

			++result;

			if (tile.getDevice() != nullptr)
				break;

			if (tile.getLightPathColor(dir) != color )
				break;

			++result;	
		}


		return result;
	}

	bool World::isLightPathEndpoint(Vec2i const& pos, Dir dir) const
	{
		assert(isValidRange(pos));

		Tile const& tile = getTile(pos);
		if (tile.getDevice() == nullptr)
		{
			Color color = tile.getLightPathColor(dir);
			if (color == COLOR_NULL)
				return false;
			Dir invDir = dir.inverse();
			if (color == tile.getLightPathColor(invDir))
			{
				Vec2i invPos = pos + LightTrace::GetDirOffset(invDir);
				if ( isValidRange( invPos ) && 
					 getTile( invPos ).getLightPathColor( dir ) == color )
					return false;
			}
				
		}

		return true;
	}

	void World::clearDevice()
	{
		int length = getMapSizeX() * getMapSizeY();
		for (int i=0;i<length ; ++i )
		{
			mTileMap[i].setDeviceData( NULL );
		}
	}

	void World::fillMap( MapType type )
	{
		int length = getMapSizeX() * getMapSizeY();
		for (int i=0;i<length ; ++i )
		{
			mTileMap[i].setType( type );
		}
	}

	TransmitStatus World::evalDeviceEffect( WorldUpdateContext& context , Device& dc , LightTrace const& light )
	{
		Tile& tile = getTile( dc.getPos() );

		Dir invDir = light.getDir().inverse();
		tile.addReceivedLightColor( invDir , light.getColor() );

		if ( dc.getFlag() & DFB_SHOT_DOWN )
			return TSS_OK;

		if ( dc.getFlag() & DFB_LAZY_EFFECT )
		{
			Color lazyColor = tile.getLazyLightColor( invDir );
			if ( lazyColor && tile.getReceivedLightColor( invDir ) != lazyColor )
				return TSS_OK;
		}

		context.mStatus   = TSS_OK;
		int prevAge = context.mLightAge;
		context.mLightAge = light.getAge();
		
		dc.effect( context , light );
		
		context.mLightAge = prevAge;
		return context.mStatus;
	}

	Device* World::goNextDevice( Dir dir , Vec2i& curPos )
	{
		for(;;)
		{
			curPos += LightTrace::GetDirOffset( dir );
			if( !isValidRange( curPos ) )  
				break;

			Device* pDC = getTile( curPos ).getDevice();

			if ( pDC )
				return pDC;
		}
		return NULL;
	}


	void Tile::clearLight()
	{
		mLightPathColor = 0;
	}

	WorldUpdateContext::WorldUpdateContext(World& world) 
		:mWorld(world)
	{
		mLightAge = 0;
		mStatus = TSS_OK;
	}

	void WorldUpdateContext::prevUpdate()
	{
		mWorld.clearLight();
		mLightParam = 0;
		mLightCount = 0;
		mNormalLights.clear();
		mStatus = TSS_OK;
		mbSyncMode = false;
	}

	void WorldUpdateContext::addLight(Vec2i const& pos , Color color , Dir dir)
	{
		if ( color == COLOR_NULL )
			return;

		Tile& tile = mWorld.getTile( pos );

		if ( isSyncMode() )
		{
			assert( mSyncProcessor );
			tile.addEmittedLightColor( dir , color );

			if ( !mSyncProcessor->prevAddLight(  pos , color , dir , mLightParam , mLightAge ) )
				return;

			LightTrace light( pos , color , dir , mLightParam , mLightAge );
			mSyncLights->push_front( light );
		}
		else
		{
			if ( ( color & tile.getEmittedLightColor( dir ) ) == color )
				return;

			tile.addEmittedLightColor( dir , color );
			LightTrace light( pos , color , dir , mLightParam , mLightAge );
			mNormalLights.push_back( light );
		}
		++mLightCount;
	}

}//namespace Chromatron
