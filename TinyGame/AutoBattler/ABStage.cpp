#include "ABPCH.h"
#include "ABStage.h"
#include "ABNetwork.h"
#include "ABDefine.h"
#include "ABAction.h"
#include "ABBot.h"
#include <algorithm>
#include "ABGameRenderer.h"

#include "RenderUtility.h"
#include "GameGlobal.h"
#include "GameGUISystem.h"


namespace AutoBattler
{
	// Console variable to toggle between 2D and 3D rendering
	// false = 3D only, true = 2D only (debug mode)
	TConsoleVariable<bool> CVarRenderDebug{ true, "AB.RenderDebug", CVF_TOGGLEABLE };


	LevelStage::LevelStage()
	{
		mController = new ABPlayerController(*this, *this);
		mRenderer = new ABGameRenderer();
	}

	bool LevelStage::onInit()
	{
		if (!BaseClass::onInit())
			return false;

		mWorld.init();
		
		::Global::GUI().cleanupWidget();
		
		// ... UI setup

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
		delete mController;
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
		IGraphics2D& g = Global::GetIGraphics2D();
		if (CVarRenderDebug)
		{
			mRenderer->render2D(g, mWorld, mController);
		}
		else
		{
			mRenderer->render3D(mWorld);
		}

		g.beginRender();
		// 2. Render UI (Foreground)
		
		// Render UI info
		{
			InlineString<64> info;
			char const* phaseName = "Unknown";
			switch (mWorld.getPhase())
			{
			case BattlePhase::Preparation: phaseName = "Preparation"; break;
			case BattlePhase::Combat:      phaseName = "Combat"; break;
			}
			info.format("%s (%.1f)", phaseName, mWorld.getPhaseTimer());

			RenderUtility::SetFont(g, FONT_S24);
			g.setTextColor(Color3ub(255, 255, 255));
			g.drawText(Vec2i(10, 10), info);
			
			Player& player = mWorld.getLocalPlayer();
			info.format("Gold: %d  HP: %d  Lvl: %d", player.getGold(), player.getHp(), player.getLevel());
			g.drawText(Vec2i(10, 40), info);

			// View Info
			InlineString<32> viewInfo;
			viewInfo.format("Viewing Player %d", mViewPlayerIndex);
			viewInfo.format("Viewing Player %d", mViewPlayerIndex);
			g.drawText(Vec2i(10, 70), viewInfo);
		}

		if (mIsConnectionLost)
		{
			RenderUtility::SetFont(g, FONT_S24);
			g.setTextColor(Color3ub(255, 0, 0));
			g.drawText(Vec2i(300, 250), "Waiting for Reconnection...");
		}

		// Render Player List
		{
			Vec2i screenSize = ::Global::GetScreenSize();
			
			int rightMargin = 40;
			int startY = 70;
			int gapY = 48; // 0.9x scale roughly
			int baseRadius = 19; // 0.9x
			int localScale = 12; // 0.9x

			std::vector<Player*> sortedPlayers;
			for(int i=0; i<AB_MAX_PLAYER_NUM; ++i)
			{
				sortedPlayers.push_back(&mWorld.getPlayer(i));
			}

			std::sort(sortedPlayers.begin(), sortedPlayers.end(), [](Player* a, Player* b) {
				return a->getHp() > b->getHp();
			});

			for(int i=0; i < sortedPlayers.size(); ++i)
			{
				Player* p = sortedPlayers[i];
				bool isLocal = (p->getIndex() == mWorld.getLocalPlayerIndex());
				
				int radius = baseRadius + (isLocal ? localScale : 0);
				int currentGap = gapY + (isLocal ? 10 : 0);
				int yPos = startY + i * currentGap; // Simplified spacing
				
				// Fix spacing accumulation if local is in middle (Simpler hack: just expand current item)
				// For perfect spacing we would track 'currentY' accumulator.
			}
			
			// Re-loop with proper Y accumulation
			int currentY = startY;

			for(int i=0; i < sortedPlayers.size(); ++i)
			{
				Player* p = sortedPlayers[i];
				bool isLocal = (p->getIndex() == mWorld.getLocalPlayerIndex());
				int radius = baseRadius + (isLocal ? localScale : 0);
				
				currentY += (isLocal ? 15 : 0); // Extra top padding for local

				Vec2i center(screenSize.x - rightMargin, currentY);
				
				// 1. Avatar (Blue Circle Placeholder)
				RenderUtility::SetBrush(g, EColor::Blue);
				RenderUtility::SetPen(g, EColor::Black);
				g.drawCircle(center, radius - 4); // Inner circle

				// 2. HP Ring (Simulated with multiple circles)
				EColor::Name ringColor = isLocal ? EColor::Yellow : EColor::Red;
				if (p->getHp() <= 0) ringColor = EColor::Gray;

				RenderUtility::SetBrush(g, EColor::Null);
				RenderUtility::SetPen(g, ringColor);
				
				g.drawCircle(center, radius);
				g.drawCircle(center, radius - 1);
				if (isLocal) g.drawCircle(center, radius - 2);

				// 3. HP Box
				InlineString<32> hpStr;
				hpStr.format("%d", p->getHp());
				
				Vec2i hpBoxSize(30, 20);
				if (isLocal) hpBoxSize = Vec2i(40, 26);

				Vec2i hpBoxPos = center - Vec2i(radius + 5 + hpBoxSize.x, hpBoxSize.y / 2);
				
				// Background Box
				g.setBrush(Color4ub(0, 0, 0, 180)); // Semi-transparent black
				RenderUtility::SetPen(g, EColor::Null);
				g.drawRect(hpBoxPos, hpBoxSize);

				// HP Text
				g.setTextColor(Color3ub(255, 255, 255));
				RenderUtility::SetFont(g, isLocal ? FONT_S16 : FONT_S12);
				
				// Center text roughly
				int textOffX = isLocal ? 8 : 6;
				int textOffY = isLocal ? 4 : 2;
				g.drawText(hpBoxPos + Vec2i(textOffX, textOffY), hpStr);

				// 4. Name Tag (Only partial or small)
				InlineString<64> nameStr;
				nameStr.format("Player %d", p->getIndex()); // Placeholder Name
				
				RenderUtility::SetFont(g, FONT_S10);
				Vec2i nameBoxSize(80, 18);
				Vec2i nameBoxPos = hpBoxPos - Vec2i(nameBoxSize.x + 2, 0);
				// Align Name box vertically centered or slightly offset? Use same Y
				
				g.setBrush(Color4ub(0, 0, 0, 150)); 
				g.drawRect(nameBoxPos, nameBoxSize);
				g.drawText(nameBoxPos + Vec2i(5, 2), nameStr);

				// Advance Y
				currentY += gapY + (isLocal ? 15 : 0);
			}
		}
		
		// Render Shop UI
		{
			Vec2i screenSize = ::Global::GetScreenSize();
			
			int panelHeight = 140;
			int panelY = screenSize.y - panelHeight;
			
			// Main Background Area (Full Width or Centered?)
			// Image shows centered block.
			int cardWidth = 140;
			int cardHeight = 120; // Slightly smaller to fit panel
			int gap = 8;
			int controlWidth = 120;
			
			int totalWidth = controlWidth + gap + (5 * (cardWidth + gap)) + gap;
			int startX = (screenSize.x - totalWidth) / 2;
			int startY = panelY + (panelHeight - cardHeight) / 2;
			
			// Draw Background
			RenderUtility::SetBrush(g, EColor::Black); // Dark background
			RenderUtility::SetPen(g, EColor::Gray);
			g.drawRect(Vec2i(startX - 10, startY - 10), Vec2i(totalWidth + 20, cardHeight + 20));

			Player& player = mWorld.getLocalPlayer();

			// 1. Control Panel (Left)
			Vec2i controlPos(startX, startY);
			
			// Refresh Button (Top Half)
			Vec2i refreshPos = controlPos;
			Vec2i refreshSize(controlWidth, cardHeight / 2 - 2);
			
			g.setBrush(Color4ub(30,30,30, 255));
			g.drawRect(refreshPos, refreshSize);
			
			g.setTextColor(Color3ub(255, 200, 0));
			RenderUtility::SetFont(g, FONT_S12);
			g.drawText(refreshPos + Vec2i(10, 5), "Refresh");
			g.drawText(refreshPos + Vec2i(10, 25), "2g"); // Cost
			
			// Buy XP Button (Bottom Half)
			Vec2i xpPos = controlPos + Vec2i(0, cardHeight / 2 + 2);
			Vec2i xpSize(controlWidth, cardHeight / 2 - 2);

			g.setBrush(Color4ub(30, 40, 60, 255)); // Blue-ish
			g.drawRect(xpPos, xpSize);
			
			g.setTextColor(Color3ub(200, 200, 255));
			g.drawText(xpPos + Vec2i(10, 5), "Buy XP");
			g.drawText(xpPos + Vec2i(10, 25), "4g"); // Cost

			// XP Bar (Bottom of XP Button)
			// Assuming Max XP is 100 for now based on previous code
			float xpRatio = (float)player.getXp() / 100.0f; 
			if (xpRatio > 1.0f) xpRatio = 1.0f;
			
			RenderUtility::SetBrush(g, EColor::Black);
			g.drawRect(xpPos + Vec2i(5, 45), Vec2i(controlWidth - 10, 6)); // Bar BG
			RenderUtility::SetBrush(g, EColor::Cyan);
			g.drawRect(xpPos + Vec2i(5, 45), Vec2i((int)((controlWidth - 10) * xpRatio), 6)); // Bar Fill

			g.setTextColor(Color3ub(255, 255, 255));
			InlineString<32> lvlStr;
			lvlStr.format("Lvl %d", player.getLevel());
			g.drawText(xpPos + Vec2i(controlWidth - 40, 5), lvlStr);


			// 2. Gold Indicator (Top Center floating)
			// In image it's above the panel.
			Vec2i goldBoxPos(screenSize.x / 2 - 40, startY - 40);
			Vec2i goldBoxSize(80, 30);
			
			// Shape (Diamond-ish rect)
			g.setBrush(Color4ub(10, 10, 20, 255));
			RenderUtility::SetPen(g, EColor::Yellow);
			g.drawRect(goldBoxPos, goldBoxSize); // Simple rect for now
			
			InlineString<16> goldStr;
			goldStr.format("%d g", player.getGold());
			
			RenderUtility::SetFont(g, FONT_S16);
			g.setTextColor(Color3ub(255, 215, 0));
			// Center Text
			g.drawText(goldBoxPos + Vec2i(20, 5), goldStr);


			// 3. Unit Cards
			int cardsStartX = startX + controlWidth + gap;

			for (int i = 0; i < player.mShopList.size(); ++i)
			{
				int unitId = player.mShopList[i];
				Vec2i cardPos(cardsStartX + i * (cardWidth + gap), startY);
				
				if (unitId == 0) 
				{
					// Empty Slot
					RenderUtility::SetBrush(g, EColor::Black);
					RenderUtility::SetPen(g, EColor::Gray);
					g.drawRect(cardPos, Vec2i(cardWidth, cardHeight));
					continue;
				}

				UnitDefinition const* def = mWorld.getUnitDataManager().getUnit(unitId);
				
				// Rarity Color
				Color3ub rarityColor(128, 128, 128); // Cost 1
				if (def)
				{
					if (def->cost == 2) rarityColor = Color3ub(20, 200, 20); // Green
					if (def->cost == 3) rarityColor = Color3ub(20, 50, 200); // Blue
					if (def->cost == 4) rarityColor = Color3ub(180, 20, 200); // Purple
					if (def->cost == 5) rarityColor = Color3ub(255, 200, 0); // Gold
				}

				// Card BG (Dark)
				g.setBrush(Color4ub(10, 10, 10, 255));
				// Border cost color
				g.setPen(rarityColor);
				g.drawRect(cardPos, Vec2i(cardWidth, cardHeight));

				// Inner Image Area (Placeholder)
				// g.setBrush(Brush(rarityColor, 100)); // Tint
				// g.drawRect(cardPos + Vec2i(5, 5), Vec2i(cardWidth - 10, cardHeight - 30));

				if (def)
				{
					g.setTextColor(Color3ub(255, 255, 255));
					
					// Name (Bottom Left)
					RenderUtility::SetFont(g, FONT_S12);
					g.drawText(cardPos + Vec2i(5, cardHeight - 20), def->name);
					
					// Cost (Bottom Right)
					InlineString<16> cStr;
					cStr.format("%d", def->cost);
					g.setTextColor(rarityColor);
					g.drawText(cardPos + Vec2i(cardWidth - 15, cardHeight - 20), cStr);
					
					// Traits (Top Left)
					// Placeholder Text
					g.setTextColor(Color3ub(180, 180, 180));
					RenderUtility::SetFont(g, FONT_S10);
					g.drawText(cardPos + Vec2i(5, 5), "Trait 1");
					g.drawText(cardPos + Vec2i(5, 18), "Trait 2");
				}
			}
		}

		g.finishXForm();
		g.endRender();
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
			mViewOffset += Vector2(offset.x, offset.y);
			lastPos = msg.getPos();
		}
		
		if (msg.onWheelFront())
		{
			mViewScale *= 1.1f;
		}
		else if (msg.onWheelBack())
		{
			mViewScale /= 1.1f;
		}

		// Sync view settings to 3D renderer
		if (mRenderer)
		{
			mRenderer->setViewOffset(mViewOffset);
			mRenderer->setScale(mViewScale);
		}

		return BaseClass::onMouse(msg);
	}

	Vector2 LevelStage::screenToWorld(Vector2 const& screenPos) const
	{
		return (screenPos - mViewOffset) / mViewScale;
	}

	Vector2 LevelStage::worldToScreen(Vector2 const& worldPos) const
	{
		return worldPos * mViewScale + mViewOffset;
	}


	Unit* LevelStage::pick3D(Vec2i screenPos, World& world)
	{
		Vec2i screenSize = ::Global::GetScreenSize();

		float ndcX = (2.0f * screenPos.x / screenSize.x) - 1.0f;
		float ndcY = 1.0f - (2.0f * screenPos.y / screenSize.y);

		Matrix4 clipToView;
		float detProj;
		mRenderer->getProjMatrix().inverse(clipToView, detProj);

		Matrix4 viewToWorld;
		float detView;
		mRenderer->getViewMatrix().inverse(viewToWorld, detView);

		Matrix4 clipToWorld = clipToView * viewToWorld;

		Vector3 nearPos = (Vector4(ndcX, ndcY, 0.0f, 1.0f) * clipToWorld).dividedVector();
		Vector3 farPos = (Vector4(ndcX, ndcY, 1.0f, 1.0f) * clipToWorld).dividedVector();

		Vector3 rayOrigin = nearPos;
		Vector3 rayDir = Math::GetNormal(farPos - nearPos);

		float t = -rayOrigin.z / rayDir.z;
		Vector3 groundHit = rayOrigin + rayDir * t;
		Vector2 worldPos2D(groundHit.x, groundHit.y);

		Unit* closestUnit = nullptr;
		float closestDistSq = FLT_MAX;

		float pickRadius = PlayerBoard::CellSize.x * 0.5f;
		float pickRadiusSq = pickRadius * pickRadius;

		for (int playerIdx = 0; playerIdx < AB_MAX_PLAYER_NUM; ++playerIdx)
		{
			Player& player = world.getPlayer(playerIdx);
			PlayerBoard& board = player.getBoard();

			for (int y = 0; y < PlayerBoard::MapSize.y; ++y)
			{
				for (int x = 0; x < PlayerBoard::MapSize.x; ++x)
				{
					Unit* unit = board.getUnit(x, y);
					if (!unit) continue;

					Vector2 pos2D = unit->getPos();

					float distSq = (pos2D - worldPos2D).length2();
					if (distSq < pickRadiusSq && distSq < closestDistSq)
					{
						closestDistSq = distSq;
						closestUnit = unit;
					}
				}
			}

			for (int i = 0; i < player.mBench.size(); ++i)
			{
				Unit* unit = player.mBench[i];
				if (!unit) continue;

				Vector2 pos2D = unit->getPos();
				float distSq = (pos2D - worldPos2D).length2();
				if (distSq < pickRadiusSq && distSq < closestDistSq)
				{
					closestDistSq = distSq;
					closestUnit = unit;
				}
			}
		}

		return closestUnit;
	}

	Unit* LevelStage::pickUnit(Vec2i screenPos)
	{
		// Try 3D picking first
		if (CVarRenderDebug)
		{		
			// 2D fallback: use screenToWorld conversion
			Vector2 worldPos = screenToWorld(Vector2((float)screenPos.x, (float)screenPos.y));
			float pickRadius = PlayerBoard::CellSize.x * 0.5f;
			float pickRadiusSq = pickRadius * pickRadius;

			Unit* closestUnit = nullptr;
			float closestDistSq = FLT_MAX;

			// Check local player's board units
			Player& player = mWorld.getLocalPlayer();
			for (int i = 0; i < player.mUnits.size(); ++i)
			{
				Unit* unit = player.mUnits[i];
				if (!unit) continue;

				float distSq = (unit->getPos() - worldPos).length2();
				if (distSq < pickRadiusSq && distSq < closestDistSq)
				{
					closestDistSq = distSq;
					closestUnit = unit;
				}
			}

			// Check local player's bench units
			PlayerBoard& board = player.getBoard();
			Vector2 boardOffset = board.getOffset();
			Vector2 benchStartPos = boardOffset + Vector2(0, PlayerBoard::MapSize.y * PlayerBoard::CellSize.y * 0.75f + 20);
			int benchSlotSize = 40;
			int benchGap = 5;

			for (int i = 0; i < player.mBench.size(); ++i)
			{
				Unit* unit = player.mBench[i];
				if (!unit) continue;

				// Calculate slot center position
				Vector2 slotPos = benchStartPos + Vector2(i * (benchSlotSize + benchGap) + benchSlotSize / 2, benchSlotSize / 2);
				float distSq = (slotPos - worldPos).length2();
				float benchPickRadiusSq = (benchSlotSize * 0.5f) * (benchSlotSize * 0.5f);

				if (distSq < benchPickRadiusSq && distSq < closestDistSq)
				{
					closestDistSq = distSq;
					closestUnit = unit;
				}
			}

			return closestUnit;
		}
		else
		{
			return pick3D(screenPos, mWorld);
		}
	}

	UnitLocation LevelStage::screenToDropTarget(Vec2i screenPos)
	{
		UnitLocation result;
		PlayerBoard& board = mWorld.getLocalPlayerBoard();
		Player& player = mWorld.getLocalPlayer();

		// Get world position (needed for bench check in both 2D and 3D modes)
		Vector2 worldPos2D;
		
		// Try 3D: get world position via ray-ground intersection
		if (mRenderer)
		{
			Vec2i screenSize = ::Global::GetScreenSize();
			
			float ndcX = (2.0f * screenPos.x / screenSize.x) - 1.0f;
			float ndcY = 1.0f - (2.0f * screenPos.y / screenSize.y);
			
			Matrix4 clipToView;
			float detProj;
			mRenderer->getProjMatrix().inverse(clipToView, detProj);
			
			Matrix4 viewToWorld;
			float detView;
			mRenderer->getViewMatrix().inverse(viewToWorld, detView);
			
			Matrix4 clipToWorld = clipToView * viewToWorld;
			
			Vector3 nearPos = (Math::Vector4(ndcX, ndcY, 0.0f, 1.0f) * clipToWorld).dividedVector();
			Vector3 farPos = (Math::Vector4(ndcX, ndcY, 1.0f, 1.0f) * clipToWorld).dividedVector();
			
			Vector3 rayOrigin = nearPos;
			Vector3 rayDir = Math::GetNormal(farPos - nearPos);
			
			// Intersect with ground plane (Z = 0)
			if (Math::Abs(rayDir.z) > 0.0001f)
			{
				float t = -rayOrigin.z / rayDir.z;
				Vector3 groundHit = rayOrigin + rayDir * t;
				worldPos2D = Vector2(groundHit.x, groundHit.y);
			}
			else
			{
				// Fallback to 2D conversion if ray is parallel to ground
				worldPos2D = screenToWorld(Vector2((float)screenPos.x, (float)screenPos.y));
			}
		}
		else
		{
			// 2D mode
			worldPos2D = screenToWorld(Vector2((float)screenPos.x, (float)screenPos.y));
		}

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
		Vector2 benchStartPos = boardOffset + Vector2(0, PlayerBoard::MapSize.y * PlayerBoard::CellSize.y * 0.75f + 20);
		int benchSlotSize = 40;
		int benchGap = 5;
		int benchTotalWidth = AB_BENCH_SIZE * (benchSlotSize + benchGap) - benchGap;

		if (worldPos2D.y >= benchStartPos.y && worldPos2D.y < benchStartPos.y + benchSlotSize &&
			worldPos2D.x >= benchStartPos.x && worldPos2D.x < benchStartPos.x + benchTotalWidth)
		{
			float relX = worldPos2D.x - benchStartPos.x;
			int slotIndex = (int)(relX / (benchSlotSize + benchGap));
			float slotRelX = relX - slotIndex * (benchSlotSize + benchGap);

			// Make sure we're on the slot itself, not the gap
			if (slotRelX < benchSlotSize && player.mBench.isValidIndex(slotIndex))
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
				mViewPlayerIndex--;
				if (mViewPlayerIndex < 0) mViewPlayerIndex = AB_MAX_PLAYER_NUM - 1;
				
				Player& player = mWorld.getPlayer(mViewPlayerIndex);
				Vector2 boardOffset = player.getBoard().getOffset();
				// Center on this board
				Vec2i screenSize = ::Global::GetScreenSize();
				mViewOffset = Vector2(screenSize.x / 2, screenSize.y / 2) - (boardOffset + Vector2(300, 300)) * mViewScale;
				
				// Sync to 3D renderer
				if (mRenderer)
				{
					mRenderer->setViewOffset(mViewOffset);
					mRenderer->setScale(mViewScale);
				}
			}
			break;
			case EKeyCode::E: // Next Player
			{
				mViewPlayerIndex++;
				if (mViewPlayerIndex > 7) mViewPlayerIndex = 0;
				
				Player& player = mWorld.getPlayer(mViewPlayerIndex);
				Vector2 boardOffset = player.getBoard().getOffset();
				// Center on this board
				Vec2i screenSize = ::Global::GetScreenSize();
				mViewOffset = Vector2(screenSize.x / 2, screenSize.y / 2) - (boardOffset + Vector2(300, 300)) * mViewScale;
				
				// Sync to 3D renderer
				if (mRenderer)
				{
					mRenderer->setViewOffset(mViewOffset);
					mRenderer->setScale(mViewScale);
				}
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
