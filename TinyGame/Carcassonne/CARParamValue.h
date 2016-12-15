#ifndef CARScore_h__f1f98b18_a3e7_48f7_b3f8_c6faff593bd6
#define CARScore_h__f1f98b18_a3e7_48f7_b3f8_c6faff593bd6

#define CAR_USE_CONST_PARAMVALUE 0

#if CAR_USE_CONST_PARAMVALUE
#define CAR_PARAM_VALUE( SETTING , NAME ) Setting->##NAME
#else
#define CAR_PARAM_VALUE( SETTING , NAME ) GameParamCollection::GetDefalut().##NAME
#endif

namespace CAR
{
	namespace Value
	{
		int const MaxPlayerNum = 6;
		int const SheepTokenTypeNum = 5;
	}

	struct GameParamCollection
	{
		int CloisterFactor = 1;

		int CityFactor = 2;
		int RoadFactor = 1;
		int FarmFactorV1 = 4;
		int FarmFactorV2 = 3;
		int FarmFactorV3 = 3;

		int PennatFactor = 2;
		int PennatNonCompletFactor = 1;

		int NonCompleteFactor = 1;
		int MeeplePlayerOwnNum = 6;

		//EXP_INNS_AND_CATHEDRALS
		int CathedralAdditionFactor = 1;
		int InnAddtitionFactor = 1;
		int RobberBaronFactor = 1;
		int KingFactor = 1;
		int BigMeeplePlayerOwnNum = 1;
		//EXP_TRADEERS_AND_BUILDERS
		int TradeScore = 10;
		int PigAdditionFactor = 1;
		int BuilderPlayerOwnNum = 1;
		int PigPlayerOwnNum = 1;

		//EXP_THE_PRINCESS_AND_THE_DRAGON
		int FairyBeginningOfATurnScore = 1;
		int FairyFearureScoringScore = 3;

		int DragonMoveStepNum = 6;

		//EXP_THE_TOWER
		int TowerPicesTotalNum = 30;
		int TowerPicesPlayerOwnNum[Value::MaxPlayerNum] = { 0 , 10 , 9 , 7 , 6 , 5 };
		int PrisonerBuyBackScore = 3;

		//EXP_ABBEY_AND_MAYOR
		int AbbeyTilePlayerOwnNum = 1;
		int BarnPlayerOwnNum = 1;
		int WagonPlayerOwnNum = 1;
		int MayorPlayerOwnNum = 1;

		int BarnAddtionFactor = 1;
		int BarnRemoveFarmerFactor = 1;

		//EXP_BRIDGES_CASTLES_AND_BAZAARS
		int BridgePicesPlayerOwnNum[Value::MaxPlayerNum] = { 0 , 3 , 3 , 3 , 2 , 2 };
		int CastleTokensPlayerOwnNum[Value::MaxPlayerNum] = { 0 , 3 , 3 , 3 , 2 , 2 };

		//EXP_HILLS_AND_SHEEP
		int SheepTokenNum[Value::SheepTokenTypeNum] = { 2 , 4 , 5 , 5 , 2 };
		int ShepherdPlayerOwnNum = 1;
		int VineyardAdditionScore = 3;

		//EXP_CASTLES
		int GermanCastleAdditionFactor = 3;

		//EXP_PHANTOM
		int PhantomPlayerOwnNum = 1;

		static GameParamCollection const& GetDefalut()
		{
			static GameParamCollection const sDefalutValue;
			return sDefalutValue;
		}
	};

}//namespace CAR
#endif // CARScore_h__f1f98b18_a3e7_48f7_b3f8_c6faff593bd6
