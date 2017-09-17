#ifndef CARFeature_h__b8a5bc79_4d4a_4362_80be_82b096535e7c
#define CARFeature_h__b8a5bc79_4d4a_4362_80be_82b096535e7c

#include "CARCommon.h"
#include "CARMapTile.h"
#include "CARLevelActor.h"

#include <set>
#include <unordered_set>

namespace CAR
{
	class GameplaySetting;
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
			eGermanCastle ,
		};
	};


	struct ActorPosInfo
	{
		MapTile*  mapTile;
		unsigned  actorTypeMask;
		ActorPos  pos;
		int       group;
	};

	struct FeatureScoreInfo
	{
		int  id;
		int  score;
		int  majority;
		int  hillFollowerCount;
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

		virtual bool checkComplete(){ return false; }
		virtual int  getActorPutInfo( int playerId , int posMeta , MapTile& mapTile, std::vector< ActorPosInfo >& outInfo ) = 0;
		virtual void generateRoadLinkFeatures( GroupSet& outFeatures ){ }
		virtual bool updateForNeighborTile( MapTile& tile ){ return false; }
		virtual void mergeData( FeatureBase& other , MapTile const& putData , int meta );
		virtual int  calcPlayerScore( int playerId ) = 0;
		virtual int  calcScore( std::vector< FeatureScoreInfo >& scoreInfos );
		virtual bool getActorPos( MapTile const& mapTile , ActorPos& actorPos ){ return false; }

		static void initFeatureScoreInfo(std::vector< FeatureScoreInfo > &scoreInfos);
		int  getMajorityValue( ActorType actorType);
		void addMajority( std::vector< FeatureScoreInfo >& scoreInfos );
		int  evalMajorityControl(std::vector< FeatureScoreInfo >& featureControls);
		void addMapTile( MapTile& mapTile ){ mapTiles.insert( &mapTile ); }

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
		virtual void generateRoadLinkFeatures( GroupSet& outFeatures );
		virtual bool getActorPos( MapTile const& mapTile , ActorPos& actorPos );


		int getSideContentNum( unsigned contentMask );


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
		virtual bool checkComplete() override;
		virtual int  calcPlayerScore( int playerId );

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
		bool isCastle;
		int  cloisterGroup;
		std::set< FarmFeature* > linkFarms;

		bool isSamllCircular();


		virtual void mergeData( FeatureBase& other , MapTile const& putData , int meta ) override;
		virtual void addNode( MapTile& mapData , unsigned dirMask , SideNode* linkNode ) override;
		virtual int  getActorPutInfo( int playerId , int posMeta , MapTile& mapTile, std::vector< ActorPosInfo >& outInfo ) override;
		virtual bool checkComplete() override;
		virtual int  calcPlayerScore( int playerId );

		virtual int calcScore(std::vector< FeatureScoreInfo >& scoreInfos);


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
		virtual bool checkComplete() override { return false; }
		virtual int  calcScore( std::vector< FeatureScoreInfo >& scoreInfos );
		virtual int  calcPlayerScore( int playerId );

		int calcPlayerScoreByBarnRemoveFarmer( int playerId );
		int calcPlayerScoreInternal(int playerId, int farmFactor);

		bool haveBarn;
		std::vector< FarmNode* > nodes;
		std::set< CityFeature* > linkCities;
	};

	class CloisterFeature : public FeatureBase
	{
	public:
		static int const Type = FeatureType::eCloister;
		CloisterFeature();

		std::vector< MapTile* > neighborTiles;

		virtual bool checkComplete(){ return neighborTiles.size() == 8; }
		virtual int  getActorPutInfo( int playerId , int posMeta , MapTile& mapTile, std::vector< ActorPosInfo >& outInfo ) override;
		virtual void mergeData( FeatureBase& other , MapTile const& putData , int meta );
		virtual int  calcScore( std::vector< FeatureScoreInfo >& scoreInfos );
		virtual int  calcPlayerScore( int playerId );
		virtual void generateRoadLinkFeatures( GroupSet& outFeatures );
		virtual bool updateForNeighborTile(MapTile& tile);
		virtual bool getActorPos(MapTile const& mapTile , ActorPos& actorPos);

	};

	class GermanCastleFeature : public FeatureBase
	{
	public:
		static int const Type = FeatureType::eGermanCastle;

		GermanCastleFeature();

		std::vector< MapTile* > neighborTiles;

		virtual bool checkComplete(){ return neighborTiles.size() == 10; }
		virtual int  getActorPutInfo( int playerId , int posMeta , MapTile& mapTile, std::vector< ActorPosInfo >& outInfo ) override;
		virtual void mergeData( FeatureBase& other , MapTile const& putData , int meta );
		virtual int  calcScore( std::vector< FeatureScoreInfo >& scoreInfos );
		virtual int  calcPlayerScore( int playerId );
		virtual void generateRoadLinkFeatures( GroupSet& outFeatures );
		virtual bool updateForNeighborTile(MapTile& tile);
		virtual bool getActorPos(MapTile const& mapTile , ActorPos& actorPos);


	};

}//namespace CAR

#endif // CARFeature_h__b8a5bc79_4d4a_4362_80be_82b096535e7c
