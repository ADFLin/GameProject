#include "ABPCH.h"
#include "ABShopUI.h"
#include "ABWorld.h"
#include "ABGameActionControl.h"
#include "RenderUtility.h"
#include "GameGlobal.h"

namespace AutoBattler
{
	//////////////////////////////////////////////////////////////////////////
	// ABShopCardButton
	//////////////////////////////////////////////////////////////////////////

	ABShopCardButton::ABShopCardButton(int id, Vec2i const& pos, Vec2i const& size, GWidget* parent)
		: BaseClass(id, pos, size, parent)
	{
	}

	void ABShopCardButton::onRender()
	{
		using namespace ShopLayout;

		IGraphics2D& g = Global::GetIGraphics2D();
		Vec2i pos = getWorldPos();
		Vec2i size = getSize();

		if (mUnitId == 0)
		{
			// Empty Slot
			RenderUtility::SetBrush(g, EColor::Black);
			RenderUtility::SetPen(g, EColor::Gray);
			g.drawRect(pos, size);
			return;
		}

		if (!mWorld)
			return;

		UnitDefinition const* def = mWorld->getUnitDataManager().getUnit(mUnitId);

		// Rarity Color based on cost
		Color3ub rarityColor(128, 128, 128); // Cost 1
		if (def)
		{
			switch (def->cost)
			{
			case 2: rarityColor = Color3ub(20, 200, 20);   break; // Green
			case 3: rarityColor = Color3ub(20, 50, 200);   break; // Blue
			case 4: rarityColor = Color3ub(180, 20, 200);  break; // Purple
			case 5: rarityColor = Color3ub(255, 200, 0);   break; // Gold
			}
		}

		// Hover effect
		ButtonState buttonState = getButtonState();
		if (buttonState == BS_HIGHLIGHT)
		{
			// Brighten the color when hovered
			g.setBrush(Color4ub(30, 30, 30, 255));
		}
		else
		{
			g.setBrush(Color4ub(10, 10, 10, 255));
		}

		g.setPen(rarityColor);
		g.drawRect(pos, size);

		if (def)
		{
			g.setTextColor(Color3ub(255, 255, 255));

			// Name (Bottom Left)
			RenderUtility::SetFont(g, FONT_S12);
			g.drawText(pos + Vec2i(5, size.y - 20), def->name);

			// Cost (Bottom Right)
			InlineString<16> cStr;
			cStr.format("%d", def->cost);
			g.setTextColor(rarityColor);
			g.drawText(pos + Vec2i(size.x - 15, size.y - 20), cStr);

			// Traits (Top Left)
			g.setTextColor(Color3ub(180, 180, 180));
			RenderUtility::SetFont(g, FONT_S10);
			g.drawText(pos + Vec2i(5, 5), "Trait 1");
			g.drawText(pos + Vec2i(5, 18), "Trait 2");
		}
	}

	void ABShopCardButton::onMouse(bool beInside)
	{
		BaseClass::onMouse(beInside);
	}

	//////////////////////////////////////////////////////////////////////////
	// ABShopControlButton
	//////////////////////////////////////////////////////////////////////////

	ABShopControlButton::ABShopControlButton(int id, Vec2i const& pos, Vec2i const& size, GWidget* parent)
		: BaseClass(id, pos, size, parent)
	{
	}

	void ABShopControlButton::onRender()
	{
		IGraphics2D& g = Global::GetIGraphics2D();
		Vec2i pos = getWorldPos();
		Vec2i size = getSize();

		// Button background
		ButtonState buttonState = getButtonState();
		if (buttonState == BS_HIGHLIGHT)
		{
			g.setBrush(Color4ub(50, 50, 60, 255));
		}
		else if (buttonState == BS_PRESS)
		{
			g.setBrush(Color4ub(20, 20, 30, 255));
		}
		else
		{
			g.setBrush(Color4ub(30, 30, 40, 255));
		}

		RenderUtility::SetPen(g, EColor::Gray);
		g.drawRect(pos, size);

		// Title
		g.setTextColor(Color3ub(255, 200, 0));
		RenderUtility::SetFont(g, FONT_S12);
		g.drawText(pos + Vec2i(10, 5), mTitle.c_str());

		// Cost
		InlineString<16> costStr;
		costStr.format("%dg", mCost);
		g.drawText(pos + Vec2i(10, 25), costStr);

		// XP Bar for Buy XP button
		if (getID() == UI_AB_BUY_XP && mPlayer)
		{
			float xpRatio = (float)mPlayer->getXp() / 100.0f;
			if (xpRatio > 1.0f) xpRatio = 1.0f;

			Vec2i barPos = pos + Vec2i(5, size.y - 12);
			Vec2i barSize(size.x - 10, 6);

			RenderUtility::SetBrush(g, EColor::Black);
			g.drawRect(barPos, barSize);
			RenderUtility::SetBrush(g, EColor::Cyan);
			g.drawRect(barPos, Vec2i((int)(barSize.x * xpRatio), barSize.y));

			// Level text
			g.setTextColor(Color3ub(255, 255, 255));
			InlineString<32> lvlStr;
			lvlStr.format("Lvl %d", mPlayer->getLevel());
			g.drawText(pos + Vec2i(size.x - 50, 5), lvlStr);
		}
	}

	void ABShopControlButton::onMouse(bool beInside)
	{
		BaseClass::onMouse(beInside);
	}

	//////////////////////////////////////////////////////////////////////////
	// ABShopPanel
	//////////////////////////////////////////////////////////////////////////

	ABShopPanel::ABShopPanel(int id, Vec2i const& pos, Vec2i const& size, GWidget* parent)
		: BaseClass(id, pos, size, parent)
	{
		setRenderType(eRectType);
		setAlpha(0.9f);
	}

	void ABShopPanel::initButtons()
	{
		using namespace ShopLayout;

		Vec2i panelSize = getSize();

		int totalWidth = ControlWidth + CardGap + (AB_SHOP_SLOT_COUNT * (CardWidth + CardGap));
		int startX = (panelSize.x - totalWidth) / 2;
		int startY = (panelSize.y - CardHeight) / 2;

		// Refresh Button (Top Half of control area)
		Vec2i refreshPos(startX, startY);
		Vec2i refreshSize(ControlWidth, CardHeight / 2 - 2);
		mRefreshButton = new ABShopControlButton(UI_AB_REFRESH_SHOP, refreshPos, refreshSize, this);
		mRefreshButton->setTitle("Refresh");
		mRefreshButton->setCost(AB_SHOP_REFRESH_COST);

		// Buy XP Button (Bottom Half of control area)
		Vec2i xpPos(startX, startY + CardHeight / 2 + 2);
		Vec2i xpSize(ControlWidth, CardHeight / 2 - 2);
		mBuyXpButton = new ABShopControlButton(UI_AB_BUY_XP, xpPos, xpSize, this);
		mBuyXpButton->setTitle("Buy XP");
		mBuyXpButton->setCost(AB_BUY_XP_COST);

		// Card Buttons
		int cardsStartX = startX + ControlWidth + CardGap;
		for (int i = 0; i < AB_SHOP_SLOT_COUNT; ++i)
		{
			Vec2i cardPos(cardsStartX + i * (CardWidth + CardGap), startY);
			Vec2i cardSize(CardWidth, CardHeight);

			int buttonId = UI_AB_SHOP_CARD_0 + i;
			mCardButtons[i] = new ABShopCardButton(buttonId, cardPos, cardSize, this);
			mCardButtons[i]->setSlotIndex(i);
		}
	}

	void ABShopPanel::updateFromWorld(World& world)
	{
		mWorld = &world;
		Player& player = world.getLocalPlayer();

		// Update control buttons
		if (mRefreshButton)
			mRefreshButton->setPlayer(&player);
		if (mBuyXpButton)
			mBuyXpButton->setPlayer(&player);

		// Update card buttons
		for (int i = 0; i < AB_SHOP_SLOT_COUNT; ++i)
		{
			if (mCardButtons[i])
			{
				mCardButtons[i]->setWorld(&world);
				if (i < player.mShopList.size())
				{
					mCardButtons[i]->setUnitId(player.mShopList[i]);
				}
				else
				{
					mCardButtons[i]->setUnitId(0);
				}
			}
		}
	}

	bool ABShopPanel::onChildEvent(int event, int id, GWidget* widget)
	{
		if (event == EVT_BUTTON_CLICK && mActionControl && mWorld)
		{
			Player& player = mWorld->getLocalPlayer();

			switch (id)
			{
			case UI_AB_REFRESH_SHOP:
				mActionControl->refreshShop(player);
				return false;

			case UI_AB_BUY_XP:
				mActionControl->buyExperience(player);
				return false;

			default:
				// Check if it's a shop card button
				if (id >= UI_AB_SHOP_CARD_0 && id <= UI_AB_SHOP_CARD_4)
				{
					int slotIndex = id - UI_AB_SHOP_CARD_0;
					mActionControl->buyUnit(player, slotIndex);
					return false;
				}
				break;
			}
		}

		return BaseClass::onChildEvent(event, id, widget);
	}

	void ABShopPanel::onRender()
	{
		IGraphics2D& g = Global::GetIGraphics2D();
		Vec2i pos = getWorldPos();
		Vec2i size = getSize();

		// Draw background
		g.setBrush(Color4ub(10, 10, 20, 230));
		RenderUtility::SetPen(g, EColor::Gray);
		g.drawRect(pos, size);

		// Draw gold indicator (above the panel)
		if (mWorld)
		{
			using namespace ShopLayout;

			Player& player = mWorld->getLocalPlayer();

			Vec2i goldBoxPos(pos.x + size.x / 2 - GoldBoxWidth / 2, pos.y - GoldBoxHeight - 10);
			Vec2i goldBoxSize(GoldBoxWidth, GoldBoxHeight);

			g.setBrush(Color4ub(10, 10, 20, 255));
			RenderUtility::SetPen(g, EColor::Yellow);
			g.drawRect(goldBoxPos, goldBoxSize);

			InlineString<16> goldStr;
			goldStr.format("%d g", player.getGold());

			RenderUtility::SetFont(g, FONT_S16);
			g.setTextColor(Color3ub(255, 215, 0));
			g.drawText(goldBoxPos + Vec2i(20, 5), goldStr);
		}
	}

} // namespace AutoBattler
