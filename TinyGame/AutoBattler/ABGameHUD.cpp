#include "ABPCH.h"
#include "ABGameHUD.h"
#include "ABWorld.h"
#include "ABViewCamera.h"
#include "ABShopUI.h"
#include "RenderUtility.h"
#include "GameGUISystem.h"

#include <algorithm>

#include "GameGlobal.h"
#include "GameGraphics2D.h"

namespace AutoBattler
{

	// UI Layout Constants
	namespace HUDLayout
	{
		// Player List (Right Side)
		constexpr int PlayerListRightMargin = 40;
		constexpr int PlayerListStartY = 70;
		constexpr int PlayerListGapY = 48;
		constexpr int PlayerAvatarBaseRadius = 19;
		constexpr int PlayerAvatarLocalScale = 12;
	}

	ABGameHUD::ABGameHUD()
	{
	}

	ABGameHUD::~ABGameHUD()
	{
		cleanup();
	}

	void ABGameHUD::init(World* world, IGameActionControl* actionControl)
	{
		mWorld = world;
		mActionControl = actionControl;

		createShopPanel();
	}

	void ABGameHUD::cleanup()
	{
		// Shop panel is managed by GUI system, it will be destroyed when GUI cleans up
		mShopPanel = nullptr;
		mWorld = nullptr;
		mActionControl = nullptr;
	}

	void ABGameHUD::createShopPanel()
	{
		using namespace ShopLayout;

		Vec2i screenSize = ::Global::GetScreenSize();
		int totalWidth = ControlWidth + CardGap + (AB_SHOP_SLOT_COUNT * (CardWidth + CardGap)) + 40;
		int panelWidth = totalWidth;
		int panelHeight = PanelHeight;
		Vec2i panelPos((screenSize.x - panelWidth) / 2, screenSize.y - panelHeight - 10);
		Vec2i panelSize(panelWidth, panelHeight);

		mShopPanel = new ABShopPanel(UI_AB_SHOP_BASE, panelPos, panelSize, nullptr);
		mShopPanel->setWorld(mWorld);
		mShopPanel->setActionControl(mActionControl);
		mShopPanel->initButtons();

		::Global::GUI().addWidget(mShopPanel);
	}

	void ABGameHUD::update()
	{
		if (mShopPanel)
		{
			mShopPanel->updateFromWorld(*mWorld);
		}
	}

	void ABGameHUD::render(IGraphics2D& g, ABViewCamera const& camera)
	{
		g.beginRender();

		// Render UI elements (Shop is handled by ABShopPanel widget)
		renderPhaseInfo(g);
		renderPlayerInfo(g, mWorld->getLocalPlayer());
		renderViewInfo(g, camera);
		renderConnectionStatus(g);
		renderPlayerList(g);

		g.endRender();
	}

	void ABGameHUD::renderPhaseInfo(IGraphics2D& g)
	{
		InlineString<64> info;
		char const* phaseName = "Unknown";
		switch (mWorld->getPhase())
		{
		case BattlePhase::Preparation: phaseName = "Preparation"; break;
		case BattlePhase::Combat:      phaseName = "Combat"; break;
		}
		info.format("%s (%.1f)", phaseName, mWorld->getPhaseTimer());

		RenderUtility::SetFont(g, FONT_S24);
		g.setTextColor(Color3ub(255, 255, 255));
		g.drawText(Vec2i(10, 10), info);
	}

	void ABGameHUD::renderPlayerInfo(IGraphics2D& g, Player& player)
	{
		InlineString<64> info;
		info.format("Gold: %d  HP: %d  Lvl: %d", player.getGold(), player.getHp(), player.getLevel());

		RenderUtility::SetFont(g, FONT_S24);
		g.setTextColor(Color3ub(255, 255, 255));
		g.drawText(Vec2i(10, 40), info);
	}

	void ABGameHUD::renderViewInfo(IGraphics2D& g, ABViewCamera const& camera)
	{
		InlineString<32> viewInfo;
		viewInfo.format("Viewing Player %d", camera.getViewPlayerIndex());

		RenderUtility::SetFont(g, FONT_S24);
		g.setTextColor(Color3ub(255, 255, 255));
		g.drawText(Vec2i(10, 70), viewInfo);
	}

	void ABGameHUD::renderConnectionStatus(IGraphics2D& g)
	{
		if (mIsConnectionLost)
		{
			RenderUtility::SetFont(g, FONT_S24);
			g.setTextColor(Color3ub(255, 0, 0));
			g.drawText(Vec2i(300, 250), "Waiting for Reconnection...");
		}
	}

	void ABGameHUD::renderPlayerList(IGraphics2D& g)
	{
		using namespace HUDLayout;

		Vec2i screenSize = ::Global::GetScreenSize();

		// Sort players by HP
		std::vector<Player*> sortedPlayers;
		for (int i = 0; i < AB_MAX_PLAYER_NUM; ++i)
		{
			sortedPlayers.push_back(&mWorld->getPlayer(i));
		}

		std::sort(sortedPlayers.begin(), sortedPlayers.end(), [](Player* a, Player* b) {
			return a->getHp() > b->getHp();
		});

		int currentY = PlayerListStartY;

		for (int i = 0; i < sortedPlayers.size(); ++i)
		{
			Player* p = sortedPlayers[i];
			bool isLocal = (p->getIndex() == mWorld->getLocalPlayerIndex());
			int radius = PlayerAvatarBaseRadius + (isLocal ? PlayerAvatarLocalScale : 0);

			currentY += (isLocal ? 15 : 0); // Extra top padding for local

			Vec2i center(screenSize.x - PlayerListRightMargin, currentY);

			// 1. Avatar (Blue Circle Placeholder)
			RenderUtility::SetBrush(g, EColor::Blue);
			RenderUtility::SetPen(g, EColor::Black);
			g.drawCircle(center, radius - 4); // Inner circle

			// 2. HP Ring
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

			// 4. Name Tag
			InlineString<64> nameStr;
			nameStr.format("Player %d", p->getIndex()); // Placeholder Name

			RenderUtility::SetFont(g, FONT_S10);
			Vec2i nameBoxSize(80, 18);
			Vec2i nameBoxPos = hpBoxPos - Vec2i(nameBoxSize.x + 2, 0);

			g.setBrush(Color4ub(0, 0, 0, 150));
			g.drawRect(nameBoxPos, nameBoxSize);
			g.drawText(nameBoxPos + Vec2i(5, 2), nameStr);

			// Advance Y
			currentY += PlayerListGapY + (isLocal ? 15 : 0);
		}
	}

} // namespace AutoBattler
