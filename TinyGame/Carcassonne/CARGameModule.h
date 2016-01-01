#ifndef CARGameModule_h__d9620f2c_1cee_4ed1_9a0e_f89f5a952f42
#define CARGameModule_h__d9620f2c_1cee_4ed1_9a0e_f89f5a952f42

#include "CARCommon.h"

#include "CARLevel.h"
#include "CARFeature.h"

#include "Random.h"
#include "IntrList.h"

namespace CAR
{
	class GameSetting;
	class FeatureBase;

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

	struct GameActionData
	{
		GameActionData()
		{
			playerId = -1;
			resultExitGame = false;
			resultSkipAction = false;
		}

		template< class T >
		T* cast(){ return static_cast< T* >( this ); }
		int  playerId;
		bool resultExitGame;
		bool resultSkipAction;
	};


	struct GamePlaceTileData : public GameActionData
	{
		TileId id;

		Vec2i  resultPos;
		int    resultRotation;
	};

	struct GameDeployActorData : public GameActionData
	{
		MapTile*  mapTile;
		ActorType resultType;
		int       resultIndex;
	};

	enum SelectActionReason
	{
		//MapTile
		SAR_CONSTRUCT_TOWER ,
		SAR_MOVE_DRAGON ,
		SAR_MAGIC_PORTAL ,
		//
		SAR_WAGON_MOVE_TO_FEATURE ,
		//Actor
		SAR_FAIRY_MOVE_NEXT_TO_FOLLOWER ,
		SAR_TOWER_CAPTURE_FOLLOWER ,
		SAR_PRINCESS_REMOVE_KINGHT ,
		//
		SAR_EXCHANGE_PRISONERS ,
	};



	struct GameSelectActionData : public GameActionData
	{
		GameSelectActionData()
		{
			resultIndex = 0;
		}
		SelectActionReason reason;
		unsigned resultIndex;
		unsigned numSelection;

		bool canSkip()
		{
			if ( reason == SAR_MOVE_DRAGON ||
				 reason == SAR_EXCHANGE_PRISONERS ||
				 reason == SAR_MAGIC_PORTAL )
				return false;
			return true;
		}

		bool checkResultVaild() const
		{
			return resultIndex < numSelection;
		}
	};

	struct GameSelectMapTileData : public GameSelectActionData
	{
		MapTile** mapTiles;	
	};

	struct GameSelectActorData : public GameSelectActionData
	{
		LevelActor** actors;
	};

	struct GameSelectFeatureData : public GameSelectActionData
	{
		LevelActor** features;
	};

	struct ActorInfo
	{
		int       playerId;
		ActorType type;

		bool operator == ( ActorInfo const& rhs ) const
		{
			return playerId == rhs.playerId &&
				   type == rhs.type;
		}
	};

	struct GameSelectActorInfoData : public GameSelectActionData
	{
		ActorInfo* actorInfos;
	};

	struct GameDragonMoveData : public GameSelectMapTileData
	{
	public:
	};
	struct GameFeatureTileSelectData : public GameSelectMapTileData
	{
		struct Info
		{
			FeatureBase* feature;
			int index;
			int num;
		};
		std::vector< Info > infos;
		FeatureBase* getResultFeature()
		{
			for( int i = 0 ;i < infos.size() ; ++i )
			{
				Info& info = infos[i];
				if ( resultIndex < info.index + info.num )
					return info.feature;
			}
			return nullptr;
		}
	};

	struct GameAuctionTileData : public GameActionData
	{
		std::vector< TileId > auctionTiles;
		
		TileId   tileIdRound;
		int      pIdRound;
		int      pIdCallMaxScore;
		int      maxScore;
		
		unsigned resultIndexTileSelect;
		unsigned resultRiseScore;
	};

	struct GameBuildCastleData : public GameActionData
	{
		CityFeature* city;
	};

	class GameModule;

	class IGameInput
	{
	public:
		virtual void requestPlaceTile( GamePlaceTileData& data ) = 0;
		virtual void requestDeployActor( GameDeployActorData& data ) = 0;
		virtual void requestSelectMapTile( GameSelectMapTileData& data ) = 0;
		virtual void requestSelectActor( GameSelectActorData& data ) = 0;
		virtual void requestSelectActorInfo( GameSelectActorInfoData& data ) = 0;
		virtual void requestTurnOver( GameActionData& data ) = 0;
		virtual void requestAuctionTile( GameAuctionTileData& data ) = 0;
		virtual void requestBuyAuctionedTile( GameAuctionTileData& data ) = 0;
		virtual void requestBuildCastle( GameBuildCastleData& data ) = 0;

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
		virtual void onPutTile( MapTile& mapTile ){}
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
		Level& getLevel(){ return mLevel; }
		void   runLogic( IGameInput& input );

		bool   buildBridge( Vec2i const& pos , int dir );
		bool   buyBackPrisoner( int ownerId , ActorType type );
		bool   changePlaceTile( TileId id );

		void   loadSetting( bool bInit );
		void   calcPlayerDeployActorPos(PlayerBase& player , MapTile& mapTile , bool bUsageMagicPortal );
		int    getRemainingTileNum();
		TileId drawPlayTile();

		void   reserveTile( TileId id );
		void   checkReservedTileToMix();

		void   generatePlayerPlayOrder();
		TileId generatePlayTile();

		void   addFeaturePoints( FeatureBase& build , std::vector< FeatureScoreInfo >& featureControls , int numPlayer );
		int    addPlayerScore( int id , int value );

		struct UpdateTileFeatureResult
		{
			UpdateTileFeatureResult()
			{
				
			}

		};

		void   updateTileFeature( MapTile& mapTile , UpdateTileFeatureResult& updateResult );

		bool   checkGameState( GameActionData& actionData , TurnResult& result );

		TurnResult  resolvePlayerTurn( IGameInput& input , PlayerBase* curTrunPlayer );
		struct CastleScoreInfo;
		TurnResult  resolveCompleteFeature( IGameInput& input , FeatureBase& feature , CastleScoreInfo* castleScore );
		TurnResult  resolveBuildCastle(IGameInput& input , FeatureBase& feature , bool& haveBuild );

		void checkCastleComplete(FeatureBase &feature, int score);

		TurnResult  resolveAbbey( IGameInput& input , PlayerBase* curTurnPlayer );
		TurnResult  resolveDragonMove( IGameInput& input , LevelActor& dragon );
		TurnResult  resolvePrincess( IGameInput& input , MapTile* placeMapTile , bool& haveDone );
		TurnResult  resolveTower(IGameInput& input , PlayerBase* curTurnPlayer , bool& haveDone );
		TurnResult  resolveAuction(IGameInput& input , PlayerBase* curTurnPlayer );


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


		int    getMaxFieldValuePlayer( FieldType::Enum type , PlayerBase* outPlayer[] , int& maxValue );
		int    updatePosibleLinkPos( PutTileParam& param );
		int    updatePosibleLinkPos();
		typedef MapTile::FarmNode FarmNode;
		typedef MapTile::SideNode SideNode;

		static void FillActionData( GameFeatureTileSelectData& data , std::vector< FeatureBase* >& linkFeatures, std::vector< MapTile* >& mapTiles );


		FarmFeature*  updateFarm( MapTile& mapTile , unsigned idxMask );
		FeatureBase*  updateBasicSideFeature( MapTile& mapTile , unsigned dirMask , SideType linkType );

		FeatureBase*  getFeature( int group );
		template< class T >
		T*         createFeatureT()
		{
			T* data = new T;
			data->type     = T::Type;
			data->group    = mFeatureMap.size();
			data->mSetting = mSetting;
			mFeatureMap.push_back( data );
			return data;
		}
		void      destroyFeature( FeatureBase* build );
		void      cleanupFeature();

		std::vector< FeatureBase* > mFeatureMap;
		int mIndexCacheBuild;

		int getFollowers( unsigned playerIdMask , ActorList& outActors , LevelActor* actorSkip  = nullptr );
		LevelActor* createActor( ActorType type );
		void destroyActor( LevelActor* actor );
		void cleanupActor();
	
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

		void addUpdateFeature( FeatureBase* feature , bool bAbbeyUpdate = false );

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
		int mCastleGroup;

		GamePlayerManager* mPlayerManager;
		TileSetManager mTileSetManager;
		GameSetting* mSetting;
		GameRandom   mRandom;
		Level        mLevel;
		bool         mDebug;

		IGameEventListener* mListener;
	};

}//namespace CAR

#endif // CARGameModule_h__d9620f2c_1cee_4ed1_9a0e_f89f5a952f42