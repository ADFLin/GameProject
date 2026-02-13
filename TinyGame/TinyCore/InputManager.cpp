#include "TinyGamePCH.h"
#include "InputManager.h"

#include "WindowsHeader.h"

#include <algorithm>
#include "Core/Memory.h"

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
	if ( key < 256 )
		return mKeys[ key ];
	return false;
}

InputManager& InputManager::Get()
{
	static InputManager sInstance;
	return sInstance;
}

InputManager::InputManager()
{
	FMemory::Set(mKeys, 0, sizeof(mKeys));
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
	if ( key < 256 )
		mKeys[ key ] = isDown;

	for( KeyCallbackList::iterator iter = mKeyCBList.begin();
		iter != mKeyCBList.end() ; ++iter )
	{
		(*iter)( (char)key , isDown );
	}
}
