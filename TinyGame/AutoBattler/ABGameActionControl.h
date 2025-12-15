#pragma once
#ifndef ABGameActionControl_H_98C1C66F_FB0A_4BD7_A574_0AE71B09B6B4
#define ABGameActionControl_H_98C1C66F_FB0A_4BD7_A574_0AE71B09B6B4

namespace AutoBattler
{
	class Player;

	class IGameActionControl
	{
	public:
		virtual void buyUnit(Player& player, int slotIndex) = 0;
		virtual void sellUnit(Player& player, int slotIndex) = 0;
		virtual void refreshShop(Player& player) = 0;
		virtual void buyExperience(Player& player) = 0;
		virtual void deployUnit(Player& player, int srcType, int srcX, int srcY, int destX, int destY) = 0;
		virtual void retractUnit(Player& player, int srcType, int srcX, int srcY, int benchIndex) = 0;
		virtual void syncDrag(Player& player, int srcType, int srcIndex, int posX, int posY, bool bDrop) = 0;
	};
}

#endif // ABGameActionControl_H_98C1C66F_FB0A_4BD7_A574_0AE71B09B6B4
