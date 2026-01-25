#ifndef MenuLayoutHelper_h__
#define MenuLayoutHelper_h__

#include "Math/Vector2.h"
#include "Core/Color.h"

namespace MenuLayout
{
	static const int SidebarWidth = 200;
	static const int ContentPadding = 10;
	static const int ButtonHeight = 28;
	static const int HeaderHeight = 60;
	static const int ModeButtonY = 70;
	static const int HistoryButtonY = 105;
	static const int TopContentLimit = 110;
	static const int BottomButtonAreaHeight = 60;
	
	static const Color3ub SidebarBgColor(20, 23, 26);
	static const Color3ub SidebarBorderColor(45, 140, 180);
	static const Color3ub HeaderBgColor(35, 38, 42);
	static const Color3ub ButtonNormalColor(30, 33, 36);
	static const Color3ub ButtonHighlightColor(65, 70, 75);
	static const Color3ub ButtonSelectedColor(55, 120, 180);
	static const Color3ub TextNormalColor(180, 180, 180);
	static const Color3ub TextSelectedColor(255, 255, 255);

	inline int GetScreenHeight() { return Global::GetScreenSize().y; }
	inline int GetBottomContentLimit() { return GetScreenHeight() - BottomButtonAreaHeight; }
	inline int GetViewportHeight() { return GetBottomContentLimit() - TopContentLimit; }
}

#endif // MenuLayoutHelper_h__
