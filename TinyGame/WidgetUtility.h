#ifndef WidgetUtility_h__
#define WidgetUtility_h__

#include "GameWidget.h"
#include "GameWidgetID.h"

class DevFrame : public GFrame
{
public:
	DevFrame( int id ,  Vec2i const& pos , Vec2i const& size , GWidget* parent );
	GButton*   addButton( int id , char const* tile );
	GButton*   addButton(char const* title, WidgetEventDelegate delegate);
	GCheckBox* addCheckBox(int id, char const* tile);
	GCheckBox* addCheckBox(char const* title, WidgetEventDelegate delegate);
	GSlider*   addSlider( int id );
	GText*     addText(char const* pText);

private:
	template< class T >
	T* addWidget(int id , char const* title);
	template< class T >
	T* addWidget(char const* title, WidgetEventDelegate delegate);
	int mNextWidgetPosY;
};



class  WidgetUtility
{
public:
	static DevFrame* CreateDevFrame( Vec2i const& size = Vec2i(150, 200));

};
#endif // WidgetUtility_h__
