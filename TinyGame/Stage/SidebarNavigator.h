#ifndef SidebarNavigator_h__
#define SidebarNavigator_h__

#include "CommonWidgets.h"
#include "MenuLayoutHelper.h"
#include "StageRegister.h"

#include <unordered_map>


class SidebarNavigator
{
public:
	SidebarNavigator();
	~SidebarNavigator();

	void createSidebar(GWidget* parent, bool bClearTweens, bool bAnimateChange);
	void handleMouseWheel(int delta);
	void resetScroll();

	// State Accessors
	void setMode(int mode) { mMode = mode; }
	int getMode() const { return mMode; }
	int getScrollOffset() const { return mScrollOffset; }
	
	struct SidebarItem
	{
		std::string name;
		int id = UI_ANY;      // UI Widget ID
		int userData = 0;     // Generic data (e.g. EExecGroup)
		TArray<SidebarItem> children;
	};

	using SelectionPolicy = std::function<bool(SidebarItem const&, int level)>;

	void addItem(int layer, SidebarItem const& item) { mContentMap[layer].push_back(item); }
	void clearItems(int layer) { mContentMap[layer].clear(); }
	
	void setSelectionPolicy(SelectionPolicy policy) { mSelectionPolicy = policy; }

	// Events (Not needed? handled by Owner usually, but keeping maybe for local widget management if needed)
	// bool onWidgetEvent(int event, int id, GWidget* ui);

private:
	void createContentItems(int& currentY, int btnHeight, bool bAnimateChange);
	void createFixedItems(int& currentY, int btnHeight, int screenHeight);

	TArray<GWidget*> mWidgets;
	
	std::unordered_map<int, TArray<SidebarItem>> mContentMap;
	SelectionPolicy mSelectionPolicy;

	int mMode = 0;
	
	int mScrollOffset = 0;
	int mMaxScroll = 0;

};

#endif // SidebarNavigator_h__
