#include "TinyGamePCH.h"
#include "WidgetUtility.h"

#include "GameGUISystem.h"
#include "DrawEngine.h"

DevFrame::DevFrame( int id , Vec2i const& pos , Vec2i const& size , GWidget* parent ) 
	:GFrame( id , pos , size , parent )
{

}

GButton* DevFrame::addButton( int id , char const* tile )
{
	Vec2i buttonSize = Vec2i( getSize().x - 12 , 20 );
	Vec2i buttonPos( 6 , 10 + ( buttonSize.y + 4 ) * getChildrenNum() );
	GButton* button = new GButton( id , buttonPos , buttonSize , this );
	button->setTitle( tile );
	return button;
}

DevFrame* WidgetUtility::createDevFrame()
{
	GUISystem& system = ::Global::getGUI();

	Vec2i sSize = Global::getDrawEngine()->getScreenSize();

	Vec2i frameSize( 100 , 200 );
	DevFrame* frame = new DevFrame( UI_ANY , Vec2i( sSize.x - frameSize.x - 5 , 5 ) , frameSize  , NULL );
	frame->setRenderType( GPanel::eRectType );
	frame->addButton( UI_MAIN_MENU , "Main Menu" );
	system.addWidget( frame );

	return frame;
}

