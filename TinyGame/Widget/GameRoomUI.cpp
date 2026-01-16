#include "TinyGamePCH.h"
#include "GameRoomUI.h"
#include "GameServer.h"

#include "GameWidgetID.h"
#include "GameGUISystem.h"

#include "GameNetPacket.h"
#include "GameWorker.h"
#include "RenderUtility.h"

SlotFrame::SlotFrame( Slot& slot , Vec2i const& pos , Vec2i const& size , PlayerListPanel* parent ) 
	:BaseClass( UI_PLAYER_SLOT , pos , size , parent )
	,mSlot( slot )
	,mCanDrag( false )
{
	setRenderType( eRectType );

	mStateChoice = new GChoice( UI_PLAYER_SLOT ,  Vec2i( 30 , 2 ) , Vec2i( 100 , 25 ) , this );
	mStateChoice->setAlignment(EHorizontalAlign::Left);
	mStateChoice->setUserData( intptr_t(&slot) );
	mStateChoice->addItem( LOCTEXT("Open") ); //0
	mStateChoice->addItem( LOCTEXT("Close") );//1
	mStateChoice->setSelection( 0 );

	slot.frame = this;
	slot.choice = mStateChoice;

}

MsgReply SlotFrame::onMouseMsg( MouseMsg const& msg )
{
	if ( getSlot().state == SLOT_PLAYER )
	{
		Vec2i oldPos = getPos();
		BaseClass::onMouseMsg( msg );
		if ( mCanDrag )
		{
			oldPos.y = getPos().y;
			setPos( oldPos );
			static_cast< PlayerListPanel* >( getParent() )->procSlotFrameMouseMsg( this , msg );
		}
		else
		{
			setPos( oldPos );
		}

	}
	return MsgReply::Handled();
}

void SlotFrame::onRender()
{
	IGraphics2D& g = Global::GetIGraphics2D();
	Vec2i pos = getWorldPos();
	Vec2i size = getSize();

	// Draw Slot Background with Border
	g.setPen(Color3ub(45, 140, 180), 1);
	g.setBrush(Color3ub(30, 32, 36)); 
	g.drawRoundRect(pos, size, Vec2i(6, 6));

	// Draw only Slot Index (1, 2, 3...) at the far left
	RenderUtility::SetFont(g, FONT_S10);
	g.setTextColor(Color3ub(120, 120, 120)); // Muted Gray
	char idxStr[8];
	sprintf(idxStr, "%d", (int)getSlot().id + 1);
	g.drawText(pos + Vec2i(10, 5), idxStr);
}


Vec2i const PlayerListPanel::WidgetSize( 400 , 300 );


PlayerListPanel::PlayerListPanel( IPlayerManager* manager ,int _id , Vec2i const& pos  , GWidget* parent ) 
	:GPanel( _id , pos , WidgetSize , parent )
{
	mManager = manager;
	mDragFrame = NULL;
	mSwapFrame = NULL;
}

void PlayerListPanel::init( bool haveServer )
{
	mSlotNum = MAX_PLAYER_NUM;

	for ( int i = 0 ; i < MAX_PLAYER_NUM ; ++ i )
	{
		Slot& slot = mSlotStorage[i];
		mPlayerSlots[i] = mSlotStorage + i;
		slot.idxData = i;
		slot.id      = SlotId( i );
		slot.state   = SLOT_OPEN;
		slot.info    = NULL;

		Vec2i pos = calcSlotFramePos( slot.id );
		SlotFrame* frame = new SlotFrame( slot ,  pos , Vec2i( 300 , 30 ) , this );
		frame->getStateChioce()->enable( haveServer );
		frame->enableDrag( haveServer );
	}
}

SlotId PlayerListPanel::addPlayer( PlayerInfo const& info )
{
	SlotId result = findFreeSlot();

	if ( result != ERROR_SLOT_ID )
	{
		Slot& slot = getSlot( result );
		slot.info  = &info;
		slot.state = SLOT_PLAYER;
		unsigned p = slot.frame->getStateChioce()->addItem( info.name );
		slot.choice->setSelection( p );
	}

	return result;
}

bool PlayerListPanel::onChildEvent( int event , int id , GWidget* ui )
{

	return true;
}

void PlayerListPanel::setSlot( int index , char const* name )
{

}

void PlayerListPanel::onUpdateUI()
{

}


void PlayerListPanel::refreshPlayerList( int* idx , int* state )
{

	for ( int i = 0 ; i < MAX_PLAYER_NUM ; ++ i )
	{
		mPlayerSlots[ i ] = mSlotStorage + idx[i];
		Slot& slot = *mPlayerSlots[i];
		SlotId id = SlotId(i);
		if ( slot.id != id )
		{
			addSlotFrameAnim( slot.frame , id );
			slot.id = id;
		}
	

		if ( state[i] >= 0 )
		{
			if ( slot.state != SLOT_PLAYER )
			{
				GamePlayer* player = mManager->getPlayer( PlayerId( state[i]) );
				slot.state = SLOT_PLAYER;
				slot.info  = &player->getInfo();
				slot.choice->addItem( slot.info->name );
			}

			slot.frame->getStateChioce()->setSelection( 2 );
		}
		else
		{

			if ( slot.state == SLOT_PLAYER )
			{
				slot.info  = NULL;
				slot.frame->getStateChioce()->removeItem( 2 );
			}

			slot.state = SlotState( state[i] );
			switch( state[i] )
			{
			case SLOT_OPEN:     
				slot.frame->getStateChioce()->setSelection( OpenSelection );
				break;
			case SLOT_CLOSE:
				slot.frame->getStateChioce()->setSelection( CloseSelection );
				break;
			}
		}
	}
}

void PlayerListPanel::setupSlotNum( int num )
{
	for( int i = 0 ; i < MAX_PLAYER_NUM ; ++i )
	{
		Slot* slot = mPlayerSlots[i];
		if ( i < num )
		{
			if ( slot->state == SLOT_DISABLE )
				slot->state = SLOT_OPEN;
			slot->frame->show( true );
		}
		else
		{
			//if ( slot->state != SLOT_PLAYER );
			slot->state = SLOT_DISABLE;
			slot->frame->show( false );
		}
	}
	mSlotNum = num;
}

void PlayerListPanel::setupPlayerList( SPPlayerStatus& status )
{
	for( int i = 0 ; i < status.numPlayer ; ++i )
	{
		Slot&  slot = getSlot( status.info[i]->slot );
		if ( status.flags[i] & ServerPlayer::eReady )
			slot.choice->setColorName( EColor::Green );
	}
}

SlotId PlayerListPanel::findFreeSlot()
{
	for( int i = 0 ; i < MAX_PLAYER_NUM ; ++i )
	{
		Slot& slot = getSlot( SlotId(i) );
		if ( slot.state == SLOT_OPEN )
		{
			assert( slot.info == NULL );
			return SlotId(i);
		}
	}
	return ERROR_SLOT_ID;
}

int PlayerListPanel::getSlotState( int* idx , int* state )
{
	for( int i = 0 ; i < MAX_PLAYER_NUM ; ++i )
	{
		Slot& slot = getSlot( SlotId(i) );
		idx[i] = slot.idxData;

		if ( slot.state == SLOT_PLAYER )
		{
			assert( slot.info );
			state[i] = (int)slot.info->playerId;
		}
		else
		{
			state[i] = (int)slot.state;
		}
	}
	return MAX_PLAYER_NUM;
}

void PlayerListPanel::addSlotFrameAnim( SlotFrame* frame , SlotId destId )
{
	Vec2i pos = calcSlotFramePos( destId );
	long time = abs( pos.y - frame->getPos().y ) * 2;

	//::Global::GUI().addMotion< Easing::OBounce >(frame, frame->getPos(), pos, time);
	UIMotionTask* task = new UIMotionTask( frame , pos , time , WMT_LINE_MOTION );
	::Global::GUI().addTask( task , false );
}

void PlayerListPanel::procSlotFrameMouseMsg( SlotFrame* frame , MouseMsg const& msg )
{
	if ( msg.onLeftDown() )
	{
		mDragFrame = frame;
		setTop();
		frame->setTop();
		//::Global::GUI().getManager().captureMouse( mDragFrame );
	}
	else if ( msg.onLeftUp() )
	{
		if ( mSwapFrame )
		{
			addSlotFrameAnim( mDragFrame , mSwapFrame->getSlot().id );

			int idx1 = (int)mSwapFrame->getSlot().id;
			int idx2 = (int)mDragFrame->getSlot().id;

			std::swap( mPlayerSlots[ idx1 ] , mPlayerSlots[ idx2 ] );
			std::swap( mSwapFrame->getSlot().id , mDragFrame->getSlot().id );
			//::Global::GUI().getManager().releaseMouse();
			sendEvent( EVT_SLOT_CHANGE );
		}
		else if ( mDragFrame )
		{
			addSlotFrameAnim( mDragFrame , mDragFrame->getSlot().id );
		}

		mDragFrame = NULL;
		mSwapFrame = NULL;
	}
	else if ( msg.isLeftDown() && msg.onMoving() )
	{
		LogMsg("mouse Pos = %d %d", msg.getPos().x, msg.getPos().y);
		SlotId swapId = calcSoltId( frame->getPos().y );

		if ( swapId != ERROR_SLOT_ID )
		{
			if ( mSwapFrame && mSwapFrame->getSlot().id != swapId )
			{
				addSlotFrameAnim( mSwapFrame , mSwapFrame->getSlot().id );

				mSwapFrame = NULL;
			}

			if ( !mSwapFrame && mDragFrame->getSlot().id != swapId )
			{
				mSwapFrame = getSlot( swapId ).frame;
				assert( mSwapFrame->getSlot().id == swapId );
				addSlotFrameAnim( mSwapFrame , mDragFrame->getSlot().id );
			}
		}
	}
}

void PlayerListPanel::getSwapSlot( SlotId swapId[2] )
{
	assert( mSwapFrame && mDragFrame );
	swapId[0] = mDragFrame->getSlot().id;
	swapId[1] = mSwapFrame->getSlot().id;
}

void PlayerListPanel::swapSlot( SlotId s1 , SlotId s2 )
{
	Slot* slot1 = mPlayerSlots[ s1 ];
	Slot* slot2 = mPlayerSlots[ s2 ];

	addSlotFrameAnim( slot1->frame , s2 );
	addSlotFrameAnim( slot2->frame , s1 );

	slot1->id = s2;
	slot2->id = s1;

	mPlayerSlots[ s1 ] = slot2;
	mPlayerSlots[ s2 ] = slot1;
}

PlayerId PlayerListPanel::emptySlot( SlotId id , SlotState state )
{
	assert( state != SLOT_PLAYER );

	PlayerId playerId = ERROR_PLAYER_ID;

	Slot& slot = getSlot( id );
	if ( slot.state == SLOT_PLAYER )
	{
		playerId = slot.info->playerId;
		slot.info  = NULL;
		slot.frame->getStateChioce()->removeItem( 2 );
	}

	slot.state = state;
	return playerId;
}

void PlayerListPanel::onRender()
{
	IGraphics2D& g = Global::GetIGraphics2D();
	Vec2i pos = getWorldPos();
	Vec2i size = getSize();

	// Semitransparent dark background for the panel
	g.setPen(Color3ub(50, 60, 80), 1);
	g.setBrush(Color3ub(20, 25, 30));
	g.drawRoundRect(pos, size, Vec2i(10, 10));
}

ComMsgPanel::ComMsgPanel( int _id , Vec2i const& pos , Vec2i const& size , GWidget* parent ) 
	:GPanel( _id , pos , size , parent )
{
	mWorker = NULL;

	int border = 3;
	Vec2i panelPos( border, border );
	Vec2i panelSize = size - Vec2i( 2 * border + 20 , 3 * border + GTextCtrl::UI_Height );

	int offset = 10;
	Vec2i ctrlPos = panelPos + Vec2i( offset - border , panelSize.y + border );
	Vec2i btnSize = Vec2i( 80 , GTextCtrl::UI_Height );

	int texCtrlLen = size.x - 2 * offset - btnSize.x - 5;
	//GPanel*    panel    = new GPanel( UI_ANY , panelPos , panelSize , this );
	mMsgTextCtrl = new GTextCtrl( UI_MSG_CTRL , ctrlPos ,  texCtrlLen  , this );
	mMsgTextCtrl->setAlignment(EHorizontalAlign::Left);
	GButton* button = new GButton( UI_MSG_CTRL , ctrlPos + Vec2i( texCtrlLen + 5 , 0 ) , btnSize , this );
	button->setTitle( "Send" );
	//GSlider*   slider   = new GSlider( UI_MSG_SLIDER , TVec2D( size.x - 10  , 0 ) , size.y - btnSize.y - 5  , false , this  );

	setRenderCallback( RenderCallBack::Create( this , &ComMsgPanel::renderText ) );
}

bool ComMsgPanel::onChildEvent( int event , int id , GWidget* ui )
{
	switch( id )
	{
	case UI_MSG_CTRL:
		switch( event )
		{
		case EVT_BUTTON_CLICK:
		case EVT_TEXTCTRL_COMMITTED:
			if ( *mMsgTextCtrl->getValue() != '\0' )
			{
				char const* str = mMsgTextCtrl->getValue();
				if ( str[0] == '#' )
				{

				}
				else
				{
					CSPMsg msg;
					msg.type = CSPMsg::ePLAYER;
					msg.playerID = mWorker->getPlayerManager()->getUserID();
					msg.content  = str;
					mWorker->sendTcpCommand( &msg );
				}

				mMsgTextCtrl->clearValue();
			}
			return false;
		}
		break;
	}
	return true;
}

void ComMsgPanel::renderText( GWidget* ui )
{
	IGraphics2D& g = Global::GetIGraphics2D();
	Vec2i p = ui->getWorldPos();
	Vec2i s = ui->getSize();

	// Panel BG
	g.setPen(Color3ub(50, 60, 80), 1);
	g.setBrush(Color3ub(20, 25, 35));
	g.drawRoundRect(p, s, Vec2i(10, 10));

	Vec2i pos = p + Vec2i( 10 , 10 );
	RenderUtility::SetFont( g , FONT_S8 );
	for( MsgList::iterator iter = mMsgList.begin();
		iter != mMsgList.end() ; ++ iter )
	{
		g.setTextColor(iter->color);
		g.drawText( pos , iter->msg.c_str() );
		pos.y += 18;
	}
}

void ComMsgPanel::addMessage( char const* str , Color3ub color )
{
	if ( mMsgList.size() > 11 )
		mMsgList.pop_front();

	mMsgList.push_back( MsgInfo( str , color ) );
}
