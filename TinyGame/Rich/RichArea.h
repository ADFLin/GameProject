#ifndef RichArea_h__
#define RichArea_h__

#include "RichBase.h"

#include <functional>
#include <string>
#include <vector>

namespace Rich
{
	class LandArea;
	class StationArea;
	class CardArea;
	class EmptyArea;
	class StartArea;
	class ComponyArea;
	class StoreArea;

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
		std::vector< LandArea* > mLands;
	};



	class OwnableArea : public Area
	{
	public:
		OwnableArea()
		{
			mOwner = nullptr;
		}
		Player*  getOwner(){ return mOwner; }
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
		};
		Info& getInfo(){  return mInfo; }

		char const* getName() const { return mInfo.name.c_str(); }

		Player* getOwner() { return mOwner;  }
		void    setOwner( Player& player )
		{
			mOwner = &player;
		}
		int calcPrice( Player const& player ) const 
		{
			return mInfo.basePrice;
		}
		int calcToll( Player const&  player ) const 
		{
			return 0;
		}

		int calcUpdateCost( Player const&  player ) const
		{
			return 0;
		}

		void onPlayerStay( PlayerTurn& turn );


		void upgradeLevel()
		{

		}


		Info    mInfo;
		int     mLevel;
		
	};


	class StoreArea : public Area
	{
	public:
		void onPlayerStay( PlayerTurn& turn );

		StoreType mType;
	};

	class ComponyArea : public Area
	{
	public:
		ACCEPT_VISIT();

	};
	class StationArea : public Area
	{
	public:
		ACCEPT_VISIT();

	};


	enum CardGroup
	{
		CG_CHANCE ,
		CG_DESTINY ,
	};

	class CardArea : public Area
	{
	public:
		ACCEPT_VISIT();
		void onPlayerStay( PlayerTurn& turn )
		{
			switch( mGroup )
			{
			case CG_CHANCE:
				break;
			case CG_DESTINY:
				break;
			}
		}
		CardGroup mGroup;
	};

	class StartArea : public Area
	{
	public:
		ACCEPT_VISIT();
		void onPlayerPass( PlayerTurn& turn );
		void onPlayerStay( PlayerTurn& turn );

		void giveMoney( Player& player );
		int money;
	};

}//namespace Rich

#endif // RichArea_h__
