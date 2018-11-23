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
#define CHECK_RESOLVE_RESULT( EXPR )\
	result = EXPR;\
	if ( result != TurnResult::eOK )\
		return result;

	static_assert(SheepToken::Num == Value::SheepTokenTypeNum,"SheepToken::Num not match Value::SheepTokenTypeNum");
	static_assert(MaxPlayerNum == Value::MaxPlayerNum,"MaxPlayerNum not match with Value::MaxPlayerNum");

	unsigned const CANT_FOLLOW_ACTOR_MASK = ( BIT( ActorType::eBuilder ) | BIT( ActorType::ePig ) );
	unsigned const FARMER_MASK = BIT( ActorType::eMeeple ) | BIT( ActorType::eBigMeeple ) | BIT( ActorType::ePhantom );
	unsigned const KINGHT_MASK = BIT( ActorType::eMeeple ) | BIT( ActorType::eBigMeeple ) | BIT( ActorType::ePhantom ) | BIT( ActorType::eMayor ) | BIT( ActorType::eWagon );

	unsigned const DRAGON_EAT_ACTOR_TYPE_MASK = 
		BIT( ActorType::eMeeple ) | BIT( ActorType::eBigMeeple ) | BIT( ActorType::eWagon ) | 
		BIT( ActorType::eMayor ) | BIT( ActorType::ePhantom ) | BIT( ActorType::eAbbot ) |
		BIT( ActorType::eShepherd ) | BIT( ActorType::eBuilder ) | BIT( ActorType::ePig ) |
		BIT( ActorType::eMage ) | BIT( ActorType::eWitch );
	unsigned const CastleScoreTypeMask = BIT( FeatureType::eCity ) | BIT( FeatureType::eCloister ) | BIT( FeatureType::eRoad ); 

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

	struct FieldProc
	{
		void exec( int type , int value );
	};
	template< class FieldProc >
	void ProcField( GameplaySetting& setting , int numPlayer  , FieldProc& proc )
	{
#define FIELD_ACTOR( TYPE , VALUE )\
	proc.exec( FieldType::Enum( FieldType::eActorStart + TYPE ) , VALUE )
#define FIELD_VALUE( TYPE , VALUE )\
	proc.exec( TYPE , VALUE )
#define FIELD_ARRAY_VALUES( TYPE , VALUES , NUM )\
	proc.exec( TYPE , VALUES , NUM )

#define VALUE( NAME ) setting.##NAME

		FIELD_ACTOR( ActorType::eMeeple , VALUE(MeeplePlayerOwnNum) );
		
		if( setting.have(Rule::eBigMeeple) )
		{
			FIELD_ACTOR( ActorType::eBigMeeple , VALUE(BigMeeplePlayerOwnNum) );
		}
		if( setting.have(Rule::eBuilder) )
		{
			FIELD_ACTOR(ActorType::eBuilder, VALUE(BuilderPlayerOwnNum) );
		}
		if( setting.have(Rule::ePig) )
		{
			FIELD_ACTOR(ActorType::ePig, VALUE(PigPlayerOwnNum) );
		}
		if( setting.have(Rule::eTraders) )
		{
			FIELD_VALUE(FieldType::eWine, 0);
			FIELD_VALUE(FieldType::eGain, 0);
			FIELD_VALUE(FieldType::eCloth, 0);
		}
		if( setting.have(Rule::eBarn) )
		{
			FIELD_ACTOR(ActorType::eBarn, VALUE(BarnPlayerOwnNum) );
		}
		if( setting.have(Rule::eWagon) )
		{
			FIELD_ACTOR(ActorType::eWagon, VALUE(WagonPlayerOwnNum) );
		}
		if( setting.have(Rule::eMayor) )
		{
			FIELD_ACTOR(ActorType::eMayor, VALUE(MayorPlayerOwnNum) );
		}
		if( setting.have(Rule::eHaveAbbeyTile) )
		{
			FIELD_VALUE(FieldType::eAbbeyPices, VALUE(AbbeyTilePlayerOwnNum) );
		}
		if( setting.have(Rule::eTower) )
		{
			FIELD_VALUE( FieldType::eTowerPices , VALUE(TowerPicesPlayerOwnNum[numPlayer-1]) );
		}
		if( setting.have(Rule::eBridge) )
		{
			FIELD_VALUE(FieldType::eBridgePices, VALUE(BridgePicesPlayerOwnNum[numPlayer-1]) );
		}
		if( setting.have(Rule::eCastleToken) )
		{
			FIELD_VALUE(FieldType::eCastleTokens, VALUE(CastleTokensPlayerOwnNum[numPlayer-1]) );
		}
		if( setting.have(Rule::eBazaar) )
		{
			FIELD_VALUE( FieldType::eTileIdAuctioned , FAIL_TILE_ID );
		}
		if ( setting.have(Rule::eShepherdAndSheep) )
		{
			FIELD_ACTOR( ActorType::eShepherd , VALUE(ShepherdPlayerOwnNum) );
		}
		if( setting.have(Rule::ePhantom) )
		{
			FIELD_ACTOR( ActorType::ePhantom , VALUE(PhantomPlayerOwnNum) );
		}
		if( setting.have(Rule::eHaveGermanCastleTile) )
		{

		}
		if( setting.have(Rule::eHaveHalflingTile) )
		{

		}

#undef FIELD_ACTOR
#undef FIELD_VALUE
#undef VALUE
	}

	GameLogic::GameLogic()
		:mWorld( mTileSetManager )
	{
		mDebug = false;
		mIdxPlayerTrun = 0;
		mIdxTile = 0;
		mNumTileNeedMix = 0;
		mDragon = nullptr;
		mFairy = nullptr;
		mIsStartGame = false;
	}

	void GameLogic::cleanupData()
	{
		for( int i = 0 ; i < mActorList.size() ; ++i )
		{
			deleteActor( mActorList[i] );
		}
		mActorList.clear();

		for( int i = 0 ; i < mFeatureMap.size() ; ++i )
		{
			delete mFeatureMap[i];
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

	void GameLogic::setupSetting(GameplaySetting& setting)
	{
		mSetting = &setting;
		mSetting->calcUsageField( mPlayerManager->getPlayerNum() );
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
			ProcField( *mSetting , mPlayerManager->getPlayerNum() , proc );
		}

		if( beInit )
		{
			mTileSetManager.addExpansion(EXP_BASIC);
			for( int i = 0; i < NUM_EXPANSIONS; ++i )
			{
				if( mSetting->mExpansionMask & BIT(i) )
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
			mTilesQueue.clear();
			mPlayerOrders.clear();
			mTowerTiles.clear();
			mPrisoners.clear();
			mSheepBags.clear();
			mWorld.restart();
		}
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
			TileId startId = generatePlayTile();
			if ( mDebug )
			{
				placeAllTileDebug( 5 );
			}
			else
			{
				PutTileParam param;
				param.checkRiverConnect = true;
				param.usageBridge = false;

				MapTile* placeMapTiles[ 8 ];
				int numMapTile = mWorld.placeTileNoCheck( startId , Vec2i(0,0) , 0 , param , placeMapTiles );
				mListener->onPutTile( startId , placeMapTiles , numMapTile );
				mUpdateFeatures.clear();
				for( int i = 0 ; i < numMapTile ; ++i )
				{
					UpdateTileFeatureResult updateResult;
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
			PlayerBase* curTrunPlayer = mPlayerOrders[ mIdxPlayerTrun ];
			curTrunPlayer->startTurn();

			TurnResult result = resolvePlayerTurn( input , curTrunPlayer );
			if ( result == eFinishGame )
				break;
			if ( result == eExitGame )
				return;

			if ( mIsRunning )
			{
				GameActionData data;
				input.requestTurnOver( data );
				if( !checkGameState(data, result) )
				{
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
			if ( mIdxPlayerTrun == mPlayerOrders.size() )
				mIdxPlayerTrun = 0;
		}

		calcFinalScore();

	}

	void GameLogic::calcFinalScore()
	{
		for( int i = 0 ; i < mFeatureMap.size() ; ++i )
		{
			FeatureBase* build = mFeatureMap[i];
			if ( build->group == ERROR_GROUP_ID )
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
							FeatureBase::initFeatureScoreInfo(scoreInfos); 
							for( std::set<FarmFeature*>::iterator iter = city->linkedFarms.begin() , itEnd = city->linkedFarms.end();
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
				if ( numPlayer > 0 )
					addFeaturePoints( *build , scoreInfos , numPlayer );
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

			for( int i = 0 ; i < mFeatureMap.size() ; ++i )
			{
				FeatureBase* build = mFeatureMap[i];
				if ( build->group == ERROR_GROUP_ID )
					continue;
				if ( build->checkComplete() == false )
					continue;

				if ( build->type == FeatureType::eCity )
				{
					if ( static_cast< CityFeature* >( build )->isCastle == false )
						++numCityComplete;
				}
				else if ( build->type == FeatureType::eRoad )
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
	}

	TileId GameLogic::generatePlayTile()
	{
		int numTile = mTileSetManager.getRegisterTileNum();
		std::vector< int > tilePieceMap( numTile );
		std::vector< TileId > specialTileList;
		for( int i = 0 ; i < numTile ; ++i )
		{
			TileSet const& tileSet = mTileSetManager.getTileSet( TileId(i) );
			if ( tileSet.tag != 0 )
				specialTileList.push_back( tileSet.tiles[0].id );
				
			tilePieceMap[i] = tileSet.numPiece;
		}
	
		bool useRiver = mTileSetManager.haveUse( EXP_THE_RIVER ) || mTileSetManager.haveUse( EXP_THE_RIVER_II ) ;

		//River
		TileId tileIdStart = FAIL_TILE_ID;
		TileId tileIdRiverEnd = FAIL_TILE_ID;
		TileId tileIdFristPlay = FAIL_TILE_ID;
		if ( useRiver )
		{
			if ( mTileSetManager.haveUse( EXP_THE_RIVER_II ) )
			{
				int idx;
				idx = findTagTileIndex( specialTileList , EXP_THE_RIVER_II , TILE_START_TAG );
				if ( idx != -1 )
					tileIdStart = specialTileList[ idx ];
				idx = findTagTileIndex( specialTileList , EXP_THE_RIVER_II , TILE_END_TAG );
				if ( idx != -1 )
					tileIdRiverEnd = specialTileList[ idx ];
				idx = findTagTileIndex( specialTileList , EXP_THE_RIVER_II , TILE_FRIST_PLAY_TAG );
				if ( idx != -1 )
					tileIdFristPlay = specialTileList[ idx ];
			}

			if ( mTileSetManager.haveUse( EXP_THE_RIVER ) )
			{
				int idx;
				idx = findTagTileIndex( specialTileList , EXP_THE_RIVER , TILE_START_TAG );
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

				idx = findTagTileIndex( specialTileList , EXP_THE_RIVER , TILE_END_TAG );
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

		std::vector< int > shuffleGroup;
		if ( useRiver )
		{
			TileIdVec const& idSetGroup = mTileSetManager.getGroup( TileSet::eRiver );
			if ( tileIdFristPlay != FAIL_TILE_ID )
			{
				mTilesQueue.push_back( tileIdFristPlay );
				shuffleGroup.push_back( mTilesQueue.size() );
			}
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
			TileIdVec const& idSetGroup = mTileSetManager.getGroup( TileSet::eCommon );
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

		if ( mTileSetManager.haveUse( EXP_ABBEY_AND_MAYOR ) )
		{
			int idx = findTagTileIndex( specialTileList , EXP_ABBEY_AND_MAYOR , TILE_ABBEY_TAG );
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

		if ( mTileSetManager.haveUse( EXP_CASTLES ) )
		{
			TileIdVec const& castales = mTileSetManager.getGroup( TileSet::eGermanCastle );
			int numCastalePlayer = mPlayerManager->getPlayerNum() > 3 ? 1 : 2;
			//#TODO
		}

		if( mTileSetManager.haveUse(EXP_HALFLINGS_I) || mTileSetManager.haveUse(EXP_HALFLINGS_II) )
		{



		}
		mIdxTile = 0;
		mNumTileNeedMix = 0;

		return tileIdStart;
	}

	void GameLogic::randomPlayerPlayOrder()
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

	bool CheckTileContent( MapTile* mapTiles[] , int numTile , unsigned contentMask )
	{
		for( int i = 0 ; i < numTile ; ++i)
		{
			if ( mapTiles[i]->getTileContent() & contentMask )
				return true;
		}
		return false;
	}

	bool CheckTileContent(TileSet const& tileSet, unsigned contentMask)
	{
		for( int i = 0; i < tileSet.numPiece; ++i )
		{
			if( tileSet.tiles[i].contentFlag & contentMask )
				return true;
		}
		return false;

	}

	TurnResult GameLogic::resolvePlayerTurn(IGameInput& input , PlayerBase* curTrunPlayer)
	{
		TurnResult result;

		//Step 1: Begin Turn
		if ( mSetting->have( Rule::eFariy ))
		{
			if ( mFairy->binder && mFairy->binder->owner == curTrunPlayer )
			{
				CAR_LOG( "%d Score:Fairy Beginning Of A Turn Score" , curTrunPlayer->getId() );
				modifyPlayerScore( curTrunPlayer->getId() , CAR_PARAM_VALUE(FairyBeginningOfATurnScore) );
			}
		}

		int numUseTilePices = 0;
		for(;;)// step 2 to 9
		{
			//Step 2: Draw a Tile
			bool haveHillTile = false;
			CHECK_RESOLVE_RESULT( resolveDrawTile( input , curTrunPlayer , haveHillTile ) );

			int const MAX_MAP_TILES_NUM = 32;
			
			MapTile* placeMapTiles[MAX_MAP_TILES_NUM];
			int numPlaceTile = 0;
			{
				//Step 3: Place the Tile
				CHECK_RESOLVE_RESULT( resolvePlaceTile( input, curTrunPlayer , placeMapTiles , numPlaceTile ) );

				++numUseTilePices;
				//b)Note: any feature that is finished is considered complete at this time.
				mUpdateFeatures.clear();
				for( int i = 0 ; i < numPlaceTile; ++i )
				{
					placeMapTiles[i]->haveHill = haveHillTile;
					UpdateTileFeatureResult updateResult;
					updateTileFeature( *placeMapTiles[i] , updateResult );
				}
			}

			if ( mSetting->have( Rule::eTower ) )
			{
				for( int i = 0 ; i < numPlaceTile; ++i )
				{
					if ( placeMapTiles[i]->getTileContent() & TileContent::eTowerFoundation )
						mTowerTiles.push_back( placeMapTiles[i] );
				}
			}

			bool haveBuilderFeatureExpend = false;
			if ( mSetting->have( Rule::eBuilder ) )
			{
				haveBuilderFeatureExpend = checkHaveBuilderFeatureExpend( curTrunPlayer );
			}

			bool haveVolcano = false;
			bool havePlagueSource = false;
			bool skipAllActorStep = false;
			

			//c)volcano
			if( mSetting->have(Rule::eDragon) )
			{
				for( int i = 0; i < numPlaceTile; ++i )
				{
					MapTile* mapTile = placeMapTiles[i];
					if( mapTile->have( TileContent::eVolcano ) )
					{
						mTowerTiles.push_back(placeMapTiles[i]);
						if( mDragon->mapTile == nullptr )
							checkReservedTileToMix();

						moveActor(mDragon, ActorPos(ActorPos::eTile, 0), mapTile);
						haveVolcano = true;
					}
				}
			}
			//d)princess
			if( mSetting->have(Rule::ePrinecess) )
			{
				bool haveDone = false;
				for( int i = 0; i < numPlaceTile; ++i )
				{
					CHECK_RESOLVE_RESULT( resolvePrincess(input, placeMapTiles[i], haveDone) );
					if ( haveDone )
						break;
				}
				if( haveDone )
					skipAllActorStep = true;
			}

			if ( skipAllActorStep == false )
			{
				//Step 4A: Move the Wood (Phase 1)
				do 
				{
					bool haveDeployActor = false;
					if ( haveVolcano == false && havePlagueSource == false )
					{
						bool havePortal = false;
						bool haveUsePortal = false;

						havePortal = CheckTileContent(placeMapTiles, numPlaceTile, TileContent::eMagicPortal);

						for( int step = 0 ; step < 2 ; ++step )
						{
							unsigned actorMask;
							if ( step == 0 )
							{
								actorMask = curTrunPlayer->getUsageActorMask() & ~BIT( ActorType::ePhantom );
							}
							else if ( step == 1 )
							{
								if ( curTrunPlayer->haveActor( ActorType::ePhantom ) == false )
									continue;

								actorMask = BIT( ActorType::ePhantom );
							}

							MapTile* deployMapTiles[MAX_MAP_TILES_NUM];
							std::copy_n( placeMapTiles , numPlaceTile , deployMapTiles);
							int numDeployTile = numPlaceTile;
							if ( havePortal && !haveUsePortal )
							{
								MapTile* portalTargetTile = nullptr;
								CHECK_RESOLVE_RESULT( resolvePortalUse( input, curTrunPlayer, portalTargetTile, haveUsePortal ) );

								if( portalTargetTile )
								{
									deployMapTiles[0] = portalTargetTile;
									numDeployTile = 1;
								}
							}

							if ( numDeployTile )
							{
								bool haveDone = false;
								if( haveUsePortal )
									actorMask &= mSetting->getFollowerMask();

								CHECK_RESOLVE_RESULT( resolveDeployActor( input , curTrunPlayer, deployMapTiles , numDeployTile , actorMask , haveUsePortal, haveDone ) );

								if ( haveDone )
									haveDeployActor = true;
							}
						}

						if ( haveDeployActor )
							break;
					}
					//d)

					//e)
					if ( mSetting->have( Rule::eTower ) )
					{
						bool haveDone = false;
						CHECK_RESOLVE_RESULT( resolveTower( input , curTrunPlayer , haveDone ) );
						if ( haveDone )
							break;
					}

					if( mSetting->have(Rule::eFariy) )
					{
						bool haveDone = false;
						CHECK_RESOLVE_RESULT( resolveMoveFairyToNextFollower( input, curTrunPlayer , haveDone ) );
						if ( haveDone )
							break;
					}
				} 
				while (0);

				//Step 4B: Move the Wood (Phase 2)

				
			}

			//Step 5: Resolve Move the Wood

			//c) Shepherd
			if ( mSetting->have( Rule::eShepherdAndSheep ) )
			{
				for( FeatureUpdateInfoVec::iterator iter = mUpdateFeatures.begin(), itEnd = mUpdateFeatures.end();
					iter != itEnd ; ++iter )
				{
					FeatureBase* feature = iter->feature;
					if ( feature->group == ERROR_GROUP_ID )
						continue;
					if ( feature->type != FeatureType::eFarm )
						continue;

					CHECK_RESOLVE_RESULT( resolveExpendShepherdFarm( input , curTrunPlayer , feature ) );
				}
			}
			//d) dragon move
			if ( mSetting->have( Rule::eDragon ) )
			{
				if ( mSetting->have( Rule::eMoveDragonBeforeScoring ) == true )
				{
					if( CheckTileContent(placeMapTiles, numPlaceTile, TileContent::eTheDragon) &&
					   mDragon->mapTile != nullptr )
					{
						CHECK_RESOLVE_RESULT( resolveDragonMove(input, *mDragon) );
					}
				}
			}

			//Step 6: Identify Completed Features
			for( FeatureUpdateInfoVec::iterator iter = mUpdateFeatures.begin(), itEnd = mUpdateFeatures.end();
				iter != itEnd ; ++iter )
			{
				FeatureBase* feature = iter->feature;
				if ( feature->group == ERROR_GROUP_ID )
					continue;

				//a) Identify all completed features
				if ( feature->checkComplete() )
				{
					CAR_LOG( "Feature %d complete by Player %d" , feature->group , curTrunPlayer->getId()  );
					//Step 7: Resolve Completed Features
					CHECK_RESOLVE_RESULT( resolveCompleteFeature( input , *feature , nullptr ) );
				}
				else if ( feature->type == FeatureType::eFarm )
				{
					if ( mSetting->have( Rule::eBarn ) && feature->haveActorMask( BIT( ActorType::eBarn ) ) )
					{
						FarmFeature* farm = static_cast< FarmFeature*>( feature );
						updateBarnFarm(farm);
					}
				}
			}
			//castle complete
			if ( mSetting->have(Rule::eCastleToken) )
			{
				CHECK_RESOLVE_RESULT( resolveCastleComplete( input ) );
			}

			//Step 8: Resolve the Tile
			//d) dragon move
			if( mSetting->have(Rule::eDragon) )
			{
				if ( mSetting->have(Rule::eMoveDragonBeforeScoring ) == false )
				{
					if( CheckTileContent(placeMapTiles, numPlaceTile, TileContent::eTheDragon) &&
					   mDragon->mapTile != nullptr )
					{
						CHECK_RESOLVE_RESULT( resolveDragonMove(input, *mDragon) );
					}
				}
			}

			//Step 9: Resolve the Turn
			//a)builder
			if ( mSetting->have( Rule::eBuilder ) && numUseTilePices == 1 )
			{
				if ( haveBuilderFeatureExpend )
				{
					CAR_LOG( "Player %d have double Turn" , getTurnPlayer()->getId() );
					continue;
				}
			}
			//b)bazaar
			if ( mSetting->have( Rule::eBazaar ) )
			{
				for( int i = 0 ; i < numPlaceTile; ++i )
				{
					MapTile* mapTile = placeMapTiles[i];
					if ( mapTile->have( TileContent::eBazaar ) )
					{
						CHECK_RESOLVE_RESULT( resolveAuction( input , curTrunPlayer ) );
					}
				}
			}

			break;
		}

		return TurnResult::eOK;
	}

	TurnResult GameLogic::resolveDrawTile(IGameInput& input , PlayerBase* curTrunPlayer , bool& haveHillTile )
	{
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
				if ( curTrunPlayer->getFieldValue( FieldType::eTileIdAuctioned ) != FAIL_TILE_ID )
				{
					mUseTileId = (TileId)curTrunPlayer->getFieldValue( FieldType::eTileIdAuctioned );
					curTrunPlayer->setFieldValue( FieldType::eTileIdAuctioned , FAIL_TILE_ID );
					CAR_LOG( "Player %d Use Auctioned Tile %d" , curTrunPlayer->getId() , mUseTileId );
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

					if ( CheckTileContent( tileSet , TileContent::eTheDragon ) &&
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

			PutTileParam param;
			param.usageBridge = 0;
			param.checkRiverConnect = 1;
			if ( updatePosibleLinkPos( param ) == 0 )
			{
				if( mSetting->have(Rule::eBridge) )
				{
					param.usageBridge = 1;
					if ( updatePosibleLinkPos( param ) != 0 )
					{
						//#TODO
						continue;
					}
				}
				if ( getRemainingTileNum() == 0 )
					return TurnResult::eFinishGame;
			}
			break;
		}

		haveHillTile = ( hillTileId != FAIL_TILE_ID );

		return TurnResult::eOK;
	}

	TurnResult GameLogic::resolvePlaceTile(IGameInput& input , PlayerBase* curTrunPlayer , MapTile* placeMapTiles[] , int& numMapTile )
	{
		TurnResult result;
		for(;;)
		{
			//a) Place the tile.
			GamePlaceTileData putTileData;
			putTileData.id = mUseTileId;
			input.requestPlaceTile( putTileData );
			if ( checkGameState( putTileData , result ) == false )
				return result;

			if ( putTileData.resultRotation < 0 || putTileData.resultRotation >= FDir::TotalNum )
			{
				CAR_LOG("Warning: Place Tile have error rotation %d" , putTileData.resultRotation );
				putTileData.resultRotation = 0;
			}

			PutTileParam param;
			param.usageBridge = 0;
			param.checkRiverConnect = 1;
			numMapTile = mWorld.placeTile( mUseTileId , putTileData.resultPos , putTileData.resultRotation , param , placeMapTiles );
			if ( numMapTile != 0 )
			{
				CAR_LOG("Player %d Place Tile %d rotation=%d" , curTrunPlayer->getId() , mUseTileId , putTileData.resultRotation );
				mListener->onPutTile( mUseTileId , placeMapTiles , numMapTile );
				break;
			}
		}

		return TurnResult::eOK;
	}

	TurnResult GameLogic::resolveDeployActor(IGameInput& input , PlayerBase* curTrunPlayer, MapTile* deployMapTiles[] , int numDeployTile , unsigned actorMask , bool haveUsePortal , bool& haveDone)
	{
		TurnResult result;
		//a-c) follower and other figures
		calcPlayerDeployActorPos(*curTrunPlayer, deployMapTiles , numDeployTile , actorMask , haveUsePortal );

		GameDeployActorData deployActorData;
		input.requestDeployActor( deployActorData );
		if ( checkGameState( deployActorData , result ) == false )
			return result;

		if ( deployActorData.resultSkipAction == true )
			return TurnResult::eOK;

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
		info.mapTile->addActor( *actor );
		feature->addActor( *actor );
		actor->owner   = curTrunPlayer;
		actor->pos     = info.pos;

		switch( actor->type )
		{
		case ActorType::ePig:
		case ActorType::eBuilder:
			{
				LevelActor* actorFollow = feature->findActor( BIT( curTrunPlayer->getId() ) , mSetting->getFollowerMask() & ~CANT_FOLLOW_ACTOR_MASK );
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

		mListener->onDeployActor( *actor );
		CAR_LOG( "Player %d deploy Actor : type = %d  " , curTrunPlayer->getId() , actor->type );

		switch( actor->type )
		{
		case ActorType::eShepherd:
			expandSheepFlock(actor);
			break;
		}

		haveDone = true;
		return TurnResult::eOK;
	}

	TurnResult GameLogic::resolveCompleteFeature( IGameInput& input , FeatureBase& feature , CastleScoreInfo* castleScore )
	{
		TurnResult result;
		assert( feature.checkComplete() );

		if( mSetting->have(Rule::eCastleToken) )
		{
			if ( feature.type == FeatureType::eCity )
			{
				bool haveBuild;
				CHECK_RESOLVE_RESULT( resolveBuildCastle( input , feature, haveBuild ) );
				if ( haveBuild )
					return TurnResult::eOK;
			}
		}

		//c)
		if ( mSetting->have(Rule::eFariy) )
		{
			LevelActor* actor = mFairy->binder;
			if ( actor && actor->feature == &feature )
			{
				CAR_LOG("Score: %d FairyFearureScoringScore" , actor->owner->getId() );
				modifyPlayerScore( actor->owner->getId() , CAR_PARAM_VALUE(FairyFearureScoringScore) );
			}
		}
		//d)
		//->Collect any trade good tokens
		if ( feature.type == FeatureType::eCity )
		{
			CityFeature& city = static_cast< CityFeature& >( feature );
			if ( mSetting->have( Rule::eTraders ) )
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
		if ( mSetting->have( Rule::eKingAndRobber ) )
		{
			if ( feature.type == FeatureType::eCity && castleScore == nullptr )
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

		int numController = 0;
		std::vector< FeatureScoreInfo > scoreInfos;
		if ( castleScore )
		{
			FeatureScoreInfo info;
			info.playerId = feature.findActor( KINGHT_MASK )->owner->getId();
			info.majority = 1;
			info.score = castleScore->value;
			scoreInfos.push_back( info );
			numController = 1;
		}
		else
		{
			numController = feature.calcScore( scoreInfos );
		}
		if ( numController > 0 )
		{
			if ( mSetting->have( Rule::eHaveGermanCastleTile ) &&
				 ( feature.type == FeatureType::eCity || feature.type == FeatureType::eRoad ) )
			{
				int numNeighborCastle = 0;
				for( int i = 0 ; i < mGermanCastles.size() ; ++i )
				{
					MapTile const* tileA = *mGermanCastles[i]->mapTiles.begin();
					MapTile const* tileB = *(++mGermanCastles[i]->mapTiles.begin());
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
					if ( feature.testInRange( min , max ) )
					{
						++numNeighborCastle;
					}
				}

				if ( numNeighborCastle )
				{
					for( int i = 0 ; i < numController ; ++i )
					{
						scoreInfos[i].score += numNeighborCastle * CAR_PARAM_VALUE(GermanCastleAdditionFactor);
					}
				}
			}

			addFeaturePoints( feature , scoreInfos , numController );
		}

		if ( mSetting->have( Rule::eCastleToken ) )
		{
			int score = 0;
			if ( numController > 0 )
			{
				for( int i = 0 ; i < numController ; ++i )
				{
					if ( score < scoreInfos[i].score )
						score = scoreInfos[i].score;
				}
			}
			else
			{
				score = feature.calcPlayerScore( FAIL_PLAYER_ID );
			}

			checkCastleComplete( feature , score );
		}

		CHECK_RESOLVE_RESULT( resolveFeatureReturnPlayerActor( input , feature ) );
	
		return TurnResult::eOK;
	}

	TurnResult GameLogic::resolveCastleComplete(IGameInput& input)
	{
		TurnResult result;
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

			for( int i = 0 ; i < castleGroup.size() ; ++i )
			{
				CastleInfo* info = castleGroup[i];
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

				CHECK_RESOLVE_RESULT( resolveCompleteFeature( input , *info->city , scoreInfo ) );

				delete info;
			}
		}
		while( castleGroup.empty() == false );

		return TurnResult::eOK;
	}

	TurnResult GameLogic::resolveDragonMove(IGameInput& input , LevelActor& dragon)
	{
		assert( dragon.mapTile != nullptr );

		TurnResult result;

		std::vector< MapTile* > moveTilesBefore;
		moveTilesBefore.resize(CAR_PARAM_VALUE(DragonMoveStepNum));

		MapTile* moveTiles[FDir::TotalNum];
		GameDragonMoveData data;
		data.mapTiles = moveTiles;
		data.reason = SAR_MOVE_DRAGON;

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
			data.playerId = mPlayerOrders[ idxPlayer ]->getId();
			
			input.requestSelectMapTile(data);
			if ( checkGameState( data , result ) == false )
				return result;

			if ( data.checkResult() == false )
			{
				CAR_LOG("Warning: Error Dragon Move Index %d" , data.resultIndex );
				data.resultIndex = 0;
			}

			moveTilesBefore[ step ] = dragon.mapTile;
			moveActor( &dragon , ActorPos( ActorPos::eTile , 0 ) , data.mapTiles[ data.resultIndex ] );
			
			for( int i = 0 ; i < dragon.mapTile->mActors.size() ; ++i )
			{
				LevelActor* actor = dragon.mapTile->mActors[i];
				bool bDrageCanEat = ( BIT( actor->type ) & DRAGON_EAT_ACTOR_TYPE_MASK ) != 0;

				if ( bDrageCanEat == false )
					continue;

				if ( mSetting->have( Rule::eShepherdAndSheep ) && actor->type == ActorType::eShepherd )
				{
					ShepherdActor* shepherd = static_cast< ShepherdActor* >( actor );
					for( int i = 0 ; i < (int)shepherd->ownSheep.size() ; ++ i )
					{
						mSheepBags.push_back( shepherd->ownSheep[i] );
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
		return TurnResult::eOK;
	}

	TurnResult GameLogic::resolvePortalUse(IGameInput& input, PlayerBase* curTrunPlayer, MapTile*& deployMapTile , bool& haveUsePortal)
	{
		TurnResult result;
		int playerId = curTrunPlayer->getId();

		std::set< MapTile* > mapTileSet;
		for( int i = 0 ; i < mFeatureMap.size() ; ++i )
		{
			FeatureBase* feature = mFeatureMap[i];
			if ( feature->group == ERROR_GROUP_ID )
				continue;
			if ( feature->checkComplete() )
				continue;
			if ( feature->haveActorMask( mSetting->getFollowerMask() ) )
				continue;

			for( MapTileSet::iterator iter = feature->mapTiles.begin() , itEnd = feature->mapTiles.end();
				iter != itEnd ; ++iter )
			{
				MapTile* mapTile = *iter;
				if ( !canDeployFollower( *mapTile ) )
					continue;
				mapTileSet.insert( mapTile );
			}
		}

		if ( mapTileSet.empty() == false )
		{
			std::vector< MapTile* > mapTiles( mapTileSet.begin() , mapTileSet.end() );

			GameSelectMapTileData data;
			data.reason = SAR_MAGIC_PORTAL;
			data.mapTiles = &mapTiles[0];
			data.numSelection = mapTiles.size();
			input.requestSelectMapTile( data );
			if ( checkGameState( data , result ) == false )
				return result;

			if( !data.checkResult() )
			{
				CAR_LOG("Warning! Error Portal Result Index Tile!");
				data.resultIndex = 0;
			}

			deployMapTile = data.mapTiles[ data.resultIndex ];
			haveUsePortal = true;
		}
		else
		{
			deployMapTile = nullptr;
		}

		return TurnResult::eOK;
	}

	CAR::TurnResult GameLogic::resolveExpendShepherdFarm(IGameInput& input , PlayerBase* curTrunPlayer , FeatureBase* feature)
	{
		assert( feature->type == FeatureType::eFarm );

		TurnResult result;

		LevelActor* actor = feature->findActor( BIT(curTrunPlayer->getId()) , BIT( ActorType::eShepherd ) );
		if ( actor == nullptr )
			return TurnResult::eOK;

		GameSelectActionOptionData data;
		data.options.push_back( ACTOPT_SHEPHERD_EXPAND_THE_FLOCK );
		data.options.push_back( ACTOPT_SHEPHERD_HERD_THE_FLOCK_INTO_THE_STABLE );
		input.requestSelectActionOption( data );
		if ( checkGameState( data , result ) == false )
			return result;

		if ( data.resultIndex >= data.options.size() )
		{
			CAR_LOG("Warning");
			data.resultIndex = 0;
		}

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
				while( ShepherdActor* shepherd = static_cast< ShepherdActor* >( feature->iteratorActorMask( AllPlayerMask , BIT( ActorType::eShepherd ) , iter ) ) )
				{
					for( int i = 0 ; i < (int)shepherd->ownSheep.size() ; ++i )
					{
						score += shepherd->ownSheep[i];
						mSheepBags.push_back( shepherd->ownSheep[i] );
					}
					shepherds.push_back( shepherd );
				}
				for( int i = 0 ; i < shepherds.size() ; ++i )
				{
					ShepherdActor* shepherd = shepherds[i];
					modifyPlayerScore( shepherd->owner->getId() , score );
					returnActorToPlayer( shepherd );
				}
			}
			break;
		}

		return TurnResult::eOK;
	}

	TurnResult GameLogic::resolveTower(IGameInput& input , PlayerBase* curTurnPlayer , bool& haveDone )
	{
		TurnResult result;
		if ( curTurnPlayer->getFieldValue( FieldType::eTowerPices ) == 0 || mTowerTiles.empty() )
			return TurnResult::eOK;

		GameSelectMapTileData data;
		data.reason = SAR_CONSTRUCT_TOWER;
		data.numSelection = mTowerTiles.size();
		data.mapTiles   = &mTowerTiles[0];
		input.requestSelectMapTile( data );
		if ( checkGameState( data , result ) == false )
			return result;

		if ( data.resultSkipAction == true )
			return TurnResult::eOK;

		if ( data.checkResult() == false )
		{
			CAR_LOG("Warning: Error ContructTower Index" );
			data.resultIndex = 0;
		}

		MapTile* mapTile = mTowerTiles[ data.resultIndex ];
		mapTile->towerHeight += 1;
		curTurnPlayer->modifyFieldValue( FieldType::eTowerPices , -1 );

		haveDone = true;
		mListener->onConstructTower( *mapTile );

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

				for( int n = 0 ; n < tileCheck->mActors.size() ; ++n )
				{
					LevelActor* actor = tileCheck->mActors[n];

					if ( mSetting->isFollower( actor->type ) == false )
						continue;

					if ( actor->owner == curTurnPlayer )
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
			input.requestSelectActor( selectActorData );
			if ( checkGameState( selectActorData , result ) == false )
				return result;

			if ( selectActorData.resultSkipAction == true )
				return TurnResult::eOK;

			if ( selectActorData.checkResult() == false )
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
					input.requestSelectActorInfo( data );
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
				curTurnPlayer->modifyActorValue( infoExchange.type , 1 );
				mPrisoners.erase( mPrisoners.begin() + idxExchange );
				CAR_LOG("Exchange Prisoners [%d %d] <-> [%d %d]" , info.playerId , info.type , infoExchange.playerId , infoExchange.type );

			}
		}

		return TurnResult::eOK;
	}


	TurnResult GameLogic::resolveMoveFairyToNextFollower(IGameInput &input, PlayerBase* curTrunPlayer , bool& haveDone)
	{
		TurnResult result;
		ActorList followers;
		int numFollower = getFollowers( BIT( curTrunPlayer->getId() ) , followers , mFairy->binder );
		if ( numFollower == 0 )
			return TurnResult::eOK;

		GameSelectActorData data;
		data.reason = SAR_FAIRY_MOVE_NEXT_TO_FOLLOWER;
		data.actors = &followers[0];
		data.numSelection = numFollower;
		input.requestSelectActor( data );
		if ( !checkGameState( data , result ) )
			return result;

		if ( data.resultSkipAction == true )
			return TurnResult::eOK;

		if ( data.checkResult() == false )
		{
			CAR_LOG( "Warning: Error Fairy Select Actor Index !" );
			data.resultIndex = 0;
		}
		LevelActor* actor = data.actors[ data.resultIndex ];
		actor->addFollower( *mFairy );
		moveActor( mFairy , ActorPos( ActorPos::eTile , 0 ) , actor->mapTile );

		haveDone = true;

		return TurnResult::eOK;
	}

	TurnResult GameLogic::resolveAuction(IGameInput& input , PlayerBase* curTurnPlayer)
	{
		TurnResult result;
		if ( getRemainingTileNum() < mPlayerOrders.size() )
			return TurnResult::eOK;

		for( int i = 0 ; i < mPlayerOrders.size() ; ++i )
		{
			if ( mPlayerOrders[ i ] ->getFieldValue( FieldType::eTileIdAuctioned ) != FAIL_TILE_ID )
				return TurnResult::eOK;
		}

		GameAuctionTileData data;
		for( int i = 0 ; i < mPlayerOrders.size() ; ++i )
		{
			data.auctionTiles.push_back( drawPlayTile() );
		}

		int idxRound = curTurnPlayer->mPlayOrder;
		for( int i = 0 ; i < mPlayerOrders.size() ; ++i )
		{
			do 
			{
				++idxRound;
				if ( idxRound >= mPlayerOrders.size() )
					idxRound = 0;
			} 
			while ( mPlayerOrders[ idxRound ]->getFieldValue( FieldType::eTileIdAuctioned ) != FAIL_TILE_ID );

			if ( data.auctionTiles.size() == 1 )
			{
				mPlayerOrders[ idxRound ]->setFieldValue( FieldType::eTileIdAuctioned , data.auctionTiles[0] );
			}
			else
			{
				data.pIdRound = mPlayerOrders[ idxRound ]->getId();
				data.maxScore = 0;
				data.pIdCallMaxScore = -1;
				data.tileIdRound = FAIL_TILE_ID;

				int idxCur = idxRound;
				for( int n = 0 ; n < mPlayerOrders.size() ; ++n )
				{
					if ( mPlayerOrders[ idxCur ]->getFieldValue( FieldType::eTileIdAuctioned ) == FAIL_TILE_ID )
					{
						data.playerId = mPlayerOrders[ idxCur ]->getId();
						data.resultRiseScore = 0;

						input.requestAuctionTile( data );
						if ( checkGameState( data , result ) == false )
							return result;

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
					if ( idxCur >= mPlayerOrders.size() )
						idxCur = 0;
				}

				bool haveBuy = true;
				if ( data.pIdCallMaxScore != data.pIdRound )
				{
					data.playerId = data.pIdRound;
					input.requestBuyAuctionedTile( data );
					if ( checkGameState( data , result ) == false )
						return result;
					haveBuy = !data.resultSkipAction; 
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

		return TurnResult::eOK;
	}


	TurnResult GameLogic::resolveFeatureReturnPlayerActor(IGameInput& input , FeatureBase& feature)
	{
		TurnResult result;

		//LevelActor* actor;
		std::vector< LevelActor* > wagonGroup;
		std::vector< LevelActor* > mageWitchGroup;
		std::vector< LevelActor* > hereticMonkGroup;
		std::vector< LevelActor* > otherGroup;
		for( int i = 0; i < feature.mActors.size(); ++i )
		{
			LevelActor* actor = feature.mActors[i];
			PlayerBase* player = actor->owner;
			if ( player  )
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
			else
			{
				//#TODO: 
			}
		}
		//l)

		if ( mSetting->have( Rule::eWagon ) && !wagonGroup.empty() )
		{
			GroupSet linkFeatureGroups;
			feature.generateRoadLinkFeatures( linkFeatureGroups );

			std::vector< FeatureBase* > linkFeatures;
			for( auto group : linkFeatureGroups )
			{
				FeatureBase* featureLink = getFeature( group );
				if ( featureLink->checkComplete() )
					continue;
				if ( featureLink->havePlayerActorMask( AllPlayerMask , mSetting->getFollowerMask() ) )
					continue;
				linkFeatures.push_back( featureLink );
			}

			if ( linkFeatures.empty() == false )
			{
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

					input.requestSelectMapTile( data );
					if ( checkGameState( data , result ) == false )
						return result;

					if ( data.resultSkipAction == true )
					{
						returnActorToPlayer( wagonGroup[i] );
					}
					else 
					{
						if ( data.checkResult() == false )
						{
							CAR_LOG("Warning: Error Wagon Move Tile Map Index");
							data.resultIndex = 0;
						}

						FeatureBase* selectedFeature = data.getResultFeature();
						ActorPos actorPos;
						bool isOK = selectedFeature->getActorPos( *mapTiles[ data.resultIndex ] , actorPos );
						assert( isOK );
						moveActor( wagonGroup[i] , actorPos , mapTiles[data.resultIndex] );
						selectedFeature->addActor( *wagonGroup[i] );
						linkFeatures.erase( std::find( linkFeatures.begin() , linkFeatures.end() , selectedFeature ) );
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

	TurnResult GameLogic::resolvePrincess(IGameInput& input , MapTile* placeMapTile , bool& haveDone)
	{
		TurnResult result;

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
			unsigned followerMask = mSetting->getFollowerMask();

			int iterActor = 0;
			ActorList kinghts;
			while( LevelActor* actor = city->iteratorActorMask(AllPlayerMask, KINGHT_MASK, iterActor) )
			{
				kinghts.push_back( actor );
			}

			if ( kinghts.empty() == false )
			{
				GameSelectActorData data;
				data.reason   = SAR_PRINCESS_REMOVE_KINGHT;
				data.numSelection = kinghts.size();
				data.actors   = &kinghts[0];

				input.requestSelectActor( data );
				if ( checkGameState( data , result ) == false )
					return result;

				if ( data.resultSkipAction == false )
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
		return TurnResult::eOK;
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
						build = updateBasicSideFeature( mapTile , linkMask , linkType );
					}
					break;
				case SideType::eGermanCastle:
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
					addUpdateFeature( build , bAbbeyUpdate );
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
								FeatureUpdateInfoVec::iterator iterNew = std::find_if( mUpdateFeatures.begin() , mUpdateFeatures.end() , FindFeature( featureNew ) );
								FeatureUpdateInfoVec::iterator iterMerged = std::find_if( mUpdateFeatures.begin() , mUpdateFeatures.end() , FindFeature( featureMerged[i] ) );

								if ( iterNew->bAbbeyUpdate )
									iterNew->bAbbeyUpdate = iterMerged->bAbbeyUpdate;
								mUpdateFeatures.erase( iterMerged );
							}
						}
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
				for( auto iter = TBitMaskIterator< FDir::TotalNum >(cityLinkMask); iter; ++iter )
				{
					int dir = iter.index;
					if ( mapTile.getSideGroup( dir ) == -1 )
					{
						TileSet const& tileSet = mTileSetManager.getTileSet( mapTile.getId() );
						CAR_LOG("Error:Farm Side Mask Error Tile exp=%d index =%d" , (int)tileSet.expansions , tileSet.idxDefine );
						continue;
					}
					CityFeature* city = static_cast< CityFeature* >( getFeature( mapTile.getSideGroup( dir ) ) );
					assert( city->type == FeatureType::eCity );
					city->linkedFarms.insert( farm );
					farm->linkedCities.insert( city );
				}

			}
		}

		if ( mapTile.have( TileContent::eCloister ) )
		{
			CloisterFeature* cloister = createFeatureT< CloisterFeature >();
			cloister->addMapTile( mapTile );
			mapTile.group = cloister->group;

			bool haveUpdate = false;
			for( int i = 0 ; i < FDir::NeighborNum ; ++i )
			{
				Vec2i posCheck = mapTile.pos + FDir::NeighborOffset(i);
				MapTile* dataCheck = mWorld.findMapTile( posCheck );
				if ( dataCheck )
				{
					haveUpdate |= cloister->updateForNeighborTile( *dataCheck );
				}
			}
			if ( haveUpdate )
				mUpdateFeatures.push_back( FeatureUpdateInfo( cloister ) );
		}

		for( int i = 0; i < FDir::NeighborNum ; ++i )
		{
			Vec2i posCheck = mapTile.pos + FDir::NeighborOffset(i);
			MapTile* dataCheck = mWorld.findMapTile( posCheck );
			if ( dataCheck && dataCheck->group != ERROR_GROUP_ID )
			{
				FeatureBase* feature = getFeature( dataCheck->group );
				if ( feature->updateForNeighborTile( mapTile ) )
				{
					mUpdateFeatures.push_back( FeatureUpdateInfo(feature) );
				}
			}
		}
	}

	void GameLogic::addFeaturePoints( FeatureBase& feature , std::vector< FeatureScoreInfo >& featureControls , int numPlayer )
	{
		for( int i = 0 ; i < numPlayer ; ++i )
		{
			FeatureScoreInfo& info = featureControls[i];
			modifyPlayerScore( info.playerId, info.score );
			CAR_LOG( "Player %d add Score %d " , info.playerId, info.score );
		}
	}

	int GameLogic::updatePosibleLinkPos( PutTileParam& param )
	{
		mPlaceTilePosList.clear();
		return mWorld.getPosibleLinkPos( mUseTileId , mPlaceTilePosList , param );
	}

	int GameLogic::updatePosibleLinkPos()
	{
		PutTileParam param;
		param.usageBridge = 0;
		param.checkRiverConnect = 1;
		return updatePosibleLinkPos( param );
	}

	int GameLogic::findTagTileIndex( std::vector< TileId >& tiles , Expansion exp , TileTag tag )
	{
		for( int i = 0 ; i < tiles.size() ; ++i )
		{
			TileSet const& tileSet = mTileSetManager.getTileSet( tiles[i] );
			if ( tileSet.expansions == exp && tileSet.tag == tag )
				return i;
		}
		return -1;
	}

	bool GameLogic::checkGameState(GameActionData& actionData , TurnResult& result)
	{
		if ( mbNeedShutdown )
		{
			result = TurnResult::eExitGame;
			return false;
		}
		if ( !mIsRunning )
		{
			result = TurnResult::eFinishGame;
			return false;
		}
		return true;
	}

	void GameLogic::shuffleTiles(TileId* begin , TileId* end)
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

	void GameLogic::calcPlayerDeployActorPos(PlayerBase& player , MapTile* deplyMapTiles[] , int numDeployTile , unsigned actorMask , bool haveUsePortal )
	{

		mActorDeployPosList.clear();
		for( int i = 0 ; i < numDeployTile ; ++i )
		{
			getActorPutInfo(player.getId(), *deplyMapTiles[i], haveUsePortal, mActorDeployPosList);
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

	FeatureBase* GameLogic::updateBasicSideFeature( MapTile& mapTile , unsigned dirLinkMask , SideType linkType )
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
		}

		if ( sideFeature->calcOpenCount() != sideFeature->openCount )
		{
			CAR_LOG( "Warning: group=%d type=%d Error OpenCount!" , sideFeature->group , sideFeature->type );
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

	int GameLogic::getActorPutInfo(int playerId , MapTile& mapTile , bool haveUsePortal , std::vector< ActorPosInfo >& outInfo )
	{
		int result = 0;
		if( mapTile.isHalflingType() )
		{
			//TODO


		}

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

				if( feature->checkComplete() )
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
				if( feature->checkComplete() )
					continue;
				result += feature->getActorPutInfo(playerId, idx, mapTile, outInfo);
			}

			if( mSetting->have(Rule::eBarn) )
			{
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
				if( feature->checkComplete() == false )
					result += feature->getActorPutInfo(playerId, 0, mapTile, outInfo);
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
				if ( farm->haveActorMask( BIT( ActorType::eShepherd )) )
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

	void GameLogic::getFeatureNeighborMapTile( FeatureBase& feature , MapTileSet& outMapTile)
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
		if ( mPlayerOrders.empty() )
			return 0;

		int numPlayer = 0;
		PlayerBase* player = mPlayerOrders[0];
		maxValue = player->getFieldValue(type);
		outPlayer[++numPlayer] = player;
		
		for( int i = 1 ; i < mPlayerOrders.size() ; ++i )
		{
			player = mPlayerOrders[i];
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
		mActorList.push_back( actor );
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
		mActorList.erase( std::find( mActorList.begin() , mActorList.end() , actor ));
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

	int GameLogic::modifyPlayerScore(int id , int value)
	{
		PlayerBase* player = mPlayerManager->getPlayer( id );
		player->mScore += value;
		return player->mScore;
	}

	int GameLogic::getFollowers(unsigned playerIdMask , ActorList& outActors , LevelActor* actorSkip )
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

	void GameLogic::returnActorToPlayer(LevelActor* actor)
	{
		assert( actor->owner );
		PlayerBase* player = actor->owner;
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
					unsigned playerMask = BIT(actor->owner->getId());
					unsigned actorMask = mSetting->getFollowerMask() & ~CANT_FOLLOW_ACTOR_MASK;
					do
					{
						newFollower = actor->feature->iteratorActorMask( playerMask , actorMask , iter );
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

	bool GameLogic::addUpdateFeature( FeatureBase* feature , bool bAbbeyUpdate )
	{
		FeatureUpdateInfoVec::iterator iter =
			std::find_if( mUpdateFeatures.begin() , mUpdateFeatures.end() , FindFeature( feature ) );
		if ( iter != mUpdateFeatures.end() )
		{
			if ( iter->bAbbeyUpdate )
				iter->bAbbeyUpdate = bAbbeyUpdate;

			return false;
		}
		else
		{
			mUpdateFeatures.push_back( FeatureUpdateInfo( feature , bAbbeyUpdate ) );
			return true;
		}
	}

	void GameLogic::moveActor(LevelActor* actor , ActorPos const& pos , MapTile* mapTile)
	{
		assert( actor );
		actor->pos = pos;
		if ( mapTile )
			mapTile->addActor( *actor );
		else if ( actor->mapTile )
			actor->mapTile->removeActor( *actor );

		for( int i = 0 ; i < actor->followers.size(); ++i )
		{
			LevelActor* followActor = actor->followers[i];
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

	void GameLogic::FillActionData( GameFeatureTileSelectData& data , std::vector< FeatureBase* >& linkFeatures, std::vector< MapTile* >& mapTiles)
	{
		for( int n = 0; n < linkFeatures.size() ; ++n )
		{
			GameFeatureTileSelectData::Info info;
			info.feature = linkFeatures[n];
			info.index   = mapTiles.size();
			info.num     = 0;
			for( MapTile* mapTile : info.feature->mapTiles)
			{
				if ( !canDeployFollower( *mapTile ) )
					continue;

				++info.num;
				mapTiles.push_back( mapTile );
			}

			data.infos.push_back( info );
		}
		data.mapTiles = &mapTiles[0];
		data.numSelection = mapTiles.size();
	}

	void GameLogic::placeAllTileDebug( int numRow )
	{
		PutTileParam param;
		param.checkRiverConnect = 0;
		param.usageBridge = 0;

		for( int i = 0 ; i < mTileSetManager.getRegisterTileNum() ; ++i )
		{
			MapTile* placeMapTiles[ 8 ];
			int numMapTile = mWorld.placeTileNoCheck( i , Vec2i( 2 *(i/numRow),2*(i%numRow) ) , 0 , param  , placeMapTiles );
			mListener->onPutTile( i , placeMapTiles , numMapTile );
			for( int i = 0 ; i < numMapTile ; ++i )
			{
				UpdateTileFeatureResult updateResult;
				updateTileFeature( *placeMapTiles[i] , updateResult );
			}
		}
	}

	TurnResult GameLogic::resolveAbbey(IGameInput& input , PlayerBase* curTurnPlayer)
	{
		TurnResult result;
		assert( curTurnPlayer->getFieldValue( FieldType::eAbbeyPices ) > 0 );
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
			input.requestSelectMapPos( data );

			if ( checkGameState( data , result ) == false )
				return result;
		}

		return TurnResult::eOK;
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

			if ( feature.testInRange( info->min , info->max ) == false )
				continue;

			CastleScoreInfo s;
			s.feature = &feature;
			s.value   = score;
			info->featureScores.push_back( s );
		}
	}

	TurnResult GameLogic::resolveBuildCastle(IGameInput& input , FeatureBase& feature , bool& haveBuild)
	{
		haveBuild = false;

		TurnResult result;
		CityFeature& city = static_cast< CityFeature& >( feature );
		if ( city.isSamllCircular() == false || city.isCastle == true )
			return TurnResult::eOK;

		LevelActor* kinght = city.findActor( KINGHT_MASK );
		if ( kinght == nullptr || kinght->owner->getFieldValue( FieldType::eCastleTokens ) <= 0 )
			return TurnResult::eOK;

		GameBuildCastleData data;
		data.playerId = kinght->owner->getId();
		data.city = &city;

		input.requestBuildCastle( data );
		if ( checkGameState( data , result ) == false )
			return result;

		if ( data.resultSkipAction == false )
		{
			data.city->isCastle = true;
			kinght->owner->modifyFieldValue( FieldType::eCastleTokens , -1 );
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
		return result;
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

	bool GameLogic::buyBackPrisoner(int ownerId , ActorType type)
	{
		if ( getTurnPlayer() == nullptr )
			return false;

		int i = 0 ;
		for( ; i < mPrisoners.size() ; ++i )
		{
			PrisonerInfo& info = mPrisoners[i];
			if ( info.playerId == getTurnPlayer()->getId() &&
				info.ownerId == ownerId &&
				info.type == type )
				break;
		}
		if ( i == mPrisoners.size() )
			return false;

		mPrisoners.erase( mPrisoners.begin() + i );

		modifyPlayerScore(ownerId, CAR_PARAM_VALUE(PrisonerBuyBackScore));
		modifyPlayerScore(getTurnPlayer()->getId(), CAR_PARAM_VALUE(PrisonerBuyBackScore));
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

		int idx = mRandom.getInt() % mSheepBags.size();

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
			while( ShepherdActor* shepherd = static_cast< ShepherdActor* >( farm->findActor( BIT( ActorType::eShepherd ) ) ) )
			{
				for( int i = 0 ; i < shepherd->ownSheep.size() ; ++i )
				{
					mSheepBags.push_back( shepherd->ownSheep[i] );
				}
				returnActorToPlayer( shepherd );
			}
		}
		else
		{
			static_cast< ShepherdActor* >( actor )->ownSheep.push_back( tok );
		}
	}

	void GameLogic::updateBarnFarm(FarmFeature* farm)
	{
		bool haveFarmer = farm->haveActorMask( FARMER_MASK );
		if ( haveFarmer == false )
			return;

		std::vector< FeatureScoreInfo > scoreInfos;
		FeatureBase::initFeatureScoreInfo( scoreInfos );
		farm->addMajority( scoreInfos );
		int numPlayer = farm->evalMajorityControl( scoreInfos );
		assert( numPlayer > 0 );
		for( int i = 0; i < numPlayer ; ++i )
		{
			scoreInfos[i].score = farm->calcPlayerScoreByBarnRemoveFarmer( scoreInfos[i].playerId );
		}
		addFeaturePoints( *farm , scoreInfos , numPlayer );
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

	bool GameLogic::checkHaveBuilderFeatureExpend(PlayerBase* curTrunPlayer)
	{
		for( FeatureUpdateInfoVec::iterator iter = mUpdateFeatures.begin(), itEnd = mUpdateFeatures.end();
			iter != itEnd ; ++iter )
		{
			FeatureBase* feature = iter->feature;
			if ( feature->group == ERROR_GROUP_ID )
				continue;

			if ( feature->type != FeatureType::eRoad && feature->type != FeatureType::eCity )
				continue;

			if ( feature->havePlayerActor( curTrunPlayer->getId() , ActorType::eBuilder ) == false )
				continue;
	
			if ( mSetting->have( Rule::eHaveAbbeyTile ) )
			{
				if ( iter->bAbbeyUpdate )
					continue;
			}

			return true;
		}	

		return false;
	}

	void GameplaySetting::calcUsageField( int numPlayer )
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