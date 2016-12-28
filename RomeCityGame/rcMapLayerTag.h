#ifndef rcMapLayerTag_h__
#define rcMapLayerTag_h__

enum BuildingTag
{
	BT_ROAD = 4,
	BT_WALL,
	BT_SIMON_STUFF ,
	BT_AQUADUCT ,
	BT_DIG ,

	BT_HOUSE_SIGN ,
	BT_HOUSE01,
	BT_HOUSE02,
	BT_HOUSE03,
	BT_HOUSE04,
	BT_HOUSE05,
	BT_HOUSE06,
	BT_HOUSE07,
	HOUSE08,
	HOUSE09,
	HOUSE10,
	HOUSE11,
	HOUSE12,
	HOUSE13,
	HOUSE14,
	HOUSE15,
	HOUSE16,
	HOUSE17,
	HOUSE18,
	HOUSE19,
	HOUSE20,

	//Entertainment
	BT_AMPITHEATRE ,
	BT_THEATRE ,
	BT_HIPPODROME ,
	BT_COLLOSEUM ,
	BT_GLADIATOR_PIT ,
	BT_LION_PIT ,
	BT_ARTIST_COLONY ,
	BT_CHATIOTEER_SCHOOL ,

	PLAZA ,
	BT_GARDENS ,
	FORT ,

	//	40,Fort (Legionaries),{,1000,-20,2,2,8,16,0,0,},
	//	41,Statue 1,{,12,3,1,-1,3,0,0,0,},
	//	42,Statue 2,{,60,10,1,-2,4,0,0,0,},
	//	43,Statue 3,{,150,14,2,-2,5,0,0,0,},
	//	44,Fort - Javelin,{,1000,-20,2,2,8,16,0,0,},16 or multiple of 16
	//	45,Fort - Horse,{,1000,-20,2,2,8,16,0,0,},
	//	46,Clinic,{,30,0,0,0,0,5,0,0,},
	//	47,Hospital,{,300,-1,2,1,2,30,0,0,},
	//	48,Baths,{,50,4,1,-1,4,10,0,0,},
	//	49,Barber,{,25,2,1,-1,2,2,0,0,},
	//	50,Distribution Center,{,0,0,0,0,0,0,0,0,},
	//	51,School,{,50,-2,1,1,2,10,0,0,},
	//	52,Academy,{,100,4,1,1,4,30,0,0,},
	//	53,Library,{,75,4,1,-1,4,20,0,0,},
	//	54,Nothing,{,0,0,0,0,0,0,0,0,},
	BT_PREFECTURE ,
	//	56,Triumphal Arch,{,0,18,2,3,5,0,0,0,},
	//	57,Fort,{,250,-20,2,2,8,16,0,0,},16 or multiple of 16
	//	58,Gatehouse,{,100,-4,1,1,3,3,0,0,},
	//	59,Tower,{,150,-8,1,2,3,6,0,0,},
	//	60,Small Farming Temple,{,50,4,2,-1,6,2,0,0,},
	//	61,Small Shipping Temple,{,50,4,2,-1,6,2,0,0,},
	//	62,Small Commerce Temple,{,50,4,2,-1,6,2,0,0,}
	//63,Small War Temple,{,50,4,2,-1,6,2,0,0,}
	//64,Small Love Temple,{,50,4,2,-1,6,2,0,0,}
	//65,Large Farming Temple,{,150,8,2,-1,8,5,0,0,}
	//66,Large Shipping Temple,{,150,8,2,-1,8,5,0,0,}
	//67,Large Commerce Temple,{,150,8,2,-1,8,5,0,0,}
	//68,Large War Temple,{,150,8,2,-1,8,5,0,0,}
	//69,Large Love Temple,{,150,8,2,-1,8,5,0,0,}
	BT_MARKET ,
	BT_GRANERY ,
	BT_WAREHOUSE ,
	//73,Warehouse Space,{,0,0,0,0,0,0,0,0,}
	//74,Shipyard,{,100,-8,2,2,3,10,0,0,}
	//75,Dock,{,100,-8,2,2,3,12,0,0,}
	//76,Wharf,{,60,-8,2,2,3,6,0,0,}
	//77,Governor Palace 1,{,150,12,2,-2,3,0,0,0,}
	//78,Governor Palace 2,{,400,20,2,-3,4,0,0,0,}
	//79,Governor Palace 3,{,700,28,2,-4,5,0,0,0,}
	//80,Mission,{,100,-3,1,1,2,20,0,0,}
	//81,Engineering Post,{,30,0,1,1,1,5,0,0,}
	//82,Foot Bridge,{,40,0,0,0,0,0,0,0,}
	//83,Ship Bridge,{,100,0,0,0,0,0,0,0,}
	//84,Senate 1,{,250,8,2,-2,2,20,0,0,}
	//85,Senate 2,{,400,8,2,-1,8,30,0,0,}
	//86,Forum 1,{,75,3,2,-1,2,6,0,0,}
	//87,Forum 2,{,125,3,2,-1,2,8,0,0,}
	//88,Native Hut,{,50,0,0,0,0,0,0,0,}
	//89,Native Meeting,{,50,0,0,0,0,0,0,0,}

	BT_RESEVIOR ,
	BT_FOUNTAIN ,
	BT_WATER_WELL ,
	//93,Native Field,{,0,0,0,0,0,0,0,0,}
	//94,Military Academy,{,1000,-3,1,1,3,20,0,0,}
	//95,Barracks,{,150,-6,1,1,3,10,0,0,}
	//96,Nothing,{,0,0,0,0,0,0,0,0,}
	//97,Nothing,{,0,0,0,0,0,0,0,0,}
	//98,Oracle,{,200,8,2,-1,6,0,0,0,}
	//99,Burning House,{,0,-1,1,1,2,0,0,0,}

	BT_FACTORY_START ,

	BT_FARM_START = BT_FACTORY_START,

	BT_FARM_WHEAT = BT_FARM_START ,
	BT_FARM_VEGETABLE ,
	BT_FARM_FIG ,
	BT_FARM_OLIVE ,
	BT_FARM_VINEYARD ,
	BT_FARM_MEAT ,

	BT_FARM_END = BT_FARM_MEAT ,

	
	//106,Quarry,{,50,-6,1,1,4,10,0,0,}
	//107,Mine,{,50,-6,1,1,4,10,0,0,}
	BT_MINE  ,
	//108,Lumber Mill,{,40,-4,1,1,3,10,0,0,}
	BT_CALY_PIT ,

	BT_WORK_SHOP_START ,
	//110,Wine workshop,{,45,-1,1,1,1,10,0,0,}
	//111,Oil Workshop,{,50,-4,1,1,2,10,0,0,}
	//112,Weapons Workshop,{,50,-4,1,1,2,10,0,0,}
	//113,Furniture Workshop,{,40,-4,1,1,2,10,0,0,}
	//114,Pottery Workshop,{,40,-4,1,1,2,10,0,0,}

	BT_WS_WINE = BT_WORK_SHOP_START,
	BT_WS_OIL ,
	BT_WS_WEAPONS ,
	BT_WS_FURNITURE ,
	BT_WS_POTTERY ,


	BUILDING_END ,

	MAX_BUILDING_TYPE_TAG ,

	BT_ERROR_TAG = -1 ,
};

enum TerrainTag
{
	TT_GRASS = 0 ,
};



#endif // rcMapLayerTag_h__
