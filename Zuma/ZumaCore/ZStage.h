#ifndef ZStage_h__
#define ZStage_h__

#include "ZBase.h"

#include "CRSpline.h"

#include "Thread.h"

#include "ZLevel.h"
#include "ZPath.h"
#include "ZParticle.h"
#include "ZUISystem.h"

#include "../FastDelegate/FastDelegate.h"

class MouseMsg;

namespace Zuma
{

	class IRenderSystem;
	class ResManager;

	struct ZLevelInfo;
	struct ZUserData;
	class  ZStage;


	struct ZUserData
	{
		unsigned unlockStage;
	};


	enum
	{
		SF_DONT_CLEAR_UI   = BIT( 0 ),
		SF_DONT_CLEAR_TASK = BIT( 1 ),
	};

	class ZStageController : public ZEventHandler
		                   , public TaskHandler
	{
	public:
		ZStageController();
		~ZStageController();

		virtual ZUserData&     getUserData() = 0;

		void     update( unsigned time );
		void     changeStage( ZStage* stage , unsigned flag = 0 );
		ZStage*  getCurStage() const { return mCurStage; }

		virtual  void onStageEnd( ZStage* stage , unsigned flag ){}

	protected:
		unsigned m_chFlag;
		ZStage*  m_nextStage;
		ZStage*  mCurStage;
	};


	class ZStage : public ZEventHandler
		         , public TaskHandler
	{
	public:
		ZStage( int stageID ):m_stageID( stageID ){}
		virtual ~ZStage(){}
		virtual bool onUIEvent( int evtID , int uiID , ZWidget* ui ){ return true; }
		virtual void onUpdate( long time ){}
		virtual void onRender( IRenderSystem& RDSystem ){}
		virtual bool onMouse( MouseMsg const& msg ){ return true; }
		virtual void onKey( unsigned key , bool isDown ){}

		virtual void onEnd(){}
		virtual void onStart(){}
		virtual bool onEvent( int evtID , ZEventData const& data , ZEventHandler* sender ){ return true; }

		void    sendEvent( int evtID  , ZEventData const& data){  m_controller->processEvent( evtID , data );  }
		int     getStageID() const { return m_stageID; }

	private:
		int  m_stageID;
		friend class ZStageController;
		ZStageController* m_controller;
	};

	enum
	{
		STAGE_EMPTY ,
		STAGE_ADV_MENU ,
		STAGE_MAIN_MENU ,
		STAGE_LEVEL ,
		STAGE_LOADING ,
		STAGE_DEV ,
	};


	class EmptyStage : public ZStage
	{
	public:
		EmptyStage():ZStage( STAGE_EMPTY ){}
	};

	class ZDevStage : public ZStage
	{
	public:
		ZDevStage( ZLevelInfo& info );
		virtual void onUpdate( long time )
		{

		}



		void  buildCurve( int idx );

		virtual void onRender( IRenderSystem& RDSystem );
		virtual void onKey( unsigned key , bool isDown );
		virtual bool onMouse( MouseMsg const& msg );

		void clearPoint();
		void movePoint( int idx , Vec2f const& dest );
		void addPoint( Vec2f const& pos );
		int  getPoint( Vec2f const& pos );
		void togglePointMask( Vec2f const& pos );

		void buildSpline( CRSpline2D& spline , int idxVtx );

		Vec2f mousePos;
		int   mvIdx;
		int   curCurveIndex;

		ZLevelInfo& lvInfo;
		CVDataVec   vtxVec;
		std::vector< CRSpline2D > splineVec;
	};


	enum LvState
	{
		LVS_PASS_LEVEL ,
		LVS_START_QUAKE ,
		LVS_START_TITLE ,
		LVS_IMPORT_BALL ,
		LVS_PLAYING ,
		LVS_FAIL    ,
		LVS_FINISH  ,
	};


	class LoadingStage : public ZStage
	{
	public:
		LoadingStage( char const* resID );


		virtual void onRender( IRenderSystem& RDSystem );
		virtual bool onMouse( MouseMsg const& msg ){ return true; }
		virtual void onKey( unsigned key , bool isDown ){}
		virtual void onUpdate( long time );
		virtual void onStart();

		char const* loadID;

		void loadResFun();
		typedef fastdelegate::FastDelegate< void () > LoadingFun;
		TFunctionThread< LoadingFun > mThreadLoading;
		int  numTotal;
		int  numCur;
	};


	class MenuStage : public ZStage
	{
		typedef ZStage BaseClass;
	public:
		MenuStage( int stageID ): ZStage( stageID ){}
		virtual void onStart()
		{
			BaseClass::onStart();
			skyPos  = 0;
		}
		virtual void onUpdate( long time )
		{
			BaseClass::onUpdate( time );
			skyPos  += 0.01f * time;
		}

		void  drawSky( IRenderSystem& RDSystem , ResID skyID );

		float skyPos;

	};

	class ZMainMenuStage : public MenuStage
	{
		typedef MenuStage BaseClass;
	public:
		ZMainMenuStage();

		void onRender( IRenderSystem& RDSystem );
		void onKey( unsigned key , bool isDown ){}
		void onUpdate( long time );

		virtual void onEnd(){}
		virtual void onStart();

		float      angleSunGlow;
		//ZStarPS*   starPS;
		Vec2f      aimPos;
		ZConBall*  aimBall;
	};


	class AdvMenuStage : public MenuStage
	{
		typedef MenuStage BaseClass;
	public:
		AdvMenuStage( ZUserData& data );

		virtual void onRender( IRenderSystem& RDSystem );

		virtual bool onMouse( MouseMsg const& msg ){ return true; }
		virtual void onKey( unsigned key , bool isDown ){}
		virtual void onUpdate( long time );

		virtual void onEnd(){}
		virtual void onStart();

		ZAdvButton* createAdvButton( unsigned stageID , ResID texID , Vec2i const& pos );
		ZUserData& userData;
		unsigned   finishTemp;
	};

}//namespace Zuma

#endif // ZStage_h__
