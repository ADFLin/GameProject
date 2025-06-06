#include "TinyGamePCH.h"
#include "ComPacket.h"

#include "GameShare.h"
#include "SocketBuffer.h"

#include "Holder.h"

struct PacketHeader
{
	ComID   type;
	uint32  size;
};
static unsigned const ComPacketHeaderSize = sizeof( PacketHeader );


ComEvaluator::ComEvaluator()
{

}


bool ComEvaluator::evalCommand( SocketBuffer& buffer , int group , void* userData )
{
	if ( !buffer.getAvailableSize() )
		return false;
	
	TPtrHolder< IComPacket > cp;
	ComID comID;

	size_t oldUseSize = buffer.getUseSize();
	try
	{
		buffer.take( comID );

		ICPFactory* factory;
		{
			NET_MUTEX_LOCK( mMutexCPFactoryMap );
			factory = findFactory( comID );
		}
		if ( factory == NULL )
			throw ComException( "Can't find com" );

		cp.reset( factory->createCom() );
		cp->mGroup = group;
		cp->mUserData = userData;

		if ( !ReadBuffer( buffer , cp.get() )  )
			return false;

		if ( factory->workerFuncSocket )
		{
			( factory->workerFuncSocket )( cp.get() );
		}

		if (  factory->workerFunc || factory->userFunc )
		{
			NET_MUTEX_LOCK( mMutexProcCPList );

			UserCom com;
			com.factory   = factory;
			com.cp        = cp.release();
			mProcCPList.push_back( com );
		}

	}
	catch ( ComException& e )
	{
		LogMsg( "%s (id =%u)" ,  e.what() , comID );
		if ( cp.get() )
			e.com = cp->getID();
		buffer.setUseSize( oldUseSize );
		throw e;
	}

	return true;
}


void ComEvaluator::execCommand( IComPacket* cp , int group , void* userData )
{
	ICPFactory* factory = findFactory( cp->getID() );

	if ( factory == NULL )
	{
		//::Msg( "Can't exec ComPacket id = %d " , cp->getID() );
		return;
	}

	cp->mGroup = group;
	cp->mUserData = userData;

	if ( factory->userFunc )
		( factory->userFunc )( cp );

	if ( factory->workerFunc )
		( factory->workerFunc )( cp );
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

uint32 ComEvaluator::WriteBuffer( SocketBuffer& buffer , IComPacket* cp )
{
	size_t oldSize = buffer.getFillSize();	
	uint32 size = 0;
	try
	{
		buffer.fill( cp->mId );
		buffer.fill( size );

		cp->writeBuffer( buffer );
	}
	catch ( BufferException& e )
	{
		buffer.setFillSize( oldSize );
		LogMsg( "%s(%d) : %s" , __FILE__ , __LINE__ ,e.what() );
		throw BufferException( "Can't fill buffer: Buffer has not space" );
	}

	size = unsigned( buffer.getFillSize() - oldSize );
	unsigned* ptr = (unsigned*)( buffer.getData() + oldSize + sizeof( ComID ) );
	*ptr = size;

	return size;
}

bool ComEvaluator::ReadBuffer( SocketBuffer& buffer , IComPacket* cp )
{
	uint32 takeSize;
	buffer.take( takeSize );

	takeSize -= ComPacketHeaderSize;
	if ( takeSize > buffer.getAvailableSize() )
	{
		LogMsg( "Error packet format : size isn't enough" );
		buffer.shiftUseSize( -int(ComPacketHeaderSize) );
		return false;
	}

	uint32 oldSize = (uint32)buffer.getAvailableSize();
	cp->readBuffer( buffer );

	if ( takeSize != oldSize - (uint32)buffer.getAvailableSize() )
		throw ComException( "Error packet format" );

	return true;
}


void ComEvaluator::procCommand()
{
	NET_MUTEX_LOCK( mMutexProcCPList );

	for( ComPacketList::iterator iter = mProcCPList.begin();
		iter != mProcCPList.end() ;  ++iter )
	{
		UserCom& com = *iter;
		assert( com.factory && com.factory->id == com.cp->getID() );

		if ( com.factory->workerFunc )
			( com.factory->workerFunc )( com.cp );

		if ( com.factory->userFunc )
			( com.factory->userFunc )( com.cp );

		delete com.cp;
	}

	mProcCPList.clear();
}

void ComEvaluator::procCommand(  ComVisitor& visitor )
{
	NET_MUTEX_LOCK( mMutexProcCPList );

	for( ComPacketList::iterator iter = mProcCPList.begin();
		iter != mProcCPList.end() ;  )
	{
		UserCom& com = *iter;
		assert( com.factory && com.factory->id == com.cp->getID() );

		if (com.factory->workerFunc)
		{
			(com.factory->workerFunc)(com.cp);
		}

		if (com.factory->userFunc)
		{
			(com.factory->userFunc)(com.cp);
		}

		switch ( visitor.visitComPacket(com.cp) )
		{
		case CVR_DISCARD: 
			delete com.cp;
			iter = mProcCPList.erase( iter );
			break;
		case CVR_TAKE:
			iter = mProcCPList.erase( iter );
			break;
		case CVR_RESERVE:
			++iter;
			break;
		}
	}
}


void ComEvaluator::removeProcesserFunc( void* processer )
{
	NET_MUTEX_LOCK( mMutexCPFactoryMap );
	for( CPFactoryMap::iterator iter = mCPFactoryMap.begin();
		iter != mCPFactoryMap.end() ; ++iter )
	{
		ICPFactory* factory = iter->second;

		if ( factory->workerProcesser == processer )
		{
			factory->workerProcesser = NULL;
			factory->workerFunc.clear();
			factory->workerFuncSocket.clear();
		}
		if ( factory->userProcesser == processer )
		{
			factory->userProcesser = NULL;
			factory->userFunc.clear();
		}
	}
}

void IComPacket::writeBuffer( SocketBuffer& buffer )
{
	doWrite( buffer );
}

void IComPacket::readBuffer( SocketBuffer& buffer )
{
	doRead( buffer );
}
