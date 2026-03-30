#pragma once
#ifndef PacketDispatcher_H_5E13512A_86EC_40F8_9D99_6E0346698A10
#define PacketDispatcher_H_5E13512A_86EC_40F8_9D99_6E0346698A10

#include "GameConfig.h"
#include "DataStructure/Array.h"
#include "FastDelegate/FastDelegate.h"

#include <list>
#include <unordered_map>

class IComPacket;
class ComVisitor;
class PacketFactory;

using ComID = uint32;
using ComProcFunc = fastdelegate::FastDelegate< void (IComPacket*) >;


class PacketDispatcher
{
public:
	typedef ComProcFunc ProcFunc;

	struct UserHandler
	{
		void*    userProcesser;
		ProcFunc func;
	};

	struct PacketHandler
	{
		PacketHandler()
			: workerProcesser(nullptr)
		{}

		void*    workerProcesser;


		TArray<UserHandler> userHanlders;         //app thread;
		ProcFunc workerFunc;       //app thread;
		ProcFunc workerFuncSocket; //socket thread;
	};

	TINY_API PacketDispatcher();
	TINY_API ~PacketDispatcher();

	template<class GamePacketT, class T, class TFunc>
	bool setUserFunc(T* processer, TFunc func){ return setUserFunc(GamePacketT::PID, processer, func); }
	template<class GamePacketT, class T, class TFunc>
	bool setWorkerFunc(T* processer, TFunc func, void* = nullptr);
	template<class GamePacketT, class T, class TFunc>
	bool setWorkerFunc(T* processer, TFunc func, TFunc funcSocket);

	template<class T, class TFunc>
	bool setUserFunc(ComID id, T* processer, TFunc func)
	{
		if (!func)
			return false;

		NET_RWLOCK_WRITE(mRWLockHandlerMap);
		PacketHandler& handler = mHandlerMap[id];

		UserHandler userHandler;
		userHandler.userProcesser = processer;
		userHandler.func.bind(processer, func);
		handler.userHanlders.push_back(userHandler);
		return true;
	}

	TINY_API void removeProcesserFunc(void* processer);
	TINY_API PacketHandler* findHandler(ComID packetId);


	TINY_API bool recvCommand(IComPacket* cp);

	TINY_API void enqueue(IComPacket* packet, PacketHandler* handler);
	TINY_API void procCommand();
	TINY_API void procCommand(ComVisitor& visitor);

private:
	struct UserCom
	{
		PacketHandler* handler;
		IComPacket* cp;
	};
	typedef std::list<UserCom> ComPacketList;

	typedef std::unordered_map<ComID, PacketHandler> HandlerMap;

	HandlerMap mHandlerMap;
	NET_RWLOCK( mRWLockHandlerMap )

	ComPacketList mProcCPList;
	ComPacketList mDispatchList;
	NET_MUTEX( mMutexProcCPList )
};

// Template implementations


template<class GamePacketT, class T, class TFunc>
bool PacketDispatcher::setWorkerFunc(T* processer, TFunc func, void*)
{
	NET_RWLOCK_WRITE( mRWLockHandlerMap );
	PacketHandler& handler = mHandlerMap[GamePacketT::PID];
	
	handler.workerProcesser = processer;
	if (func)
	{
		handler.workerFunc.bind(processer, func);
	}
	return true;
}

template<class GamePacketT, class T, class TFunc>
bool PacketDispatcher::setWorkerFunc(T* processer, TFunc func, TFunc funcSocket)
{
	NET_RWLOCK_WRITE( mRWLockHandlerMap );
	PacketHandler& handler = mHandlerMap[GamePacketT::PID];
	
	handler.workerProcesser = processer;
	if (func)
	{
		handler.workerFunc.bind(processer, func);
	}
	if (funcSocket)
	{
		handler.workerFuncSocket.bind(processer, funcSocket);
	}
	return true;
}

#endif // PacketDispatcher_H_5E13512A_86EC_40F8_9D99_6E0346698A10
