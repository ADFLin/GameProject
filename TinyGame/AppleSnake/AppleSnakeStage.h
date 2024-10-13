#pragma once
#ifndef AppleSnakeStage_H_098D9222_7656_40B4_A0AA_F570D11BEF88
#define AppleSnakeStage_H_098D9222_7656_40B4_A0AA_F570D11BEF88

#include "Stage/TestStageHeader.h"
#include "GameRenderSetup.h"

#include "AppleSnakeLevel.h"

#include "RHI/RHICommand.h"
#include "RHI/DrawUtility.h"
#include "RHI/GpuProfiler.h"
#include "RHI/RHIGraphics2D.h"

#include "Core/IntegerType.h"
#include "MarcoCommon.h"

#include "Async/Coroutines.h"
#include "Tween.h"
#include "Misc/DebugDraw.h"
#include "DebugDraw.h"


namespace AppleSnake
{
	struct LevelData;

	using namespace Render;

	namespace EEntityFlag
	{
		enum Type : uint8
		{
			Fall       = BIT(0),
			CrossPortal= BIT(1),
		};
	};

	class LevelStage : public StageBase
		             , public IGameRenderSetup
					 , public World
	{
		using BaseClass = StageBase;
	public:

		bool loadMap(LevelData const& levelData);


		bool onInit() override
		{
			if (!BaseClass::onInit())
				return false;

			::Global::GUI().cleanupWidget();

			auto frame = WidgetUtility::CreateDevFrame();

			frame->addButton("Restart", [this](int event, GWidget*) -> bool
			{
				restart();
				return false;
			});
			frame->addCheckBox("Pause", bPaused);

			restart();

			return true;
		}

		void restart();



		bool bPaused = false;
		void onUpdate(GameTimeSpan deltaTime) override
		{
			if (bRestartRequest)
			{
				bRestartRequest = false;
				restart();
			}

			if (bPaused)
				return;

			mTweener.update(deltaTime);
		}

		struct Sprite
		{
			Vector2 offset = Vector2::Zero();
			int color = EColor::Null;
		};

		RenderTransform2D mWorldToScreen;
		RenderTransform2D mScreenToWorld;
		Tween::GroupTweener<float> mTweener;

		DirType mActionDir;
		float mMoveOffset;
		float mFallOffset;
		float mBlockFallOffset;
		int   mMoveEntityIndex;
		int   mIndexDraw;
		int   mCPIndex;
		int   mCPSnakeBodyIndex;
		int   mPoisonCount;
		bool  bMoving;
		bool  bGrowing;

		void resetParams()
		{
			mActionDir = INDEX_NONE;
			mIndexDraw = INDEX_NONE;
			mMoveOffset = 0;
			mFallOffset = 0;
			mBlockFallOffset = 0;
			mMoveEntityIndex = INDEX_NONE;
			mCPIndex = INDEX_NONE;
			mCPSnakeBodyIndex = INDEX_NONE;
			mPoisonCount = 0;

			bMoving = false;
			bGrowing = false;
		}

		void drawTile(RHIGraphics2D& g, Tile const& tile, Vector2 const& rPos);
		void drawSnake(RHIGraphics2D& g);
		void drawDeadSnake(RHIGraphics2D& g, Snake const& snake, Vector2 const& rPos);



		void onRender(float dFrame) override;

		Coroutines::ExecutionHandle mGameHandle;
		bool bRestartRequest = false;

		struct MoveInfo
		{
			Vec2i pos;
			uint8 tileId;
			DirType dir;
			int crossPortalIndex = INDEX_NONE;
			int pushIndex = INDEX_NONE;
			int pushProtalIndex = INDEX_NONE;
			bool bBrokenBody = false;
		};

		bool canMove(DirType dir, MoveInfo& moveInfo);
		void doMove(DirType dir, MoveInfo& moveInfo);
		void moveActionEntry(DirType dir);
		void moveAction(DirType dir)
		{
			if (mActionDir != INDEX_NONE)
				return;

			mGameHandle = Coroutines::Start([this,dir]
			{
				TGuardValue<int> scopedValue(mActionDir, dir);
				moveActionEntry(dir);
			});
		}

		MsgReply onKey(KeyMsg const& msg) override
		{
			if (msg.isDown())
			{
				switch (msg.getCode())
				{
				case EKeyCode::D: moveAction(0); break;
				case EKeyCode::A: moveAction(2); break;
				case EKeyCode::S: moveAction(GravityDir); break;
				case EKeyCode::W: moveAction(InverseDir(GravityDir)); break;
				default:
					break;
				}
			}

			return BaseClass::onKey(msg);
		}

		MsgReply onMouse(MouseMsg const& msg) override
		{
			if (msg.onLeftDown())
			{
				Vector2 worldPos = mScreenToWorld.transformPosition(msg.getPos());
				Vec2i blockPos;
				blockPos.x = Math::FloorToInt(Math::ToTileValue(worldPos.x, 1.0f));
				blockPos.y = Math::FloorToInt(Math::ToTileValue(worldPos.y, 1.0f));
			}
			else if (msg.onRightDown())
			{
			}
			return BaseClass::onMouse(msg);
		}
	};
}

#endif // AppleSnakeStage_H_098D9222_7656_40B4_A0AA_F570D11BEF88