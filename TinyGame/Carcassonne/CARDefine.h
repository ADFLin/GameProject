#ifndef CARDefine_h__201a14a3_46e7_42a6_b6ac_29fbc7d227ba
#define CARDefine_h__201a14a3_46e7_42a6_b6ac_29fbc7d227ba

#include "Core/IntegerType.h"

#include <cassert>

#ifndef BIT
#	define BIT( n ) ( 1 << (n) )
#endif

#ifndef ARRAY_SIZE
#	define ARRAY_SIZE( array ) ( sizeof(array) / sizeof( array[0]) )
#endif

namespace CAR
{
	class MapTile;
	class LevelActor;

	using TileId   = uint32;
	using PlayerId = uint32;

	TileId const FAIL_TILE_ID = TileId(-1);
	TileId const TEMP_TILE_ID = TileId(-2);

	PlayerId const CAR_ERROR_PLAYER_ID = PlayerId(-1);
	PlayerId const FAIL_PLAYER_ID = PlayerId(-2);

	int const ERROR_GROUP_ID = -1;
	int const ABBEY_GROUP_ID = -2;

	template< class T >
	constexpr int BitMaskToIndex(T mask)
	{
		return (mask >> 1) ? 1 + BitMaskToIndex(mask >> 1) : 0;
	}

	enum Expansion : uint8;


	namespace ESide
	{
		enum Type : uint8
		{
			Field = 0,
			Road,
			River,
			City,
			Abbey, //EXP_ABBEY_AND_MAYOR
			InsideLink,

			Empty,
		};
	};


	using SideContentMask = uint16;
	using TileContentMask = uint32;

	struct TileContent
	{
		enum Enum
		{
			eCloister            ,
			eCathedral           , //EXP_INNS_AND_CATHEDRALS
			eVolcano             , //EXP_THE_PRINCESS_AND_THE_DRAGON 
			eTheDragon           , //EXP_THE_PRINCESS_AND_THE_DRAGON 
			eMagicPortal         , //EXP_THE_PRINCESS_AND_THE_DRAGON
			eTowerFoundation     , //EXP_THE_TOWER
			eBazaar              , //EXP_BRIDGES_CASTLES_AND_BAZAARS
			eHill                , //EXP_HILLS_AND_SHEEP ?
			eVineyard            , //EXP_HILLS_AND_SHEEP
			eHalfling            , //EXP_HALFLINGS_I EXP_HALFLINGS_II
			eLaPorxada           , //EXP_LA_PORXADA
			eMage                , //EXP_MAGE_AND_WITCH
			eGold                , //EXP_GOLDMINES
			eAircraft            , //EXP_THE_FLIER
			eCropCirclePitchfork , //EXP_CROP_CIRCLE_I EXP_CROP_CIRCLE_II
			eCropCircleClub      , //EXP_CROP_CIRCLE_I EXP_CROP_CIRCLE_II
			eCropCircleShield    , //EXP_CROP_CIRCLE_I EXP_CROP_CIRCLE_II
			eBesieger            ,
			eShrine              , //EXP_HERETICS_AND_SHRINES
			eMonastery           ,
			eFestival            , //EXP_THE_FESTIVAL
			eWindRose_W          ,
			eWindRose_N          ,
			eWindRose_S          ,
			eWindRose_E          ,
			eBlueWindRose        ,
			eFair                ,
			eGarden              ,
			//runtime
			eTemp                ,


			Count ,
		};

		static TileContentMask const FeatureMask = BIT(eCloister) | BIT(eShrine) | BIT(eMonastery);
		static TileContentMask const CropCircleMask = BIT(eCropCirclePitchfork) | BIT(eCropCircleClub) | BIT(eCropCircleShield);
		static TileContentMask const OrangeWindRoseMask = BIT(eWindRose_W) | BIT(eWindRose_N) | BIT(eWindRose_S) | BIT(eWindRose_E);
		static TileContentMask const WindRoseMask = OrangeWindRoseMask | BIT(eBlueWindRose);
	};

	struct  SideContent
	{
		enum Enum
		{
			ePennant             ,
			eInn                 , //EXP_INNS_AND_CATHEDRALS
			eWineHouse           , //EXP_TRADEERS_AND_BUILDERS
			eGrainHouse          , //EXP_TRADEERS_AND_BUILDERS
			eClothHouse          , //EXP_TRADEERS_AND_BUILDERS
			ePrincess            , //EXP_THE_PRINCESS_AND_THE_DRAGON
			eNotSemiCircularCity , //EXP_BRIDGES_CASTLES_AND_BAZAARS
			eSheep               , //EXP_HILLS_AND_SHEEP
			eHalfSeparate        , //EXP_HILLS_AND_SHEEP
			eSchool              , //EXP_THE_SCHOOL
			eGermanCastle        ,
			eCityOfCarcassonne   ,
			eAircraftDirMark     ,

			Count ,
		};

		static SideContentMask const InternalLinkTypeMask = BIT(eSchool) | BIT(eGermanCastle);
	};


	static_assert(SideContent::Count < 8 * sizeof(SideContentMask), "SideContentMask can't set all SideContent enum");
	static_assert(TileContent::Count < 8 * sizeof(TileContentMask), "TileContentMask can't set all TileContent enum");


	namespace ECityQuarter
	{
		enum Type
		{
			Castle ,
			Market ,
			Blacksmith ,
			Cathedral ,
		};
	}

	struct ActorPos
	{
		enum Type
		{
			eSideNode ,
			eFarmNode ,
			eTile     ,
			eTower    ,
			eTileCorner ,
			ePlayer ,
			eCityQuarter ,
			eScoreBoard ,
			eWheelSector,
			eNone ,
		};
		ActorPos( Type aType , int aMeta )
			:type( aType ),meta( aMeta ){}
		ActorPos()= default;

		static ActorPos None() { return ActorPos(eNone, 0); }
		static ActorPos Tile() { return ActorPos(eTile, 0); }
		static ActorPos Player(PlayerId id) { return ActorPos(ePlayer, id); }
		static ActorPos SideNode(int index) { return ActorPos(eSideNode, index); }
		static ActorPos FarmNode(int index) { return ActorPos(eFarmNode, index); }
		static ActorPos WheelSector(int index) { return ActorPos(eWheelSector, index); }
		Type type;
		int  meta;

		bool operator == (ActorPos const& rhs) const
		{
			if( type != rhs.type )
				return false;
			if( type != ActorPos::eTile && meta != rhs.meta )
				return false;
			return true;
		}
	};


	enum class EFollowerClassName
	{
		Undefined,
		Thief,
		Knight,
		Monk,
		Abbot,
		Heretic,
		Farmer,
		Lord,
	};


	namespace EActor
	{
		enum Type
		{
			//follower

			Meeple,
			BigMeeple, //EXP_INNS_AND_CATHEDRALS 
			Mayor, //EXP_ABBEY_AND_MAYOR
			Wagon, //EXP_ABBEY_AND_MAYOR
			Barn, //EXP_ABBEY_AND_MAYOR
			Shepherd, //EXP_HILLS_AND_SHEEP
			Phantom, //EXP_PHANTOM
			Abbot, //CII


			Builder,  //EXP_TRADEERS_AND_BUILDERS
			Pig,     //EXP_TRADEERS_AND_BUILDERS

			PLAYER_TYPE_COUNT,
			//neutral
			Dragon = PLAYER_TYPE_COUNT, //EXP_THE_PRINCESS_AND_THE_DRAGON
			Fariy, //EXP_THE_PRINCESS_AND_THE_DRAGON
			Count,
			Mage,  //EXP_MAGE_AND_WITCH
			Witch, //EXP_MAGE_AND_WITCH

			BigPinkPig,
			Techer, //EXP_THE_SCHOOL

			Fair, //EXP_THE_CATAPULT

			COUNT,

			None = -1,
		};
	}

	struct EField
	{
		enum Type
		{
			ActorStart = 0 ,
			ActorEnd   = ActorStart + EActor::PLAYER_TYPE_COUNT - 1 ,

			//EXP_TRADEERS_AND_BUILDERS
			Gain , 
			Wine ,
			Cloth ,
			//EXP_THE_TOWER
			TowerPices ,
			//EXP_ABBEY_AND_MAYOR
			AbbeyPices ,
			//EXP_BRIDGES_CASTLES_AND_BAZAARS
			BridgePices ,
			CastleTokens ,
			TileIdAuctioned ,
			//EXP_CASTLES
			GermanCastleTiles ,
			//EXP_GOLDMINES
			GoldPieces ,
			//EXP_HALFLINGS_I && EXP_HALFLINGS_II
			HalflingTiles ,
			//EXP_LA_PORXADA
			LaPorxadaFinishScoring ,
			//EXP_THE_MESSSAGES
			WomenScore ,
			//EXP_THE_ROBBERS
			RobberScorePos,

			//EXP_LITTLE_BUILDINGS
			TowerBuildingTokens,
			HouseBuildingTokens,
			ShedBuildingTokens,

			COUNT,
		};
	};

	enum TileTag
	{
		TILE_NO_TAG = 0,
		TILE_START_TAG ,
		TILE_START_SEQ_BEGIN_TAG ,
		TILE_START_SEQ_END_TAG ,
		TILE_FIRST_PLAY_TAG ,
		TILE_END_TAG ,
		TILE_ABBEY_TAG , //EXP_ABBEY_AND_MAYOR
		TILE_GERMAN_CASTLE_TAG ,
		TILE_HALFLING_TAG ,
		TILE_SCHOOL_TAG ,
	};

	enum SheepToken
	{
		eWolf = 0,
		eOne ,
		eTwo ,
		eThree ,
		eFour ,
		
		Num ,
	};

	namespace EMessageTile
	{
		enum Type
		{
			ScoreSmallestRoad,
			ScoreSmallestCity,
			ScoreSmallestCloister,
			TwoPointsForEachPennant ,
			TwoPointsForEachKnight ,
			TwoPointsForEachFarmer ,
			OneTile ,
			ScoreAFollowerAndRemove ,
		};
	};

	struct ActorInfo
	{
		int          playerId;
		EActor::Type type;

		bool operator == ( ActorInfo const& rhs ) const
		{
			return playerId == rhs.playerId &&
				type == rhs.type;
		}
	};


}//namespace CAR

#endif // CARDefine_h__201a14a3_46e7_42a6_b6ac_29fbc7d227ba
