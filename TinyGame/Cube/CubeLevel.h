#ifndef CubeLevel_h__
#define CubeLevel_h__

#include "CubeBase.h"

namespace Cube
{
	class Level
	{
	public:
		Level();
		~Level();

		static void init();
		void    setupWorld();

		World&   getWorld(){ return *mWorld; }
	private:
		World* mWorld;
	};

}//namespace Cube

#endif // CubeLevel_h__
