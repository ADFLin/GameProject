#ifndef WorkspaceRenderer_h__
#define WorkspaceRenderer_h__

#include "GameWidget.h"

#include "DataStructure/Array.h"
#include <string>

class GScrollBar;

struct WorkspaceItem
{
	std::string title;
	std::string detail;
	std::string statusText;
	Color3ub statusColor;
	int uiID;
};

class WorkspaceRenderer
{
public:
	WorkspaceRenderer(); // No owner needed
	~WorkspaceRenderer();

	void init();
	void refresh(std::vector<WorkspaceItem> const& items);
	void handleMouseWheel(int change);
	void resetScroll();

private:
	void updateWidgetPositions(int diff);
	void cleanupWidgets();
	
	TArray<GWidget*> mWidgets;
	GScrollBar* mScrollBar = nullptr;
	int mScrollOffset = 0;
};

#endif // WorkspaceRenderer_h__
