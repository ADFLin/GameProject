#include "TDPCH.h"
#include "TDController.h"
#include "TDLevel.h"

#define KEY_OP_APPEND  VK_SHIFT

namespace TowerDefend
{
	Controller::Controller()
	{
		mRectSelectStep = STEP_NONE;
		mBlockLeftUp    = false;
		mEnableRectSelect = true;
	}


	void   Controller::clearFrameInput()
	{
		BaseClass::clearFrameInput();

		if ( mRectSelectStep == STEP_LEFT_UP )
			mRectSelectStep = STEP_NONE;

		mUIComID = CID_NULL;
	}

	bool Controller::haveLockMouse()
	{
		return mEnableRectSelect && mRectSelectStep != STEP_NONE;
	}

	bool Controller::scanInput( bool beUpdateFrame )
	{
		if ( !BaseClass::scanInput( beUpdateFrame ) )
			return false;

		MouseMsg* msg = getMouseEvent( LBUTTON );
		if ( msg )
		{
			if ( msg->onLeftDown() )
			{
				if ( mEnableRectSelect )
				{
					mSelectRect.start = msg->getPos();
					mRectSelectStep = STEP_LEFT_DOWN;
				}
			}
			else if ( msg->onLeftUp() )
			{
				if ( mBlockLeftUp )
				{
					removeMouseEvent( LBUTTON );
					mBlockLeftUp = false;
				}
				else
				{
					if ( mEnableRectSelect )
					{
						mSelectRect.end = msg->getPos();
						mRectSelectStep = STEP_LEFT_UP;
					}
				}
			}
		}

		if ( mEnableRectSelect )
		{
			if ( mRectSelectStep == STEP_LEFT_DOWN || mRectSelectStep == STEP_MOVE )
			{
				msg = getMouseEvent( MOVING );
				if ( msg && msg->isLeftDown() )
				{
					mSelectRect.end = msg->getPos();
					mRectSelectStep = STEP_MOVE;
				}
			}
		}
		return true;
	}

	int Controller::scanCom( ComMap& comMap , int idx )
	{
		ComMap* curMap = &comMap;
		while( 1 )
		{
			ComMap::KeyValue& value = curMap->value[idx];
			if ( value.comID != CID_NULL )
			{
				if ( checkKey( value.key , KEY_ONCE ) )
				{
					return value.comID;
				}
			}
			if ( curMap->baseID == COM_MAP_NULL )
				break;

			curMap = &Actor::getComMap( curMap->baseID );
		}
		return 0;
	}
	bool Controller::checkAction( ActionParam& param )
	{
		switch( param.act )
		{
		case ACT_TD_SELECT_UNIT_RANGE:
			{
				if ( mEnableRectSelect && mRectSelectStep == STEP_LEFT_UP )
				{
					CISelectRange& info = param.getResult< CISelectRange >();
					info.rect.start = mSelectRect.start;
					info.rect.end   = mSelectRect.end;
					info.opFlag =  ( checkKey( KEY_OP_APPEND ) ) ? CF_OP_APPEND : 0;
					return true;
				}
			}
			break;
		case ACT_TD_SELECT_UNIT_GROUP:
			{
				MouseMsg* msg = getMouseEvent( LBUTTON );
				if ( msg && msg->onLeftDClick() )
				{
					CIMouse& info = param.getResult< CIMouse >();
					info.pos  = msg->getPos();
					info.opFlag =  ( checkKey( KEY_OP_APPEND ) ) ? CF_OP_APPEND : 0;

					mBlockLeftUp = true;
					return true;
				}
			}
			break;
		case ACT_TD_MOUSE_COM:
			{
				MouseMsg* msg = getMouseEvent( RBUTTON );
				if ( msg && msg->onDown() )
				{
					CIMouse& info = param.getResult< CIMouse >();
					info.pos    = msg->getPos();
					info.opFlag =  ( checkKey( KEY_OP_APPEND ) ) ? CF_OP_APPEND : 0;
					return true;
				}
			}
			break;
		case ACT_TD_TARGET_CHOICE:
			{
				MouseMsg* msg = getMouseEvent( LBUTTON );
				if ( msg && msg->onDown() )
				{
					CIMouse& info = param.getResult< CIMouse >();
					info.pos      = msg->getPos();
					info.opFlag =  ( checkKey( KEY_OP_APPEND ) ) ? CF_OP_APPEND : 0;

					mBlockLeftUp = true;
					return true;
				}
			}
			break;
		case ACT_TD_CHOICE_COM_ID:
			{
				CIComID& cInfo = param.getResult< CIComID >();

				if ( mUIComID != CID_NULL )
				{
					cInfo.chioceID  = mUIComID;
					cInfo.idxComMap = mIdxComMap;
					return true;
				}

				ComMap& comMap = Actor::getComMap( cInfo.comMapID );
				for( int i = 0 ; i < COM_MAP_ELEMENT_NUN ; ++i )
				{
					int id = scanCom( comMap , i );
					if ( id )
					{
						cInfo.chioceID  = id;
						cInfo.idxComMap = i;
						return true;
					}
				}
			}
			break;
		case ACT_TD_CANCEL_COM_MODE:
			{
				MouseMsg* msg = getMouseEvent( RBUTTON );
				if ( msg && msg->onDown() )
				{
					return true;
				}
			}
			break;
		case ACT_TD_VIEWPORT_MOVE:
			{
				float VPScrolSpeed = 10;

				CIViewportMove& move = param.getResult< CIViewportMove >();

			
				bool beOk = false;
				move.offset.setValue( 0 , 0 );
				if ( checkKey( Keyboard::eLEFT , (uint8)0 ) )
				{
					move.mode   = CIViewportMove::eREL_MOVE;
					move.offset.x -= VPScrolSpeed;
					beOk = true;
				}
				else if ( checkKey(Keyboard::eRIGHT , (uint8)0 ) )
				{
					move.mode   = CIViewportMove::eREL_MOVE;
					move.offset.x += VPScrolSpeed;
					beOk = true;
				}
				if ( checkKey( Keyboard::eUP , (uint8)0 ) )
				{
					move.mode   = CIViewportMove::eREL_MOVE;
					move.offset.y -= VPScrolSpeed;
					beOk = true;
				}
				else if ( checkKey(Keyboard::eDOWN , (uint8)0 ) )
				{
					move.mode   = CIViewportMove::eREL_MOVE;
					move.offset.y += VPScrolSpeed;
					beOk = true;
				}
				if ( beOk )
					return true;
			}
			break;
		}
		return false;
	}

	void Controller::enableRectSelect( bool beE )
	{
		if ( mEnableRectSelect == beE )
			return;
		mEnableRectSelect = beE;
		mRectSelectStep   = STEP_NONE;
	}

}//namespace TowerDefend
