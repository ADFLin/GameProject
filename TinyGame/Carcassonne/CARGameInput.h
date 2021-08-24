#ifndef CARGameInput_h__fc0b51d0_f649_490d_8b64_f49edd2550ac
#define CARGameInput_h__fc0b51d0_f649_490d_8b64_f49edd2550ac

#include "CARDefine.h"
#include "CARDebug.h"
#include "CARFeature.h"

#include "Template/ArrayView.h"

#include <vector>

namespace CAR
{

	struct GameActionData
	{
		GameActionData()
		{
			playerId = -1;
			bCanSkip = true;
			bSkipActionForResult = false;
		}

		template< class T >
		T* cast(){ return static_cast< T* >( this ); }
		int  playerId;
		bool bCanSkip;
		bool bSkipActionForResult;
	};


	struct GamePlaceTileData : public GameActionData
	{
		TileId id;

		Vec2i  resultPos;
		int    resultRotation;
	};


	enum ActionOptionGroup
	{
		ACTOPT_GROUP_DRAW_TILE ,
		ACTOPT_GROUP_MOVE_THE_WOOD,
		ACTOPT_GROUP_SHEPHERD ,
		ACTOPT_GROUP_LA_PORXADA,
		ACTOPT_GROUP_MOVE_MAGE_OR_WITCH ,
		ACTOPT_GROUP_MESSAGE ,
		ACTOPT_GROUP_CROP_CIRCLE ,
		ACTOPT_GROUP_SELECT_SCORE_TYPE ,
	};

	enum AcionOption
	{
		//ACTOPT_GROUP_DRAW_TILE
		ACTOPT_TILE_USE_RANDOM_TILE ,
		ACTOPT_TILE_USE_ABBEY ,
		ACTOPT_TILE_USE_HALFLING_TILE ,
		//ACTOPT_GROUP_MOVE_THE_WOOD
		ACTOPT_MTW_PLACE_FIGURES ,
		ACTOPT_MTW_PLACE_FOLLOWER_ON_THE_WHEEL_OF_FORTUNE,
		ACTOPT_MTW_REMOVE_ABBOT,
		ACTOPT_MTW_PLACE_TOWER ,
		ACTOPT_MTW_PLACE_LITTLE_BUILDING,
		ACTOPT_MTW_MOVE_FARIY ,
		//ACTOPT_GROUP_SHEPHERD
		ACTOPT_SHEPHERD_EXPAND_THE_FLOCK ,
		ACTOPT_SHEPHERD_HERD_THE_FLOCK_INTO_THE_STABLE ,
		//ACTOPT_GROUP_LA_PORXADA
		ACTOPT_LA_PORXADA_EXCHANGE_FOLLOWER_POS ,
		ACTOPT_LA_PORXADA_SCORING ,
		//ACTOPT_GROUP_MOVE_MAGE_OR_WITCH
		ACTOPT_MOVE_MAGE ,
		ACTOPT_MOVE_WITCH ,
		//ACTOPT_GROUP_MESSAGE
		ACTOPT_MESSAGE_PERFORM_ACTION ,
		ACTOPT_MESSAGE_SCORE_TWO_POINTS ,
		//ACTOPT_GROUP_CROP_CIRCLE
		ACTOPT_CROP_CIRCLE_DEPLOY_FOLLOWER ,
		ACTOPT_CROP_CIRCLE_REMOVE_FOLLOWER,
		//ACTOPT_GROUP_SELECT_SCORE_TYPE
		ACTOPT_SCORE_NORMAL ,
		ACTOPT_SCORE_WOMEN ,
	};


	struct GameSelectActionOptionData : public GameActionData
	{
		ActionOptionGroup group;
		std::vector< AcionOption > options;
		unsigned resultIndex;

		void validateResult(char const* actionName)
		{
			if( resultIndex >= options.size() )
			{
				CAR_LOG("Warning!! %s have error action option result Index!!", actionName);
				resultIndex = 0;
			}
		}
	};

	struct GameSelectCropCircleOptionData : public GameSelectActionOptionData
	{
		GameSelectCropCircleOptionData()
		{
			group = ACTOPT_GROUP_CROP_CIRCLE;
			options = { ACTOPT_CROP_CIRCLE_DEPLOY_FOLLOWER , ACTOPT_CROP_CIRCLE_REMOVE_FOLLOWER };
		}
		FeatureType::Enum cropType;
	};

	struct GameSelectMessageOptionData : public GameSelectActionOptionData
	{
		GameSelectMessageOptionData()
		{
			group = ACTOPT_GROUP_MESSAGE;
			options = { ACTOPT_MESSAGE_PERFORM_ACTION , ACTOPT_MESSAGE_SCORE_TWO_POINTS };
		}
		EMessageTile::Type token;
		int messageScore;
		union Object
		{
			LevelActor*  actor;
			FeatureBase* feature;
		};
		std::vector<Object> objects;
	};


	enum SelectActionReason
	{
		//MapTile
		SAR_CONSTRUCT_TOWER ,
		SAR_MOVE_DRAGON ,
		SAR_MAGIC_PORTAL ,
		SAR_PLACE_GOLD_PIECES ,
		SAR_MOVE_MEAGE_OR_WITCH ,
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

		//
		SAR_LA_PORXADA_SELF_FOLLOWER,
		SAR_LA_PORXADA_OTHER_PLAYER_FOLLOWER,
	};



	struct GameSelectActionData : public GameActionData
	{
		GameSelectActionData()
		{
			resultIndex = 0;
		}
		SelectActionReason reason;
		unsigned resultIndex;
	};


	template<typename T >
	struct TGameSelectActionData : public GameSelectActionData
	{
		TArrayView<T> options;

		void validateResult(char const* actionName)
		{
			if (!checkResult())
			{
				CAR_LOG("Warning!! %s have error resultIndex!!", actionName);
				resultIndex = 0;
			}
		}

		bool checkResult() const
		{
			return resultIndex < options.size();
		}

		T& getResult() { return options[resultIndex]; }
	};


	using GameSelectMapPosData = TGameSelectActionData< Vec2i >;
	using GameSelectMapTileData = TGameSelectActionData< MapTile* >;
	using GameSelectActorData = TGameSelectActionData< LevelActor* >;
	using GameSelectFeatureData = TGameSelectActionData< FeatureBase* >;
	using GameSelectActorInfoData = TGameSelectActionData< ActorInfo >;

	struct GameDeployActorData : public TGameSelectActionData< ActorPosInfo >
	{
		EActor::Type resultType;
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

		void fill(std::vector<FeatureBase*> const& linkFeatures , std::vector<MapTile*>& mapTileStorage , bool bCheckCanDeployFollower );

		std::vector< Info > infos;
		FeatureBase* getResultFeature();
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

	struct GameExchangeActorPosData : public GameActionData
	{
		LevelActor* selfActor;
		LevelActor* targetActor;
		EActor::Type   resultActorType;
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
		virtual void requestExchangeActorPos(GameExchangeActorPosData& data) = 0;
	};

}//namespace CAR

#endif // CARGameInput_h__fc0b51d0_f649_490d_8b64_f49edd2550ac