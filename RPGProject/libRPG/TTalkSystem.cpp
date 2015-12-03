#include "ProjectPCH.h"
#include "TTalkSystem.h"

#include "ConsoleSystem.h"

#include "UtilityGlobal.h"
#include "EventType.h"


DEFINE_CON_VAR( float , talk_section_wait_time , 3 );

struct TalkContent
{
	int numSection;
	char const** speakStr;
};


char const* talkStr000[] =
{
	" ",
	"歡迎來到這個世界\n",
	"前面有你要的裝備。\n",
};

char const* talkStr001[] = { "這裡有你要的物品，快來購買八!" };
char const* talkStr002[] = { "歡迎下次再來!" };


#define TALK_CONTENT( name )\
	{ ARRAY_SIZE( name ) , name } ,

TalkContent talkContentTable[] =
{
	TALK_CONTENT( talkStr000 )
	TALK_CONTENT( talkStr001 )
	TALK_CONTENT( talkStr002 )
};

TTalkSystem::TTalkSystem()
{
	m_curContent = NULL;
	m_curSpeaker = NULL;
	
	m_talkSpeed = 15.0 / 30;
}
TalkContent* TTalkSystem::sreachTalkTable( CNPCBase* speaker , int flag  )
{
	unsigned npcID = 0;
	return &talkContentTable[ flag ];
}

static bool isDoubleChar( char c )
{ 
	return  ( c & 0x80) ? true : false;
}

void TTalkSystem::updateFrame()
{
	if ( !m_curContent )
		return;

	if ( m_talkState != TALK_SPEAKING )
		return;

	m_fillWordNum += m_talkSpeed;

	int fillCharNum = 0;

	char const* curStr = m_curContent->speakStr[m_curSection];
	while ( m_fillWordNum > 1 && m_curTalkStrLen < m_curSectionStrLen )
	{
		char c = curStr[ m_curTalkStrLen ];

		if ( isDoubleChar(c) )
		{
			(*m_ptrFill++ ) = curStr[ m_curTalkStrLen++ ];
			(*m_ptrFill++ ) = curStr[ m_curTalkStrLen++ ];
		}
		else
		{
			(*m_ptrFill++ ) = curStr[ m_curTalkStrLen++ ];
		}

		m_ptrFill[0] = '\0';
		m_fillWordNum -= 1.0;
		++fillCharNum;

		if ( m_curTalkStrLen >= m_curSectionStrLen  )
		{
			m_talkState = TALK_SECTION_END;
			TEvent event( EVT_TALK_SECTION_END , 0 , m_curSpeaker , &m_curSection );
			UG_SendEvent( event );

			ThinkContent* tc = setupThink( this , &TTalkSystem::speakDelayThink );
			tc->nextTime = g_GlobalVal.curtime + talk_section_wait_time;
		}
	}

	if ( fillCharNum > 0 )
	{
		TEvent event( EVT_TALK_CONCENT , 0 , m_curSpeaker , m_talkBuffer );
		UG_SendEvent( event );
	}

}


void TTalkSystem::speakNextSection()
{
	++m_curSection;
	if ( m_curSection ==  m_curContent->numSection )
	{
		talkEnd();
		return;
	}

	m_curSectionStrLen = strlen( m_curContent->speakStr[ m_curSection ] );
	m_curTalkStrLen = 0;
	m_fillWordNum = 0;
	m_ptrFill = m_talkBuffer;
	m_ptrFill[0] = '\0';
	m_talkState = TALK_SPEAKING;
}

bool TTalkSystem::speak( CNPCBase* npc , int flag )
{
	if ( m_curContent )
	{
		if ( m_curSection ==  m_curContent->numSection - 1 &&
			m_talkState == TALK_SECTION_END )
		{
			talkEnd();
			setupThink( this , &TTalkSystem::speakDelayThink , THINK_REMOVE );
		}

		if ( m_talkState != TALK_CONCENT_END )
			return false;
	}

	unsigned npcID = 1;

	m_curContent = sreachTalkTable( npc , flag );
	
	if ( m_curContent )
	{
		m_curSpeaker = npc;
		m_curSection = -1;
		m_talkState = TALK_SPEAKING;
		speakNextSection();

		TEvent event( EVT_TALK_START , 0 , m_curSpeaker );
		UG_SendEvent( event );

		return true;
	}

	return false;
}

void TTalkSystem::talkEnd()
{
	m_curSpeaker = NULL;
	m_curContent = NULL;
	m_talkState = TALK_CONCENT_END;
	TEvent event( EVT_TALK_END , 0 , m_curSpeaker );
	UG_SendEvent( event );
}

void TTalkSystem::speakDelayThink( ThinkContent& content )
{
	speakNextSection();
}
