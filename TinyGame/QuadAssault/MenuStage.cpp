#include "MenuStage.h"

#include "GameInterface.h"
#include "TextureManager.h"
#include "GUISystem.h"
#include "RenderSystem.h"

#include "GlobalVariable.h"
#include "DataPath.h"
#include "RenderUtility.h"
#include "Texture.h"

#include "LevelStage.h"
#include "DevStage.h"

#include "Dependence.h"
#include "InlineString.h"

#include "RHI/RHIGraphics2D.h"
#include "RHI/RHICommand.h"

#include "TinyCore/RenderUtility.h"


using namespace Render;

#include <fstream>
#include <sstream>


MenuStage::MenuStage( State state )
{
	mState = state;
}

bool MenuStage::onInit()
{
	TextureManager* texMgr = getRenderSystem()->getTextureMgr();
	texCursor = texMgr->getTexture("cursor.tga");
	texBG     = texMgr->getTexture("Menu1.tga");
	texBG2    = texMgr->getTexture("MenuLoading1.tga");		

	IFont* font = getGame()->getFont( 0 );

	GUISystem::Get().cleanupWidget();

	Vec2f poz;
	poz.x = getGame()->getScreenSize().x/2 - 224;
	poz.y = getGame()->getScreenSize().y/2 + getGame()->getScreenSize().y/8;

	QTextButton* button;
	button = new QTextButton( UI_START , Vec2i( poz.x , poz.y ) , Vec2i(128, 64) , NULL );
	button->text->setFont( font );
	button->text->setString( "Start" );
	button->show( false );
	GUISystem::Get().addWidget( button );

	button = new QTextButton( UI_ABOUT , Vec2i( poz.x +128+32 , poz.y ) , Vec2i(128, 64) , NULL );
	button->text->setFont( font );
	button->text->setString( "About" );
	button->show( false );
	GUISystem::Get().addWidget( button );

	button = new QTextButton( UI_DEV_TEST , Vec2i( poz.x +128+32 , poz.y + 80 ) , Vec2i(128, 64) , NULL );
	button->text->setFont( font );
	button->text->setString( "Dev Test" );
	button->show( false );
	GUISystem::Get().addWidget( button );

	button = new QTextButton( UI_EXIT , Vec2i( poz.x +256+64, poz.y ) , Vec2i(128, 64) , NULL );
	button->text->setFont( font );
	button->text->setString( "Exit" );
	button->show( false );
	GUISystem::Get().addWidget( button );

	button = new QTextButton( UI_BACK , Vec2i( 32 , getGame()->getScreenSize().y-96 ) , Vec2i(128, 64) , NULL );
	button->text->setFont( font );
	button->text->setString( "Back" );
	button->show( false );
	GUISystem::Get().addWidget( button );

	char const* text =
		"QuadAssault v1.0\n"
		"----------------\n"
		"You control a tank which in itself can handle various weapons.\n"
		"Movement: W or up arrow, down arrow or S\n"
		"Revolution: A or left arrow, right arrow or D\n"		
		"Shooting: Left mouse button\n"		
		"---------------------\n"
		"Made by Marko Stanic, 4.A-2\n"
		"Supervisor: Dario Jembrek, prof.\n"
		"Trade schools Koprivnica\n"
		"---------------------\n"
		"Contact :\n"
		"Mail/MSN : mstani19@gmail.com\n"
		"Blog : staniks.blogspot.com\n"
		"Youtube : www.youtube.com/geomancer120\n"
		"\n"
		"The game is written in C++, using Microsoft Visual Studio IDE-a.\n"
		"The game uses a library of functions and classes OpenGL and SFML, and shaders is\n"
		"koristen scripting language GLSL. Not used outside engine, but it was written\n"
		"own, special to the game.\n";

	mTextAbout.reset( IText::Create( font , 22 , Color4ub(50,255,25) ) );
	mTextAbout->setString( text );
	

	std::ifstream file( LEVEL_DIR "LevelList.gdf" , std::ios::in );
	std::string strLine;
	while(getline(file,strLine))
	{
		std::istringstream stream(strLine, std::ios::in);
		std::string token;
		while(getline(stream,token,' '))
		{
			if(token=="[LEVEL]")
			{				
				mLevels.push_back(LevelInfo());
				mLevels.back().index = mLevels.size() - 1;
			}
			else if(token=="level_file")
			{
				getline(stream,token,' ');
				mLevels.back().levelFile = token;
			}
			else if(token=="map_file")
			{
				getline(stream,token,' ');
				mLevels.back().mapFile = token;
			}
		}
	}
	file.close();


	std::ifstream fs( LEVEL_DIR LEVEL_LOCK_FILE );	
	for(int i=0; i<MAX_LEVEL_NUM; i++)
	{
		fs >> gLevelEnabled[i];	
	}
	fs.close();
	for(int i=0; i<mLevels.size(); i++)
	{		

		InlineString< 256 > str;
		str.format( "Level %d" , i + 1 );

		button = new QTextButton( UI_LEVEL , Vec2i( getGame()->getScreenSize().x/2-64, 64+i*96 ) , Vec2i(128, 64) , NULL );
		button->text->setFont( font );
		button->text->setString( str );
		button->setUserData( &mLevels[i] );
		button->show( false );
		GUISystem::Get().addWidget( button );

	
		mLevels[i].button = button;
	}


	State stateStart = MS_SELECT_MENU;
	if ( mState != MS_NONE )
		State stateStart = mState;

	mState = MS_NONE;
	changeState( stateStart );

	mScreenFade.setColor( 1 );
	mScreenFade.fadeIn();

	return true;
}

void MenuStage::onExit()
{	
	mTextAbout.clear();
	mLevels.clear();	
}

void MenuStage::onUpdate(float deltaT)
{
	getGame()->procSystemEvent();
	mScreenFade.update( deltaT );
}


void MenuStage::onWidgetEvent( int event , int id , QWidget* sender )
{
	switch( id )
	{
	case UI_LEVEL:
		{
			LevelInfo* info = static_cast< LevelInfo* >( sender->getUserData() );
			gLevelFileName   = info->levelFile;
			gMapFileName     = info->mapFile;
			gIdxCurLevel     = info->index;
			getGame()->addStage(new LevelStage(), true);
		}
		break;
	case UI_START:
		changeState( MS_SELECT_LEVEL ); 
		break;
	case UI_ABOUT:
		changeState( MS_ABOUT );
		break;
	case UI_BACK:
		changeState( MS_SELECT_MENU );
		break;
	case UI_EXIT:
		mScreenFade.fadeOut( std::bind( &GameStage::stop , this ) );
		break;
	case UI_DEV_TEST:
		{
			getGame()->addStage( new DevStage , true );
		}
		break;
	}
}

void MenuStage::changeState( State state )
{
	if ( mState == state )
		return;

	showStateWidget( mState , false );
	mState = state;
	showStateWidget( state , true );
}

void MenuStage::showStateWidget( State state , bool beShow )
{
	switch( state )
	{
	case MS_ABOUT:
		GUISystem::Get().findTopWidget( UI_BACK )->show( beShow );
		break;
	case MS_SELECT_LEVEL:
		GUISystem::Get().findTopWidget( UI_BACK )->show( beShow );
		for(int i=0; i<mLevels.size(); ++i)
			mLevels[i].button->show( beShow );
		break;
	case MS_SELECT_MENU:
		GUISystem::Get().findTopWidget( UI_START )->show( beShow );
		GUISystem::Get().findTopWidget( UI_ABOUT )->show( beShow );
		GUISystem::Get().findTopWidget( UI_EXIT  )->show( beShow );
		GUISystem::Get().findTopWidget( UI_DEV_TEST )->show( beShow );
		break;
	}
}

void MenuStage::onRender()
{
	RHIGraphics2D& g = getGame()->getGraphics2D();

	g.beginRender();

	g.setBrush(Color3f::White());
	switch (mState)
	{
	case MS_SELECT_LEVEL:
		g.drawTexture(*texBG2->resource, Vec2f(0, 0), getGame()->getScreenSize());
		break;
	case MS_SELECT_MENU:
		g.drawTexture(*texBG->resource, Vec2f(0, 0), getGame()->getScreenSize());
		break;
	case MS_ABOUT:
		g.drawTexture(*texBG2->resource, Vec2f(0, 0), getGame()->getScreenSize());
		//getRenderSystem()->drawText(mTextAbout, Vec2i(32, 32), TEXT_SIDE_LEFT | TEXT_SIDE_TOP);
		break;
	}

	GUISystem::Get().render();

	g.beginBlend(1, ESimpleBlendMode::Add);
	g.drawTexture(*texCursor->resource, Vec2f(getGame()->getMousePos()) - Vec2f(16, 16), Vec2f(32, 32));
	g.endBlend();


	//mScreenFade.render();
	g.endRender();

}

MsgReply MenuStage::onKey(KeyMsg const& msg)
{
	if (msg.isDown())
	{
		switch (msg.getCode())
		{
		case EKeyCode::Escape:
			if (mState == MS_SELECT_MENU)
				stop();
			else
				changeState(MS_SELECT_MENU);
			break;
		}
	}
	return BaseClass::onKey(msg);
}

MsgReply MenuStage::onMouse( MouseMsg const& msg )
{
	return BaseClass::onMouse(msg);
}
