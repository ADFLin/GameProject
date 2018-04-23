#include "TinyGamePCH.h"
#include "OptionNoteBook.h"

#include "GameWidgetID.h"

#include "Tetris/TetrisActionId.h"

#include "RenderUtility.h"

GameController* KeyButton::sController = NULL;
KeyButton*      KeyButton::sInputButton = NULL;
Vec2i const    KeyButton::UI_Size( 100 , 20 );

Vec2i OptionNoteBook::UI_Size( 300 , 300 );


void OptionNoteBook::init( GameController& controller )
{
	KeyButton::sController = &controller;

	Vec2i pageSize = getPageSize();
	Page* page;
	page = addPage( LOCTEXT("Control") );
	page->setRenderCallback( RenderCallBack::Create( this , &OptionNoteBook::renderControl ) );
	{
		Vec2i buttonPos( 130 , 20 );
		Vec2i bPosOff( 0 , 25 );
		KeyButton* button;
		button = createKeyButton( buttonPos += bPosOff , Tetris::ACT_MOVE_LEFT , page );
		button = createKeyButton( buttonPos += bPosOff , Tetris::ACT_MOVE_RIGHT , page );
		button = createKeyButton( buttonPos += bPosOff , Tetris::ACT_MOVE_DOWN , page );
		button = createKeyButton( buttonPos += bPosOff , Tetris::ACT_ROTATE_CW , page );
		button = createKeyButton( buttonPos += bPosOff , Tetris::ACT_ROTATE_CCW , page );
		button = createKeyButton( buttonPos += bPosOff , Tetris::ACT_HOLD_PIECE , page );
		button = createKeyButton( buttonPos += bPosOff , Tetris::ACT_FALL_PIECE  , page );
	}
	{
		Vec2i buttonSize( 100 , 20 );
		Vec2i buttonPos = pageSize - buttonSize - Vec2i( 5 , 5 );
		Vec2i bPosOff( -( buttonSize.x + 10 ) , 0 );

		GButton* button; 
		button = new GButton( UI_YES        , buttonPos , buttonSize , page );
		button->setTitle( LOCTEXT("Accept") );
		button = new GButton( UI_SET_DEFULT , buttonPos += bPosOff  , buttonSize , page );
		button->setTitle( LOCTEXT("Set Default") );
	}

	page = addPage( "User");
	page->setRenderCallback( RenderCallBack::Create( this , &OptionNoteBook::renderUser ) );
}

KeyButton* OptionNoteBook::createKeyButton( Vec2i const& pos , ControlAction action , GWidget* parent )
{
	KeyButton* button;
	button = new KeyButton( UI_SET_KEY , pos, action, parent );
	button->setRenderCallback( RenderCallBack::Create( this , &OptionNoteBook::renderKeyTitle ) );
	button->setColor( Color::eRed );
	return button;
}

void OptionNoteBook::renderControl( GWidget* ui )
{
	Vec2i pos  = ui->getWorldPos();
	Vec2i size = ui->getSize();
	size.y = 50;

	Graphics2D& g = Global::getGraphics2D();

	RenderUtility::SetFont( g , FONT_S16 );
	g.drawText( pos , size , LOCTEXT("Keyborad") );
}

void OptionNoteBook::renderKeyTitle( GWidget* ui )
{
	KeyButton* button = static_cast< KeyButton* >( ui );

	Graphics2D& g = Global::getGraphics2D();

	char const* str = NULL;
	switch( button->getAction() )
	{
	case Tetris::ACT_MOVE_LEFT:        str = "Left";  break;
	case Tetris::ACT_MOVE_RIGHT:       str = "Right"; break;
	case Tetris::ACT_MOVE_DOWN:        str = "Down";  break;
	case Tetris::ACT_ROTATE_CW:        str = "Rote";  break;
	case Tetris::ACT_ROTATE_CCW:       str = "Rote-R"; break;
	case Tetris::ACT_HOLD_PIECE:       str = "Hold Piece"; break;
	case Tetris::ACT_FALL_PIECE:       str = "Fall Piece"; break;
	default:
		str = "Unknow Key";
	}

	Vec2i pos = ui->getWorldPos();

	RenderUtility::SetFont( g , FONT_S12 );
	if ( button == KeyButton::sInputButton )
		g.setTextColor( Color3ub(255, 255 , 255) );
	else
		g.setTextColor( Color3ub(255 , 255 , 0) );

	g.drawText( pos + Vec2i( -80 , 0 ) , str );
}

void OptionNoteBook::renderUser( GWidget* ui )
{
	Vec2i pos = ui->getWorldPos();
	Graphics2D& g = Global::getGraphics2D();

	g.drawText( pos + Vec2i( 10 , 10 ) , "BB" );
}

KeyButton::KeyButton( int id , Vec2i const& pos , ControlAction action , GWidget* parent ) 
	:GButton( id , pos , UI_Size , parent )
	,mAction( action )
{
	setCurKey( sController->getKey( 0 ,mAction ) );
}

bool KeyButton::onKeyMsg( unsigned key , bool isDown )
{
	if ( sInputButton == this && isDown )
	{
		setCurKey( key );
		sInputButton = NULL;
		return false;
	}
	return true;
}

void KeyButton::setCurKey( unsigned key )
{
	FixString< 256 > title;
	sController->setKey( 0 , mAction , key );
	title.format( "%c" , key );
	setTitle( title );
}

void KeyButton::onClick()
{
	if ( sInputButton != this )
	{
		if ( sInputButton )
			sInputButton->cancelInput();
		sInputButton = this;

		setTitle( LOCTEXT("Input Key") );
	}
	else
	{
		cancelInput();
		sInputButton = NULL;
	}
	GButton::onClick();
}

void KeyButton::onMouse( bool beInside )
{

}

void KeyButton::cancelInput()
{
	setCurKey( sController->getKey( 0 , mAction ) );
}
