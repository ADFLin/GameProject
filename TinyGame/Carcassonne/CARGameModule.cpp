#include "CARGameModule.h"

#include <algorithm>

#include "CARPlayer.h"
#include "CARMapTile.h"
#include "CARGameSetting.h"
#include "CARParamValue.h"
#include "CARDebug.h"


namespace CAR
{
	struct FieldProc
	{
		void exec( int type , int value );
	};
	template< class FieldProc >
	void ProcField( GameSetting& setting , int numPlayer  , FieldProc& proc )
	{
#define FIELD_ACTOR( TYPE , VALUE )\
	proc.exec( FieldType::Enum( FieldType::eActorStart + TYPE ) , VALUE )
#define FIELD_VALUE( TYPE , VALUE )\
	proc.exec( TYPE , VALUE )

		FIELD_ACTOR( ActorType::eMeeple , Value::MeeplePlayerOwnNum );
		if ( setting.haveUse( EXP_INNS_AND_CATHEDRALS ) )
		{
			FIELD_ACTOR( ActorType::eBigMeeple , Value::BigMeeplePlayerOwnNum );
		}
		if ( setting.haveUse( EXP_TRADEERS_AND_BUILDERS ) )
		{
			FIELD_ACTOR( ActorType::eBuilder , Value::BuilderPlayerOwnNum );
			FIELD_ACTOR( ActorType::ePig     , Value::PigPlayerOwnNum );
			FIELD_VALUE( FieldType::eWine    , 0 );
			FIELD_VALUE( FieldType::eGain    , 0 );
			FIELD_VALUE( FieldType::eCloth   , 0 );
		}
		if ( setting.haveUse( EXP_ABBEY_AND_MAYOR ) )
		{
			FIELD_ACTOR( ActorType::eBarn , Value::BarnPlayerOwnNum );
			FIELD_ACTOR( ActorType::eWagon , Value::WagonPlayerOwnNum );
			FIELD_ACTOR( ActorType::eMayor , Value::MeeplePlayerOwnNum );
			FIELD_VALUE( FieldType::eAbbeyPices  , Value::AbbeyTilePlayerOwnNum );
		}
		if ( setting.haveUse( EXP_THE_TOWER ) )
		{
			FIELD_VALUE( FieldType::eTowerPices , Value::TowerPicesPlayerOwnNum[ numPlayer ] );
		}

#undef FIELD_ACTOR
#undef FIELD_VALUE
	}

	GameModule::GameModule()
	{
		mDebug = false;
		mIdxPlayerTrun = 0;
		mIdxTile = 0;
		mNumTileNeedMix = 0;
		mLevel.mTileSetManager = &mTileSetManager;
		mDragon = nullptr;
		mFairy = nullptr;
	}

	void GameModule::setupSetting(GameSetting& setting)
	{
		mSetting = &setting;
	}

	void GameModule::loadSetting()
	{
		if ( mSetting->haveUse( EXP_INNS_AND_CATHEDRALS) )
		{
			mMaxCityTileNum = 0;
			mMaxRoadTileNum = 0;
		}

		if ( mSetting->haveUse( EXP_THE_PRINCESS_AND_THE_DRAGON ) )
		{
			mDragon = createActor( ActorType::eDragon );
			mFairy  = createActor( ActorType::eFariy );
		}

		struct MyFieldProc
		{
			void exec( FieldType::Enum type , int value )
			{
				player->setFieldValue( type , value );
			}
			PlayerBase* player;
		} proc;
		for( int i = 0; i < mPlayerManager->getPlayerNum(); ++i )
		{
			PlayerBase* player = mPlayerManager->getPlayer(i);
			player->setupSetting( *mSetting );
			player->mScore = 0;
			proc.player = player;
			ProcField( *mSetting , mPlayerManager->getPlayerNum() , proc );
		}

		for( int idx = 0 ;  ; ++idx )
		{
			ExpansionTileContent const& tileData = gAllExpansionTileContents[idx];
			if ( tileData.exp == EXP_NULL )
				break;
			if ( tileData.exp == EXP_BASIC || 
				mSetting->haveUse( tileData.exp ) )
			{
				mTileSetManager.import( tileData );
			}
			else if ( mDebug && tileData.exp == EXP_TEST )
			{
				mTileSetManager.import( tileData );
			}
		}
	}

	void GameModule::restart( bool bInit )
	{
		if ( !bInit )
		{
			mLevel.restart();
			mTilesQueue.clear();
			mPlayerOrders.clear();
			cleanupActor();
			cleanupFeature();
			mTowerTiles.clear();
		}

		mIsStartGame = false;
		mIsRunning = false;
		loadSetting();
	}


	void GameModule::runLogic(IGameCoroutine& cort)
	{
		mIsStartGame = true;
		mIdxPlayerTrun = 0;
		generatePlayerPlayOrder();
		{
			TileId startId = generatePlayTile();
			if ( mDebug )
			{
				placeAllTileDebug();
			}
			else
			{
				MapTile* mapTile = mLevel.placeTileNoCheck( startId , Vec2i(0,0) , 0 );
				mListener->onPutTile( *mapTile );
				UpdateTileFeatureResult updateResult;
				updateTileFeature( *mapTile , updateResult );
			}
		}

		mIsRunning = true;
		mNumTrun = 0;

		while( mIsRunning )
		{
			if ( getRemainingTileNum() == 0 )
				break;

			++mNumTrun;
			PlayerBase* curTrunPlayer = mPlayerOrders[ mIdxPlayerTrun ];
			curTrunPlayer->startTurn();

			TurnResult result = procPlayerTurn( cort , curTrunPlayer );
			if ( result == eFinishGame )
				break;
			if ( result == eExitGame )
				return;

			if ( mIsRunning )
			{
				GameActionData data;
				cort.waitTurnOver( *this , data );
				if ( data.resultExitGame )
					return;
			}

			curTrunPlayer->endTurn();
			++mIdxPlayerTrun;
			if ( mIdxPlayerTrun == mPlayerOrders.size() )
				mIdxPlayerTrun = 0;
		}

		for( int i = 0 ; i < mFeatureMap.size() ; ++i )
		{
			FeatureBase* build = mFeatureMap[i];
			if ( build->group == -1 )
				continue;

			std::vector< FeatureScoreInfo > scoreInfos;
			if ( build->checkComplete() )
			{
				if ( build->type == FeatureType::eCity )
				{
					CityFeature* city = static_cast< CityFeature* >( build );
					switch( mSetting->getFarmScoreVersion() )
					{
					case 1:
						{
							scoreInfos.resize( MaxPlayerNum );
							for( int i = 0 ; i < MaxPlayerNum ; ++i )
							{
								scoreInfos[i].id       = i;
								scoreInfos[i].majority = 0;
							}
							for( std::set<FarmFeature*>::iterator iter = city->linkFarms.begin() , itEnd = city->linkFarms.end();
								iter != itEnd ; ++iter )
							{
								FarmFeature* farm = *iter;
								farm->addMajority( scoreInfos );
							}
							//int numPlayer = FeatureBase::evalMajorityControl( scoreInfos );
							//for( int i = 0 ; i < numPlayer ; ++i )
							//{
							//	scoreInfos[i].score = farm->calcPlayerScore( scoreInfos[i].id );
							//}
						}
						break;
					case 2:

						break;
					}
				}
			}
			else
			{
				int numPlayer = build->calcScore( scoreInfos );
				addFeaturePoints( *build , scoreInfos , numPlayer );
			}
		}

		if ( mSetting->haveUse( EXP_TRADEERS_AND_BUILDERS ) )
		{
			PlayerBase* players[ MaxPlayerNum ];
			FieldType::Enum treadeFields[] = { FieldType::eWine , FieldType::eGain , FieldType::eCloth };
			for( int i = 0 ; i < ARRAY_SIZE( treadeFields ) ; ++i )
			{
				int maxValue = 0;
				int numPlayer = getMaxFieldValuePlayer( treadeFields[i] , players , maxValue );
				for( int n = 0 ; n < numPlayer ; ++n )
				{
					addPlayerScore( players[n]->getId() , Value::TradeScore );
				}
			}
		}

		if ( mSetting->haveUse( EXP_KING_AND_ROBBER ) )
		{		
			int numCityComplete = 0;
			int numRoadComplete = 0;

			for( int i = 0 ; i < mFeatureMap.size() ; ++i )
			{
				FeatureBase* build = mFeatureMap[i];
				if ( build->group == -1 )
					continue;
				if ( build->checkComplete() == false )
					continue;

				if ( build->type == FeatureType::eCity )
					++numCityComplete;
				else if ( build->type == FeatureType::eRoad )
					++numRoadComplete;
			}

			if ( mMaxCityTileNum != 0 )
			{
				addPlayerScore( mIdKing , numCityComplete * Value::KingFactor );
			}
			if ( mMaxRoadTileNum != 0 )
			{
				addPlayerScore( mIdRobberBaron , numRoadComplete * Value::RobberBaronFactor );
			}
		}
	}

	TileId GameModule::generatePlayTile()
	{
		int numTile = mTileSetManager.getReigsterTileNum();
		std::vector< int > tilePieceMap( numTile );
		std::vector< TileId > specialTileList;
		for( int i = 0 ; i < numTile ; ++i )
		{
			TileSet const& tileSet = mTileSetManager.getTileSet( TileId(i) );
			if ( tileSet.tag != 0 )
				specialTileList.push_back( tileSet.tile->id );
				
			tilePieceMap[i] = tileSet.numPiece;
		}
	
		bool useRiver = mSetting->haveUse( EXP_THE_RIVER ) || mSetting->haveUse( EXP_THE_RIVER_II ) ;

		//River
		TileId tileIdStart = FAIL_TILE_ID;
		TileId tileIdRiverEnd = FAIL_TILE_ID;
		if ( useRiver )
		{
			if ( mSetting->haveUse( EXP_THE_RIVER_II ) )
			{
				int idx;
				idx = findTagTileIndex( specialTileList , EXP_THE_RIVER_II , TILE_START_TAG );
				if ( idx != -1 )
					tileIdStart = specialTileList[ idx ];
				idx = findTagTileIndex( specialTileList , EXP_THE_RIVER_II , TILE_END_TAG );
				if ( idx != -1 )
					tileIdRiverEnd = specialTileList[ idx ];
			}

			if ( mSetting->haveUse( EXP_THE_RIVER ) )
			{
				int idx;
				idx = findTagTileIndex( specialTileList , EXP_THE_RIVER , TILE_START_TAG );
				if ( idx != -1 )
				{
					if ( tileIdStart != FAIL_TILE_ID )
					{
						tilePieceMap[ specialTileList[idx] ] = 0;
					}
					else
					{
						tileIdStart = specialTileList[idx];
					}
				}

				idx = findTagTileIndex( specialTileList , EXP_THE_RIVER , TILE_END_TAG );
				if ( idx != -1 )
				{
					if ( tileIdRiverEnd != FAIL_TILE_ID )
					{
						tilePieceMap[ specialTileList[idx] ] = 0;
					}
					else
					{
						tileIdRiverEnd = specialTileList[idx];
					}
				}
			}

			if ( tileIdStart != FAIL_TILE_ID )
				tilePieceMap[ tileIdStart ] -= 1;
			if ( tileIdRiverEnd != FAIL_TILE_ID )
				tilePieceMap[ tileIdRiverEnd ] -= 1;

			if ( tileIdStart == FAIL_TILE_ID || tileIdRiverEnd == FAIL_TILE_ID )
			{
				CAR_LOG( "Exp River No Start or End Tile!");
			}
		}

		std::vector< int > shuffleGroup;
		if ( useRiver )
		{
			TileIdVec const& idSetGroup = mTileSetManager.getSetGroup( TileSet::eRiver );
			for( TileIdVec::const_iterator iter = idSetGroup.begin() , itEnd = idSetGroup.end() ; 
				iter != itEnd ; ++iter )
			{
				TileId id = *iter;
				if ( tilePieceMap[id] != 0)
					mTilesQueue.insert( mTilesQueue.end() , tilePieceMap[id] , id );
			}
			shuffleGroup.push_back( mTilesQueue.size() );

			if ( tileIdRiverEnd != FAIL_TILE_ID )
			{
				mTilesQueue.push_back( tileIdRiverEnd );
				shuffleGroup.push_back( mTilesQueue.size() );
			}
		}
		//Common
		if ( tileIdStart == FAIL_TILE_ID )
		{
			int idx = findTagTileIndex( specialTileList , EXP_BASIC , TILE_START_TAG );
			if ( idx == -1 )
			{
				tileIdStart = 0;
				CAR_LOG( "Warning: Cant Find Start Tile In Common Set");
			}
			else
			{
				tileIdStart = specialTileList[idx];
			}
			tilePieceMap[ tileIdStart ] -= 1;
		}

		{
			TileIdVec const& idSetGroup = mTileSetManager.getSetGroup( TileSet::eCommon );
			for( TileIdVec::const_iterator iter = idSetGroup.begin() , itEnd = idSetGroup.end() ; 
				iter != itEnd ; ++iter )
			{
				TileId id = *iter;
				if ( tilePieceMap[id] != 0)
					mTilesQueue.insert( mTilesQueue.end() , tilePieceMap[id] , id );
			}
			shuffleGroup.push_back( mTilesQueue.size() );
		}

		int idxPrev = 0;
		for( int i = 0 ; i < shuffleGroup.size() ; ++i )
		{
			int idx = shuffleGroup[i];
			if ( idx != idxPrev )
				shuffleTiles( &mTilesQueue[0] + idxPrev , &mTilesQueue[0] + idx );
			idxPrev = idx;
		}

		mIdxTile = 0;
		mNumTileNeedMix = 0;

		return tileIdStart;
	}

	void GameModule::generatePlayerPlayOrder()
	{
		int numPlayer = mPlayerManager->getPlayerNum();
		for( int i = 0 ; i < numPlayer ; ++i )
		{
			mPlayerOrders.push_back( mPlayerManager->getPlayer(i) );
		}

		for( int i = 0 ; i < numPlayer ; ++i )
		{
			for( int n = 0 ; n < numPlayer ; ++n )
			{
				int idx = mRandom.getInt() % numPlayer;
				std::swap( mPlayerOrders[i] , mPlayerOrders[idx] );
			}
		}

		for( int i = 0 ; i < numPlayer ; ++i )
		{
			mPlayerOrders[i]->mPlayOrder = i;
		}
	}

	TurnResult GameModule::procPlayerTurn(IGameCoroutine& cort , PlayerBase* curTrunPlayer)
	{
		TurnResult result;

		//Step 1: Begin Turn
		if ( mSetting->haveUse( EXP_THE_PRINCESS_AND_THE_DRAGON ))
		{
			if ( mFairy->binder && mFairy->binder->owner == curTrunPlayer )
			{
				CAR_LOG( "%d Score:FairyBeginningOfATurnScore" , curTrunPlayer->getId() );
				addPlayerScore( curTrunPlayer->getId() , Value::FairyBeginningOfATurnScore );
			}
		}

		int numPlaceTile = 0;
		for(;;)// step 2 to 9
		{
			//Step 2: Draw a Tile

			int countDrawTileLoop = 0;
			for(;;)
			{
				++countDrawTileLoop;
			
				mUseTileId = drawPlayTile();
				if ( mSetting->haveUse( EXP_THE_PRINCESS_AND_THE_DRAGON ) )
				{
					if ( countDrawTileLoop < getRemainingTileNum() )
					{
						TileSet tileSet = mTileSetManager.getTileSet( mUseTileId );
						if ( tileSet.tile->contentFlag & TileContent::eTheDragon &&
							mDragon->mapTile == nullptr )
						{
							reservePlayTile();
							continue;
						}
					}
				}
				if ( updatePosibleLinkPos() == 0 )
				{
					if ( getRemainingTileNum() == 0 )
						return eFinishGame;
				}
				break;
			}

			//Step 3: Place the Tile
			MapTile* placeMapTile = nullptr;
			for(;;)
			{
				//a) Place the tile.
				GamePlaceTileData putTileData;
				putTileData.id = mUseTileId;
				cort.waitPlaceTile( *this , putTileData );
				if ( checkGameState( putTileData , result ) == false )
					return result;

				if ( putTileData.resultRotation < 0 || putTileData.resultRotation >= FDir::TotalNum )
				{
					CAR_LOG("Warning: Place Tile have error rotation %d" , putTileData.resultRotation );
					putTileData.resultRotation = 0;
				}
				placeMapTile = mLevel.placeTile( mUseTileId , putTileData.resultPos , putTileData.resultRotation );
				if ( placeMapTile != nullptr )
				{
					CAR_LOG("Player %d Place Tile %d rotation=%d" , curTrunPlayer->getId() , mUseTileId , putTileData.resultRotation );
					mListener->onPutTile( *placeMapTile );
					++numPlaceTile;
					break;
				}
			}

			//b)Note: any feature that is finished is considered complete at this time.
			UpdateTileFeatureResult updateResult;
			updateTileFeature( *placeMapTile , updateResult );

			if ( mSetting->haveUse( EXP_THE_TOWER ) )
			{
				if ( placeMapTile->getTileContent() & TileContent::eTowerFoundation )
				{
					mTowerTiles.push_back( placeMapTile );
				}
			}

			bool haveBuilderFeatureExpend = false;
			if ( mSetting->haveUse( EXP_TRADEERS_AND_BUILDERS ) )
			{
				for( FeatureUpdateInfoVec::iterator iter = mUpdateFeatures.begin(), itEnd = mUpdateFeatures.end();
					iter != itEnd ; ++iter )
				{
					FeatureBase* feature = iter->feature;
					if ( feature->type != FeatureType::eRoad && feature->type != FeatureType::eCity )
						continue;

					if ( feature->havePlayerActor( getTurnPlayer()->getId() , ActorType::eBuilder ) )
					{
						if ( mSetting->haveUse( EXP_ABBEY_AND_MAYOR ) )
						{
							if ( iter->bAbbeyUpdate )
								continue;
						}
						haveBuilderFeatureExpend = true;
						break;
					}
				}
			}

			bool skipPlaceActor = false;
			bool skipAllActorStep = false;
			
			
			if ( mSetting->haveUse( EXP_THE_PRINCESS_AND_THE_DRAGON ) )
			{
				//c)volcano
				if ( placeMapTile->getTileContent() & TileContent::eVolcano )
				{
					if ( mDragon->mapTile == nullptr )
						checkReservedTileToMix();

					moveActor( mDragon , ActorPos( ActorPos::eTile , 0 ) , placeMapTile );
					skipPlaceActor = true;
				}

				//d)princess
				bool haveDone = false;
				result = procPrincess( cort , placeMapTile , haveDone );
				if ( result != TurnResult::eOK )
					return result;
				if ( haveDone )
					skipAllActorStep = true;
			}
			
			if ( skipAllActorStep == false )
			{
				//Step 4A: Move the Wood (Phase 1)
				if ( skipPlaceActor == false )
				{
					MapTile* deployMapTile = placeMapTile;
					bool bUsagePortal = false;
					if ( placeMapTile->getTileContent() & TileContent::eMagicPortal )
					{
						//TODO
						int playerId = curTrunPlayer->getId();

						std::set< MapTile* > mapTileSet;
						for( int i = 0 ; i < mFeatureMap.size() ; ++i )
						{
							FeatureBase* feature = mFeatureMap[i];
							if ( feature->group == -1 )
								continue;
							if ( feature->checkComplete() )
								continue;
							if ( feature->havePlayerActorMask( AllPlayerMask , mSetting->getFollowerMask() ) )
								continue;

							mapTileSet.insert( feature->mapTiles.begin() , feature->mapTiles.end() );
						}

						if ( mapTileSet.empty() == false )
						{
							std::vector< MapTile* > mapTiles( mapTileSet.begin() , mapTileSet.end() );

							GameSelectMapTileData data;
							data.reason = SAR_MAGIC_PORTAL;
							data.mapTiles = &mapTiles[0];
							data.numSelection = mapTiles.size();
							cort.waitSelectMapTile( *this , data );
							if ( checkGameState( data , result ) == false )
								return result;

							deployMapTile = data.mapTiles[ data.resultIndex ];
							bUsagePortal = true;
						}
						else
						{
							deployMapTile = nullptr;
						}
					}

					if ( deployMapTile )
					{
						//a-b) follower and other figures
						calcPlayerDeployActorPos(*curTrunPlayer, *deployMapTile , bUsagePortal );

						GameDeployActorData deployActorData;
						deployActorData.mapTile = deployMapTile;
						cort.waitDeployActor( *this , deployActorData );
						if ( checkGameState( deployActorData , result ) == false )
							return result;

						if ( deployActorData.resultSkipAction == false  )
						{
							if ( deployActorData.resultIndex < 0 && deployActorData.resultIndex >= mActorDeployPosList.size() )
							{
								CAR_LOG("Warning: Error Deploy Actor Index !!" );
								deployActorData.resultIndex = 0;
							}
							ActorPosInfo& info = mActorDeployPosList[ deployActorData.resultIndex ];

							FeatureBase* feature = getFeature( info.group );
							assert( feature );
							curTrunPlayer->modifyActorValue( deployActorData.resultType , -1 );
							LevelActor* actor = createActor( deployActorData.resultType );

							assert( actor );
							deployMapTile->addActor( *actor );
							feature->addActor( *actor );
							actor->owner   = curTrunPlayer;
							actor->pos     = info.pos;

							mListener->onDeployActor( *actor );
							CAR_LOG( "Player %d deploy Actor : type = %d  " , curTrunPlayer->getId() , actor->type );
						}
					}
				}
				else
				{
					//e)
					if ( mSetting->haveUse( EXP_THE_TOWER ) )
					{
						result = procTower( cort , curTrunPlayer );
						if ( result != TurnResult::eOK )
							return result;
					}
					if ( mSetting->haveUse( EXP_THE_PRINCESS_AND_THE_DRAGON ) )
					{
						ActorList followers;
						int numFollower = getFollowers( BIT( curTrunPlayer->getId() ) , followers , mFairy->binder );
						if ( numFollower != 0 )
						{
							GameSelectActorData selectFollowerData;
							selectFollowerData.reason = SAR_FAIRY_MOVE_NEXT_TO_FOLLOWER;
							selectFollowerData.actors = &followers[0];
							selectFollowerData.numSelection = numFollower;
							cort.waitSelectActor( *this , selectFollowerData );
							if ( !checkGameState( selectFollowerData , result ) )
								return result;
							if ( selectFollowerData.resultSkipAction == false ) 
							{
								if ( selectFollowerData.checkResultVaild() == false )
								{
									CAR_LOG( "Warning: Error Fairy Select Actor Index !" );
									selectFollowerData.resultIndex = 0;
								}
								LevelActor* actor = selectFollowerData.actors[ selectFollowerData.resultIndex ];
								actor->addFollower( *mFairy );
								moveActor( mFairy , ActorPos( ActorPos::eTile , 0 ) , actor->mapTile );
							}
						}
					}
				}

				//Step 4B: Move the Wood (Phase 2)
			}

			//Step 5: Resolve Move the Wood

			//dragon move
			if ( mSetting->haveUse( EXP_THE_PRINCESS_AND_THE_DRAGON ) )
			{
				if ( mSetting->haveRule(eMoveDragonBeforeScoring ) == true )
				{
					if ( mDragon->mapTile != nullptr && ( placeMapTile->getTileContent() & TileContent::eTheDragon ) )
					{
						result = procDragonMove( cort , *mDragon );
						if ( result != TurnResult::eOK )
							return result;
					}
				}
			}

			//Step 6: Identify Completed Features
			for( FeatureUpdateInfoVec::iterator iter = mUpdateFeatures.begin(), itEnd = mUpdateFeatures.end();
				iter != itEnd ; ++iter )
			{
				FeatureBase* feature = iter->feature;
				if ( feature->group == -1 )
					continue;

				//a) Identify all completed features
				if ( feature->checkComplete() )
				{
					CAR_LOG( "Feature %d complete by Player %d" , feature->group , curTrunPlayer->getId()  );
					//Step 7: Resolve Completed Features
					result = resolveCompleteFeature( cort , *feature );
					if ( result != TurnResult::eOK )
						return result;
				}
			}

			//Step 8: Resolve the Tile
			//d) dragon move
			if ( mSetting->haveUse( EXP_THE_PRINCESS_AND_THE_DRAGON ) )
			{
				if ( mSetting->haveRule(eMoveDragonBeforeScoring ) == false )
				{
					if ( mDragon->mapTile != nullptr && ( placeMapTile->getTileContent() & TileContent::eTheDragon ) )
					{
						result = procDragonMove( cort , *mDragon );
						if ( result != TurnResult::eOK )
							return result;
					}
				}
			}

			//Step 9: Resolve the Turn
			//a)builder
			if ( mSetting->haveUse( EXP_TRADEERS_AND_BUILDERS ) && numPlaceTile == 1 )
			{
				if ( haveBuilderFeatureExpend )
				{
					CAR_LOG( "Player %d have double Turn" , getTurnPlayer()->getId() );
					continue;
				}
			}

			break;
		}

		return TurnResult::eOK;
	}

	TurnResult GameModule::procDragonMove(IGameCoroutine& cort , LevelActor& dragon)
	{
		assert( dragon.mapTile != nullptr );

		TurnResult result;
		MapTile* moveTilesBefore[ Value::DragonMoveStepNum ];

		MapTile* moveTiles[FDir::TotalNum];
		GameDragonMoveData data;
		data.mapTiles = moveTiles;
		data.reason = SAR_MOVE_DRAGON;

		int idxPlayer = mIdxPlayerTrun;

		for( int step = 0 ; step < Value::DragonMoveStepNum ; ++step )
		{
			int numTile = 0;
			
			for( int i = 0 ; i < FDir::TotalNum ; ++i )
			{
				Vec2i pos = FDir::LinkPos( dragon.mapTile->pos , i );
				MapTile* mapTile = mLevel.findMapTile( pos );
				if ( mapTile == nullptr )
					continue;

				//TODO: castle school city of car. , wheel of fea.
				assert( mSetting->haveUse( EXP_THE_PRINCESS_AND_THE_DRAGON ) );
				if ( mFairy->mapTile == mapTile )
					continue;


				int idx = 0;
				for ( ; idx < step ; ++idx )
				{
					if ( mapTile == moveTilesBefore[idx] )
						break;
				}
				if ( idx != step )
					continue;


				data.mapTiles[ numTile ] = mapTile;
				++numTile;
			}

			if ( numTile == 0 )
				break;

			data.numSelection = numTile;
			data.playerId = mPlayerOrders[ idxPlayer ]->getId();
			
			cort.waitSelectMapTile(*this,data);
			if ( checkGameState( data , result ) == false )
				return result;

			if ( data.checkResultVaild() == false )
			{
				CAR_LOG("Warning: Error Dragon Move Index %d" , data.resultIndex );
				data.resultIndex = 0;
			}

			moveTilesBefore[ step ] = dragon.mapTile;
			moveActor( &dragon , ActorPos( ActorPos::eTile , 0 ) , data.mapTiles[ data.resultIndex ] );
			

			for( int i = 0 ; i < dragon.mapTile->mActors.size() ; ++i )
			{
				LevelActor* actor = dragon.mapTile->mActors[i];
				if ( mSetting->isFollower( actor->type ) == false )
					continue;

				//TODO
				if ( actor->binder )
				{

				}
				returnActorToPlayer( actor );
			}

			++idxPlayer;
			if ( idxPlayer >= mPlayerManager->getPlayerNum() )
				idxPlayer = 0;
		}
		return TurnResult::eOK;
	}

	TurnResult GameModule::procTower(IGameCoroutine& cort , PlayerBase* curTurnPlayer)
	{
		TurnResult result;
		if ( curTurnPlayer->getFieldValue( FieldType::eTowerPices ) == 0 || mTowerTiles.empty() )
			return TurnResult::eOK;

		GameSelectMapTileData data;
		data.reason = SAR_CONSTRUCT_TOWER;
		data.numSelection = mTowerTiles.size();
		data.mapTiles   = &mTowerTiles[0];
		cort.waitSelectMapTile( *this , data );
		if ( checkGameState( data , result ) == false )
			return result;

		if ( data.resultSkipAction == true )
			return TurnResult::eOK;

		if ( data.checkResultVaild() == false )
		{
			CAR_LOG("Warning: Error ContructTower Index" );
			data.resultIndex = 0;
		}

		MapTile* mapTile = mTowerTiles[ data.resultIndex ];
		mapTile->towerHeight += 1;
		curTurnPlayer->modifyFieldValue( FieldType::eTowerPices , -1 );

		mListener->onConstructTower( *mapTile );

		std::vector< LevelActor* > actors;
		for( int i = 1 ; i <= mapTile->towerHeight ; ++i )
		{
			for( int dir = 0 ; dir < FDir::TotalNum ; ++dir )
			{
				Vec2i posCheck = mapTile->pos + i * FDir::LinkOffset( dir );
				MapTile* tileCheck = mLevel.findMapTile( posCheck );
				if ( tileCheck == nullptr )
					continue;

				for( int n = 0 ; n < tileCheck->mActors.size() ; ++n )
				{
					LevelActor* actor = tileCheck->mActors[n];

					if ( mSetting->isFollower( actor->type ) == false )
						continue;

					if ( actor->owner == curTurnPlayer )
						continue;

					actors.push_back( actor );
				}
			}
		}

		if ( actors.empty() == false )
		{
			GameSelectActorData selectActorData;
			selectActorData.reason = SAR_TOWER_CAPTURE_FOLLOWER;
			selectActorData.numSelection = actors.size();
			selectActorData.actors = &actors[0];
			cort.waitSelectActor( *this , selectActorData );
			if ( checkGameState( selectActorData , result ) == false )
				return result;

			if ( selectActorData.resultSkipAction == true )
				return TurnResult::eOK;

			if ( selectActorData.checkResultVaild() == false )
			{
				CAR_LOG("Warning: Error Tower Capture follower Index" );
				selectActorData.resultIndex = 0;
			}

			LevelActor* actor = actors[selectActorData.resultIndex];
			PrisonerInfo info;
			info.playerId  = actor->owner->getId();
			info.type = actor->type;
			info.ownerId = curTurnPlayer->getId();

			std::vector< ActorInfo > actorInfos;
			for( int i = 0 ; i < mPrisoners.size() ; ++i )
			{
				if ( mPrisoners[i].playerId == info.ownerId && 
					 mPrisoners[i].ownerId == info.playerId )
				{
					actorInfos.push_back( mPrisoners[i] );
				}
			}

			destroyActor( actor );

			if ( actorInfos.empty() )
			{
				mPrisoners.push_back( info );
			}
			else
			{
				int idxExchange = 0;
				if ( actorInfos.size() > 1 )
				{
					GameSelectActorInfoData data;
					data.reason = SAR_EXCHANGE_PRISONERS;
					data.actorInfos = &actorInfos[0];
					data.numSelection = actorInfos.size();
					cort.waitSelectActorInfo( *this , data );
					if ( data.checkResultVaild() == false )
					{
						CAR_LOG("Warning: Error Prisoner exhange Index" );
						data.resultIndex = 0;
					}

					for( int i = 0 ; i < mPrisoners.size() ; ++i )
					{
						if ( mPrisoners[i].playerId == data.actorInfos[data.resultIndex].playerId && 
							 mPrisoners[i].type == data.actorInfos[data.resultIndex].type )
						{
							idxExchange = i;
							break;
						}
					}
				}
				PrisonerInfo& infoExchange = mPrisoners[idxExchange];
				PlayerBase* player = mPlayerManager->getPlayer( info.playerId );
				player->modifyActorValue( info.type , 1 );
				curTurnPlayer->modifyActorValue( infoExchange.type , 1 );
				mPrisoners.erase( mPrisoners.begin() + idxExchange );
				CAR_LOG("Exchange Prisoners [%d %d] <-> [%d %d]" , info.playerId , info.type , infoExchange.playerId , infoExchange.type );

			}
		}

		return TurnResult::eOK;
	}

	CAR::TurnResult GameModule::procPrincess(IGameCoroutine& cort , MapTile* placeMapTile , bool& haveDone)
	{
		TurnResult result;
		unsigned sideMask = Tile::AllSideMask;
		int dir;
		while( FBit::MaskIterator4( sideMask , dir ) )
		{
			sideMask &= ~placeMapTile->getSideLinkMask( dir );

			if ( ( placeMapTile->getSideContnet( dir ) & SideContent::ePrincess ) == 0 )
				continue;

			if (  placeMapTile->sideNodes[dir].group == -1 )
			{
				CAR_LOG( "Warning: Tile id = %d Format Error for Princess" , placeMapTile->getId() );
				continue;
			}

			CityFeature* city = static_cast< CityFeature* >( getFeature( placeMapTile->sideNodes[dir].group ) );

			assert( city->type == FeatureType::eCity );
			unsigned followerMask = mSetting->getFollowerMask();

			int iter = 0;
			LevelActor* actor;
			ActorList kinghts;
			while( actor = city->iteratorActorMask( AllPlayerMask , followerMask , iter ) )
			{
				kinghts.push_back( actor );
			}

			if ( kinghts.empty() == false )
			{
				GameSelectActorData data;
				data.reason   = SAR_PRINCESS_REMOVE_KINGHT;
				data.numSelection = kinghts.size();
				data.actors   = &kinghts[0];

				cort.waitSelectActor( *this , data );
				if ( checkGameState( data , result ) == false )
					return result;

				if ( data.resultSkipAction == false )
				{
					if ( data.checkResultVaild() == false )
					{
						CAR_LOG("Warning: Error Select Kinght Index");
						data.resultIndex = 0;
					}
					LevelActor* actor = kinghts[ data.resultIndex ];
					returnActorToPlayer( actor );
					haveDone = true;
					break;
				}
			}
		}
		return TurnResult::eOK;
	}

	void GameModule::updateTileFeature( MapTile& mapTile , UpdateTileFeatureResult& updateResult )
	{
		mUpdateFeatures.clear();
		//update Build
		{
			unsigned maskAll = Tile::AllSideMask;
			int dir;
			while( FBit::MaskIterator4( maskAll , dir ) )
			{
				unsigned linkMask = mapTile.getSideLinkMask( dir );
				maskAll &= ~linkMask;
				SideType linkType = mapTile.getLinkType( dir );

				FeatureBase* build = nullptr;
				bool bAbbeyUpdate = false;
				switch( linkType )
				{
				case SideType::eCity:
				case SideType::eRoad:
					build = updateBasicSideFeature( mapTile , linkMask , linkType );
					break;
				case SideType::eAbbey:
					{
						int sideGroup = mapTile.sideNodes[dir].outConnect->group;
						if( sideGroup != -1 )
						{
							SideFeature* sideFeature = static_cast< SideFeature* >( getFeature( sideGroup ) );
							sideFeature->addAbbeyNode( mapTile , dir );
							bAbbeyUpdate = true;
							build = sideFeature;
						}
					}

				}
				if ( build )
				{
					addUpdateFeature( build , bAbbeyUpdate );
				}
			}
		}

		{
			unsigned maskAll = Tile::AllFarmMask;
			int idx;
			while( FBit::MaskIterator8( maskAll , idx ) )
			{
				unsigned linkMask = mapTile.getFarmLinkMask( idx );
				if ( linkMask == 0 )
					continue;

				maskAll &= ~linkMask;

				FarmFeature* farm = updateFarm( mapTile , linkMask );
				unsigned cityLinkMask = mapTile.getCityLinkFarmMask( idx );
				unsigned cityLinkMaskCopy = cityLinkMask;
				int dir;
				while ( FBit::MaskIterator4( cityLinkMask , dir ) )
				{
					if ( mapTile.getSideGroup( dir ) == -1 )
					{
						TileSet const& tileSet = mTileSetManager.getTileSet( mapTile.getId() );
						CAR_LOG("Error:Farm Side Mask Error Tile exp=%d index =%d" , (int)tileSet.expansions , tileSet.idxDefine );
						continue;
					}
					CityFeature* city = static_cast< CityFeature* >( getFeature( mapTile.getSideGroup( dir ) ) );
					assert( city->type == FeatureType::eCity );
					city->linkFarms.insert( farm );
					farm->linkCities.insert( city );
				}

				addUpdateFeature( farm );
			}
		}


		if ( mapTile.getTileContent() & TileContent::eCloister )
		{
			CloisterFeature* cloister = createFeatureT< CloisterFeature >();
			cloister->addMapTile( mapTile );
			mapTile.group = cloister->group;

			bool haveUpdate = false;
			for( int i = -1 ; i <= 1 ; ++i )
			{
				for( int j = -1 ; j <= 1 ; ++j )
				{
					if ( i == 0 && j == 0 )
						continue;

					Vec2i posCheck = mapTile.pos + Vec2i(i,j);
					MapTile* dataCheck = mLevel.findMapTile( posCheck );
					if ( dataCheck )
					{
						haveUpdate |= cloister->updateForNeighborTile( *dataCheck );
					}
				}
			}
			if ( haveUpdate )
				mUpdateFeatures.push_back( FeatureUpdateInfo( cloister ) );
		}


		for( int i = -1 ; i <= 1 ; ++i )
		{
			for( int j = -1 ; j <= 1 ; ++j )
			{
				if ( i == 0 && j == 0 )
					continue;

				Vec2i posCheck = mapTile.pos + Vec2i(i,j);
				MapTile* dataCheck = mLevel.findMapTile( posCheck );

				if ( dataCheck && dataCheck->group != -1 )
				{
					FeatureBase* feature = getFeature( dataCheck->group );
					if ( feature->updateForNeighborTile( mapTile ) )
					{
						mUpdateFeatures.push_back( FeatureUpdateInfo(feature) );
					}
				}
			}
		}
	}

	TurnResult GameModule::resolveCompleteFeature( IGameCoroutine& cort , FeatureBase& feature)
	{
		TurnResult result;
		assert( feature.checkComplete() );

		//c)
		if ( mSetting->haveUse(EXP_THE_PRINCESS_AND_THE_DRAGON) )
		{
			LevelActor* actor = mFairy->binder;
			if ( actor && actor->feature == &feature )
			{
				CAR_LOG("Score: %d FairyFearureScoringScore" , actor->owner->getId() );
				addPlayerScore( actor->owner->getId() , Value::FairyFearureScoringScore );
			}
		}
		//d)
		//->Collect any trade good tokens
		if ( feature.type == FeatureType::eCity )
		{
			CityFeature& city = static_cast< CityFeature& >( feature );
			if ( mSetting->haveUse( EXP_TRADEERS_AND_BUILDERS ) )
			{
				for( std::vector< SideNode* >::iterator iter = city.nodes.begin() , itEnd = city.nodes.end();
					iter != itEnd ; ++iter )
				{
					SideNode* node = *iter;
					MapTile const* mapTile = node->getMapTile();
					uint32 content = mapTile->getSideContnet( node->index );
					if ( content & SideContent::eClothHouse )
					{
						getTurnPlayer()->modifyFieldValue( FieldType::eCloth , 1 );
					}
					if ( content & SideContent::eGrainHouse )
					{
						getTurnPlayer()->modifyFieldValue( FieldType::eGain , 1 );
					}
					if ( content & SideContent::eWineHouse )
					{
						getTurnPlayer()->modifyFieldValue( FieldType::eWine, 1 );
					}
				}
			}
		}
		//->King or Robber Baron
		if ( mSetting->haveUse( EXP_KING_AND_ROBBER ) )
		{
			if ( feature.type == FeatureType::eCity )
			{
				if ( feature.mapTiles.size() > mMaxCityTileNum )
				{
					mMaxCityTileNum = feature.mapTiles.size();
					mIdKing = getTurnPlayer()->getId();
				}
			}
			else if ( feature.type == FeatureType::eRoad )
			{

				if ( feature.mapTiles.size() > mMaxRoadTileNum )
				{
					mMaxCityTileNum = feature.mapTiles.size();
					mIdRobberBaron = getTurnPlayer()->getId();
				}
			}
		}

		//f - h)
		std::vector< FeatureScoreInfo > scoreInfo;
		int numController = feature.calcScore( scoreInfo );
		if ( numController > 0 )
		{
			addFeaturePoints( feature , scoreInfo , numController );
		}

		LevelActor* actor;
		std::vector< LevelActor* > wagonGroup;
		std::vector< LevelActor* > mageWitchGroup;
		std::vector< LevelActor* > hereticMonkGroup;
		std::vector< LevelActor* > otherGroup;
		while( actor = feature.popActor() )
		{
			PlayerBase* player = actor->owner;

			if ( actor->owner )
			{
				returnActorToPlayer( actor );
			}
			else
			{
				switch( actor->type )
				{
				case ActorType::eWagon: 
					wagonGroup.push_back( actor );
					break;
				default:
					otherGroup.push_back( actor );
				}
			}
		}
		//l)

		if ( mSetting->haveUse( EXP_ABBEY_AND_MAYOR ) && wagonGroup.empty() != false )
		{
			std::set< unsigned > linkFeatureGroups;
			feature.generateRoadLinkFeatures( mLevel , linkFeatureGroups );

			std::vector< FeatureBase* > linkFeatures;
			for( std::set<unsigned>::iterator iter = linkFeatureGroups.begin() , itEnd = linkFeatureGroups.end();
				 iter != itEnd ; ++iter )
			{
				FeatureBase* featureLink = getFeature( *iter );
				if ( featureLink->checkComplete() )
					continue;
				if ( featureLink->havePlayerActorMask( AllPlayerMask , mSetting->getFollowerMask() ) )
					continue;
				linkFeatures.push_back( featureLink );
			}

			if ( linkFeatures.empty() == false )
			{
				struct WagonCompFun
				{
					bool operator()( LevelActor* lhs , LevelActor* rhs ) const
					{
						return calcOrder( lhs ) < calcOrder( rhs );
					}
					int calcOrder( LevelActor* actor ) const
					{
						int result = actor->owner->mPlayOrder - curPlayerOrder;
						if ( result < 0 )
							result += numPlayer;
						return result;
					}
					int curPlayerOrder;
					int numPlayer;
				};
				WagonCompFun wagonFun;
				wagonFun.curPlayerOrder = getTurnPlayer()->mPlayOrder;
				wagonFun.numPlayer = mPlayerManager->getPlayerNum();
				std::sort( wagonGroup.begin() , wagonGroup.end() , wagonFun );
				
				std::vector< MapTile* > mapTiles;
				GameFeatureTileSelectData data;
				data.reason = SAR_WAGON_MOVE_TO_FEATURE;
				for( int i = 0 ; i < wagonGroup.size() ; ++i )
				{
					data.infos.clear();
					mapTiles.clear();
					FillActionData( data , linkFeatures, mapTiles );

					cort.waitSelectMapTile( *this , data );
					if ( checkGameState( data , result ) == false )
						return result;

					if ( data.resultSkipAction == true )
					{
						returnActorToPlayer( wagonGroup[i] );
					}
					else 
					{
						if ( data.checkResultVaild() == false )
						{
							CAR_LOG("Warning: Error Wagon Move Tile Map Index");
							data.resultIndex = 0;
						}

						FeatureBase* feature = data.getResultFeature();
						ActorPos actorPos;
						bool isOK = feature->getActorPos( *mapTiles[ data.resultIndex ] , actorPos );
						assert( isOK );

						moveActor( wagonGroup[i] , actorPos , mapTiles[data.resultIndex] );
						linkFeatures.erase( std::find( linkFeatures.begin() , linkFeatures.end() , feature ) );
					}
				}
			}
			else
			{
				for( int i = 0 ; i < wagonGroup.size() ; ++i )
				{
					returnActorToPlayer( wagonGroup[i] );
				}
			}
		}

		//m)
		for( int i = 0 ; i < otherGroup.size() ; ++i )
		{
			returnActorToPlayer( otherGroup[i] );
		}
		return TurnResult::eOK;
	}

	void GameModule::addFeaturePoints( FeatureBase& feature , std::vector< FeatureScoreInfo >& featureControls , int numPlayer )
	{
		for( int i = 0 ; i < numPlayer ; ++i )
		{
			PlayerBase* player = mPlayerManager->getPlayer( featureControls[i].id );
			int score = feature.calcPlayerScore( player->getId() );
			featureControls[i].score = score;
			addPlayerScore( player->getId() , score );
			CAR_LOG( "Player %d add Score %d " , player->getId() , score );
		}
	}

	int GameModule::updatePosibleLinkPos()
	{
		mPlaceTilePosList.clear();
		return mLevel.getPosibleLinkPos( mUseTileId , mPlaceTilePosList );
	}

	int GameModule::findTagTileIndex( std::vector< TileId >& tiles , Expansion exp , TileTag tag )
	{
		for( int i = 0 ; i < tiles.size() ; ++i )
		{
			TileSet const& tileSet = mTileSetManager.getTileSet( tiles[i] );
			if ( tileSet.expansions == exp && tileSet.tag == tag )
				return i;
		}
		return -1;
	}

	bool GameModule::checkGameState(GameActionData& actionData , TurnResult& result)
	{
		if ( actionData.resultExitGame )
		{
			result = TurnResult::eExitGame;
			return false;
		}
		if ( mIsRunning == false )
		{
			result = TurnResult::eFinishGame;
			return false;
		}
		return true;
	}

	void GameModule::shuffleTiles(TileId* begin , TileId* end)
	{
		int num = end - begin;
		for( TileId* ptr = begin ; ptr != end ; ++ptr )
		{
			for ( int i = 0 ; i < num; ++i)
			{
				int idx = mRandom.getInt() % num;
				std::swap( *ptr , *(begin + idx) );
			}
		}
	}

	int GameModule::getRemainingTileNum()
	{
		return mTilesQueue.size() - mIdxTile;
	}

	TileId GameModule::drawPlayTile()
	{
		TileId result = mTilesQueue[ mIdxTile ];
		++mIdxTile;
		return result;
	}

	void GameModule::calcPlayerDeployActorPos(PlayerBase& player , MapTile& mapTile , bool bUsageMagicPortal )
	{
		mActorDeployPosList.clear();
		getActorPutInfo( player.getId() , mapTile , bUsageMagicPortal , mActorDeployPosList );

		unsigned actorMask = player.getUsageActorMask();
		if ( bUsageMagicPortal )
			actorMask &= mSetting->getFollowerMask();

		int num = mActorDeployPosList.size();
		for( int i = 0 ; i < num ; )
		{
			mActorDeployPosList[i].actorTypeMask &= actorMask;
			if ( mActorDeployPosList[i].actorTypeMask == 0 )
			{
				--num;
				if ( i != num )
				{
					std::swap( mActorDeployPosList[i] , mActorDeployPosList[num]);
				} 

			}
			else
			{
				++i;
			}
		}
		mActorDeployPosList.resize( num );
	}

	FeatureBase* GameModule::updateBasicSideFeature( MapTile& putData , unsigned dirMask , SideType linkType )
	{
		int numLinkGroup = 0;
		int linkGroup[ Tile::NumSide ];
		SideNode* linkNodeFrist = nullptr;
		unsigned mask = dirMask;
		int dir = 0;
		while ( FBit::MaskIterator4( mask , dir ) )
		{
			SideNode* linkNode = putData.sideNodes[ dir ].outConnect;
			if ( linkNode == nullptr )
				continue;

			int idxFind = 0;
			for( ; idxFind < numLinkGroup ; ++idxFind )
			{
				if ( linkNode->group == linkGroup[idxFind] )
					break;
			}

			if ( idxFind == numLinkGroup )
			{
				linkGroup[numLinkGroup] = linkNode->group;
				if ( numLinkGroup == 0 )
				{
					linkNodeFrist = linkNode;
				}
				++numLinkGroup;
			}
		}

		SideFeature* sideFeature;
		if ( numLinkGroup == 0 )
		{
			switch( linkType )
			{
			case SideType::eCity:
				sideFeature = createFeatureT< CityFeature >();
				break;
			case SideType::eRoad:
				sideFeature = createFeatureT< RoadFeature >();
				break;
			default:
				assert(0);
				return nullptr;
			}
			sideFeature->addNode( putData , dirMask , nullptr );
		}
		else
		{
			sideFeature = static_cast< SideFeature* >( getFeature( linkGroup[0] ) );
			sideFeature->addNode( putData , dirMask , linkNodeFrist );
		}

		for( int i = 1 ; i < numLinkGroup ; ++i )
		{
			FeatureBase* buildOther = getFeature( linkGroup[i] );
			sideFeature->mergeData( *buildOther , putData , linkNodeFrist->outConnect->index );
			destroyFeature(  buildOther );
		}

		if ( sideFeature->calcOpenCount() != sideFeature->openCount )
		{
			CAR_LOG( "Warning: group=%d type=%d Error OpenCount!" , sideFeature->group , sideFeature->type );
		}
		return sideFeature;
	}

	FarmFeature* GameModule::updateFarm(MapTile& putData , unsigned idxMask)
	{
		int numLinkGroup = 0;
		int linkGroup[ Tile::NumFarm ];
		FarmNode* linkNodeFrist = nullptr;
		unsigned mask = idxMask;
		int idx = 0;
		while ( FBit::MaskIterator8( mask , idx ) )
		{
			FarmNode* linkNode = putData.farmNodes[ idx ].outConnect;
			if ( linkNode == nullptr )
				continue;

			int idxFind = 0;
			for( ; idxFind < numLinkGroup ; ++idxFind )
			{
				if ( linkNode->group == linkGroup[idxFind] )
					break;
			}

			if ( idxFind == numLinkGroup )
			{
				linkGroup[numLinkGroup] = linkNode->group;
				if ( numLinkGroup == 0 )
				{
					linkNodeFrist = linkNode;
				}
				++numLinkGroup;
			}
		}

		FarmFeature* farm;
		if ( numLinkGroup == 0 )
		{
			farm = createFeatureT< FarmFeature >();
			farm->addNode( putData , idxMask , nullptr );
		}
		else
		{
			farm = static_cast< FarmFeature* >( getFeature( linkGroup[0] ) );
			farm->addNode( putData , idxMask , linkNodeFrist );
		}

		for( int i = 1 ; i < numLinkGroup ; ++i )
		{
			FeatureBase* farmOther = getFeature( linkGroup[i] );
			farm->mergeData( *farmOther , putData , linkNodeFrist->outConnect->index );
			destroyFeature( farmOther );
		}
		return farm;
	}

	void GameModule::cleanupFeature()
	{
		for( int i = 0 ; i < mFeatureMap.size() ; ++i )
		{
			delete mFeatureMap[i];
		}
		mFeatureMap.clear();
	}

	void GameModule::destroyFeature(FeatureBase* build)
	{
		assert( build->group != -1 );
		build->group = -1;
		//TODO
	}

	int GameModule::getActorPutInfo(int playerId , MapTile& mapTile , bool bUsageMagicPortal , std::vector< ActorPosInfo >& outInfo )
	{
		int result = 0;
		unsigned maskAll = 0;

		maskAll = Tile::AllSideMask;
		int dir;
		while( FBit::MaskIterator4( maskAll , dir ) )
		{
			maskAll &= ~mapTile.getSideLinkMask( dir );
			int group = mapTile.getSideGroup( dir );
			if ( group == -1 )
				continue;
			FeatureBase* feature = getFeature( group );
			if ( bUsageMagicPortal && feature->checkComplete() )
				continue;

			result += feature->getActorPutInfo( playerId , dir , outInfo );
		}

		maskAll = Tile::AllFarmMask;
		int idx;
		while( FBit::MaskIterator8( maskAll , idx ) )
		{
			maskAll &= ~mapTile.getFarmLinkMask( idx );
			int group = mapTile.getFarmGroup( idx );
			if ( group == -1 )
				continue;
			FeatureBase* feature = getFeature( group );
			if ( bUsageMagicPortal && feature->checkComplete() )
				continue;
			result += feature->getActorPutInfo( playerId , idx , outInfo );
		}

		if ( mSetting->haveUse( EXP_ABBEY_AND_MAYOR ) && bUsageMagicPortal == false )
		{
			//Barn
			for( int i = 0 ; i < FDir::TotalNum ; ++i )
			{
				int n = 0;
				Vec2i posCheck = mapTile.pos;
				MapTile* mapTileCheck = &mapTile;
				int dirCheck = i;
				if ( Tile::CanLinkFarm( mapTileCheck->getLinkType( dirCheck) ) == false )
					break;
				for(  ; n < FDir::TotalNum - 1 ; ++n )
				{
					dirCheck = ( dirCheck + 1 ) % FDir::TotalNum;
					posCheck = FDir::LinkPos( posCheck , dirCheck );
					mapTileCheck = mLevel.findMapTile( posCheck );
					if ( mapTileCheck == nullptr )
						break;
					if ( Tile::CanLinkFarm( mapTileCheck->getLinkType( dirCheck) ) == false )
						break;
				}
				if ( n == FDir::TotalNum - 1 )
				{
					ActorPosInfo info;
					info.pos.type = ActorPos::eTileCorner;
					info.pos.meta = i;
					info.actorTypeMask = BIT( ActorType::eBarn );
					info.group = mapTile.farmNodes[ Tile::DirToFramIndexFrist( i ) + 1 ].group;
					outInfo.push_back( info );
					result += 1;
				}
			}
		}

		if ( mapTile.group != -1 )
		{
			FeatureBase* feature = getFeature( mapTile.group );
			if ( ( bUsageMagicPortal && feature->checkComplete() ) == false )
				result += feature->getActorPutInfo( playerId , 0 , outInfo );
		}


		return result;
	}

	int GameModule::getMaxFieldValuePlayer(FieldType::Enum type , PlayerBase* outPlayer[] , int& maxValue)
	{
		maxValue = -10000000;
		int numPlayer = 0;
		for( int i = 0 ; i < mPlayerOrders.size() ; ++i )
		{
			PlayerBase* player = mPlayerOrders[i];
			int value = player->getFieldValue(type);
			if ( value > maxValue )
			{
				outPlayer[0] = player;
				numPlayer = 1;
			}
			else if ( value == maxValue )
			{
				outPlayer[++numPlayer] = player;
			}
		}
		return numPlayer;
	}

	LevelActor* GameModule::createActor(ActorType type)
	{
		LevelActor* actor = new LevelActor;
		actor->type = type;
		mActorList.push_back( actor );
		return actor;
	}

	void GameModule::destroyActor(LevelActor* actor)
	{
		assert( actor );
		if ( actor->feature )
		{
			actor->feature->removeActor( *actor );
		}
		if ( actor->mapTile )
		{
			actor->mapTile->removeActor( *actor );
		}
		while( LevelActor* follower = actor->popFollower() )
		{
			switch( follower->type )
			{
			case ActorType::eFariy:
				break;
			}
		}
		mActorList.erase( std::find( mActorList.begin() , mActorList.end() , actor ));
		delete actor;
	}

	void GameModule::cleanupActor()
	{
		for( int i = 0 ; i < mActorList.size() ; ++i )
		{
			delete mActorList[i];
		}
		mActorList.clear();
	}

	int GameModule::addPlayerScore(int id , int value)
	{
		PlayerBase* player = mPlayerManager->getPlayer( id );
		player->mScore += value;
		return player->mScore;
	}

	int GameModule::getFollowers(unsigned playerIdMask , ActorList& outActors , LevelActor* actorSkip )
	{
		int result = 0;
		for( int i = 0 ; i < mActorList.size() ; ++i )
		{
			LevelActor* actor = mActorList[i];
			if ( actor == actorSkip )
				continue;

			if ( actor->owner && ( playerIdMask & BIT( actor->owner->getId() ) ) )
			{
				if ( mSetting->isFollower( actor->type ) )
				{
					outActors.push_back( actor );
					++result;
				}
			}
		}
		return result;
	}

	void GameModule::returnActorToPlayer(LevelActor* actor)
	{
		assert( actor->owner );
		PlayerBase* player = actor->owner;
		CAR_LOG( "Actor type=%d Return To Player %d" , actor->type , player->getId() );
		player->modifyActorValue( actor->type , 1 );

		while( LevelActor* follower = actor->popFollower() )
		{
			//TODO
			switch( follower->type )
			{
			case ActorType::eFariy:
				break;
			}
		}
		destroyActor( actor );
	}

	void GameModule::addUpdateFeature( FeatureBase* feature , bool bAbbeyUpdate )
	{
		FeatureUpdateInfoVec::iterator iter =
			std::find_if( mUpdateFeatures.begin() , mUpdateFeatures.end() , FindFeature( feature ) );
		if ( iter != mUpdateFeatures.end() )
		{
			if ( iter->bAbbeyUpdate )
				iter->bAbbeyUpdate = bAbbeyUpdate;
		}
		else
		{
			mUpdateFeatures.push_back( FeatureUpdateInfo( feature , bAbbeyUpdate ) );
		}
	}

	void GameModule::moveActor(LevelActor* actor , ActorPos const& pos , MapTile* mapTile)
	{
		assert( actor );
		actor->pos = pos;
		if ( mapTile )
			mapTile->addActor( *actor );
		else if ( actor->mapTile )
			actor->mapTile->removeActor( *actor );
	}

	void GameModule::reservePlayTile()
	{
		if ( mIdxTile == mTilesQueue.size() )
			return;

		--mIdxTile;
		mTilesQueue.erase( mTilesQueue.begin() + mIdxTile );
		mTilesQueue.push_back( mUseTileId );
		++mNumTileNeedMix;
	}

	void GameModule::checkReservedTileToMix()
	{
		if ( mNumTileNeedMix == 0 )
			return;

		int num = mTilesQueue.size() - mIdxTile;
		for( int i = 0 ; i < num ; ++i )
		{
			for( int n = 0 ; n < num ; ++n )
			{
				int idx = mIdxTile + mRandom.getInt() % num;
				std::swap( mTilesQueue[i] , mTilesQueue[idx] );
			}
		}
		mNumTileNeedMix = 0;
	}

	void GameModule::FillActionData(GameFeatureTileSelectData& data , std::vector< FeatureBase* >& linkFeatures, std::vector< MapTile* >& mapTiles)
	{
		for( int n = 0; n < linkFeatures.size() ; ++n )
		{
			GameFeatureTileSelectData::Info info;
			info.feature = linkFeatures[n];
			info.index   = mapTiles.size();
			info.num     = info.feature->mapTiles.size();
			mapTiles.insert( mapTiles.end() , info.feature->mapTiles.begin() , info.feature->mapTiles.end() );
			data.infos.push_back( info );
		}
		data.mapTiles = &mapTiles[0];
		data.numSelection = mapTiles.size();
	}

	void GameModule::placeAllTileDebug()
	{
		for( int i = 0 ; i < mTileSetManager.getReigsterTileNum() ; ++i )
		{
			MapTile* mapTile = mLevel.placeTileNoCheck( i , Vec2i( 2 *(i/5),2*(i%5) ) , 2 );
			mListener->onPutTile( *mapTile );
			UpdateTileFeatureResult updateResult;
			updateTileFeature( *mapTile , updateResult );
		}
	}

	void GameSetting::calcUsageField( int numPlayer )
	{
		struct MyFieldProc
		{
			void exec( FieldType::Enum type , int value )
			{
				fieldIndex[ type ] = numFeild++;
			}
			GameSetting* setting;
			int* fieldIndex;
			int  numFeild;
		} proc;

		proc.fieldIndex = mFieldIndex;
		proc.numFeild = 0;

		ProcField( *this , numPlayer , proc );
		mNumField = proc.numFeild;
	}

	GamePlayerManager::GamePlayerManager()
	{
		std::fill_n( mPlayerMap , MaxPlayerNum , (PlayerBase*)nullptr );
		mNumPlayer = 0;
	}

	void GamePlayerManager::clearAllPlayer(bool bDelete)
	{
		for( int i = 0 ; i < mNumPlayer ; ++i )
		{
			if ( bDelete )
				delete mPlayerMap[i];

			mPlayerMap[i] = nullptr;
		}
		mNumPlayer = 0;
	}

	void GamePlayerManager::addPlayer(PlayerBase* player)
	{
		mPlayerMap[ mNumPlayer ] = player;
		player->mId = mNumPlayer;
		++mNumPlayer;
	}


}//namespace CAR