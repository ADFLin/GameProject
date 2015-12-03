#ifndef ZLevelStage_h__
#define ZLevelStage_h__

#include "ZBase.h"
#include "ZStage.h"
#include "ZLevel.h"

namespace Zuma
{
	class   LevelStageTask;
	struct ZQuakeMap;


	enum LevelRenderOrder
	{
		LRO_QUAKE_MAP ,
		LRO_TEXT ,
		LRO_UI = RO_UI ,
	};

	class LevelStage :  public ZStage
	{
		typedef ZStage BaseClass;
	public:

		LevelStage( ZLevelInfo& info );

		virtual void onEnd(){}
		virtual void onStart();

		virtual bool onUIEvent( int evtID , int uiID , ZWidget* ui );

		virtual void onRender( IRenderSystem& rs );
		virtual bool onMouse( MouseMsg const& msg );
		virtual void onKey( unsigned key , bool isDown );
		virtual void onUpdate( long time );
		virtual bool onEvent( int evtID , ZEventData const& data , ZEventHandler* sender );


		ZLevel& getLevel(){  return mLevel;  }
		bool  isPauseGame(){ return  mPauseGame;  }

		void  onLevelTaskEnd( LevelStageTask* task );

		void changeState( LvState state );
		void restartLevel();
		void addScore( int score )
		{
			levelScore += score;
		}
		void showQuake( ZLevelInfo* info ){ mPreLvInfo = info; }
		void setTitle( char const* name ){ lvTitle = name; }

		std::string const& getLevelTitle() const { return lvTitle; }

		ZLevel           mLevel;
		LevelStageTask*  mLvSkipTask;

		float            pathPosGroupFinish[ 2 ];

		ZLevelInfo*      mPreLvInfo;
		int              levelScore;
		int              showSorce;

		std::string      lvTitle;
		bool             mPauseGame;
		LvState          lvState;
		int              numLive;

		ZLevel::RenderParam mRenderParam;
	};


	class LevelStageTask : public TaskBase
	{
		typedef TaskBase BaseClass;
	public:
		LevelStageTask( LevelStage* stage );

		virtual bool  updateAction( long time ) = 0;
		virtual void  doSkipAction(){}

		void   skipAction();
		bool   checkSoundTimer( unsigned time );

	protected:
		bool   onUpdate( long time );
		void   onEnd( bool beComplete );

		unsigned     timerSound;
		int          curProgess;
		LevelStage*  mLevelStage;
	};


	class LevelTitleTask : public LevelStageTask
		                 , public TaskListener
		                 , public IRenderable
	{
		typedef LevelStageTask BaseClass;
	public:

		LevelTitleTask( LevelStage* level ):LevelStageTask( level ){}
		void release(){ delete this; }

		enum
		{
			eShowLevelTitle ,
			eShowCurveLine  ,
		};

		enum TaskID
		{
			TASK_FONT_FADE_IN  = 1,
			TASK_FONT_FADE_OUT ,
		};

		virtual void onTaskMessage( TaskBase* task , TaskMsg const& msg );
		virtual void onStart();

		void  onRender( IRenderSystem& RDSystem );
		bool  updateAction( long time );

		virtual void onEnd( bool beComplete );

		bool        isFinish;
		bool        fontFadeOut;
		float       scaleFontT;
		float       scaleFontB;
		int         idxCurPath;
		ZPath*      curPath;
		float       pathPos;

	};

	class LevelImportBallTask : public LevelStageTask
	{
		typedef LevelStageTask BaseClass;
	public:
		LevelImportBallTask( LevelStage* level , float speed )
			:LevelStageTask( level )
			,saveSpeed( speed ){}
		void release(){ delete this; }

		virtual void onStart();
		virtual bool updateAction( long time );

		float saveSpeed;
		float importLen;
	};

	class LevelFinishTask : public LevelStageTask
	{
		typedef LevelStageTask BaseClass;
	public:
		LevelFinishTask( LevelStage* level )
			:LevelStageTask( level )
		{
		}
		void release(){ delete this; }
		virtual void onStart();
		virtual bool updateAction( long time );

		int          idxCurPath;
		ZPath*       curPath;
		float        pathPos;
		float        PSPos;

	};



	class LevelQuakeTask : public LevelStageTask
		, public IRenderable
	{
		typedef LevelStageTask BaseClass;
	public:

		enum QuakeState
		{
			eQuakeShake ,
			eQuakeMove  ,
		};
		LevelQuakeTask( LevelStage* level , ZQuakeMap& QMap , ZLevelInfo& preLvInfo );
		void release(){ delete this; }
		virtual void onStart();
		virtual bool updateAction( long time );
		virtual void onEnd( bool beComplete );
		void onRender( IRenderSystem& RDSystem );

	private:
		ITexture2D* mLvBGImage;
		ZLevelInfo& mPreLvInfo;
		ZQuakeMap&  mQMap;
		Vec2D       bgAlphaPos[6];
	};

	class PSRunTask;

	class LevelFailTask :public LevelStageTask
	{
		typedef LevelStageTask BaseClass;
	public:
		LevelFailTask( LevelStage* level ):LevelStageTask( level  ){}
		void release(){ delete this; }
		enum
		{
			eRemoveBall  ,
			eShowLifeNum ,
		};
		virtual void onStart();
		virtual bool updateAction( long time );
		float        angle;
		PSRunTask*   psTask;

	};
}//namespace Zuma


#endif // ZLevelStage_h__
