#ifndef CARGameInput_h__fc0b51d0_f649_490d_8b64_f49edd2550ac
#define CARGameInput_h__fc0b51d0_f649_490d_8b64_f49edd2550ac

#include "CARDefine.h"

#include <vector>

namespace CAR
{

	struct GameActionData
	{
		GameActionData()
		{
			playerId = -1;
			resultSkipAction = false;
		}

		template< class T >
		T* cast(){ return static_cast< T* >( this ); }
		int  playerId;
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
		ActorType resultType;
		int       resultIndex;
	};

	enum ActionOptionGroup
	{
		ACTOPT_GROUP_DRAW_TILE ,

	};
	enum AcionOption
	{
		//ACTOPT_GROUP_DRAW_TILE
		ACTOPT_TILE_USE_RANDOM_TILE ,
		ACTOPT_TILE_USE_ABBEY ,
		ACTOPT_TILE_USE_HALFLING_TILE ,
		//
		ACTOPT_SHEPHERD_EXPAND_THE_FLOCK ,
		ACTOPT_SHEPHERD_HERD_THE_FLOCK_INTO_THE_STABLE ,
	};


	struct GameSelectActionOptionData : public GameActionData
	{
		ActionOptionGroup group;
		std::vector< AcionOption > options;
		unsigned resultIndex;
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
		//MapPos
		SAR_PLACE_ABBEY_TILE ,
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

		bool canSkip() const
		{
			return !(BIT(reason) & (BIT(SAR_MOVE_DRAGON) | BIT(SAR_MAGIC_PORTAL) | BIT(SAR_EXCHANGE_PRISONERS)));
		}

		bool checkResult() const
		{
			return resultIndex < numSelection;
		}
	};

	struct GameSelectMapPosData : public GameSelectActionData
	{
		Vec2i* mapPositions;	
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
				if ( resultIndex < (unsigned)(info.index + info.num) )
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

	class GameLogic;

	class IGameInput
	{
	public:
		virtual void requestPlaceTile( GamePlaceTileData& data ) = 0;
		virtual void requestDeployActor( GameDeployActorData& data ) = 0;
		virtual void requestSelectMapTile( GameSelectMapTileData& data ) = 0;
		virtual void requestSelectActor( GameSelectActorData& data ) = 0;
		virtual void requestSelectActorInfo( GameSelectActorInfoData& data ) = 0;
		virtual void requestSelectMapPos( GameSelectMapPosData& data ) = 0;
		virtual void requestSelectActionOption( GameSelectActionOptionData& data ) = 0;
		virtual void requestTurnOver( GameActionData& data ) = 0;
		virtual void requestAuctionTile( GameAuctionTileData& data ) = 0;
		virtual void requestBuyAuctionedTile( GameAuctionTileData& data ) = 0;
		virtual void requestBuildCastle( GameBuildCastleData& data ) = 0;

	};

}//namespace CAR

#endif // CARGameInput_h__fc0b51d0_f649_490d_8b64_f49edd2550ac