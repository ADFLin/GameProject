#include "TinyGamePCH.h"
#include "InputManager.h"

#include "Win32Header.h"

#include <algorithm>

void InputManager::listenMouseEvent( MouseCallback const& callback )
{
	assert( std::find( mMouseCBList.begin() , mMouseCBList.end() , callback ) == mMouseCBList.end() );
	mMouseCBList.push_back( callback );
}

void InputManager::stopMouseEvent( MouseCallback const& callback )
{
	MouseCallbackList::iterator iter = std::find( mMouseCBList.begin() , mMouseCBList.end() , callback );
	if ( iter != mMouseCBList.end() )
		mMouseCBList.erase( iter );
}

void InputManager::listenKeyEvent( KeyCallback const& callback )
{
	assert( std::find( mKeyCBList.begin() , mKeyCBList.end() , callback ) == mKeyCBList.end() );
	mKeyCBList.push_back( callback );
}

void InputManager::stopKeyEvent( KeyCallback const& callback )
{
	KeyCallbackList::iterator iter = std::find( mKeyCBList.begin() , mKeyCBList.end() , callback );
	if ( iter != mKeyCBList.end() )
		mKeyCBList.erase( iter );
}


bool InputManager::isKeyDown( unsigned key )
{
	return( ::GetAsyncKeyState( key ) &0x8000 ) != 0;
}

void InputManager::procMouseEvent( MouseMsg const& msg )
{
	for( MouseCallbackList::iterator iter = mMouseCBList.begin();
		iter != mMouseCBList.end() ; ++iter )
	{
		(*iter)( msg );
	}
}

void InputManager::procKeyEvent( unsigned key , bool isDown )
{
	for( KeyCallbackList::iterator iter = mKeyCBList.begin();
		iter != mKeyCBList.end() ; ++iter )
	{
		(*iter)( key , isDown );
	}
}
