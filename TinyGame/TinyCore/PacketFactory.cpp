#include "TinyGamePCH.h"
#include "PacketFactory.h"
#include "GameShare.h"
#include "SocketBuffer.h"
#include "Holder.h"
#include "ComPacket.h"

struct PacketHeader
{
	ComID   type;
	uint32  size;
};
static unsigned const ComPacketHeaderSize = sizeof(PacketHeader);

// ========================================
// PacketFactory Implementation
// ========================================

PacketFactory::PacketFactory()
{
}

PacketFactory::~PacketFactory()
{
	for (auto& pair : mCPFactoryMap)
	{
		delete pair.second;
	}
}

PacketFactory::ICPFactory* PacketFactory::findFactory(ComID com)
{
	NET_RWLOCK_READ( mRWLockCPFactoryMap );
	auto iter = mCPFactoryMap.find(com);
	if (iter != mCPFactoryMap.end())
		return iter->second;
	return nullptr;
}

// ========================================
// Parse Packet from SocketBuffer
// ========================================

IComPacket* PacketFactory::createPacketFromBuffer(SocketBuffer& buffer, int group /*= -1*/, void* userData /*= nullptr*/)
{
	// 保存当前 buffer 位置，以便出错时回退
	size_t oldUseSize = buffer.getUseSize();

	// 读取封包 ID
	ComID comID;
	if (buffer.getAvailableSize() < sizeof(ComID))
		return nullptr;

	buffer.take(comID);

	// 查找封包工厂
	auto factory = findFactory(comID);
	if (!factory)
	{
		// 未知封包类型，回退并停止解析
		buffer.setUseSize(oldUseSize);
		LogWarning(0, "PacketFactory: Unknown packet ID: %u", comID);
		return nullptr;
	}

	// 创建封包实例
	IComPacket* packet = factory->createCom();
	if (!packet)
	{
		buffer.setUseSize(oldUseSize);
		return nullptr;
	}

	// 设置封包的群组和用户数据（用于旧系统兼容）
	packet->mGroup = group;
	packet->mUserData = userData;

	// 读取封包数据
	if (!PacketFactory::ReadBuffer(buffer, packet))
	{
		// 读取失败，释放封包并回退
		delete packet;
		buffer.setUseSize(oldUseSize);
		return nullptr;
	}

	return packet;
}

// ========================================
// Static Helper Methods
// ========================================

uint32 PacketFactory::WriteBuffer(SocketBuffer& buffer, IComPacket* cp)
{
	size_t oldSize = buffer.getFillSize();
	uint32 size = 0;
	try
	{
		buffer.fill(cp->mId);
		buffer.fill(size);

		cp->writeBuffer(buffer);
	}
	catch (BufferException& e)
	{
		buffer.setFillSize(oldSize);
		LogMsg("%s(%d) : %s", __FILE__, __LINE__, e.what());
		throw BufferException("Can't fill buffer: Buffer has not space");
	}

	size = unsigned(buffer.getFillSize() - oldSize);
	unsigned* ptr = (unsigned*)(buffer.getData() + oldSize + sizeof(ComID));
	*ptr = size;

	return size;
}

bool PacketFactory::ReadBuffer(SocketBuffer& buffer, IComPacket* cp)
{
	uint32 takeSize;
	buffer.take(takeSize);

	takeSize -= ComPacketHeaderSize;
	if (takeSize > buffer.getAvailableSize())
	{
		LogMsg("PacketFactory: Error packet format - size isn't enough");
		buffer.shiftUseSize(-int(ComPacketHeaderSize));
		return false;
	}

	uint32 oldSize = (uint32)buffer.getAvailableSize();
	cp->readBuffer(buffer);

	if (takeSize != oldSize - (uint32)buffer.getAvailableSize())
	{
		LogMsg("PacketFactory: Error packet format - size mismatch");
		return false;
	}

	return true;
}

void PacketFactory::removeFactory(ComID id)
{
	auto iter = mCPFactoryMap.find(id);
	if (iter != mCPFactoryMap.end())
	{
		delete iter->second;
		mCPFactoryMap.erase(iter);
	}
}
