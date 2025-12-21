#include "ABPCH.h"
#include "ABPlayerController.h"

#include "ABWorld.h"
#include "ABPlayer.h"
#include "ABAction.h"
#include "GameGlobal.h"
#include "GameGUISystem.h"
#include "RenderUtility.h"

namespace AutoBattler
{
	ABPlayerController::ABPlayerController(IGameActionControl& action, IPlayerControllerContext& context)
		: mAction(action)
		, mContext(context)
	{

	}

	MsgReply ABPlayerController::onMouse(MouseMsg const& msg)
	{
		Player& player = *mPlayer;
		PlayerBoard& board = mPlayer->getBoard();
		World& mWorld = mContext.getWorld();

		// Shop Interaction
		if (msg.onLeftDown())
		{
			Vec2i screenSize = ::Global::GetScreenSize();
			Vec2i shopSize(screenSize.x, 150);
			Vec2i shopPos(0, screenSize.y - shopSize.y);
			Vec2i mousePos = msg.getPos();

			if (mousePos.x >= shopPos.x && mousePos.x < shopPos.x + shopSize.x &&
				mousePos.y >= shopPos.y && mousePos.y < shopPos.y + shopSize.y)
			{
				int cardWidth = 100;
				int cardHeight = 120;
				int gap = 15;
				int startX = (shopSize.x - (5 * cardWidth + 4 * gap)) / 2;
				int startY = shopPos.y + (shopSize.y - cardHeight) / 2;

				for (int i = 0; i < 5; ++i)
				{
					Vec2i cardPos(startX + i * (cardWidth + gap), startY);
					if (mousePos.x >= cardPos.x && mousePos.x < cardPos.x + cardWidth &&
						mousePos.y >= cardPos.y && mousePos.y < cardPos.y + cardHeight)
					{
						mAction.buyUnit(player, i);
						return MsgReply::Handled();
					}
				}
				return MsgReply::Handled(); // Consumed by shop background
			}
		}

		// Right Click Action
		if (msg.onRightDown())
		{
			UnitLocation target = mContext.screenToDropTarget(msg.getPos());

			if (target.isBoard())
			{
				// 1. Check Board (Quick Retract)
				if (mWorld.getPhase() == BattlePhase::Preparation)
				{
					Unit* unit = board.getUnit(target.pos.x, target.pos.y);
					if (unit && unit->getTeam() == UnitTeam::Player)
					{
						// Find empty bench slot
						int slotIndex = -1;
						for (int j = 0; j < player.mBench.size(); ++j)
						{
							if (player.mBench[j] == nullptr)
							{
								slotIndex = j;
								break;
							}
						}

						if (slotIndex != -1)
						{
							mAction.retractUnit(player, 0, target.pos.x, target.pos.y, slotIndex);
						}
						return MsgReply::Handled();
					}
				}
			}
			else if (target.isBench())
			{
				// 2. Check Bench (Quick Sell)
				Unit* unit = player.mBench[target.index];
				if (unit)
				{
					mAction.sellUnit(player, target.index);
					return MsgReply::Handled();
				}
			}
		}

		if (msg.onLeftDown())
		{
			Vec2i mousePos = msg.getPos();
			Vector2 worldPos = mContext.screenToWorld(Vector2(msg.getPos().x, msg.getPos().y));

			if (mWorld.getPhase() == BattlePhase::Preparation)
			{
				Unit* pickedUnit = mContext.pickUnit(mousePos);
				
				// Only allow dragging player's own units
				if (pickedUnit && pickedUnit->getTeam() == UnitTeam::Player)
				{
					// Use HoldLocation directly
					mDraggedUnit = pickedUnit;
					mDragStartPos = pickedUnit->getPos();
					mDragOffset = pickedUnit->getPos() - worldPos;
					mDragStartLocation = pickedUnit->getHoldLocation();

					return MsgReply::Handled();
				}
			}
			
			// Check Bench (World Space)
			Vector2 boardOffset = board.getOffset();
			Vector2 benchStartPos = boardOffset + Vector2(AB_BENCH_OFFSET_X, PlayerBoard::MapSize.y * PlayerBoard::CellSize.y * 0.75f + AB_BENCH_OFFSET_Y);
			int benchTotalWidth = AB_BENCH_SIZE * (AB_BENCH_SLOT_SIZE + AB_BENCH_GAP) - AB_BENCH_GAP;

			if (worldPos.y >= benchStartPos.y && worldPos.y < benchStartPos.y + AB_BENCH_SLOT_SIZE)
			{
				float relX = worldPos.x - benchStartPos.x;
				if (relX >= 0 && relX < benchTotalWidth)
				{
					int slotIndex = (int)(relX / (AB_BENCH_SLOT_SIZE + AB_BENCH_GAP));
					// Check gap
					float slotRelX = relX - slotIndex * (AB_BENCH_SLOT_SIZE + AB_BENCH_GAP);
					
					if (slotRelX < AB_BENCH_SLOT_SIZE && player.mBench.isValidIndex(slotIndex))
					{
						Unit* unit = player.mBench[slotIndex];
						if (unit)
						{
							mDraggedUnit = unit;
							// Calculate World Pos of slot logic
							Vector2 slotPos = benchStartPos + Vector2(slotIndex * (AB_BENCH_SLOT_SIZE + AB_BENCH_GAP), 0);
							mDragStartPos = slotPos + Vector2(AB_BENCH_SLOT_SIZE / 2.0f, AB_BENCH_SLOT_SIZE / 2.0f);
							
							mDragOffset = mDragStartPos - worldPos; 
							
							// For dragging visual, set pos to actual world pos
							unit->setPos(worldPos); 
							// Actually drag offset should be relative to unit center?
							// If we picked unit, unit->getPos() might be undefined if it was on bench.
							// So valid logic is: mDragStartPos is the slot center.
							// mDragOffset is (SlotCenter - MousePos).
							// When rendering dragged unit: Pos = MousePos + Offset = MousePos + SlotCenter - MousePosStart...
							// Actually standar drag logic:
							// OnPick: Offset = UnitPos - MousePos.
							// OnDrag: UnitPos = MousePos + Offset.
							// Here UnitPos (virtual) is SlotCenter.
							mDragOffset = mDragStartPos - worldPos;

							mDragStartLocation.type = ECoordType::Bench;
							mDragStartLocation.index = slotIndex;
							return MsgReply::Handled();
						}
					}
				}
			}
		}
		else if (msg.onLeftUp())
		{
			if (mDraggedUnit)
			{
				UnitLocation target = mContext.screenToDropTarget(msg.getPos());
				bool bDropSuccess = false;

				if (target.isBoard())
				{
					Vec2i coord = target.pos;
					bool bPhaseOK = (mWorld.getPhase() == BattlePhase::Preparation);
					bool bAreaOK = (coord.y >= PlayerBoard::MapSize.y / 2);

					if (bPhaseOK && bAreaOK)
					{
						// Check Population Cap Client-Side
						bool bCanDeploy = true;
						// Prevent dropping on occupied cell
						Unit* targetUnit = board.getUnit(coord.x, coord.y);
						if (targetUnit && targetUnit != mDraggedUnit)
						{
							bCanDeploy = false;
						}

						if (bCanDeploy && mDragStartLocation.type == ECoordType::Bench)
						{
							if (player.mUnits.size() >= player.getMaxPopulation())
							{
								bCanDeploy = false;
							}
						}

						if (bCanDeploy)
						{
							int srcType = (mDragStartLocation.type == ECoordType::Board) ? 0 : 1;
							int srcX = (srcType == 0) ? mDragStartLocation.pos.x : mDragStartLocation.index;
							int srcY = (srcType == 0) ? mDragStartLocation.pos.y : 0;
							
							mAction.deployUnit(player, srcType, srcX, srcY, coord.x, coord.y);
							
							// Client Prediction: Snap to Target
							mDraggedUnit->setPos(board.getWorldPos(coord.x, coord.y));
							bDropSuccess = true;
						}
					}
				}
				else if (target.isBench())
				{
					int slotIndex = target.index;
					
					int srcType = (mDragStartLocation.type == ECoordType::Board) ? 0 : 1;
					int srcX = (srcType == 0) ? mDragStartLocation.pos.x : mDragStartLocation.index;
					int srcY = (srcType == 0) ? mDragStartLocation.pos.y : 0;
					
					mAction.retractUnit(player, srcType, srcX, srcY, slotIndex);

					// Client Prediction: Snap to Bench Slot
					mDraggedUnit->setPos(player.getBenchSlotPos(slotIndex));
					bDropSuccess = true;
				}

				if (!bDropSuccess)
				{
					// Revert to start pos
					mDraggedUnit->setPos(mDragStartPos);

					// Send Cancel Sync to reset remotes
					int srcType = (mDragStartLocation.type == ECoordType::Board) ? 0 : 1;
					int srcIndex = (srcType == 0) ? (mDragStartLocation.pos.y * PlayerBoard::MapSize.x + mDragStartLocation.pos.x) : mDragStartLocation.index;
					mAction.syncDrag(player, srcType, srcIndex, 0, 0, true);
				}
				// If Success, we don't send SYNC_DRAG(bDrop=1) because that would reset position on remote.
				// We rely on ACT_DEPLOY arriving shortly to update remote state.

				mDraggedUnit = nullptr;
				mDragStartLocation = UnitLocation(); // Reset
			}
		}
		
		if (mDraggedUnit && msg.isLeftDown())
		{
			Vector2 worldPos = mContext.screenToWorld(Vector2(msg.getPos().x, msg.getPos().y));
			Vector2 newPos = worldPos + mDragOffset;
			mDraggedUnit->setPos(newPos);

			// Throttle Sync
			if (mContext.isNetMode())
			{
				static long lastSyncTime = 0;
				long curTime = ::Global::GameNet().getNetWorker()->getNetRunningTime();
				if (curTime - lastSyncTime > 30) // 30 FPS sync
				{
					lastSyncTime = curTime;
					int srcType = (mDragStartLocation.type == ECoordType::Board) ? 0 : 1;
					int srcIndex = (srcType == 0) ? (mDragStartLocation.pos.y * PlayerBoard::MapSize.x + mDragStartLocation.pos.x) : mDragStartLocation.index;
					
					mAction.syncDrag(player, srcType, srcIndex, (int)newPos.x, (int)newPos.y, false);
				}
			}
		}

		return MsgReply::Unhandled();
	}

	MsgReply ABPlayerController::onKey(KeyMsg const& msg)
	{
		if (msg.isDown())
		{
			Player& player = *mPlayer;

			switch (msg.getCode())
			{
			case EKeyCode::D:
				mAction.refreshShop(player);
				return MsgReply::Handled();
			case EKeyCode::F:
				mAction.buyExperience(player);
				return MsgReply::Handled();
			}
		}
		return MsgReply::Unhandled();
	}

	void ABPlayerController::renderDrag(IGraphics2D& g)
	{
		if (mDraggedUnit)
		{
			// Render highlight? logic from Stage?
			// Stage: Highlight Valid Cells
			PlayerBoard& board = mPlayer->getBoard();
			RenderUtility::SetBrush(g, EColor::Null);

			for (int y = PlayerBoard::MapSize.y / 2; y < PlayerBoard::MapSize.y; ++y)
			{
				// ... Copy render loop ...
				// Simplification:
				for (int x = 0; x < PlayerBoard::MapSize.x; ++x)
				{
					Vector2 pos = board.getWorldPos(x, y);
					
					bool bOccupied = board.isOccupied(x, y);
					bool bValid = !bOccupied;
					if (bOccupied)
					{
						Unit* other = board.getUnit(x, y);
						// Allow swap if friendly
						if (other && other->getTeam() == UnitTeam::Player)
							bValid = true;
					}

					if (bValid)
					{
						RenderUtility::SetPen(g, EColor::Green);
						g.drawCircle(pos, PlayerBoard::CellSize.x / 2 - 5);
					}
					else
					{
						RenderUtility::SetPen(g, EColor::Red);
						g.drawCircle(pos, PlayerBoard::CellSize.x / 2 - 5);
					}
				}
			}

			mDraggedUnit->render(g); 
		}
	}
}
