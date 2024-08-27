#ifndef CARScore_h__f1f98b18_a3e7_48f7_b3f8_c6faff593bd6
#define CARScore_h__f1f98b18_a3e7_48f7_b3f8_c6faff593bd6

#define CAR_USE_CONST_PARAMVALUE 1

#if CAR_USE_CONST_PARAMVALUE
#define CAR_PARAM_QUALIFIER constexpr
#else
#define CAR_PARAM_QUALIFIER
#endif


namespace CAR
{
	namespace Value
	{
		int const MaxPlayerNum = 6;
		int const SheepTokenTypeNum = 5;
	}
#if CAR_USE_CONST_PARAMVALUE
	class GameParamCollection {};
	namespace GameParamSpace
	{
#else
	class GameParamCollection
	{
		static GameParamCollection const& GetDefalut()
		{
			static GameParamCollection const sDefalutValue;
			return sDefalutValue;
		}
#endif

		CAR_PARAM_QUALIFIER int CloisterFactor = 1;

		CAR_PARAM_QUALIFIER int CityFactor = 2;
		CAR_PARAM_QUALIFIER int RoadFactor = 1;
		CAR_PARAM_QUALIFIER int FarmFactorV1 = 4;
		CAR_PARAM_QUALIFIER int FarmFactorV2 = 3;
		CAR_PARAM_QUALIFIER int FarmFactorV3 = 3;

		CAR_PARAM_QUALIFIER int PennatFactor = 2;
		CAR_PARAM_QUALIFIER int PennatNonCompletFactor = 1;

		CAR_PARAM_QUALIFIER int NonCompleteFactor = 1;
		CAR_PARAM_QUALIFIER int MeeplePlayerOwnNum = 6;

		//EXP_INNS_AND_CATHEDRALS
		CAR_PARAM_QUALIFIER int CathedralAdditionFactor = 1;
		CAR_PARAM_QUALIFIER int InnAddtitionFactor = 1;
		CAR_PARAM_QUALIFIER int RobberBaronFactor = 1;
		CAR_PARAM_QUALIFIER int KingFactor = 1;
		CAR_PARAM_QUALIFIER int BigMeeplePlayerOwnNum = 1;
		//EXP_TRADEERS_AND_BUILDERS
		CAR_PARAM_QUALIFIER int TradeScore = 10;
		CAR_PARAM_QUALIFIER int PigAdditionFactor = 1;
		CAR_PARAM_QUALIFIER int BuilderPlayerOwnNum = 1;
		CAR_PARAM_QUALIFIER int PigPlayerOwnNum = 1;

		//EXP_THE_PRINCESS_AND_THE_DRAGON
		CAR_PARAM_QUALIFIER int FairyBeginningOfATurnScore = 1;
		CAR_PARAM_QUALIFIER int FairyFearureScoringScore = 3;

		CAR_PARAM_QUALIFIER int DragonMoveStepNum = 6;

		//EXP_THE_TOWER
		CAR_PARAM_QUALIFIER int TowerPicesTotalNum = 30;
		CAR_PARAM_QUALIFIER int TowerPicesPlayerOwnNum[Value::MaxPlayerNum] = { 0 , 10 , 9 , 7 , 6 , 5 };
		CAR_PARAM_QUALIFIER int PrisonerRedeemScore = 3;

		//EXP_ABBEY_AND_MAYOR
		CAR_PARAM_QUALIFIER int AbbeyTilePlayerOwnNum = 1;
		CAR_PARAM_QUALIFIER int BarnPlayerOwnNum = 1;
		CAR_PARAM_QUALIFIER int WagonPlayerOwnNum = 1;
		CAR_PARAM_QUALIFIER int MayorPlayerOwnNum = 1;

		CAR_PARAM_QUALIFIER int BarnAddtionFactor = 1;
		CAR_PARAM_QUALIFIER int BarnRemoveFarmerFactor = 1;

		//EXP_BRIDGES_CASTLES_AND_BAZAARS
		CAR_PARAM_QUALIFIER int BridgePicesPlayerOwnNum[Value::MaxPlayerNum] = { 0 , 3 , 3 , 3 , 2 , 2 };
		CAR_PARAM_QUALIFIER int CastleTokensPlayerOwnNum[Value::MaxPlayerNum] = { 0 , 3 , 3 , 3 , 2 , 2 };

		//EXP_HILLS_AND_SHEEP
		CAR_PARAM_QUALIFIER int SheepTokenNum[Value::SheepTokenTypeNum] = { 2 , 4 , 5 , 5 , 2 };
		CAR_PARAM_QUALIFIER int ShepherdPlayerOwnNum = 1;
		CAR_PARAM_QUALIFIER int VineyardAdditionScore = 3;

		//EXP_CASTLES
		CAR_PARAM_QUALIFIER int GermanCastleAdditionFactor = 3;

		//EXP_PHANTOM
		CAR_PARAM_QUALIFIER int PhantomPlayerOwnNum = 1;

		//EXP_GOLDMINES
		CAR_PARAM_QUALIFIER int GoldPiecesScoreFactor[4] = { 4 , 3 , 2 , 1 };
		CAR_PARAM_QUALIFIER int GoldPiecesScoreFactorNum[4] = { 10 , 7 , 4 , 1 };

		//EXP_LITTLE_BUILDINGS
		CAR_PARAM_QUALIFIER int TowerBuildingTokenScoreFactor = 3;
		CAR_PARAM_QUALIFIER int HouseBuildingTokenScoreFactor = 2;
		CAR_PARAM_QUALIFIER int ShedBuildingTokenScoreFactor = 1;
		CAR_PARAM_QUALIFIER int TowerBuildingTokensPlayerOwnNum = 1;
		CAR_PARAM_QUALIFIER int HouseBuildingTokensPlayerOwnNum = 1;
		CAR_PARAM_QUALIFIER int ShedBuildingTokensPlayerOwnNum = 1;

		//EXP_THE_MESSSAGES
		CAR_PARAM_QUALIFIER int MessageSkipActionScore = 2;
		//EXP_THE_WIND_ROSES
		CAR_PARAM_QUALIFIER int WindRoseScore = 3;

	};


	template <class T >
	GameParamCollection& GetParamCollection(T& t)
	{
		static_assert(0, "Please overload this function or add static member function for using CAR_PARAM_VALUE");
	}

#if CAR_USE_CONST_PARAMVALUE
#define CAR_PARAM_VALUE( NAME ) GameParamSpace::NAME
#else
#define CAR_PARAM_VALUE( NAME ) GetParamCollection(*this).NAME
#endif
}//namespace CAR

#endif // CARScore_h__f1f98b18_a3e7_48f7_b3f8_c6faff593bd6
