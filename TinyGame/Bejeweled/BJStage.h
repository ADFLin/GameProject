#ifndef BJStage_h__
#define BJStage_h__

#include "BJScene.h"
#include "StageBase.h"

namespace Bejeweled
{
	class TestStage : public StageBase
	{
		typedef StageBase BaseClass;
	public:
		TestStage(){}

		bool onInit();
		void onUpdate( long time );
		void onRender( float dFrame );

		void restart();
		void tick();
		void updateFrame( int frame );

		bool onMouse( MouseMsg const& msg );
		bool onKey(KeyMsg const& msg);
	protected:
		Scene mScene;
	};

}




#endif // BJStage_h__
