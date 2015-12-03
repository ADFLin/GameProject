#ifndef WidgetUtility_h__
#define WidgetUtility_h__

#include "GameWidget.h"
#include "GameWidgetID.h"

class DevFrame : public GFrame
{
public:
	DevFrame( int id ,  Vec2i const& pos , Vec2i const& size , GWidget* parent );
	GButton* addButton( int id , char const* tile );
};
class  WidgetUtility
{
public:
	static DevFrame* createDevFrame();

};
#endif // WidgetUtility_h__
