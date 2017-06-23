#ifndef GameControl_h__
#define GameControl_h__

#include "GameGlobal.h"
#include "SystemMessage.h"

#include <cassert>
#include <cstdio>
#include <vector>

typedef unsigned ControlAction;

class IActionInput;
class ActionProcessor;

#define ERROR_ACTION_PORT (-1)

struct ActionParam
{
	unsigned      port;
	ControlAction act;
	void*         result;
	bool          beUpdateFrame;
	bool          bePeek;

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
	void      setPort( unsigned port ){ mParam.port = port;  }
	unsigned  getPort() const { return  mParam.port;  }

	GAME_API bool      peek( ControlAction action );
	template < class T >
	bool      peek( ControlAction action , T* result )
	{
		mParam.setResult( result );
		return peek( action );
	}
	GAME_API bool      detect( ControlAction action );
	template < class T >
	bool      detect( ControlAction action , T* result )
	{
		mParam.setResult( result );
		return detect( action );
	}
	bool      haveUpdateFrame(){ return mParam.beUpdateFrame; }
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
	virtual bool scanInput( bool beUpdateFrame ) = 0;
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

#include <list>

enum ControlFlag
{
	CTF_FREEZE_FRAME   = BIT(0),
	CTF_BLOCK_ACTION   = BIT(1),
};


class  ActionProcessor
{
public:
	GAME_API ActionProcessor();

	GAME_API void beginAction( unsigned flag = 0 );
	GAME_API void endAction();

	GAME_API void setLanucher( IActionLanucher* lanucher );
	GAME_API void addListener( IActionListener& listener );
	GAME_API bool removeListener(IActionListener& listener);
	
	GAME_API void addInput   ( IActionInput& input , unsigned targetPort = ERROR_ACTION_PORT );
	GAME_API bool removeInput( IActionInput& input );

public:
	GAME_API void _prevFireAction( ActionParam& param );
	GAME_API bool _checkAction( ActionParam& param );
protected:
	GAME_API void scanControl( unsigned flag = 0 );
	GAME_API void scanControl( IActionLanucher& lanucher , unsigned flag = 0 );
private:
	typedef std::vector< IActionListener*> ListenerList;
	typedef std::vector< IActionInput* >   InputList;

	struct InputInfo
	{
		IActionInput* input;
		unsigned     port;
	};
	std::vector< IActionListener* > mListeners;

	IActionLanucher* mLanucher;
	InputList        mInputList;
	InputList        mActiveInputs;
};



typedef unsigned char uint8;
#define KEY_ONCE (BYTE)(~0)

enum GameAction
{
	ACT_BUTTON0 = 0,
	ACT_BUTTON1 = 1,
	ACT_BUTTON2 = 2,
};

class  GameController : public IActionInput
{
public:
	//ActionInput
	virtual bool  scanInput( bool beUpdateFrame ) = 0;
	virtual bool  checkAction( ActionParam& param  ) = 0;

	virtual void  blockKeyEvent( bool beB ) = 0;
	virtual bool  haveLockMouse(){ return false;  }

	virtual void  setPortControl( unsigned port , unsigned cID ) = 0;
	virtual void  setKey( unsigned cID , ControlAction action , unsigned key ) = 0;
	virtual char  getKey( unsigned cID , ControlAction action ) = 0;
	virtual bool  checkKey( unsigned cID , ControlAction action ) = 0;

	virtual void  recvMouseMsg( MouseMsg const& msg ) = 0;
	virtual void  clearFrameInput() = 0;
};



class  SimpleController : public GameController
{
public:
	GAME_API SimpleController();

	
	GAME_API void  clearAllKey();
	GAME_API void  initKey( ControlAction act , int sen , uint8 key0 , uint8 key1 = 0xff );
	GAME_API void  setKey( unsigned cID , ControlAction action , unsigned key );
	GAME_API char  getKey( unsigned cID , ControlAction action );
	GAME_API bool  checkKey( unsigned cID , ControlAction action );
	GAME_API void  setPortControl( unsigned port , unsigned cID );

	GAME_API bool  checkKey( unsigned key );
	GAME_API bool  checkKey( unsigned key , uint8 sen );
	GAME_API void  recvMouseMsg( MouseMsg const& msg );

	void  blockKeyEvent( bool beB ){ mKeyBlocked = beB;  }
	
	GAME_API void  clearFrameInput();
	GAME_API Vec2i const& getLastMosuePos(){ return mLastMousePos;  }

protected:

	enum
	{
		LBUTTON ,
		MBUTTON ,
		RBUTTON ,
		MOVING ,
	};

	GAME_API MouseMsg* getMouseEvent( int evt );
	GAME_API void      removeMouseEvent( int evt );
	GAME_API bool      checkActionKey( ActionParam& param );
	GAME_API virtual bool checkAction( ActionParam& param ){ return checkActionKey( param ); }
	GAME_API virtual bool scanInput( bool beUpdateFrame );

private:	

	static int const MaxControlerNum = 2;
	static int const MaxActionKeyNum = 32;

	unsigned mEventMask;
	MouseMsg mMouseEvt[ 4 ]; 

private:
	bool    mKeyBlocked;
	Vec2i   mLastMousePos;

	uint8    mKeyState[ 256 ];
	uint8    mKeySen[ 256 ];
	int     mCMap[ MAX_PLAYER_NUM ];

	struct ActionKey
	{
		uint8 sen;
		uint8 keyChar[ MaxControlerNum ];
		uint8 senCur[ MaxControlerNum ];
	};


	ActionKey actionKey[ MaxActionKeyNum ];
	bool checkKey( uint8 k , uint8& keySen , uint8 sen /*= 2 */ );
	bool peekKey( uint8 k , uint8 keySen , uint8 sen );
};

#endif // GameControl_h__