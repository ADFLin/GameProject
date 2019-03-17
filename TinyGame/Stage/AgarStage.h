#include "StageBase.h"

#include "GameGlobal.h"
#include "DrawEngine.h"

namespace Agar
{

	class TestStage : public StageBase
	{
		typedef StageBase BaseClass;
	public:
		TestStage(){}

		virtual bool onInit()
		{
			if ( !::Global::GetDrawEngine().startOpenGL() )
				return false;

			::Global::GUI().cleanupWidget();
			restart();
			return true;
		}

		virtual void onEnd()
		{

			::Global::GetDrawEngine().stopOpenGL();
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

		bool onKey( unsigned key , bool isDown )
		{
			if ( !isDown )
				return false;

			switch( key )
			{
			case Keyboard::eR: restart(); break;
			}
			return false;
		}

	protected:

	};
}