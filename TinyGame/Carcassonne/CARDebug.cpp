#include "CAR_PCH.h"
#include "CARDebug.h"

#include "LogSystem.h"
#include <cstdarg>

namespace CAR
{
	void Log( char const* format , ... )
	{
		va_list argptr;
		va_start( argptr, format );
		::Msg( format , argptr );
		va_end( argptr );
	}


}//namespace CAR