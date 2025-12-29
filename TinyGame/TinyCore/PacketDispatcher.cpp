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

	for (auto& com : mDispatchList)
	{
		delete com.cp;
	}
	mDispatchList.clear();
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
	CHECK(IsInNetThread());

	PacketDispatcher::PacketHandler* handler = findHandler(cp->getID());

	if (handler)
	{
		if (handler->workerFuncSocket)
		{
			(handler->workerFuncSocket)(cp);
		}

		if (handler->workerFunc || handler->userHanlders.size())
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

		for (auto iter = handler.userHanlders.begin(); iter != handler.userHanlders.end(); )
		{
			if (iter->userProcesser == processer)
			{
				iter = handler.userHanlders.erase(iter);
			}
			else
			{
				++iter;
			}
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
	{
		NET_MUTEX_LOCK(mMutexProcCPList);
		mDispatchList.swap(mProcCPList);
	}

	for (auto& com : mDispatchList)
	{
		if (com.handler)
		{
			if (com.handler->workerFunc)
				(com.handler->workerFunc)(com.cp);

			if (!com.handler->userHanlders.empty())
			{
				for (auto& userHandler : com.handler->userHanlders)
				{
					(userHandler.func)(com.cp);
				}
			}
		}

		delete com.cp;
	}

	mDispatchList.clear();
}

void PacketDispatcher::procCommand(ComVisitor& visitor)
{
	{
		NET_MUTEX_LOCK(mMutexProcCPList);
		mDispatchList.swap(mProcCPList);
	}

	for (auto iter = mDispatchList.begin(); iter != mDispatchList.end(); )
	{
		UserCom& com = *iter;

		if (com.handler)
		{
			if (com.handler->workerFunc)
			{
				(com.handler->workerFunc)(com.cp);
			}

			if (!com.handler->userHanlders.empty())
			{
				for (auto& userHandler : com.handler->userHanlders)
				{
					(userHandler.func)(com.cp);
				}
			}
		}

		switch (visitor.visitComPacket(com.cp))
		{
		case CVR_DISCARD:
			delete com.cp;
			iter = mDispatchList.erase(iter);
			break;
		case CVR_TAKE:
			iter = mDispatchList.erase(iter);
			break;
		case CVR_RESERVE:
			++iter;
			break;
		}
	}

	if (!mDispatchList.empty())
	{
		NET_MUTEX_LOCK(mMutexProcCPList);
		mProcCPList.splice(mProcCPList.begin(), mDispatchList);
	}
}
