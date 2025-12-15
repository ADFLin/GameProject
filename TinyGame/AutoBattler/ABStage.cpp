#include "ABPCH.h"
#include "ABStage.h"
#include "ABNetwork.h"
#include "ABDefine.h"
#include "ABAction.h"
#include "ABBot.h"
#include "ABGameRenderer.h"
#include "ABViewCamera.h"
#include "ABGameHUD.h"

#include "RenderUtility.h"
#include "GameGlobal.h"
#include "GameGUISystem.h"


namespace AutoBattler
{
	// Console variable to toggle between 2D and 3D rendering
	// false = 3D only, true = 2D only (debug mode)
	TConsoleVariable<bool> CVarRenderDebug{false, "AB.RenderDebug", CVF_TOGGLEABLE };


	LevelStage::LevelStage()
		: mController(std::make_unique<ABPlayerController>(*this, *this))
		, mRenderer(std::make_unique<ABGameRenderer>())
		, mHUD(std::make_unique<ABGameHUD>())
	{
	}

	bool LevelStage::onInit()
	{
		if (!BaseClass::onInit())
			return false;

		mWorld.init();
		
		::Global::GUI().cleanupWidget();
		
		// Initialize HUD (creates shop panel and other UI)
		mHUD->init(&mWorld, this);

		return true;
	}

	void LevelStage::onEnd()
	{
		for (auto bot : mBots)
		{
			delete bot;
		}
		mBots.clear();

		mWorld.cleanup();
		// mController, mRenderer, mHUD are automatically cleaned up by unique_ptr
		BaseClass::onEnd();
	}

	void LevelStage::onRestart(bool beInit)
	{
		mWorld.restart(beInit);
	}

	void LevelStage::runLogic(float dt)
	{
		for (auto bot : mBots)
			bot->update(dt);
		mWorld.tick(dt);
	}

	void LevelStage::tick()
	{
		if (getModeType() != EGameStageMode::Net)
		{
			runLogic(float(getTickTime()) / 1000.0f);
		}
	}

	IFrameActionTemplate* LevelStage::createActionTemplate(unsigned version)
	{
		if (getModeType() == EGameStageMode::Net)
		{
			ABFrameActionTemplate* ptr = nullptr;
			auto netMode = getGameStage()->getStageMode()->getNetLevelMode();
			if (netMode && netMode->haveServer())
			{
				ptr = new ABServerNetFrameCollector(AB_MAX_PLAYER_NUM);
			}
			else
			{
				ptr = new ABClientNetFrameCollector(AB_MAX_PLAYER_NUM);
			}
			return ptr;
		}
		return nullptr;
	}

	void LevelStage::configLevelSetting(GameLevelInfo& info)
	{
		info.seed = ::Global::Random();
	}

	void LevelStage::setupLocalGame(LocalPlayerManager& playerManager)
	{
		GamePlayer* localPlayer = playerManager.createPlayer(0);
		localPlayer->setSlot(0);
		playerManager.setUserID(0);


		for (int i = 1; i < AB_MAX_PLAYER_NUM; ++i)
		{
			GamePlayer* player = playerManager.createPlayer(i);
			player->setType(PT_COMPUTER);
			player->setSlot(i);
		}
	}

	void LevelStage::setupLevel(GameLevelInfo const& info)
	{
		::Global::RandSeedNet(info.seed);
		mWorld.restart(true);
	}

	void LevelStage::setupScene(IPlayerManager& playerManager)
	{
		for (auto it = playerManager.createIterator(); it; ++it)
		{
			GamePlayer* gp = it.getElement();
			if (gp->getType() != PT_SPECTATORS)
			{
				gp->setActionPort(gp->getSlot());
			}
		}

		int localID = playerManager.getUserID();
		if (localID != ERROR_PLAYER_ID)
		{
			mWorld.setLocalPlayerIndex(localID);
			mController->setPlayer(mWorld.getLocalPlayer());
		}

		bool bCanCreateBot = true;
		if (getModeType() == EGameStageMode::Net)
		{
			if (!::Global::GameNet().haveServer())
				bCanCreateBot = false;


		}

		if (bCanCreateBot)
		{
			bool filled[AB_MAX_PLAYER_NUM] = { false };

			for (auto it = playerManager.createIterator(); it; ++it)
			{
				GamePlayer* gp = it.getElement();
				if (gp->getType() == PT_COMPUTER)
				{
					int id = gp->getActionPort();
					if (0 <= id && id < AB_MAX_PLAYER_NUM)
					{
						Player& player = mWorld.getPlayer(id);
						mBots.push_back(new BotController(player, *this));
					}
				}
				
				int id = gp->getActionPort();
				if (0 <= id && id < AB_MAX_PLAYER_NUM)
					filled[id] = true;
			}

			if (mUseBots)
			{
				for (int i = 0; i < AB_MAX_PLAYER_NUM; ++i)
				{
					if (!filled[i])
					{
						Player& player = mWorld.getPlayer(i);
						mBots.push_back(new BotController(player, *this));
					}
				}
			}
		}
	}

	bool LevelStage::setupNetwork(NetWorker* netWorker, INetEngine** engine)
	{
		ABNetEngine* netEngine = new ABNetEngine(this);
		*engine = netEngine;
		mNetEngine = netEngine;

		netEngine->OnTimeout = [this](int playerId, bool bLost)
		{
			mIsConnectionLost = bLost;
		};
		return true;
	}

	void LevelStage::executeAction(ActionPort port, ABActionItem const& item)
	{
		// Map Port to Player Index
		// Ideally we use IPlayerManager, but for now assuming Port == PlayerIndex (if strictly assigned)
		// Or assume local linear mapping 0..N
		int playerIndex = (int)port;

		if (playerIndex >= 0 && playerIndex < AB_MAX_PLAYER_NUM)
		{
			Player& player = mWorld.getPlayer(playerIndex);

			switch (item.type)
			{
			case ACT_BUY_UNIT:
				player.buyUnit(item.buy.slotIndex);
				break;
			case ACT_SELL_UNIT:
				player.sellUnit(item.sell.slotIndex);
				break;
			case ACT_REFRESH_SHOP:
				player.rerollShop();
				break;
			case ACT_LEVEL_UP:
				player.buyExperience();
				break;
			case ACT_DEPLOY_UNIT:
				{
					// Resolve Unit from Source
					Unit* unit = nullptr;
					Vec2i srcPos(-1, -1);
					
					if (item.deploy.srcType == 1) // Bench
					{
						if (player.mBench.isValidIndex(item.deploy.srcX))
							unit = player.mBench[item.deploy.srcX];
					}
					else // Board
					{
						int x = item.deploy.srcX;
						int y = item.deploy.srcY;
						if (player.getBoard().isValid(Vec2i(x, y)))
						{
							unit = player.getBoard().getUnit(x, y);
							srcPos = Vec2i(x, y);
						}
					}

					if (unit)
					{
						player.deployUnit(unit, Vec2i(item.deploy.destX, item.deploy.destY), srcPos);
					}
				}
				break;
			case ACT_RETRACT_UNIT:
				{
					Unit* unit = nullptr;
					if (item.retract.srcType == 0) // Board
					{
						int x = item.retract.srcX;
						int y = item.retract.srcY;
						if (player.getBoard().isValid(Vec2i(x, y)))
							unit = player.getBoard().getUnit(x, y);
					}
					else // Bench
					{
						if (player.mBench.isValidIndex(item.retract.srcX))
							unit = player.mBench[item.retract.srcX];
					}

					if (unit)
					{
						Vec2i srcPos(-1, -1);
						if (item.retract.srcType == 0) srcPos = Vec2i(item.retract.srcX, item.retract.srcY);
						player.retractUnit(unit, item.retract.benchIndex, srcPos);
					}
				}
				break;

			case ACT_SYNC_DRAG:
				{
					// Ignore local player's own drag (handled locally by mouse)
					if ((int)port == mWorld.getLocalPlayerIndex())
						break;

					Unit* unit = nullptr;
					if (item.syncDrag.srcType == 0) // Board
					{
						// Need Board from Player?
						// "port" maps to player index.
						if (player.getBoard().isValid(Vec2i(item.syncDrag.srcIndex % PlayerBoard::MapSize.x, item.syncDrag.srcIndex / PlayerBoard::MapSize.x)))
						{
							// Wait, srcIndex for Board needs X,Y.
							// In packet we used srcIndex.
							// We need to encode X,Y into Index or use X,Y.
							// ActionSyncDrag has srcIndex.
							// But Board is 2D.
							// Let's assume srcIndex = y * Width + x.
							// Check how we encoded it in Send.
							// We haven't implemented Send yet.
							// Let's assume we use getIndex(x,y).
							// PlayerBoard::getIndex is private.
							// But we can verify with:
							int x = item.syncDrag.srcIndex % PlayerBoard::MapSize.x;
							int y = item.syncDrag.srcIndex / PlayerBoard::MapSize.x;
							unit = player.getBoard().getUnit(x, y);
						}
					}
					else // Bench
					{
						if (player.mBench.isValidIndex(item.syncDrag.srcIndex))
							unit = player.mBench[item.syncDrag.srcIndex];
					}

					if (unit)
					{
						if (item.syncDrag.bDrop)
						{
							// Reset to Source Pos (Snap back)
							// If ACT_DEPLOY follows, it will overwrite this.
							// Use resolve logic to find "Start Pos".
							// Actually simpler: Recalculate based on Source.
							if (item.syncDrag.srcType == 0)
							{
								int x = item.syncDrag.srcIndex % PlayerBoard::MapSize.x;
								int y = item.syncDrag.srcIndex / PlayerBoard::MapSize.x;
								unit->setPos(player.getBoard().getPos(x, y));
							}
							else
							{
								unit->setPos(player.getBenchSlotPos(item.syncDrag.srcIndex));
							}
							LogMsg("SyncDrag: Drop Reset Unit Port %d", (int)port);
						}
						else
						{
							// Update Visual Pos
							unit->setPos(Vector2((float)item.syncDrag.posX, (float)item.syncDrag.posY));
							// LogMsg("SyncDrag: Move Unit Port %d to %d %d", (int)port, item.syncDrag.posX, item.syncDrag.posY);
						}
					}
					else
					{
						LogMsg("SyncDrag: Unit Not Found Port %d Src %d %d", (int)port, item.syncDrag.srcType, item.syncDrag.srcIndex);
					}
				}
				break;
			}
		}
	}
	
	void LevelStage::sendAction(ABActionItem const& item)
	{
		unsigned id = ERROR_PLAYER_ID;
		if (getModeType() == EGameStageMode::Net)
		{
			auto* manager = getGameStage()->getStageMode()->getPlayerManager();
			id = manager->getUserID();
		}

		if (id == ERROR_PLAYER_ID)
			id = mWorld.getLocalPlayerIndex();

		sendAction((ActionPort)id, item);
	}

	void LevelStage::sendAction(ActionPort port, ABActionItem const& item)
	{
		if (getModeType() == EGameStageMode::Net)
		{
			if (mNetEngine)
			{
				mNetEngine->sendAction(port, item);
			}
		}
		else
		{
			executeAction(port, item);
		}
	}

	void LevelStage::buyUnit(Player& player, int slotIndex)
	{
		ABActionItem item;
		item.type = ACT_BUY_UNIT;
		item.buy.slotIndex = slotIndex;
		ActionPort port = (ActionPort)player.getIndex();
		sendAction(port, item);
	}

	void LevelStage::sellUnit(Player& player, int slotIndex)
	{
		ABActionItem item;
		item.type = ACT_SELL_UNIT;
		item.sell.slotIndex = slotIndex;
		ActionPort port = (ActionPort)player.getIndex();
		sendAction(port, item);
	}

	void LevelStage::refreshShop(Player& player)
	{
		ABActionItem item;
		item.type = ACT_REFRESH_SHOP;
		ActionPort port = (ActionPort)player.getIndex();
		sendAction(port, item);
	}

	void LevelStage::buyExperience(Player& player)
	{
		ABActionItem item;
		item.type = ACT_LEVEL_UP;
		ActionPort port = (ActionPort)player.getIndex();
		sendAction(port, item);
	}

	void LevelStage::deployUnit(Player& player, int srcType, int srcX, int srcY, int destX, int destY)
	{
		ABActionItem item;
		item.type = ACT_DEPLOY_UNIT;
		item.deploy.srcType = srcType;
		item.deploy.srcX = srcX;
		item.deploy.srcY = srcY;
		item.deploy.destX = destX;
		item.deploy.destY = destY;
		ActionPort port = (ActionPort)player.getIndex();
		sendAction(port, item);
	}

	void LevelStage::retractUnit(Player& player, int srcType, int srcX, int srcY, int benchIndex)
	{
		ABActionItem item;
		item.type = ACT_RETRACT_UNIT;
		item.retract.srcType = srcType;
		item.retract.srcX = srcX;
		item.retract.srcY = srcY;
		item.retract.benchIndex = benchIndex;
		ActionPort port = (ActionPort)player.getIndex();
		sendAction(port, item);
	}

	void LevelStage::syncDrag(Player& player, int srcType, int srcIndex, int posX, int posY, bool bDrop)
	{
		ABActionItem item;
		item.type = ACT_SYNC_DRAG;
		item.syncDrag.srcType = srcType;
		item.syncDrag.srcIndex = srcIndex;
		item.syncDrag.posX = posX;
		item.syncDrag.posY = posY;
		item.syncDrag.bDrop = bDrop ? 1 : 0;
		ActionPort port = (ActionPort)player.getIndex();
		sendAction(port, item);
	}

	void LevelStage::onUpdate(GameTimeSpan deltaTime)
	{
		BaseClass::onUpdate(deltaTime);
		if (getModeType() != EGameStageMode::Net)
		{
			mWorld.tick(deltaTime);
		}
	}

	void LevelStage::onRender(float dFrame)
	{
		// Update camera matrices before rendering (also needed for picking)
		mCamera.updateMatrices(::Global::GetScreenSize());

		IGraphics2D& g = Global::GetIGraphics2D();
		if (CVarRenderDebug)
		{
			mRenderer->render2D(g, mWorld, mCamera, mController.get());
		}
		else
		{
			mRenderer->render3D(mWorld, mCamera);
		}

		// Update and render HUD
		mHUD->setConnectionLost(mIsConnectionLost);
		mHUD->update();
		mHUD->render(g, mCamera);
	}

	ERenderSystem LevelStage::getDefaultRenderSystem()
	{
		return ERenderSystem::D3D11;
	}

	void LevelStage::configRenderSystem(ERenderSystem systenName, RenderSystemConfigs& systemConfigs)
	{
		systemConfigs.bWasUsedPlatformGraphics = true;
		systemConfigs.screenWidth = 1080;
		systemConfigs.screenHeight = 768;
	}

	bool LevelStage::setupRenderResource(ERenderSystem systemName)
	{
		if (mRenderer)
			mRenderer->init();
		return true;
	}

	void LevelStage::preShutdownRenderSystem(bool bReInit /*= false*/)
	{

	}

	MsgReply LevelStage::onMouse(MouseMsg const& msg)
	{
		MsgReply reply = mController->onMouse(msg);
		if (reply.isHandled())
			return reply;

		static Vec2i lastPos = msg.getPos();
		if (msg.onRightDown())
		{
			lastPos = msg.getPos();
		}
		else if (msg.isRightDown() && msg.getPos() != lastPos)
		{
			Vec2i offset = msg.getPos() - lastPos;
			mCamera.pan(offset);
			lastPos = msg.getPos();
		}
		
		if (msg.onWheelFront())
		{
			mCamera.zoom(1.1f);
		}
		else if (msg.onWheelBack())
		{
			mCamera.zoom(1.0f / 1.1f);
		}

		return BaseClass::onMouse(msg);
	}

	Vector2 LevelStage::screenToWorld(Vector2 const& screenPos) const
	{
		return mCamera.screenToWorld(screenPos);
	}

	Vector2 LevelStage::worldToScreen(Vector2 const& worldPos) const
	{
		return mCamera.worldToScreen(worldPos);
	}

	Unit* LevelStage::pickUnitAtWorldPos(Vector2 worldPos, float pickRadius)
	{
		Player& player = mWorld.getLocalPlayer();
		
		Unit* closestUnit = nullptr;
		float closestDistSq = FLT_MAX;
		float pickRadiusSq = pickRadius * pickRadius;

		// Check all units owned by local player
		for (Unit* unit : player.mUnits)
		{
			if (!unit) continue;

			float distSq = (unit->getPos() - worldPos).length2();
			if (distSq < pickRadiusSq && distSq < closestDistSq)
			{
				closestDistSq = distSq;
				closestUnit = unit;
			}
		}

		// Check bench units
		for (int i = 0; i < player.mBench.size(); ++i)
		{
			Unit* unit = player.mBench[i];
			if (!unit) continue;

			float distSq = (unit->getPos() - worldPos).length2();
			if (distSq < pickRadiusSq && distSq < closestDistSq)
			{
				closestDistSq = distSq;
				closestUnit = unit;
			}
		}

		return closestUnit;
	}

	Unit* LevelStage::pickUnit(Vec2i screenPos)
	{
		// Convert screen position to world position (unified 2D/3D conversion)
		Vector2 worldPos = mCamera.screenToWorldPos(Vector2((float)screenPos.x, (float)screenPos.y), !CVarRenderDebug);

		float pickRadius = PlayerBoard::CellSize.x * 0.5f;
		return pickUnitAtWorldPos(worldPos, pickRadius);
	}

	UnitLocation LevelStage::screenToDropTarget(Vec2i screenPos)
	{
		UnitLocation result;
		PlayerBoard& board = mWorld.getLocalPlayerBoard();
		Player& player = mWorld.getLocalPlayer();

		// Get world position (unified 2D/3D conversion)
		Vector2 worldPos2D = mCamera.screenToWorldPos(Vector2((float)screenPos.x, (float)screenPos.y), !CVarRenderDebug);

		// 1. Check Board first
		Vec2i coord = board.getCoord(worldPos2D);
		if (board.isValid(coord))
		{
			result.type = ECoordType::Board;
			result.pos = coord;
			return result;
		}

		// 2. Check Bench
		Vector2 boardOffset = board.getOffset();
		Vector2 benchStartPos = boardOffset + Vector2(AB_BENCH_OFFSET_X, PlayerBoard::MapSize.y * PlayerBoard::CellSize.y * 0.75f + AB_BENCH_OFFSET_Y);
		int benchTotalWidth = AB_BENCH_SIZE * (AB_BENCH_SLOT_SIZE + AB_BENCH_GAP) - AB_BENCH_GAP;

		if (worldPos2D.y >= benchStartPos.y && worldPos2D.y < benchStartPos.y + AB_BENCH_SLOT_SIZE &&
			worldPos2D.x >= benchStartPos.x && worldPos2D.x < benchStartPos.x + benchTotalWidth)
		{
			float relX = worldPos2D.x - benchStartPos.x;
			int slotIndex = (int)(relX / (AB_BENCH_SLOT_SIZE + AB_BENCH_GAP));
			float slotRelX = relX - slotIndex * (AB_BENCH_SLOT_SIZE + AB_BENCH_GAP);

			// Make sure we're on the slot itself, not the gap
			if (slotRelX < AB_BENCH_SLOT_SIZE && player.mBench.isValidIndex(slotIndex))
			{
				result.type = ECoordType::Bench;
				result.index = slotIndex;
				return result;
			}
		}

		// 3. No valid target
		return result;
	}

	MsgReply LevelStage::onKey(KeyMsg const& msg)
	{
		MsgReply reply = mController->onKey(msg);
		if (reply.isHandled())
			return reply;
	
		if (msg.isDown())
		{
			switch (msg.getCode())
			{
			case EKeyCode::Q: // Previous Player
			{
				mCamera.prevPlayer();
				Player& player = mWorld.getPlayer(mCamera.getViewPlayerIndex());
				mCamera.centerOnBoard(player.getBoard(), ::Global::GetScreenSize());
			}
			break;
			case EKeyCode::E: // Next Player
			{
				mCamera.nextPlayer();
				Player& player = mWorld.getPlayer(mCamera.getViewPlayerIndex());
				mCamera.centerOnBoard(player.getBoard(), ::Global::GetScreenSize());
			}
			break;
			case EKeyCode::Space:
				if (mWorld.getPhase() == BattlePhase::Preparation)
					mWorld.changePhase(BattlePhase::Combat);
				else if (mWorld.getPhase() == BattlePhase::Combat)
					mWorld.changePhase(BattlePhase::Preparation);
				break;
			}
		}
		return BaseClass::onKey(msg);
	}

}
