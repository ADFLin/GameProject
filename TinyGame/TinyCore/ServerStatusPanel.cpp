#include "TinyGamePCH.h"
#include "ServerStatusPanel.h"

#include "GameServer.h"
#include "RenderUtility.h"
#include "GameGlobal.h"

ServerStatusPanel::ServerStatusPanel(Vec2i const& pos, Vec2i const& size, GWidget* parent)
	: BaseClass(UI_ANY, pos, size, parent)
{
	mStateName = "Initializing";
	mGameName = "Unknown";
	mPlayerListText = "";
}

ServerStatusPanel::~ServerStatusPanel()
{
}

void ServerStatusPanel::setServerState(char const* stateName)
{
	mStateName = stateName;
}

void ServerStatusPanel::setGameName(char const* gameName)
{
	mGameName = gameName;
}

void ServerStatusPanel::setCountdown(float seconds)
{
	mCountdown = seconds;
}

void ServerStatusPanel::updatePlayerList(SVPlayerManager* playerMgr)
{
	mPlayerListText.clear();
	mPlayerCount = 0;
	
	if (playerMgr)
	{
		for (auto iter = playerMgr->createIterator(); iter; ++iter)
		{
			GamePlayer* player = iter.getElement();
			if (player)
			{
				if (mPlayerCount > 0)
					mPlayerListText += "\n";
				mPlayerListText += "  ";
				mPlayerListText += player->getName();
				++mPlayerCount;
			}
		}
	}
	
	if (mPlayerCount == 0)
	{
		mPlayerListText = "  (No players connected)";
	}
}

void ServerStatusPanel::onRender()
{
	BaseClass::onRender();
	
	IGraphics2D& g = Global::GetIGraphics2D();
	Vec2i pos = getWorldPos();
	Vec2i size = getSize();
	
	// Title
	RenderUtility::SetFont(g, FONT_S12);
	g.setTextColor(Color3ub(255, 255, 255));
	
	int yOffset = 10;
	int lineHeight = 18;
	
	// Server Title
	InlineString<128> titleText;
	titleText.format("=== Dedicated Server: %s ===", mGameName.c_str());
	g.drawText(pos + Vec2i(10, yOffset), titleText);
	yOffset += lineHeight + 5;
	
	// State
	g.setTextColor(Color3ub(200, 200, 100));
	InlineString<64> stateText;
	stateText.format("State: %s", mStateName.c_str());
	g.drawText(pos + Vec2i(10, yOffset), stateText);
	yOffset += lineHeight;
	
	// Countdown (if active)
	if (mCountdown > 0.0f)
	{
		g.setTextColor(Color3ub(255, 200, 100));
		InlineString<32> countdownText;
		countdownText.format("Starting in: %.1f", mCountdown);
		g.drawText(pos + Vec2i(10, yOffset), countdownText);
		yOffset += lineHeight;
	}
	
	yOffset += 5;
	
	// Player count
	g.setTextColor(Color3ub(100, 200, 255));
	InlineString<32> playerCountText;
	playerCountText.format("Players (%d):", mPlayerCount);
	g.drawText(pos + Vec2i(10, yOffset), playerCountText);
	yOffset += lineHeight;
	
	// Player list
	g.setTextColor(Color3ub(180, 180, 180));
	
	// Draw each line of player list
	char const* start = mPlayerListText.c_str();
	char const* cur = start;
	while (*cur)
	{
		if (*cur == '\n')
		{
			InlineString<64> line(start, cur - start);
			g.drawText(pos + Vec2i(10, yOffset), line);
			yOffset += lineHeight;
			start = cur + 1;
		}
		++cur;
	}
	// Draw last line
	if (start != cur)
	{
		g.drawText(pos + Vec2i(10, yOffset), start);
	}
}
