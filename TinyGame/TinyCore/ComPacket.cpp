#include "TinyGamePCH.h"
#include "ComPacket.h"

#include "GameShare.h"
#include "SocketBuffer.h"

#include "Holder.h"

ComEvaluator::ComEvaluator()
	: mFactory(&GGamePacketFactory)
{

}

ComEvaluator::ComEvaluator(PacketFactory& factory)
	: mFactory(&factory)
{
}

ComEvaluator::~ComEvaluator()
{
}

bool ComEvaluator::evalCommand( SocketBuffer& buffer , int group , void* userData )
{
	if ( !buffer.getAvailableSize() )
		return false;
	
	TPtrHolder< IComPacket > cp;
	ComID comID = 0;

	size_t oldUseSize = buffer.getUseSize();
	try
	{
		// 先预读 ComID 用于错误报告
		if (buffer.getAvailableSize() >= sizeof(ComID))
		{
			buffer.take(comID);
			buffer.setUseSize(oldUseSize); // 回退，让 createPacketFromBuffer 重新读取
		}

		// 使用 PacketFactory 解析并创建 Packet
		cp.reset( mFactory->createPacketFromBuffer(buffer, group, userData) );
		if (!cp.get())
		{
			throw ComException( "Can't create packet from buffer" );
		}

		comID = cp->getID();
		if (mDispatcher.recvCommand(cp.get()))
		{
			cp.release();
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


IComPacket* ComEvaluator::readNewPacket(SocketBuffer& buffer, int group /*= -1*/, void* userData)
{
	// 委托给 PacketFactory 进行解析
	return mFactory->createPacketFromBuffer(buffer, group, userData);
}

void ComEvaluator::procCommand()
{
	mDispatcher.procCommand();
}

void ComEvaluator::procCommand(  ComVisitor& visitor )
{
	mDispatcher.procCommand(visitor);
}

void ComEvaluator::procCommand(IComPacket* cp)
{
	mDispatcher.procCommand(cp);
}

void ComEvaluator::removeProcesserFunc( void* processer )
{
	mDispatcher.removeProcesserFunc(processer);
}

void IComPacket::writeBuffer( SocketBuffer& buffer )
{
	doWrite( buffer );
}

void IComPacket::readBuffer( SocketBuffer& buffer )
{
	doRead( buffer );
}
