#ifndef HoldemScene_h__
#define HoldemScene_h__

#include "HoldemLevel.h"
#include "GameWidget.h"

class IPlayerManager;

namespace Poker
{
	class ICardDraw;
}//namespace Poker

namespace Poker { namespace Holdem {

	class BetPanelBase: public GFrame
	{
		typedef GFrame BaseClass;
	public:
		enum
		{
			UI_FOLD = BaseClass::NEXT_UI_ID,
			UI_CALL ,
			UI_RISE ,
			UI_ALL_IN ,

			UI_RISE_MONEY ,
			UI_RISE_MONEY_TEXT ,
		};
		BetPanelBase( int id , Vec2i const& pos , GWidget* parent );
		bool onChildEvent( int event , int id , GWidget* ui );
		void setRiseMoney( int money );
		void doRefreshWidget( LevelBase& level , int pos );
	};


	class BetPanel;

	class Scene : public ClientLevel::Listener
	{
	public:
		Scene( ClientLevel& level , IPlayerManager& playerMgr );
		~Scene();

		ClientLevel& getLevel(){ return *mLevel; }
		void drawPlayerState( Graphics2D& g , Vec2i const& pos , int posSlot , SlotInfo const& info );
		void setupWidget();
		void drawSlot( Graphics2D& g , Vec2i const& pos , SlotInfo const& info );
		void render( Graphics2D& g );


		void betRequest( BetType type , int money = 0 )
		{
			mLevel->betRequest( type , money );
		}

		void drawSlotPanel( GWidget* widget );


		virtual void onBetCall( int slot );
		virtual void onBetResult( int slot , BetType type , int money )
		{

		}

		IPlayerManager* mPlayerMgr;
		ClientLevel*    mLevel;
		BetPanel*       mPanel;
		ICardDraw*      mCardDraw;
	};


}//namespace Holdem
}//namespace Poker

#endif // HoldemScene_h__
