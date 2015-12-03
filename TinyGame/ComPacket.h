#ifndef ComPacket_h__
#define ComPacket_h__

#include <map>
#include <list>
#include <cassert>

#include "Thread.h"
#include "FastDelegate/FastDelegate.h"

#include "IntegerType.h"
#include "GameConfig.h"

class SBuffer;
typedef uint32 ComID;

class ComException : public std::exception
{
public:
	ComException( char const* what )
		:std::exception( what ){}
	ComID com;
};

class IComPacket;

class ComConnection
{

};

class  IComPacket
{
public:

	IComPacket( ComID com )
		: mId( com )
		, mConnection( NULL ){}
	virtual ~IComPacket(){}

	GAME_API void fillBuffer( SBuffer& buffer );
	GAME_API void takeBuffer( SBuffer& buffer );

	ComID           getID()        { return mId; }
	ComConnection*  getConnection(){ return mConnection; }


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
	virtual void doFill( SBuffer& buffer ) = 0;
	virtual void doTake( SBuffer& buffer ) = 0;

	ComID          mId;
	ComConnection* mConnection;
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
	virtual ComVisitResult visit( IComPacket* cp ){ return CVR_DISCARD; }
};

class ComLibrary
{







};

typedef fastdelegate::FastDelegate< void ( IComPacket* ) > ComProcFun;
class  ComEvaluator : public ComLibrary
{
public:
	typedef ComProcFun ProcFun;

	ComEvaluator();
	~ComEvaluator();

	static GAME_API unsigned fillBuffer( IComPacket* cp , SBuffer& buffer );

	template< class GamePacket , class T , class Fun >
	bool setWorkerFun( T* processer, Fun fun , Fun funSocket );
	template< class GamePacket , class T , class Fun >
	bool setWorkerFun( T* processer, Fun fun , void* );
	template< class GamePacket , class T , class Fun >
	bool setUserFun( T* processer , Fun fun );

	GAME_API void     removeProcesserFun( void* processer );
	GAME_API void     procCommand( ComVisitor& visitor );
	GAME_API void     procCommand();
	GAME_API bool     takeBuffer( IComPacket* cp , SBuffer& buffer );
	
	GAME_API bool     evalCommand( SBuffer& buffer , ComConnection* con = NULL );
	GAME_API void     execCommand( IComPacket* cp );

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
	GAME_API ICPFactory* findFactory( ComID com );

	typedef std::map< ComID , ICPFactory* > CPFactoryMap;
	struct UserCom 
	{
		ICPFactory* factory;
		IComPacket* cp;
	};
	typedef std::list< UserCom > ComPacketList;

	CPFactoryMap  mCPFactoryMap;
	ComPacketList mProcCPList;
	DEFINE_MUTEX( mMutexProcCPList )
};


template< class GamePacket , class T , class Fun >
bool ComEvaluator::setUserFun( T* processer , Fun fun )
{
	ICPFactory* factory = addFactory< GamePacket >();
	if ( !factory )
		return false;

	factory->userProcesser = processer;

	if ( fun )
		factory->userFun.bind( processer , fun );

	return true;
}

template< class GamePacket , class T , class Fun >
bool ComEvaluator::setWorkerFun( T* processer, Fun fun , void* )
{
	ICPFactory* factory = addFactory< GamePacket >();
	if ( !factory )
		return false;

	factory->workerProcesser = processer;

	if ( fun )
		factory->workerFun.bind( processer , fun );
	return true;
}

template< class GamePacket , class T , class Fun >
bool ComEvaluator::setWorkerFun( T* processer, Fun fun , Fun funSocket )
{
	ICPFactory* factory = addFactory< GamePacket >();
	if ( !factory )
		return false;

	factory->workerProcesser = processer;

	if ( fun )
		factory->workerFun.bind( processer , fun );

	if ( funSocket )
		factory->workerFunSocket.bind( processer , funSocket );

	return true;
}

template< class GamePacket >
ComEvaluator::ICPFactory* ComEvaluator::addFactory()
{
	ICPFactory* factory = findFactory( GamePacket::PID );

	if ( !factory || factory->id != GamePacket::PID )
	{
		delete factory;

		factory = new CPFactory< GamePacket >;
		mCPFactoryMap.insert( std::make_pair( GamePacket::PID , factory ) );
	}
	return factory;
}


#endif // ComPacket_h__