#ifndef CARGame_h__9c08f051_07b8_4f17_b7f0_9f5cf184db8e
#define CARGame_h__9c08f051_07b8_4f17_b7f0_9f5cf184db8e

#include "GamePackage.h"

#define CAR_GAME_NAME "Carcassonne"

namespace CAR
{

	class CGamePackage : public IGamePackage
	{
	public:
		CGamePackage();
		virtual ~CGamePackage();
		virtual bool  create(){ return true; }
		virtual void  cleanup(){}
		virtual bool  load(){ return true; }
		virtual void  release(){} 
		virtual void  deleteThis(){ delete this; }
		//
		virtual void  enter( StageManager& manger );
	public:
		virtual char const*           getName(){ return CAR_GAME_NAME; }
		virtual GameController&       getController(){ return IGamePackage::getController(); }
		virtual GameSubStage*         createSubStage( unsigned id );
		virtual StageBase*            createStage( unsigned id );
		virtual SettingHepler*        createSettingHelper( SettingHelperType type );
		virtual bool                  getAttribValue( AttribValue& value );

		//old replay version
		virtual ReplayTemplate*       createReplayTemplate( unsigned version ){ return NULL; }

	public:
	};




}//namespace CAR

#endif // CARGame_h__9c08f051_07b8_4f17_b7f0_9f5cf184db8e
