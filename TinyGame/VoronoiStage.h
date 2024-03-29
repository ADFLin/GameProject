#ifndef VoronoiStage_h__
#define VoronoiStage_h__

#include "StageBase.h"
#include "GameGlobal.h"

#include "Voronoi.h"

namespace Voronoi 
{
	class TestStage : public StageBase
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

		MsgReply onMouse( MouseMsg const& msg )
		{
			return BaseClass::onMouse(msg);
		}

		MsgReply onKey(KeyMsg const& msg)
		{
			if (msg.isDown())
			{
				switch (msg.getCode())
				{
				case EKeyCode::R: restart(); break;
				}
			}
			return BaseClass::onKey(msg);
		}

	protected:
		Solver mSolver;

	};


}//namespace Voronoi



#endif // VoronoiStage_h__
