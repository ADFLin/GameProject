#ifndef PokerGame_h__
#define PokerGame_h__

#include "GameModule.h"


#define POKER_GAME_NAME "Poker"

namespace Poker
{
	enum GameRule
	{
		RULE_BIG2  ,
		RULE_HOLDEM ,
		RULE_FREECELL ,

	};

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
		virtual void beginPlay( StageManager& manger, EGameStageMode modeType ) override;
	public:
		virtual char const*           getName(){ return POKER_GAME_NAME; }
		virtual InputControl&         getInputControl(){ return IGameModule::getInputControl(); }
		virtual StageBase*            createStage( unsigned id );
		virtual SettingHepler*        createSettingHelper( SettingHelperType type );
		virtual bool                  queryAttribute( GameAttribute& value );

		//old replay version
		virtual ReplayTemplate*       createReplayTemplate( unsigned version ){ return NULL; }


	public:
		void     setRule( GameRule rule ){ mRule = rule; }
		GameRule getRule(){ return mRule; }

		GameRule mRule;
	};




}//namespace Poker

#endif // PokerGame_h__
