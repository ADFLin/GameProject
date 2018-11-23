#pragma once
#ifndef GameModule_H_5E5B32B4_50E2_43F3_B7AB_58F84870E299
#define GameModule_H_5E5B32B4_50E2_43F3_B7AB_58F84870E299

#include "GameDefines.h"
#include "CppVersion.h"
#include "DataStream.h"
#include "MarcoCommon.h"
#include "FastDelegate/FastDelegate.h"

class GWidget;
typedef fastdelegate::FastDelegate< bool ( int , int , GWidget* ) > EvtCallBack;

class StageBase;
class StageManager;
class GameController;
class IPlayerManager;
class GameStage;
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
	virtual bool  checkSettingSV(){ return true;  }

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

class IGameInstance
{
public:
	virtual ~IGameInstance(){}
};

class IGameModule
{
public:
	virtual ~IGameModule(){}
	virtual bool initialize(){ return true; }
	virtual void cleanup(){}

	virtual void enter(){}
	virtual void exit(){} 
	//
	virtual void beginPlay( StageModeType type, StageManager& manger );
	virtual void endPlay(){}

	virtual void  deleteThis() = 0;

	virtual IGameInstance*        createInstance() { return nullptr; }
public:
	virtual char const*           getName() = 0;
	virtual GameController&       getController() = 0;
	virtual StageBase*            createStage( unsigned id ){ return nullptr; }
	virtual SettingHepler*        createSettingHelper( SettingHelperType type ){ return nullptr; }
	virtual bool                  getAttribValue( AttribValue& value ){ return false; }

	//old replay version
	virtual ReplayTemplate*       createReplayTemplate( unsigned version ){ return nullptr; }
};

typedef IGameModule* (*CreateGameModuleFun)();

#define CREATE_GAME_MODULE CreateGameModule
#define CREATE_GAME_MODULE_STR MAKE_STRING(CREATE_GAME_MODULE)

#define EXPORT_GAME_MODULE( CLASS )\
	extern "C" DLLEXPORT IGameModule* CREATE_GAME_MODULE()\
	{\
		return new CLASS; \
	}\

#endif // GameModule_H_5E5B32B4_50E2_43F3_B7AB_58F84870E299


