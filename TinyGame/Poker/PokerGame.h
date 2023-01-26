#ifndef PokerGame_h__
#define PokerGame_h__

#include "GameModule.h"
#include "GameRenderSetup.h"
#include "CardDraw.h"


#define POKER_GAME_NAME "Poker"

namespace Poker
{
	enum GameRule
	{
		RULE_Big2  ,
		RULE_Holdem ,
		RULE_FreeCell ,
		RULE_DouDizhu,

		RULE_COUNT,
	};

	char const* ToRuleString(GameRule rule)
	{
		switch (rule)
		{
		case RULE_Big2: return "Big2";
		case RULE_Holdem: return "Holdem";
		case RULE_FreeCell: return "FreeCell";
		case RULE_DouDizhu: return "Dou-Dizhu";
		}

		return "Unknown";
	}

	class GameModule : public IGameModule
		             , public LegacyPlatformRenderSetup
	{
	public:
		GameModule();
		virtual ~GameModule();
	
		virtual void  enter();
		virtual void  exit(){} 
		//
		virtual void beginPlay( StageManager& manger, EGameStageMode modeType ) override;
		
		virtual void notifyStageInitialized(StageBase* stage);
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

		void initializeResource()
		{
			mCardDraw = ICardDraw::Create(ICardDraw::eWin7);
		}

		void releaseResource()
		{
			if (mCardDraw)
			{
				mCardDraw->release();
				mCardDraw = nullptr;
			}
		}

		bool setupRenderSystem(ERenderSystem systemName) override
		{
			initializeResource();
			if (mSetup)
			{
				mSetup->setupCardDraw(mCardDraw);
			}
			return true;
		}
		void preShutdownRenderSystem(bool bReInit) override
		{
			releaseResource();
		}
		void setupPlatformGraphic() override
		{
			initializeResource();
			if (mSetup)
			{
				mSetup->setupCardDraw(mCardDraw);
			}
		}

		ICardDraw* mCardDraw = nullptr;
		ICardResourceSetup* mSetup = nullptr;
	};




}//namespace Poker

#endif // PokerGame_h__
