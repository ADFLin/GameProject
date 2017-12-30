#include "TinyGamePCH.h"
#include "WidgetUtility.h"

#include "GameGUISystem.h"
#include "DrawEngine.h"

int const WidgetPosOffset = 6;
int const WidgetGapY = 4;
DevFrame::DevFrame( int id , Vec2i const& pos , Vec2i const& size , GWidget* parent ) 
	:GFrame( id , pos , size , parent )
{
	mNextWidgetPosY = 10;
}

GButton* DevFrame::addButton( int id , char const* title )
{
	Vec2i widgetSize = Vec2i( getSize().x - 2 * WidgetPosOffset, 20 );
	Vec2i widgetPos( 6 , mNextWidgetPosY );
	
	GButton* widget = new GButton( id , widgetPos , widgetSize , this );
	widget->setTitle( title );

	mNextWidgetPosY += widget->getSize().y + WidgetGapY;
	return widget;
}

GButton* DevFrame::addButton(char const* title, WidgetEventDelegate delegate)
{
	Vec2i widgetSize = Vec2i(getSize().x - 2 * WidgetPosOffset, 20);
	Vec2i widgetPos(6, mNextWidgetPosY);

	GButton* widget = new GButton(UI_ANY, widgetPos, widgetSize, this);
	widget->setTitle(title);
	widget->onEvent = delegate;

	mNextWidgetPosY += widget->getSize().y + WidgetGapY;

	return widget;
}

GSlider* DevFrame::addSlider(int id)
{
	Vec2i widgetSize = Vec2i(getSize().x - 2 * WidgetPosOffset, 20);
	Vec2i widgetPos(6, mNextWidgetPosY);
	GSlider* widget = new GSlider(id, widgetPos, widgetSize.x , true , this);

	mNextWidgetPosY += widget->getSize().y + WidgetGapY;
	return widget;
}

GText* DevFrame::addText(char const* pText)
{
	Vec2i widgetSize = Vec2i(getSize().x - 2 * WidgetPosOffset, 10);
	Vec2i widgetPos(6, mNextWidgetPosY);

	GText* widget = new GText( widgetPos, widgetSize, this);
	widget->setText(pText);

	mNextWidgetPosY += widget->getSize().y + WidgetGapY;
	return widget;
}

DevFrame* WidgetUtility::CreateDevFrame( Vec2i const& size )
{
	GUISystem& system = ::Global::GUI();

	Vec2i screenSize = Global::getDrawEngine()->getScreenSize();

	DevFrame* frame = new DevFrame( UI_ANY , Vec2i( screenSize.x - size.x - 5 , 5 ) , size  , NULL );
	frame->setRenderType( GPanel::eRectType );
	frame->addButton( UI_MAIN_MENU , "Main Menu" );
	system.addWidget( frame );

	return frame;
}

