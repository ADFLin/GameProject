#ifndef GamePlayer_h__
#define GamePlayer_h__

#include "GameControl.h"
#include "TTable.h"
#include "UserProfile.h"

#include <map>

typedef unsigned PlayerId;
typedef int      SlotId;
#define ERROR_PLAYER_ID  PlayerId(-1)
#define ERROR_SLOT_ID    SlotId(-1)

enum PlayerType
{
	PT_PLAYER     ,
	PT_COMPUTER   ,
	PT_SPECTATORS ,

	PT_UNKNOW ,
};

class AIBase
{
public:
	virtual IActionInput* getActionInput() = 0;
};

struct PlayerInfo
{ 
	FixString< MAX_PLAYER_NAME_LENGTH > name;
	PlayerType type;
	PlayerId   playerId;
	SlotId     slot;
	unsigned   actionPort;
	unsigned   ping;

	PlayerInfo()
	{
		actionPort = ERROR_ACTION_PORT;
		playerId   = ERROR_PLAYER_ID;
		slot       = -1;
		type       = PT_UNKNOW;
	}

};

class  GamePlayer
{	
public:
	GamePlayer(){ mInfo = NULL ; mAI = NULL; }
	void        init( PlayerInfo& info ){  mInfo = &info;  }
	PlayerId    getId()        { return getInfo().playerId; }
	PlayerType  getType()      { return getInfo().type; }
	char const* getName()      { return getInfo().name; }
	unsigned    getActionPort(){ return getInfo().actionPort;  }
	SlotId      getSlot()      { return getInfo().slot; }

	PlayerInfo& getInfo(){ return *mInfo; }
	void        setInfo( PlayerInfo const& info ){ *mInfo = info; }

	void        setAI( AIBase* ai ){ mAI = ai; }
	AIBase*     getAI(){ return mAI;  }
	
protected:
	PlayerInfo* mInfo;
	AIBase*     mAI;
};


class  IPlayerManager
{
public:
	virtual ~IPlayerManager(){}
	virtual PlayerId     getUserID() = 0;
	virtual GamePlayer*  getPlayer( PlayerId id ) = 0;
	virtual size_t       getPlayerNum() = 0;

	class Iterator
	{
	public:
		bool          haveMore()  { return numCount >= 0 ; }
		GamePlayer*   getElement(){ return player; }
		GAME_API void goNext();

	private:
		GAME_API Iterator( IPlayerManager* _mgr );
		GamePlayer*     player;
		IPlayerManager* mgr;
		int             numCount;
		friend class IPlayerManager;
	};

	Iterator     getIterator(){ return Iterator( this ); }
	GamePlayer*  getUser()
	{ 
		PlayerId id = getUserID();
		return id == ERROR_PLAYER_ID ? NULL : getPlayer( id );
	}

};

template< class T > 
class SimplePlayerManagerT : public IPlayerManager
{
	typedef T PlayerType;
public:
	SimplePlayerManagerT(){ mUserID = ERROR_PLAYER_ID; }
	~SimplePlayerManagerT()
	{
		for( PlayerTable::iterator iter = mPlayerTable.begin(); 
			iter != mPlayerTable.end() ; ++iter )
		{
			delete iter->second;
		}
	}

	PlayerType* getPlayer( PlayerId id )
	{
		PlayerTable::iterator iter = mPlayerTable.find( id );
		if ( iter != mPlayerTable.end() )
			return iter->second;
		return NULL;
	}

	
	bool    removePlayer( PlayerId id )
	{
		if ( mUserID == id )
			return false;
		PlayerTable::iterator iter = mPlayerTable.find( id );
		if ( iter == mPlayerTable.end() )
			return false;
		delete iter->second;
		mPlayerTable.erase( iter );
		return true;
	}

	PlayerType* createPlayer( unsigned id )
	{
		PlayerType* player = new PlayerType();

		PlayerType* oldPlayer = getPlayer( id );
		if ( oldPlayer )
			delete oldPlayer;

		mPlayerTable[ id ] = player;

		player->init( mPlayerInfo[ id ] );
		player->getInfo().name.format( "Player%u" , id );
		player->getInfo().playerId = id;

		return player;
	}

	size_t        getPlayerNum(){  return mPlayerTable.size();  }
	PlayerId      getUserID(){  return mUserID;  }
	void          setUserID( PlayerId id ){ mUserID = id; }

protected:
	typedef std::map< PlayerId , PlayerType* > PlayerTable;
	PlayerTable mPlayerTable;
	PlayerId    mUserID;
	PlayerInfo  mPlayerInfo[ MAX_PLAYER_NUM ];
};


class LocalPlayerManager : public SimplePlayerManagerT< GamePlayer >
{
public:

};




#endif // GamePlayer_h__