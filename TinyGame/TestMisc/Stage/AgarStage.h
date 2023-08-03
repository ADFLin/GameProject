#include "StageBase.h"

#include "GameGlobal.h"
#include "GameRenderSetup.h"
#include "GameGUISystem.h"



namespace Agar
{


	struct GameParams
	{




	};

	class Cell
	{
	public:
		Cell()
		{

		}


		Vector2 pos;
		Vector2 vel;
		float   mass;

		float getRadius()
		{
			float mu = 1;
			return Math::Sqrt(mass / mu);
		}

		float getLimitedSpeed()
		{

			mass;

		}
		void update(float deltaTime)
		{



		}

		void postUpdate()
		{



		}
	};

	class Player
	{
	public:



		void upate(float deltaTime)
		{




		}

		TArray<Cell> mCellStorage;


		Vector2 targetPos;


	};

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

		MsgReply onMouse( MouseMsg const& msg )
		{
			return BaseClass::onMouse(msg);
		}

		MsgReply onKey(KeyMsg const& msg)
		{
			if ( msg.isDown() )
			{
				switch (msg.getCode())
				{
				case EKeyCode::R: restart(); break;
				}
			}
			return BaseClass::onKey(msg);
		}

	protected:

	};


}

