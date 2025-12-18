#include "TinyGamePCH.h"
#include "PacketDispatcher.h"
#include "ComPacket.h"

// ========================================
// PacketDispatcher Implementation
// ========================================

PacketDispatcher::PacketDispatcher()
{
}

PacketDispatcher::~PacketDispatcher()
{
	// 清理未处理的packet
	NET_MUTEX_LOCK( mMutexProcCPList );
	for (auto& com : mProcCPList)
	{
		delete com.cp;
	}
	mProcCPList.clear();
}

PacketDispatcher::PacketHandler* PacketDispatcher::findHandler(ComID packetId)
{
	NET_RWLOCK_READ( mRWLockHandlerMap );
	auto iter = mHandlerMap.find(packetId);
	if (iter != mHandlerMap.end())
		return &iter->second;
	return nullptr;
}

bool PacketDispatcher::recvCommand(IComPacket* cp)
{
	PacketDispatcher::PacketHandler* handler = findHandler(cp->getID());

	if (handler)
	{
		if (handler->workerFuncSocket)
		{
			(handler->workerFuncSocket)(cp);
		}

		if (handler->workerFunc || handler->userFunc)
		{
			enqueue(cp, handler);
			return true;
		}

	}

	return false;
}

void PacketDispatcher::removeProcesserFunc(void* processer)
{
	NET_RWLOCK_WRITE( mRWLockHandlerMap );
	for (auto& pair : mHandlerMap)
	{
		PacketHandler& handler = pair.second;

		if (handler.workerProcesser == processer)
		{
			handler.workerProcesser = nullptr;
			handler.workerFunc.clear();
			handler.workerFuncSocket.clear();
		}
		if (handler.userProcesser == processer)
		{
			handler.userProcesser = nullptr;
			handler.userFunc.clear();
		}
	}
}

void PacketDispatcher::enqueue(IComPacket* packet, PacketHandler* handler)
{
	NET_MUTEX_LOCK( mMutexProcCPList );

	UserCom com;
	com.handler = handler;
	com.cp = packet;
	mProcCPList.push_back(com);
}

void PacketDispatcher::procCommand()
{
	NET_MUTEX_LOCK( mMutexProcCPList );

	for (auto& com : mProcCPList)
	{
		if (com.handler)
		{
			if (com.handler->workerFunc)
				(com.handler->workerFunc)(com.cp);

			if (com.handler->userFunc)
				(com.handler->userFunc)(com.cp);
		}

		delete com.cp;
	}

	mProcCPList.clear();
}

void PacketDispatcher::procCommand(ComVisitor& visitor)
{
	NET_MUTEX_LOCK( mMutexProcCPList );

	for (auto iter = mProcCPList.begin(); iter != mProcCPList.end(); )
	{
		UserCom& com = *iter;

		if (com.handler)
		{
			if (com.handler->workerFunc)
			{
				(com.handler->workerFunc)(com.cp);
			}

			if (com.handler->userFunc)
			{
				(com.handler->userFunc)(com.cp);
			}
		}

		switch (visitor.visitComPacket(com.cp))
		{
		case CVR_DISCARD:
			delete com.cp;
			iter = mProcCPList.erase(iter);
			break;
		case CVR_TAKE:
			iter = mProcCPList.erase(iter);
			break;
		case CVR_RESERVE:
			++iter;
			break;
		}
	}
}

void PacketDispatcher::procCommand(IComPacket* cp)
{
	auto handler = findHandler(cp->getID());
	if (handler && handler->userFunc)
	{
		(handler->userFunc)(cp);
	}
}
