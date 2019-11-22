#pragma once
#ifndef GameModule_H_5E5B32B4_50E2_43F3_B7AB_58F84870E299
#define GameModule_H_5E5B32B4_50E2_43F3_B7AB_58F84870E299

#include "GameDefines.h"
#include "CppVersion.h"
#include "Serialize/DataStream.h"
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

class IModuleInterface
{
public:
	virtual ~IModuleInterface() {}
	virtual bool initialize() { return true; }
	virtual void cleanup() {}
	virtual bool isGameModule() const = 0;

	virtual void  deleteThis() = 0;
};

class IGameModule : public IModuleInterface
{
public:

	virtual void enter(){}
	virtual void exit(){} 
	//
	virtual void beginPlay(StageManager& manger, StageModeType modeType) = 0;
	virtual void endPlay(){}

	virtual IGameInstance*        createInstance() { return nullptr; }

	virtual bool isGameModule() const override { return true; }
public:
	virtual char const*           getName() = 0;
	virtual GameController&       getController() = 0;
	virtual StageBase*            createStage( unsigned id ){ return nullptr; }
	virtual SettingHepler*        createSettingHelper( SettingHelperType type ){ return nullptr; }
	virtual bool                  queryAttribute( GameAttribute& value ){ return false; }

	//old replay version
	virtual ReplayTemplate*       createReplayTemplate( unsigned version ){ return nullptr; }


	bool changeDefaultStage(StageManager& stageManager, StageModeType modeType);
};

typedef IModuleInterface* (*CreateModuleFun)();

#define CREATE_MODULE CreateModule
#define CREATE_MODULE_STR MAKE_STRING(CREATE_MODULE)

#define EXPORT_MODULE( CLASS )\
	extern "C" DLLEXPORT IModuleInterface* CREATE_MODULE()\
	{\
		return new CLASS; \
	}\

#define EXPORT_GAME_MODULE( CLASS )\
	EXPORT_MODULE(CLASS)

#endif // GameModule_H_5E5B32B4_50E2_43F3_B7AB_58F84870E299


