#ifndef ComPacket_h__
#define ComPacket_h__

#include "CppVersion.h"
#include "Core/IntegerType.h"

#include "PlatformThread.h"
#include "FastDelegate/FastDelegate.h"

#include "GameConfig.h"

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
	friend class ComEvaluator;
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
class  ComEvaluator : public ComLibrary
{
public:
	typedef ComProcFunc ProcFunc;

	TINY_API ComEvaluator();
	TINY_API ~ComEvaluator();

	TINY_API static uint32 WriteBuffer( SocketBuffer& buffer , IComPacket* cp );
	TINY_API static bool   ReadBuffer( SocketBuffer& buffer , IComPacket* cp );

	template< class GamePacketT , class T , class TFunc >
	bool setWorkerFunc( T* processer, TFunc func, TFunc funcSocket );
	template< class GamePacketT , class T , class TFunc >
	bool setWorkerFunc( T* processer, TFunc func, void* );
	template< class GamePacketT , class T , class TFunc >
	bool setUserFunc( T* processer , TFunc func);

	TINY_API void  removeProcesserFunc( void* processer );
	TINY_API void  procCommand( ComVisitor& visitor );
	TINY_API void  procCommand();
	TINY_API void  procCommand(IComPacket* cp);

	TINY_API bool  evalCommand( SocketBuffer& buffer, int group = -1, void* userData = nullptr );
	TINY_API void  execCommand( IComPacket* cp , int group = -1 , void* userData = nullptr );

	TINY_API IComPacket* readNewPacket(SocketBuffer& buffer, int group = -1, void* userData = nullptr);


	template< class GamePacketT >
	void addFactory(){ addFactoryInternal<GamePacketT>(); }

private:

	struct ICPFactory
	{
		ICPFactory()
			:userProcesser(NULL)
			, workerProcesser(NULL) {}

		virtual IComPacket* createCom() = 0;
		unsigned id;
		void*    userProcesser;
		void*    workerProcesser;
		ProcFunc userFunc;         //app thread;
		ProcFunc workerFunc;       //app thread;
		ProcFunc workerFuncSocket; //socket thread;
	};

	template< class GamePacketT >
	ICPFactory* addFactoryInternal();

	TINY_API ICPFactory* findFactory(ComID com);

	template < class GamePacketT >
	struct CPFactory : public ICPFactory
	{
		CPFactory(){  id = GamePacketT::PID;  }
		virtual IComPacket* createCom(){   return new GamePacketT;  }
	};


	typedef std::unordered_map< ComID , ICPFactory* > CPFactoryMap;
	struct UserCom 
	{
		ICPFactory* factory;
		IComPacket* cp;
	};
	typedef std::list< UserCom > ComPacketList;

	CPFactoryMap  mCPFactoryMap;
	NET_MUTEX( mMutexCPFactoryMap )
	ComPacketList mProcCPList;
	NET_MUTEX( mMutexProcCPList )
};


template< class GamePacketT , class T , class TFunc >
bool ComEvaluator::setUserFunc( T* processer , TFunc func)
{
	NET_MUTEX_LOCK( mMutexCPFactoryMap );
	ICPFactory* factory = addFactoryInternal< GamePacketT >();
	if ( !factory )
		return false;

	factory->userProcesser = processer;

	if (func)
	{
		factory->userFunc.bind(processer, func);
	}
	return true;
}

template< class GamePacketT , class T , class TFunc >
bool ComEvaluator::setWorkerFunc( T* processer, TFunc func, void* )
{
	NET_MUTEX_LOCK( mMutexCPFactoryMap );
	ICPFactory* factory = addFactoryInternal< GamePacketT >();
	if ( !factory )
		return false;

	factory->workerProcesser = processer;

	if (func)
	{
		factory->workerFunc.bind(processer, func);
	}
	return true;
}

template< class GamePacketT , class T , class TFunc >
bool ComEvaluator::setWorkerFunc( T* processer, TFunc func, TFunc funcSocket )
{
	NET_MUTEX_LOCK( mMutexCPFactoryMap );
	ICPFactory* factory = addFactoryInternal< GamePacketT >();
	if ( !factory )
		return false;

	factory->workerProcesser = processer;

	if (func)
	{
		factory->workerFunc.bind(processer, func);
	}

	if (funcSocket)
	{
		factory->workerFuncSocket.bind(processer, funcSocket);
	}
	return true;
}

template< class GamePacketT >
ComEvaluator::ICPFactory* ComEvaluator::addFactoryInternal()
{
	NET_MUTEX_LOCK( mMutexCPFactoryMap );
	ICPFactory* factory = findFactory( GamePacketT::PID );
	if ( factory == nullptr )
	{
		factory = new CPFactory< GamePacketT >;
		mCPFactoryMap.insert( std::make_pair( GamePacketT::PID , factory ) );
	}
	return factory;
}


#endif // ComPacket_h__