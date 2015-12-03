#ifndef debug_h__
#define debug_h__

#include <cstdarg>
//#include <cassert>

void Msg(char const* format , ... );
void DevMsg( int level , char const* format , ... );
void WarmingMsg( int level , char const* format , ... );
void ErrorMsg( char const* format , ... );

#ifdef _DEBUG
#define TASSERT( expr , str )\
	if ( !( expr ) ) { assert( str == 0 ); }
#else
#define TASSERT( expr , str )
#endif

class TMsgProxy
{
public:
	TMsgProxy();
	virtual ~TMsgProxy();
	void setLevel( int level ){ m_level = level; }
	int  getLevel() const { return m_level; }
	void setCurrentOutput( int chanel );
	virtual void receive( char const* str ) = 0;

protected:
	friend void SendMsg( TMsgProxy* output , char const* addStr , char const* format ,va_list argptr );
	bool m_showChanelName;
	int  m_level;
};

enum
{
	MSG_NORMAL   = 0,
	MSG_DEV,      
	MSG_WARNING,
	MSG_ERROR,
	MSG_LOG,
	MSG_CHANEL_NUM ,
};



#endif // debug_h__