#include "TGameStage.h"
#include "TGame.h"
#include "profile.h"
#include "TMessageShow.h"
#include "TFlyConsoleSystem.h"
#include "TlogOutput.h"
#include "TAudioPlayer.h"
#include "TUISystem.h"
#include "TTextButton.h"
#include "TEventSystem.h"
#include "TEventSystem.h"
#include "EventType.h"

TFlyConsoleSystem g_console;
TLogOutput        g_log;

bool TGameStage::changeStage( StageType state , bool beReset )
{
	return g_Game->changeStage( state , beReset );
}

void TGameStage::reader()
{
	g_Game->render3DScene();
	g_Game->renderUI();
}

bool TMainStage::OnMessage( unsigned key , bool isDown )
{
	if ( !isDown )
		return true;

	if ( key == VK_OEM_3 )
	{
		changeStage( GS_CONSOLE );
		return false;
	}
	else if ( key == VK_TAB )
	{
		changeStage( GS_PROFILE  , true );
		return false;
	}
	return true;
}

void TConsoleStage::reader()
{
	TGameStage::reader();

	TMessageShow msgShow( g_Game->getWorld() );
	msgShow.setPos( 20 , 700 );
	msgShow.setOffsetY(-18);
	msgShow.start();
	msgShow.push( "Console : %s" , g_console.getInputStr().c_str() );
	g_log.show(msgShow);
	msgShow.finish();
}

bool TConsoleStage::OnMessage( unsigned key , bool isDown )
{
	if ( !isDown )
		return false;

	switch( key )
	{
	case VK_OEM_3:  changeStage( GS_MAIN ); break;;
	case VK_RETURN: g_console.doCommand(); break;
	case VK_BACK:   g_console.popChar(); break;
	case VK_UP:     g_console.setSaveString(-1); break;
	case VK_DOWN:   g_console.setSaveString(1);  break;
	case VK_TAB:    g_console.setFindCommand(1); break;
	}
	return false;
}

bool TConsoleStage::OnMessage( char c )
{
	if ( c> 31 && c != '`' )
	{
		g_console.pushChar( c );
	}
	return false;
}

TProFileStage::TProFileStage() 
	: TGameStage( GS_PROFILE )
{
	profileIter = TProfileManager::createIterator();
}

TProFileStage::~TProFileStage()
{
	TProfileManager::releaseIterator( profileIter );
}

bool TProFileStage::OnMessage( unsigned key , bool isDown )
{
	if ( !isDown )
		return false;

	TProfileIterator* iter = profileIter;

	if ( iter->isDone() )
		iter->enterParent();

	switch( key )
	{
	case VK_TAB:    g_Game->changeStage( GS_MAIN ); break;
	case VK_RETURN: profileIter->getCurNode()->showChild( ! iter->getCurNode()->isShowChild() ); break;
	case VK_UP:     profileIter->Before(); break;
	case VK_DOWN:   profileIter->Next(); break;
	case VK_LEFT:   profileIter->enterParent(); break;
	case VK_RIGHT:
		profileIter->getCurNode()->showChild( true );
		profileIter->enterChild();
		profileIter->First();
		break;
	}

	if ( iter->isDone() )
		iter->enterParent();

	return false;
}

void TProFileStage::reset()
{
	if ( TProfileManager::resetIter )
	{
		TProfileManager::releaseIterator( profileIter );
		profileIter = TProfileManager::createIterator();

		TProfileManager::resetIter = false;
	}
}
