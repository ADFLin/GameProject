#include "CometBase.h"

#include <cstdlib>

namespace Comet
{
	class UniformRandom : public IRandom
	{
	public:
		//virtual 
		void   setSeed( uint32 seed )
		{
			::srand( seed );
		}
		Real   getReal()
		{
			return Real( ::rand() ) / ( RAND_MAX );
		}
		int    getInt()
		{
			return ::rand();
		}
	};

}//namespace Comet
