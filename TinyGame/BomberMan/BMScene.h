#ifndef BMScene_h__
#define BMScene_h__

#include "BMLevel.h"
#include "BMRender.h"

class ActionTrigger;
class Graphics2D;

namespace BomberMan
{

	int const TileLength = 16;
	Vec2i const TileSize = Vec2i( TileLength , TileLength );

	class Scene : public IAnimManager
	{
	public:

		Scene();
		World& getWorld(){ return mLevel;  }

		void   updateFrame( int frame );
		void   render( Graphics2D& g );
		
		void   setViewPos( Vec2i const& pos ){ mViewPos = pos; }
	private:


		void   setupLevel( LevelTheme theme )
		{

		}
		void   restartLevel( bool beInit );

		void   setPlayerAnim( Player& player , AnimAction action );
		void   setTileAnim( Vec2i const& pos , AnimAction action );

		void   drawTile( Graphics2D& g , Vec2i const& pos , Vec2i const& tPos , TileData const& tile );
		void   drawEntity( Graphics2D& g , Entity& entity );
		void   drawBomb( Graphics2D& g , Vec2i const& pos , Bomb const& bomb );
		void   drawPlayer( Graphics2D& g , Vec2i const& pos , Player& player );
		void   drawBlast( Graphics2D& g , Vec2i const& pos , Bomb const& bomb );

		struct RenderData
		{
			AnimAction action;
			int        frame;
		};
		int   mCurFrame;
		RenderData mPlayerData[ gMaxPlayerNum ];

		struct TileAnimData
		{
			int animId;
			int frame;

			TileAnimData()
			{ 
				animId = -1; 
				frame = 0;
			}
		};
		TGrid2D< TileAnimData > mTileAnim;
		typedef std::vector< Entity* >  EntityList;
		std::vector< Entity* >  mSortedEntites;

		Vec2i mViewPos;
		World mLevel;
		bool  mDrawDebug;
	};

}//namespace BomberMan

#endif // BMScene_h__
