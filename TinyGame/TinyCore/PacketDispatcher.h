#ifndef PacketDispatcher_h__
#define PacketDispatcher_h__

#include "GameConfig.h"
#include "FastDelegate/FastDelegate.h"
#include <list>
#include <unordered_map>

class IComPacket;
class ComVisitor;
class PacketFactory;
typedef uint32 ComID;
typedef fastdelegate::FastDelegate< void (IComPacket*) > ComProcFunc;

// ========================================
// PacketDispatcher: 负责 packet 的分发和处理
// ========================================
class PacketDispatcher
{
public:
	typedef ComProcFunc ProcFunc;

	// Packet Handler 信息
	struct PacketHandler
	{
		PacketHandler()
			: userProcesser(nullptr)
			, workerProcesser(nullptr)
		{}

		void*    userProcesser;
		void*    workerProcesser;
		ProcFunc userFunc;         //app thread;
		ProcFunc workerFunc;       //app thread;
		ProcFunc workerFuncSocket; //socket thread;
	};

	TINY_API PacketDispatcher();
	TINY_API ~PacketDispatcher();

	// Handler 管理
	template<class GamePacketT, class T, class TFunc>
	bool setUserFunc(T* processer, TFunc func){ return setUserFunc(GamePacketT::PID, processer, func); }
	template<class GamePacketT, class T, class TFunc>
	bool setWorkerFunc(T* processer, TFunc func, void* = nullptr);
	template<class GamePacketT, class T, class TFunc>
	bool setWorkerFunc(T* processer, TFunc func, TFunc funcSocket);

	template<class T, class TFunc>
	bool setUserFunc(ComID id, T* processer, TFunc func)
	{
		NET_RWLOCK_WRITE(mRWLockHandlerMap);
		PacketHandler& handler = mHandlerMap[id];
		handler.userProcesser = processer;
		if (func)
		{
			handler.userFunc.bind(processer, func);
		}
		return true;
	}

	TINY_API void removeProcesserFunc(void* processer);
	TINY_API PacketHandler* findHandler(ComID packetId);


	TINY_API bool recvCommand(IComPacket* cp);

	// Packet 处理
	TINY_API void enqueue(IComPacket* packet, PacketHandler* handler);
	TINY_API void procCommand();
	TINY_API void procCommand(ComVisitor& visitor);
	TINY_API void procCommand(IComPacket* cp);

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

#endif // PacketDispatcher_h__
