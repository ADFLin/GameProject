#ifndef PacketFactory_h__
#define PacketFactory_h__

#include "GameConfig.h"
#include "FastDelegate/FastDelegate.h"
#include <unordered_map>

class IComPacket;
class SocketBuffer;
typedef uint32 ComID;
typedef fastdelegate::FastDelegate< void (IComPacket*) > ComProcFunc;

class PacketFactory
{
public:
	typedef ComProcFunc ProcFunc;

	struct ICPFactory
	{
		ICPFactory() = default;
		virtual ~ICPFactory() = default;
		virtual IComPacket* createCom() = 0;
		ComID id;
	};

	template <class GamePacketT>
	struct CPFactory : public ICPFactory
	{
		CPFactory() { id = GamePacketT::PID; }
		virtual IComPacket* createCom() { return new GamePacketT; }
	};

	TINY_API PacketFactory();
	TINY_API ~PacketFactory();

	template<class GamePacketT>
	void addFactory() { addFactoryInternal<GamePacketT>(); }

	template<class GamePacketT>
	void removeFactory(){ removeFactory(GamePacketT::PID); }

	TINY_API ICPFactory* findFactory(ComID com);
	TINY_API IComPacket* createPacketFromBuffer(SocketBuffer& buffer, int group = -1, void* userData = nullptr);

	TINY_API static uint32 WriteBuffer(SocketBuffer& buffer, IComPacket* cp);
	TINY_API static bool ReadBuffer(SocketBuffer& buffer, IComPacket* cp);

private:
	template<class GamePacketT>
	ICPFactory* addFactoryInternal();

	TINY_API void removeFactory(ComID id);

	typedef std::unordered_map<ComID, ICPFactory*> CPFactoryMap;
	CPFactoryMap  mCPFactoryMap;
	NET_RWLOCK( mRWLockCPFactoryMap );
};

// Template implementations
template<class GamePacketT>
PacketFactory::ICPFactory* PacketFactory::addFactoryInternal()
{
	ICPFactory* factory = findFactory(GamePacketT::PID);
	if (factory == nullptr)
	{
		NET_RWLOCK_WRITE(mRWLockCPFactoryMap);
		factory = new CPFactory<GamePacketT>;
		mCPFactoryMap.insert(std::make_pair(GamePacketT::PID, factory));
	}
	return factory;
}

#endif // PacketFactory_h__
