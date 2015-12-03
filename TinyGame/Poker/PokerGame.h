#ifndef PokerGame_h__
#define PokerGame_h__

#include "GamePackage.h"


#define POKER_GAME_NAME "Poker"

namespace Poker
{
	enum GameRule
	{
		RULE_BIG2  ,
		RULE_HOLDEM ,
		RULE_FREECELL ,

	};

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
		virtual char const*           getName(){ return POKER_GAME_NAME; }
		virtual GameController&       getController(){ return IGamePackage::getController(); }
		virtual GameSubStage*         createSubStage( unsigned id );
		virtual StageBase*            createStage( unsigned id );
		virtual SettingHepler*        createSettingHelper( SettingHelperType type );
		virtual bool                  getAttribValue( AttribValue& value );

		//old replay version
		virtual ReplayTemplate*       createReplayTemplate( unsigned version ){ return NULL; }


	public:
		void     setRule( GameRule rule ){ mRule = rule; }
		GameRule getRule(){ return mRule; }

		GameRule mRule;
	};




}//namespace Poker

#endif // PokerGame_h__
