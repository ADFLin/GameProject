#ifndef CARScore_h__f1f98b18_a3e7_48f7_b3f8_c6faff593bd6
#define CARScore_h__f1f98b18_a3e7_48f7_b3f8_c6faff593bd6

#define CAR_USE_CONST_PARAMVALUE 1


#if CAR_USE_CONST_PARAMVALUE
#define CAR_PARAM_VALUE( NAME ) GameParamCollection::NAME
#define CAR_PARAM_CONST const
#else
#define CAR_PARAM_VALUE( NAME ) getSetting().NAME
#define CAR_PARAM_CONST
#endif

namespace CAR
{
	namespace Value
	{
		int const MaxPlayerNum = 6;
		int const SheepTokenTypeNum = 5;
	}
#if CAR_USE_CONST_PARAMVALUE
	namespace GameParamCollection
	{
#else
	struct GameParamCollection
	{
		static GameParamCollection const& GetDefalut()
		{
			static GameParamCollection const sDefalutValue;
			return sDefalutValue;
		}
#endif

		CAR_PARAM_CONST int CloisterFactor = 1;

		CAR_PARAM_CONST int CityFactor = 2;
		CAR_PARAM_CONST int RoadFactor = 1;
		CAR_PARAM_CONST int FarmFactorV1 = 4;
		CAR_PARAM_CONST int FarmFactorV2 = 3;
		CAR_PARAM_CONST int FarmFactorV3 = 3;

		CAR_PARAM_CONST int PennatFactor = 2;
		CAR_PARAM_CONST int PennatNonCompletFactor = 1;

		CAR_PARAM_CONST int NonCompleteFactor = 1;
		CAR_PARAM_CONST int MeeplePlayerOwnNum = 6;

		//EXP_INNS_AND_CATHEDRALS
		CAR_PARAM_CONST int CathedralAdditionFactor = 1;
		CAR_PARAM_CONST int InnAddtitionFactor = 1;
		CAR_PARAM_CONST int RobberBaronFactor = 1;
		CAR_PARAM_CONST int KingFactor = 1;
		CAR_PARAM_CONST int BigMeeplePlayerOwnNum = 1;
		//EXP_TRADEERS_AND_BUILDERS
		CAR_PARAM_CONST int TradeScore = 10;
		CAR_PARAM_CONST int PigAdditionFactor = 1;
		CAR_PARAM_CONST int BuilderPlayerOwnNum = 1;
		CAR_PARAM_CONST int PigPlayerOwnNum = 1;

		//EXP_THE_PRINCESS_AND_THE_DRAGON
		CAR_PARAM_CONST int FairyBeginningOfATurnScore = 1;
		CAR_PARAM_CONST int FairyFearureScoringScore = 3;

		CAR_PARAM_CONST int DragonMoveStepNum = 6;

		//EXP_THE_TOWER
		CAR_PARAM_CONST int TowerPicesTotalNum = 30;
		CAR_PARAM_CONST int TowerPicesPlayerOwnNum[Value::MaxPlayerNum] = { 0 , 10 , 9 , 7 , 6 , 5 };
		CAR_PARAM_CONST int PrisonerRedeemScore = 3;

		//EXP_ABBEY_AND_MAYOR
		CAR_PARAM_CONST int AbbeyTilePlayerOwnNum = 1;
		CAR_PARAM_CONST int BarnPlayerOwnNum = 1;
		CAR_PARAM_CONST int WagonPlayerOwnNum = 1;
		CAR_PARAM_CONST int MayorPlayerOwnNum = 1;

		CAR_PARAM_CONST int BarnAddtionFactor = 1;
		CAR_PARAM_CONST int BarnRemoveFarmerFactor = 1;

		//EXP_BRIDGES_CASTLES_AND_BAZAARS
		CAR_PARAM_CONST int BridgePicesPlayerOwnNum[Value::MaxPlayerNum] = { 0 , 3 , 3 , 3 , 2 , 2 };
		CAR_PARAM_CONST int CastleTokensPlayerOwnNum[Value::MaxPlayerNum] = { 0 , 3 , 3 , 3 , 2 , 2 };

		//EXP_HILLS_AND_SHEEP
		CAR_PARAM_CONST int SheepTokenNum[Value::SheepTokenTypeNum] = { 2 , 4 , 5 , 5 , 2 };
		CAR_PARAM_CONST int ShepherdPlayerOwnNum = 1;
		CAR_PARAM_CONST int VineyardAdditionScore = 3;

		//EXP_CASTLES
		CAR_PARAM_CONST int GermanCastleAdditionFactor = 3;

		//EXP_PHANTOM
		CAR_PARAM_CONST int PhantomPlayerOwnNum = 1;

		//EXP_GOLDMINES
		CAR_PARAM_CONST int GoldPiecesScoreFactor[4] = { 4 , 3 , 2 , 1 };
		CAR_PARAM_CONST int GoldPiecesScoreFactorNum[4] = { 10 , 7 , 4 , 1 };

		//EXP_LITTLE_BUILDINGS
		CAR_PARAM_CONST int TowerBuildingTokenScoreFactor = 3;
		CAR_PARAM_CONST int HouseBuildingTokenScoreFactor = 2;
		CAR_PARAM_CONST int ShedBuildingTokenScoreFactor = 1;
		CAR_PARAM_CONST int TowerBuildingTokensPlayerOwnNum = 1;
		CAR_PARAM_CONST int HouseBuildingTokensPlayerOwnNum = 1;
		CAR_PARAM_CONST int ShedBuildingTokensPlayerOwnNum = 1;

		//EXP_THE_MESSSAGES
		CAR_PARAM_CONST int MessageSkipActionScore = 2;

	};




}//namespace CAR
#endif // CARScore_h__f1f98b18_a3e7_48f7_b3f8_c6faff593bd6
