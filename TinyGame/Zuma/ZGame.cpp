#include "TinyGamePCH.h"
#include "ZGame.h"

#include "DrawEngine.h"
#include "WinGLPlatform.h"
#include "GameStage.h"
#include "RenderUtility.h"

#include "ZumaCore/ZumaGame.h"
#include "ZumaCore/GLRenderSystem.h"
#include "ZumaCore/ZDraw.h"
#include "ZumaCore/ZEvent.h"

#include "GameWidget.h"
#include "GameGUISystem.h"

namespace Zuma
{
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

			Vec2D pos = spline.getPoint( 0 );

			glBegin( GL_LINE_STRIP );
			glVertex3f( pos.x , pos.y  , 0 );

			for ( int i = 1 ; i <= 20 ; ++i )
			{
				Vec2D pos = spline.getPoint( 0.05f * i );
				glVertex3f( pos.x , pos.y  , 0 );
			}
			glEnd();
		}

		glDisable(GL_BLEND);				

		for( int i = 0; i < vtxVec.size(); ++i )
		{
			Vec2D& pt = vtxVec[i].pos;
			ball.setPos( pt );

			if ( vtxVec[i].flag &  CVData::eMask )
				ball.setColor( zRed );
			else
				ball.setColor( zWhite );

			ZRenderUtility::drawBall( RDSystem , ball , 0 , TOOL_NORMAL );
		}
	}


	class ProxyStage : public StageBase
		             , public ZEventHandler
					 , public GameInitialer
	{
		typedef StageBase BaseClass;
	public:
		

		virtual bool           setupWindow( char const* title , int w , int h )
		{
			DrawEngine* drawEngine = ::Global::getDrawEngine();
			mOldSize = drawEngine->getScreenSize();
			drawEngine->changeScreenSize( w , h );
			

			return true;
		}

		virtual IRenderSystem* createRenderSystem()
		{
			DrawEngine* drawEngine = ::Global::getDrawEngine();
			GameWindow& window = drawEngine->getWindow();

			//HDC hDC = ::Global::getGraphics2D().getTargetDC();
			HDC hDC = window.getHDC();
			drawEngine->startOpenGL( false );

			GLRenderSystem* renderSys = new GLRenderSystem;
			renderSys->mNeedSweepBuffer = false;
			renderSys->init( window.getHWnd(), hDC , drawEngine->getGLContext().getHandle() );

			return renderSys;
		}

		virtual bool onInit()
		{ 
			::Global::getGUI().cleanupWidget();
			if ( !BaseClass::onInit() )
				return false;

			mGameCore->resManager.setWorkDir( "Zuma/" );
			mGameCore->setParentHandler( this );

			if ( !mGameCore->init( *this ) )
				return false;

			LoadingStage* stage = new LoadingStage( "LoadingThread" );
			mGameCore->changeStage( stage );

			return true; 
		}

		virtual void onEnd()
		{
			mGameCore->cleanup();

			::Global::getDrawEngine()->stopOpenGL();
			::Global::getDrawEngine()->changeScreenSize( mOldSize.x , mOldSize.y );

			BaseClass::onEnd();

		}
		virtual void onUpdate( long time ){ mGameCore->update( time ); }
		virtual void onRender( float dFrame )
		{ 
			mGameCore->render( dFrame ); 
		}
		virtual bool onChar( unsigned code ){ return false; }
		virtual bool onMouse( MouseMsg const& msg )
		{ 
			return mGameCore->onMouse( msg ); 
		}

		virtual bool onKey( unsigned key , bool isDown )
		{ 
			return mGameCore->onKey( key , isDown );
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

	bool CGamePackage::getAttribValue( AttribValue& value )
	{ 
		switch ( value.id )
		{
		case ATTR_SINGLE_SUPPORT:
#if 0
			value.iVal = true;
#else 
			value.iVal = false;
#endif
			return true;
		case ATTR_NET_SUPPORT:
		case ATTR_REPLAY_INFO_DATA:
		case ATTR_CONTROLLER_DEFUAULT_SETTING:
			return false;
		case ATTR_TICK_TIME:
			value.iVal = g_GameTime.updateTime;
			return true;
		}
		return false;
	}

	StageBase* CGamePackage::createStage( unsigned id )
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

	CGamePackage::CGamePackage()
	{
		mCore = NULL;
	}

	void CGamePackage::enter( StageManager& manger )
	{
		manger.changeStage( STAGE_SINGLE_GAME );
	}

	bool CGamePackage::load()
	{
		if ( mCore == NULL )
			mCore = new GameCore;
		return true;
	}

	void CGamePackage::release()
	{
		delete mCore;
		mCore = NULL;
	}

}

EXPORT_GAME( Zuma::CGamePackage )