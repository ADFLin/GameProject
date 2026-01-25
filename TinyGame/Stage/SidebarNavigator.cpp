#include "SidebarNavigator.h"
#include "MainMenuStage.h"
#include "StageRegister.h"
#include "MenuLayoutHelper.h"
#include "GameWidgetID.h"
#include "Localization.h"
#include "GameGlobal.h"
#include "GameGUISystem.h"
#include "EasingFunction.h"

class GSidebarButton : public GButtonBase
{
	typedef GButtonBase BaseClass;
public:
	GSidebarButton(int id, Vec2i const& pos, Vec2i const& size, GWidget* parent)
		:BaseClass(id, pos, size, parent)
	{
	}

	void setTitle(char const* str) { mTitle = str; }

	void onRender() override
	{
		IGraphics2D& g = Global::GetIGraphics2D();

		Vec2i pos = getWorldPos();
		Vec2i size = getSize();

		if (getButtonState() == BS_HIGHLIGHT || getButtonState() == BS_PRESS)
		{
			g.setBrush(MenuLayout::ButtonHighlightColor);
		}
		else if (bSelected)
		{
			g.setBrush(MenuLayout::ButtonSelectedColor);
		}
		else
		{
			g.setBrush(MenuLayout::ButtonNormalColor);
		}
		RenderUtility::SetPen(g, EColor::Null);
		g.drawRoundRect(pos, size, Vec2i(4, 4));

		if (bGroupTitle)
		{
			Vec2i arrowPos = pos + Vec2i(10, size.y / 2 - 4);
			g.setPen(Color3ub(120, 120, 120));
			if (bExpanded)
				RenderUtility::DrawTriangle(g, arrowPos, Vec2i(8, 8), RenderUtility::eDown);
			else
				RenderUtility::DrawTriangle(g, arrowPos, Vec2i(8, 8), RenderUtility::eRight);
		}

		RenderUtility::SetFont(g, bGroupTitle ? FONT_S10 : FONT_S8);
		g.setTextColor(bSelected ? Color3ub(255, 255, 255) : Color3ub(180, 180, 180));

		int textX = 30;
		g.drawText(pos + Vec2i(textX, (size.y - 12) / 2), mTitle.c_str());
	}

	String mTitle;
	bool bSelected = false;
	bool bGroupTitle = false;
	bool bExpanded = false;
};


SidebarNavigator::SidebarNavigator()
{

}

SidebarNavigator::~SidebarNavigator()
{
}

void SidebarNavigator::handleMouseWheel(int delta)
{
	mScrollOffset += delta;
	if (mScrollOffset > 0) mScrollOffset = 0;
	if (mScrollOffset < -mMaxScroll) mScrollOffset = -mMaxScroll;
	
	// Refresh layout essentially
	createSidebar(nullptr, false, false); // Parent is passed as nullptr? No, widgets need parent.
	// Actually widgets are usually added to Global::GUI() root or a container.
	// In the original code, they were added to ::Global::GUI().addWidget
}

void SidebarNavigator::resetScroll()
{
	mScrollOffset = 0;
}


void SidebarNavigator::createSidebar(GWidget* parent, bool bClearTweens, bool bAnimateChange)
{
	if (bClearTweens)
	{
		::Global::GUI().getTweener().clear();
	}
	for (auto ui : mWidgets)
	{
		ui->destroy();
	}
	mWidgets.clear();

	int sidebarWidth = MenuLayout::SidebarWidth - 20; 
	int btnHeight = MenuLayout::ButtonHeight;
	int screenHeight = MenuLayout::GetScreenHeight();

	// Calculate content limits
	int topLimit = MenuLayout::TopContentLimit;
	int bottomLimit = MenuLayout::GetBottomContentLimit();
	int viewportHeight = MenuLayout::GetViewportHeight();

	// Use generic content map
	
	// Create currentY variable
	int currentY = topLimit + mScrollOffset;

	if (mContentMap.find(mMode) != mContentMap.end())
	{
		// Calculate total content height for scroll (Simplification: just estimate size)
		// Or better, let createContentItems return height
		createContentItems(currentY, btnHeight, bAnimateChange);
	}
	else if (mMode == (int)MainMenuStage::ViewMode::Category) // Fallback if still using specific mode check logic temporarily?
	{
		// Should be in map now
	}

	createFixedItems(currentY, btnHeight, screenHeight);
}


// Removed initGroupItems

// Renamed from createGroupItems
void SidebarNavigator::createContentItems(int& currentY, int btnHeight, bool bAnimateChange)
{
	int sidebarX = MenuLayout::ContentPadding;
	int sidebarWidth = MenuLayout::SidebarWidth - 20;
	int topLimit = MenuLayout::TopContentLimit;
	int bottomLimit = MenuLayout::GetBottomContentLimit();

	int expandedPushHeight = 0;

	// Recursive lambda for creating buttons
	std::function<void(TArray<SidebarItem> const&, int)> createButtons;
	createButtons = [&](TArray<SidebarItem> const& items, int level)
	{
		for (auto const& item : items)
		{
			bool bExpanded = false;
			bool bSelected = false;

			// Logic for selection and expansion delegated to Policy
			if (mSelectionPolicy)
			{
				bSelected = mSelectionPolicy(item, level);
			}
			
			bExpanded = bSelected && !item.children.empty();

			// Add Button
			if (currentY >= topLimit - btnHeight && currentY <= bottomLimit)
			{
				int x = sidebarX + (level - 1) * 15;
				int w = sidebarWidth - (level - 1) * 15;
				
				// ID generation logic: If item.id is set, use it. Otherwise need MainMenuStage specific logic...
				//Ideally item.id should be set. MainMenuStage sets it now.
				CHECK(item.id != UI_ANY);

				GSidebarButton* btn = new GSidebarButton(item.id, Vec2i(x, currentY), Vec2i(w, (level > 1 ? btnHeight - 2 : btnHeight)), nullptr);
				btn->setTitle(item.name.c_str());
				btn->bSelected = bSelected; 
				btn->bGroupTitle = !item.children.empty();
				btn->bExpanded = bExpanded;

				::Global::GUI().addWidget(btn);
				mWidgets.push_back(btn);

				// Animation
				if (level > 1 && bAnimateChange) 
				{
					Vec2i startPos = Vec2i(x - 20, currentY);
					Vec2i endPos = Vec2i(x, currentY);
					btn->setPos(startPos);
					::Global::GUI().addMotion<Easing::OQuad>(btn, startPos, endPos, 200, (currentY - topLimit) / 2);
				}
				else if (bAnimateChange && expandedPushHeight > 0 && level == 1)
				{
					// Push down animation
					Vec2i startPos = Vec2i(x, currentY - expandedPushHeight);
					Vec2i endPos = Vec2i(x, currentY);
					::Global::GUI().addMotion<Easing::OQuad>(btn, startPos, endPos, 200, 0);
				}
			}
			
			int yBefore = currentY;
			currentY += (level > 1 ? btnHeight - 2 : btnHeight) + 4;

			if (bExpanded)
			{
				int startY = currentY;
				createButtons(item.children, level + 1);
				expandedPushHeight = currentY - startY;
			}
		}
	};

	if (mContentMap.find(mMode) != mContentMap.end())
	{
		createButtons(mContentMap[mMode], 1);
	}
}

void SidebarNavigator::createFixedItems(int& currentY, int btnHeight, int screenHeight)
{
	int sidebarX = MenuLayout::ContentPadding;
	int sidebarWidth = MenuLayout::SidebarWidth - 20;
	
	GButton* modeBtn = new GButton(MainMenuStage::UI_MAIN_GROUP, Vec2i(sidebarX, MenuLayout::ModeButtonY), Vec2i(sidebarWidth, 32), nullptr);
	if (mMode == (int)MainMenuStage::ViewMode::Category)
	{
		modeBtn->setTitle("MODE: CATEGORY");
		modeBtn->setColor(Color3ub(60, 140, 220));
	}
	else
	{
		modeBtn->setTitle("MODE: GROUP");
		modeBtn->setColor(Color3ub(60, 140, 220));
	}
	::Global::GUI().addWidget(modeBtn);
	mWidgets.push_back(modeBtn);

	int bottomBtnW = (MenuLayout::SidebarWidth - 30) / 2;
	GButton* optionBtn = new GButton(MainMenuStage::UI_GAME_OPTION, Vec2i(sidebarX, screenHeight - 50), Vec2i(bottomBtnW, 35), nullptr);
	optionBtn->setTitle("Option");
	::Global::GUI().addWidget(optionBtn);
	mWidgets.push_back(optionBtn);

	GButton* exitBtn = new GButton(UI_EXIT_GAME, Vec2i(sidebarX + bottomBtnW + 10, screenHeight - 50), Vec2i(bottomBtnW, 35), nullptr);
	exitBtn->setTitle("Exit");
	::Global::GUI().addWidget(exitBtn);
	mWidgets.push_back(exitBtn);
}
