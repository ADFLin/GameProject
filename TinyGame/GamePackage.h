#ifndef GamePackage_h__
#define GamePackage_h__

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

enum GameAttrib
{
	//Game
	ATTR_GAME_VERSION ,
	ATTR_NET_SUPPORT     ,
	ATTR_SINGLE_SUPPORT  ,
	ATTR_REPLAY_SUPPORT  ,
	ATTR_AI_SUPPORT       ,
	ATTR_REPLAY_INFO_DATA ,
	
	ATTR_CONTROLLER_DEFUAULT_SETTING ,
	//SubStage
	ATTR_REPLAY_INFO     ,
	ATTR_TICK_TIME       ,
	ATTR_GAME_INFO       ,
	ATTR_REPLAY_UI_POS   ,
	ATTR_NEXT_ID ,
};
struct AttribValue
{
	AttribValue( int id ): id( id ){ ptr = 0; }
	AttribValue( int id , int val ):id( id ),iVal( val ){}
	AttribValue( int id , long val ):id( id ),iVal( val ){}
	AttribValue( int id , float val ):id( id ),fVal( val ){}
	AttribValue( int id , char const* val ):id( id ),str( val ){}
	AttribValue( int id , void* val ):id( id ),ptr( val ){}
	int id;
	union
	{
		struct
		{
			int x , y;
		} v2;
		long  iVal;
		float fVal;
		void* ptr;
		char const* str;
	};
};

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
	virtual bool  create(){ return true; }
	virtual void  cleanup(){}
	virtual bool  load(){ return true; }
	virtual void  release(){} 
	//
	virtual void  enter( StageManager& manger ) = 0;
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
