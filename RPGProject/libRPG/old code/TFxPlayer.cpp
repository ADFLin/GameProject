#include "TFxPlayer.h"

#include "FyFX.h"
#include "TEventSystem.h"
#include "TResManager.h"
#include "TEntityManager.h"
#include "EventType.h"
#include "profile.h"

void TFxData::playCycle( int cycle )
{
	flagBit &= ~FX_PLAY_TYPE_MASK;
	flagBit |= FX_CYCLE;
	cycleTime = cycle;
}

void TFxData::playLifeTime( float life )
{
	flagBit &= ~FX_PLAY_TYPE_MASK;
	flagBit |= FX_LIFE_TIME;

	lifeTime = life;
}

void TFxData::playForever()
{
	flagBit &= ~FX_PLAY_TYPE_MASK;
	flagBit |= FX_FOREVER;
}

void TFxData::setNextFx( TFxData* nFx )
{
	nextFx = nFx;
	nextFx->flagBit |= FX_WAIT;
}

FnObject TFxData::getBaseObj()
{
	FnObject obj;
	obj.Object( getFx()->GetBaseObject() );
	return obj;
}

TFxPlayer::TFxPlayer()
{
	setGlobal( true );
	listenEvent( EVT_FX_DELETE , EVENT_ANY_ID , FnMemEvent( &TFxPlayer::cancelFx ) );
	TEntityManager::instance().addEntity( this );
}

TFxPlayer::~TFxPlayer()
{

}

TFxData* TFxPlayer::play( char const* name )
{
	eF3DFX* fx = TResManager::instance().cloneFx( name );
	if ( !fx )
		return NULL;

	TFxData data;
	data.fx   = fx;
	m_fxList.push_back( data );
	return &m_fxList.back();
}

void TFxPlayer::updateFrame()
{
	TPROFILE("Update FX");

	for( std::list< TFxData >::iterator iter = m_fxList.begin();
		iter != m_fxList.end(); )
	{
		if ( iter->flagBit & FX_WAIT )
		{
			++iter;
			continue;
		}

		bool isOk = iter->fx->Play(1);
		bool beD = false;

		switch ( iter->flagBit & FX_PLAY_TYPE_MASK )
		{
		case FX_CYCLE:
			if ( !isOk )
			{
				--iter->cycleTime;
				beD = (iter->cycleTime == 0 );
			}
			break;
		case FX_ONCE:
			beD = !isOk; 
			break;
		case FX_LIFE_TIME:
			iter->lifeTime -= g_GlobalVal.frameTime;
			beD = ( iter->lifeTime <= 0 );
			break;
		}

		if ( beD )
		{
			delete iter->fx;
			if ( iter->nextFx )
			{
				iter->nextFx->flagBit &= ~( FX_WAIT );
			}
			iter = m_fxList.erase( iter );
		}
		else
		{
			if ( !isOk )
				iter->getFx()->Reset();
			++iter;
		}
	}
}

void TFxPlayer::cancelFx()
{
	TEvent& event =TEventSystem::instance().getCurEvent();
	for( std::list< TFxData >::iterator iter = m_fxList.begin();
		iter != m_fxList.end();++iter)
	{
		if ( &(*iter) == event.data )
		{
			delete iter->fx;
			m_fxList.erase( iter );
			return;
		}
	}
}

void TFxPlayer::clear()
{
	for( std::list< TFxData >::iterator iter = m_fxList.begin();
		iter != m_fxList.end();++iter )
	{
		//delete iter->fx;
	}
	m_fxList.clear();
}