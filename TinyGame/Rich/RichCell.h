#ifndef RichCell_h__
#define RichCell_h__

#include "RichBase.h"

#include <functional>
#include <string>
#include <vector>

namespace Rich
{
	class LandCell;
	class StationCell;
	class CardCell;
	class EmptyCell;
	class StartCell;
	class ComponyCell;
	class StoreCell;

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

	class CellVisitor
	{
	public:
		virtual void visit( LandCell& land ){}
		virtual void visit( StationCell& ts ){}
		virtual void visit( CardCell& cardCell ){}
		virtual void visit( EmptyCell& emptyCell ){}
		virtual void visit( StartCell& startcell ){}
		virtual void visit( ComponyCell& compony ){}
		virtual void visit( StoreCell& compony ){}
	};

	class EmptyCell : public Cell
	{
	public:
		EmptyCell( CellId id ){ setId( id ); }
		ACCEPT_VISIT();
	};

	class LandGroup
	{
	public:
		std::vector< LandCell* > mLands;
	};



	class OwnedCell : public Cell
	{
	public:
		OwnedCell()
		{
			mOwner = nullptr;
		}
		Player*  getOwner(){ return mOwner; }
	protected:
		MapCoord mPos;
		Player*  mOwner;
	};


	class LandCell : public OwnedCell
	{
	public:
		ACCEPT_VISIT();

		LandCell()
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

		void setOwner( Player& player )
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


	class StoreCell : public Cell
	{
	public:
		void onPlayerStay( PlayerTurn& turn );

		StoreType mType;
	};

	class ComponyCell : public Cell
	{
	public:
		ACCEPT_VISIT();

	};
	class StationCell : public Cell
	{
	public:
		ACCEPT_VISIT();

	};


	enum CardGroup
	{
		CG_CHANCE ,
		CG_DESTINY ,
	};

	class CardCell : public Cell
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

	class StartCell : public Cell
	{
	public:
		ACCEPT_VISIT();
		void onPlayerPass( PlayerTurn& turn );
		void onPlayerStay( PlayerTurn& turn );

		void giveMoney( Player& player );
		int money;
	};

}//namespace Rich

#endif // RichCell_h__
