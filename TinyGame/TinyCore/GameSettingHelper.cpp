#include "TinyGamePCH.h"
#include "GameSettingHelper.h"

#include "GameSettingPanel.h"
#include "GameServer.h"
#include "GameNetPacket.h"

#include "GameWidgetID.h"
#include "GameInstanceManager.h"
#include "DataSteamBuffer.h"

void NetRoomSettingHelper::addGUIControl( GWidget* ui )
{
	switch( ui->getID() )
	{
	case UI_GAME_SETTING_PANEL:
		{
			mSettingPanel = GUI::CastFast< GameSettingPanel >( ui );
			mSettingPanel->setEventCallback( EvtCallBack( this , &NetRoomSettingHelper::onWidgetEvent ) );
		}
		break;
	case UI_PLAYER_LIST_PANEL:
		{
			mPlayerListPanel = GUI::CastFast< PlayerListPanel >( ui );
		}
		break;
	}
}

NetRoomSettingHelper::NetRoomSettingHelper()
{
	mPlayerListPanel = NULL;
	mSettingPanel = NULL;
	mServer       = NULL;
}

void NetRoomSettingHelper::setMaxPlayerNum( int num )
{
	typedef PlayerListPanel::Slot Slot;
	if ( num > MAX_PLAYER_NUM )
	{
		::Msg( "( %s %s )Number of Slots are less than Max Player Number" , __FILE__ , __LINE__ );
	}

	int numPlayer = 0;
	for( int i = 0 ; i < getPlayerListPanel()->getSlotNum() ; ++i )
	{
		Slot& slot = getPlayerListPanel()->getSlot( i );
		if ( slot.info )
			++numPlayer;
	}


	if ( mServer )
	{
		bool bUpdatePlayerStatus = false;
		bool bUpdatePlayerSlot   = false;

		int check = 0;
		for( int i = num ; i < getPlayerListPanel()->getSlotNum() ; ++i )
		{
			Slot& slot = getPlayerListPanel()->getSlot( i );
			if ( slot.info )
			{
				while( check < num )
				{
					Slot& checkSlot = getPlayerListPanel()->getSlot( check );
					if ( checkSlot.state == SLOT_OPEN )
					{
						getPlayerListPanel()->swapSlot( i , check );
						bUpdatePlayerSlot = true;
						break;
					}
					++check;
				}

				if ( check == num )
				{
					switch( slot.info->type )
					{
					case PT_COMPUTER:
						mServer->kickPlayer( slot.info->playerId );
						break;
					case PT_PLAYER:
						GamePlayer* player = mServer->getPlayerManager()->getPlayer( slot.info->playerId );
						player->getInfo().type = PT_SPECTATORS;
						break;
					}

					bUpdatePlayerStatus = true;
				}
			}
		}

		getPlayerListPanel()->setupSlotNum( num );

		if ( bUpdatePlayerStatus )
			sendPlayerStatusSV();

		if ( bUpdatePlayerSlot )
			sendSlotStateSV();

	}
	else
	{
		getPlayerListPanel()->setupSlotNum( num );
	}
}

void NetRoomSettingHelper::importSetting( DataSteamBuffer& buffer )
{
	int numSlot;
	buffer.take( numSlot );
	getPlayerListPanel()->setupSlotNum( numSlot );
	doImportSetting( buffer );
}

void NetRoomSettingHelper::exportSetting( DataSteamBuffer& buffer )
{
	int numSlot = getPlayerListPanel()->getSlotNum();
	buffer.fill( numSlot );
	doExportSetting( buffer );
}

void NetRoomSettingHelper::sendSlotStateSV()
{
	assert( mServer );
	SPSlotState com;
	getPlayerListPanel()->getSlotState( com.idx , com.state );
	mServer->sendTcpCommand( &com );
}

bool NetRoomSettingHelper::addPlayerSV( PlayerId id )
{
	assert( mServer );

	ServerPlayer* player = mServer->getPlayerManager()->getPlayer( id );

	SlotId slotId = getPlayerListPanel()->addPlayer( player->getInfo() );
	if ( slotId == ERROR_SLOT_ID )
	{
		return false;
		//FIXME
	}

	player->getInfo().slot = slotId;
	sendPlayerStatusSV();
	sendSlotStateSV();
	return true;
}

void NetRoomSettingHelper::sendPlayerStatusSV()
{
	assert( mServer );
	SPPlayerStatus PSCom;
	mServer->generatePlayerStatus( PSCom );
	mServer->sendTcpCommand( &PSCom );
}

void NetRoomSettingHelper::emptySlotSV( SlotId id , SlotState state )
{
	assert( mServer );
	if ( id == ERROR_SLOT_ID )
		return;

	SVPlayerManager* playerManager = mServer->getPlayerManager();

	PlayerInfo const* info = getPlayerListPanel()->getSlot( id ).info;

	if ( info && info->playerId == playerManager->getUserID() )
		return;

	PlayerId playerId = getPlayerListPanel()->emptySlot( id , state );

	if ( playerId != ERROR_PLAYER_ID )
	{
		ServerPlayer* player = playerManager->getPlayer( playerId );

		if ( player )
		{
			if ( player->isNetwork() )
			{
				mServer->kickPlayer( playerId );
			}
			else
			{
				mServer->kickPlayer( playerId );
			}
		}

		sendPlayerStatusSV();
	}

	sendSlotStateSV();
}
