#ifndef CARScore_h__f1f98b18_a3e7_48f7_b3f8_c6faff593bd6
#define CARScore_h__f1f98b18_a3e7_48f7_b3f8_c6faff593bd6

namespace CAR
{
	namespace Value
	{
		//Basic
		int const CloisterFactor = 1;

		int const CityFactor = 2;
		int const RoadFactor = 1;
		int const FarmFactorV1 = 4;
		int const FarmFactorV2 = 3;
		int const FarmFactorV3 = 3;

		int const PennatFactor = 2;
		int const PennatNonCompletFactor = 1;


		int const NonCompleteFactor = 1;
		int const MeeplePlayerOwnNum = 6;

		//EXP_INNS_AND_CATHEDRALS
		int const CathedralAdditionFactor = 1;
		int const InnAddtitionFactor = 1;
		int const RobberBaronFactor = 1;
		int const KingFactor = 1;
		int const BigMeeplePlayerOwnNum = 1;
		//EXP_TRADEERS_AND_BUILDERS
		int const TradeScore = 10;
		int const PigAdditionFactor = 1;
		int const BuilderPlayerOwnNum = 1;
		int const PigPlayerOwnNum = 1;

		//EXP_THE_PRINCESS_AND_THE_DRAGON
		int const FairyBeginningOfATurnScore = 1;
		int const FairyFearureScoringScore = 3;

		//EXP_THE_TOWER
		int const TowerPicesTotalNum = 30;
		int const TowerPicesPlayerOwnNum[] = { 0 , 0 , 10 , 9 , 7 , 6 , 5 };

		//EXP_ABBEY_AND_MAYOR
		int const AbbeyTilePlayerOwnNum = 1;
		int const BarnPlayerOwnNum = 1;
		int const WagonPlayerOwnNum = 1;
		int const MayorPlayerOwnNum = 1;

		//EXP_BRIDGES_CASTLES_AND_BAZAARS
		int const BridgePicesPlayerOwnNum[] = { 0 , 0 , 3 , 3 , 3 , 2 , 2 };
		int const CastleTokensPlayerOwnNum[] = { 0 , 0 , 3 , 3 , 3 , 2 , 2 };




		int const DragonMoveStepNum = 6;

	}

}//namespace CAR
#endif // CARScore_h__f1f98b18_a3e7_48f7_b3f8_c6faff593bd6
