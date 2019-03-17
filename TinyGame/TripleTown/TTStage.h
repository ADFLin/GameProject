#ifndef TTStage_h__
#define TTStage_h__

#include "GameStage.h"

#include "TTLevel.h"
#include "TTScene.h"

#include "StageRegister.h"

#include "Widget/WidgetUtility.h"

namespace TripleTown
{

	class LevelStage : public StageBase
	{
		typedef StageBase BaseClass;
	public:
		virtual bool onInit();
		virtual void onEnd()
		{
			::Global::GetDrawEngine().stopOpenGL();
		}
		virtual void onRestart( bool beInit )
		{
			if ( beInit )
			{
				mLevel.create( LT_LAKE );
			}
			mLevel.restart();
		}
		virtual void onRender( float dFrame )
		{
			GameWindow& window = ::Global::GetDrawEngine().getWindow();
			mScene.render();
		}

		virtual bool onMouse( MouseMsg const& msg )
		{
			mScene.setLastMousePos( msg.getPos() );
			if ( msg.onLeftDown() )
			{
				mScene.click( msg.getPos() );
				return false;
			}
			else if ( msg.onMoving() )
			{
				mScene.peekObject( msg.getPos() );
			}
			return true;
		}

		virtual bool onKey( unsigned key , bool isDown )
		{
			if ( !isDown )
				return true;

			switch( key )
			{
			case 'S': mLevel.setQueueObject( OBJ_BEAR ); return false;
			case 'A': mLevel.setQueueObject( OBJ_GRASS ); return false;
			case 'Q': mLevel.setQueueObject( OBJ_CRYSTAL ); return false;
			case 'W': mLevel.setQueueObject( OBJ_ROBOT ); return false;
			}
			return true;
		}

		virtual void onUpdate( long time )
		{
			BaseClass::onUpdate( time );

			int frame = time / gDefaultTickTime;
			for( int i = 0 ; i < frame ; ++i )
				tick();
			updateFrame( frame );
		}
		virtual void tick()
		{

		}
		virtual void updateFrame( int frame )
		{
			mScene.updateFrame( frame );
		}

		Scene mScene;
		Level mLevel;
	};


}


REGISTER_STAGE("Triple Town Test", TripleTown::LevelStage, EStageGroup::Dev);

#endif // TTStage_h__
