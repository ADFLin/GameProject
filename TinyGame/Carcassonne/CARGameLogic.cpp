#include "CAR_PCH.h"
#include "CARGameLogic.h"

#include "CARGameInput.h"
#include "CARPlayer.h"
#include "CARMapTile.h"
#include "CARDebug.h"

#include "CARGameplaySetting.h"
#include "CARExpansion.h"
#include "CARParamValue.h"

#include "TemplateMisc.h"
#include <algorithm>

namespace CAR
{
#if CAR_LOGIC_DEBUG
#define MARK_EXPR_STACK( EXPR ) turnContext.returnStack.push_back(#EXPR);
#else
#define MARK_EXPR_STACK( EXPR )
#endif

#define CHECK_INPUT_RESULT( EXPR )\
	( EXPR );\
	if( !checkGameStatus(turnContext.result) ){ MARK_EXPR_STACK(EXPR); return; }

#define CHECK_RESOLVE_RESULT( EXPR )\
	( EXPR );\
	if ( turnContext.result != TurnStatus::eKeep ){ MARK_EXPR_STACK(EXPR); return; }

	static_assert(SheepToken::Num == Value::SheepTokenTypeNum,"SheepToken::Num not match Value::SheepTokenTypeNum");
	static_assert(MaxPlayerNum == Value::MaxPlayerNum,"MaxPlayerNum not match with Value::MaxPlayerNum");

	unsigned const FARMER_MASK = BIT( ActorType::eMeeple ) | BIT( ActorType::eBigMeeple ) | BIT( ActorType::ePhantom );
	unsigned const KINGHT_MASK = BIT( ActorType::eMeeple ) | BIT( ActorType::eBigMeeple ) | BIT( ActorType::ePhantom ) | BIT( ActorType::eMayor ) | BIT( ActorType::eWagon );

	unsigned const DRAGON_EAT_ACTOR_TYPE_MASK = 
		BIT( ActorType::eMeeple ) | BIT( ActorType::eBigMeeple ) | BIT( ActorType::eWagon ) | 
		BIT( ActorType::eMayor ) | BIT( ActorType::ePhantom ) | BIT( ActorType::eAbbot ) |
		BIT( ActorType::eShepherd ) | BIT( ActorType::eBuilder ) | BIT( ActorType::ePig ) |
		BIT( ActorType::eMage ) | BIT( ActorType::eWitch );
	unsigned const CastleScoreTypeMask = BIT( FeatureType::eCity ) | BIT( FeatureType::eCloister ) | BIT( FeatureType::eRoad );

	GameParamCollection& GameLogic::GetParamCollection(GameLogic& logic)
	{
		return logic.getSetting();
	}

	GameLogic::GameLogic()
		:mWorld( mTileSetManager )
	{
		mDebugMode = 0;
		mIdxPlayerTrun = 0;
		mIdxTile = 0;
		mNumTileNeedMix = 0;
		mDragon = nullptr;
		mFairy = nullptr;
		mIsStartGame = false;
		mBaseWindRose = nullptr;
		mWitch = nullptr;
		mMage = nullptr;
	}

	void GameLogic::cleanupData()
	{
		for(auto* actor : mActors)
		{
			deleteActor( actor );
		}
		mActors.clear();

		for(auto* feature : mFeatureMap)
		{
			delete feature;
		}
		mFeatureMap.clear();

		while( !mCastles.empty() )
		{
			CastleInfo* info = mCastles.back();
			delete info;
		}
		while( !mCastlesRoundBuild.empty() )
		{
			CastleInfo* info = mCastlesRoundBuild.back();
			delete info;
		}
	}

	PlayerBase* GameLogic::getOwnedPlayer(LevelActor* actor)
	{
		assert(actor && actor->ownerId != CAR_ERROR_PLAYER_ID);
		return mPlayerManager->getPlayer(actor->ownerId);
	}

	template< class T >
	T* GameLogic::createFeatureT()
	{
		T* data = new T;
		data->type     = T::Type;
		data->group    = mFeatureMap.size();
		data->mSetting = mSetting;
		mFeatureMap.push_back( data );
		return data;
	}

	void GameLogic::setup(GameplaySetting& setting, GameRandom& random , GamePlayerManager& playerManager)
	{
		mSetting = &setting;
		mRandom = &random;
		mPlayerManager = &playerManager;
		mSetting->calcUsageOfField( mPlayerManager->getPlayerNum() );
	}

	void GameLogic::loadSetting( bool beInit )
	{
		if( mSetting->have(Rule::eKingAndRobber) )
		{
			mMaxCityTileNum = 0;
			mMaxRoadTileNum = 0;
		}
		if( mSetting->have(Rule::eDragon) )
		{
			mDragon = createActor(ActorType::eDragon);
		}
		if ( mSetting->have(Rule::eFariy) )
		{
			mFairy  = createActor( ActorType::eFariy );
		}
		if( mSetting->have(Rule::eShepherdAndSheep) )
		{
			for( int i = 0 ; i < SheepToken::Num ; ++i )
			{
				mSheepBags.insert( mSheepBags.end() , CAR_PARAM_VALUE(SheepTokenNum[i]) , SheepToken(i) );
			}
		}
		if( mSetting->have(Rule::eTeacher) )
		{
			mTecher = createActor(ActorType::eTecher);
		}
		if( mSetting->have(Rule::eMageAndWitch) )
		{
			mMage = createActor(ActorType::eMage);
			mWitch = createActor(ActorType::eWitch);
		}

		struct MyFieldProc
		{
			void exec( FieldType::Enum type , int value )
			{
				player->setFieldValue( type , value );
			}
			void exec(FieldType::Enum type, int* values, int num)
			{
				player->setFieldArrayValues(type, values, num);
			}
			PlayerBase* player;
		} proc;
		for( int i = 0; i < mPlayerManager->getPlayerNum(); ++i )
		{
			PlayerBase* player = mPlayerManager->getPlayer(i);
			player->setupSetting( *mSetting );
			player->mScore = 0;
			proc.player = player;
			ProcessFields( *mSetting , mPlayerManager->getPlayerNum() , proc );
		}

		if( beInit )
		{
			if( !(mDebugMode & EDebugModeMask::RemoveBasicTitles) )
			{
				mTileSetManager.addExpansion(EXP_BASIC);
			}
			if( mDebugMode & EDebugModeMask::DrawTestTileFrist )
			{
				mTileSetManager.addExpansion(EXP_TEST);
			}
			for( int i = 0; i < NUM_EXPANSIONS; ++i )
			{
				if( mSetting->haveUse(Expansion(i)) )
					mTileSetManager.addExpansion(Expansion(i));
			}
		}
	}

	void GameLogic::restart( bool beInit )
	{
		if ( !beInit )
		{
			cleanupData();
			mDragon = nullptr;
			mFairy = nullptr;
			mTecher = nullptr;
			mMage = nullptr;
			mWitch = nullptr;

			mBaseWindRose = nullptr;

			mTilesQueue.clear();
			mOrderedPlayers.clear();
			mTowerTiles.clear();
			mPrisoners.clear();
			mSheepBags.clear();
			mWorld.restart();
		}

		mbUseLaPorxadaScoring = false;
		mIsRunning = false;
		loadSetting( beInit );
	}

	void GameLogic::run(IGameInput& input)
	{
		TGuardValue< bool > gurdValue(mIsStartGame, true);

		if ( mPlayerManager->getPlayerNum() == 0 )
		{
			CAR_LOG( "GameModule::runLogic: No Player Play !!");
			return;
		}

		
		mIdxPlayerTrun = 0;
		randomPlayerPlayOrder();
		{
			TileId startId = generatePlayTiles();
			if ( mDebugMode & EDebugModeMask::ShowAllTitles )
			{
				placeAllTileDebug( 5 );
			}
			else
			{
				if( startId == FAIL_TILE_ID )
				{
					startId = drawPlayTile();
				}

				PlaceTileParam param;
				param.checkRiverConnect = true;
				param.canUseBridge = false;

				MapTile* placeMapTiles[ 9 ];
				int numMapTile = mWorld.placeTileNoCheck( startId , Vec2i(0,0) , 0 , param , placeMapTiles );
				mListener->notifyPlaceTiles( startId , placeMapTiles , numMapTile );

				FeatureUpdateInfoVec updateFeatures;
				UpdateTileFeatureResult updateResult(updateFeatures);
				for( int i = 0 ; i < numMapTile ; ++i )
				{
					updateTileFeature( *placeMapTiles[i] , updateResult );
				}
			}
		}

		mNumTrun = 0;
		mIsRunning = true;
		mbNeedShutdown = false;

		while( mIsRunning )
		{
			if ( getRemainingTileNum() == 0 )
				break;

			++mNumTrun;
			PlayerBase* curTrunPlayer = mOrderedPlayers[ mIdxPlayerTrun ];
			curTrunPlayer->startTurn();


			TurnContext turnContext( curTrunPlayer , input );
			resolvePlayerTurn(turnContext);
			if ( turnContext.result == eFinishGame )
				break;
			if ( turnContext.result == eExitGame )
				return;

			if ( mIsRunning )
			{
				GameActionData data;
				TurnStatus result;
				input.requestTurnOver( data );
				if( !checkGameStatus(result) )
				{

#if CAR_LOGIC_DEBUG
					for( auto const& str : turnContext.returnStack )
					{
						CAR_LOG(str.c_str());
					}
#endif
					if( result == eFinishGame )
						break;
					if( result == eExitGame )
						return;
				}
			}

			curTrunPlayer->endTurn();

			if ( mSetting->have(Rule::eCastleToken) )
			{
				mCastlesRoundBuild.moveAfter( mCastles.back() );
			}
			++mIdxPlayerTrun;
			if ( mIdxPlayerTrun == mOrderedPlayers.size() )
				mIdxPlayerTrun = 0;
		}

		calcFinalScore();

	}

	void GameLogic::calcFinalScore()
	{
		for(FeatureBase* build : mFeatureMap)
		{
			if ( build->group == ERROR_GROUP_ID )
				continue;

			FeatureScoreInfo featureScore;
			if ( build->checkComplete() )
			{
				if ( build->type == FeatureType::eCity )
				{
					CityFeature* city = static_cast< CityFeature* >( build );
					switch( mSetting->getFarmScoreVersion() )
					{
					case 1:
						{
							for( FarmFeature* farm : city->linkedFarms )
							{
								farm->generateMajority(featureScore.controllerScores);
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
				build->calcScore( *mPlayerManager , featureScore );
				if( featureScore.numController > 0 )
				{
					for( int i = 0; i < featureScore.numController; ++i )
					{
						auto& controllerScore = featureScore.controllerScores[i];
						modifyPlayerScore(controllerScore.playerId, controllerScore.score);
					}
				}
					
			}
		}

		if ( mSetting->have(Rule::eTraders) )
		{
			PlayerBase* players[ MaxPlayerNum ];
			FieldType::Enum treadeFields[] = { FieldType::eWine , FieldType::eGain , FieldType::eCloth };
			for( int i = 0 ; i < ARRAY_SIZE( treadeFields ) ; ++i )
			{
				int maxValue = 0;
				int numPlayer = getMaxFieldValuePlayer( treadeFields[i] , players , maxValue );
				for( int n = 0 ; n < numPlayer ; ++n )
				{
					modifyPlayerScore( players[n]->getId() , CAR_PARAM_VALUE(TradeScore) );
				}
			}
		}

		if( mSetting->have(Rule::eKingAndRobber) )
		{		
			int numCityComplete = 0;
			int numRoadComplete = 0;

			for(FeatureBase* feature : mFeatureMap)
			{
				if ( feature->group == ERROR_GROUP_ID )
					continue;
				if ( feature->checkComplete() == false )
					continue;

				if ( feature->type == FeatureType::eCity )
				{
					if ( static_cast< CityFeature* >( feature )->isCastle == false )
						++numCityComplete;
				}
				else if ( feature->type == FeatureType::eRoad )
					++numRoadComplete;
			}

			if ( mMaxCityTileNum != 0 )
			{
				modifyPlayerScore( mIdKing , numCityComplete * CAR_PARAM_VALUE(KingFactor) );
			}
			if ( mMaxRoadTileNum != 0 )
			{
				modifyPlayerScore( mIdRobberBaron , numRoadComplete * CAR_PARAM_VALUE(RobberBaronFactor) );
			}
		}

		if( mSetting->have(Rule::eGold) )
		{
			for( int i = 0; i < mPlayerManager->getPlayerNum(); ++i )
			{
				PlayerBase* player = mPlayerManager->getPlayer(i);
				int numGoldPieces = player->getFieldValue(FieldType::eGoldPieces);
				int scoreFactor = 0;
				for( int i = 0; i < 4; ++i )
				{
					if( numGoldPieces > CAR_PARAM_VALUE(GoldPiecesScoreFactorNum[i]) )
					{
						scoreFactor = CAR_PARAM_VALUE(GoldPiecesScoreFactor[i]);
						break;
					}
				}
				modifyPlayerScore(player->getId(), scoreFactor * numGoldPieces);
			}
		}

		if( mSetting->have(Rule::eMessage) )
		{
			for( int i = 0; i < mPlayerManager->getPlayerNum(); ++i )
			{
				PlayerBase* player = mPlayerManager->getPlayer(i);
				player->mScore += player->getFieldValue(FieldType::eWomenScore);
			}
		}
	}

	TileId GameLogic::generatePlayTiles()
	{
		int numTile = mTileSetManager.getRegisterTileNum();
		std::vector< int > tilePieceMap ( numTile );
		std::vector< TileId > specialTileList;
		for( int i = 0 ; i < numTile ; ++i )
		{
			TileSet const& tileSet = mTileSetManager.getTileSet( TileId(i) );
			if ( tileSet.tag != 0 )
				specialTileList.push_back( tileSet.tiles[0].id );
				
			tilePieceMap[i] = tileSet.numPieces;
		}
	
		auto FindSpecialTileIndex = [&](Expansion exp, TileTag tag) -> int
		{
			for( int idx = 0; idx < specialTileList.size(); ++idx )
			{
				TileSet const& tileSet = mTileSetManager.getTileSet(specialTileList[idx]);
				if( tileSet.expansion == exp && tileSet.tag == tag )
					return idx;
			}
			return -1;
		};

		bool bUseTestTiles = !!(mDebugMode & EDebugModeMask::DrawTestTileFrist);
		bool bUseRiver = false;
		if( mSetting->have(Rule::eHaveRiverTile) )
		{
			bUseRiver = true;
		}

		//River
		TileId tileIdStart = FAIL_TILE_ID;
		TileId tileIdRiverEnd = FAIL_TILE_ID;
		TileId tileIdFristPlay = FAIL_TILE_ID;

		if ( bUseRiver )
		{
			if ( mSetting->haveUse( EXP_THE_RIVER_II ) )
			{
				int idx;
				idx = FindSpecialTileIndex( EXP_THE_RIVER_II , TILE_START_TAG );
				if ( idx != -1 )
					tileIdStart = specialTileList[ idx ];
				idx = FindSpecialTileIndex( EXP_THE_RIVER_II , TILE_END_TAG );
				if ( idx != -1 )
					tileIdRiverEnd = specialTileList[ idx ];
				idx = FindSpecialTileIndex( EXP_THE_RIVER_II , TILE_FRIST_PLAY_TAG );
				if ( idx != -1 )
					tileIdFristPlay = specialTileList[ idx ];
			}

			if ( mSetting->haveUse( EXP_THE_RIVER_I ) )
			{
				int idx;
				idx = FindSpecialTileIndex( EXP_THE_RIVER_I , TILE_START_TAG );
				if ( idx != -1 )
				{
					TileId id = specialTileList[idx];
					if ( tileIdStart != FAIL_TILE_ID )
					{
						tilePieceMap[ id ] = 0;
					}
					else
					{
						tileIdStart = id;
					}
				}

				idx = FindSpecialTileIndex( EXP_THE_RIVER_I , TILE_END_TAG );
				if ( idx != -1 )
				{
					TileId id = specialTileList[idx];
					if ( tileIdRiverEnd != FAIL_TILE_ID )
					{
						tilePieceMap[ id ] = 0;
					}
					else
					{
						tileIdRiverEnd = id;
					}
				}
			}

			if ( tileIdStart != FAIL_TILE_ID )
				tilePieceMap[ tileIdStart ] -= 1;
			if ( tileIdRiverEnd != FAIL_TILE_ID )
				tilePieceMap[ tileIdRiverEnd ] -= 1;
			if ( tileIdFristPlay != FAIL_TILE_ID )
				tilePieceMap[ tileIdFristPlay ] -= 1;

			if ( tileIdStart == FAIL_TILE_ID || tileIdRiverEnd == FAIL_TILE_ID )
			{
				CAR_LOG( "Exp River No Start or End Tile!");
			}
		}

		struct ShuffleGroup
		{
			int  indexEnd;
			bool bNeedShuffle;
		};

		std::vector< ShuffleGroup > shuffleGroups;
		auto PushShuffleGroup = [&]( bool bNeedShuffle )
		{
			shuffleGroups.push_back({ (int)mTilesQueue.size() , bNeedShuffle });
		};

		if ( bUseRiver )
		{
			TileIdVec const& idSetGroup = mTileSetManager.getGroup( TileSet::eRiver );
			if ( tileIdFristPlay != FAIL_TILE_ID )
			{
				mTilesQueue.push_back( tileIdFristPlay );
				PushShuffleGroup(false);
			}
			for(TileId id : idSetGroup)
			{
				if ( tilePieceMap[id] != 0)
					mTilesQueue.insert( mTilesQueue.end() , tilePieceMap[id] , id );
			}

			PushShuffleGroup(true);
			if ( tileIdRiverEnd != FAIL_TILE_ID )
			{
				mTilesQueue.push_back( tileIdRiverEnd );
				PushShuffleGroup(false);
			}
		}

		if( mSetting->have(Rule::eWindRose) )
		{


		}
		//Common
		if ( tileIdStart == FAIL_TILE_ID )
		{
			int idx = FindSpecialTileIndex( EXP_BASIC , TILE_START_TAG );
			if ( idx == -1 )
			{
				tileIdStart = FAIL_TILE_ID;
				CAR_LOG( "Warning: Cant Find Start Tile In Common Set");
			}
			else
			{
				tileIdStart = specialTileList[idx];
			}
			tilePieceMap[ tileIdStart ] -= 1;
		}

		auto AddGroupTitles = [&](TileSet::EGroup group, bool bNeedShuffle)
		{
			TileIdVec const& idSetGroup = mTileSetManager.getGroup(group);
			for(TileId id : idSetGroup)
			{
				if (tilePieceMap[id])
				{
					mTilesQueue.insert(mTilesQueue.end(), tilePieceMap[id], id);
				}
			}

			PushShuffleGroup(bNeedShuffle);
		};

		if( bUseTestTiles )
		{
			AddGroupTitles(TileSet::eTest, false);
		}

		AddGroupTitles(TileSet::eCommon, true);

		int idxPrev = 0;
		for(ShuffleGroup const& group : shuffleGroups)
		{
			int idx = group.indexEnd;
			if (idx != idxPrev && group.bNeedShuffle)
			{
				shuffleTiles(&mTilesQueue[0] + idxPrev, &mTilesQueue[0] + idx);
			}
			idxPrev = idx;
		}

		if ( mSetting->have(Rule::eHaveAbbeyTile) )
		{
			int idx = FindSpecialTileIndex( EXP_ABBEY_AND_MAYOR , TILE_ABBEY_TAG );
			if ( idx != 0 )
			{
				CAR_LOG( "Can't Find Abbey Tile!");
				mAbbeyTileId = FAIL_TILE_ID;
			}
			else
			{
				mAbbeyTileId = specialTileList[idx];
			}
		}

		if ( mSetting->have( Rule::eCastleToken ) )
		{
			TileIdVec const& castales = mTileSetManager.getGroup( TileSet::eGermanCastle );
			int numCastalePlayer = mPlayerManager->getPlayerNum() > 3 ? 1 : 2;
			//#TODO
		}

		if( mSetting->have( Rule::eHaveHalflingTile ) )
		{



		}
		mIdxTile = 0;
		mNumTileNeedMix = 0;
		return tileIdStart;
	}

	void GameLogic::randomPlayerPlayOrder()
	{
		int numPlayer = mPlayerManager->getPlayerNum();
		for( int i = 0; i < numPlayer; ++i )
		{
			mOrderedPlayers.push_back(mPlayerManager->getPlayer(i));
		}

		for( int i = 0; i < numPlayer; ++i )
		{
			for( int n = 0; n < numPlayer; ++n )
			{
				int idx = mRandom->getInt() % numPlayer;
				std::swap(mOrderedPlayers[i], mOrderedPlayers[idx]);
			}
		}

		for( int i = 0; i < numPlayer; ++i )
		{
			mOrderedPlayers[i]->mPlayOrder = i;
		}
	}

	bool CheckTileContent(MapTile* mapTiles[], int numTile, TileContentMask contentMask)
	{
		for( int i = 0; i < numTile; ++i )
		{
			if( mapTiles[i]->getTileContent() & contentMask )
				return true;
		}
		return false;
	}

	bool CheckTileContent(TileSet const& tileSet, TileContentMask contentMask)
	{
		for( int i = 0; i < tileSet.numPieces; ++i )
		{
			if( tileSet.tiles[i].contentFlag & contentMask )
				return true;
		}
		return false;
	}

	void GameLogic::resolvePlayerTurn(TurnContext& turnContext)
	{
		//Any time during your turn : 
		//-You may ask for advice.
		//-You may read the rules for the expansions you are playing with.
		//-You may buy back any one of your imprisoned followers by paying the captor 3 points. ROBBERS 
		//-You may claim an unclaimed tunnel portal by placing a tunnel token on it.
		//-You may allow a follower to take flight from the Plague(once an infestation is active).
		//-You must spread the plague by placing a flea token(once an infestation is active).

		//Step 1: Begin Turn
		//a)  If the fairy is next to one of your followers, score 1 point. MESSAGES ROBBERS
		//b)  if no fleas are left in the supply, eliminate the oldest Outbreak unless this would eliminate all Outbreaks.
		if( mSetting->have(Rule::eFariy) )
		{
			if( mFairy->binder && mFairy->binder->ownerId == turnContext.getPlayer()->getId() )
			{
				CAR_LOG("%d Score:Fairy Beginning Of A Turn Score", turnContext.getPlayer()->getId());
				CHECK_RESOLVE_RESULT(resolveScorePlayer(turnContext, turnContext.getPlayer()->getId(), CAR_PARAM_VALUE(FairyBeginningOfATurnScore)));
			}
		}


		int numUseTilePices = 0;
		for( ;;)// step 2 to 9
		{
			bool haveHillTile;
			//Step 2: Draw a Tile
			CHECK_RESOLVE_RESULT(resolveDrawTile(turnContext, haveHillTile));

			int const MAX_MAP_TILES_NUM = 32;

			MapTile* placeMapTiles[MAX_MAP_TILES_NUM];
			int numPlaceTile = 0;
			{
				//Step 3: Place the Tile
				CHECK_RESOLVE_RESULT(resolvePlaceTile(turnContext, placeMapTiles, numPlaceTile));

				++numUseTilePices;
				//b)Note: any feature that is finished is considered complete at this time.

				UpdateTileFeatureResult updateResult( turnContext.getUpdateFeatures() );
				for( int i = 0; i < numPlaceTile; ++i )
				{
					placeMapTiles[i]->haveHill = haveHillTile;	
					updateTileFeature(*placeMapTiles[i], updateResult);
				}

				if( mSetting->have(Rule::eMageAndWitch) )
				{
					if( updateResult.numSideFeatureMerged )
					{
						if( mMage->feature && mMage->feature == mWitch->feature )
						{
							resolveMoveMageOrWitch(turnContext);
						}
					}
				}
			}

			bool haveVolcano = false;
			bool havePlagueSource = false;
			bool skipAllActorStep = false;


			TileContentMask allTileConstent = 0;

			for (int i = 0; i < numPlaceTile; ++i)
			{
				allTileConstent |= placeMapTiles[i]->getTileContent();
			}

			auto CheckHaveTileContextInPlaceTiles = [allTileConstent](TileContent::Enum content) -> bool
			{
				return !!(allTileConstent & BIT(content));
			};
			auto GetTileInPlaceTiles = [&](TileContent::Enum content) -> MapTile*
			{
				for( int i = 0; i < numPlaceTile; ++i )
				{
					if( placeMapTiles[i]->have(content) )
						return placeMapTiles[i];
				}
				return nullptr;
			};

			//c)  If a volcano symbol is on the tile, place the dragon on this tile.
			if( mSetting->have(Rule::eDragon) )
			{
				for( int i = 0; i < numPlaceTile; ++i )
				{
					MapTile* mapTile = placeMapTiles[i];
					if( mapTile->have(TileContent::eVolcano) )
					{
						mTowerTiles.push_back(placeMapTiles[i]);
						if( mDragon->mapTile == nullptr )
							checkReservedTileToMix();

						moveActor(mDragon, ActorPos::Tile(), mapTile);
						haveVolcano = true;
					}
				}
			}

			//d) If a princess symbol is on the tile, and the tile is added to an existing city with a knight on it, 
			//   you may remove a knight of your choice and skip all of Step 4.
			if( mSetting->have(Rule::ePrinecess) )
			{
				bool haveDone = false;
				for( int i = 0; i < numPlaceTile; ++i )
				{
					CHECK_RESOLVE_RESULT( resolvePrincess(turnContext , placeMapTiles[i], haveDone) );
					if ( haveDone )
						break;
				}
				if( haveDone )
					skipAllActorStep = true;
			}

			//e)  If a plague source is on the tile, the lowest numbered Outbreak token not yet in play must be placed on it.


			//f)  If a gold symbol is on the tile, you must place a gold piece on that tile and an adjacent tile.
			if( mSetting->have(Rule::eGold) )
			{
				bool bHaveGoldSymbol = false;
				for( int i = 0; i < numPlaceTile; ++i )
				{
					if( placeMapTiles[i]->have(TileContent::eGold) )
					{
						CHECK_RESOLVE_RESULT(resolvePlaceGoldPieces(turnContext, placeMapTiles[i]));
					}
				}
			}

			//g)  If a mage and witch symbol is on the tile, or if the tile joins the features with theMage and Witch, you must move the Mage or the Witch.
			if( mSetting->have(Rule::eMageAndWitch) )
			{
				if( CheckHaveTileContextInPlaceTiles(TileContent::eMage) )
				{
					CHECK_RESOLVE_RESULT(resolveMoveMageOrWitch(turnContext));
				}
			}

			//h)  If a robber symbol is on the tile, your robber and the next player's robber may be placed on the scoreboard.If you had already played your robber, you may move it.

			//i)  If there is a quarter-wind rose on the tile and you place the tile in the appropriate quadrant of the playing field, 
			//    you score 3 points. MESSAGES ROBBERS 
			if( mSetting->have(Rule::eWindRose) )
			{
				for( int i = 0; i < numPlaceTile; ++i )
				{
					auto const tileContext = placeMapTiles[i]->getTileContent();
					if( tileContext & TileContent::OrangeWindRoseMask )
					{
						assert(mBaseWindRose);
						Vec2i offsetLocal = FDir::ToLocal( placeMapTiles[i]->pos - mBaseWindRose->pos , mBaseWindRose->rotation );

						if( ( (tileContext & BIT(TileContent::eWindRose_E)) == 0 || offsetLocal.x >= 0 ) &&
							( (tileContext & BIT(TileContent::eWindRose_W)) == 0 || offsetLocal.x <= 0 ) &&
							( (tileContext & BIT(TileContent::eWindRose_N)) == 0 || offsetLocal.y >= 0 ) &&
							( (tileContext & BIT(TileContent::eWindRose_S)) == 0 || offsetLocal.y <= 0 ) )
						{
							resolveScorePlayer(turnContext, turnContext.getPlayer()->getId(), CAR_PARAM_VALUE(WindRoseScore));
						}	
					}
					else if( tileContext & BIT(TileContent::eBlueWindRose) )
					{
						mBaseWindRose = placeMapTiles[i];
					}
				}
			}

			//j)  If a hill is on the tile, place the hill tile while keeping the face - down second tile underneath it.
			//
			if( mSetting->have(Rule::eLaPorxada) )
			{
				if(CheckHaveTileContextInPlaceTiles(TileContent::eLaPorxada))
				{
					CHECK_RESOLVE_RESULT(resolveLaPorxadaCommand(turnContext));
				}
			}

			if( mSetting->have(Rule::eTower) )
			{
				for( int i = 0; i < numPlaceTile; ++i )
				{
					if( placeMapTiles[i]->have( TileContent::eTowerFoundation ) )
						mTowerTiles.push_back(placeMapTiles[i]);
				}
			}

			bool haveBuilderFeatureExpend = false;
			if( mSetting->have(Rule::eBuilder) )
			{
				haveBuilderFeatureExpend = checkHaveBuilderFeatureExpend(turnContext.getPlayer(), turnContext.getUpdateFeatures());
			}

			if ( skipAllActorStep == false )
			{

				//Step 4A: Move the Wood (Phase 1)
				bool bMoveTheWoodStepFinished = false;
				do 
				{
					GameSelectActionOptionData actionDataMTW;
					actionDataMTW.group = ACTOPT_GROUP_MOVE_THE_WOOD;
					actionDataMTW.bCanSkip = true;

					bool haveDeployActor = false;
					if ( haveVolcano == false && havePlagueSource == false )
					{
						bool havePortal = CheckHaveTileContextInPlaceTiles(TileContent::eMagicPortal);
						bool haveFlier  = CheckHaveTileContextInPlaceTiles(TileContent::eAircraft );
						bool hadUsedPortal = false;
						bool hadUsedAircraft = false;

						for( int step = 0 ; step < 2 ; ++step )
						{
							unsigned actorMask;
							unsigned ignoreDeployFeaturesMask = 0;
							if ( step == 0 )
							{
								actorMask = turnContext.getPlayer()->getSupportActorMask() & ~BIT( ActorType::ePhantom );
							}
							else //step == 1
							{
								if ( turnContext.getPlayer()->haveActor( ActorType::ePhantom ) == false )
									continue;

								actorMask = BIT( ActorType::ePhantom );
							}

							MapTile* deployMapTiles[MAX_MAP_TILES_NUM];
							std::copy_n( placeMapTiles , numPlaceTile , deployMapTiles);
							int numDeployTile = numPlaceTile;
							do 
							{
								if( havePortal && !hadUsedPortal )
								{
									MapTile* targetTile = nullptr;
									CHECK_RESOLVE_RESULT(resolvePortalUse(turnContext, targetTile));

									if( targetTile )
									{
										deployMapTiles[0] = targetTile;
										numDeployTile = 1;
										hadUsedPortal = true;
										break;
									}
								}
								if( haveFlier && !hadUsedAircraft )
								{
									MapTile* aircraftTile = GetTileInPlaceTiles(TileContent::eAircraft );
									MapTile* targetTile = nullptr;
									CHECK_RESOLVE_RESULT(resolveeAircraftUse(turnContext, aircraftTile, targetTile));

									if( targetTile )
									{
										deployMapTiles[0] = targetTile;
										numDeployTile = 1;
										hadUsedAircraft = true;
										ignoreDeployFeaturesMask |= BIT(FeatureType::eFarm);
										break;
									}
								}
							} 
							while (false);

							if ( numDeployTile )
							{
								bool haveDone = false;
								if( hadUsedPortal )
									actorMask &= mSetting->getFollowerMask();

								//a-c) follower and other figures
								CHECK_RESOLVE_RESULT( resolveDeployActor(turnContext, deployMapTiles , numDeployTile , actorMask , hadUsedPortal, ignoreDeployFeaturesMask, haveDone ) );

								if ( haveDone )
									haveDeployActor = true;
							}
						}
					}
					if( haveDeployActor )
						break;

					//d) Perform a special action
					// - Place a follower on the Wheel of Fortune
					

					// - Remove a figure from anywhere in the playing area if the festival symbol was on the played tile
					if (mSetting->have(Rule::eFestival))
					{
						if ( CheckHaveTileContextInPlaceTiles(TileContent::eFestival))
						{

						}
					}

					// - Remove your abbot(C II) and score its points MESSAGE SROBBERS
					if (mSetting->have(Rule::eAbbot))
					{
						LevelActor* abbot = findActor(BIT(turnContext.getPlayer()->getId()), BIT(ActorType::eAbbot));
						if( abbot )
						{
							assert(abbot->mapTile);
							FeatureBase* feature = getFeature( abbot->mapTile->group );
							int score = feature->calcPlayerScore(turnContext.getPlayer());
							CHECK_RESOLVE_RESULT( resolveScorePlayer(turnContext, turnContext.getPlayer()->getId(), score) );
							returnActorToPlayer(abbot);
						}
					}

					//e) Deploy / move a neutral figure
					// - Place a tower piece on any tower base or available tower.
					//   = You may capture a follower if appropriate.
					//   = If two players have captured one of each other's followers, they must immediately be exchanged.
					bool haveDone = false;
					if( mSetting->have(Rule::eTower) )
					{
						CHECK_RESOLVE_RESULT(resolveTower(turnContext, haveDone));
						if( haveDone )
							break;
					}
					// - Place a little building on the tile just played
					if( mSetting->have(Rule::eLittleBuilding) )
					{
						CHECK_RESOLVE_RESULT(resolvePlaceLittleBuilding(turnContext, placeMapTiles, numPlaceTile, haveDone));
						if( haveDone )
							break;
					}

					// - Move the fairy next to one of your followers.
					if( mSetting->have(Rule::eFariy) )
					{
						CHECK_RESOLVE_RESULT(resolveMoveFairyToNextFollower(turnContext, haveDone));
						if( haveDone )
							break;
					}
				} 
				while ( bMoveTheWoodStepFinished );

				//Step 4B: Move the Wood (Phase 2)
				//Skip this step if you removed a knight with a princess symbol.Otherwise, you may do the following:

				//a) You may place the phantom in the same way as a normal follower.The flier and magic portal can be used as long as they were not used in Step 4A.
				//    This step is forbidden if a volcano or plague source tile was played.
				if( !haveVolcano && !havePlagueSource )
				{



				}
				
			}

			//Step 5: Resolve Move the Wood
			//a) If a ferry lake was on the placed tile, place a ferry.

			//b) If placement of a tile extended a road with a ferry, the ferry maybe moved.
			//   Each ferry may only be moved once per turn.

			//c) If placement of the tile extended the field with your shepherd, you must choose one of the following two actions
			//  - Expand the flock(draw a token)
			//  - Herd the flock into the stable(score 1 point per sheep)
			if ( mSetting->have( Rule::eShepherdAndSheep ) )
			{
				for( auto& updateInfo : turnContext.getUpdateFeatures() )
				{
					FeatureBase* feature = updateInfo.feature;

					if ( feature->group == ERROR_GROUP_ID )
						continue;
					if ( feature->type != FeatureType::eFarm )
						continue;

					CHECK_RESOLVE_RESULT( resolveExpendShepherdFarm(turnContext, feature ) );
				}
			}
			//d) HiG & ZMG rules:If a dragon symbol was on the placed tile, move the dragon.
			if ( mSetting->have( Rule::eDragon ) )
			{
				if ( mSetting->have( Rule::eMoveDragonBeforeScoring ) == true )
				{
					if( CheckHaveTileContextInPlaceTiles(TileContent::eTheDragon) &&
					    mDragon->mapTile != nullptr )
					{
						CHECK_RESOLVE_RESULT( resolveDragonMove(turnContext, *mDragon) );
					}
				}
			}

			std::vector< FeatureScoreInfo > featureScoreList;

			//Step 6: Identify Completed Features
			for( auto& updateInfo : turnContext.getUpdateFeatures() )
			{
				FeatureBase* feature = updateInfo.feature;

				if ( feature->group == ERROR_GROUP_ID )
					continue;

				//a) Identify all completed features
				if ( feature->checkComplete() )
				{
					CAR_LOG( "Feature %d complete by Player %d" , feature->group , turnContext.getPlayer()->getId()  );
					//Step 7: Resolve Completed Features

					FeatureScoreInfo featureScore;
					CHECK_RESOLVE_RESULT( resolveCompletedFeature(turnContext, *feature , featureScore , nullptr ) );

					if(  featureScore.numController > 0 )
					{
						featureScoreList.push_back(std::move(featureScore));
					}
				}
				else if ( feature->type == FeatureType::eFarm )
				{
					if ( mSetting->have( Rule::eBarn ) && feature->haveActorFromType( BIT( ActorType::eBarn ) ) )
					{
						FarmFeature* farm = static_cast< FarmFeature*>( feature );
						FeatureScoreInfo featureScore;
						updateBarnFarm(farm , featureScore);

						if( featureScore.numController > 0 )
						{
							featureScoreList.push_back(std::move(featureScore));
						}
					}
				}
			}

			//castle complete
			if ( mSetting->have(Rule::eCastleToken) )
			{
				CHECK_RESOLVE_RESULT( resolveCompletedCastles(turnContext, featureScoreList) );
			}


			//Step 8: Resolve the Tile
			//a) Consider the total movement of your counting followers after all completed features are scored : MESSAGES

			struct PlayerFeatureScoreInfo
			{
				FeatureBase* feature;
				PlayerId     playerId;
				int          score;
			};

			if ( !featureScoreList.empty() )
			{
				std::vector< PlayerFeatureScoreInfo > sortedPlayerScoreList;
				for( auto& featureScore : featureScoreList )
				{
					assert(featureScore.numController > 0);
					for( int i = 0; i < featureScore.numController; ++i )
					{
						auto& controllerScore = featureScore.controllerScores[i];
						PlayerFeatureScoreInfo info;
						info.playerId = controllerScore.playerId;
						info.score = controllerScore.score;
						info.feature = featureScore.feature;
						sortedPlayerScoreList.push_back(info);
						CHECK_RESOLVE_RESULT(resolveScorePlayer(turnContext, controllerScore.playerId, controllerScore.score));
					}
				}

				std::sort(sortedPlayerScoreList.begin(), sortedPlayerScoreList.end(), [](auto const& lhs, auto const& rhs)
				{
					return lhs.playerId < rhs.playerId;
				});

				auto NeedConsiderScoreOrder = [&](PlayerId id)
				{
					PlayerBase* player = mPlayerManager->getPlayer(id);
					return false;
				};
				PlayerFeatureScoreInfo* playerScoreStart = &sortedPlayerScoreList[0];
				PlayerFeatureScoreInfo* scoreEnd = &sortedPlayerScoreList.back() + 1;
				while( playerScoreStart != scoreEnd )
				{
					PlayerFeatureScoreInfo* playerScoreEnd = playerScoreStart + 1;
					while( playerScoreEnd != scoreEnd )
					{
						if ( playerScoreEnd->playerId != playerScoreStart->playerId )
							break;
						++playerScoreEnd;
					}

					//@TODO : consider other player order

					for ( ; playerScoreStart != playerScoreEnd ; ++playerScoreStart )
					{
						CHECK_RESOLVE_RESULT( resolveScorePlayer( turnContext , playerScoreStart->playerId, playerScoreStart->score) );
					}
				}
			}
			//b) If you played a gingerbread man tile on this turn, move the Gingerbread Man to a different incomplete city.
			//   All players receive points for any knights they have in the city that the Gingerbread Man just left. MESSAGES ROBBERS

			//c) If you did not score any points from placement of the tile this turn, but one or more opponents did, 
			//   you may place a follower in the City of Carcassonne.Then you may move the Count to a quarter of your choice.

			//d) RGG rules only : If a dragon symbol was on the placed tile, move the dragon.
			if( mSetting->have(Rule::eDragon) )
			{
				if ( mSetting->have(Rule::eMoveDragonBeforeScoring ) == false )
				{
					if( CheckHaveTileContextInPlaceTiles(TileContent::eTheDragon)  &&
					   mDragon->mapTile != nullptr )
					{
						CHECK_RESOLVE_RESULT( resolveDragonMove(turnContext, *mDragon) );
					}
				}
			}
			//e) If a fair symbol was on the placed tile, use the catapult. MESSAGES ROBBERS
			if( mSetting->have(Rule::eFair) )
			{
				if( CheckHaveTileContextInPlaceTiles(TileContent::eFair) )
				{

				}
			}
			
			//f) If a crop circle was on the placed tile, you decide whether all players now 
			//   ->A) may deploy a follower next to one already in play or 
			//   ->B) must remove a follower.The type of follower is determined by the type of crop circle.
			if( mSetting->have(Rule::eCropCircle) )
			{



			}

			//Step 9: Resolve the Turn
			//a) If you had a builder on a road or city that was extended by the placement of the tile, 
			//   repeat Steps 2 through 8 once more and only once more.
			if ( mSetting->have( Rule::eBuilder ) && numUseTilePices == 1 )
			{
				if ( haveBuilderFeatureExpend )
				{
					CAR_LOG( "Player %d have double Turn" , getTurnPlayer()->getId() );
					continue;
				}
			}
			//b) If a bazaar symbol was on the placed tile (and the tile was not purchased during an auction), 
			//   perform an auction.ROBBERS
			if ( mSetting->have( Rule::eBazaar ) )
			{
				for( int i = 0 ; i < numPlaceTile; ++i )
				{
					MapTile* mapTile = placeMapTiles[i];
					if ( mapTile->have( TileContent::eBazaar ) )
					{
						CHECK_RESOLVE_RESULT( resolveAuction(turnContext ) );
					}
				}
			}

			//c) You may remove one knight from a besieged city if a cloister directly borders any tile of that city(Cults & Sieges) or 
			//   if a cloister directly borders a Cathar tile(The Cathars) or Besiegers tile(The Besiegers).
			break;
		}
	}

	void GameLogic::resolveDrawTile(TurnContext& turnContext, bool& haveHillTile )
	{
		//a)  If you have a tile from a previous bazaar auction, you must use that tile.
		//b)  If you have an abbey tile, you may draw it in place of drawing a regular tile.
		//c)  If you have a Halfling tile and did not play an abbey tile, you may choose it in place of drawing a regular tile.
		//d)  If you did not perform a - c, randomly draw a tile.
		//e)  Show the tile to all players.
		//f)  If a Wheel of Fortune icon is on the tile, resolve Wheel of Fortune. MESSAGES ROBBERS
		//g)  If a hill is depicted on the tile, immediately draw a second tile.Keep the second tile face - down underneath the tile with the hill.
		haveHillTile = false;

		TileId hillTileId = FAIL_TILE_ID;
		int countDrawTileLoop = 0;
		for(;;)
		{
			GameSelectActionOptionData data;
			data.group = ACTOPT_GROUP_DRAW_TILE;
			if( mSetting->have( Rule::eHaveHalflingTile ) )
			{

			}
			
			++countDrawTileLoop;

			hillTileId = FAIL_TILE_ID;
			mUseTileId = FAIL_TILE_ID;

			if ( mSetting->have(Rule::eBazaar) )
			{
				if ( turnContext.getPlayer()->getFieldValue( FieldType::eTileIdAuctioned ) != FAIL_TILE_ID )
				{
					mUseTileId = (TileId)turnContext.getPlayer()->getFieldValue( FieldType::eTileIdAuctioned );
					turnContext.getPlayer()->setFieldValue( FieldType::eTileIdAuctioned , FAIL_TILE_ID );
					CAR_LOG( "Player %d Use Auctioned Tile %d" , turnContext.getPlayer()->getId() , mUseTileId );
				}
			}

			if ( mUseTileId == FAIL_TILE_ID )
			{
				mUseTileId = drawPlayTile();
			}

			if ( mSetting->have( Rule::eDragon ) )
			{
				if ( countDrawTileLoop < getRemainingTileNum() )
				{
					TileSet const& tileSet = mTileSetManager.getTileSet( mUseTileId );

					if ( CheckTileContent( tileSet , BIT(TileContent::eTheDragon) ) &&
						 mDragon->mapTile == nullptr )
					{
						reserveTile( mUseTileId );
						continue;
					}
				}
			}
			
			if ( mSetting->have( Rule::eUseHill ) )
			{
				if ( getRemainingTileNum() > 0 )
				{
					hillTileId = drawPlayTile();
				}
			}

			TileSet const& tileSet = mTileSetManager.getTileSet(mUseTileId);

			PlaceTileParam param;
			param.canUseBridge = 0;
			param.checkRiverConnect = 1;
			int numPos = updatePosibleLinkPos(param);
			if (numPos)
			{
				if (mSetting->have(Rule::eShrine))
				{
					assert(tileSet.numPieces == 1);
					TilePiece& piece = tileSet.tiles[0];
					if ( piece.contentFlag & ( BIT(TileContent::eCloister) | BIT(TileContent::eShrine)) )
					{

						for ( auto iter = mPlaceTilePosList.begin(); iter != mPlaceTilePosList.end(); )
						{
							Vec2i pos = *iter;
							unsigned neighborCotent = 0;
							int cloisterLikeCount = 0;
							int shrineLikeCount = 0;
							for (int i = 0; i < FDir::NeighborNum; ++i)
							{
								MapTile* mapTile = getWorld().findMapTile(pos + FDir::NeighborOffset(i));
								if (mapTile)
								{
									if (mapTile->have(TileContent::eCloister))
										++cloisterLikeCount;
									if (mapTile->have(TileContent::eShrine))
										++shrineLikeCount;
								}
							}

							++iter;
						}
					}
				}
			}

			if ( numPos == 0 )
			{


				if( tileSet.group == TileSet::eRiver )
				{
					continue;
				}

				if( mSetting->have(Rule::eBridge) )
				{
					param.canUseBridge = 1;
					if ( updatePosibleLinkPos( param ) != 0 )
					{
						//#TODO
						continue;
					}
				}
				if( getRemainingTileNum() == 0 )
				{
					turnContext.result = TurnStatus::eFinishGame;
					return;
				}
					
			}
			break;
		}

		haveHillTile = ( hillTileId != FAIL_TILE_ID );
	}

	void GameLogic::resolvePlaceTile(TurnContext& turnContext, MapTile* placeMapTiles[] , int& numMapTile )
	{
		for(;;)
		{
			//a)  Place the tile. You must build a bridge if doing so is required to make the tile placement legal.
			//   (Otherwise, a bridge can be placed at any time during the turn.)
			//   You may build a bridge on the tile just placed or a tile orthogonal to the tile just placed.
			GamePlaceTileData putTileData;
			putTileData.id = mUseTileId;
			CHECK_INPUT_RESULT(turnContext.getInput().requestPlaceTile( putTileData ) );

			if ( putTileData.resultRotation < 0 || putTileData.resultRotation >= FDir::TotalNum )
			{
				CAR_LOG("Warning: Place Tile have error rotation %d" , putTileData.resultRotation );
				putTileData.resultRotation = 0;
			}

			//#TODO

			PlaceTileParam param;
			param.canUseBridge = 0;
			param.checkRiverConnect = 1;
			numMapTile = mWorld.placeTile( mUseTileId , putTileData.resultPos , putTileData.resultRotation , param , placeMapTiles );
			if ( numMapTile != 0 )
			{
				CAR_LOG("Player %d Place Tile %d rotation=%d" , turnContext.getPlayer()->getId() , mUseTileId , putTileData.resultRotation );
				mListener->notifyPlaceTiles( mUseTileId , placeMapTiles , numMapTile );
				break;
			}
		}
	}

	void GameLogic::resolveDeployActor(TurnContext& turnContext, MapTile* deployMapTiles[], int numDeployTile, unsigned actorMask, bool haveUsePortal, unsigned ignoreFeatureMask, bool& haveDone)
	{
		//a) Deploy a follower(forbidden if volcano or plague source was played)
		//	- Normal follower
		//	- Big follower
		//	- Mayor
		//	- Wagon
		//	- Abbot(C II)
		//b) Deploy one of your other figures(forbidden if volcano was played)
		//	(unknown if allowed after playing plague source)
		//	- Pig to a field containing one of your followers
		//	- Builder to a city or road containing one of your followers
		//	- Shepherd to a field not already containing a shepherd
		//		o You must perform an expand flock action and resolve the result
		//		o MESSAGES ROBBERS
		//c) Deploy the barn.The farm will be scored as a normal feature in Step 7.
		calcPlayerDeployActorPos(*turnContext.getPlayer(), deployMapTiles , numDeployTile , actorMask , haveUsePortal, ignoreFeatureMask );

		GameDeployActorData deployActorData;
		CHECK_INPUT_RESULT(turnContext.getInput().requestDeployActor( deployActorData ) );

		if ( deployActorData.bSkipActionForResult == true )
			return;

		if ( deployActorData.resultIndex < 0 && deployActorData.resultIndex >= mActorDeployPosList.size() )
		{
			CAR_LOG("Warning: Error Deploy Actor Index !!" );
			deployActorData.resultIndex = 0;
		}
		ActorPosInfo& info = mActorDeployPosList[ deployActorData.resultIndex ];

		FeatureBase* feature = getFeature( info.group );
		assert( feature );
		turnContext.getPlayer()->modifyActorValue( deployActorData.resultType , -1 );
		LevelActor* actor = createActor( deployActorData.resultType );

		assert( actor );
		info.mapTile->addActor( *actor );
		feature->addActor( *actor );
		actor->ownerId = turnContext.getPlayer()->getId();
		actor->pos     = info.pos;

		switch( actor->type )
		{
		case ActorType::ePig:
		case ActorType::eBuilder:
			{
				LevelActor* actorFollow = feature->findActor( BIT(turnContext.getPlayer()->getId() ) , mSetting->getFollowerMask() );
				if ( actor == nullptr )
				{
					CAR_LOG( "Error: Builder or Pig No Follower In Deploying" );
				}
				else
				{
					actorFollow->addFollower( *actor );
				}
			}
			break;

		}

		mListener->notifyDeployActor( *actor );
		CAR_LOG( "Player %d deploy Actor : type = %d  " , turnContext.getPlayer()->getId() , actor->type );

		switch( actor->type )
		{
		case ActorType::eShepherd:
			expandSheepFlock(actor);
			break;
		}

		haveDone = true;
	}

	void GameLogic::resolveCompletedFeature(TurnContext& turnContext, FeatureBase& completedFeature , FeatureScoreInfo& featureScore , CastleScoreInfo* castleScore )
	{
		assert( completedFeature.checkComplete() );

		//a) If a town was created by the tile placement, the town maybe converted into a castle by the occupying player.
		//   If converted, this feature's completion is considered to be resolved. Go to the next feature.
		if( mSetting->have(Rule::eCastleToken) )
		{
			if( completedFeature.type == FeatureType::eCity )
			{
				bool haveBuild;
				CHECK_RESOLVE_RESULT(resolveBuildCastle(turnContext, completedFeature, haveBuild));
				if( haveBuild )
					return;
			}
		}

		//b) If the Gingerbread Man is in a completed city, all players with knights in the city receive points.
		//   You then place the Gingerbread Man in an unfinished city of your choice. 
		//  (How this would interact with a newly-formed castle is unknown.As a castle is no longer a city, 
		//   the Gingerbread Man should likely be removed with no scoring.)


		//?)
		if( mSetting->have(Rule::eLaPorxada) && mbUseLaPorxadaScoring )
		{
			if( completedFeature.type == FeatureType::eCity )
			{
				CityFeature& city = static_cast<CityFeature&>(completedFeature);

				if( city.haveLaPorxada )
				{
					int iter = 0;
					while( LevelActor* actor = city.iteratorActorFromType(KINGHT_MASK, iter) )
					{
						if( actor->ownerId != CAR_ERROR_PLAYER_ID )
						{
							mPlayerManager->getPlayer(actor->ownerId)->setFieldValue(FieldType::eLaPorxadaFinishScoring, 1);
						}
					}
				}
			}
		}

		//c) If a fairy is next to a follower in the completed feature, that follower¡¦s owner receives 3 points.
		if ( mSetting->have(Rule::eFariy) )
		{
			LevelActor* actor = mFairy->binder;
			if ( actor && actor->feature == &completedFeature )
			{
				CAR_LOG("Score: %d FairyFearureScoringScore" , actor->ownerId );
				modifyPlayerScore( actor->ownerId , CAR_PARAM_VALUE(FairyFearureScoringScore) );
			}
		}
		//d) Rewards for completing the featur
		//  - Collect any trade good tokens
		if ( completedFeature.type == FeatureType::eCity )
		{
			CityFeature& city = static_cast< CityFeature& >( completedFeature );
			if ( mSetting->have( Rule::eTraders ) )
			{
				for(SideNode* node : city.nodes)
				{
					MapTile const* mapTile = node->getMapTile();
					SideContentType content = mapTile->getSideContnet( node->index );
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
		//  - If the completed feature is a city or road, check to see if it is the new largest one and, if so, 
		//    receive the King or Robber Baron
		if ( mSetting->have( Rule::eKingAndRobber ) )
		{
			if ( completedFeature.type == FeatureType::eCity && castleScore == nullptr )
			{
				if ( completedFeature.mapTiles.size() > mMaxCityTileNum )
				{
					mMaxCityTileNum = completedFeature.mapTiles.size();
					mIdKing = getTurnPlayer()->getId();
				}
			}
			else if ( completedFeature.type == FeatureType::eRoad )
			{

				if ( completedFeature.mapTiles.size() > mMaxRoadTileNum )
				{
					mMaxCityTileNum = completedFeature.mapTiles.size();
					mIdRobberBaron = getTurnPlayer()->getId();
				}
			}
		}
		//e) The player to your left may move one or more followers from the City of Carcassonne to the current feature. 
		//   Everyone has this opportunity in turn,ending with you.
		
		
		
		int numController = 0;
		if ( castleScore )
		{
			FeatureControllerScoreInfo info;
			info.playerId = completedFeature.findActorFromType( KINGHT_MASK )->ownerId;
			info.majority = 1;
			info.score = castleScore->value;
			featureScore.controllerScores.push_back( info );
			featureScore.numController = 1;
		}
		else
		{
			//f)  Resolve control of the completed feature
			numController = completedFeature.calcScore( *mPlayerManager , featureScore );
		}


		if ( featureScore.numController > 0 )
		{
			//g) If there is at least one gold piece on a tile with the completed structure, 
			//   gold pieces are distributed to the controlling player(s)
			if( mSetting->have(Rule::eGold) )
			{
				int numGoldPieces = 0;
				switch( completedFeature.type )
				{
				case FeatureType::eCity:
				case FeatureType::eRoad:
					for( auto mapTile : completedFeature.mapTiles )
					{
						if( mapTile->goldPices )
						{
							numGoldPieces += mapTile->goldPices;
						}
					}
					break;

				case FeatureType::eCloister:
				case FeatureType::eGarden:
				case FeatureType::eChrine:
					for( auto mapTile : completedFeature.mapTiles )
					{
						if( mapTile->goldPices )
						{
							numGoldPieces += mapTile->goldPices;
						}
					}
					for( auto mapTile : static_cast<AdjacentTileScoringFeature&>(completedFeature).neighborTiles )
					{
						if( mapTile->goldPices )
						{
							numGoldPieces += mapTile->goldPices;
						}
					}
					break;
				}
			}
			//h) Tally points and award points to the controlling player(s) ROBBER CHOICE
			if ( mSetting->have( Rule::eHaveGermanCastleTile ) &&
				 ( completedFeature.type == FeatureType::eCity || completedFeature.type == FeatureType::eRoad ) )
			{
				int numNeighborCastle = 0;
				for(GermanCastleFeature* castle : mGermanCastles)
				{
					MapTile const* tileA = *castle->mapTiles.begin();
					MapTile const* tileB = *(++castle->mapTiles.begin());
					Vec2i min , max;
					bool beH = tileA->pos.x == tileB->pos.x;
					if ( beH )
					{
						min.y = tileA->pos.y;
						max.y = tileA->pos.y;
						min.x = tileA->pos.x;
						max.x = tileB->pos.x;
						if ( min.x > max.x )
							std::swap( min.x , max.x );
					}
					else
					{
						min.x = tileA->pos.x;
						max.x = tileA->pos.x;
						min.y = tileA->pos.y;
						max.y = tileB->pos.y;
						if ( min.y > max.y )
							std::swap( min.y , max.y );
					}
					if ( completedFeature.testInRectArea( min , max ) )
					{
						++numNeighborCastle;
					}
				}

				if ( numNeighborCastle )
				{
					for( int i = 0 ; i < numController ; ++i )
					{
						featureScore.controllerScores[i].score += numNeighborCastle * CAR_PARAM_VALUE(GermanCastleAdditionFactor);
					}
				}
			}

			//i) If a player has the teacher, that player scores the same number of points that were awarded. 
			//   (If more than one player is receiving points simultaneously, the holder of the teacher can choose which points to receive.) 
			//   The teacher is then returned to the school. ROBBER CHOICE
			if( mSetting->have(Rule::eTeacher) )
			{
				if( mTecher->ownerId != CAR_ERROR_PLAYER_ID )
				{
					if( featureScore.numController == 1 )
					{

					}
					else
					{


					}
				}
			}


			//j)  Remove the mage or witch if they were involved in the scoring of this feature.
			if( mSetting->have(Rule::eMageAndWitch) )
			{
				assert(mMage->feature == nullptr || mMage->feature != mWitch->feature);
				if( mMage->feature == &completedFeature )
				{
					int numTiles = completedFeature.mapTiles.size();
					for( auto& info : featureScore.controllerScores )
					{
						info.score += numTiles;
					}
					removeActor(mMage);
				}
				else if( mWitch->feature == &completedFeature )
				{
					for( auto& info : featureScore.controllerScores )
					{
						info.score = ( info.score + 1 ) / 2;
					}
					removeActor(mWitch);
				}
			}

			//k) If a heretic or monk completes its feature, determine if a ¡§race to completion¡¨ has been won.
			//   If so, return the losing follower to its owner.
			//l) Move any wagons on the completed feature to any adjoining unoccupied uncompleted feature.
			//   If more than one wagon can move, you move first, and then order of movement proceeds clockwise.
			//m) Return all remaining followers on the completed feature to their owners.
			CHECK_RESOLVE_RESULT(resolveFeatureReturnPlayerActor(turnContext, completedFeature));
		}


		if ( mSetting->have(Rule::eCastleToken) )
		{
			int score = 0;
			if ( featureScore.numController > 0 )
			{
				for( int i = 0 ; i < featureScore.numController; ++i )
				{
					if ( score < featureScore.controllerScores[i].score )
						score = featureScore.controllerScores[i].score;
				}
			}
			else
			{
				score = completedFeature.calcPlayerScore( nullptr );
			}

			checkCastleComplete( completedFeature , score );
		}
	}

	void GameLogic::resolveCompletedCastles(TurnContext& turnContext, std::vector< FeatureScoreInfo >& featureScoreList)
	{
		std::vector< CastleInfo* > castleGroup;
		do
		{
			castleGroup.clear();

			for( CastleInfoList::iterator iter = mCastles.begin() , itEnd = mCastles.end() ;
				iter != itEnd ; ++iter )
			{
				CastleInfo* info = *iter;
				if ( info->featureScores.empty() == true )
					continue;

				castleGroup.push_back( info );
			}

			for(CastleInfo* info : castleGroup)
			{
				CastleScoreInfo* scoreInfo;
				if ( info->featureScores.size() == 1 )
				{
					scoreInfo = &info->featureScores[0];
				}
				else
				{
					//#TODO : Need Request
					scoreInfo = &info->featureScores[0];
					for( int i = 1 ; i < info->featureScores.size() ; ++i )
					{
						if ( scoreInfo->value < info->featureScores[i].value )
							scoreInfo = &info->featureScores[i];
					}
				}

				FeatureScoreInfo featureScore;
				featureScore.feature = info->city;
				CHECK_RESOLVE_RESULT( resolveCompletedFeature(turnContext, *info->city , featureScore , scoreInfo ) );
				if( featureScore.numController )
				{
					featureScoreList.push_back(std::move(featureScore));
				}

				delete info;
			}
		}
		while( castleGroup.empty() == false );
	}

	void GameLogic::resolveDragonMove(TurnContext& turnContext, LevelActor& dragon)
	{
		assert( dragon.mapTile != nullptr );


		std::vector< MapTile* > moveTilesBefore;
		moveTilesBefore.resize(CAR_PARAM_VALUE(DragonMoveStepNum));

		MapTile* moveTiles[FDir::TotalNum];
		GameDragonMoveData data;
		data.mapTiles = moveTiles;
		data.reason = SAR_MOVE_DRAGON;
		data.bCanSkip = false;

		int idxPlayer = mIdxPlayerTrun;

		for( int step = 0 ; step < CAR_PARAM_VALUE(DragonMoveStepNum) ; ++step )
		{
			int numTile = 0;
			
			for( int i = 0 ; i < FDir::TotalNum ; ++i )
			{
				Vec2i pos = FDir::LinkPos( dragon.mapTile->pos , i );
				MapTile* mapTile = mWorld.findMapTile( pos );
				if ( mapTile == nullptr )
					continue;

				//#TODO: castle school city of car. , wheel of fea.
				if ( mSetting->have( Rule::eFariy ) && mFairy->mapTile == mapTile )
					continue;
				

				//check prev move
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
			data.playerId = mOrderedPlayers[ idxPlayer ]->getId();
			
			CHECK_INPUT_RESULT(turnContext.getInput().requestSelectMapTile(data) );

			if ( data.checkResult() == false )
			{
				CAR_LOG("Warning: Error Dragon Move Index %d" , data.resultIndex );
				data.resultIndex = 0;
			}

			moveTilesBefore[ step ] = dragon.mapTile;
			moveActor( &dragon , ActorPos::Tile() , data.mapTiles[ data.resultIndex ] );
				
			for(LevelActor* actor : dragon.mapTile->mActors)
			{
				bool bDrageCanEat = ( BIT( actor->type ) & DRAGON_EAT_ACTOR_TYPE_MASK ) != 0;

				if ( bDrageCanEat == false )
					continue;

				if ( mSetting->have( Rule::eShepherdAndSheep ) && actor->type == ActorType::eShepherd )
				{
					ShepherdActor* shepherd = static_cast< ShepherdActor* >( actor );
					for(auto token : shepherd->ownSheep)
					{
						mSheepBags.push_back(token);
					}
				}
				//#TODO
				if ( actor->binder )
				{

				}
				returnActorToPlayer( actor );
			}

			++idxPlayer;
			if ( idxPlayer >= mPlayerManager->getPlayerNum() )
				idxPlayer = 0;
		}
	}

	void GameLogic::resolvePortalUse(TurnContext& turnContext, MapTile*& outDeployMapTile)
	{
		int playerId = turnContext.getPlayer()->getId();

		std::set< MapTile* > mapTileSet;
		for(FeatureBase* feature : mFeatureMap)
		{
			if ( feature->group == ERROR_GROUP_ID )
				continue;
			if ( feature->checkComplete() )
				continue;
			if ( feature->haveActorFromType( mSetting->getFollowerMask() ) )
				continue;

			for (MapTile* mapTile : feature->mapTiles)
			{
				if ( !mapTile->canDeployFollower() )
					continue;
				mapTileSet.insert( mapTile );
			}
		}

		if ( mapTileSet.empty() == false )
		{
			std::vector< MapTile* > mapTiles( mapTileSet.begin() , mapTileSet.end() );

			GameSelectMapTileData data;
			data.reason = SAR_MAGIC_PORTAL;
			data.bCanSkip = false;
			data.mapTiles = &mapTiles[0];
			data.numSelection = mapTiles.size();
			CHECK_INPUT_RESULT( turnContext.getInput().requestSelectMapTile( data ) );

			if( !data.checkResult() )
			{
				CAR_LOG("Warning! Error Portal Result Index Tile!");
				data.resultIndex = 0;
			}

			outDeployMapTile = data.mapTiles[ data.resultIndex ];
		}
		else
		{
			outDeployMapTile = nullptr;
		}
	}

	void GameLogic::resolveeAircraftUse(TurnContext& turnContext, MapTile* aircraftTile, MapTile*& outDeployMapTile)
	{
		Vec2i offsetFlyDir = aircraftTile->getAircaftDirOffset();


	}

	void GameLogic::resolveExpendShepherdFarm(TurnContext& turnContext, FeatureBase* feature)
	{
		assert( feature->type == FeatureType::eFarm );

		LevelActor* actor = feature->findActor( BIT(turnContext.getPlayer()->getId()) , BIT( ActorType::eShepherd ) );
		if ( actor == nullptr )
			return;

		GameSelectActionOptionData data;
		data.options.push_back( ACTOPT_SHEPHERD_EXPAND_THE_FLOCK );
		data.options.push_back( ACTOPT_SHEPHERD_HERD_THE_FLOCK_INTO_THE_STABLE );
		CHECK_INPUT_RESULT(turnContext.getInput().requestSelectActionOption( data ) );
		data.validateResult("ExpendShepherdFarm");

		switch( data.options[ data.resultIndex ] )
		{
		case ACTOPT_SHEPHERD_EXPAND_THE_FLOCK:
			expandSheepFlock( actor );
			break;
		case ACTOPT_SHEPHERD_HERD_THE_FLOCK_INTO_THE_STABLE:
			{
				int score = 0;
				std::vector< ShepherdActor* > shepherds;
				int iter = 0;
				while( ShepherdActor* shepherd = static_cast< ShepherdActor* >( feature->iteratorActor( AllPlayerMask , BIT( ActorType::eShepherd ) , iter ) ) )
				{
					for( auto token : shepherd->ownSheep )
					{
						score += token;
						mSheepBags.push_back(token);
					}
					shepherds.push_back( shepherd );
				}
				for(ShepherdActor* shepherd : shepherds)
				{
					PlayerId ownerId = shepherd->ownerId;
					returnActorToPlayer(shepherd);
					CHECK_RESOLVE_RESULT( resolveScorePlayer(turnContext, ownerId, score) );
				}
			}
			break;
		}
	}

	void GameLogic::resolveTower(TurnContext& turnContext, bool& haveDone )
	{
		if ( turnContext.getPlayer()->getFieldValue( FieldType::eTowerPices ) == 0 || mTowerTiles.empty() )
			return;

		GameSelectMapTileData data;
		data.bCanSkip = true;
		data.reason = SAR_CONSTRUCT_TOWER;
		data.numSelection = mTowerTiles.size();
		data.mapTiles   = &mTowerTiles[0];
		CHECK_INPUT_RESULT(turnContext.getInput().requestSelectMapTile( data ) );

		if ( data.bSkipActionForResult == true )
			return;

		if ( data.checkResult() == false )
		{
			CAR_LOG("Warning: Error ContructTower Index" );
			data.resultIndex = 0;
		}

		MapTile* mapTile = mTowerTiles[ data.resultIndex ];
		mapTile->towerHeight += 1;
		turnContext.getPlayer()->modifyFieldValue( FieldType::eTowerPices , -1 );

		haveDone = true;
		mListener->notifyConstructTower( *mapTile );

		std::vector< LevelActor* > actors;
		for( int i = 0 ; i <= mapTile->towerHeight ; ++i )
		{
			int num = ( i == 0 ) ? 1 : FDir::TotalNum;
			for( int dir = 0 ; dir < num ; ++dir )
			{
				Vec2i posCheck = mapTile->pos + i * FDir::LinkOffset( dir );

				MapTile* tileCheck = mWorld.findMapTile( posCheck );
				if ( tileCheck == nullptr )
					continue;

				for( LevelActor * actor : tileCheck->mActors)
				{
					if ( mSetting->isFollower( actor->type ) == false )
						continue;

					if ( actor->ownerId == turnContext.getPlayer()->getId() )
						continue;

					if ( actor->feature->type == FeatureType::eCity &&
						 static_cast< CityFeature* >( actor->feature )->isCastle )
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
			CHECK_INPUT_RESULT(turnContext.getInput().requestSelectActor( selectActorData ) );

			if ( selectActorData.bSkipActionForResult == true )
				return;

			if ( selectActorData.checkResult() == false )
			{
				CAR_LOG("Warning: Error Tower Capture follower Index" );
				selectActorData.resultIndex = 0;
			}

			LevelActor* actor = actors[selectActorData.resultIndex];
			PrisonerInfo info;
			info.playerId  = actor->ownerId;
			info.type = actor->type;
			info.ownerId = turnContext.getPlayer()->getId();

			std::vector< ActorInfo > actorInfos;
			for(auto & prisoner : mPrisoners)
			{
				if ( prisoner.playerId == info.ownerId && 
					 prisoner.ownerId == info.playerId )
				{
					actorInfos.push_back( prisoner );
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
					data.bCanSkip = false;
					data.actorInfos = &actorInfos[0];
					data.numSelection = actorInfos.size();
					CHECK_INPUT_RESULT(turnContext.getInput().requestSelectActorInfo( data ) );
					if ( data.checkResult() == false )
					{
						CAR_LOG("Warning: Error Prisoner exchange Index" );
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
				turnContext.getPlayer()->modifyActorValue( infoExchange.type , 1 );
				mPrisoners.erase( mPrisoners.begin() + idxExchange );
				CAR_LOG("Exchange Prisoners [%d %d] <-> [%d %d]" , info.playerId , info.type , infoExchange.playerId , infoExchange.type );

			}
		}
	}


	void GameLogic::resolveMoveFairyToNextFollower(TurnContext& turnContext, bool& haveDone)
	{
		ActorList followers;
		int numFollower = getFollowers( BIT( turnContext.getPlayer()->getId() ) , followers , mFairy->binder );
		if( numFollower == 0 )
			return;

		GameSelectActorData data;
		data.bCanSkip = true;
		data.reason = SAR_FAIRY_MOVE_NEXT_TO_FOLLOWER;
		data.actors = &followers[0];
		data.numSelection = numFollower;
		CHECK_INPUT_RESULT(turnContext.getInput().requestSelectActor( data ) );
		if ( data.bSkipActionForResult == true )
			return;

		if ( data.checkResult() == false )
		{
			CAR_LOG( "Warning: Error Fairy Select Actor Index !" );
			data.resultIndex = 0;
		}
		LevelActor* actor = data.actors[ data.resultIndex ];
		actor->addFollower( *mFairy );
		moveActor( mFairy , ActorPos::Tile() , actor->mapTile );

		haveDone = true;
	}

	void GameLogic::resolveAuction(TurnContext& turnContext)
	{
		if ( getRemainingTileNum() < mOrderedPlayers.size() )
			return;

		for(auto player : mOrderedPlayers)
		{
			if(player->getFieldValue(FieldType::eTileIdAuctioned) != FAIL_TILE_ID )
				return;
		}

		GameAuctionTileData data;
		data.bCanSkip = true;
		for( int i = 0 ; i < mOrderedPlayers.size() ; ++i )
		{
			data.auctionTiles.push_back( drawPlayTile() );
		}

		int idxRound = turnContext.getPlayer()->mPlayOrder;
		for( int i = 0 ; i < mOrderedPlayers.size() ; ++i )
		{
			do 
			{
				++idxRound;
				if ( idxRound >= mOrderedPlayers.size() )
					idxRound = 0;
			} 
			while ( mOrderedPlayers[ idxRound ]->getFieldValue( FieldType::eTileIdAuctioned ) != FAIL_TILE_ID );

			if ( data.auctionTiles.size() == 1 )
			{
				mOrderedPlayers[ idxRound ]->setFieldValue( FieldType::eTileIdAuctioned , data.auctionTiles[0] );
			}
			else
			{
				data.pIdRound = mOrderedPlayers[ idxRound ]->getId();
				data.maxScore = 0;
				data.pIdCallMaxScore = -1;
				data.tileIdRound = FAIL_TILE_ID;

				int idxCur = idxRound;
				for( int n = 0 ; n < mOrderedPlayers.size() ; ++n )
				{
					if ( mOrderedPlayers[ idxCur ]->getFieldValue( FieldType::eTileIdAuctioned ) == FAIL_TILE_ID )
					{
						data.playerId = mOrderedPlayers[ idxCur ]->getId();
						data.resultRiseScore = 0;

						CHECK_INPUT_RESULT(turnContext.getInput().requestAuctionTile( data ) );

						if (  data.playerId == data.pIdRound )
						{
							if ( data.resultIndexTileSelect >= data.auctionTiles.size()  )
							{
								CAR_LOG("Warning");
								data.resultIndexTileSelect = 0;
							}
							data.tileIdRound = data.auctionTiles[ data.resultIndexTileSelect ];
							data.maxScore = data.resultRiseScore;
							data.pIdCallMaxScore = data.playerId;

							data.auctionTiles.erase( data.auctionTiles.begin() + data.resultIndexTileSelect );
						}
						else if ( data.resultRiseScore != 0 )
						{
							data.maxScore += data.resultRiseScore;
							data.pIdCallMaxScore = data.playerId;
						}
					}

					++idxCur;
					if ( idxCur >= mOrderedPlayers.size() )
						idxCur = 0;
				}

				bool haveBuy = true;
				if ( data.pIdCallMaxScore != data.pIdRound )
				{
					data.playerId = data.pIdRound;
					CHECK_INPUT_RESULT(turnContext.getInput().requestBuyAuctionedTile( data ) );
					haveBuy = !data.bSkipActionForResult;
				}

				PlayerBase* pBuyer;
				PlayerBase* pSeller;
				if ( haveBuy )
				{
					pBuyer = mPlayerManager->getPlayer( data.pIdRound );
					pSeller = mPlayerManager->getPlayer( data.pIdCallMaxScore );
				}
				else
				{
					pSeller = mPlayerManager->getPlayer( data.pIdRound );
					pBuyer = mPlayerManager->getPlayer( data.pIdCallMaxScore );
				}

				if ( pBuyer != pSeller )
				{
					modifyPlayerScore( pSeller->getId() , data.maxScore );
				}
				modifyPlayerScore( pBuyer->getId() , -data.maxScore );
				pBuyer->setFieldValue( FieldType::eTileIdAuctioned , data.tileIdRound );
			}
		}
	}


	void GameLogic::resolveFeatureReturnPlayerActor(TurnContext& turnContext, FeatureBase& feature)
	{
		//LevelActor* actor;
		std::vector< LevelActor* > wagonGroup;
		std::vector< LevelActor* > mageWitchGroup;
		std::vector< LevelActor* > hereticMonkGroup;
		std::vector< LevelActor* > otherGroup;
		for( LevelActor * actor : feature.mActors)
		{
			PlayerBase* player = getOwnedPlayer(actor);
			if ( player  )
			{
				switch( actor->type )
				{
				case ActorType::eWagon: 
					wagonGroup.push_back( actor );
					break;
				case ActorType::eMage:
				case ActorType::eWitch:
					mageWitchGroup.push_back(actor);
					break;
				default:
					otherGroup.push_back( actor );
				}
			}
			else
			{
				//#TODO: 
			}
		}
		//l)

		if ( mSetting->have( Rule::eWagon ) && !wagonGroup.empty() )
		{
			GroupSet linkFeatureGroups;
			feature.generateRoadLinkFeatures( mWorld , linkFeatureGroups );

			std::vector< FeatureBase* > linkFeatures;
			for( auto group : linkFeatureGroups )
			{
				FeatureBase* featureLink = getFeature( group );
				if ( featureLink->checkComplete() )
					continue;
				if ( featureLink->haveActor( AllPlayerMask , mSetting->getFollowerMask() ) )
					continue;
				linkFeatures.push_back( featureLink );
			}

			if ( linkFeatures.empty() == false )
			{
				struct WagonCompFun
				{
					bool operator()(LevelActor* lhs, LevelActor* rhs) const
					{
						return calcOrder(lhs) < calcOrder(rhs);
					}
					int calcOrder(LevelActor* actor) const
					{
						PlayerBase* player = manager->getPlayer(actor->ownerId);
						int result = player->mPlayOrder - curPlayerOrder;
						if( result < 0 )
							result += numPlayer;
						return result;
					}

					GamePlayerManager* manager;
					int curPlayerOrder;
					int numPlayer;
				};

				WagonCompFun wagonFun;
				wagonFun.curPlayerOrder = getTurnPlayer()->mPlayOrder;
				wagonFun.numPlayer = mPlayerManager->getPlayerNum();
				wagonFun.manager = mPlayerManager;
				std::sort( wagonGroup.begin() , wagonGroup.end() , wagonFun );

				std::vector< MapTile* > mapTiles;
				GameFeatureTileSelectData data;
				data.bCanSkip = true;
				data.reason = SAR_WAGON_MOVE_TO_FEATURE;
				for(auto wagon : wagonGroup)
				{
					data.infos.clear();
					mapTiles.clear();
					data.fill(linkFeatures, mapTiles, true);
					CHECK_INPUT_RESULT(turnContext.getInput().requestSelectMapTile( data ) );

					if ( data.bSkipActionForResult == true )
					{
						returnActorToPlayer(wagon);
					}
					else 
					{
						data.validateResult("WagonMoveTileMap");

						FeatureBase* selectedFeature = data.getResultFeature();
						ActorPos actorPos;
						bool isOK = selectedFeature->getActorPos( *mapTiles[ data.resultIndex ] , actorPos );
						assert( isOK );
						moveActor(wagon , actorPos , mapTiles[data.resultIndex] );
						selectedFeature->addActor( *wagon );
						linkFeatures.erase( std::find( linkFeatures.begin() , linkFeatures.end() , selectedFeature ) );
					}
				}
			}
			else
			{
				for(auto* actor : wagonGroup)
				{
					returnActorToPlayer(actor);
				}
			}
		}

		//m)
		for(auto* actor : otherGroup)
		{
			returnActorToPlayer(actor);
		}
	}

	void GameLogic::resolveLaPorxadaCommand(TurnContext& turnContext)
	{
		GameSelectActionOptionData data;
		data.group = ACTOPT_GROUP_LA_PORXADA;
		data.options = { ACTOPT_LA_PORXADA_EXCHANGE_FOLLOWER_POS , ACTOPT_LA_PORXADA_SCORING };
		CHECK_INPUT_RESULT(turnContext.getInput().requestSelectActionOption(data) );
		data.validateResult("ChoiceLaPorxadaCommand");

		switch( data.options[ data.resultIndex ] )
		{
		case ACTOPT_LA_PORXADA_EXCHANGE_FOLLOWER_POS:
			{
				std::vector< LevelActor* > selfFollowers;
				std::vector< LevelActor* > otherFollowers;

				int iter = 0;
				while ( auto actor = iteratorActorFromType( mSetting->getFollowerMask() , iter ) )
				{
					if (actor->ownerId == turnContext.getPlayer()->getId())
					{
						otherFollowers.push_back(actor);
					}
					else
					{
						selfFollowers.push_back(actor);
					}
				}

				if( !otherFollowers.empty() && !selfFollowers.empty() )
				{
					LevelActor* selfActor;
					LevelActor* targetActor;
					//select self follower
					{
						GameSelectActorData data;
						data.reason = SAR_LA_PORXADA_SELF_FOLLOWER;
						data.numSelection = selfFollowers.size();
						data.actors = selfFollowers.data();
						data.bCanSkip = false;
						CHECK_INPUT_RESULT(turnContext.getInput().requestSelectActor(data) );
						data.validateResult("LaPorxadaCommand:SelectSelfFollower");
						selfActor = data.actors[data.resultIndex];
						assert(selfActor);
					}
					//select target follower
					{
						GameSelectActorData data;
						data.reason = SAR_LA_PORXADA_OTHER_PLAYER_FOLLOWER;
						data.numSelection = selfFollowers.size();
						data.actors = selfFollowers.data();
						data.bCanSkip = false;
						CHECK_INPUT_RESULT(turnContext.getInput().requestSelectActor(data) );
						data.validateResult("LaPorxadaCommand:SelectTargetFollower");
						targetActor = data.actors[data.resultIndex];
						assert(targetActor);
					}
					//request exchange

					{
						GameExchangeActorPosData data;
						data.selfActor = selfActor;
						data.targetActor = targetActor;
						CHECK_INPUT_RESULT(turnContext.getInput().requestExchangeActorPos(data) );

						if( data.resultActorType == ActorType::eNone )
						{
							selfActor->pos;
						}
						else
						{

						}
					}
				}
			}
			break;
		case ACTOPT_LA_PORXADA_SCORING:
			{
				mbUseLaPorxadaScoring = true;
			}
			break;
		}
	}

	void GameLogic::resolveMoveMageOrWitch(TurnContext& turnContext)
	{

		LevelActor* targetActor;
		LevelActor* skipActor;
		{
			GameSelectActionOptionData data;
			data.group = ACTOPT_GROUP_MOVE_MAGE_OR_WITCH;
			data.options = { ACTOPT_MOVE_MAGE , ACTOPT_MOVE_WITCH };
			data.bCanSkip = false;
			CHECK_INPUT_RESULT(turnContext.getInput().requestSelectActionOption(data));
			data.validateResult("SelectMageOrWitch");

			if( data.resultIndex == 0 )
			{
				targetActor = mMage;
				skipActor = mWitch;
			}
			else
			{
				targetActor = mWitch;
				skipActor = mMage;
			}
		}

		std::vector< FeatureBase* > destFeatures;
		for( auto feature : mFeatureMap )
		{
			if( feature->group == ERROR_GROUP_ID )
				continue;

			if ( skipActor->feature == feature )
				continue;

			if( feature->type != FeatureType::eCity && feature->type != FeatureType::eRoad )
				continue;

			if( feature->checkComplete() )
				continue;

			destFeatures.push_back(feature);
		}

		if( destFeatures.empty() )
		{
			CAR_LOG("Mage/Witch No Target can move , need remove");

			if( targetActor->feature )
			{
				removeActor(targetActor);
			}
			return; 
		}

		{
			GameFeatureTileSelectData data;
			std::vector< MapTile* > selectedTitles;
			data.fill(destFeatures, selectedTitles, false);
			assert(!selectedTitles.empty());
			data.bCanSkip = false;
			data.validateResult("MoveMageOrWitch:SelectTile");
			moveActor(targetActor, ActorPos::Tile(), selectedTitles[data.resultIndex]);
		}
	}

	void GameLogic::resolvePlaceGoldPieces(TurnContext& turnContext, MapTile* mapTile)
	{
		assert(mapTile->have(TileContent::eGold));

		mapTile->goldPices += 1;

		std::vector< MapTile* > neighborTiles;
		for ( auto const& offset : gAdjacentOffset)
		{
			Vec2i neighborPos = mapTile->pos + offset;
			MapTile* neighborTile = mWorld.findMapTile(neighborPos);
			if( neighborTile )
			{
				neighborTiles.push_back(neighborTile);
			}
		}

		if( !neighborTiles.empty() )
		{
			GameSelectMapTileData data;
			data.reason = SAR_PLACE_GOLD_PIECES;
			data.mapTiles = &neighborTiles[0];
			data.numSelection = neighborTiles.size();
			data.bCanSkip = false;
			CHECK_INPUT_RESULT(turnContext.getInput().requestSelectMapTile(data));
			data.validateResult("PlaceGoldPices");

			neighborTiles[data.resultIndex]->goldPices += 1;
		}
	}

	void GameLogic::resolveCropCircle(TurnContext& turnContext, FeatureType::Enum cropType)
	{
		AcionOption cropOption;
		{
			GameSelectCropCircleOptionData data;
			data.cropType = cropType;
			CHECK_INPUT_RESULT(turnContext.getInput().requestSelectActionOption(data));
			cropOption = data.options[data.resultIndex];
		}
		for( int i = 1; i <= mOrderedPlayers.size(); ++i )
		{
			PlayerBase* player = mOrderedPlayers[(mIdxPlayerTrun + i) % mOrderedPlayers.size()];

			switch( cropOption )
			{
			case ACTOPT_CROP_CIRCLE_DEPLOY_FOLLOWER:
				break;
			case ACTOPT_CROP_CIRCLE_REMOVE_FOLLOWER:
				break;
			}

		}
	}

	void GameLogic::resolvePrincess(TurnContext& turnContext, MapTile* placeMapTile, bool& haveDone)
	{
		for( auto iter = TBitMaskIterator< FDir::TotalNum >(TilePiece::AllSideMask); iter; ++iter )
		{
			int dir = iter.index;
			iter.mask &= ~placeMapTile->getSideLinkMask( dir );

			if ( ( placeMapTile->getSideContnet( dir ) & SideContent::ePrincess ) == 0 )
				continue;

			if (  placeMapTile->sideNodes[dir].group == ERROR_GROUP_ID )
			{
				CAR_LOG( "Warning: Tile id = %d Format Error for Princess" , placeMapTile->getId() );
				continue;
			}

			CityFeature* city = static_cast< CityFeature* >( getFeature( placeMapTile->sideNodes[dir].group ) );

			assert( city->type == FeatureType::eCity );

			int iterActor = 0;
			ActorList kinghts;
			while( LevelActor* actor = city->iteratorActor(AllPlayerMask, KINGHT_MASK, iterActor) )
			{
				kinghts.push_back( actor );
			}

			if ( ! kinghts.empty() )
			{
				GameSelectActorData data;
				data.bCanSkip = true;
				data.reason   = SAR_PRINCESS_REMOVE_KINGHT;
				data.numSelection = kinghts.size();
				data.actors   = &kinghts[0];

				CHECK_INPUT_RESULT(turnContext.getInput().requestSelectActor( data ) );

				if ( data.bSkipActionForResult == false )
				{
					if ( data.checkResult() == false )
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
	}

	void GameLogic::updateTileFeature( MapTile& mapTile , UpdateTileFeatureResult& updateResult )
	{
		//update Build
		{
			FeatureBase* sideFeatures[ FDir::TotalNum ];
			int numFeature = 0;

			for( auto iter = TBitMaskIterator< FDir::TotalNum >(TilePiece::AllSideMask); iter; ++iter )
			{
				int dir = iter.index;
				unsigned linkMask = mapTile.getSideLinkMask( dir );
				iter.mask &= ~linkMask;

				if( mapTile.getSideGroup(dir) != ERROR_GROUP_ID )
					continue;

				SideType linkType = mapTile.getLinkType( dir );

				FeatureBase* build = nullptr;
				
				bool bAbbeyUpdate = false;
				switch( linkType )
				{
				case SideType::eCity:
				case SideType::eRoad:
					{
						build = updateBasicSideFeature( mapTile , linkMask , linkType, updateResult );
					}
					break;
				case SideType::eInsideLink:
					{

					}
					break;
				case SideType::eAbbey:
					mapTile.sideNodes[dir].group = ABBEY_GROUP_ID;
					if ( mapTile.sideNodes[dir].outConnect )
					{
						int sideGroup = mapTile.sideNodes[dir].outConnect->group;
						if( sideGroup != ERROR_GROUP_ID )
						{
							SideFeature* sideFeature = static_cast< SideFeature* >( getFeature( sideGroup ) );
							sideFeature->addAbbeyNode( mapTile , dir );
							bAbbeyUpdate = true;
							build = sideFeature;
						}
					}
					break;
				case SideType::eEmptySide:
					break;
				}
				if ( build )
				{
					updateResult.addUpdateFeature( build , bAbbeyUpdate );
				}
			}

			for ( int dir = 0 ; dir < FDir::TotalNum ; ++dir )
			{
				if ( ( mapTile.getSideContnet( dir ) & SideContent::eHalfSeparate ) != 0 )
				{
					int dirA = ( dir + 1 ) % FDir::TotalNum;
					SideFeature* featureA = static_cast< SideFeature* >( getFeature( mapTile.sideNodes[ dirA ].group ) );
					featureA->halfSepareteCount += 1;
						
					int dirB = ( dir - 1 + FDir::TotalNum ) % FDir::TotalNum;
					SideFeature* featureB = static_cast< SideFeature* >( getFeature( mapTile.sideNodes[ dirB ].group ) );
					featureB->halfSepareteCount += 1;

					if ( mapTile.sideNodes[dir].outConnect )
					{
						//#TODO??
						int numMerged;
						FeatureBase* featureMerged[2];
						FeatureBase* featureNew = mergeHalfSeparateBasicSideFeature( mapTile , dir , featureMerged , numMerged );
						if ( numMerged )
						{
							for( int i = 0 ; i < numMerged ; ++i )
							{
								auto iterNew = std::find_if(updateResult.mUpdateFeatures.begin() , updateResult.mUpdateFeatures.end() , FindFeature( featureNew ) );
								auto iterMerged = std::find_if(updateResult.mUpdateFeatures.begin() , updateResult.mUpdateFeatures.end() , FindFeature( featureMerged[i] ) );

								if ( iterNew->bAbbeyUpdate )
									iterNew->bAbbeyUpdate = iterMerged->bAbbeyUpdate;
								updateResult.mUpdateFeatures.erase( iterMerged );
							}
						}

						updateResult.numSideFeatureMerged += numMerged;
					}
				}
			}
		}

		{
			for( auto iter = TBitMaskIterator< TilePiece::NumFarm >(TilePiece::AllFarmMask); iter; ++iter )
			{
				int idx = iter.index;
				unsigned linkMask = mapTile.getFarmLinkMask( idx );
				if ( linkMask == 0 )
					continue;

				iter.mask &= ~linkMask;

				if (mapTile.getFarmGroup(idx) != ERROR_GROUP_ID )
				{
					continue;
				}

				FarmFeature* farm = updateFarm( mapTile , linkMask );
				unsigned cityLinkMask = mapTile.getCityLinkFarmMask( idx );
				//unsigned cityLinkMaskCopy = cityLinkMask;
				for( auto iter2 = TBitMaskIterator< FDir::TotalNum >(cityLinkMask); iter2; ++iter2 )
				{
					int dir = iter2.index;
					if ( mapTile.getSideGroup( dir ) == -1 )
					{
						TileSet const& tileSet = mTileSetManager.getTileSet( mapTile.getId() );
						CAR_LOG("Error:Farm Side Mask Error Tile exp=%d index =%d" , (int)tileSet.expansion , tileSet.idxDefine );
						continue;
					}
					CityFeature* city = static_cast< CityFeature* >( getFeature( mapTile.getSideGroup( dir ) ) );
					assert( city->type == FeatureType::eCity );
					city->linkedFarms.insert( farm );
					farm->linkedCities.insert( city );
				}

			}
		}

		auto ProcAdjTilesFeatureInit = [&]( AdjacentTileScoringFeature* feature)
		{
			feature->addMapTile(mapTile);
			mapTile.group = feature->group;

			bool haveUpdate = false;
			for( int i = 0; i < FDir::NeighborNum; ++i )
			{
				Vec2i posCheck = mapTile.pos + FDir::NeighborOffset(i);
				MapTile* dataCheck = mWorld.findMapTile(posCheck);
				if( dataCheck )
				{
					haveUpdate |= feature->updateForAdjacentTile(*dataCheck);
				}
			}
			if( haveUpdate )
				updateResult.mUpdateFeatures.push_back(FeatureUpdateInfo(feature));
		};

		if ( mapTile.have(TileContent::eCloister) )
		{
			CloisterFeature* cloister = createFeatureT< CloisterFeature >();
			ProcAdjTilesFeatureInit(cloister);
		}
		if( mapTile.have(TileContent::eShrine) )
		{

		}
		if( mapTile.have(TileContent::eGarden) )
		{
			GardenFeature* garden = createFeatureT< GardenFeature >();
			ProcAdjTilesFeatureInit(garden);
		}

		for( int i = 0; i < FDir::NeighborNum ; ++i )
		{
			Vec2i posCheck = mapTile.pos + FDir::NeighborOffset(i);
			MapTile* dataCheck = mWorld.findMapTile( posCheck );
			if ( dataCheck && dataCheck->group != ERROR_GROUP_ID )
			{
				FeatureBase* feature = getFeature( dataCheck->group );
				if ( feature->updateForAdjacentTile( mapTile ) )
				{
					updateResult.mUpdateFeatures.push_back( FeatureUpdateInfo(feature) );
				}
			}
		}
	}

	int GameLogic::updatePosibleLinkPos(PlaceTileParam& param)
	{
		mPlaceTilePosList.clear();
		return mWorld.getPosibleLinkPos( mUseTileId , mPlaceTilePosList , param );
	}

	int GameLogic::updatePosibleLinkPos()
	{
		PlaceTileParam param;
		param.canUseBridge = 0;
		param.checkRiverConnect = 1;
		return updatePosibleLinkPos( param );
	}

	bool GameLogic::checkGameStatus( TurnStatus& result)
	{
		if ( mbNeedShutdown )
		{
			result = TurnStatus::eExitGame;
			return false;
		}
		if ( !mIsRunning )
		{
			result = TurnStatus::eFinishGame;
			return false;
		}
		return true;
	}

	void GameLogic::shuffleTiles(TileId* begin, TileId* end)
	{
		int num = end - begin;
		for( TileId* ptr = begin ; ptr != end ; ++ptr )
		{
			for ( int i = 0 ; i < num; ++i)
			{
				int idx = mRandom->getInt() % num;
				std::swap( *ptr , *(begin + idx) );
			}
		}
	}

	int GameLogic::getRemainingTileNum()
	{
		return mTilesQueue.size() - mIdxTile;
	}

	TileId GameLogic::drawPlayTile()
	{
		TileId result = mTilesQueue[ mIdxTile ];
		++mIdxTile;
		return result;
	}

	void GameLogic::calcPlayerDeployActorPos(PlayerBase& player, MapTile* deplyMapTiles[], int numDeployTile, unsigned actorMask, bool haveUsePortal, unsigned ignoreFeatureMask )
	{

		mActorDeployPosList.clear();
		for( int i = 0 ; i < numDeployTile ; ++i )
		{
			getActorPutInfo(player.getId(), *deplyMapTiles[i], haveUsePortal, ignoreFeatureMask, mActorDeployPosList);
		}

		int num = mActorDeployPosList.size();
		for( int i = 0; i < num; )
		{
			mActorDeployPosList[i].actorTypeMask &= actorMask;
			if( mActorDeployPosList[i].actorTypeMask == 0 )
			{
				--num;
				if( i != num )
				{
					std::swap(mActorDeployPosList[i], mActorDeployPosList[num]);
				}

			}
			else
			{
				++i;
			}
		}

		mActorDeployPosList.resize( num );
	}

	SideFeature* GameLogic::mergeHalfSeparateBasicSideFeature( MapTile& mapTile , int dir , FeatureBase* featureMerged[] , int& numMerged )
	{
		assert( ( mapTile.getSideContnet(dir) & SideContent::eHalfSeparate ) != 0 );

		numMerged = 0;

		SideFeature* feature = static_cast< SideFeature* >( getFeature( mapTile.sideNodes[dir].group ) );
		int dirA = ( dir + 1 ) % FDir::TotalNum;
		SideFeature* featureA = static_cast< SideFeature* >( getFeature( mapTile.sideNodes[ dirA ].group ) );
		assert( feature->type == featureA->type );
		featureA->halfSepareteCount -= 1;

		if ( ( mapTile.getSideContnet(dirA) & SideContent::eHalfSeparate ) == 0 ||
			   mapTile.sideNodes[dirA].outConnect != nullptr )
		{
			feature->mergeData( *featureA , mapTile , dir );
			destroyFeature( featureA );

			unsigned mask = mapTile.getSideLinkMask( dir ) | mapTile.getSideLinkMask( dirA );
			mapTile.updateSideLink( mask );

			featureMerged[numMerged++] = featureA;
		}

		int dirB = ( dir - 1 + FDir::TotalNum ) % FDir::TotalNum;
		SideFeature* featureB = static_cast< SideFeature* >( getFeature( mapTile.sideNodes[ dirB ].group ) );
		assert( feature->type == featureB->type );
		featureB->halfSepareteCount -= 1;

		if ( ( mapTile.getSideContnet(dirB) & SideContent::eHalfSeparate ) == 0 ||
			   mapTile.sideNodes[dirB].outConnect != nullptr )
		{
			if ( featureB != feature )
			{
				feature->mergeData( *featureB , mapTile , dir );
				destroyFeature( featureB );

				unsigned mask = mapTile.getSideLinkMask( dir ) | mapTile.getSideLinkMask( dirB );
				mapTile.updateSideLink( mask );

				featureMerged[numMerged++] = featureB;
			}
		}
		return feature;
	}

	FeatureBase* GameLogic::updateBasicSideFeature( MapTile& mapTile , unsigned dirLinkMask , SideType linkType , UpdateTileFeatureResult& updateResult )
	{
		int numLinkGroup = 0;
		int linkGroup[ TilePiece::NumSide ];
		SideNode* linkNodeFrist = nullptr;

		for( auto iter = TBitMaskIterator<FDir::TotalNum >( dirLinkMask ) ; iter ; ++iter )
		{
			int dir = iter.index;
			SideNode* linkNode = mapTile.sideNodes[ dir ].outConnect;

			if ( linkNode == nullptr || linkNode->group == ABBEY_GROUP_ID )
				continue;

			if ( linkNode->group == ERROR_GROUP_ID )
				continue;

			MapTile* linkTile = linkNode->getMapTile();
			int linkDir = FDir::Inverse( dir );
			if ( linkTile->getSideContnet( linkDir ) & SideContent::eHalfSeparate )
			{
				int numMerged;
				FeatureBase* featureMerged[2];
				mergeHalfSeparateBasicSideFeature( *linkTile , linkDir , featureMerged , numMerged );

				updateResult.numSideFeatureMerged += numMerged;
			}

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
			sideFeature->addNode( mapTile , dirLinkMask , nullptr );
		}
		else
		{
			sideFeature = static_cast< SideFeature* >( getFeature( linkGroup[0] ) );
			sideFeature->addNode( mapTile , dirLinkMask , linkNodeFrist );
		}

		if ( numLinkGroup > 0 )
		{
			for( int i = 1 ; i < numLinkGroup ; ++i )
			{
				FeatureBase* buildOther = getFeature( linkGroup[i] );
				sideFeature->mergeData( *buildOther , mapTile , linkNodeFrist->outConnect->index );
				destroyFeature(  buildOther );
			}

			updateResult.numSideFeatureMerged += numLinkGroup;
		}

		if ( sideFeature->calcOpenCount() != sideFeature->openCount )
		{
			CAR_LOG( "Warning: group=%d type=%d Error OpenCount!" , sideFeature->group , sideFeature->type );
			//@TODO: recalc open count
		}

		return sideFeature;
	}

	FarmFeature* GameLogic::updateFarm(MapTile& mapTile , unsigned idxLinkMask)
	{
		int numLinkGroup = 0;
		int linkGroup[ TilePiece::NumFarm ];
		FarmNode* linkNodeFrist = nullptr;

		for( auto iter = TBitMaskIterator<TilePiece::NumFarm>(idxLinkMask); iter; ++iter )
		{
			FarmNode* linkNode = mapTile.farmNodes[ iter.index ].outConnect;
			if ( linkNode == nullptr )
				continue;
			if ( linkNode->group == ERROR_GROUP_ID )
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
			farm->addNode( mapTile , idxLinkMask , nullptr );
		}
		else
		{
			farm = static_cast< FarmFeature* >( getFeature( linkGroup[0] ) );
			farm->addNode( mapTile , idxLinkMask , linkNodeFrist );
		}

		for( int i = 1 ; i < numLinkGroup ; ++i )
		{
			FeatureBase* farmOther = getFeature( linkGroup[i] );
			farm->mergeData( *farmOther , mapTile , linkNodeFrist->outConnect->index );
			destroyFeature( farmOther );
		}
		return farm;
	}


	void GameLogic::destroyFeature(FeatureBase* build)
	{
		assert( build->group != -1 );
		build->group = ERROR_GROUP_ID;
		//#TODO
	}

	int GameLogic::getActorPutInfo(int playerId , MapTile& mapTile , bool haveUsePortal , unsigned ignoreFeatureMask , std::vector< ActorPosInfo >& outInfo )
	{
		int result = 0;
		if( mapTile.isHalflingType() )
		{
			//#TODO


		}

		auto CanDeplayActor = [&](FeatureBase* feature) -> bool
		{
			if( ignoreFeatureMask & BIT(feature->type) )
				return false;

			if( feature->checkComplete() )
				return false;

			return true;
		};

		if ( haveUsePortal == false )
		{
			for( auto iter = TBitMaskIterator< FDir::TotalNum >(TilePiece::AllSideMask); iter; ++iter )
			{
				int dir = iter.index;
				iter.mask &= ~mapTile.getSideLinkMask(dir);

				if( mapTile.getSideContnet(dir) & SideContent::eHalfSeparate )
					continue;

				int group = mapTile.getSideGroup(dir);
				if( group == ERROR_GROUP_ID )
					continue;

				FeatureBase* feature = getFeature(group);

				if ( !CanDeplayActor(feature) )
					continue;

				result += feature->getActorPutInfo(playerId, dir, mapTile, outInfo);
			}

			for( auto iter = TBitMaskIterator< TilePiece::NumFarm >(TilePiece::AllFarmMask) ; iter ; ++iter )
			{
				int idx = iter.index;
				iter.mask &= ~mapTile.getFarmLinkMask(idx);
				int group = mapTile.getFarmGroup(idx);
				if( group == ERROR_GROUP_ID )
					continue;
				FeatureBase* feature = getFeature(group);

				if( !CanDeplayActor(feature) )
					continue;

				result += feature->getActorPutInfo(playerId, idx, mapTile, outInfo);
			}

			if( mSetting->have(Rule::eBarn) )
			{

				//TODO:



				//Barn
				for( int i = 0; i < FDir::TotalNum; ++i )
				{
					int n = 0;
					Vec2i posCheck = mapTile.pos;
					MapTile* mapTileCheck = &mapTile;
					int dirCheck = i;
					if( TilePiece::CanLinkFarm(mapTileCheck->getLinkType(dirCheck)) == false )
						continue;


					FarmFeature* farm = static_cast<FarmFeature*>(getFeature(mapTile.farmNodes[TilePiece::DirToFarmIndexFrist(i) + 1].group));
					if( farm->haveBarn )
						continue;

					for( ; n < FDir::TotalNum - 1; ++n )
					{
						posCheck = FDir::LinkPos(posCheck, dirCheck);
						mapTileCheck = mWorld.findMapTile(posCheck);
						dirCheck = (dirCheck + 1) % FDir::TotalNum;
						if( mapTileCheck == nullptr )
							break;
						if( TilePiece::CanLinkFarm(mapTileCheck->getLinkType(dirCheck)) == false )
							break;

					}
					if( n == FDir::TotalNum - 1 )
					{
						ActorPosInfo info;
						info.mapTile = &mapTile;
						info.pos.type = ActorPos::eTileCorner;
						info.pos.meta = i;
						info.actorTypeMask = BIT(ActorType::eBarn);
						info.group = mapTile.farmNodes[TilePiece::DirToFarmIndexFrist(i) + 1].group;
						outInfo.push_back(info);
						result += 1;
					}
				}
			}


			if( mapTile.group != ERROR_GROUP_ID )
			{
				FeatureBase* feature = getFeature(mapTile.group);

				if( CanDeplayActor(feature) )
				{
					result += feature->getActorPutInfo(playerId, 0, mapTile, outInfo);
				}
			}
		}

		if ( mSetting->have( Rule::eShepherdAndSheep ) )
		{
			//Shepherd
			unsigned mask = 0;
			for( int i = 0 ; i < FDir::TotalNum ; ++i )
			{
				if ( mask & BIT(i) )
					continue;
				if ( mapTile.getLinkType(i) != SideType::eField )
					continue;
				int group = mapTile.farmNodes[ TilePiece::DirToFarmIndexFrist( i ) + 1 ].group;
				
				FeatureBase* farm = getFeature( group );
				if ( farm->haveActorFromType( BIT( ActorType::eShepherd )) )
					continue;

				mask |= mapTile.getSideLinkMask(i);

				ActorPosInfo info;
				info.mapTile = &mapTile;
				info.pos.type = ActorPos::eSideNode;
				info.pos.meta = i;
				info.actorTypeMask = BIT( ActorType::eShepherd );
				info.group = group;
				outInfo.push_back( info );
				result += 1;
			}
		}

		return result;
	}

	void GameLogic::getMinTitlesNoCompletedFeature(FeatureType::Enum type, unsigned playerMask, unsigned actorTypeMask , std::vector<FeatureBase*>& outFeatures)
	{
		int minTileNum = INT_MAX;

		FeatureBase* result = nullptr;
		for(FeatureBase * feature : mFeatureMap)
		{
			if( feature->type != type )
				continue;
			if( feature->group == ERROR_GROUP_ID )
				continue;
			if( feature->checkComplete() )
				continue;
			if( !feature->haveActor(playerMask, actorTypeMask) )
				continue;
			int num = feature->getScoreTileNum();
			if( num == minTileNum )
			{
				outFeatures.push_back(feature);
			}
			else if( num < minTileNum )
			{
				outFeatures.clear();
				outFeatures.push_back(feature);
				minTileNum = num;
			}
		}
	}

	void GameLogic::getFeatureNeighborMapTile(FeatureBase& feature, MapTileSet& outMapTile)
	{
		for ( MapTileSet::iterator iter = feature.mapTiles.begin() , itEnd = feature.mapTiles.end();
			iter != itEnd ; ++iter)
		{
			MapTile* mapTile = *iter;
			for (int i = 0 ; i < FDir::NeighborNum ; ++i )
			{
				Vec2i pos = mapTile->pos + FDir::NeighborOffset( i );
				MapTile* testTile = mWorld.findMapTile( pos );
				if ( testTile && feature.mapTiles.find( testTile ) == itEnd )
				{
					outMapTile.insert( testTile );
				}
			}
		}
	}

	int GameLogic::getMaxFieldValuePlayer(FieldType::Enum type , PlayerBase* outPlayer[] , int& maxValue)
	{
		if ( mOrderedPlayers.empty() )
			return 0;

		int numPlayer = 0;
		PlayerBase* player = mOrderedPlayers[0];
		maxValue = player->getFieldValue(type);
		outPlayer[++numPlayer] = player;
		
		for( int i = 1 ; i < mOrderedPlayers.size() ; ++i )
		{
			player = mOrderedPlayers[i];
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

	LevelActor* GameLogic::createActor(ActorType type)
	{
		LevelActor* actor; 
		switch( type )
		{
		case ActorType::eShepherd:
			actor = new ShepherdActor;
			break;
		default:
			actor = new LevelActor;
		}
		assert( actor );
		actor->type = type;
		mActors.push_back( actor );
		return actor;
	}

	void GameLogic::destroyActor(LevelActor* actor)
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
		mActors.erase( std::find(mActors.begin() , mActors.end() , actor ));
		deleteActor( actor );
	}

	void GameLogic::deleteActor(LevelActor* actor)
	{
		switch( actor->type )
		{
		case ActorType::eShepherd:
			delete static_cast< ShepherdActor*>( actor );
			break;
		default:
			delete actor;
		}
	}	
	
	void GameLogic::resolveScorePlayer(TurnContext& turnContext, int playerId, int value , EScroeType* forceScoreType )
	{
		assert(value > 0);


		if( mSetting->have(Rule::eMessage) )
		{
			EScroeType scoreType;
			if( forceScoreType == nullptr || *forceScoreType == EScroeType::None )
			{
				GameSelectActionOptionData data;
				data.playerId = playerId;
				data.options = { ACTOPT_SCORE_NORMAL , ACTOPT_SCORE_WOMEN };
				data.group = ACTOPT_GROUP_SELECT_SCORE_TYPE;
				data.bCanSkip = false;
				CHECK_INPUT_RESULT(turnContext.getInput().requestSelectActionOption(data));
				data.validateResult("ScorePlayer");
				scoreType = (data.resultIndex == 0) ? EScroeType::Normal : EScroeType::Woman;
				if( forceScoreType )
					*forceScoreType = scoreType;

			}
			else
			{
				scoreType = *forceScoreType;
			}

			modifyPlayerScore(playerId, value, scoreType);

			int const DarkNumberSpaceOffset = 5;
			if ( turnContext.getPlayer()->mScore % DarkNumberSpaceOffset == 0 ||
				 turnContext.getPlayer()->getFieldValue(FieldType::eWomenScore) % DarkNumberSpaceOffset == 0 )
			{
				CHECK_RESOLVE_RESULT(resolveDrawMessageTile(turnContext));
			}
		}
		else
		{
			 modifyPlayerScore(playerId, value, EScroeType::Normal);
		}	
	}


	void GameLogic::resolveDrawMessageTile(TurnContext& turnContext)
	{
		//@TODO: Draw message token
		EMessageTile::Type token = drawMessageTile();


		GameSelectMessageOptionData data;
		data.token = token;
		switch( token )
		{
		case EMessageTile::ScoreSmallestRoad:
		case EMessageTile::ScoreSmallestCity:
		case EMessageTile::ScoreSmallestCloister:
			{
				std::vector< FeatureBase* > minScoreTilefeatures;
				FeatureType::Enum selectFeatureType;
				switch( token )
				{
				case EMessageTile::ScoreSmallestRoad:
					selectFeatureType = FeatureType::eRoad;
					break;
				case EMessageTile::ScoreSmallestCity:
					selectFeatureType = FeatureType::eCity;
					break;
				case EMessageTile::ScoreSmallestCloister:
					selectFeatureType = FeatureType::eCloister;
					break;
				}
				getMinTitlesNoCompletedFeature(selectFeatureType, BIT(turnContext.getPlayer()->getId()), mSetting->getFollowerMask(), minScoreTilefeatures);

				if( minScoreTilefeatures.empty() )
				{


				}
				else if( minScoreTilefeatures.size() == 1 )
				{


				}
				else
				{


				}
			}
			break;
		case EMessageTile::TwoPointsForEachPennant:
			{
				int messageSocre = 0;
				FeatureBase* result = nullptr;
				for (FeatureBase* feature : mFeatureMap)
				{
					if( feature->group == ERROR_GROUP_ID )
						continue;
					if( feature->checkComplete() )
						continue;
					if( !feature->haveActor(BIT(turnContext.getPlayer()->getId()), mSetting->getFollowerMask()) )
						continue;
					if( feature->type != FeatureType::eCity )
						continue;
					int numPennant = static_cast<CityFeature*>(feature)->getSideContentNum(BIT(SideContent::ePennant));
					messageSocre += 2 * numPennant;
				}
			}
			break;
		case EMessageTile::TwoPointsForEachKnight:
			{
				int messageSocre = 0;
				std::vector< LevelActor* > kinghts;
				FeatureBase* result = nullptr;
				for(FeatureBase* feature : mFeatureMap)
				{
					if( feature->group == ERROR_GROUP_ID )
						continue;
					if( feature->checkComplete() )
						continue;
					if( feature->type != FeatureType::eCity )
						continue;

					int iter = 0;
					while( auto actor = feature->iteratorActor(BIT(turnContext.getPlayer()->getId()), KINGHT_MASK, iter) )
					{
						kinghts.push_back(actor);
					}
				}
				messageSocre += 2 * kinghts.size();
			}
			break;
		case EMessageTile::TwoPointsForEachFarmer:
			{
				int messageSocre = 0;
				std::vector< LevelActor* > farmer;
				FeatureBase* result = nullptr;
				for(FeatureBase* feature : mFeatureMap)
				{
					if( feature->group == ERROR_GROUP_ID )
						continue;
					if( feature->checkComplete() )
						continue;
					if( feature->type != FeatureType::eFarm )
						continue;

					int iter = 0;
					while( auto actor = feature->iteratorActor(BIT(turnContext.getPlayer()->getId()), FARMER_MASK, iter) )
					{
						farmer.push_back(actor);
					}
				}
				messageSocre += 2 * farmer.size();
			}
			break;
		case EMessageTile::OneTile:
			break;
		case EMessageTile::ScoreAFollowerAndRemove:
			break;
		}

		CHECK_INPUT_RESULT(turnContext.getInput().requestSelectActionOption(data));
		data.validateResult("MessageSelectAction");

		if ( data.options[ data.resultIndex ] == ACTOPT_MESSAGE_PERFORM_ACTION )
		{
			switch( token )
			{
			case EMessageTile::ScoreSmallestRoad:
			case EMessageTile::ScoreSmallestCity:
			case EMessageTile::ScoreSmallestCloister:
			case EMessageTile::TwoPointsForEachPennant:
			case EMessageTile::TwoPointsForEachKnight:
			case EMessageTile::TwoPointsForEachFarmer:
			case EMessageTile::ScoreAFollowerAndRemove:
				modifyPlayerScore(turnContext.getPlayer()->getId(), data.messageScore);
				break;
			case EMessageTile::OneTile:
				//@TODO
				break;
			}
		}
		else
		{
			modifyPlayerScore(turnContext.getPlayer()->getId(), CAR_PARAM_VALUE(MessageSkipActionScore) );
		}

	}

	void GameLogic::resolvePlaceLittleBuilding(TurnContext& turnContext, MapTile* placeTiles[], int numPlaceTile, bool& haveDone)
	{

	}

	int GameLogic::modifyPlayerScore(int playerId, int value, EScroeType scoreType)
	{
		PlayerBase* player = mPlayerManager->getPlayer( playerId );
		switch( scoreType )
		{
		case EScroeType::Woman:
			assert(mSetting->have(Rule::eMessage));
			return player->modifyFieldValue(FieldType::eWomenScore, value);
		}
		
		player->mScore += value;
		return player->mScore;
	}

	int GameLogic::getFollowers(unsigned playerIdMask , ActorList& outActors , LevelActor* actorSkip )
	{
		int result = 0;
		for(LevelActor* actor : mActors)
		{
			if ( actor == actorSkip )
				continue;

			if ( actor->ownerId != CAR_ERROR_PLAYER_ID && ( playerIdMask & BIT( actor->ownerId ) ) )
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

	void GameLogic::returnActorToPlayer(LevelActor* actor)
	{
		PlayerBase* player = getOwnedPlayer( actor );
		assert(player);
		CAR_LOG( "Actor type=%d Return To Player %d" , actor->type , player->getId() );
		player->modifyActorValue( actor->type , 1 );

		while( LevelActor* follower = actor->popFollower() )
		{
			//#TODO
			switch( follower->type )
			{
			case ActorType::eFariy:
				break;
			case ActorType::ePig:
			case ActorType::eBuilder:
				{
					int iter = 0;
					LevelActor* newFollower = nullptr;
					unsigned playerMask = BIT(actor->ownerId);
					unsigned actorMask = mSetting->getFollowerMask();
					do
					{
						newFollower = actor->feature->iteratorActor( playerMask , actorMask , iter );
						if ( newFollower != actor )
							break;
					}
					while ( newFollower != nullptr );

					if ( newFollower != nullptr )
					{
						newFollower->addFollower( *follower );
					}
					else
					{
						returnActorToPlayer( follower );
					}
				}
				break;

			}
		}
		destroyActor( actor );
	}

	void GameLogic::moveActor(LevelActor* actor , ActorPos const& pos , MapTile* mapTile)
	{
		assert( actor );
		actor->pos = pos;
		if( mapTile )
		{
			mapTile->addActor(*actor);
			int group = mapTile->getFeatureGroup(pos);
			if( group != ERROR_GROUP_ID )
			{
				getFeature(group)->addActor(*actor);
			}
		}
		else if( actor->mapTile )
		{
			if( actor->feature )
			{
				actor->feature->removeActor(*actor);
			}
			actor->mapTile->removeActor(*actor);
		}

		for( LevelActor * followActor : actor->followers)
		{
			switch( followActor->type )
			{
			case ActorType::eFariy:
				break;
			}
		}
	}

	void GameLogic::reserveTile( TileId id )
	{
		mTilesQueue.push_back( id );
		++mNumTileNeedMix;
	}

	void GameLogic::checkReservedTileToMix()
	{
		if ( mNumTileNeedMix )
		{
			int num = mTilesQueue.size() - mIdxTile;
			for( int i = 0; i < num; ++i )
			{
				for( int n = 0; n < num; ++n )
				{
					int idx = mIdxTile + mRandom->getInt() % num;
					std::swap(mTilesQueue[i], mTilesQueue[idx]);
				}
			}
			mNumTileNeedMix = 0;
		}
	}

	void GameLogic::placeAllTileDebug( int numRow )
	{
		PlaceTileParam param;
		param.checkRiverConnect = 0;
		param.canUseBridge = 0;

		for( int i = 0 ; i < mTileSetManager.getRegisterTileNum() ; ++i )
		{
			MapTile* placeMapTiles[ 8 ];
			int numMapTile = mWorld.placeTileNoCheck( i , Vec2i( 2 *(i/numRow),2*(i%numRow) ) , 0 , param  , placeMapTiles );
			mListener->notifyPlaceTiles( i , placeMapTiles , numMapTile );
			for( int i = 0 ; i < numMapTile ; ++i )
			{
				FeatureUpdateInfoVec updateFeatures;
				UpdateTileFeatureResult updateResult(updateFeatures);
				updateTileFeature( *placeMapTiles[i] , updateResult );
			}
		}
	}

	EMessageTile::Type GameLogic::drawMessageTile()
	{
		EMessageTile::Type result = mMessageStack[mIndexTopMessage];
		++mIndexTopMessage;
		if( mIndexTopMessage == mMessageStack.size() )
			mIndexTopMessage = 0;
		return result;
	}

	void GameLogic::resolveAbbey(TurnContext& turnContext)
	{
		assert(turnContext.getPlayer()->getFieldValue( FieldType::eAbbeyPices ) > 0 );
		WorldTileManager& world = getWorld();

		TilePiece const& tile = world.getTile( mAbbeyTileId );

		std::vector< Vec2i > mapTilePosVec;

		for( auto iter = world.createEmptyLinkPosIterator(); iter ; ++iter )
		{
			Vec2i const& pos = *iter;

			int dir = 0;
			for(  ; dir < FDir::TotalNum ; ++dir )
			{
				int lDir = FDir::ToLocal( dir , 0 );

				Vec2i linkPos = FDir::LinkPos( pos , dir );
				MapTile* dataCheck = world.findMapTile( linkPos );
				if ( dataCheck == nullptr )
					break;

				int lDirCheck = FDir::ToLocal( FDir::Inverse( dir ) , dataCheck->rotation );
				TilePiece const& tileCheck = world.getTile( dataCheck->getId() );
				if ( !TilePiece::CanLink( tileCheck , lDirCheck , tile , lDir ) )
					break;
			}
			if ( dir == 4 )
			{
				mapTilePosVec.push_back( pos );
			}
		}

		if ( mapTilePosVec.empty() == false )
		{
			GameSelectMapPosData data;
			data.reason = SAR_PLACE_ABBEY_TILE;
			data.mapPositions = &mapTilePosVec[0];
			data.numSelection = mapTilePosVec.size();
			CHECK_INPUT_RESULT(turnContext.getInput().requestSelectMapPos( data ) );
		}
	}

	FeatureBase* GameLogic::getFeature(int group)
	{
		assert( group != -1 );
		return mFeatureMap[group];
	}

	void GameLogic::checkCastleComplete(FeatureBase &feature, int score)
	{
		for( CastleInfo* info : mCastles )
		{
			if ( ( BIT(feature.type) & CastleScoreTypeMask ) == 0 )
				continue;

			if ( feature.testInRectArea( info->min , info->max ) == false )
				continue;

			CastleScoreInfo s;
			s.feature = &feature;
			s.value   = score;
			info->featureScores.push_back( s );
		}
	}

	void GameLogic::resolveBuildCastle(TurnContext& turnContext, FeatureBase& feature , bool& haveBuild)
	{
		haveBuild = false;

		CityFeature& city = static_cast< CityFeature& >( feature );
		if ( city.isSamllCircular() == false || city.isCastle == true )
			return;

		LevelActor* kinght = city.findActorFromType( KINGHT_MASK );
		if ( kinght == nullptr || getOwnedPlayer( kinght )->getFieldValue( FieldType::eCastleTokens ) <= 0 )
			return;

		GameBuildCastleData data;
		data.bCanSkip = true;
		data.playerId = kinght->ownerId;
		data.city = &city;
		CHECK_INPUT_RESULT( turnContext.getInput().requestBuildCastle( data ) );

		if ( data.bSkipActionForResult == false )
		{
			data.city->isCastle = true;
			mPlayerManager->getPlayer(data.playerId)->modifyFieldValue( FieldType::eCastleTokens , -1 );
			CastleInfo* info = new CastleInfo;
			info->city = data.city;
			
			SideNode* nodeA = info->city->nodes[0];
			SideNode* nodeB = info->city->nodes[1];
			MapTile const* tileA = nodeA->getMapTile();
			MapTile const* tileB = nodeB->getMapTile();
			bool beH = tileA->pos.x == tileB->pos.x;
			if ( beH )
			{
				info->min.y = tileA->pos.y - 1;
				info->max.y = tileA->pos.y + 1;
				info->min.x = tileA->pos.x;
				info->max.x = tileB->pos.x;
				if ( info->min.x > info->max.x )
					std::swap( info->min.x , info->max.x );
			}
			else
			{
				info->min.x = tileA->pos.x - 1;
				info->max.x = tileA->pos.x + 1;
				info->min.y = tileA->pos.y;
				info->max.y = tileB->pos.y;
				if ( info->min.y > info->max.y )
					std::swap( info->min.y , info->max.y );
			}

			mCastlesRoundBuild.push_back( info );
			haveBuild = true;

		}
	}

	bool GameLogic::buildBridge(Vec2i const& pos , int dir)
	{
		if ( getTurnPlayer() == nullptr )
			return false;

		if ( getTurnPlayer()->getFieldValue( FieldType::eBridgePices ) == 0 )
			return false;

		MapTile* mapTile = mWorld.findMapTile( pos );
		if ( mapTile == nullptr )
			return false;

		mapTile->addBridge( dir );
		getTurnPlayer()->modifyFieldValue( FieldType::eBridgePices , -1 );
		return true;
	}

	bool GameLogic::redeemPrisoner(int ownerId , ActorType type)
	{
		if ( getTurnPlayer() == nullptr )
			return false;

		int idx = 0 ;
		for( ; idx < mPrisoners.size() ; ++idx )
		{
			PrisonerInfo& info = mPrisoners[idx];
			if ( info.playerId == getTurnPlayer()->getId() &&
				 info.ownerId == ownerId &&
				 info.type == type )
				break;
		}
		if ( idx == mPrisoners.size() )
			return false;

		mPrisoners.erase( mPrisoners.begin() + idx);

		modifyPlayerScore(ownerId, CAR_PARAM_VALUE(PrisonerRedeemScore));
		modifyPlayerScore(getTurnPlayer()->getId(), CAR_PARAM_VALUE(PrisonerRedeemScore));
		getTurnPlayer()->modifyActorValue( type , 1 );
		return true;
	}

	bool GameLogic::changePlaceTile(TileId id)
	{
		mUseTileId = id;
		updatePosibleLinkPos();
		return true;
	}

	void GameLogic::expandSheepFlock(LevelActor* actor)
	{
		assert( actor->type == ActorType::eShepherd );

		int idx = mRandom->getInt() % mSheepBags.size();

		SheepToken tok = mSheepBags[ idx ];
		int idxLast = mSheepBags.size() - 1;
		if ( idx != idxLast )
		{
			std::swap( mSheepBags[ idx ] , mSheepBags[ idxLast ] );
		}
		mSheepBags.pop_back();

		if ( tok == eWolf )
		{
			mSheepBags.push_back( tok );

			FeatureBase* farm = actor->feature;
			int iter = 0;
			while( ShepherdActor* shepherd = static_cast< ShepherdActor* >( farm->findActorFromType( BIT( ActorType::eShepherd ) ) ) )
			{
				for(auto token : shepherd->ownSheep)
				{
					mSheepBags.push_back(token);
				}
				returnActorToPlayer( shepherd );
			}
		}
		else
		{
			static_cast< ShepherdActor* >( actor )->ownSheep.push_back( tok );
		}
	}

	void GameLogic::updateBarnFarm(FarmFeature* farm , FeatureScoreInfo& featureScore)
	{
		bool haveFarmer = farm->haveActorFromType( FARMER_MASK );
		if ( haveFarmer == false )
			return;

		featureScore.feature = farm;
		featureScore.numController = farm->generateController(featureScore.controllerScores);
		assert(featureScore.numController > 0 );
		for( int i = 0; i < featureScore.numController; ++i )
		{
			featureScore.controllerScores[i].score = farm->calcPlayerScoreByBarnRemoveFarmer( mPlayerManager->getPlayer(featureScore.controllerScores[i].playerId ) );
		}
		

		farm->haveBarn = true;
		for( int i = 0 ; i < farm->mActors.size() ;  )
		{
			LevelActor* actor = farm->mActors[i];
			if ( FARMER_MASK & BIT( actor->type ) )
			{
				farm->removeActorByIndex( i );
				returnActorToPlayer( actor );
			}
			else
			{
				++i;
			}
		}
	}

	bool GameLogic::checkHaveBuilderFeatureExpend(PlayerBase* turnPlayer, FeatureUpdateInfoVec const& updateFeatures)
	{
		for (auto const& updateFeature : updateFeatures)
		{
			FeatureBase* feature = updateFeature.feature;
			if ( feature->group == ERROR_GROUP_ID )
				continue;

			if ( feature->type != FeatureType::eRoad && feature->type != FeatureType::eCity )
				continue;

			if ( feature->havePlayerActor( turnPlayer->getId() , ActorType::eBuilder ) == false )
				continue;
	
			if ( mSetting->have( Rule::eHaveAbbeyTile ) )
			{
				if ( updateFeature.bAbbeyUpdate )
					continue;
			}

			return true;
		}	

		return false;
	}

	void GameplaySetting::calcUsageOfField( int numPlayer )
	{
		struct MyFieldProc
		{
			void exec( FieldType::Enum type , int value )
			{
				++numFeild;
				fieldInfos[type].index = indexOffset++;
				fieldInfos[type].num = 1;
			}
			void exec( FieldType::Enum type, int* values, int num )
			{
				++numFeild;
				fieldInfos[type].index = indexOffset++;
				fieldInfos[type].num = num;
			}
			GameplaySetting::FieldInfo*   fieldInfos;
			int          indexOffset;
			int          numFeild;
		} proc;

		proc.fieldInfos = mFieldInfos;
		proc.indexOffset = 0;
		proc.numFeild = 0;
		ProcessFields( *this , numPlayer , proc );
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