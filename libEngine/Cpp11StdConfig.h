#ifndef Cpp11StdConfig_h__
#define Cpp11StdConfig_h__

#include "CppVersion.h"
#include "PlatformConfig.h"

#if !CPP_C11_STDLIB_SUPPORT
#include "boost/bind/placeholders.hpp"
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
#endif //!CPP_C11_STDLIB_SUPPORT

#endif // Cpp11StdConfig_h__
