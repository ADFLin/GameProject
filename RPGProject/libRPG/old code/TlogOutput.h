#ifndef TlogOutput_h__
#define TlogOutput_h__

#include "debug.h"

class TLogOutput : public TMsgProxy
{
public:
	TLogOutput()
	{
		lastStrPos = 0;
		showLineNum = 40;
		setCurrentOutput( MSG_NORMAL );
	}

	void receive( char const* str )
	{
		--lastStrPos;
		if ( lastStrPos < 0 )
			lastStrPos += MaxLineNum;

		strArray[lastStrPos] = str;
	}

	void show( TMessageShow& msgShow )
	{
		for( int i = 0 ; i < showLineNum ; ++ i)
		{
			int pos = lastStrPos + i;
			if ( pos >= MaxLineNum )
				pos-=MaxLineNum;

			msgShow.pushStr( strArray[pos].c_str() );
		}

	}


	static int const MaxLineNum = 40;
	int     showLineNum;
	int     lastStrPos;
	TString strArray[MaxLineNum];
};
#endif // TlogOutput_h__