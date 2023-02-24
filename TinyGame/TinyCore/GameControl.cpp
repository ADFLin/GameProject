#include "TinyGamePCH.h"
#include "GameControl.h"

#include "StdUtility.h"
#include <algorithm>


namespace
{
	class EmptyIActionLanucher : public IActionLanucher
	{
	public:
		virtual void fireAction( ActionTrigger& trigger ){}
	} gEmptyLanucher;
}

ActionProcessor::ActionProcessor() 
	:mLanucher( &gEmptyLanucher )
{

}

void ActionProcessor::scanControl( unsigned flag /*= 0 */ )
{
	scanControl( *mLanucher , flag );
}

void ActionProcessor::scanControl( IActionLanucher& lanucher , unsigned flag )
{
	bool bUpdateFrame = ( flag & CTF_FREEZE_FRAME ) == 0;

	visitListener([&](IActionListener* listener)
	{
		listener->onScanActionStart(bUpdateFrame);
	});
	
	for( auto info : mInputList )
	{
		if ( info.input->scanInput( bUpdateFrame ) )
		{
			mActiveInputs.push_back( &info );
		}
	}

	ActionTrigger trigger;
	trigger.mParam.bUpdateFrame = bUpdateFrame;
	trigger.mbAcceptFireAction = (flag & CTF_BLOCK_ACTION) == 0;
	trigger.mProcessor = this;
	trigger.mParam.port = ERROR_ACTION_PORT;
	lanucher.fireAction( trigger );

	visitListener([&](IActionListener* listener)
	{
		listener->onScanActionEnd();
	});

	mActiveInputs.clear();
}

bool ActionProcessor::checkActionPrivate( ActionParam& param )
{
	bool result = false;
	for( auto pInfo : mActiveInputs )
	{
		IActionInput* input = pInfo->input;
		if ( pInfo->port == ERROR_ACTION_PORT || param.port == pInfo->port  )
		{
			if( input->checkAction(param) )
			{
				result = true;
			}
		}
	}
	return result;
}

void ActionProcessor::addInput( IActionInput& input , ActionPort targetPort )
{
	InputInfo info;
	info.input = &input;
	info.port = targetPort;
	mInputList.push_back(info);
}

bool ActionProcessor::removeInput( IActionInput& input )
{
	assert(mActiveInputs.empty());
	return mInputList.removePred( [&input](InputInfo const& info) { return info.input == &input; });
}

void ActionProcessor::prevFireActionPrivate( ActionParam& param )
{
	visitListener([&](IActionListener* listener)
	{
		listener->onFireAction(param);
	});
}

void ActionProcessor::beginAction( unsigned flag /*= 0 */ )
{
	scanControl( flag );
}

void ActionProcessor::endAction()
{

}

void ActionProcessor::addListener( IActionListener& listener )
{
	assert(std::find(mListeners.begin(), mListeners.end(), &listener) == mListeners.end());
	mListeners.push_back(&listener);
}

bool ActionProcessor::removeListener(IActionListener& listener)
{
	auto iter = std::find(mListeners.begin(), mListeners.end(), &listener);
	if( iter == mListeners.end() )
		return false;

	mListeners.erase(iter);
	return true;
}

void ActionProcessor::setLanucher( IActionLanucher* lanucher )
{
	mLanucher = (lanucher) ? lanucher : &gEmptyLanucher;
}

bool ActionTrigger::detect(ActionPort port, ControlAction action)
{
	mParam.act = action;
	mParam.port = port;
	mParam.bPeek = false;
	if( !mProcessor->checkActionPrivate(mParam) )
		return false;

	mProcessor->prevFireActionPrivate(mParam);
	return mbAcceptFireAction;
}

bool ActionTrigger::peek(ActionPort port, ControlAction action )
{
	mParam.act = action;
	mParam.port = port;
	mParam.bPeek = true;
	return mProcessor->checkActionPrivate( mParam );
}

DefaultInputControl::DefaultInputControl()
{
	mActionBlocked = false;

	for( int i = 0 ; i < MAX_PLAYER_NUM ; ++i )
		mCMap[i] = -1;

	std::fill_n( mKeySen , ARRAY_SIZE( mKeySen ) , 0 );
}

void DefaultInputControl::restart()
{
	std::fill_n(mKeySen, ARRAY_SIZE(mKeySen), 0);
}

void DefaultInputControl::initKey( ControlAction act , int sen , uint8 key0 , uint8 key1 )
{
	ActionKey& key = mActionKeyMap[act];
	key.keyChar[0] = key0;
	key.keyChar[1] = key1;
	key.sen = sen;
	key.senCur[0] = 0;
	key.senCur[1] = 0;
}

bool DefaultInputControl::scanInput( bool beUpdateFrame )
{
	if( mActionBlocked )
		return false;

	return ::GetKeyboardState(mKeyState) || mMouseEventMask;
}

void DefaultInputControl::setKey( unsigned cID , ControlAction action , unsigned key )
{
	assert( 0 <= cID && cID < MaxControlerNum );
	mActionKeyMap[ action ].keyChar[ cID ] = key;
}

bool DefaultInputControl::checkKey( unsigned key )
{
	return mKeyState[ key ] > 1;
}

bool DefaultInputControl::peekKey( uint8 k , uint8 keySen , uint8 sen )
{
	if ( sen == KEY_ONCE )
	{
		if ( checkKey( k ) )
		{
			if ( keySen == 0 )
			{
				return true;
			}
		}
	}
	else
	{
		if ( checkKey( k ) )
		{
			if ( keySen >= sen )
			{
				return true;
			}
		}
	}
	return false;
}

bool DefaultInputControl::checkKey( uint8 k , uint8& keySen , uint8 sen )
{
	if ( sen == KEY_ONCE )
	{
		if ( checkKey( k ) )
		{
			if ( keySen == 0 )
			{
				keySen = 1;
				return true;
			}
		}
		else if ( keySen )
		{
			keySen = 0;
		}
	}
	else
	{
		if ( checkKey( k ) )
		{
			if ( keySen >= sen )
			{
				keySen = 0;
				return true;
			}
			else
			{
				++keySen;
			}
		}
		else
		{
			keySen = sen;
		}
	}
	return false;
}

bool DefaultInputControl::checkKey( unsigned key , uint8 sen )
{
	return checkKey( key , mKeySen[ key ] , sen );
}

bool DefaultInputControl::checkKey( unsigned cID , ControlAction action )
{
	ActionKey& key = mActionKeyMap[ action ];
	uint8 keyChar = key.keyChar[ cID ];
	return checkKey( keyChar , key.senCur[ cID ] , key.sen );
}

bool DefaultInputControl::checkActionKey( ActionParam& param )
{
	unsigned cID = mCMap[ param.port ];

	if ( cID == -1 )
		return false;

	ActionKey& key = mActionKeyMap[ param.act ];
	uint8 keyChar = key.keyChar[ cID ];

	if ( param.bPeek )
		return peekKey( keyChar , mKeySen[ keyChar ] , key.sen );
	else
		return checkKey( keyChar , mKeySen[ keyChar ] , key.sen );
}

void DefaultInputControl::recvMouseMsg( MouseMsg const& msg )
{
	int button = -1;
	switch ( msg.getMsg() & MBS_BUTTON_MASK )
	{
	case MBS_LEFT:   button = LBUTTON; break;
	case MBS_RIGHT:  button = RBUTTON; break;
	case MBS_MIDDLE: button = MBUTTON; break;
	default:
		if ( msg.getMsg() & MBS_MOVING )
		{
			button = MOVING;
		}
	}

	if ( button != -1 )
	{
		mMouseEventMask |= BIT( button );
		mMouseEvt[ button ] = msg;
	}

	mLastMousePos = msg.getPos();
}

void DefaultInputControl::removeMouseEvent( int evt )
{
	mMouseEventMask &= ~evt;
}

MouseMsg* DefaultInputControl::getMouseEvent( int evt )
{
	if ( mMouseEventMask & BIT(evt ) )
		return &mMouseEvt[ evt ];
	return NULL;
}

void DefaultInputControl::setPortControl( unsigned port , unsigned cID )
{
	assert( 0 <= cID && cID < MaxControlerNum );
	assert( 0 <= port && port < MAX_PLAYER_NUM );
	if ( port == ERROR_ACTION_PORT )
		return;

	mCMap[ port ] = cID;
}

char DefaultInputControl::getKey( unsigned cID , ControlAction action )
{
	assert( 0 <= cID && cID < MaxControlerNum );
	return mActionKeyMap[ action ].keyChar[ cID ];
}

void DefaultInputControl::clearFrameInput()
{
	mMouseEventMask = 0;
}

void DefaultInputControl::clearAllKey()
{

}
