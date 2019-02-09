#pragma once
#ifndef CARGameLogic_h__d9620f2c_1cee_4ed1_9a0e_f89f5a952f42
#define CARGameLogic_h__d9620f2c_1cee_4ed1_9a0e_f89f5a952f42

#include "CARCommon.h"

#include "CARWorldTileManager.h"
#include "CARFeature.h"

#include "Random.h"
#include "DataStructure/IntrList.h"

#include <algorithm>

#define CAR_LOGIC_DEBUG 1

namespace CAR
{
	class GameplaySetting;
	class GameParamCollection;
	class GamePlayerManager;
	class FeatureBase;
	class IGameInput;
	struct GameActionData;

	class GameRandom
	{
	public:
		virtual int getInt() = 0;
	};

	class IGameEventListener
	{
	public:
		virtual void onPutTile( TileId id , MapTile* mapTiles[] , int numMapTile ){}
		virtual void onDeployActor( LevelActor& actor ){}
		virtual void onActorMove( LevelActor& actor , MapTile* oldMapTile ){}
		virtual void onConstructTower( MapTile& mapTile ){}
	};

	enum TurnStatus
	{
		eKeep        ,
		eFinishGame  ,
		eExitGame    ,
	};

	namespace EDebugModeMask
	{
		enum
		{
			ShowAllTitles = BIT(0),
			DrawTestTileFrist = BIT(1),
			RemoveBasicTitles = BIT(2),
		};
	}

	class GameLogic : public ActorContainer
	{
	public:
		GameLogic();

		void   setup( GameplaySetting& setting , GameRandom& random , GamePlayerManager& playerManager );
		void   restart( bool bInit );
		void   run(IGameInput& input);

		GameplaySetting&  getSetting() { return *mSetting; }
		WorldTileManager& getWorld(){ return mWorld; }

		
		bool   buildBridge( Vec2i const& pos , int dir );
		bool   redeemPrisoner( int ownerId , ActorType type );
		bool   changePlaceTile( TileId id );

		
		void   loadSetting( bool bInit );
		void   calcPlayerDeployActorPos(PlayerBase& player, MapTile* deplyMapTiles[], int numDeployTile, unsigned actorMask, bool haveUsePortal);
		int    getRemainingTileNum();
		TileId drawPlayTile();

		void   reserveTile( TileId id );
		void   checkReservedTileToMix();

		void   randomPlayerPlayOrder();
		TileId generatePlayTiles();

		enum class EScroeType
		{
			Normal ,
			Woman ,
			None ,
		};
		int    modifyPlayerScore( int playerId, int value , EScroeType scoreType = EScroeType::Normal );
		void   calcFinalScore();

		struct TurnContext
		{
			TurnContext(PlayerBase* inPlayer , IGameInput& inInput)
				:mPlayer(inPlayer),mInput(inInput)
			{
				result = TurnStatus::eKeep;
			}

			PlayerBase* getPlayer() { return mPlayer; }
			IGameInput& getInput() { return mInput; }

			TurnStatus  result;
#if CAR_LOGIC_DEBUG
			std::vector< std::string > returnStack;
#endif
		private:
			PlayerBase* mPlayer;
			IGameInput& mInput;
		};

		struct CastleScoreInfo;

		void resolvePlayerTurn(TurnContext& turnContext );
		void resolveDeployActor(TurnContext& turnContext, MapTile* deployMapTiles[], int numDeployTile, unsigned actorMask, bool haveUsePortal, bool& haveDone);
		void resolveMoveFairyToNextFollower(TurnContext& turnContext, bool& haveDone);
		void resolvePlaceTile(TurnContext& turnContext, MapTile* placeMapTiles[] , int& numMapTile );
		void resolvePortalUse(TurnContext& turnContext, MapTile*& deployMapTile, bool& haveUsePortal);
		void resolveExpendShepherdFarm(TurnContext& turnContext, FeatureBase* feature );
		void resolveCompletedCastles(TurnContext& turnContext , std::vector< FeatureScoreInfo >& featureScoreList );
		void resolveDrawTile(TurnContext& turnContext, bool& haveHillTile );
		void resolveCompletedFeature(TurnContext& turnContext, FeatureBase& completedFeature, FeatureScoreInfo& featureScore, CastleScoreInfo* castleScore);
		void resolveBuildCastle(TurnContext& turnContext, FeatureBase& feature , bool& haveBuild );
		void resolveAbbey(TurnContext& turnContext );
		void resolveDragonMove(TurnContext& turnContext, LevelActor& dragon );
		void resolvePrincess(TurnContext& turnContext, MapTile* placeMapTile , bool& haveDone );
		void resolveTower(TurnContext& turnContext, bool& haveDone );
		void resolveAuction(TurnContext& turnContext );
		void resolveFeatureReturnPlayerActor(TurnContext& turnContext, FeatureBase& feature );
		void resolveLaPorxadaCommand(TurnContext& turnContext);
		void resolveMoveMageOrWitch(TurnContext& turnContext);
		void resolvePlaceGoldPieces(TurnContext& turnContext, MapTile* mapTile);
		void resolveCropCircle(TurnContext& turnContext, FeatureType::Enum cropType );

		void resolveScorePlayer(TurnContext& turnContext, int playerId, int value, EScroeType* forceScoreType = nullptr);
		void resolveDrawMessageTile(TurnContext& turnContext);
		void resolvePlaceLittleBuilding(TurnContext& turnContext, MapTile* placeTiles[] , int numPlaceTile , bool& haveDone);

		struct UpdateTileFeatureResult
		{
			UpdateTileFeatureResult()
			{
				numSideFeatureMerged = 0;
			}

			int numSideFeatureMerged;
		};

		void updateTileFeature( MapTile& mapTile , UpdateTileFeatureResult& updateResult );
		void checkCastleComplete(FeatureBase &feature, int score);
		void expandSheepFlock(LevelActor* actor);
		void updateBarnFarm(FarmFeature* farm, FeatureScoreInfo& featureScore);
		bool checkHaveBuilderFeatureExpend(PlayerBase* trunPlayer);
		bool checkGameStatus( TurnStatus& result );

		PlayerBase* getTurnPlayer(){ return mPlayerOrders[ mIdxPlayerTrun ]; }
	
		void  returnActorToPlayer( LevelActor* actor );
		void  removeActor(LevelActor* actor)
		{
			moveActor(actor, ActorPos::None(), nullptr);
		}
		void  moveActor( LevelActor* actor , ActorPos const& pos , MapTile* mapTile );

		void  shuffleTiles( TileId* begin , TileId* end );

		int   getActorPutInfo(int playerId , MapTile& mapTile , bool bUsageMagicPortal , std::vector< ActorPosInfo >& outInfo);

		void   getMinTitlesNoCompletedFeature(FeatureType::Enum type, unsigned playerMask, unsigned actorTypeMask, std::vector<FeatureBase*>& outFeatures);
		void   getFeatureNeighborMapTile(FeatureBase& feature, MapTileSet& outMapTile);
		int    getMaxFieldValuePlayer( FieldType::Enum type , PlayerBase* outPlayer[] , int& maxValue );
		int    updatePosibleLinkPos( PlaceTileParam& param );
		int    updatePosibleLinkPos();
		typedef MapTile::FarmNode FarmNode;
		typedef MapTile::SideNode SideNode;

		FarmFeature*  updateFarm( MapTile& mapTile , unsigned idxLinkMask );
		FeatureBase*  updateBasicSideFeature( MapTile& mapTile, unsigned dirLinkMask, SideType linkType , UpdateTileFeatureResult& updateResult);
		SideFeature*  mergeHalfSeparateBasicSideFeature(MapTile& mapTile, int dir, FeatureBase* featureMerged[], int& numMerged);

		FeatureBase*  getFeature( int group );
		template< class T >
		T*         createFeatureT();
		void       destroyFeature( FeatureBase* build );

		std::vector< FeatureBase* > mFeatureMap;
		int mIndexCacheBuild;

		int getFollowers( unsigned playerIdMask , ActorList& outActors , LevelActor* actorSkip  = nullptr );
		LevelActor* createActor( ActorType type );
		void destroyActor( LevelActor* actor );


		void deleteActor( LevelActor* actor );
		
		

		void cleanupData();

		PlayerBase* getOwnedPlayer(LevelActor* actor);

		static GameParamCollection& GetParamCollection(GameLogic& logic);
		//
		GamePlayerManager* mPlayerManager;
		TileSetManager     mTileSetManager;
		GameplaySetting*   mSetting;
		GameRandom*        mRandom;
		WorldTileManager   mWorld;
		
		IGameEventListener* mListener;
		unsigned  mDebugMode;

		struct Team
		{
			int score;
		};

		int    mNumTrun;
		bool   mbNeedShutdown;
		bool   mIsRunning;
		bool   mIsStartGame;
		TileId mUseTileId;
		int    mIdxPlayerTrun;
		int    mIdxTile;
		int    mNumTileNeedMix;
		bool   mbCanKeep;

		std::vector< Vec2i >  mPlaceTilePosList;
		std::vector< TileId > mTilesQueue;
		std::vector< PlayerBase* > mPlayerOrders;
		std::vector< ActorPosInfo > mActorDeployPosList;

		bool addUpdateFeature( FeatureBase* feature , bool bAbbeyUpdate = false );
		void placeAllTileDebug( int numRow );
		struct FeatureUpdateInfo
		{
			FeatureUpdateInfo( FeatureBase* inFeature , bool inbAbbeyUpdate = false )
				:feature( inFeature )
				,bAbbeyUpdate( inbAbbeyUpdate )
			{
			}
			FeatureBase* feature;
			bool bAbbeyUpdate;
		};

		struct FindFeature
		{ 
			FindFeature( FeatureBase* aFeature ):feature( aFeature ){}
			bool operator()( FeatureUpdateInfo const& info ) const
			{
				return info.feature == feature;
			}
			FeatureBase* feature;
		};
		
		typedef std::vector< FeatureUpdateInfo > FeatureUpdateInfoVec;
		FeatureUpdateInfoVec mUpdateFeatures;

		//EXP_THE_PRINCESS_AND_THE_DRAGON
		LevelActor*  mDragon;
		LevelActor*  mFairy;

		//EXP_THE_TOWER
		std::vector< MapTile* > mTowerTiles;
		struct PrisonerInfo : public ActorInfo
		{
			int ownerId;
		};
		std::vector< PrisonerInfo > mPrisoners;

		//EXP_KING_AND_ROBBER
		int    mIdKing;
		int    mMaxCityTileNum;
		int    mIdRobberBaron;
		int    mMaxRoadTileNum;

		//EXP_ABBEY_AND_MAYOR
		TileId mAbbeyTileId;

		//EXP_BRIDGES_CASTLES_AND_BAZAARS
		struct CastleScoreInfo
		{
			FeatureBase* feature;
			int value;
		};
		struct CastleInfo
		{
			HookNode node;
			CityFeature* city;
			Vec2i min;
			Vec2i max;
			std::vector< CastleScoreInfo > featureScores;
		};
		typedef TIntrList< 
			CastleInfo , 
			MemberHook< CastleInfo , &CastleInfo::node > , 
			PointerType 
		> CastleInfoList;
		CastleInfoList mCastles;
		CastleInfoList mCastlesRoundBuild;

		//EXP_HILLS_AND_SHEEP
		class ShepherdActor : public LevelActor
		{
		public:
			std::vector< SheepToken > ownSheep;
		};
		std::vector< SheepToken > mSheepBags;
		
		//EXP_CASTLES
		std::vector< GermanCastleFeature* > mGermanCastles;


		//EXP_MAGE_AND_WITCH
		LevelActor* mMage;
		LevelActor* mWitch;

		//EXP_THE_MESSSAGES
		std::vector< EMessageTile::Type > mMessageStack;
		int mIndexTopMessage;
		EMessageTile::Type drawMessageTile();
		//
		bool mbUseLaPorxadaScoring;

	};

}//namespace CAR

#endif // CARGameLogic_h__d9620f2c_1cee_4ed1_9a0e_f89f5a952f42