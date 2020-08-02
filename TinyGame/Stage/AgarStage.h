#include "StageBase.h"

#include "GameGlobal.h"
#include "GameRenderSetup.h"

namespace Agar
{

	class TestStage : public StageBase
		            , public IGameRenderSetup
	{
		typedef StageBase BaseClass;
	public:
		TestStage(){}

		virtual bool onInit()
		{
			::Global::GUI().cleanupWidget();
			restart();
			return true;
		}

		virtual void onEnd()
		{
		}

		virtual void onUpdate( long time )
		{
			BaseClass::onUpdate( time );

			int frame = time / gDefaultTickTime;
			for( int i = 0 ; i < frame ; ++i )
				tick();

			updateFrame( frame );
		}

		void onRender( float dFrame )
		{
			Graphics2D& g = Global::GetGraphics2D();
		}

		void restart()
		{

		}


		void tick()
		{

		}

		void updateFrame( int frame )
		{

		}

		bool onMouse( MouseMsg const& msg )
		{
			if ( !BaseClass::onMouse( msg ) )
				return false;
			return true;
		}

		bool onKey(KeyMsg const& msg)
		{
			if ( !isDown )
				return false;

			switch( key )
			{
			case EKeyCode::R: restart(); break;
			}
			return false;
		}

	protected:

	};
}