#include "TinyGamePCH.h"
#include "GameControl.h"

#include "Win32Header.h"
#include <algorithm>

namespace
{
	class EmptyActionListener : public ActionListener
	{
	public:
		virtual void onFireAction( ActionParam& param ){}
	} gListerner;

	class EmptyActionEnumer : public ActionEnumer
	{
	public:
		virtual void fireAction( ActionTrigger& trigger ){}
	} gEnumer;
}

ActionProcessor::ActionProcessor() 
	:mListener( &gListerner )
	,mEnumer( &gEnumer )
{

}

void ActionProcessor::scanControl( unsigned flag /*= 0 */ )
{
	scanControl( *mEnumer , flag );
}

void ActionProcessor::scanControl( ActionEnumer& enumer , unsigned flag )
{
	mListener->onScanActionStart();

	ActionTrigger trigger;

	bool beUpdateFrame = ( flag & CTF_FREEZE_FRAME ) == 0;

	trigger.mParam.beUpdateFrame = beUpdateFrame;
	trigger.mbAcceptFireAction   = ( flag & CTF_BLOCK_ACTION ) == 0;
	trigger.mProcessor           = this;

	enumer.prevScanAction( beUpdateFrame );

	mActiveInputs.clear();
	for( InputList::iterator iter = mInputList.begin();
		iter != mInputList.end(); ++iter )
	{
		ActionInput* input = *iter;
		if ( input->scanInput( beUpdateFrame ) )
		{
			mActiveInputs.push_back( input );
		}
	}

	trigger.mParam.port = ERROR_ACTION_PORT;
	enumer.fireAction( trigger );

	mListener->onScanActionEnd();
}

bool ActionProcessor::_checkAction( ActionParam& param )
{
	bool result = false;
	for( InputList::iterator iter = mActiveInputs.begin();
		iter != mActiveInputs.end() ; ++iter )
	{
		ActionInput* input = *iter;
		if ( input->checkAction( param ) )
		{
			result = true;
		}
	}
	return result;
}

void ActionProcessor::addInput( ActionInput& input , unsigned targetPort )
{
	mInputList.push_back( &input );
}

bool ActionProcessor::removeInput( ActionInput& input )
{
	InputList::iterator iter = std::find( mInputList.begin() , mInputList.end() , &input );
	if ( iter == mInputList.end() )
		return false;
	mInputList.erase( iter );
	return true;
}

void ActionProcessor::_prevFireAction( ActionParam& param )
{
	mListener->onFireAction( param );
}

void ActionProcessor::beginAction( unsigned flag /*= 0 */ )
{
	scanControl( flag );
}

void ActionProcessor::endAction()
{

}

void ActionProcessor::setListener( ActionListener* listener )
{
	mListener = listener;
	if ( mListener == NULL )
		mListener = &gListerner;
}

void ActionProcessor::setEnumer( ActionEnumer* enumer )
{
	mEnumer = enumer;
	if ( mEnumer == NULL )
		mEnumer = &gEnumer;
}

bool ActionTrigger::detect( ControlAction action )
{
	mParam.act    = action;
	mParam.bePeek = false;
	if ( !mProcessor->_checkAction( mParam ) )
		return false;
	
	mProcessor->_prevFireAction( mParam );
	return mbAcceptFireAction;
}

bool ActionTrigger::peek( ControlAction action )
{
	mParam.act    = action;
	mParam.bePeek = false;
	return mProcessor->_checkAction( mParam );
}

SimpleController::SimpleController()
{
	mKeyBlocked = false;

	for( int i = 0 ; i < MAX_PLAYER_NUM ; ++i )
		mCMap[i] = -1;

	std::fill_n( mKeySen , ARRAY_SIZE( mKeySen ) , 0 );
}


void SimpleController::initKey( ControlAction act , int sen , uint8 key0 , uint8 key1 )
{
	actionKey[ act ].keyChar[0] = key0;
	actionKey[ act ].keyChar[1] = key1;
	actionKey[ act ].sen = sen;
	actionKey[ act ].senCur[0] = 0;
	actionKey[ act ].senCur[1] = 0;
}

bool SimpleController::scanInput( bool beUpdateFrame )
{
	if ( mKeyBlocked )
		return false;
	::GetKeyboardState( mKeyState );
	return true;
}

void SimpleController::setKey( unsigned cID , ControlAction action , unsigned key )
{
	assert( 0 <= cID && cID < MaxControlerNum );
	actionKey[ action ].keyChar[ cID ] = key;
}

bool SimpleController::checkKey( unsigned key )
{
	return mKeyState[ key ] > 1;
}

bool SimpleController::peekKey( uint8 k , uint8 keySen , uint8 sen )
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

bool SimpleController::checkKey( uint8 k , uint8& keySen , uint8 sen )
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

bool SimpleController::checkKey( unsigned key , uint8 sen )
{
	return checkKey( key , mKeySen[ key ] , sen );
}

bool SimpleController::checkKey( unsigned cID , ControlAction action )
{
	ActionKey& key = actionKey[ action ];
	char keyChar = key.keyChar[ cID ];
	return checkKey( keyChar , key.senCur[ cID ] , key.sen );
}

bool SimpleController::checkActionKey( ActionParam& param )
{
	unsigned cID = mCMap[ param.port ];

	if ( cID == -1 )
		return false;

	ActionKey& key = actionKey[ param.act ];
	char keyChar = key.keyChar[ cID ];

	if ( param.bePeek )
		return peekKey( keyChar , mKeySen[ keyChar ] , key.sen );
	else
		return checkKey( keyChar , mKeySen[ keyChar ] , key.sen );
}

void SimpleController::recvMouseMsg( MouseMsg const& msg )
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
		mEventMask |= BIT( button );
		mMouseEvt[ button ] = msg;
	}

	mLastMousePos = msg.getPos();
}

void SimpleController::removeMouseEvent( int evt )
{
	mEventMask &= ~evt;
}

MouseMsg* SimpleController::getMouseEvent( int evt )
{
	if ( mEventMask & BIT(evt ) )
		return &mMouseEvt[ evt ];
	return NULL;
}

void SimpleController::setPortControl( unsigned port , unsigned cID )
{
	assert( 0 <= cID && cID < MaxControlerNum );
	assert( 0 <= port && port < MAX_PLAYER_NUM );
	if ( port == ERROR_ACTION_PORT )
		return;

	mCMap[ port ] = cID;
}

char SimpleController::getKey( unsigned cID , ControlAction action )
{
	assert( 0 <= cID && cID < MaxControlerNum );
	return actionKey[ action ].keyChar[ cID ];
}

void SimpleController::clearFrameInput()
{
	mEventMask = 0;
}

void SimpleController::clearAllKey()
{

}
