#ifndef Cpp11StdConfig_h__
#define Cpp11StdConfig_h__

#include "CppVersion.h"
#include "CompilerConfig.h"

#if !CPP_11_STDLIB_SUPPORT
#	if 0//CPP_TR1_SUPPORT

namespace std { namespace tr1 {} }
namespace std 
{
	using namespace ::std::tr1;
}
#	else
#include "boost/bind/placeholders.hpp"
#include "boost/function.hpp"
namespace boost {}
namespace std
{
	using namespace ::boost;

	namespace placeholders
	{
		using ::_1;using ::_2;using ::_3;using ::_4;using ::_5;
		using ::_6;using ::_7;using ::_8;using ::_9;
	}
}
#	endif
#endif //!CPP_C11_STDLIB_SUPPORT

#endif // Cpp11StdConfig_h__
