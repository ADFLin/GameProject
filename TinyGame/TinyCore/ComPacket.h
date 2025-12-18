#ifndef ComPacket_h__
#define ComPacket_h__

#include "CppVersion.h"
#include "Core/IntegerType.h"

#include "PlatformThread.h"
#include "FastDelegate/FastDelegate.h"

#include "GameConfig.h"
#include "PacketFactory.h"
#include "PacketDispatcher.h"

#include <unordered_map>
#include <list>
#include <cassert>

class SocketBuffer;
typedef uint32 ComID;


class ComException : public std::exception
{
public:
	STD_EXCEPTION_CONSTRUCTOR_WITH_WHAT( ComException )

	ComID com;
};

class  IComPacket
{
public:

	IComPacket( ComID com )
		: mId( com ) , mGroup( -1 ) , mUserData( nullptr )
	{
	}
	virtual ~IComPacket(){}

	TINY_API void writeBuffer( SocketBuffer& buffer );
	TINY_API void readBuffer( SocketBuffer& buffer );

	ComID  getID(){ return mId; }
	int    getGroup() { return mGroup; }
	void*  getUserData() { return mUserData; }

	template < class GamePacketT >
	GamePacketT* cast()
	{
		assert( mId == GamePacketT::PID );
		return static_cast< GamePacketT* >( this );
	}

	template < class GamePacketT >
	GamePacketT* safeCast()
	{
		if ( mId != GamePacketT::PID )
			return NULL;
		return static_cast< GamePacketT* >( this );
	}

	void     setUserData(void* data) { mUserData = data; }

protected:
	friend class PacketFactory;

	virtual void doWrite( SocketBuffer& buffer ) = 0;
	virtual void doRead( SocketBuffer& buffer ) = 0;

	ComID   mId;
	void*   mUserData;
	int     mGroup;
};


enum ComVisitResult
{
	CVR_RESERVE ,
	CVR_DISCARD ,
	CVR_TAKE    ,
};

class ComVisitor
{
public:
	virtual ~ComVisitor(){}
	virtual ComVisitResult visitComPacket( IComPacket* cp ){ return CVR_DISCARD; }
};

class ComLibrary
{




};

typedef fastdelegate::FastDelegate< void ( IComPacket*) > ComProcFunc;

// ========================================
// ComEvaluator: 组合 PacketFactory 和 PacketDispatcher
// ========================================
class  ComEvaluator : public ComLibrary
{
public:
	typedef ComProcFunc ProcFunc;

	TINY_API ComEvaluator();
	TINY_API ComEvaluator(PacketFactory& factory); // 使用外部 factory
	TINY_API ~ComEvaluator();

	// 委托给 PacketDispatcher（需要 PacketFactory 协助注册）
	template<class GamePacketT , class T , class TFunc >
	bool setWorkerFunc( T* processer, TFunc func, TFunc funcSocket )
	{
		return mDispatcher.setWorkerFunc<GamePacketT>(processer, func, funcSocket, mFactory);
	}

	template<class GamePacketT , class T , class TFunc >
	bool setWorkerFunc( T* processer, TFunc func, void* dummy )
	{
		return mDispatcher.setWorkerFunc<GamePacketT>(processer, func, dummy, mFactory);
	}

	template<class GamePacketT , class T , class TFunc >
	bool setUserFunc( T* processer , TFunc func)
	{
		return mDispatcher.setUserFunc<GamePacketT>(processer, func, mFactory);
	}

	TINY_API void  removeProcesserFunc( void* processer );

	// 委托给 PacketDispatcher
	TINY_API void  procCommand( ComVisitor& visitor );
	TINY_API void  procCommand();
	TINY_API void  procCommand(IComPacket* cp);

	// 使用两个组件
	TINY_API bool  evalCommand( SocketBuffer& buffer, int group = -1, void* userData = nullptr );

	TINY_API IComPacket* readNewPacket(SocketBuffer& buffer, int group = -1, void* userData = nullptr);

	template<class GamePacketT >
	void addFactory() { mFactory->addFactory<GamePacketT>(); }

private:
	PacketFactory*   mFactory;      // 可以指向外部或内部 factory
	PacketDispatcher mDispatcher;
};


#endif // ComPacket_h__