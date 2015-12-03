#include "TinyGamePCH.h"
#include "ComPacket.h"

#include "CoreShare.h"
#include "SocketBuffer.h"

#include "THolder.h"

struct PacketHeader
{
	ComID   type;
	uint32  size;
};
static unsigned const ComPacketHeaderSize = sizeof( PacketHeader );


ComEvaluator::ComEvaluator()
{

}


bool ComEvaluator::evalCommand( SBuffer& buffer , ComConnection* con )
{
	if ( !buffer.getAvailableSize() )
		return false;
	
	TPtrHolder< IComPacket > cp;
	ComID comID;

	size_t oldUseSize = buffer.getUseSize();
	try
	{
		buffer.take( comID );

		ICPFactory* factory = findFactory( comID );
		if ( factory == NULL )
			throw ComException( "Can't find com" );

		cp.reset( factory->createCom() );
		cp->mConnection = con;

		if ( !takeBuffer( cp.get() , buffer )  )
			return false;

		if ( factory->workerFunSocket )
		{
			( factory->workerFunSocket )( cp.get() );
		}

		if (  factory->workerFun || factory->userFun )
		{
			MUTEX_LOCK( mMutexProcCPList );

			UserCom com;
			com.factory   = factory;
			com.cp        = cp.release();
			mProcCPList.push_back( com );
		}

	}
	catch ( ComException& e )
	{
		Msg( "%s (id =%u)" ,  e.what() , comID );
		if ( cp.get() )
			e.com = cp->getID();
		buffer.setUseSize( oldUseSize );
		throw e;
	}

	return true;
}


void ComEvaluator::execCommand( IComPacket* cp )
{
	ICPFactory* factory = findFactory( cp->getID() );

	if ( factory == NULL )
	{
		//::Msg( "Can't exec ComPacket id = %d " , cp->getID() );
		return;
	}

	cp->mConnection = NULL;
	if ( factory->userFun )
		( factory->userFun )( cp );

	if ( factory->workerFun )
		( factory->workerFun )( cp );
}

ComEvaluator::~ComEvaluator()
{
	for( CPFactoryMap::iterator iter = mCPFactoryMap.begin();
		iter != mCPFactoryMap.end() ; ++iter )
	{
		delete iter->second;
	}
}

ComEvaluator::ICPFactory* ComEvaluator::findFactory( ComID com )
{
	CPFactoryMap::iterator iter = mCPFactoryMap.find( com );
	if ( iter != mCPFactoryMap.end() )
		return iter->second;
	return NULL;
}

unsigned ComEvaluator::fillBuffer( IComPacket* cp , SBuffer& buffer )
{
	size_t oldSize = buffer.getFillSize();	
	unsigned size = 0;
	try
	{
		buffer.fill( cp->mId );
		buffer.fill( size );

		cp->fillBuffer( buffer );
	}
	catch ( BufferException& e )
	{
		buffer.setFillSize( oldSize );
		Msg( "%s(%d) : %s" , __FILE__ , __LINE__ ,e.what() );
		throw BufferException( "Can't fill buffer: Buffer has not space" );
	}

	size = unsigned( buffer.getFillSize() - oldSize );
	unsigned* ptr = (unsigned*)( buffer.getData() + oldSize + sizeof( ComID ) );
	*ptr = size;

	return size;
}

bool ComEvaluator::takeBuffer( IComPacket* cp , SBuffer& buffer )
{
	unsigned takeSize;
	buffer.take( takeSize );

	takeSize -= ComPacketHeaderSize;
	if ( takeSize > buffer.getAvailableSize() )
	{
		::Msg( "Error packet format : size isn't enough" );
		buffer.shiftUseSize( -int(ComPacketHeaderSize) );
		return false;
	}

	unsigned oldSize = (unsigned)buffer.getAvailableSize();
	cp->takeBuffer( buffer );

	if ( takeSize != oldSize - buffer.getAvailableSize() )
		throw ComException( "Error packet format" );

	return true;
}


void ComEvaluator::procCommand()
{
	MUTEX_LOCK( mMutexProcCPList );

	for( ComPacketList::iterator iter = mProcCPList.begin();
		iter != mProcCPList.end() ;  ++iter )
	{
		UserCom& com = *iter;
		assert( com.factory && com.factory->id == com.cp->getID() );

		if ( com.factory->workerFun )
			( com.factory->workerFun )( com.cp );

		if ( com.factory->userFun )
			( com.factory->userFun )( com.cp );

		delete com.cp;
	}

	mProcCPList.clear();
}

void ComEvaluator::procCommand(  ComVisitor& visitor )
{
	MUTEX_LOCK( mMutexProcCPList );

	for( ComPacketList::iterator iter = mProcCPList.begin();
		iter != mProcCPList.end() ;  )
	{
		UserCom& com = *iter;
		assert( com.factory && com.factory->id == com.cp->getID() );

		if ( com.factory->workerFun )
			( com.factory->workerFun )( com.cp );

		if ( com.factory->userFun )
			( com.factory->userFun )( com.cp );

		switch ( visitor.visit( com.cp ) )
		{
		case CVR_DISCARD: 
			delete com.cp;
			iter = mProcCPList.erase( iter );
			break;
		case CVR_RESERVE:
			++iter;
			break;
		case CVR_TAKE:
			iter = mProcCPList.erase( iter );
			break;
		}
	}
}


void ComEvaluator::removeProcesserFun( void* processer )
{
	for( CPFactoryMap::iterator iter = mCPFactoryMap.begin();
		iter != mCPFactoryMap.end() ; ++iter )
	{
		ICPFactory* factory = iter->second;

		if ( factory->workerProcesser == processer )
		{
			factory->workerProcesser = NULL;
			factory->workerFun.clear();
			factory->workerFunSocket.clear();
		}
		if ( factory->userProcesser == processer )
		{
			factory->userProcesser = NULL;
			factory->userFun.clear();
		}
	}
}

void IComPacket::fillBuffer( SBuffer& buffer )
{
	doFill( buffer );
}

void IComPacket::takeBuffer( SBuffer& buffer )
{
	doTake( buffer );
}
