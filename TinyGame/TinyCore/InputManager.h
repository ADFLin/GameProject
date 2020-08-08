#ifndef InputManager_h__
#define InputManager_h__

#include "GameConfig.h"
#include "FastDelegate/FastDelegate.h"

#include <list>

class MouseMsg;
typedef fastdelegate::FastDelegate< void ( MouseMsg const& ) > MouseCallback;
typedef fastdelegate::FastDelegate< void ( char , bool ) >     KeyCallback;

class  InputManager
{
public:
	TINY_API void listenMouseEvent( MouseCallback const& callback );
	TINY_API void stopMouseEvent  ( MouseCallback const& callback );
	TINY_API void listenKeyEvent  ( KeyCallback const& callback );
	TINY_API void stopKeyEvent    ( KeyCallback const& callback );

	TINY_API bool isKeyDown( unsigned key );

	TINY_API static InputManager& Get();

public:
	TINY_API void procMouseEvent( MouseMsg const& msg );
	TINY_API void procKeyEvent  ( unsigned key , bool isDown );

	bool bActive;

private:
	typedef std::list< MouseCallback > MouseCallbackList;
	MouseCallbackList mMouseCBList;

	typedef std::list< KeyCallback > KeyCallbackList;
	KeyCallbackList mKeyCBList;
};

#endif // InputManager_h__
