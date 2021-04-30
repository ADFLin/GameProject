#ifndef GamePlayer_h__
#define GamePlayer_h__

#include "GameControl.h"
#include "DataStructure/TTable.h"
#include "UserProfile.h"

#include <map>

using PlayerId = unsigned;
using SlotId = int;
#define ERROR_PLAYER_ID  PlayerId(-1)
#define SERVER_PLAYER_ID PlayerId(-2)
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
	InlineString< MAX_PLAYER_NAME_LENGTH > name;
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
	GamePlayer()
	{ 
		mInfo = nullptr; 
		mAI = nullptr; 
	}

	void        init( PlayerInfo& info ){  mInfo = &info;  }
	PlayerId    getId()        { return mInfo->playerId; }
	PlayerType  getType()      { return mInfo->type; }
	char const* getName()      { return mInfo->name; }
	unsigned    getActionPort(){ return mInfo->actionPort;  }
	SlotId      getSlot()      { return mInfo->slot; }


	void        setType(PlayerType type) { mInfo->type = type; }
	void        setSlot(SlotId slot) { mInfo->slot = slot; }
	void        setActionPort(unsigned port) { mInfo->actionPort = port; }

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
		bool          haveMore() const { return mPlayer != nullptr; }
		GamePlayer*   getElement(){ return mPlayer; }
		TINY_API void goNext();

		operator bool() const { return haveMore(); }

		Iterator& operator++()  { goNext(); return *this; }
		Iterator  operator++(int) { Iterator temp = *this; goNext(); return temp; }

	private:
		TINY_API Iterator( IPlayerManager* inManager );
		GamePlayer*     mPlayer;
		IPlayerManager* mManager;
		int             mPlayerCount;
		friend class IPlayerManager;
	};

	Iterator     createIterator(){ return Iterator( this ); }
	GamePlayer*  getUser()
	{ 
		PlayerId id = getUserID();
		return id == ERROR_PLAYER_ID ? nullptr : getPlayer( id );
	}

};

template< class T > 
class SimplePlayerManagerT : public IPlayerManager
{
	using PlayerType = T;
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

	PlayerType* getPlayer( PlayerId id ) override
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

	size_t        getPlayerNum() override{  return mPlayerTable.size();  }
	PlayerId      getUserID() override{  return mUserID;  }
	void          setUserID( PlayerId id ){ mUserID = id; }

protected:
	using PlayerTable = std::map< PlayerId , PlayerType* >;
	PlayerTable mPlayerTable;
	PlayerId    mUserID;
	PlayerInfo  mPlayerInfo[ MAX_PLAYER_NUM ];
};


class LocalPlayerManager : public SimplePlayerManagerT< GamePlayer >
{
public:

};




#endif // GamePlayer_h__