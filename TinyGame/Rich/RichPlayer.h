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
	class IController;

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

		bool   updateTile(MapCoord const& pos);

		World& getWorld(){ return *mOwnedWorld; }
		bool   changePos( MapCoord const& pos );

		void    setRole( RoleId id ){ mRoleId = id; }
		RoleId  getRoleId(){ return mRoleId; }

		int     getMovePower();

		int     getTotalMoney() const { return mMoney; }
		void    modifyMoney( int delta )
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
		World*       mOwnedWorld;
		RoleId       mRoleId;
		State        mState;
		int          mMoney;
		int          mCardPoint;
		IController* mController;
	};


}//namespace Rich


#endif // RichPlayer_h__
