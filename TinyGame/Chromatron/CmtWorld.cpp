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

	bool World::isRange(Vec2D const& pos) const
	{
		return mTileMap.checkRange( pos.x , pos.y );
	}

	Tile const& World::getMapData(Vec2D const& pos) const
	{
		return mTileMap.getData( pos.x , pos.y );
	}

	Tile& World::getMapData( Vec2D const& pos )
	{
		return mTileMap.getData( pos.x , pos.y );
	}


	static Vec2D const gTestOffset[] = { Vec2D(0,-1) , Vec2D(-1,-1) , Vec2D(-1,0) , Vec2D(0,0) };

	bool World::transmitLightStep( LightTrace& light , Tile** curData )
	{
		assert( &getMapData( light.getEndPos()  ) == *curData );

		(*curData)->addLightPathColor( light.getDir(),light.getColor() );

		if ( light.getDir() % 2 == 1 )
		{
			Vec2D testPos = light.getEndPos() + gTestOffset[ light.getDir() / 2 ];
			if ( isRange( testPos ) && getMapData( testPos ).blockRBConcerLight() )
				return false;
		}

		light.advance();

		if ( !isRange( light.getEndPos() ) ) 
		{
			*curData = NULL;
			return false;
		}

		Tile& mapData = getMapData( light.getEndPos() );
		*curData = &mapData;

		if ( mapData.blockLight() )
			return false;

		mapData.addLightPathColor( light.getDir().inverse() , light.getColor() );

		if ( mapData.getDevice() )
			return false;

		return true;
	}

	TransmitStatus World::transmitLightSync( WorldUpdateContext& context , SyncProcessor& processor , LightList& transmitLights )
	{
		bool prevMode = context.isSyncMode();
		context.setSyncMode( true );

		SyncProcessor* prevProvider  = context.mSyncProcessor;
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

				Tile* mapData = &getMapData( curLight.getEndPos() );
				bool canKeep = transmitLightStep( curLight , &mapData );

				Device* pDC = ( mapData ) ? ( mapData->getDevice() ) : NULL;

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

						Vec2D const& pos = light.getEndPos();

						assert( isRange( pos ) );

						Tile& mapData = getMapData( pos );
						Device* dc = mapData.getDevice();
						assert( dc );

						if ( processor.prevEffectDevice( *dc , light , pass ) )
						{
							if ( procDeviceEffect( context , *dc , light ) != TSS_OK )
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
			
			Tile* mapData = &getMapData( light.getEndPos() );

			for(;;)
			{
				if ( !transmitLightStep( light , &mapData ) )
					break;
			}

			if ( mapData )
			{
				Device* dc = mapData->getDevice();
				if ( dc )
				{
					TransmitStatus status = procDeviceEffect( context , *dc , light );
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

	int World::countSameLighPathColortStepNum(Vec2D const& pos, Dir dir) const
	{
		assert(isRange(pos));
		Color color = getMapData(pos).getLightPathColor(dir);
		if ( color == COLOR_NULL )
			return 0;
		int result = 1;
		Dir invDir = dir.inverse();
		Vec2D testPos = pos;
		for (;;)
		{
			testPos += LightTrace::GetDirOffset(dir);
			if (isRange(testPos) == false)
				break;

			Tile const& tile = getMapData(testPos);
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

	bool World::isLightPathEndpoint(Vec2D const& pos, Dir dir) const
	{
		assert(isRange(pos));

		Tile const& tile = getMapData(pos);
		if (tile.getDevice() == nullptr)
		{
			Color color = tile.getLightPathColor(dir);
			if (color == COLOR_NULL)
				return false;
			Dir invDir = dir.inverse();
			if (color == tile.getLightPathColor(invDir))
			{
				Vec2D invPos = pos + LightTrace::GetDirOffset(invDir);
				if ( isRange( invPos ) && 
					 getMapData( invPos ).getLightPathColor( dir ) == color )
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
			mTileMap[i].setDevice( NULL );
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

	TransmitStatus World::procDeviceEffect( WorldUpdateContext& context , Device& dc , LightTrace const& light )
	{
		Tile& mapData = getMapData( dc.getPos() );

		Dir invDir = light.getDir().inverse();
		mapData.addReceivedLightColor( invDir , light.getColor() );

		if ( dc.getFlag() & DFB_SHOT_DOWN )
			return TSS_OK;

		if ( dc.getFlag() & DFB_LAZY_EFFECT )
		{
			Color lazyColor = mapData.getLazyLightColor( invDir );
			if ( lazyColor &&  mapData.getReceivedLightColor( invDir ) != lazyColor )
				return TSS_OK;
		}

		context.mStatus   = TSS_OK;
		int prevAge = context.mLightAge;
		context.mLightAge = light.getAge();
		
		dc.effect( context , light );
		
		context.mLightAge = prevAge;
		return context.mStatus;
	}

	Device* World::goNextDevice( Dir dir , Vec2D& curPos )
	{
		for(;;)
		{
			curPos += LightTrace::GetDirOffset( dir );
			if( !isRange( curPos ) )  
				break;

			Device* pDC = getMapData( curPos ).getDevice();

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
		mIsSyncMode = false;
	}

	void WorldUpdateContext::addLight(Vec2D const& pos , Color color , Dir dir)
	{
		if ( color == COLOR_NULL )
			return;

		Tile& mapData = mWorld.getMapData( pos );

		if ( isSyncMode() )
		{
			assert( mSyncProcessor );
			mapData.addEmittedLightColor( dir , color );

			if ( !mSyncProcessor->prevAddLight(  pos , color , dir , mLightParam , mLightAge ) )
				return;

			LightTrace light( pos , color , dir , mLightParam , mLightAge );
			mSyncLights->push_front( light );
		}
		else
		{
			if ( ( color & mapData.getEmittedLightColor( dir ) ) == color )
				return;

			mapData.addEmittedLightColor( dir , color );
			LightTrace light( pos , color , dir , mLightParam , mLightAge );
			mNormalLights.push_back( light );
		}
		++mLightCount;
	}

}//namespace Chromatron
