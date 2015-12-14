#ifndef GamePackage_h__
#define GamePackage_h__

#include "GameDefines.h"
#include "CppVersion.h"
#include "DataStream.h"
#include "FastDelegate/FastDelegate.h"

class GWidget;
typedef fastdelegate::FastDelegate< bool ( int , int , GWidget* ) > EvtCallBack;

class StageBase;
class StageManager;
class GameController;
class IPlayerManager;
class GameStage;
class GameSubStage;  
class ReplayTemplate;

struct ReplayInfo;



class SettingListener
{
public:
	virtual void onModify( GWidget* ui ) = 0;
};

enum SettingHelperType
{
	SHT_NET_ROOM_HELPER ,
};

class SettingHepler
{
public:
	SettingHepler()
	{
		mListener = NULL;
	}
	virtual ~SettingHepler(){}
	virtual void  addGUIControl( GWidget* ui ) = 0;
	virtual bool  checkSettingVaildSV(){ return true;  }

	void modifyCallback( GWidget* ui )
	{
		if ( mListener )
			mListener->onModify( ui );
	}

	void setListener( SettingListener* listener ){ mListener = listener; }
private:
	SettingListener* mListener;
};


struct GameEvent
{

};

class IGamePackage
{
public:
	virtual ~IGamePackage(){}
	virtual bool  initialize(){ return true; }
	virtual void  cleanup(){}

	virtual void enter(){}
	virtual void exit(){} 
	//
	virtual void beginPlay( GameType type, StageManager& manger );
	virtual void endPlay(){}

	virtual void  deleteThis() = 0;
public:
	virtual char const*           getName() = 0;
	virtual GameController&       getController() = 0;
	virtual GameSubStage*         createSubStage( unsigned id ){ return nullptr; }
	virtual StageBase*            createStage( unsigned id ){ return nullptr; }
	virtual SettingHepler*        createSettingHelper( SettingHelperType type ){ return nullptr; }
	virtual bool                  getAttribValue( AttribValue& value ){ return false; }

	//old replay version
	virtual ReplayTemplate*       createReplayTemplate( unsigned version ){ return nullptr; }
};

typedef IGamePackage* (*CREATEGAMEFUN)();
#define GAME_CREATE_FUN_NAME "CreateGame"

#define EXPORT_GAME( CLASS )\
extern "C"\
{\
	__declspec( dllexport ) IGamePackage* CreateGame()\
	{\
		return new CLASS;\
	}\
}


#endif // GamePackage_h__
