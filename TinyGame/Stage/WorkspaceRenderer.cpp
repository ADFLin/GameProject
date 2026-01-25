#include "TinyGamePCH.h"
#include "WorkspaceRenderer.h"
#include "MainMenuStage.h"
#include "MenuLayoutHelper.h"
#include "CommonWidgets.h"
#include "TinyCore/GameGlobal.h"
#include "TinyCore/GameGUISystem.h"
#include "RenderUtility.h"
#include "EasingFunction.h"
#include <algorithm>

//--- Internal Helper Class ---
class GLauncherItem : public GWidget
{
	typedef GWidget BaseClass;
public:
	WorkspaceItem mData;
	GButton* mLaunchBtn;

	GLauncherItem(WorkspaceItem const& data, Vec2i const& pos, Vec2i const& size, GWidget* parent)
		:BaseClass(pos, size, parent)
		,mData(data)
	{
		mID = data.uiID;
		// Create the "Launch" button at the bottom right
		int btnW = 70;
		int btnH = 30;
		mLaunchBtn = new GButton(mID, Vec2i(size.x - btnW - 10, size.y - btnH - 12), Vec2i(btnW, btnH), this);
		mLaunchBtn->setTitle("Launch");
	}

	void onRender() override
	{
		IGraphics2D& g = Global::GetIGraphics2D();
		Vec2i p = getWorldPos();
		Vec2i s = getSize();

		// Draw Card Background
		g.setPen(Color3ub(45, 140, 180)); // Cyan border
		g.setBrush(Color3ub(35, 38, 42)); // Dark Slate
		g.drawRoundRect(p, s, Vec2i(12, 12));

		// Text Rendering
		RenderUtility::SetFont(g, FONT_S12);
		g.setTextColor(Color3ub(255, 255, 255));
		g.drawText(p + Vec2i(15, 12), mData.title.c_str());

		RenderUtility::SetFont(g, FONT_S8);
		g.setTextColor(Color3ub(120, 180, 220)); // Subtle Cyan
		g.drawText(p + Vec2i(15, 36), mData.detail.c_str()); 

		// Status
		g.setTextColor(mData.statusColor);
		RenderUtility::SetFont(g, FONT_S8);
		g.drawText(p + Vec2i(15, s.y - 23), mData.statusText.c_str()); 
	}
};

WorkspaceRenderer::WorkspaceRenderer()
{
}

void WorkspaceRenderer::init()
{
	if (mScrollBar) return;
	// Use MainMenuStage ID to ensure consistency with event system
	mScrollBar = new GScrollBar(MainMenuStage::UI_SCROLLBAR, Vec2i(Global::GetScreenSize().x - 25, 80), Global::GetScreenSize().y - 160, false, nullptr);
	mScrollBar->show(false);
	mScrollBar->onEvent = [this](int event, GWidget* ui)
	{
		if (event == EVT_SCROLL_CHANGE)
		{
			int newVal = mScrollBar->getValue();
			int diff = newVal - mScrollOffset;
			if (diff != 0)
			{
				mScrollOffset = newVal;
				updateWidgetPositions(diff);
			}
		}
		return false;
	};
	::Global::GUI().addWidget(mScrollBar);
}

WorkspaceRenderer::~WorkspaceRenderer()
{
	cleanupWidgets();
	if (mScrollBar) mScrollBar->destroy();
}

void WorkspaceRenderer::cleanupWidgets()
{
	::Global::GUI().getTweener().clear();
	for (auto ui : mWidgets)
	{
		ui->destroy();
	}
	mWidgets.clear();
}

void WorkspaceRenderer::resetScroll()
{
	mScrollOffset = 0;
	if (mScrollBar) mScrollBar->setValue(0);
}

void WorkspaceRenderer::updateWidgetPositions(int diff)
{
	for (auto ui : mWidgets)
	{
		Vec2i pos = ui->getPos();
		pos.y -= diff;
		ui->setPos(pos);
	}
}

void WorkspaceRenderer::handleMouseWheel(int change)
{
	if (mScrollBar && mScrollBar->isShow())
	{
		int oldVal = mScrollBar->getValue();
		mScrollBar->setValue(oldVal - change);
		
		int newVal = mScrollBar->getValue();
		int diff = newVal - mScrollOffset;
		if (diff != 0)
		{
			mScrollOffset = newVal;
			updateWidgetPositions(diff);
		}
	}
}

void WorkspaceRenderer::refresh(std::vector<WorkspaceItem> const& items)
{
	resetScroll(); // Always reset scroll when loading new items
	cleanupWidgets();

	int baseStartY = MenuLayout::HeaderHeight + 20;
	int startX = MenuLayout::SidebarWidth + 20;
	int startY = baseStartY - mScrollOffset;

	int cardWidth = 220;
	int cardHeight = 110;
	int spacing = 20;

	// Calculate columns based on fixed width area
	int availableWidth = Global::GetScreenSize().x - startX - 40;
	int columns = availableWidth / (cardWidth + spacing);
	if (columns < 1) columns = 1;

	int idx = 0;
	int delay = 0;

	for (auto const& itemData : items)
	{
		int x = startX + (idx % columns) * (cardWidth + spacing);
		int ry = (idx / columns) * (cardHeight + spacing);
		int y = startY + ry;

		GLauncherItem* widget = new GLauncherItem(itemData, Vec2i(::Global::GetScreenSize().x + 50, y), Vec2i(cardWidth, cardHeight), nullptr);
		::Global::GUI().addWidget(widget);
		mWidgets.push_back(widget);

		::Global::GUI().addMotion<Easing::OQuad>(widget, Vec2i(::Global::GetScreenSize().x + 50, y), Vec2i(x, y), 350, delay);
		delay += 15;
		idx++;
	}

	// Scrollbar logic - use fixed viewport height
	if (idx > 0)
	{
		int totalRows = (idx + columns - 1) / columns;
		int totalHeight = totalRows * cardHeight + (totalRows - 1) * spacing;
		int viewportHeight = Global::GetScreenSize().y - baseStartY - 20; 

		if (totalHeight > viewportHeight)
		{
			if (mScrollBar)
			{
				mScrollBar->setRange(totalHeight - viewportHeight + 20, viewportHeight);
				mScrollBar->show(true);
			}
		}
		else
		{
			if (mScrollBar) mScrollBar->show(false);
		}
	}
	else
	{
		if (mScrollBar) mScrollBar->show(false);
	}
}
