#pragma once
#ifndef GameModule_H_5E5B32B4_50E2_43F3_B7AB_58F84870E299
#define GameModule_H_5E5B32B4_50E2_43F3_B7AB_58F84870E299

#include "ModuleInterface.h"

#include "GameDefines.h"
#include "CppVersion.h"
#include "Serialize/DataStream.h"
#include "MarcoCommon.h"
#include "FastDelegate/FastDelegate.h"


class GWidget;
using WidgetEventCallBack = fastdelegate::FastDelegate< bool ( int , int , GWidget* ) >;

class StageBase;
class StageManager;
class InputControl;
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
		mListener = nullptr;
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

class IGameModule : public IModuleInterface
{
public:

	virtual void enter(){}
	virtual void exit(){} 
	//
	virtual void beginPlay(StageManager& manger, EGameStageMode modeType) = 0;
	virtual void endPlay(){}

	virtual IGameInstance*        createInstance() { return nullptr; }

	bool isGameModule() const final { return true; }
public:
	virtual char const*           getName() = 0;
	virtual InputControl&         getInputControl() = 0;
	virtual StageBase*            createStage( unsigned id ){ return nullptr; }
	virtual SettingHepler*        createSettingHelper( SettingHelperType type ){ return nullptr; }
	virtual bool                  queryAttribute( GameAttribute& value ){ return false; }

	//old replay version
	virtual ReplayTemplate*       createReplayTemplate( unsigned version ){ return nullptr; }

	bool changeDefaultStage(StageManager& stageManager, EGameStageMode modeType);
};


#endif // GameModule_H_5E5B32B4_50E2_43F3_B7AB_58F84870E299


