#ifndef TSkillBook_h__
#define TSkillBook_h__

#include "common.h"
#include "TSkill.h"

class TSkillBook
{
public:
	TSkillBook( unsigned castID )
		:m_casterID(castID)
	{

	}

	struct SkillState
	{    
		bool canUse;
		char const*       name;
		SkillInfo const*  info;
	};

	int          getAllSkillName( char const** str , int maxNum );
	int          getSkillLevel( char const* name );
	SkillState*  getSkillState( char const* name );
	bool         haveSkill( char const * name );
	bool         learnSkill( char const* name , int level );
	
	SkillInfo const*  getSkillInfo( char const* name );
protected:

	struct StrCmp
	{
		bool operator()(const char* s1, const char* s2) const
		{
			return strcmp(s1, s2) < 0;
		}
	};

	unsigned m_casterID;
	typedef std::map< char const* , SkillState , StrCmp > SkillMap;
	SkillMap m_skillMap;
	
};

class CDMask;

enum CDType
{
	CD_ITEM  ,
	CD_SKILL ,
};

struct CDInfo
{
	char const* name;
	CDType      type;
	float       cdTime;
};



class TColdDownManager
{
public:
	TColdDownManager( unsigned casterID )
		:m_casterID( casterID )
	{

	}

	
	bool    canPlay( char const* name , CDType type );
	float   getCDTime( char const* name , CDType type );
	CDInfo* getCDInfo( char const* name );

	bool canPlayItem( char const* name )
	{
		return canPlay( name , CD_ITEM );
	}


	bool canPlaySkill( char const* name )
	{
		return canPlay( name , CD_SKILL );
	}

	void    updateFrame();

	void pushItem( char const* name , float cdTime );
	void pushSkill( char const* name , float cdTime );
	void push( char const* name , float cdTime , CDType type );
protected:

	typedef std::list< CDInfo > CDList;
	CDList   m_cdList;
	unsigned m_casterID;
};

#endif // TSkillBook_h__