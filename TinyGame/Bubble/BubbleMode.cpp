#include "BubblePCH.h"
#include "BubbleMode.h"

#include "BubbleScene.h"
#include "GameControl.h"

namespace Bubble
{

	PlayerData::PlayerData( Mode* mode , unsigned id ) 
		:mMode( mode )
		,mId( id )
	{
		mScene = new Scene( this );
	}

	void PlayerData::onRemoveShootBubble( Bubble* bubble )
	{
		Event event;
		event.id     = BU_EVT_REMOVE_SHOOT_BUBBLE;
		event.bubble = bubble;
		mMode->onLevelEvent( *this , event );
		mScene->onRemoveShootBubble( bubble );
	}

	void PlayerData::onUpdateShootBubble( Bubble* bubble , unsigned result )
	{
		Event event;
		event.id     = BU_EVT_UPDATE_SHOOT_BUBBLE;
		event.bubble = bubble;
		event.result = result;
		mMode->onLevelEvent( *this , event );
		mScene->onUpdateShootBubble( bubble , result );
	}

	void PlayerData::onPopCell( BubbleCell& cell , int index )
	{
		Event event;
		event.id    = BU_EVT_POP_BUBBLE;
		event.cell  = &cell;
		event.index = index;
		mMode->onLevelEvent( *this , event );
		mScene->onPopCell( cell , index );
	}

	void PlayerData::onAddFallCell( BubbleCell& cell , int index )
	{
		Event event;
		event.id    = BU_EVT_ADD_FALL_BUBBLE;
		event.cell  = &cell;
		event.index = index;
		mMode->onLevelEvent( *this , event );
		mScene->onAddFallCell( cell , index );
	}

	void PlayerDataManager::tick()
	{
		mMode->tick();

		Event event;
		event.id = BU_EVT_TICK;

		for( PlayerDataVec::iterator iter = mPlayerDataVec.begin();
			iter != mPlayerDataVec.end() ; ++iter )
		{
			PlayerData* data = *iter;
			data->getScene().tick();
			mMode->onLevelEvent( *data , event );
		}
	}

	void PlayerDataManager::updateFrame( int frame )
	{
		for( PlayerDataVec::iterator iter = mPlayerDataVec.begin();
			iter != mPlayerDataVec.end() ; ++iter )
		{
			PlayerData* data = *iter;
			data->getScene().updateFrame( frame );
		}
	}

	void PlayerDataManager::fireAction( ActionTrigger& trigger )
	{
		for( PlayerDataVec::iterator iter = mPlayerDataVec.begin();
			iter != mPlayerDataVec.end() ; ++iter )
		{
			PlayerData* data = *iter;
			trigger.setPort( data->getId() );
			data->getScene().fireAction( trigger );
		}
	}

	PlayerData* PlayerDataManager::createData()
	{
		assert( mMode );
		PlayerData* data = mMode->createPlayerData( mPlayerDataVec.size() );
		assert( data );
		mPlayerDataVec.push_back( data );
		return data;
	}

	void PlayerDataManager::render( Graphics2D& g )
	{
		for( PlayerDataVec::iterator iter = mPlayerDataVec.begin();
			iter != mPlayerDataVec.end() ; ++iter )
		{
			PlayerData* data = *iter;
			data->getScene().render( g );
		}
	}

	void PlayerDataManager::firePlayerAction( ActionTrigger& trigger )
	{
		PlayerData* data = mPlayerDataVec[ trigger.getPort() ];
		data->getScene().fireAction( trigger );
	}

}//namespace Bubble