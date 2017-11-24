#ifndef CARGame_h__9c08f051_07b8_4f17_b7f0_9f5cf184db8e
#define CARGame_h__9c08f051_07b8_4f17_b7f0_9f5cf184db8e

#include "GameModule.h"

#define CAR_GAME_NAME "Carcassonne"

namespace CAR
{

	class GameModule : public IGameModule
	{
	public:
		GameModule();
		virtual ~GameModule();
		virtual bool  initialize(){ return true; }
		virtual void  cleanup(){}
		virtual void  enter(){}
		virtual void  exit(){} 
		virtual void  deleteThis(){ delete this; }
		//
		virtual void beginPlay( StageModeType type, StageManager& manger );
	public:
		virtual char const*           getName(){ return CAR_GAME_NAME; }
		virtual GameController&       getController(){ return IGameModule::getController(); }
		virtual StageBase*            createStage( unsigned id );
		virtual SettingHepler*        createSettingHelper( SettingHelperType type );
		virtual bool                  getAttribValue( AttribValue& value );

		//old replay version
		virtual ReplayTemplate*       createReplayTemplate( unsigned version ){ return NULL; }

	public:
	};




}//namespace CAR

#endif // CARGame_h__9c08f051_07b8_4f17_b7f0_9f5cf184db8e
