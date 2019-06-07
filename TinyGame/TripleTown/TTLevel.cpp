#include "TinyGamePCH.h"
#include "TTLevel.h"

#include <algorithm>
#include "Singleton.h"
#include "CppVersion.h"

namespace TripleTown
{

	LevelListener gEmptyListener;
	PlayerData    gEmptyPlayerData;

	int const DirOffsetX[] = { 1 , -1 , 0 , 0 };
	int const DirOffsetY[] = { 0 , 0 , 1 , -1 };

	TilePos getNeighborPos( TilePos const& pos , int dir ){ return TilePos( pos.x + DirOffsetX[ dir ] , pos.y + DirOffsetY[ dir ] ); }

	int Random()
	{
		return rand();
	}
	float RandFloat()
	{
		return float(rand()) / RAND_MAX;
	}

	int const RELINK = -99999999;

	extern ObjectInfo gObjectInfo[];
	

	class ECNull   : public ObjectClass
	{
	public:
		ECNull():ObjectClass( OT_NULL ){}
	};


	class ECFunLand : public ObjectClass
	{
	public:
		ECFunLand():ObjectClass( OT_FUN_LAND ){}
		virtual void setup( Level& level , Tile& tile , TilePos const& pos , ObjectId id )
		{
			assert( tile.id == OBJ_NULL && !tile.haveActor() );
			level.markObject( tile , pos , id );
		}

		virtual bool use( Level& level , Tile& tile , TilePos const& pos , ObjectId id )
		{
			if ( !tile.isEmpty() )
				return false;
			level.markObject( tile , pos , id );
			return true;
		}

	};

	class ECBasicLand : public ObjectClass
	{
	public:
		ECBasicLand():ObjectClass( OT_BASIC ){}
		virtual void setup( Level& level , Tile& tile , TilePos const& pos , ObjectId id )
		{
			assert( tile.isEmpty() );
			level.markObjectWithLink( tile , pos , id );
		}

		virtual bool use( Level& level , Tile& tile , TilePos const& pos , ObjectId id )
		{
			return addObject( level , tile , pos , id );
		}

		virtual int peek( Level& level , Tile& tile , TilePos const& pos , ObjectId id , TilePos posRemove[] )
		{
			if ( !tile.isEmpty() )
				return 0;
			return peekObject( level , pos , id , posRemove );
		}
		virtual bool effect( Level& level , Tile& tile , TilePos const& pos )
		{
			return checkUpgrade( level , tile , pos );
		}

		virtual void onRemove(Level& level , TilePos const& pos , ObjectId id )
		{
			level.relinkNeighborTile( pos , id );
		}

		static bool addObject( Level& level , Tile& tile , TilePos const& pos , ObjectId id )
		{
			if ( !tile.isEmpty() )
				return false;


			int num = level.markObjectWithLink( tile , pos , id );
			checkUpgrade( level , tile , pos , num );
			return true;
		}
		static bool checkUpgrade( Level& level , Tile& tile , TilePos const& pos , int num = 0 )
		{
			assert( &tile == &level.getTile( pos ) );
			assert( level.GetInfo( tile.id ).typeClass->getType() == OT_BASIC );

			if ( num == 0 )
			{
				num = level.getLinkNum( pos );
			}

			assert( level.getLinkNum( pos ) == num );

			ObjectId id = tile.id;

			bool result =false;

			for( ; ; )
			{
				int numUpgrade = level.getUpgradeNum( id );
				if ( numUpgrade == 0 )
					break;

				if ( num < numUpgrade )
					break;

				bool bOverUpgradeNum = (num > numUpgrade);

				level.removeConnectObject( tile , pos );
				id  = level.getUpgradeId( id );
				num = level.markObjectWithLink( tile , pos , id );

				tile.bSpecial = bOverUpgradeNum;
				result = true;
			}

			return result;
		}

		static int peekObject( Level& level , TilePos const& pos , ObjectId id , TilePos posRemove[] )
		{
			assert( level.GetInfo( id ).typeClass->getType() == OT_BASIC );

			unsigned dirBitSkip = 0;
			int result = 0;
			for(;;)
			{
				if ( level.getUpgradeId(id) == OBJ_NULL )
					break;

				unsigned dirBit = 0;
				int numUpgradeNum = level.getUpgradeNum(id);
				assert(numUpgradeNum > 0);

				int num = level.countConnectLink( pos , id , dirBitSkip , dirBit );
				if ( num < numUpgradeNum - 1 )
					break;

				level.increaseCheckCount();
				for( int dir = 0 ; dir < DirNum ; ++dir )
				{
					if ( dirBit & BIT( dir ) )
					{
						TilePos nPos = getNeighborPos( pos , dir );
						result += level.getConnectObjectPos( nPos , id , posRemove + result );
					}
				}

				id = level.getUpgradeId( id );
				dirBitSkip |= dirBit;
			}

			return result;
		}

	};

	class ECChest : public ECBasicLand
	{
		typedef ECBasicLand BaseClass;
	public:
		ECChest( int numTool ):mNumTool( numTool ){}
		virtual void play( Level& level , Tile& tile , TilePos const& pos , ObjectId id )
		{
			assert( tile.id == OBJ_CHEST || tile.id == OBJ_LARGE_CHEST );
			level.removeObject(tile, pos);

			int coins = (id == OBJ_CHEST) ? 500 : 1500;

			level.getPlayerData().addCoins(coins);
			level.getListener().notifyAddCoins(coins);

		}

		virtual int peek(Level& level, Tile& tile, TilePos const& pos, ObjectId id, TilePos posRemove[])
		{
			if( id == OBJ_LARGE_CHEST )
				return 0;
			return BaseClass::peek(level, tile, pos, id, posRemove);
		}
		static ECChest* NoramlInstance(){ static ECChest out( 3 ); return &out; }
		static ECChest* LargeInstance(){ static ECChest out( 5 ); return &out; }

	private:
		int mNumTool;
	};

	class ECActor : public ObjectClass
	{
	public:
		ECActor():ObjectClass( OT_ACTOR ){}

		virtual void setup( Level& level , Tile& tile , TilePos const& pos , ObjectId id )
		{
			assert( tile.isEmpty() );
			level.addActor( tile , pos , id );
		}

		virtual bool use( Level& level , Tile& tile , TilePos const& pos , ObjectId id )
		{
			if ( !tile.isEmpty() )
				return false;
			level.addActor( tile , pos , id );
			return true;
		}

		static ECActor* FromId( ObjectId id )
		{
			assert( OBJ_ACTOR_START <= id && id < OBJ_ACTOR_START + NUM_ACTOR );
			return static_cast< ECActor* >( gObjectInfo[ id ].typeClass );
		}

		virtual void  evalState( Level& level , ActorData& e ){}
		virtual void  evolve( Level& level , ActorData& e ){}

		virtual bool  useTool( Level& level , ActorData& e , ObjectId id ){ return false; }

		virtual bool  effect( Level& level , Tile& tile , TilePos const& pos )
		{
			ActorData& e = level.getActor( tile );
			if ( level.updateActorState( e ) && e.state == STATE_DEAD )
			{
				ECActor::FromId( e.id )->evolve( level , e );
				return true;
			}
			return false;
		}

	};

	class ECBearBase : public ECActor
	{
	public:
		virtual void onRemove( Level& level , TilePos const& pos , ObjectId id )
		{
			ObjectId upId = level.getUpgradeId( id );
			level.addObject( pos , upId );
		}

		virtual bool useTool( Level& level , ActorData& e , ObjectId id )
		{
			switch( id )
			{
			case OBJ_ROBOT:
				{
					TilePos posOld = e.pos;
					level.killActor( e );
					level.checkEffect( posOld );
				}
				return true;
			}
			return false;
		}

		void die( Level& level , ActorData& e )
		{
			unsigned mask = ACTOR_MASK( OBJ_BEAR ) | ACTOR_MASK( OBJ_NINJA );
			TilePos posNew;
			level.killConnectActor( e , mask , posNew );
			level.checkEffect( posNew );
		}
	};

	class ECNinja : public ECBearBase 
	{
	public:

		virtual void evalState( Level& level , ActorData& e )
		{
			if ( level.getEmptyTileNum() == 0 )
			{
				e.state = STATE_DEAD;
			}
		}

		virtual void evolve( Level& level , ActorData& e )
		{
			switch( e.state )
			{
			case STATE_DEAD:
				die( level , e );
				break;
			case STATE_ALIVE:
				{
					if ( level.getEmptyTileNum() == 0 )
						return;

					TileMap const& map = level.getMap();
					int num = Random() % level.getEmptyTileNum();
					for( int i = 0 ; i < map.getSizeX() ; ++i )
					{
						for( int j = 0 ; j < map.getSizeY() ; ++j )
						{
							Tile const& tileCheck = map.getData( i , j );
							if ( !tileCheck.isEmpty() )
								continue;

							if ( num )
							{
								--num;
							}
							else
							{
								level.moveActor( e , TilePos( i , j ) );
								return;
							}
						}
					}
				}
				break;
			}
		}
	};

	class ECBear : public ECBearBase 
	{
	public:
		virtual void evalState( Level& level , ActorData& e )
		{
			if ( level.getEmptyTileNum() == 0 || !checkBearAlive( level , e ) )
			{
				e.state = STATE_DEAD;
			}
		}

		virtual void evolve( Level& level , ActorData& e )
		{
			switch( e.state )
			{
			case STATE_DEAD:
				die( level , e );
				break;
			case STATE_ALIVE:
				{
					int dirPosible[4];
					int numPosible = 0;

					TileMap const& map = level.getMap();

					for( int dir = 0 ; dir < DirNum ; ++dir )
					{
						TilePos nPos = getNeighborPos( e.pos , dir );

						if ( !level.isMapRange( nPos ) )
							continue;

						Tile const& nTile = level.getTile( nPos );
						if ( nTile.isEmpty() )
						{
							dirPosible[ numPosible ] = dir;
							++numPosible;
						}
					}
					if ( numPosible )
					{
						int dir = dirPosible[ Random() % numPosible ];
						level.moveActor( e , getNeighborPos( e.pos , dir ) );
					}
				}
				break;
			}
		}


		static bool checkBearAlive( Level& level , ActorData& e )
		{
			assert( e.id == OBJ_BEAR );
			Tile& tile = level.getTile( e.pos );
			assert( tile.haveActor() );

			level.increaseCheckCount();
			tile.checkResult = false;
			for( int dir = 0 ; dir < DirNum ; ++dir )
			{
				if ( checkBearAlive_R( level , getNeighborPos( e.pos , dir ) ) )
				{
					tile.checkResult = true;
					break;
				}
			}
			return tile.checkResult;
		}

		static bool checkBearAlive_R( Level& level , TilePos const& pos )
		{
			if ( !level.isMapRange( pos ) )
				return false;

			Tile& tile = level.getTile( pos );

			if ( tile.isEmpty() )
				return true;

			if ( !tile.haveActor() )
				return false;

			if ( !level.testCheckCount( tile ) )
				return false;

			tile.checkResult = false;

			ActorData& e = level.getActor( tile );
			switch( e.id )
			{
			case OBJ_NINJA:
				tile.checkResult = true;
				break;
			case OBJ_BEAR:
				for ( int dir = 0 ; dir < DirNum ; ++dir )
				{
					if ( checkBearAlive_R( level , getNeighborPos( pos , dir ) ) )
					{
						tile.checkResult = true;
						break;
					}
				}
				break;
			default:
				tile.checkCount = level.mCheckCount - 1;
			}
			return tile.checkResult;
		}


	};

	class ECToolBase : public ObjectClass
	{
	public:
		ECToolBase():ObjectClass( OT_TOOL ){}
	};

	class ECStoreHouse : public ECFunLand
	{
	public:
		virtual void play( Level& level , Tile& tile , TilePos const& pos , ObjectId id )
		{
			if ( tile.meta )
			{
				level.setQueueObject( ObjectId( tile.meta ) );
				tile.meta = id;
			}
			else
			{
				tile.meta = id;
				level.nextQueueObject();
			}
		}

	};

	class ECCrystal : public ECToolBase
	{
	public:
		virtual bool use( Level& level , Tile& tile , TilePos const& pos , ObjectId id )
		{
			assert( id == OBJ_CRYSTAL );
			if ( !tile.isEmpty() )
				return false;

			ObjectId idUsed = evalCrystalBasicId( level , pos );
			if( idUsed == OBJ_NULL )
				return false;

			return ECBasicLand::addObject( level , tile , pos , idUsed );
		}

		virtual int peek( Level& level , Tile& tile , TilePos const& pos , ObjectId id , TilePos posRemove[] )
		{
			if( !tile.isEmpty() )
				return 0;

			ObjectId idUsed = evalCrystalBasicId(level, pos);
			if( idUsed == OBJ_NULL )
				return 0;
			return ECBasicLand::peekObject(level, pos, idUsed, posRemove);
		}

		static ObjectId evalCrystalBasicId( Level& level , TilePos const& pos )
		{
			unsigned dirBitUsed = 0;

			ObjectId id = OBJ_NULL;
			for( int dir = 0 ; dir < DirNum ; ++dir )
			{
				if ( dirBitUsed & BIT( dir ) )
					continue;

				TilePos nPos = getNeighborPos( pos , dir );

				if ( !level.isMapRange( nPos ) )
					continue;

				Tile& nTile = level.getTile( nPos );
				ObjectId nId = level.getObjectId( nTile );

				if ( level.getUpgradeId(nId) == OBJ_NULL )
					continue;

				auto nObjInfo = level.GetInfo(nId);
				if ( nObjInfo.typeClass->getType() == OT_BASIC )
				{
					if ( nId > id )
					{
						int num = level.countConnectLink( pos , nId , dirBitUsed , dirBitUsed );
						int upgradeNum = level.getUpgradeNum(nId);
						if ( num >= level.getUpgradeNum( nId ) - 1 )
						{
							id = nId;
						}
					}
				}
			}

			if( id == OBJ_NULL )
				return level.getUpgradeId( OBJ_CRYSTAL );

			return id;
		}

	};

	class ECRobot : public ECToolBase
	{
	public:
		virtual bool use( Level& level , Tile& tile , TilePos const& pos  , ObjectId id )
		{
			if ( tile.isEmpty() )
				return false;

			if ( tile.haveActor() )
			{
				ActorData& e = level.getActor( tile );
				return ECActor::FromId( e.id )->useTool( level , e , OBJ_ROBOT );
			}

			switch( tile.id )
			{
			case OBJ_NULL:
			case OBJ_STOREHOUSE:
			case OBJ_CHEST:
			case OBJ_LARGE_CHEST:
				return false;
			case OBJ_MOUNTAIN:
				level.removeObject(tile, pos);
				level.useObject(pos, OBJ_CHEST);
				break;
			default:
				level.removeObject(tile, pos);
				break;
			}
			return true;
		}
	};

	class ECUndoMove : public ECToolBase
	{
	public:
	};

	class ECCookie : public ECToolBase
	{
	public:
	};

	ObjectInfo const& Level::GetInfo( ObjectId id )
	{
		return gObjectInfo[ id ];
	}

	Level::Level()
	{
#ifdef _DEBUG
		for( int i = 0 ; i < NUM_OBJ ; ++i )
		{
			assert( gObjectInfo[i].id == i );
		}
#endif // _DEBUG
		mBGTerrain = TT_GRASS;
		mPlayerData = &gEmptyPlayerData;
		mListener = &gEmptyListener;
	}

	void Level::setupLand( LandType type , bool bTestMode )
	{
		getListener().prevPrevSetupLand();

		mLandType = type;

		ProduceInfo const* pInfo = NULL;
		int          numInfo = 0;

		ProduceInfo const infoPeaceful[] =
		{
			{ OBJ_BEAR , 0 } , { OBJ_NINJA , 0 } ,
		};

		ProduceInfo const infoDangerous[] =
		{
			{ OBJ_BEAR , 15 } , { OBJ_NINJA , 2.5 } ,
		};

		switch ( type )
		{
		case LT_STANDARD:
			mMap.resize( 6 , 6 );
			break;
		case LT_LAKE:
			mMap.resize( 6 , 6 );
			break;
		case LT_PEACEFUL:
			mMap.resize( 5 , 5 );
			pInfo  = infoPeaceful;
			numInfo = sizeof( infoPeaceful ) / sizeof( infoPeaceful[0] );
			break;
		case LT_DANGERUOS:
			mMap.resize( 6 , 5 );
			pInfo  = infoDangerous;
			numInfo = sizeof( infoDangerous ) / sizeof( infoDangerous[0] );
			break;
		}

		if( bTestMode )
		{
			ProduceInfo const infoTest[] =
			{
				{ OBJ_GRASS , 100 } ,
			};
			setupRandProduce( EProduceOverwriteMode::ReplaceAll , infoTest, ARRAY_SIZE(infoTest));
		}
		else
		{
			setupRandProduce( EProduceOverwriteMode::ReplaceObject , pInfo, numInfo);
		}
		restart();
		getListener().postSetupLand();
	}

	void Level::restart()
	{
		mMap.fillValue( Tile::InitTile() );
		mNumEmptyTile = mMap.getSizeX() * mMap.getSizeY();

		mCheckCount = 0;
		mStep = 0;
		mActorStorage.clear();
		mIdxFreeEntity = -1;

		switch( mLandType )
		{
		case LT_STANDARD:
			{
				int numRock = 1 + ((Random() % 100) < 30 ? 1 : 0);
				for( int i = 0; i < numRock; ++i )
				{
					int x, y;
					do
					{
						x = Random() % mMap.getSizeX();
						y = Random() % mMap.getSizeY();
					} while( x == 0 && y == 0 );

					addObject(TilePos(x, y), OBJ_ROCK, true);
				}
				addObject(TilePos(0, 0), OBJ_STOREHOUSE, true);
			}
			break;
		case LT_LAKE:
			setTerrain( TilePos( 2 , 3 ) , TT_WATER );
			setTerrain( TilePos( 3 , 2 ) , TT_WATER );
			setTerrain( TilePos( 3 , 3 ) , TT_WATER );
			addObject( TilePos( 2 , 2 ) , OBJ_STOREHOUSE, true);
			break;
		}
		resetObjectQueue();
	}

	void Level::setTerrain( TilePos const& pos , TerrainType type )
	{
		assert( isMapRange( pos ) );
		Tile& tile = getTile( pos );
		assert( tile.isEmpty() );
		tile.terrainBase = type;
		if ( type != TT_LAND )
			--mNumEmptyTile;
	}

	void Level::setupRandProduce(EProduceOverwriteMode mode, ProduceInfo const overwriteProduces[] , int num )
	{
		if( mode == EProduceOverwriteMode::ReplaceAll )
		{
			mNumRandProduces = num;
			std::copy(overwriteProduces, overwriteProduces + num, mRandProduceMap);
		}
		else if ( mode == EProduceOverwriteMode::ReplaceObject )
		{
			ProduceInfo temp[NUM_OBJ];
			std::copy(overwriteProduces, overwriteProduces + num, temp);
			std::sort(temp, temp + num, [](Level::ProduceInfo const& p1, Level::ProduceInfo const& p2)
			{
				return p1.id < p2.id;
			});

			int idxReplace = 0;
			mNumRandProduces = 0;
			mTotalRate = 0;
			for( int i = 0; i < NUM_OBJ; ++i )
			{
				ObjectInfo const& info = gObjectInfo[i];

				int rate = info.randRate;
				if( idxReplace < num && temp[idxReplace].id == i )
				{
					rate = temp[idxReplace].rate;
					++idxReplace;
				}

				if( rate > 0 )
				{
					mRandProduceMap[mNumRandProduces].id = info.id;
					mRandProduceMap[mNumRandProduces].rate = rate;
					mTotalRate += rate;
					++mNumRandProduces;
				}
			}
		}

		std::sort(mRandProduceMap, mRandProduceMap + mNumRandProduces, [](auto const& lhs, auto const& rhs)
		{
			return lhs.rate < rhs.rate;
		});
	}

	void Level::resetObjectQueue()
	{
		for ( int i = 0 ; i < ObjQueueSize ; ++i )
		{
			mObjQueue[i] = randomObject();
		}
		mIdxNextUse = 0;
	}

	ObjectId Level::randomObject()
	{
		float value = mTotalRate *  RandFloat();
		for( int i = 0 ; i < mNumRandProduces ; ++i )
		{
			ProduceInfo const& info = mRandProduceMap[ i ];
			if ( value < info.rate )
				return info.id;
			value -= info.rate;
		}
		return OBJ_NULL;
	}


	void Level::clickTile( TilePos const& pos , bool canUse )
	{
		ObjectId id = mObjQueue[ mIdxNextUse ];

		Tile& tile = getTile( pos );

		ObjectId tileId = getObjectId( tile );

		if ( canUse && useObjectImpl( tile , pos , id ) )
		{
			runStep( tile, pos );
			nextQueueObject();
		}
		else if ( tileId != OBJ_NULL )
		{
			GetInfo( tileId ).typeClass->play( *this , tile , pos , id );
		}
	}

	bool Level::useObject( TilePos const& pos , ObjectId id )
	{
		assert( isMapRange( pos ) );
		Tile& tile = getTile( pos );
		return useObjectImpl( tile , pos , id );
	}

	bool Level::useObjectImpl( Tile& tile , TilePos const& pos , ObjectId id )
	{
		assert( id != OBJ_STOREHOUSE );
		return GetInfo( id ).typeClass->use( *this , tile , pos , id );
	}

	void Level::addObject( TilePos const& pos , ObjectId id , bool bInit )
	{
		assert( GetInfo( id ).typeClass->getType() == OT_BASIC ||
			    GetInfo( id ).typeClass->getType() == OT_FUN_LAND ||
			    GetInfo( id ).typeClass->getType() == OT_ACTOR );

		assert( isMapRange( pos ) );
		Tile& tile = getTile( pos );
		GetInfo( id ).typeClass->setup( *this , tile , pos , id );
		if ( !bInit )
		{
			addPoints(id);
		}
		
	}

	void Level::runStep( Tile& tile , TilePos const& pos )
	{
		++mStep;
		increaseCheckCount();

		checkEffect( pos );
		for( int dir = 0 ; dir < DirNum ; ++dir )
		{
			TilePos nPos = getNeighborPos( pos , dir );
			if ( !isMapRange( nPos ) )
				continue;
			checkEffect( nPos );
		}

		unsigned idxCheck = 0;
		while( idxCheck < mIdxUpdateQueue.size() )
		{
			ActorData& e = mActorStorage[ mIdxUpdateQueue[ idxCheck ] ];

			if ( updateActorState( e ) && e.state == STATE_DEAD )
			{
				ECActor::FromId( e.id )->evolve( *this , e );
				idxCheck = 0;		
			}
			else
			{
				++idxCheck;
			}
		}
	
		for( IdxList::iterator iter = mIdxUpdateQueue.begin();
			iter != mIdxUpdateQueue.end() ; ++iter )
		{
			ActorData& e = mActorStorage[ *iter ];
			assert( getTile( e.pos ).meta == *iter );
			ECActor::FromId( e.id )->evolve( *this , e  );
		}
	}

	void Level::checkActor( Tile& tile , TilePos const& pos )
	{
		assert( &tile == &getTile( pos ) );
		if ( !tile.haveActor() )
			return;

		ActorData& e = getActor( tile );
		if ( updateActorState( e ) && e.state == STATE_DEAD )
			ECActor::FromId( e.id )->evolve( *this , e );
	}

	bool Level::updateActorState( ActorData& e )
	{
		Tile& tile = getTile( e.pos );
		//Entity have updated state
		if ( e.stepUpdate == mStep )
			return false;

		e.stepUpdate = mStep;
		ECActor* actorClass = ECActor::FromId( e.id );
		actorClass->evalState( *this , e );


		return true;
	}

	int Level::peekObject( TilePos const& pos , ObjectId id , TilePos posRemove[] )
	{

		if ( !isMapRange( pos ) )
			return 0;

		Tile& tile = getTile( pos );

		int num = GetInfo( id ).typeClass->peek( *this , tile , pos , id , posRemove );

		return num;
	}

	void Level::removeConnectObject( Tile& tile , TilePos const& pos )
	{
		removeConnectObjectImpl_R( tile , pos , tile.id );
	}

	void Level::removeConnectObject_R( TilePos const& pos , ObjectId id )
	{
		if ( !isMapRange( pos ) )
			return;
		Tile& tile = getTile( pos );
		if ( tile.id != id )
			return;

		removeConnectObjectImpl_R( tile , pos , id );
	}

	inline void Level::removeConnectObjectImpl_R( Tile& tile , TilePos const& pos , ObjectId id )
	{
		tile.id   = OBJ_NULL;
		tile.meta = 0;
		tile.bSpecial = false;
		++mNumEmptyTile;
		getListener().notifyObjectRemoved( pos , id );

		for ( int i = 0 ; i < DirNum ; ++i )
			removeConnectObject_R( getNeighborPos( pos , i ) , id );
	}

	void Level::markObject( Tile& tile , TilePos const& pos , ObjectId id )
	{
		assert( &tile == &getTile( pos ) );
		assert( id != OBJ_NULL );
		assert( tile.isEmpty() );
		tile.id   = id;
		tile.link = -1;
		tile.meta =  0;
		tile.bSpecial = false;
		--mNumEmptyTile;
		addPoints(id);
	}

	int Level::markObjectWithLink( Tile& tile , TilePos const& pos , ObjectId id )
	{
		markObject(tile, pos, id);

		int idxRoot = mMap.toIndex( pos.x , pos.y );
		int result = 1;
		for ( int i = 0 ; i < DirNum ; ++i )
			result += linkTile( getNeighborPos( pos , i ) , id , idxRoot );

		return result;
	}

	int Level::countConnectLink( TilePos const& pos , ObjectId id , unsigned dirBitSkip , unsigned& dirBitUsed )
	{
		int result = 0;
		int countRoot[ DirNum ];
		int countNum = 0;
		for ( int dir = 0 ; dir < DirNum ; ++dir )
		{
			if ( dirBitSkip & BIT( dir ) )
				continue;

			TilePos nPos = getNeighborPos( pos , dir );


			if ( !isMapRange( nPos ) )
				continue;
			Tile& tileCon = getTile( nPos );
			if ( tileCon.id != id )
				continue;

			dirBitUsed |= BIT( dir );

			int idx = getLinkRoot( nPos );
			bool isOK = true;
			for ( int i = 0 ; i < countNum ; ++ i)
			{
				if ( countRoot[i] == idx )
					isOK = false;
			}
			if ( isOK )
			{
				result += -mMap[ idx ].link;
				countRoot[ countNum ] = idx;
				++countNum;
			}
		}
		return result;
	}

	void Level::setListener( LevelListener* listener )
	{
		mListener = ( listener ) ? listener : ( &gEmptyListener );
	}

	void Level::nextQueueObject()
	{
		mObjQueue[ mIdxNextUse ] = randomObject();
		++mIdxNextUse;
		if ( mIdxNextUse == ObjQueueSize )
			mIdxNextUse = 0;
	}

	int Level::getLinkRoot( TilePos const& pos )
	{
		assert( isMapRange( pos ) );
		int idx = mMap.toIndex( pos.x , pos.y );
		for( ;; )
		{
			Tile& tile = mMap[ idx ];
			if ( tile.link < 0 )
				break;
			idx = tile.link;
		}
		return idx;
	}

	Tile* Level::getConnectTile( TilePos const& pos , int dir )
	{
		TilePos nPos = getNeighborPos( pos , dir );
		if ( !isMapRange( nPos ) )
			return NULL;
		return &getTile( nPos );
	}

	void Level::removeObject( Tile& tile , TilePos const& pos )
	{
		assert( &tile == &getTile( pos ) );
		assert( tile.id != OBJ_NULL && !tile.haveActor() );

		ObjectId id = tile.id;
		tile.id   = OBJ_NULL;
		tile.meta = 0;
		++mNumEmptyTile;

		GetInfo( id ).typeClass->onRemove( *this , pos , id );

	}

	void Level::relinkNeighborTile( TilePos const& pos , ObjectId id )
	{
		for( int dir = 0 ; dir < DirNum ; ++dir )
		{
			Tile* pTile = getConnectTile( pos , dir );
			if ( pTile && pTile->id == id )
			{
				pTile->link = RELINK;
			}
		}

		increaseCheckCount();
		for ( int dir = 0 ; dir < DirNum ; ++dir )
			rebuildLink( getNeighborPos( pos , dir ) , id );
	}

	int Level::linkTile( TilePos const& pos , ObjectId id  , int& idxRoot )
	{
		if ( !isMapRange( pos ) )
			return 0;
		Tile& tile = getTile( pos );
		if ( tile.id != id )
			return 0;

		int idxConRoot = getLinkRoot( pos );
		if ( idxConRoot == idxRoot )
			return 0;

		Tile& tileRoot = mMap[ idxRoot ];
		Tile& tileConRoot = mMap[ idxConRoot ];

		int result = -tileConRoot.link;
		if ( tileConRoot.link < tileRoot.link )
		{
			tileConRoot.link += tileRoot.link;
			tileRoot.link = idxConRoot;
			idxRoot = idxConRoot;
		}
		else
		{
			tileRoot.link += tileConRoot.link;
			tileConRoot.link = idxRoot;
		}
		return result;
	}

	void Level::rebuildLink( TilePos const& pos , ObjectId id )
	{
		if ( !isMapRange( pos ) )
			return;

		Tile& tile = getTile( pos );
		if ( tile.id != id )
			return;
		if ( tile.link != RELINK )
			return;

		if( !testCheckCount(tile) )
			return;

		int idxRoot = mMap.toIndex( pos.x , pos.y );
		int num = 1;
		for ( int i = 0 ; i < DirNum ; ++i )
			num += relink_R( getNeighborPos( pos , i ) , id , idxRoot );

		tile.link = -num;
	}

	int Level::getLinkNum( TilePos const& pos )
	{
		assert( isMapRange( pos ) );

		int idx = getLinkRoot( pos );
		return -mMap[ idx ].link;
	}

	int Level::relink_R( TilePos const& pos , ObjectId id , int idxRoot )
	{
		if ( !isMapRange( pos ) )
			return 0;

		Tile& tile = getTile( pos );
		if ( tile.id != id )
			return 0;


		if( !testCheckCount(tile) )
			return 0;

		tile.link = idxRoot;

		int num = 1;

		for ( int dir = 0 ; dir < DirNum ; ++dir )
			num += relink_R( getNeighborPos( pos , dir ) , id , idxRoot );

		return num;
	}

	int Level::getUpgradeNum( ObjectId id )
	{
		return GetInfo( id ).numUpgrade;
	}

	ObjectId Level::getUpgradeId( ObjectId id )
	{
		return GetInfo( id ).idUpgrade;
	}

	ObjectId Level::getQueueObject( int order /*= 0 */ )
	{
		int idx = mIdxNextUse;
		if ( order )
		{
			idx += order;
			if ( idx >= ObjQueueSize )
				idx -= ObjQueueSize;
		}
		return mObjQueue[ idx ];
	}

	void Level::setQueueObject( ObjectId id , int order /*= 0 */ )
	{
		assert( id != OBJ_NULL );

		int idx = mIdxNextUse;
		if ( order )
		{
			idx += order;
			if ( idx >= ObjQueueSize )
				idx -= ObjQueueSize;
		}
		mObjQueue[ idx ] = id;
	}

	int Level::getConnectObjectPos( TilePos const& pos , ObjectId id , TilePos posConnect[] )
	{
		assert( isMapRange( pos ) );

		Tile& tile = getTile( pos );

		assert( tile.id == id );

		if ( !testCheckCount( tile ) )
			return 0;

		posConnect[0] = pos;

		int num = 1;
		for ( int dir = 0 ; dir < DirNum ; ++dir )
		{
			num += getConnectObjectPos_R( getNeighborPos( pos , dir ) , id , posConnect + num );
		}
		return num;
	}

	int Level::getConnectObjectPos_R( TilePos const& pos , ObjectId id , TilePos posConnect[] )
	{
		if ( !isMapRange( pos ) )
			return 0;

		Tile& tile = getTile( pos );
		if ( tile.id != id )
			return 0;

		if ( !testCheckCount( tile ) )
			return 0;

		posConnect[0] = pos;

		int num = 1;
		for ( int dir = 0 ; dir < DirNum ; ++dir )
		{
			num += getConnectObjectPos_R( getNeighborPos( pos , dir ) , id , posConnect + num );
		}
		return num;
	}

	int Level::addActor( Tile& tile , TilePos const& pos , ObjectId id )
	{
		assert( tile.isEmpty() );

		ActorData* pEntity;
		int idx;
		if ( mIdxFreeEntity >= 0 )
		{
			idx = mIdxFreeEntity;
			pEntity = &mActorStorage[ mIdxFreeEntity ];
			mIdxFreeEntity = pEntity->stepBorn;
		}
		else
		{
			idx = mActorStorage.size();
			mActorStorage.resize( mActorStorage.size() + 1 );
			pEntity = &mActorStorage.back();
		}

		tile.id   = OBJ_ACTOR;
		tile.meta = idx;

		pEntity->id  = id;
		pEntity->pos = pos;
		pEntity->stepBorn   = mStep;
		pEntity->stepUpdate = mStep;
		pEntity->state = STATE_ALIVE;

		--mNumEmptyTile;
		mIdxUpdateQueue.push_back( idx );
		getListener().nodifyActorAdded( pos , id );

		return tile.meta;
	}

	void Level::removeActor( Tile& tile )
	{
		assert( tile.haveActor() );

		int idx = tile.meta;
		ActorData& e = mActorStorage[ idx ];

		getListener().prevRemoveActor( tile , e );

		ObjectId id = e.id;
		TilePos pos = e.pos;

		assert( &getTile( pos ) == &tile );
		e.id       = OBJ_NULL;
		e.stepBorn = mIdxFreeEntity;
		mIdxFreeEntity = idx;

		mIdxUpdateQueue.erase(
			std::find( mIdxUpdateQueue.begin() , mIdxUpdateQueue.end() , idx ) );

		tile.id   = OBJ_NULL;
		tile.meta = 0;
		++mNumEmptyTile;

		GetInfo( id ).typeClass->onRemove( *this , pos , id );
		
		getListener().postRemoveActor( pos , id );
	}


	void Level::moveActor( ActorData& e , TilePos const& posTo )
	{
		moveActor( getTile( e.pos ) , e , posTo );
	}

	void Level::moveActor( Tile& tile , ActorData& e , TilePos const& posTo )
	{
		assert( &getActor( tile ) == &e );
		assert( isMapRange( posTo ) );

		Tile& tileNew = getTile( posTo );
		assert( tileNew.isEmpty() );

		tileNew.id   = OBJ_ACTOR;
		tileNew.meta = tile.meta;

		tile.id   = OBJ_NULL;
		tile.meta = 0;

		TilePos posOld = e.pos;
		e.pos = posTo;
		getListener().notifyActorMoved( e.id , posOld , posTo );
	}

	void Level::increaseCheckCount()
	{
		mCheckCount += 1;
		if ( mCheckCount == 0 )
		{
			int size = mMap.getSizeX() * mMap.getSizeY();
			for( int i = 0 ; i < size ; ++i )
				mMap[i].checkCount = 0;
			++mCheckCount;
		}
	}

	bool Level::testCheckCount( Tile& tile )
	{
		if ( tile.checkCount == mCheckCount )
			return false;
		tile.checkCount = mCheckCount;
		return true;
	}

	int Level::killConnectActor( Tile& tile , TilePos const& pos , unsigned bitMask , TilePos& posNew )
	{
		assert( &tile == &getTile( pos ) );
		assert( tile.haveActor() );

		ActorData& e = getActor( tile );
		assert( ( bitMask & ACTOR_MASK( e.id ) ) != 0 );

		KillInfo info;
		info.pos = e.pos;
		info.step  = e.stepBorn;
		killActor( tile , e );

		int num = 1;
		for( int dir = 0 ; dir < DirNum ; ++dir )
			num += killConnectActor_R( getNeighborPos( pos , dir ) , bitMask , info );

		posNew = info.pos;
		return num;
	}

	int Level::killConnectActor_R( TilePos const& pos , unsigned bitMask , KillInfo& info )
	{
		if ( !isMapRange( pos ) )
			return 0;
		Tile& tile = getTile( pos );

		if ( !tile.haveActor() )
			return 0;

		ActorData& e = getActor( tile );
		if ( ( bitMask & ACTOR_MASK( e.id ) ) == 0 )
			return 0;

		if ( e.stepBorn > info.step )
		{
			info.pos = e.pos;
			info.step = e.stepBorn;
		}

		killActor( tile , e );

		int num = 1;
		for( int dir = 0 ; dir < DirNum ; ++dir )
			num += killConnectActor_R( getNeighborPos( pos , dir ) , bitMask , info );

		
		return num;
	}

	void Level::killActor( Tile& tile , ActorData& e )
	{
		removeActor( tile );
	}

	void Level::addPoints(ObjectId id)
	{
		int points = GetInfo(id).points;
		if( points > 0 )
		{
			mPlayerData->addPoints(points);
			mListener->notifyAddPoints(points);
		}
	}

	bool Level::checkEffect(TilePos const& pos)
	{
		assert( isMapRange( pos ) );
		Tile& tile = getTile( pos );
		ObjectId id = getObjectId( tile );
		if ( id == OBJ_NULL )
			return false;
		return GetInfo( id ).typeClass->effect( *this , tile , pos );
	}

#define OBJINFO_MAP_BEGIN()\
	ObjectInfo gObjectInfo[] = {
#define OBJINFO( id , CPTR , randRate , numUp , pt, idUp  )\
	{ id , CPTR , idUp , numUp , randRate , pt } ,
#define OBJINFO_MAP_END() \
	};

	template < class T >
	T* MakeECInstance(){ static T out; return &out; }
#define MKS_PTR( CLASS ) MakeECInstance< CLASS >()


	OBJINFO_MAP_BEGIN()
		OBJINFO( OBJ_NULL            , MKS_PTR( ECNull )         , 0  , 0 ,      0, OBJ_NULL )
		OBJINFO( OBJ_GRASS           , MKS_PTR( ECBasicLand )    , 60 , 3 ,      5, OBJ_BUSH )
		OBJINFO( OBJ_BUSH            , MKS_PTR( ECBasicLand )    , 15 , 3 ,     20, OBJ_TREE )
		OBJINFO( OBJ_TREE            , MKS_PTR( ECBasicLand )    , 2  , 3 ,    100, OBJ_HUT )
		OBJINFO( OBJ_HUT             , MKS_PTR( ECBasicLand )    , 0.6, 3 ,    500, OBJ_HOUSE )
		OBJINFO( OBJ_HOUSE           , MKS_PTR( ECBasicLand )    , 0  , 3 ,   1500, OBJ_MANSION )
		OBJINFO( OBJ_MANSION         , MKS_PTR( ECBasicLand )    , 0  , 3 ,   5000, OBJ_CASTLE )
		OBJINFO( OBJ_CASTLE          , MKS_PTR( ECBasicLand )    , 0  , 3 ,	 20000, OBJ_FLOATING_CASTLE )
		OBJINFO( OBJ_FLOATING_CASTLE , MKS_PTR( ECBasicLand )    , 0  , 4 , 100000, OBJ_TRIPLE_CASTLE )
		OBJINFO( OBJ_TRIPLE_CASTLE   , MKS_PTR( ECBasicLand )    , 0  , 0 ,      0, OBJ_NULL )

		OBJINFO( OBJ_TOMBSTONE       , MKS_PTR( ECBasicLand )    , 0 , 3 ,       0, OBJ_CHURCH )
		OBJINFO( OBJ_CHURCH          , MKS_PTR( ECBasicLand )    , 0 , 3 ,    1000, OBJ_CATHEDRAL )
		OBJINFO( OBJ_CATHEDRAL       , MKS_PTR( ECBasicLand )    , 0 , 3 ,    5000, OBJ_CHEST )
		OBJINFO( OBJ_CHEST           , ECChest::NoramlInstance() , 0 , 3 ,   10000, OBJ_LARGE_CHEST )
		OBJINFO( OBJ_LARGE_CHEST     , ECChest::LargeInstance()  , 0 , 0 ,   50000, OBJ_NULL )

		OBJINFO( OBJ_ROCK            , MKS_PTR( ECBasicLand )    , 0, 3 ,        0, OBJ_MOUNTAIN )
		OBJINFO( OBJ_MOUNTAIN        , MKS_PTR( ECBasicLand )    , 0, 3 ,     1000, OBJ_LARGE_CHEST )

		OBJINFO( OBJ_STOREHOUSE      , MKS_PTR( ECStoreHouse )   , 0 , 0 , 0, OBJ_NULL )
		OBJINFO( OBJ_CRATE           , MKS_PTR( ECFunLand )      , 0 , 0 , 0, OBJ_NULL )
		OBJINFO( OBJ_TIME_MACHINE    , MKS_PTR( ECFunLand )      , 0 , 0 , 0, OBJ_NULL )

		OBJINFO( OBJ_BEAR            , MKS_PTR( ECBear )         , 15 , 0 , 0, OBJ_TOMBSTONE )
		OBJINFO( OBJ_NINJA           , MKS_PTR( ECNinja )        , 2.5 , 0 , 0, OBJ_TOMBSTONE )

		OBJINFO( OBJ_CRYSTAL         , MKS_PTR( ECCrystal  )     , 2.5 , 0 , 0, OBJ_ROCK )
		OBJINFO( OBJ_ROBOT           , MKS_PTR( ECRobot )        , 2.5 , 0 , 0, OBJ_NULL )
		OBJINFO( OBJ_UNDO_MOVE       , MKS_PTR( ECUndoMove )     , 0 , 0 , 0, OBJ_NULL )
		OBJINFO( OBJ_FORTUNE_COOKIE  , MKS_PTR( ECCookie )       , 0 , 0 , 0, OBJ_NULL )

	OBJINFO_MAP_END()

#undef  MKS_PTR
#undef  OBJINFO_MAP_BEGIN
#undef  OBJINFO
#undef  OBJINFO_MAP_END

}//namespace TripleTown