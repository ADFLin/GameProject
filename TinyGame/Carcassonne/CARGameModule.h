#ifndef CARGameModule_h__d9620f2c_1cee_4ed1_9a0e_f89f5a952f42
#define CARGameModule_h__d9620f2c_1cee_4ed1_9a0e_f89f5a952f42

#include "CARCommon.h"

#include "CARWorldTileManager.h"
#include "CARFeature.h"

#include "Random.h"
#include "IntrList.h"

#include <algorithm>

namespace CAR
{
	class GameSetting;
	class FeatureBase;
	class IGameInput;
	struct GameActionData;

	class GamePlayerManager
	{
	public:
		GamePlayerManager();
		int         getPlayerNum(){ return mNumPlayer; }
		PlayerBase* getPlayer( int id ){ return mPlayerMap[id];}
		void        addPlayer( PlayerBase* player );
		void        clearAllPlayer( bool bDelete );


		PlayerBase* mPlayerMap[ MaxPlayerNum ];
		int mNumPlayer;
	};

	class GameRandom
	{
	public:
		int getInt()
		{
			return ::rand();
		}
	};

	class IGameEventListener
	{
	public:
		virtual void onPutTile( TileId id , MapTile* mapTiles[] , int numMapTile ){}
		virtual void onDeployActor( LevelActor& actor ){}
		virtual void onActorMove( LevelActor& actor , MapTile* oldMapTile ){}
		virtual void onConstructTower( MapTile& mapTile ){}
	};

	enum TurnResult
	{
		eOK          ,
		eFinishGame  ,
		eExitGame    ,
	};

	class GameModule
	{
	public:
		GameModule();

		void   setupSetting( GameSetting& setting );
		void   restart( bool bInit );
		WorldTileManager& getLevel(){ return mLevel; }
		void   runLogic( IGameInput& input );

		bool   buildBridge( Vec2i const& pos , int dir );
		bool   buyBackPrisoner( int ownerId , ActorType type );
		bool   changePlaceTile( TileId id );

		void   doRunLogic( IGameInput& input );

		
		void   loadSetting( bool bInit );
		void   calcPlayerDeployActorPos(PlayerBase& player , MapTile& mapTile , unsigned actorMask , bool bUsageMagicPortal );
		int    getRemainingTileNum();
		TileId drawPlayTile();

		void   reserveTile( TileId id );
		void   checkReservedTileToMix();

		void   generatePlayerPlayOrder();
		TileId generatePlayTile();

		void   addFeaturePoints( FeatureBase& build , std::vector< FeatureScoreInfo >& featureControls , int numPlayer );
		int    addPlayerScore( int id , int value );
		void   calcFinalScore();

		struct TrunContext
		{

		};

		TurnResult resolvePlayerTurn( IGameInput& input , PlayerBase* curTrunPlayer );
		TurnResult resolveDeployActor( IGameInput& input , PlayerBase* curTrunPlayer, MapTile* deployMapTile, unsigned actorMask , bool haveUsePortal , bool& haveDone );
		TurnResult resolveMoveFairyToNextFollower( IGameInput &input, PlayerBase* curTrunPlayer , bool& haveDone );
		TurnResult resolvePlaceTile(IGameInput& input , PlayerBase* curTrunPlayer , MapTile* placeMapTile[] , int& numMapTile );
		TurnResult resolvePortalUse( IGameInput& input, PlayerBase* curTrunPlayer, MapTile*& deployMapTile, bool& haveUsePortal);
		TurnResult resolveExpendShepherdFarm( IGameInput& input , PlayerBase* curTrunPlayer , FeatureBase* feature );
		TurnResult resolveCastleComplete( IGameInput& input );
		TurnResult resolveDrawTile( IGameInput& input , PlayerBase* curTrunPlayer , bool& haveHillTile );
		
		struct CastleScoreInfo;
		TurnResult resolveCompleteFeature( IGameInput& input , FeatureBase& feature , CastleScoreInfo* castleScore );

		TurnResult resolveBuildCastle(IGameInput& input , FeatureBase& feature , bool& haveBuild );
		TurnResult resolveAbbey( IGameInput& input , PlayerBase* curTurnPlayer );
		TurnResult resolveDragonMove( IGameInput& input , LevelActor& dragon );
		TurnResult resolvePrincess( IGameInput& input , MapTile* placeMapTile , bool& haveDone );
		TurnResult resolveTower(IGameInput& input , PlayerBase* curTurnPlayer , bool& haveDone );
		TurnResult resolveAuction(IGameInput& input , PlayerBase* curTurnPlayer );
		TurnResult resolveFeatureReturnPlayerActor( IGameInput& input , FeatureBase& feature );
		

		struct UpdateTileFeatureResult
		{
			UpdateTileFeatureResult()
			{

			}
		};

		void updateTileFeature( MapTile& mapTile , UpdateTileFeatureResult& updateResult );
		void checkCastleComplete(FeatureBase &feature, int score);
		void expandSheepFlock(LevelActor* actor);
		bool checkHaveBuilderFeatureExpend( PlayerBase* curTrunPlayer );
		void updateBarnFarm(FarmFeature* farm);
		bool checkGameState( GameActionData& actionData , TurnResult& result );

		static bool canDeployFollower( MapTile const& mapTile )
		{
			if ( mapTile.towerHeight != 0 )
				return false;
			return true;
		}


		PlayerBase* getTurnPlayer(){ return mPlayerOrders[ mIdxPlayerTrun ]; }

		void   initFeatureScoreInfo(std::vector< FeatureScoreInfo > &scoreInfos);
	
		void  returnActorToPlayer( LevelActor* actor );
		void  moveActor( LevelActor* actor , ActorPos const& pos , MapTile* mapTile );

		void  shuffleTiles( TileId* begin , TileId* end );
		int   findTagTileIndex( std::vector< TileId >& tiles , Expansion exp , TileTag tag );

		int   getActorPutInfo(int playerId , MapTile& mapTile , bool bUsageMagicPortal , std::vector< ActorPosInfo >& outInfo);


		void   getFeatureNeighborMapTile( FeatureBase& feature , MapTileSet& outMapTile );
		int    getMaxFieldValuePlayer( FieldType::Enum type , PlayerBase* outPlayer[] , int& maxValue );
		int    updatePosibleLinkPos( PutTileParam& param );
		int    updatePosibleLinkPos();
		typedef MapTile::FarmNode FarmNode;
		typedef MapTile::SideNode SideNode;

		static void FillActionData( struct GameFeatureTileSelectData& data , std::vector< FeatureBase* >& linkFeatures, std::vector< MapTile* >& mapTiles );


		FarmFeature*  updateFarm( MapTile& mapTile , unsigned idxLinkMask );
		FeatureBase*  updateBasicSideFeature( MapTile& mapTile , unsigned dirLinkMask , SideType linkType );
		SideFeature*  mergeHalfSeparateBasicSideFeature( MapTile& mapTile , int dir , FeatureBase* featureMerged[] , int& numMerged );

		FeatureBase*  getFeature( int group );
		template< class T >
		T*         createFeatureT();
		void      destroyFeature( FeatureBase* build );

		std::vector< FeatureBase* > mFeatureMap;
		int mIndexCacheBuild;

		int getFollowers( unsigned playerIdMask , ActorList& outActors , LevelActor* actorSkip  = nullptr );
		LevelActor* createActor( ActorType type );
		void destroyActor( LevelActor* actor );


		void deleteActor( LevelActor* actor );
		
		
		void cleanupData();


		//
		GamePlayerManager* mPlayerManager;
		TileSetManager     mTileSetManager;
		GameSetting*       mSetting;
		GameRandom         mRandom;
		WorldTileManager              mLevel;
		
		IGameEventListener* mListener;
		bool     mDebug;

		ActorList mActorList;

		struct Team
		{
			int score;
		};

		int    mNumTrun;
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
			FeatureUpdateInfo( FeatureBase* aFeature , bool abAbbeyUpdate = false )
				:feature( aFeature )
				,bAbbeyUpdate( abAbbeyUpdate )
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
		typedef IntrList< 
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

	};

}//namespace CAR

#endif // CARGameModule_h__d9620f2c_1cee_4ed1_9a0e_f89f5a952f42