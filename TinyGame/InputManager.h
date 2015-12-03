#ifndef InputManager_h__
#define InputManager_h__

#include "Singleton.h"
#include "FastDelegate/FastDelegate.h"

#include <list>

class MouseMsg;
typedef fastdelegate::FastDelegate< void ( MouseMsg const& ) > MouseCallback;
typedef fastdelegate::FastDelegate< void ( char , bool ) >     KeyCallback;

class  InputManager : public SingletonT< InputManager >
{
public:
	GAME_API void listenMouseEvent( MouseCallback const& callback );
	GAME_API void stopMouseEvent  ( MouseCallback const& callback );
	GAME_API void listenKeyEvent  ( KeyCallback const& callback );
	GAME_API void stopKeyEvent    ( KeyCallback const& callback );

	GAME_API bool isKeyDown( unsigned key );

public:
	GAME_API void procMouseEvent( MouseMsg const& msg );
	GAME_API void procKeyEvent  ( unsigned key , bool isDown );
private:
	typedef std::list< MouseCallback > MouseCallbackList;
	MouseCallbackList mMouseCBList;

	typedef std::list< KeyCallback > KeyCallbackList;
	KeyCallbackList mKeyCBList;
};

#endif // InputManager_h__
