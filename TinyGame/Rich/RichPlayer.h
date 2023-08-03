#ifndef RichPlayer_h__
#define RichPlayer_h__

#include "RichBase.h"
#include "RichWorld.h"

#include "Singleton.h"

#include <functional>

namespace Rich
{
	class Player;
	class LandArea;
	class IPlayerController;

	class Player : public ActorComp
	{
	public:

		enum State
		{
			eIDLE ,
			eJAILED ,
			eTAKE_SICK ,
			eGAME_OVER ,
		};
		Player( World& world , ActorId id );
		void   initPos(MapCoord const& pos, MapCoord const& prevPos);

		Tile*   updateTile(MapCoord const& pos);

		void    updateTile(Tile& tile);

		World&  getWorld() const { return *mOwnedWorld; }
		Tile*   changePos( MapCoord const& pos );

		void    setRole( RoleId id ){ mRoleId = id; }
		RoleId  getRoleId(){ return mRoleId; }

		int     getMovePower();

		int     getTotalMoney() const { return mMoney; }
		void    modifyMoney( int delta );

		IPlayerController& getController(){ assert( mController ); return *mController; }
		void         setController( IPlayerController& controller ){ mController = &controller; }

		bool  isActive() const;

		void  changeState( State state )
		{
			if ( mState == state )
				return;
			mState = state;
			switch (mState)
			{
			case Player::eIDLE:
				break;
			case Player::eJAILED:
				mStateCount = 3;
				break;
			case Player::eTAKE_SICK:
				break;
			}
		}

		int getTotalAssetValue() const
		{
			int totalAssetValue = mMoney;
			for (auto asset : mAssets)
			{
				totalAssetValue += asset->getAssetValue();
			}
			return totalAssetValue;
		}

		void startTurn()
		{
			switch (mState)
			{
			case Player::eIDLE:
				break;
			case Player::eJAILED:
				--mStateCount;
				if (mStateCount <= 0)
				{
					changeState(Player::eIDLE);
				}
				break;
			case Player::eTAKE_SICK:
				break;
			}
		}

		void reset()
		{
			mAssets.clear();
			mState = eIDLE;
			mStateCount = 0;
		}


	public:
		TArray< PlayerAsset*> mAssets;
		int          mMovePower;
		World*       mOwnedWorld;
		RoleId       mRoleId;
		State        mState;
		int          mStateCount = 0;

		int          mMoney;
		int          mCardPoint;
		IPlayerController* mController;
	};


}//namespace Rich


#endif // RichPlayer_h__
