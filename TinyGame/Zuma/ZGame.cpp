#include "TinyGamePCH.h"
#include "ZGame.h"

#include "DrawEngine.h"
#include "WinGLPlatform.h"
#include "GameStage.h"
#include "RenderUtility.h"

#include "ZumaCore/ZumaGame.h"
#include "ZumaCore/ZDraw.h"
#include "ZumaCore/ZEvent.h"

#include "GameWidget.h"
#include "GameGUISystem.h"


#define ZUMA_USE_RHI 1

#if ZUMA_USE_RHI
#include "ZRenderSystemRHI.h"
#else
#include "WGLContext.h"
#include "ZumaCore/GLRenderSystem.h"
#endif

namespace Zuma
{
	EXPORT_GAME_MODULE(GameModule)

	void ZDevStage::onRender( IRenderSystem& RDSystem )
	{
		ZObject ball( zWhite );

		ZRenderUtility::drawLevelBG( RDSystem , lvInfo );
		RDSystem.enableBlend( true );

		RDSystem.setBlendFun( BLEND_ONE , BLEND_ONE );

		glLineWidth( 5 );

		for( int i = 0; i < splineVec.size(); ++i )
		{
			CRSpline2D& spline = splineVec[i];

			Vector2 pos = spline.getPoint( 0 );

			glBegin( GL_LINE_STRIP );
			glVertex3f( pos.x , pos.y  , 0 );

			for ( int i = 1 ; i <= 20 ; ++i )
			{
				Vector2 pos = spline.getPoint( 0.05f * i );
				glVertex3f( pos.x , pos.y  , 0 );
			}
			glEnd();
		}

		glDisable(GL_BLEND);				

		for( int i = 0; i < vtxVec.size(); ++i )
		{
			Vector2& pt = vtxVec[i].pos;
			ball.setPos( pt );

			if ( vtxVec[i].flag &  CurveVertex::eMask )
				ball.setColor( zRed );
			else
				ball.setColor( zWhite );

			ZRenderUtility::drawBall( RDSystem , ball , 0 , TOOL_NORMAL );
		}
	}


	class ProxyStage : public StageBase
		             , public ZEventHandler
					 , public GameInitializer
	{
		typedef StageBase BaseClass;
	public:
		
		//GameInitializer
		virtual bool setupWindow( char const* title , int w , int h )
		{
			DrawEngine& drawEngine = ::Global::GetDrawEngine();
			mOldSize = drawEngine.getScreenSize();
			drawEngine.changeScreenSize( w , h );
			

			return true;
		}

		virtual IRenderSystem* createRenderSystem()
		{
			DrawEngine& drawEngine = ::Global::GetDrawEngine();
			GameWindow& window = drawEngine.getWindow();

#if ZUMA_USE_RHI
			if (!drawEngine.startupSystem(ERenderSystem::D3D11))
			{
				return nullptr;
			}
			RenderSystemRHI* renderSys = new RenderSystemRHI;
			renderSys->init(::Global::GetRHIGraphics2D());
#else
			//HDC hDC = ::Global::getGraphics2D().getTargetDC();
			HDC hDC = window.getHDC();
			if (!drawEngine.startupSystem(ERenderSystem::OpenGL))
			{
				return nullptr;
			}
			GLRenderSystem* renderSys = new GLRenderSystem;
			renderSys->mNeedSweepBuffer = false;
			renderSys->init( window.getHWnd(), hDC , drawEngine.getGLContext()->getHandle() );
#endif

			return renderSys;
		}
		//~GameInitializer

		virtual bool onInit()
		{ 
			::Global::GUI().cleanupWidget();
			if ( !BaseClass::onInit() )
				return false;

			mGameCore->resManager.setWorkDir( "Zuma/" );
			mGameCore->setParentHandler( this );

			{
				TIME_SCOPE("GameCore Init");
				if (!mGameCore->init(*this))
					return false;
			}

			LoadingStage* stage = new LoadingStage( "LoadingThread" );
			stage->audioPlayer = &mGameCore->audioPlayer;
			mGameCore->changeStage( stage );

			return true; 
		}

		virtual void onEnd()
		{
			mGameCore->cleanup();

			::Global::GetDrawEngine().shutdownSystem();
			::Global::GetDrawEngine().changeScreenSize( mOldSize.x , mOldSize.y );

			BaseClass::onEnd();

		}
		virtual void onUpdate( long time ){ mGameCore->update( time ); }
		virtual void onRender( float dFrame )
		{ 
			mGameCore->render( dFrame ); 
		}
		virtual MsgReply onChar( unsigned code ){ return MsgReply::Handled(); }
		virtual MsgReply onMouse( MouseMsg const& msg )
		{ 
			return mGameCore->onMouse( msg ); 
		}

		virtual MsgReply onKey(KeyMsg const& msg)
		{ 
			return mGameCore->onKey(msg);
		}

		virtual bool onEvent( int evtID , ZEventData const& data , ZEventHandler* sender )
		{ 
			switch( evtID )
			{
			case EVT_UI_BUTTON_CLICK:
				{
					ZWidget* ui = data.getPtr< ZWidget >();
					if ( ui->getUIID() == UI_QUIT_GAME )
					{
						getManager()->changeStage( ::STAGE_MAIN_MENU );
					}
				}
				break;
			}
			return true; 
		}

		Vec2i     mOldSize;
		GameCore* mGameCore;
	};

	bool GameModule::queryAttribute( GameAttribute& value )
	{ 
		switch ( value.id )
		{
		case ATTR_SINGLE_SUPPORT:
#if 1
			value.iVal = true;
#else 
			value.iVal = false;
#endif
			return true;
		case ATTR_NET_SUPPORT:
		case ATTR_INPUT_DEFUAULT_SETTING:
			return false;
		case ATTR_TICK_TIME:
			value.iVal = g_GameTime.updateTime;
			return true;
		}
		return false;
	}

	StageBase* GameModule::createStage( unsigned id )
	{
		switch( id )
		{
		case STAGE_SINGLE_GAME:
			{
				ProxyStage* stage = new ProxyStage;
				stage->mGameCore = mCore;
				return stage;
			}
			break;
		}

		return NULL;
	}

	GameModule::GameModule()
	{
		mCore = NULL;
	}

	void GameModule::beginPlay( StageManager& manger, EGameStageMode modeType )
	{		
		if( mCore == NULL )
		{
			mCore = new GameCore;
		}

		changeDefaultStage(manger, modeType);
	}

	void GameModule::enter()
    {

	}

	void GameModule::exit()
	{
		delete mCore;
		mCore = NULL;
	}

}
