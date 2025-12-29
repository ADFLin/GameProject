#pragma once
#ifndef ServerStatusPanel_H_INCLUDED
#define ServerStatusPanel_H_INCLUDED

#include "GameWidget.h"
#include "InlineString.h"

class SVPlayerManager;

/**
 * ServerStatusPanel - UI Panel for Dedicated Server Status Display
 * 
 * Shows:
 *   - Server state (Waiting, Countdown, Running, etc.)
 *   - Connected player list
 *   - Countdown timer (when applicable)
 *   - Game name and config
 */
class ServerStatusPanel : public GPanel
{
	typedef GPanel BaseClass;
public:
	ServerStatusPanel(Vec2i const& pos, Vec2i const& size, GWidget* parent);
	~ServerStatusPanel();

	// Update display content
	void setServerState(char const* stateName);
	void setGameName(char const* gameName);
	void setCountdown(float seconds);
	void updatePlayerList(SVPlayerManager* playerMgr);
	
	// GWidget override
	void onRender() override;

private:
	void refreshText();
	
	InlineString<64> mStateName;
	InlineString<64> mGameName;
	InlineString<512> mPlayerListText;
	float mCountdown = 0.0f;
	int mPlayerCount = 0;
};

#endif // ServerStatusPanel_H_INCLUDED
