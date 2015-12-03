#ifndef StageBase_h__
#define StageBase_h__

#include "TaskBase.h"
#include "GamePlayer.h"
#include "GameWidget.h"

class StageManager;
class IGamePackage;
class GamePackageManager;
class MouseMsg;
class NetWorker;
class GWidget;

enum StageID
{
	STAGE_MAIN_MENU    ,
	STAGE_GAME_MENU    ,
	STAGE_SINGLE_GAME  ,

	STAGE_NET_ROOM     ,
	STAGE_NET_GAME     ,

	STAGE_REPLAY_EDIT  ,
	STAGE_REPLAY_GAME  ,

	STAGE_ABOUT_GAME  ,
	STAGE_RECORD_GAME ,

	STAGE_NEXT_ID     ,
};


class StageBase : public TaskHandler
{
public:
	enum
	{
		NEXT_UI_ID = UI_STAGE_ID ,
	};
	StageBase();
	virtual ~StageBase();

	void  update( long time );
	void  render( float dFrame );

	virtual bool onInit(){ return true; }
	virtual void onEnd(){}
	virtual void onUpdate( long time ){}
	virtual void onRender( float dFrame ){}
	
	virtual bool onChar( unsigned code );
	virtual bool onMouse( MouseMsg const& msg );
	virtual bool onKey( unsigned key , bool isDown );

	virtual bool onEvent( int event , int id , GWidget* ui ){ return true; }
	virtual void onTaskMessage( TaskBase* task , TaskMsg const& msg );

	StageManager*  getManager(){ return mManager; }

	//virtual void destroyThis();

private:
	StageManager*    mManager;
	friend class StageManager;
};


class StageManager : public TaskHandler
	               , public WidgetEventHandler               
{
public:
	StageManager();
	~StageManager();
	virtual void        setTickTime( long time ) = 0;
	virtual NetWorker*  buildNetwork( bool beServer ) = 0;
	virtual NetWorker*  getNetWorker() = 0;
	virtual StageBase*  createStage( StageID stage ) = 0;

	void                cleanup();
	StageBase*          getCurStage(){ return mCurStage; }
	StageBase*          getNextStage(){ return mNextStage; }

	void                setNextStage( StageBase* chStage );
	StageBase*          changeStage( StageID stageId );
	void                setErrorMsg( char const* msg ){ /*mErrorMsg = msg;*/  }

protected:

	enum FailReason
	{
		FR_NO_STAGE ,
		FR_INIT_FAIL ,
	};
	virtual StageBase* onStageChangeFail( FailReason reason ){  return NULL;  }
	virtual void       postStageChange( StageBase* stage ){}
	virtual void       prevChangeStage(){}
	

public:
	UserProfile&     getUserProfile(){ return mUserProfile;  }
protected:
	void             checkNewStage();
	void             setupStage();

	//std::string     mErrorMsg;
private:
	UserProfile     mUserProfile;
	StageBase*      mNextStage;
	StageBase*      mCurStage;
};

#endif // StageBase_h__