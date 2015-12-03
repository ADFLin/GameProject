#include "ProjectPCH.h"
#include "TSkillBook.h"

#include "EventType.h"
#include "UtilityGlobal.h"

TSkillBook::SkillState* TSkillBook::getSkillState( char const* name )
{
	SkillMap::iterator iter = m_skillMap.find( name );

	if ( iter != m_skillMap.end() )
	{
		return &iter->second;
	}
	return NULL;
}

bool TSkillBook::haveSkill( char const * name )
{
	SkillState* state = getSkillState( name );

	return state != NULL;
}

bool TSkillBook::learnSkill( char const* name , int level )
{
	SkillState* state ;
	if ( state = getSkillState( name ))
	{
		if ( state->info->level >= level  )
			return false;

		state->info = TSkillLibrary::getInstance().querySkill( name , level );
		
		TEvent event( EVT_SKILL_LEARN , m_casterID , this , state );
		UG_SendEvent( event );

		return true;
	}
	else if ( SkillInfo const* info = TSkillLibrary::getInstance().querySkill( name , level ) )
	{
		SkillState state;
		state.info  = info;
		state.name  = name;
		m_skillMap.insert( std::make_pair( name , state ) );

		TEvent event( EVT_SKILL_LEARN , m_casterID , this , getSkillState( name )  );
		UG_SendEvent( event );
		return true;
	}
	return false;
}

int TSkillBook::getSkillLevel( char const* name )
{
	if ( SkillState* state = getSkillState(name) )
	{
		return state->info->level;
	}
	return 0;
}

int TSkillBook::getAllSkillName( char const** str , int maxNum )
{
	int num = 0;
	for( SkillMap::iterator iter = m_skillMap.begin();
		iter != m_skillMap.end(); ++iter )
	{
		if ( num > maxNum )
			break;

		str[num] = iter->first;
		++num;
	}
	return num;
}

SkillInfo const* TSkillBook::getSkillInfo( char const* name )
{
	if ( SkillState* state = getSkillState( name ))
		return state->info;

	return 0;
}
bool TColdDownManager::canPlay( char const* name , CDType type )
{
	for( CDList::iterator iter = m_cdList.begin();
		iter != m_cdList.end() ; ++iter )
	{
		CDInfo& state = *iter;
		if ( state.type == type &&  state.name == name )
			return false;

		TASSERT( strcmp( state.name , name ) != 0 ,
			     "Use alloc string In TCDManager" );
	}
	return true;
}

void TColdDownManager::updateFrame()
{
	for( CDList::iterator iter = m_cdList.begin();
		iter != m_cdList.end() ; )
	{
		CDInfo& state = *iter;

		state.cdTime -= g_GlobalVal.frameTime;
		if ( state.cdTime <=0 )
		{
			if ( state.type == CD_SKILL )
			{
				TEvent event( EVT_SKILL_CD_FINISH , m_casterID , NULL , (void*)state.name );
				UG_SendEvent( event );
			}
			else
			{
				TEvent event( EVT_ITEM_CD_FINISH , m_casterID , NULL , (void*)state.name );
				UG_SendEvent( event );
			}

			iter = m_cdList.erase( iter );
		}
		else
		{
			++iter;
		}
	}
}


void TColdDownManager::push( char const* name , float cdTime , CDType type  )
{
	CDInfo state;
	state.type   = type;
	state.cdTime = cdTime;
	state.name   = name;

	m_cdList.push_back( state );
	TEvent event( EVT_PLAY_CD_START , m_casterID , NULL , &m_cdList.back() );
	UG_SendEvent( event );
}
void TColdDownManager::pushItem( char const* name , float cdTime )
{
	push( name , cdTime , CD_ITEM );
}

void TColdDownManager::pushSkill( char const* name , float cdTime )
{
	push( name , cdTime , CD_SKILL );
}

float TColdDownManager::getCDTime( char const* name , CDType type )
{
	for( CDList::iterator iter = m_cdList.begin();
		iter != m_cdList.end() ; ++iter )
	{
		CDInfo& state = *iter;
		if ( state.type == type &&  state.name == name )
			return state.cdTime;

		TASSERT( strcmp( state.name , name ) != 0 ,
			"Use alloc string In TCDManager" );
	}
	return 0;
}

CDInfo* TColdDownManager::getCDInfo( char const* name  )
{
	for( CDList::iterator iter = m_cdList.begin();
		iter != m_cdList.end() ; ++iter )
	{
		CDInfo& state = *iter;
		if ( state.name == name )
			return &state;

		TASSERT( strcmp( state.name , name ) != 0 ,
			"Use alloc string In TCDManager" );
	}
	return NULL;
}