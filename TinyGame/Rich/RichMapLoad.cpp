#include "RichArea.h"

#include "RichScene.h"
#include "RichActionEvent.h"

namespace Rich
{
	namespace Monopoly
	{
		struct TileDef
		{
			TileDef() = default;
			TileDef(uint8 id):id(id){}

			uint8 id;
			uint8 index;
			uint8 tag = 0;
			uint8 padding = 0;
		};
		uint8 const BB = 0;
		uint8 const TE = 1;
		uint8 const BL = 2;
		uint8 const GO = 3;
		uint8 const ST = 4;
		uint8 const GJ = 5;
		uint8 const JA = 6;
		uint8 const CD = 7;
		uint8 const CC = 8;
		uint8 const TypeMaskBit = 4;
		uint8 const TypeMask = (1 << TypeMaskBit) - 1;
		TileDef TL(uint8 index, uint8 tag = 0)
		{
			TileDef def;
			def.id = BL;
			def.index = index;
			def.tag = tag;
			return def;
		}
		TileDef TS(uint8 index)
		{
			TileDef def;
			def.id = ST;
			def.index = index;
			return def;
		}

		uint8 TC = (uint8)EAreaTag::Chance_MoveTo;
		int const SizeX = 11;
		int const SizeY = 11;
		TileDef Data[] =
		{
			GO    ,TL( 0),CC    ,TL( 1),TE    ,TS( 0),TL( 2),CD    ,TL( 3),TL( 4),JA    ,
			TL(21),BB    ,BB    ,BB    ,BB    ,BB    ,BB    ,BB    ,BB    ,BB    ,TL( 5, TC),
			TE    ,BB    ,BB    ,BB    ,BB    ,BB    ,BB    ,BB    ,BB    ,BB    ,TE    ,
			TL(20),BB    ,BB    ,BB    ,BB    ,BB    ,BB    ,BB    ,BB    ,BB    ,TL( 6),
			CD    ,BB    ,BB    ,BB    ,BB    ,BB    ,BB    ,BB    ,BB    ,BB    ,TL( 7),
			TS( 3),BB    ,BB    ,BB    ,BB    ,BB    ,BB    ,BB    ,BB    ,BB    ,TS( 1),
			TL(19),BB    ,BB    ,BB    ,BB    ,BB    ,BB    ,BB    ,BB    ,BB    ,TL( 8),
			CC    ,BB    ,BB    ,BB    ,BB    ,BB    ,BB    ,BB    ,BB    ,BB    ,CC    ,
			TL(18),BB    ,BB    ,BB    ,BB    ,BB    ,BB    ,BB    ,BB    ,BB    ,TL( 9),
			TL(17),BB    ,BB    ,BB    ,BB    ,BB    ,BB    ,BB    ,BB    ,BB    ,TL(10),
			GJ    ,TL(16),TE    ,TL(15),TL(14),TS( 2),TL(13),TL(12),CD    ,TL(11),TE    ,
		};


		LandArea::Info const LandData[] =
		{
			{ "A1" ,  60,  30, {    2,   10,   30,   90,  160,  250 },  50 ,1 },
			{ "A2" ,  60,  30, {    4,   20,   60,  180,  320,  450 },  50 ,1 },
			{ "B1" , 100,  50, {    6,   30,   90,  270,  400,  550 },  50 ,2 },
			{ "B2" , 100,  50, {    6,   30,   90,  270,  400,  550 },  50 ,2 },
			{ "B3" , 120,  60, {    8,   40,  100,  300,  450,  600 },  50 ,2 },
			{ "C1" , 140,  70, {   10,   50,  150,  450,  625,  750 }, 100 ,3 },
			{ "C2" , 140,  70, {   10,   50,  150,  450,  625,  750 }, 100 ,3 },
			{ "C3" , 160,  80, {   12,   60,  180,  500,  700,  900 }, 100 ,3 },
			{ "D1" , 180,  90, {   14,   70,  200,  550,  750,  950 }, 100 ,4 },
			{ "D2" , 180,  90, {   14,   70,  200,  550,  750,  950 }, 100 ,4 },
			{ "D3" , 200, 100, {   16,   80,  220,  600,  800, 1000 }, 100 ,4 },
			{ "E1" , 220, 110, {   18,   90,  250,  700,  875, 1050 }, 150 ,5 },
			{ "E2" , 220, 110, {   18,   90,  250,  700,  875, 1050 }, 150 ,5 },
			{ "E3" , 240, 120, {   20,  100,  300,  750,  925, 1100 }, 150 ,5 },
			{ "F1" , 260, 130, {   22,  110,  330,  800,  975, 1150 }, 150 ,6 },
			{ "F2" , 260, 130, {   22,  110,  330,  800,  975, 1150 }, 150 ,6 },
			{ "F3" , 280, 140, {   24,  120,  360,  850, 1025, 1200 }, 150 ,6 },
			{ "G1" , 300, 150, {   26,  130,  390,  900, 1100, 1275 }, 200 ,7 },
			{ "G2" , 300, 150, {   26,  130,  390,  900, 1100, 1275 }, 200 ,7 },
			{ "G3" , 320, 160, {   28,  150,  450, 1000, 1200, 1400 }, 200 ,7 },
			{ "H1" , 350, 175, {   35,  175,  500, 1100, 1300, 1500 }, 200 ,8 },
			{ "H2" , 400, 200, {   50,  200,  600, 1400, 1700, 2000 }, 200 ,8 },
		};

		StationArea::Info const StationData[] =
		{
			{ "S1", 200, 100, {    0,   25,   50,  100,  200,  200 } },
			{ "S2", 200, 100, {    0,   25,   50,  100,  200,  200 } },
			{ "S3", 200, 100, {    0,   25,   50,  100,  200,  200 } },
			{ "S4", 200, 100, {    0,   25,   50,  100,  200,  200 } },
		};

	}


	void LoadMap( Scene& scene )
	{
		World& world = scene.getLevel().getWorld();

		int idx = 0;
		for (int j = 0; j < Monopoly::SizeY; ++j)
		{
			for (int i = 0; i < Monopoly::SizeX; ++i)
			{
				auto coord = MapCoord(i, j);
				auto tileDef = Monopoly::Data[idx];
				AreaId id = -1;
				switch (tileDef.id)
				{
				case Monopoly::TE:
					world.addTile(coord, EMPTY_AREA_ID);
					break;
				case Monopoly::BL:
					{
						LandArea* land = new LandArea();
						land->mInfo = Monopoly::LandData[tileDef.index];
						id = world.registerArea(land);
						world.addTile(coord, id);
					}
					break;
				case Monopoly::ST:
					{
						StationArea* station = new StationArea;
						station->mInfo = Monopoly::StationData[tileDef.index];
						id = world.registerArea(station);
						world.addTile(coord, id);
					}
					break;
				case Monopoly::GO:
					{
						StartArea* start = new StartArea;
						id = world.registerArea(start);
						world.addTile(coord, id);
					}
					break;
				case Monopoly::GJ:
					{
						Area* area = new TActionEventArea< ActionEvent_GoJail >;
						id = world.registerArea(area);
						world.addTile(coord, id);
					}
					break;
				case Monopoly::JA:
					{
						JailArea* area = new JailArea;
						id = world.registerArea(area);
						world.addTile(coord, id);
					}
					break;
				case Monopoly::CD:
					{
						CardArea* area = new CardArea;
						area->mGroup = CG_CHANCE;
						id = world.registerArea(area);
						world.addTile(coord, id);
					}
					break;
				case Monopoly::CC:
					{
						CardArea* area = new CardArea;
						area->mGroup = CG_COMMUNITY;
						id = world.registerArea(area);
						world.addTile(coord, id);
					}
					break;
				}

				if (tileDef.tag)
				{
					world.registerAreaTag((EAreaTag)tileDef.tag, world.getArea(id));
				}
				++idx;
			}
		}
	}
}