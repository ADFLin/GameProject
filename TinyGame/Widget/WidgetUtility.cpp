#include "TinyGamePCH.h"
#include "Widget/WidgetUtility.h"

#include "GameGUISystem.h"
#include "DrawEngine.h"

int const WidgetPosOffset = 6;
int const WidgetGapY = 4;


DevFrame::DevFrame( int id , Vec2i const& pos , Vec2i const& size , GWidget* parent ) 
	:GFrame( id , pos , size , parent )
{
	mNextWidgetPosY = 10;
}


template< class T , class LAMBDA >
T* DevFrame::addWidget( LAMBDA Lambda )
{
	Vec2i widgetSize = Vec2i(getSize().x - 2 * WidgetPosOffset, 20);
	Vec2i widgetPos(6, mNextWidgetPosY);

	T* widget = Lambda(widgetPos , widgetSize);

	mNextWidgetPosY += widget->getSize().y + WidgetGapY;
	if( getSize().y < mNextWidgetPosY )
	{
		setSize(Vec2i(getSize().x, mNextWidgetPosY));
	}
	return widget;
}

template< class T >
T* DevFrame::addWidget(int id, char const* title)
{
	return addWidget<T>([&](Vec2i const& widgetPos, Vec2i const& widgetSize) ->auto
	{
		T* widget = new T(id, widgetPos, widgetSize, this);
		widget->setTitle(title);
		return widget;
	});
}

GButton* DevFrame::addButton( int id , char const* title )
{
	return addWidget<GButton>(id, title);
}

GCheckBox* DevFrame::addCheckBox(int id, char const* title)
{
	return addWidget<GCheckBox>(id, title);
}

template< class T >
T* DevFrame::addWidget(char const* title, WidgetEventDelegate delegate)
{
	return addWidget<T>([&](Vec2i const& widgetPos, Vec2i const& widgetSize) ->auto
	{
		T* widget = new T(UI_ANY, widgetPos, widgetSize, this);
		widget->setTitle(title);
		widget->onEvent = delegate;
		return widget;
	});
}

GButton* DevFrame::addButton(char const* title, WidgetEventDelegate delegate)
{
	return addWidget<GButton>( title , delegate );
}

GCheckBox* DevFrame::addCheckBox(char const* title, WidgetEventDelegate delegate)
{
	return addWidget<GCheckBox>(title, delegate);
}

GSlider* DevFrame::addSlider(int id)
{
	return addWidget<GSlider>([&](Vec2i const& widgetPos, Vec2i const& widgetSize) ->auto
	{
		GSlider* widget = new GSlider(id, widgetPos, widgetSize.x, true, this);
		return widget;
	});
}

GText* DevFrame::addText(char const* pText)
{
	return addWidget<GText>([&](Vec2i const& widgetPos , Vec2i const& widgetSize ) ->auto
	{
		GText* widget = new GText(widgetPos, widgetSize, this);
		widget->setText(pText);
		return widget;
	});
}

DevFrame* WidgetUtility::CreateDevFrame( Vec2i const& size )
{
	GUISystem& system = ::Global::GUI();

	Vec2i screenSize = Global::GetDrawEngine().getScreenSize();

	DevFrame* frame = new DevFrame( UI_ANY , Vec2i( screenSize.x - size.x - 5 , 5 ) , size  , NULL );
	frame->setRenderType( GPanel::eRectType );
	frame->addButton( UI_MAIN_MENU , "Main Menu" );
	system.addWidget( frame );

	return frame;
}

