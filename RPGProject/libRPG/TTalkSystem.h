#ifndef TTalkSystem_h__
#define TTalkSystem_h__

#include "Singleton.h"
#include "Thinkable.h"

class TNPCBase;


enum TalkState
{
	TALK_SPEAKING ,
	TALK_SECTION_END ,
	TALK_CONCENT_END ,
};

struct TalkContent;
class CNPCBase;

class TTalkSystem : public SingletonT<TTalkSystem>
	              , public Thinkable
{
public:
	TTalkSystem();

	void updateFrame();

	bool speak( CNPCBase* npc , int flag = 0 );
	void speakNextSection();

	TalkContent* sreachTalkTable( CNPCBase* speaker , int flag = 0 );
	TalkState    getTalkState() const { return m_talkState; }

private:

	void   speakDelayThink( ThinkContent& content );

	void  talkEnd();

	TalkState    m_talkState;
	char         m_talkBuffer[ 512 ];
	char*        m_ptrFill;
	float        m_fillWordNum;

	// ( word / frame );
	float        m_talkSpeed;
	int          m_curTalkStrLen;
	int          m_curSectionStrLen;

	CNPCBase*  m_curSpeaker;
	int          m_curSection;
	TalkContent* m_curContent;

};

class TTalkStringParser
{
	TTalkStringParser()
	{

	}

	void updateFrame()
	{

	}

	void setString( char const* str )
	{
		showAllword = false;
	}


	void speakToEnd( TEvent& event )
	{
		showAllword = true;
	}

	char const* parseStr;
	int  showWordSpeed;
	bool showAllword;
};


#endif // TTalkSystem_h__