#ifndef CARFeature_h__b8a5bc79_4d4a_4362_80be_82b096535e7c
#define CARFeature_h__b8a5bc79_4d4a_4362_80be_82b096535e7c

#include "CARCommon.h"
#include "CARMapTile.h"
#include "CARLevelActor.h"

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
		std::vector< FeatureControllerScoreInfo > controllerScores;

		FeatureScoreInfo()
		{
			numController = 0;
		}
	};

	typedef std::unordered_set< MapTile* > MapTileSet;
	typedef std::unordered_set< unsigned > GroupSet;

	class  FeatureBase : public ActorContainer
	{
	public:

		typedef MapTile::FarmNode FarmNode;
		typedef MapTile::SideNode SideNode;
		FeatureBase()
		{
			userData = nullptr;
		}
		virtual ~FeatureBase(){}

		int         group;
		int         type;
		MapTileSet  mapTiles;
		void*       userData;

		void        addActor( LevelActor& actor );
		void        removeActor( LevelActor& actor );
		LevelActor* removeActorByIndex( int index );
		bool        testInRange( Vec2i const& min , Vec2i const& max );
		int         calcScore(GamePlayerManager& playerManager, FeatureScoreInfo& outResult);

		virtual bool checkComplete() const { return false; }
		virtual int  getActorPutInfo( int playerId , int posMeta , MapTile& mapTile, std::vector< ActorPosInfo >& outInfo ) = 0;
		virtual void generateRoadLinkFeatures( WorldTileManager& worldTileManager, GroupSet& outFeatures ){ }
		virtual bool updateForAdjacentTile( MapTile& tile ){ return false; }
		virtual void mergeData( FeatureBase& other , MapTile const& putData , int meta );
		virtual int  calcPlayerScore( PlayerBase* player ) = 0;
		virtual int  doCalcScore( GamePlayerManager& playerManager , FeatureScoreInfo& outResult);
		virtual bool getActorPos( MapTile const& mapTile , ActorPos& actorPos ){ return false; }
		virtual int  getScoreTileNum() const { return 0; }
		virtual void onAddFollower(LevelActor& actor){}

		int  generateController(std::vector< FeatureControllerScoreInfo >& controllerScores);
		int  getMajorityValue( ActorType actorType);
		void generateMajority( std::vector< FeatureControllerScoreInfo >& controllerScores);
		int  evalMajorityControl(std::vector< FeatureControllerScoreInfo >& controllerScores);
		void addMapTile( MapTile& mapTile ){ mapTiles.insert( &mapTile ); }
		bool haveTileContent(unsigned contentMask) const;

		template< class T >
		static void MergeData( T& to , T const& src )
		{
			to.insert( to.end() , src.begin() , src.end() );
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

		int getDefaultActorPutInfo( int playerId , ActorPos const& actorPos , MapTile& mapTile, unsigned actorMasks[] , std::vector< ActorPosInfo >& outInfo );
		int getActorPutInfoInternal( int playerId , ActorPos const& actorPos , MapTile& mapTile, unsigned actorMasks[] , int numMask , std::vector< ActorPosInfo >& outInfo);

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
		typedef FeatureBase BaseClass;
	public:
		SideFeature();

		std::vector< SideNode* > nodes;
		int  openCount;
		int  halfSepareteCount;

		virtual void mergeData( FeatureBase& other , MapTile const& putData , int meta );
		virtual void addNode( MapTile& mapData , unsigned dirMask , SideNode* linkNode );
		virtual void addAbbeyNode( MapTile& mapData , int dir );
		virtual void generateRoadLinkFeatures( WorldTileManager& worldTileManager, GroupSet& outFeatures );
		virtual bool getActorPos( MapTile const& mapTile , ActorPos& actorPos );
		virtual int  getScoreTileNum() const { return mapTiles.size(); }

		int getSideContentNum(unsigned contentMask);
		bool checkNodesConnected() const;

		int calcOpenCount();
	};



	class RoadFeature : public SideFeature
	{
		typedef SideFeature BaseClass;
	public:
		static int const Type = FeatureType::eRoad;
		RoadFeature();
		virtual void mergeData( FeatureBase& other , MapTile const& putData , int meta ) override;
		virtual void addNode( MapTile& mapData , unsigned dirMask , SideNode* linkNode ) override;
		virtual int  getActorPutInfo( int playerId , int posMeta , MapTile& mapTile, std::vector< ActorPosInfo >& outInfo ) override;
		virtual bool checkComplete() const override;
		virtual int  calcPlayerScore(PlayerBase* player) override;
		virtual int  getScoreTileNum() const { return mapTiles.size(); }
		virtual void onAddFollower(LevelActor& actor) override { actor.className = EFollowerClassName::Thief; }
		bool      haveInn;
	};

	class FarmFeature;
	class CityFeature : public SideFeature
	{
		typedef SideFeature BaseClass;
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


		virtual void mergeData( FeatureBase& other , MapTile const& putData , int meta ) override;
		virtual void addNode( MapTile& mapData , unsigned dirMask , SideNode* linkNode ) override;
		virtual int  getActorPutInfo( int playerId , int posMeta , MapTile& mapTile, std::vector< ActorPosInfo >& outInfo ) override;
		virtual bool checkComplete() const override;
		virtual int  calcPlayerScore( PlayerBase* player );
		virtual void onAddFollower(LevelActor& actor) override 
		{ 
			actor.className = EFollowerClassName::Knight;
		}
		virtual int doCalcScore( GamePlayerManager& playerManager , FeatureScoreInfo& outResult) override;
	};

	class CityOfCarcassonneFeature : public FeatureBase
	{
	public:
		static int const Type = FeatureType::eCityOfCarcassonne;

		virtual bool checkComplete() const { return true; }
		virtual int  getActorPutInfo(int playerId, int posMeta, MapTile& mapTile, std::vector< ActorPosInfo >& outInfo) { return 0; }
		virtual void generateRoadLinkFeatures(WorldTileManager& worldTileManager, GroupSet& outFeatures) {}
		virtual bool updateForAdjacentTile(MapTile& tile) { return false; }
		virtual void mergeData(FeatureBase& other, MapTile const& putData, int meta) { NEVER_REACH("City of Carcassonne can't be merged"); }
		virtual int  calcPlayerScore(PlayerBase* player) { return 0; }
		virtual int  doCalcScore(GamePlayerManager& playerManager, FeatureScoreInfo& outResult) { return -1; }
		virtual bool getActorPos(MapTile const& mapTile, ActorPos& actorPos) { return false; }
		virtual int  getScoreTileNum() const { return 0; }
		virtual void onAddFollower(LevelActor& actor) {}
	};

	class FarmFeature : public FeatureBase
	{
		typedef FeatureBase BaseClass;
	public:
		FarmFeature();

		static int const Type = FeatureType::eFarm;

		void addNode( MapTile& mapData , unsigned idxMask , FarmNode* linkNode );
		virtual void mergeData( FeatureBase& other , MapTile const& putData , int meta ) override;
		virtual int  getActorPutInfo( int playerId , int posMeta , MapTile& mapTile, std::vector< ActorPosInfo >& outInfo ) override;
		virtual bool checkComplete() const override { return false; }
		virtual int  doCalcScore( GamePlayerManager& playerManager , FeatureScoreInfo& outResult) override;
		virtual int  calcPlayerScore( PlayerBase* player );
		virtual void onAddFollower(LevelActor& actor) override
		{
			actor.className = EFollowerClassName::Farmer;
		}
		int calcPlayerScoreByBarnRemoveFarmer(PlayerBase* player);
		int calcPlayerScoreInternal(PlayerBase* player, int farmFactor);

		bool haveBarn;
		std::vector< FarmNode* > nodes;
		std::unordered_set< FeatureBase* > linkedCities;
	};


	class AdjacentTileScoringFeature : public FeatureBase
	{
	public:
		std::vector< MapTile* > neighborTiles;
		virtual bool checkComplete() const { return neighborTiles.size() == 8; }
		virtual int  getScoreTileNum() const { return neighborTiles.size() + 1; }
		virtual void mergeData(FeatureBase& other, MapTile const& putData, int meta) { NEVER_REACH("Feature can't be merged!!"); }
		virtual int doCalcScore(GamePlayerManager& playerManager, FeatureScoreInfo& outResult) override;
		virtual bool updateForAdjacentTile(MapTile& tile) override;

		virtual bool getActorPos(MapTile const& mapTile, ActorPos& actorPos) override;
	};

	class CloisterFeature : public AdjacentTileScoringFeature
	{
	public:
		static int const Type = FeatureType::eCloister;
		CloisterFeature();

		CloisterFeature* challenger;

		virtual int  getActorPutInfo(int playerId, int posMeta, MapTile& mapTile, std::vector< ActorPosInfo >& outInfo) override;
		virtual int  calcPlayerScore(PlayerBase* player) override;
		virtual void generateRoadLinkFeatures(WorldTileManager& worldTileManager, GroupSet& outFeatures) override;
		virtual void onAddFollower(LevelActor& actor) override
		{
			actor.className = EFollowerClassName::Monk;
		}
	};

	class GardenFeature : public AdjacentTileScoringFeature
	{
	public:
		static int const Type = FeatureType::eGarden;
		GardenFeature();
		virtual int  getActorPutInfo(int playerId, int posMeta, MapTile& mapTile, std::vector< ActorPosInfo >& outInfo) override;
		virtual int  calcPlayerScore(PlayerBase* player) override;
		virtual void generateRoadLinkFeatures(WorldTileManager& worldTileManager, GroupSet& outFeatures) override;
		virtual void onAddFollower(LevelActor& actor) override
		{
			assert(actor.type == ActorType::eAbbot);
			actor.className = EFollowerClassName::Abbot;
		}
	};

	class GermanCastleFeature : public FeatureBase
	{
	public:
		static int const Type = FeatureType::eGermanCastle;

		GermanCastleFeature();

		std::vector< MapTile* > neighborTiles;

		virtual bool checkComplete() const override { return neighborTiles.size() == 10; }
		virtual int  getActorPutInfo( int playerId , int posMeta , MapTile& mapTile, std::vector< ActorPosInfo >& outInfo ) override;
		virtual void mergeData( FeatureBase& other , MapTile const& putData , int meta );
		virtual int  doCalcScore( GamePlayerManager& playerManager , FeatureScoreInfo& outResult) override;
		virtual int  calcPlayerScore(PlayerBase* player) override;
		virtual void generateRoadLinkFeatures( WorldTileManager& worldTileManager, GroupSet& outFeatures ) override;
		virtual bool updateForAdjacentTile(MapTile& tile) override;
		virtual bool getActorPos(MapTile const& mapTile , ActorPos& actorPos) override;
		virtual void onAddFollower(LevelActor& actor) override
		{
			actor.className = EFollowerClassName::Lord;
		}
	};


}//namespace CAR

#endif // CARFeature_h__b8a5bc79_4d4a_4362_80be_82b096535e7c
