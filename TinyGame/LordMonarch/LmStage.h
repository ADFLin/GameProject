#ifndef LmStage_h__
#define LmStage_h__

#include "StageBase.h"
#include "LordMonarch/LmLevel.h"

namespace LordMonarch
{
	class TestStage : public StageBase
	{
		using BaseClass = StageBase;
	public:
		bool onInit() override;
		void onUpdate(GameTimeSpan deltaTime) override;
		void onRender(float dFrame) override;
		MsgReply onMouse(MouseMsg const& msg) override;
		MsgReply onKey(KeyMsg const& msg) override;

		void restart();

	private:
		Vec2i screenToTile(Vec2i const& pos) const;
		Vec2i tileToScreen(Vec2i const& pos) const;
		int findActorAt(Vec2i const& tilePos, bool bOnlyPlayer) const;
		int findNearestPlayerActor(Vec2i const& tilePos) const;

		Level mLevel;
		int mPlayerId = 0;
		int mSelectedActor = INDEX_NONE;
		Action::Id mPendingAction = Action::eMove;
		bool mbPause = false;
		int mTickAccumulator = 0;
		Vec2i mViewOffset = Vec2i(24, 42);
		int mTileSize = 20;
	};
}

#endif // LmStage_h__
