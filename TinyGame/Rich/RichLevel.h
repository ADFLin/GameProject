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
		void    reset();
		World&  getWorld(){ return mWorld; }

		void    tick();
		void    start();

		void   runLogicAsync();
		void   resumeLogic();

		Player* getActivePlayer()
		{
			if ( mIdxActive == -1 )
				return nullptr;
			return mPlayerList[ mIdxActive ];
		}

		void    runPlayerMove( int movePower );
		TArray<Player*> const& getPlayerList() { return mPlayerList; }
		GameOptions const& getGameOptions() override { return mGameOptions; }

		Player* createPlayer();
		void    destroyPlayer( Player* player );
		void    runPlayerTurn(Player& player);
		Player* nextTurnPlayer();

		Player* getPlayer( ActorId id )
		{
			if ( id >= mPlayerList.size() )
				return nullptr;
			return mPlayerList[ id ];
		}

		friend class Scene;
		typedef TArray< Player* > PlayerVec;
		TArray< Player* > mPlayerList;
		typedef TIntrList< 
			ActorComp , MemberHook< ActorComp , &ActorComp::levelHook > , PointerType 
		> ActorList;
		ActorList     mActors;
		World         mWorld;
		PlayerTurn    mTurn;
		int           mIdxActive;

		GameOptions mGameOptions;

		Coroutines::ExecutionHandle mRunHandle;


	};

}//namespace Rich

#endif // RichLevel_h__