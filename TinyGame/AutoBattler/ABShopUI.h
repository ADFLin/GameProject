#ifndef ABShopUI_h__
#define ABShopUI_h__

#include "ABDefine.h"
#include "GameWidget.h"

namespace AutoBattler
{
	class World;
	class Player;
	class IGameActionControl;

	// Widget IDs for Shop UI
	enum EShopWidgetId
	{
		UI_AB_SHOP_BASE = UI_GAME_ID,
		UI_AB_REFRESH_SHOP,
		UI_AB_BUY_XP,
		UI_AB_SHOP_CARD_0,
		UI_AB_SHOP_CARD_1,
		UI_AB_SHOP_CARD_2,
		UI_AB_SHOP_CARD_3,
		UI_AB_SHOP_CARD_4,
	};

	// UI Layout Constants for Shop
	namespace ShopLayout
	{
		constexpr int PanelHeight = 140;
		constexpr int CardWidth = 140;
		constexpr int CardHeight = 120;
		constexpr int CardGap = 8;
		constexpr int ControlWidth = 120;
		constexpr int GoldBoxWidth = 80;
		constexpr int GoldBoxHeight = 30;
	}

	// Custom button for shop cards
	class ABShopCardButton : public GButtonBase
	{
		using BaseClass = GButtonBase;
	public:
		ABShopCardButton(int id, Vec2i const& pos, Vec2i const& size, GWidget* parent);

		void setUnitId(int unitId) { mUnitId = unitId; }
		int getUnitId() const { return mUnitId; }
		void setSlotIndex(int index) { mSlotIndex = index; }
		int getSlotIndex() const { return mSlotIndex; }

		void setWorld(World* world) { mWorld = world; }

		void onRender() override;
		void onMouse(bool beInside) override;

	private:
		int mUnitId = 0;
		int mSlotIndex = 0;
		World* mWorld = nullptr;
	};

	// Custom button for Refresh/Buy XP
	class ABShopControlButton : public GButtonBase
	{
		using BaseClass = GButtonBase;
	public:
		ABShopControlButton(int id, Vec2i const& pos, Vec2i const& size, GWidget* parent);

		void setTitle(char const* title) { mTitle = title; }
		void setCost(int cost) { mCost = cost; }
		void setPlayer(Player* player) { mPlayer = player; }

		void onRender() override;
		void onMouse(bool beInside) override;

	private:
		String mTitle;
		int mCost = 0;
		Player* mPlayer = nullptr;
	};

	// Shop Panel that contains all shop UI elements
	class ABShopPanel : public GPanel
	{
		using BaseClass = GPanel;
	public:
		ABShopPanel(int id, Vec2i const& pos, Vec2i const& size, GWidget* parent);

		void initButtons();
		void updateFromWorld(World& world);

		void setWorld(World* world) { mWorld = world; }
		void setActionControl(IGameActionControl* control) { mActionControl = control; }

		bool onChildEvent(int event, int id, GWidget* widget) override;
		void onRender() override;

	private:
		World* mWorld = nullptr;
		IGameActionControl* mActionControl = nullptr;

		ABShopControlButton* mRefreshButton = nullptr;
		ABShopControlButton* mBuyXpButton = nullptr;
		ABShopCardButton* mCardButtons[AB_SHOP_SLOT_COUNT] = {};
	};

} // namespace AutoBattler

#endif // ABShopUI_h__
