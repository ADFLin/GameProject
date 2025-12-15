#ifndef ABPlayerController_h__
#define ABPlayerController_h__

#include "Math/Vector2.h"
#include "ABDefine.h"
#include "GameStage.h" 
#include "ABGameActionControl.h"

namespace AutoBattler
{
	class LevelStage;
	class Unit;
	class World;
	class Player;

	class IPlayerControllerContext
	{
	public:
		virtual ~IPlayerControllerContext() = default;
		virtual Vector2    screenToWorld(Vector2 const& screenPos) const = 0;
		virtual Unit*      pickUnit(Vec2i screenPos) = 0;
	    virtual UnitLocation screenToDropTarget(Vec2i screenPos) = 0;  // Returns UnitLocation for both Board and Bench
		virtual World&     getWorld() = 0;
		virtual bool       isNetMode() const = 0;
	};

	class ABPlayerController
	{
	public:
		ABPlayerController(IGameActionControl& action, IPlayerControllerContext& context);

		MsgReply onMouse(MouseMsg const& msg);
		MsgReply onKey(KeyMsg const& msg);
		void renderDrag(IGraphics2D& g);

		void setPlayer(Player& player) { mPlayer = &player; }

	private:
		IGameActionControl& mAction;
		IPlayerControllerContext& mContext;

		Player* mPlayer = nullptr;

		Unit*   mDraggedUnit = nullptr;
		Vector2 mDragOffset;
		Vector2 mDragStartPos;
		UnitLocation mDragStartLocation;
	};
}
#endif // ABPlayerController_h__
