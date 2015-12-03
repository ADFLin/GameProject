#ifndef BMRender_h__
#define BMRender_h__

namespace BomberMan
{
	enum AnimAction
	{
		ANIM_WALK    ,
		ANIM_VICTORY ,
		ANIM_STAND   ,
		ANIM_IDLE    ,
		ANIM_DIE     ,

		ANIM_WALL_BURNING ,
	};

	enum LevelTheme
	{
		LT_CLASSICAL ,
	};

	class IAnimManager
	{
	public:
		virtual void setupLevel( LevelTheme theme ) = 0;
		virtual void restartLevel( bool beInit ) = 0;
		virtual void setPlayerAnim( Player& player , AnimAction action ) = 0;
		virtual void setTileAnim( Vec2i const& pos , AnimAction action ) = 0;
	};


}//namespace BomberMan

#endif // BMRender_h__
