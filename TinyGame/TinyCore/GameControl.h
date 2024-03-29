#ifndef GameControl_h__
#define GameControl_h__

#include "GameGlobal.h"
#include "SystemMessage.h"

#include "DataStructure/Array.h"

#include <cassert>

typedef uint32 ControlAction;
struct ActionPort
{
	ActionPort() = default;
	ActionPort( uint32 inValue ): value(inValue){}

	operator uint32() const { return value; }
	uint32 value;
};

class IActionInput;
class ActionProcessor;

#define ERROR_ACTION_PORT ActionPort(-1)

struct ActionParam
{
	ActionPort    port;
	ControlAction act;
	void*         result;
	bool          bUpdateFrame;
	bool          bPeek;

	template< class T >
	void setResult( T* pVal )
	{
#ifdef _DEBUG
		info = &typeid( T );
#endif // _DEBUG
		result = pVal;
	}

	template< class T >
	T&  getResult()
	{
#ifdef _DEBUG
		assert( typeid( T ) == *info );
#endif // _DEBUG
		return *static_cast< T* >( result );
	}
#ifdef _DEBUG
	type_info const* info;
#endif // _DEBUG
};

class ActionTrigger
{
public:

	TINY_API bool      peek(ActionPort port, ControlAction action );
	template < class T >
	bool      peek(ActionPort port, ControlAction action , T* result )
	{
		mParam.setResult( result );
		return peek( port , action );
	}

	TINY_API bool      detect(ActionPort port , ControlAction action);
	template < class T >
	bool      detect(ActionPort port, ControlAction action, T* result)
	{
		mParam.setResult(result);
		return detect(port, action);
	}
	bool      haveUpdateFrame(){ return mParam.bUpdateFrame; }
private:
	ActionParam      mParam;
	ActionProcessor* mProcessor;
	bool             mbAcceptFireAction;
	friend class ActionProcessor;
};

class  IActionLanucher
{
public:
	//call ActionTrigger::detect and act action if return ture
	virtual void fireAction( ActionTrigger& trigger ) = 0;
};

class IActionInput
{
public:
	virtual ~IActionInput(){}
	virtual bool scanInput( bool bUpdateFrame ) = 0;
	virtual bool checkAction( ActionParam& param ) = 0;
};

class  IActionListener
{
public:
	virtual ~IActionListener(){}
	virtual void onScanActionStart( bool bUpdateFrame ){}
	virtual void onFireAction( ActionParam& param ) = 0;
	virtual void onScanActionEnd(){}
};

enum ControlFlag
{
	CTF_FREEZE_FRAME   = BIT(0),
	CTF_BLOCK_ACTION   = BIT(1),
};

class  ActionProcessor
{
public:
	TINY_API ActionProcessor();

	TINY_API void beginAction( unsigned flag = 0 );
	TINY_API void endAction();

	TINY_API void setLanucher( IActionLanucher* lanucher );
	TINY_API void addListener( IActionListener& listener );
	TINY_API bool removeListener(IActionListener& listener);
	
	TINY_API void addInput   ( IActionInput& input , ActionPort targetPort = ERROR_ACTION_PORT );
	TINY_API bool removeInput( IActionInput& input );

public:
	void prevFireActionPrivate( ActionParam& param );
	bool checkActionPrivate( ActionParam& param );
protected:
	TINY_API void scanControl( unsigned flag = 0 );
	TINY_API void scanControl( IActionLanucher& lanucher , unsigned flag = 0 );
private:

	template< class TFunc >
	void visitListener(TFunc&& func)
	{
		for (auto listener : mListeners)
		{
			func(listener);
		}
	}

	struct InputInfo
	{
		IActionInput* input;
		unsigned     port;
	};
	TArray< InputInfo >  mInputList;
	TArray< InputInfo* > mActiveInputs;

	TArray< IActionListener* > mListeners;

	IActionLanucher* mLanucher;
};



typedef unsigned char uint8;
#define KEY_ONCE (BYTE)(~0)

enum GameAction
{
	ACT_BUTTON0 = 0,
	ACT_BUTTON1 = 1,
	ACT_BUTTON2 = 2,
};

class  InputControl
{
public:
	virtual void  restart(){}
	virtual void  setupInput(ActionProcessor& proccessor) = 0;

	virtual void  blockAllAction( bool beB ) = 0;
	virtual bool  shouldLockMouse(){ return false;  }

	virtual void  setPortControl( unsigned port , unsigned cID ) = 0;
	virtual void  setKey( unsigned cID , ControlAction action , unsigned key ) = 0;
	virtual char  getKey( unsigned cID , ControlAction action ) = 0;
	virtual bool  checkKey( unsigned cID , ControlAction action ) = 0;

	virtual void  recvMouseMsg( MouseMsg const& msg ) = 0;
	virtual void  clearFrameInput() = 0;
};



class  DefaultInputControl : public InputControl
	                       , public IActionInput
{
public:
	TINY_API DefaultInputControl();

	TINY_API void  restart();
	TINY_API void  clearAllKey();
	TINY_API void  initKey( ControlAction act , int sen , uint8 key0 , uint8 key1 = 0xff );
	TINY_API void  setKey( unsigned cID , ControlAction action , unsigned key );
	TINY_API char  getKey( unsigned cID , ControlAction action );
	TINY_API bool  checkKey( unsigned cID , ControlAction action );
	TINY_API void  setPortControl( unsigned port , unsigned cID );

	TINY_API bool  checkKey( unsigned key );
	TINY_API bool  checkKey( unsigned key , uint8 sen );
	TINY_API void  recvMouseMsg( MouseMsg const& msg );

	void  blockAllAction( bool beB ){ mActionBlocked = beB;  }
	
	TINY_API void  clearFrameInput();
	TINY_API Vec2i const& getLastMosuePos(){ return mLastMousePos;  }

protected:

	enum
	{
		LBUTTON ,
		MBUTTON ,
		RBUTTON ,
		MOVING ,

		NUM_MOUSE_EVENT ,
	};

	TINY_API MouseMsg* getMouseEvent( int evt );
	TINY_API void      removeMouseEvent( int evt );
	TINY_API bool      checkActionKey( ActionParam& param );
	TINY_API virtual bool checkAction( ActionParam& param ){ return checkActionKey( param ); }
	TINY_API virtual bool scanInput( bool beUpdateFrame );

	virtual void  setupInput(ActionProcessor& proccessor)
	{
		proccessor.addInput(*this);
	}

private:	

	static int const MaxControlerNum = 2;
	static int const MaxActionKeyNum = 32;

	unsigned mMouseEventMask;
	MouseMsg mMouseEvt[ NUM_MOUSE_EVENT ];

private:
	bool    mActionBlocked;
	Vec2i   mLastMousePos;

	uint8    mKeyState[ 256 ];
	uint8    mKeySen[ 256 ];
	int      mCMap[ MAX_PLAYER_NUM ];

	struct ActionKey
	{
		uint8 sen;
		uint8 keyChar[ MaxControlerNum ];
		uint8 senCur[ MaxControlerNum ];
	};


	ActionKey mActionKeyMap[ MaxActionKeyNum ];
	bool checkKey( uint8 k , uint8& keySen , uint8 sen /*= 2 */ );
	bool peekKey( uint8 k , uint8 keySen , uint8 sen );
};

#endif // GameControl_h__