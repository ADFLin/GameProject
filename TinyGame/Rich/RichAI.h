#pragma once
#ifndef RichAI_H_E39B471C_0B60_464A_B529_B167292CA5E9
#define RichAI_H_E39B471C_0B60_464A_B529_B167292CA5E9

#include "RichPlayerTurn.h"
#include "RichPlayer.h"
namespace Rich
{
	class AIController : public IPlayerController
	{
	public:
		void startTurn(PlayerTurn& turn) override;
		void endTurn(PlayerTurn& turn) override
		{

		}
		void queryAction(ActionRequestID id, PlayerTurn& turn, ActionReqestData const& data) override;

		void process(Player& player)
		{
			player.setController(*this);
			mPlayer = &player;
		}


		Player* mPlayer;
	};
}



#endif // RichAI_H_E39B471C_0B60_464A_B529_B167292CA5E9
