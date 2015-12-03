#ifndef RichPlayer_h__
#define RichPlayer_h__

#include "RichBase.h"
#include "RichWorld.h"

#include "Singleton.h"

#include <functional>

namespace Rich
{
	class Player;
	class LandCell;
	class IController;

	class Player : public Actor
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

		World& getWorld(){ return *mWorld; }
		bool   changePos( MapCoord const& pos );

		void    setRole( RoleId id ){ mRoleId = id; }
		RoleId  getRoleId(){ return mRoleId; }


		int     getMovePower();
		MapCoord const& getPos() const { return mPos; }


		int            getMoney() const { return mMoney; }
		void           modifyMoney( int delta )
		{
			mMoney += delta;
		}
		IController& getController(){ assert( mController ); return *mController; }
		void         setController( IController& controller ){ mController = &controller; }

		bool  isActive();

		void  changeState( State state )
		{
			if ( mState == state )
				return;
			mState = state;
		}


	public:

		int          mMovePower;
		World*       mWorld;
		RoleId       mRoleId;
		State        mState;
		int          mMoney;
		int          mCardPoint;
		IController* mController;
	};


}//namespace Rich


#endif // RichPlayer_h__
