#ifndef ComPacket_h__
#define ComPacket_h__

#include "CppVersion.h"
#include "Core/IntegerType.h"

#include "PlatformThread.h"
#include "FastDelegate/FastDelegate.h"

#include <map>
#include <list>
#include <cassert>

#include "GameConfig.h"

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

	template < class GamePacket >
	GamePacket* cast()
	{
		assert( mId == GamePacket::PID );
		return static_cast< GamePacket* >( this );
	}

	template < class GamePacket >
	GamePacket* safeCast()
	{
		if ( mId != GamePacket::PID )
			return NULL;
		return static_cast< GamePacket* >( this );
	}

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

typedef fastdelegate::FastDelegate< void ( IComPacket*) > ComProcFun;
class  ComEvaluator : public ComLibrary
{
public:
	typedef ComProcFun ProcFun;

	TINY_API ComEvaluator();
	TINY_API ~ComEvaluator();

	TINY_API static unsigned WriteBuffer( SocketBuffer& buffer , IComPacket* cp );
	TINY_API static bool     ReadBuffer( SocketBuffer& buffer , IComPacket* cp );

	template< class GamePacket , class T , class TFun >
	bool setWorkerFun( T* processer, TFun func, TFun funSocket );
	template< class GamePacket , class T , class TFun >
	bool setWorkerFun( T* processer, TFun func, void* );
	template< class GamePacket , class T , class TFun >
	bool setUserFun( T* processer , TFun func);

	TINY_API void  removeProcesserFun( void* processer );
	TINY_API void  procCommand( ComVisitor& visitor );
	TINY_API void  procCommand();

	TINY_API bool  evalCommand( SocketBuffer& buffer, int group = -1, void* userData = nullptr );
	TINY_API void  execCommand( IComPacket* cp , int group = -1 , void* userData = nullptr );

private:
	struct ICPFactory
	{
		ICPFactory()
			:userProcesser(NULL)
			,workerProcesser( NULL ){}

		virtual IComPacket* createCom() = 0;
		unsigned id;
		void*    userProcesser;
		void*    workerProcesser;
		ProcFun  userFun;         //app thread;
		ProcFun  workerFun;       //app thread;
		ProcFun  workerFunSocket; //socket thread;
	};


	template < class GamePacket >
	struct CPFactory : public ICPFactory
	{
		CPFactory(){  id = GamePacket::PID;  }
		virtual IComPacket* createCom(){   return new GamePacket;  }
	};

	template< class GamePacket > 
	ICPFactory* addFactory();
	TINY_API ICPFactory* findFactory( ComID com );

	typedef std::map< ComID , ICPFactory* > CPFactoryMap;
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


template< class GamePacket , class T , class TFun >
bool ComEvaluator::setUserFun( T* processer , TFun func)
{
	NET_MUTEX_LOCK( mMutexCPFactoryMap );
	ICPFactory* factory = addFactory< GamePacket >();
	if ( !factory )
		return false;

	factory->userProcesser = processer;

	if (func)
		factory->userFun.bind( processer , func);

	return true;
}

template< class GamePacket , class T , class TFun >
bool ComEvaluator::setWorkerFun( T* processer, TFun func, void* )
{
	NET_MUTEX_LOCK( mMutexCPFactoryMap );
	ICPFactory* factory = addFactory< GamePacket >();
	if ( !factory )
		return false;

	factory->workerProcesser = processer;

	if (func)
		factory->workerFun.bind( processer , func);
	return true;
}

template< class GamePacket , class T , class TFun >
bool ComEvaluator::setWorkerFun( T* processer, TFun func, TFun funSocket )
{
	NET_MUTEX_LOCK( mMutexCPFactoryMap );
	ICPFactory* factory = addFactory< GamePacket >();
	if ( !factory )
		return false;

	factory->workerProcesser = processer;

	if (func)
		factory->workerFun.bind( processer , func);

	if ( funSocket )
		factory->workerFunSocket.bind( processer , funSocket );

	return true;
}

template< class GamePacket >
ComEvaluator::ICPFactory* ComEvaluator::addFactory()
{
	NET_MUTEX_LOCK( mMutexCPFactoryMap );
	ICPFactory* factory = findFactory( GamePacket::PID );
	if ( factory == nullptr )
	{
		factory = new CPFactory< GamePacket >;
		mCPFactoryMap.insert( std::make_pair( GamePacket::PID , factory ) );
	}
	return factory;
}


#endif // ComPacket_h__