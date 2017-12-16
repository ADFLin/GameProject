#ifndef RichLevel_h__
#define RichLevel_h__

#include "RichBase.h"

#include "RichWorld.h"
#include "RichPlayerTurn.h"

#include "DataStructure/IntrList.h"



namespace Rich
{

	class Level : public IObjectQuery
	{
	public:
		Level();
		void    init();

		World&  getWorld(){ return mWorld; }

		void    tick();
		void    restart();

		Player* getActivePlayer()
		{
			if ( mIdxActive == -1 )
				return nullptr;
			return mPlayerVec[ mIdxActive ];
		}

		void    runPlayerMove( int movePower );

		Player* createPlayer();
		void    destroyPlayer( Player* player );
		void    nextActivePlayer();

		void    updateTurn();

		Player* getPlayer( ActorId id )
		{
			if ( id >= mPlayerVec.size() )
				return nullptr;
			return mPlayerVec[ id ];
		}

		friend class Scene;
		typedef std::vector< Player* > PlayerVec;
		std::vector< Player* > mPlayerVec;
		typedef TIntrList< 
			ActorComp , MemberHook< ActorComp , &ActorComp::levelHook > , PointerType 
		> ActorList;
		ActorList     mActors;
		World         mWorld;
		PlayerTurn    mTurn;
		int           mIdxActive;
	};

}//namespace Rich

#endif // RichLevel_h__