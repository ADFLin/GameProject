#ifndef TDMapEditor_h__
#define TDMapEditor_h__

#include "TDDefine.h"
namespace TowerDefend
{
	enum 
	{
		TILE_GRASS ,
		TILE_ROAD  ,
		TILE_ROCK  ,
	};

	enum TileFlag
	{
		TEF_BLOCK_WALK  = BIT(0),
		TEF_BLOCK_BUILD = BIT(1),
		TEF_BLOCK_FLY   = BIT(2),
		TEF_CALC_PARAM  = BIT(3),
	};

	struct TileInfo
	{
		unsigned id;
		unsigned flag;
	};

	TileInfo gTileInfo[] = 
	{
		{ TILE_GRASS , 0 } , 
		{ TILE_ROAD  , TEF_BLOCK_BUILD } ,
		{ TILE_ROCK  , TEF_BLOCK_BUILD | TEF_BLOCK_WALK } ,
	};

	class MapEditor
	{
	public:
		MapEditor( WorldMap& map ):mMap( map ){}
		void setTerrain( Vec2i const& pos , TileInfo const& info )
		{
			Tile& tile = mMap.getTile( pos );
			tile.tileID = info.id;

			if ( info.flag & TEF_CALC_PARAM )
			{
				for( int i = 0 ; i < 8 ; ++i )
				{

				}
			}
			else
			{
				tile.tileParam = 0;
			}

			if ( info.flag & TEF_BLOCK_BUILD )
				tile.blockMask[ CL_BUILD ] |= BM_TERRAIN;
			if ( info.flag & TEF_BLOCK_WALK )
				tile.blockMask[ CL_WALK ] |= BM_TERRAIN;

		}
		WorldMap& mMap;
	};

}//namespace TowerDefend

#endif // TDMapEditor_h__
