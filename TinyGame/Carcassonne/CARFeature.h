#ifndef CARFeature_h__b8a5bc79_4d4a_4362_80be_82b096535e7c
#define CARFeature_h__b8a5bc79_4d4a_4362_80be_82b096535e7c

#include "CARCommon.h"
#include "CARMapTile.h"
#include "CARLevelActor.h"

#include "DataStructure/Array.h"

#include <set>
#include <unordered_set>


namespace CAR
{
	class GameParamCollection;
	class GameplaySetting;
	class GamePlayerManager;
	class PlayerBase;
	class WorldTileManager;

	class IWorldQuery
	{
	public:
		virtual FeatureBase* getFeature( int groupId ) = 0;
		virtual WorldTileManager& getTileManager() = 0;
	};

	struct FeatureType
	{
		enum Enum
		{
			eFarm ,
			eCity ,
			eRoad ,
			eCloister ,
			eChrine ,
			eGarden ,
			eGermanCastle ,
			eCityOfCarcassonne ,
		};
	};


	struct ActorPosInfo
	{
		MapTile*  mapTile;
		unsigned  actorTypeMask;
		ActorPos  pos;
		int       group;
	};

	struct FeatureControllerScoreInfo
	{
		int  playerId;
		int  score;
		int  majority;
		int  hillFollowerCount;
	};

	struct FeatureScoreInfo
	{
		FeatureBase* feature;
		int numController;
		TArray< FeatureControllerScoreInfo > controllerScores;

		FeatureScoreInfo()
		{
			numController = 0;
		}
	};

	using MapTileSet = std::unordered_set< MapTile* >;
	using GroupSet   = std::unordered_set< unsigned >;

	class  FeatureBase : public ActorCollection
	{
	public:

		using FarmNode = MapTile::FarmNode;
		using SideNode = MapTile::SideNode;
		FeatureBase()
		{
			userData = nullptr;
		}
		virtual ~FeatureBase() = default;

		int         group;
		int         type;
		MapTileSet  mapTiles;
		void*       userData;

		void        addActor( LevelActor& actor );
		void        removeActor( LevelActor& actor );
		LevelActor* removeActorByIndex( int index );
		bool        testInRectArea( Vec2i const& min , Vec2i const& max );
		int         calcScore(GamePlayerManager& playerManager, FeatureScoreInfo& outResult);

		virtual bool checkComplete() const { return false; }
		virtual int  getActorPutInfo( int playerId , int posMeta , MapTile& mapTile, TArray< ActorPosInfo >& outInfo ) = 0;
		virtual void generateRoadLinkFeatures( WorldTileManager& worldTileManager, GroupSet& outFeatures ){ }
		virtual bool updateForAdjacentTile( MapTile& tile ){ return false; }
		virtual void mergeData( FeatureBase& other , MapTile const& putData , int meta );
		virtual int  calcPlayerScore( PlayerBase* player ) = 0;
		virtual int  doCalcScore( GamePlayerManager& playerManager , FeatureScoreInfo& outResult);
		virtual bool getActorPos( MapTile const& mapTile , ActorPos& actorPos ){ return false; }
		virtual int  getScoreTileNum() const { return 0; }
		virtual void onAddFollower(LevelActor& actor){}

		int  generateController(TArray< FeatureControllerScoreInfo >& controllerScores);
		int  getMajorityValue( EActor::Type actorType);
		void generateMajority( TArray< FeatureControllerScoreInfo >& controllerScores);
		int  evalMajorityControl(TArray< FeatureControllerScoreInfo >& controllerScores);
		void addMapTile( MapTile& mapTile ){ mapTiles.insert( &mapTile ); }
		bool haveTileContent(TileContentMask contentMask) const;

		template< class T >
		static void MergeData( T& to , T const& src )
		{
			to.insert( to.end() , src.begin() , src.end() );
		}

		template< class T >
		static void MergeData(TArray<T>& to, TArray<T> const& src)
		{
			to.append(src);
		}

		template< class T >
		static void MergeData( std::set< T >& to , std::set< T > const& src )
		{
			to.insert( src.begin() , src.end() );
		}

		template< class T >
		static void MergeData(std::unordered_set< T >& to, std::unordered_set< T > const& src)
		{
			to.insert(src.begin(), src.end());
		}

		int getDefaultActorPutInfo( int playerId , ActorPos const& actorPos , MapTile& mapTile, unsigned actorMasks[] , TArray< ActorPosInfo >& outInfo );
		int getActorPutInfoInternal( int playerId , ActorPos const& actorPos , MapTile& mapTile, unsigned actorMasks[] , int numMask , TArray< ActorPosInfo >& outInfo);

		//bool useRule(RuleFunc::Enum ruleFunc)
		//{
		//	return mSetting->haveRule(ruleFunc);
		//}

		static GameParamCollection& GetParamCollection(FeatureBase& t);
		GameplaySetting& getSetting() { return *mSetting; }
		GameplaySetting* mSetting;
	};

	class SideFeature : public FeatureBase
	{
		using BaseClass = FeatureBase;
	public:
		SideFeature();

		TArray< SideNode* > nodes;
		int  openCount;
		int  halfSepareteCount;

		void mergeData( FeatureBase& other , MapTile const& putData , int meta ) override;
		virtual void addNode( MapTile& mapData , unsigned dirMask , SideNode* linkNode );
		virtual void addAbbeyNode( MapTile& mapData , int dir );
		void generateRoadLinkFeatures( WorldTileManager& worldTileManager, GroupSet& outFeatures ) override;
		bool getActorPos( MapTile const& mapTile , ActorPos& actorPos ) override;
		int  getScoreTileNum() const override { return (int)mapTiles.size(); }

		int getSideContentNum(SideContentMask contentMask);
		int getSideContentNum(SideContent::Enum type) { return getSideContentNum(BIT(type)); }
		bool checkNodesConnected() const;

		int calcOpenCount();
	};



	class RoadFeature : public SideFeature
	{
		using BaseClass = SideFeature;
	public:
		static int const Type = FeatureType::eRoad;
		RoadFeature();
		void mergeData( FeatureBase& other , MapTile const& putData , int meta ) override;
		void addNode( MapTile& mapData , unsigned dirMask , SideNode* linkNode ) override;
		int  getActorPutInfo( int playerId , int posMeta , MapTile& mapTile, TArray< ActorPosInfo >& outInfo ) override;
		bool checkComplete() const override;
		int  calcPlayerScore(PlayerBase* player) override;
		int  getScoreTileNum() const override { return mapTiles.size(); }
		void onAddFollower(LevelActor& actor) override { actor.className = EFollowerClassName::Thief; }
		bool      haveInn;
	};

	class FarmFeature;
	class CityFeature : public SideFeature
	{
		using BaseClass = SideFeature;
	public:
		static int const Type = FeatureType::eCity;
		CityFeature();
		
		bool haveCathedral;
		bool haveLaPorxada;
		bool isCastle;
		int  cloisterGroup;
		std::unordered_set< FarmFeature* > linkedFarms;

		bool isSamllCircular() const;
		bool isBesieged() const;
		bool haveAdjacentCloister() const;


		void mergeData( FeatureBase& other , MapTile const& putData , int meta ) override;
		void addNode( MapTile& mapData , unsigned dirMask , SideNode* linkNode ) override;
		int  getActorPutInfo( int playerId , int posMeta , MapTile& mapTile, TArray< ActorPosInfo >& outInfo ) override;
		bool checkComplete() const override;
		int  calcPlayerScore( PlayerBase* player ) override;
		void onAddFollower(LevelActor& actor) override 
		{ 
			actor.className = EFollowerClassName::Knight;
		}
		int doCalcScore( GamePlayerManager& playerManager , FeatureScoreInfo& outResult) override;
	};

	class CityOfCarcassonneFeature : public FeatureBase
	{
	public:
		static int const Type = FeatureType::eCityOfCarcassonne;

		bool checkComplete() const override { return true; }
		int  getActorPutInfo(int playerId, int posMeta, MapTile& mapTile, TArray< ActorPosInfo >& outInfo) override { return 0; }
		void generateRoadLinkFeatures(WorldTileManager& worldTileManager, GroupSet& outFeatures) override {}
		bool updateForAdjacentTile(MapTile& tile) override { return false; }
		void mergeData(FeatureBase& other, MapTile const& putData, int meta) override { NEVER_REACH("City of Carcassonne can't be merged"); }
		int  calcPlayerScore(PlayerBase* player) override { return 0; }
		int  doCalcScore(GamePlayerManager& playerManager, FeatureScoreInfo& outResult) override { return -1; }
		bool getActorPos(MapTile const& mapTile, ActorPos& actorPos) override { return false; }
		int  getScoreTileNum() const override { return 0; }
		void onAddFollower(LevelActor& actor) override {}
	};

	class FarmFeature : public FeatureBase
	{
		using BaseClass = FeatureBase;
	public:
		FarmFeature();

		static int const Type = FeatureType::eFarm;

		void addNode( MapTile& mapData , unsigned idxMask , FarmNode* linkNode );
		void mergeData( FeatureBase& other , MapTile const& putData , int meta ) override;
		int  getActorPutInfo( int playerId , int posMeta , MapTile& mapTile, TArray< ActorPosInfo >& outInfo ) override;
		bool checkComplete() const override { return false; }
		int  doCalcScore( GamePlayerManager& playerManager , FeatureScoreInfo& outResult) override;
		int  calcPlayerScore( PlayerBase* player ) override;
		void onAddFollower(LevelActor& actor) override
		{
			actor.className = EFollowerClassName::Farmer;
		}
		int calcPlayerScoreByBarnRemoveFarmer(PlayerBase* player);
		int calcPlayerScoreInternal(PlayerBase* player, int farmFactor);

		bool haveBarn;
		TArray< FarmNode* > nodes;
		std::unordered_set< FeatureBase* > linkedCities;
	};


	class AdjacentTileScoringFeature : public FeatureBase
	{
	public:
		TArray< MapTile* > neighborTiles;
		bool checkComplete() const override { return neighborTiles.size() == 8; }
		int  getScoreTileNum() const override { return (int)neighborTiles.size() + 1; }
		void mergeData(FeatureBase& other, MapTile const& putData, int meta) override { NEVER_REACH("Feature can't be merged!!"); }
		int doCalcScore(GamePlayerManager& playerManager, FeatureScoreInfo& outResult) override;
		bool updateForAdjacentTile(MapTile& tile) override;

		bool getActorPos(MapTile const& mapTile, ActorPos& actorPos) override;
	};

	class CloisterFeature : public AdjacentTileScoringFeature
	{
	public:
		static int const Type = FeatureType::eCloister;
		CloisterFeature();

		CloisterFeature* challenger;

		int  getActorPutInfo(int playerId, int posMeta, MapTile& mapTile, TArray< ActorPosInfo >& outInfo) override;
		int  calcPlayerScore(PlayerBase* player) override;
		void generateRoadLinkFeatures(WorldTileManager& worldTileManager, GroupSet& outFeatures) override;
		void onAddFollower(LevelActor& actor) override
		{
			actor.className = EFollowerClassName::Monk;
		}
	};

	class GardenFeature : public AdjacentTileScoringFeature
	{
	public:
		static int const Type = FeatureType::eGarden;
		GardenFeature();
		int  getActorPutInfo(int playerId, int posMeta, MapTile& mapTile, TArray< ActorPosInfo >& outInfo) override;
		int  calcPlayerScore(PlayerBase* player) override;
		void generateRoadLinkFeatures(WorldTileManager& worldTileManager, GroupSet& outFeatures) override;
		void onAddFollower(LevelActor& actor) override
		{
			assert(actor.type == EActor::Abbot);
			actor.className = EFollowerClassName::Abbot;
		}
	};

	class GermanCastleFeature : public FeatureBase
	{
	public:
		static int const Type = FeatureType::eGermanCastle;

		GermanCastleFeature();

		TArray< MapTile* > neighborTiles;

		bool checkComplete() const override 
		{ 
			assert(mapTiles.size() == 2);
			return neighborTiles.size() == 10; 
		}
		int  getActorPutInfo( int playerId , int posMeta , MapTile& mapTile, TArray< ActorPosInfo >& outInfo ) override;
		void mergeData( FeatureBase& other , MapTile const& putData , int meta ) override;
		int  doCalcScore( GamePlayerManager& playerManager , FeatureScoreInfo& outResult) override;
		int  calcPlayerScore(PlayerBase* player) override;
		void generateRoadLinkFeatures( WorldTileManager& worldTileManager, GroupSet& outFeatures ) override;
		bool updateForAdjacentTile(MapTile& tile) override;
		bool getActorPos(MapTile const& mapTile , ActorPos& actorPos) override;
		void onAddFollower(LevelActor& actor) override
		{
			actor.className = EFollowerClassName::Lord;
		}
	};


}//namespace CAR

#endif // CARFeature_h__b8a5bc79_4d4a_4362_80be_82b096535e7c
