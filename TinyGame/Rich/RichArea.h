#ifndef RichArea_h__
#define RichArea_h__

#include "RichBase.h"

#include "DataStructure/Array.h"

#include <functional>
#include <string>
#include <vector>


namespace Rich
{

	int const STATION_GROUP = 0;


	class LandArea;
	class StationArea;
	class CardArea;
	class EmptyArea;
	class StartArea;
	class ComponyArea;
	class StoreArea;
	class JailArea;
	class ActionEventArea;

	struct AreaGroupInfo;
	enum StationType
	{
		STATION_TRAIN ,
		STATION_AIRPLANE ,
	};

	enum StoreType
	{
		STORE_TOOL ,
		STORE_CARD ,
		STORE_BANK ,
	};

	class AreaVisitor
	{
	public:
		virtual void visit( LandArea& area ){}
		virtual void visit( StationArea& area ){}
		virtual void visit( CardArea& area ){}
		virtual void visit( EmptyArea& area){}
		virtual void visit( StartArea& area ){}
		virtual void visit( ComponyArea& area ){}
		virtual void visit( StoreArea& area ){}
		virtual void visit( JailArea& area) {}
		virtual void visit( ActionEventArea& area) {}


		void visitWorld(World& world);
	};

#define ACCEPT_VISIT()\
	virtual void accept( AreaVisitor& visitor ) override { visitor.visit( *this ); }

	class EmptyArea : public Area
	{
	public:
		EmptyArea( AreaId id ){ setId( id ); }
		ACCEPT_VISIT();
	};

	class LandGroup
	{
	public:
		TArray< LandArea* > mLands;
	};



	class OwnableArea : public Area
		              , public PlayerAsset
	{
	public:
		OwnableArea()
		{
			mOwner = nullptr;
		}
		Player*  getOwner(){ return mOwner; }
		void     setOwner(Player& player)
		{
			mOwner = &player;
		}

		Area* getAssetArea() { return this; }
		void releaseAsset() override {}
	protected:
		Player*  mOwner;
	};


	class LandArea : public OwnableArea
	{
	public:
		ACCEPT_VISIT();

		LandArea()
		{
			mLevel = 0;
		}

		struct Info
		{
			String name;
			int    basePrice;
			int    motgageCost;
			TArray<int> tolls;
			int    upgradeCost;
			int    group;
		};
		Info& getInfo(){  return mInfo; }

		char const* getName() const { return mInfo.name.c_str(); }


		int calcPrice( Player const& player ) const 
		{
			return mInfo.basePrice * ( mLevel + 1 );
		}
		int calcToll( Player const&  player ) const;

		int calcUpgradeCost(Player const& player) const
		{
			return mInfo.upgradeCost;
		}

		void onPlayerStay( PlayerTurn& turn, Player& player);


		void upgradeLevel()
		{
			mLevel += 1;
		}

		int getMaxLevel() const
		{
			return (int)mInfo.tolls.size() - 1;
		}

		Info    mInfo;
		int     mLevel = 0;	

		int getAssetValue() override;
		void changeOwner(Player* player) override;


		void reset() override
		{
			mLevel = 0;
			mOwner = nullptr;
		}

		bool isGroupOwned( World& world ) const;

		void install(World& world) override;

	};


	class StationArea : public OwnableArea

	{
	public:
		ACCEPT_VISIT();

		struct Info
		{
			String name;
			int    basePrice;
			int    motgageCost;
			TArray<int> tolls;
		};

		void onPlayerStay(PlayerTurn& turn, Player& player);

		void install(World& world) override;
		int calcPrice(Player const& player) const
		{
			return mInfo.basePrice;
		}
		int calcToll(Player const& player) const;

		int getGroupNum(World& world) const;

		Info mInfo;

		int getAssetValue() override;
		void changeOwner(Player* player) override;

	};


	class ComponyArea : public Area
	{
	public:
		ACCEPT_VISIT();

	};


	enum CardGroup
	{
		CG_CHANCE ,
		CG_COMMUNITY ,
	};

	class CardArea : public Area
	{
	public:
		ACCEPT_VISIT();
		void onPlayerStay( PlayerTurn& turn, Player& player);
		CardGroup mGroup;
	};

	class StartArea : public Area
	{
	public:
		ACCEPT_VISIT();
		void onPlayerPass( PlayerTurn& turn );
		void onPlayerStay( PlayerTurn& turn, Player& player);
	};

	class JailArea : public Area
	{
	public:
		ACCEPT_VISIT();
		void install(World& world) override;
	};

	class ActionEvent;
	class ActionEventArea : public Area
	{
	public:
		ACCEPT_VISIT();
		virtual ActionEvent& getEvent() = 0;
	};

	template< typename TEvent >
	class TActionEventArea : public ActionEventArea
	{
	public:
		void onPlayerStay(PlayerTurn& turn, Player& player) override
		{
			mEvent.doAction(&player, turn);
		}

		ActionEvent& getEvent() { return mEvent; }
		TEvent mEvent;
	};


	class StoreArea : public Area
	{
	public:
		void onPlayerStay(PlayerTurn& turn, Player& player);

		StoreType mType;
	};

}//namespace Rich

#endif // RichArea_h__
