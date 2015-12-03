#ifndef CvMap_h__
#define CvMap_h__

namespace Civ
{
	class UnitTile;

	enum Resource
	{
		RES_NONE ,
		RES_B_BANANA  ,
		RES_B_CATTLEC ,
	};

	enum Terrain
	{
		TER_GRASSLAND ,
		TER_PLAINS    ,
		TER_TUNDRA    ,
		TER_DESERT    ,
		TER_SNOW      ,
		TER_MOUNTAINS ,
		TER_COAST     ,
		TER_OCEANS    ,
	};

	enum Feature
	{
		TFT_WONDERS ,
		TFT_FOREST  ,
		TFT_JUNGLE  ,
		TFT_HILL    ,
		TFT_MARSH   ,
		TFT_PLAINS  ,
		TFT_OASIS   ,
		TFT_ICE     ,
	};

	enum Improvement
	{
		TIV_FARM ,
		TIV_MINE ,
		TIV_LUMBER_MILL ,
		TIV_TRADING_POST ,
		TIV_FORT ,
	};

	struct TileData
	{
		Terrain  terrain;
		Feature  feature;
		int      meta;
		int      res[2];
	};

	enum
	{
		FOOD ,
		GOLD ,

	};

	class Map
	{






	};

}//namespace Civ

#endif // CvMap_h__
