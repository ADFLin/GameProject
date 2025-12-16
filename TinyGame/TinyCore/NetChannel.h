#pragma once
#ifndef NetChannel_H_5A8C3E2F_B1D4_4E9A_8F7C_D2E5A6B8C9F1
#define NetChannel_H_5A8C3E2F_B1D4_4E9A_8F7C_D2E5A6B8C9F1

#include "GameNetConnect.h"
#include "ComPacket.h"

class IComPacket;
class ComEvaluator;

//=============================================================================
// Network Channel Types
//=============================================================================
enum class ENetChannelType
{
	Tcp,           // Reliable, ordered TCP channel
	Udp,           // Unreliable UDP channel (raw datagrams)
	UdpChain,      // Reliable UDP with sequence/ack system
};

//=============================================================================
// INetChannel - Abstract network channel interface
//=============================================================================
// Provides a unified interface for sending/receiving network commands
// regardless of the underlying transport protocol.
//=============================================================================
class INetChannel
{
public:
	virtual ~INetChannel() = default;

	//-------------------------------------------------------------------------
	// Channel Properties
	//-------------------------------------------------------------------------
	
	// Returns the type of this channel
	virtual ENetChannelType getType() const = 0;
	
	// Returns true if this channel guarantees delivery
	virtual bool isReliable() const = 0;
	
	// Returns true if this channel guarantees ordering
	virtual bool isOrdered() const = 0;
	
	// Returns true if the channel is connected/ready for communication
	virtual bool isConnected() const = 0;

	//-------------------------------------------------------------------------
	// Data Operations
	//-------------------------------------------------------------------------
	
	// Queues a packet for sending. The packet is serialized immediately.
	// Returns the number of bytes written, or 0 on failure.
	virtual size_t send(IComPacket* packet) = 0;
	
	// Processes pending data (send buffered data to network)
	// Should be called periodically, typically from the network thread
	virtual void flush(long time) = 0;
	
	// Clears all pending send/receive data
	virtual void clearBuffer() = 0;

	//-------------------------------------------------------------------------
	// Access to underlying buffer (for advanced usage)
	//-------------------------------------------------------------------------
	
	// Returns the send buffer operator (for direct buffer manipulation)
	virtual NetBufferOperator& getSendCtrl() = 0;
};


//=============================================================================
// TcpNetChannel - TCP-based reliable channel
//=============================================================================
class TcpNetChannel : public INetChannel
{
public:
	// Wraps an existing TcpClient
	explicit TcpNetChannel(TcpClient& client);
	
	// INetChannel interface
	ENetChannelType getType() const override { return ENetChannelType::Tcp; }
	bool isReliable() const override { return true; }
	bool isOrdered() const override { return true; }
	bool isConnected() const override;
	
	size_t send(IComPacket* packet) override;
	void flush(long time) override;
	void clearBuffer() override;
	
	NetBufferOperator& getSendCtrl() override;
	
	// Access to underlying client
	TcpClient& getClient() { return mClient; }

private:
	TcpClient& mClient;
};


//=============================================================================
// TcpServerClientChannel - TCP channel for server-side client connections
//=============================================================================
class TcpServerClientChannel : public INetChannel
{
public:
	// Wraps an existing TcpServer::Client
	explicit TcpServerClientChannel(TcpServer::Client& client);

	// INetChannel interface
	ENetChannelType getType() const override { return ENetChannelType::Tcp; }
	bool isReliable() const override { return true; }
	bool isOrdered() const override { return true; }
	bool isConnected() const override;

	size_t send(IComPacket* packet) override;
	void flush(long time) override;
	void clearBuffer() override;

	NetBufferOperator& getSendCtrl() override;

	// Access to underlying client
	TcpServer::Client& getClient() { return mClient; }

private:
	TcpServer::Client& mClient;
};


//=============================================================================
// UdpNetChannel - Raw UDP channel (unreliable)
//=============================================================================
class UdpNetChannel : public INetChannel
{
public:
	// Wraps an existing UdpClient
	explicit UdpNetChannel(UdpClient& client);
	
	// INetChannel interface
	ENetChannelType getType() const override { return ENetChannelType::Udp; }
	bool isReliable() const override { return false; }
	bool isOrdered() const override { return false; }
	bool isConnected() const override { return true; } // UDP is connectionless
	
	size_t send(IComPacket* packet) override;
	void flush(long time) override;
	void clearBuffer() override;
	
	NetBufferOperator& getSendCtrl() override;
	
	// Access to underlying client
	UdpClient& getClient() { return mClient; }

private:
	UdpClient& mClient;
};


//=============================================================================
// UdpChainChannel - Reliable UDP channel with sequence/ack
//=============================================================================
class UdpChainChannel : public INetChannel
{
public:
	// Wraps an existing UdpClient (which has UdpChain)
	explicit UdpChainChannel(UdpClient& client);
	
	// Wraps a UdpServer::Client
	explicit UdpChainChannel(UdpServer::Client& serverClient, NetSocket& socket, NetAddress& addr);

	// INetChannel interface
	ENetChannelType getType() const override { return ENetChannelType::UdpChain; }
	bool isReliable() const override { return true; }
	bool isOrdered() const override { return true; }
	bool isConnected() const override { return true; }
	
	size_t send(IComPacket* packet) override;
	void flush(long time) override;
	void clearBuffer() override;
	
	NetBufferOperator& getSendCtrl() override;

private:
	enum class Mode { Client, ServerClient };
	Mode mMode;
	
	// Client mode
	UdpClient* mClient = nullptr;
	
	// Server client mode
	UdpServer::Client* mServerClient = nullptr;
	NetSocket* mSocket = nullptr;
	NetAddress* mAddr = nullptr;
};


//=============================================================================
// NetChannelGroup - Manages TCP and UDP channels per connection
//=============================================================================
// Provides convenient access to TCP and UDP (chain) channels.
// Note: Both TCP and UdpChain are reliable in this system.
//=============================================================================
class NetChannelGroup
{
public:
	NetChannelGroup() = default;
	~NetChannelGroup() = default;

	// Non-copyable
	NetChannelGroup(const NetChannelGroup&) = delete;
	NetChannelGroup& operator=(const NetChannelGroup&) = delete;
	
	// Move semantics
	NetChannelGroup(NetChannelGroup&&) = default;
	NetChannelGroup& operator=(NetChannelGroup&&) = default;

	//-------------------------------------------------------------------------
	// Channel Management
	//-------------------------------------------------------------------------
	
	void setTcpChannel(std::unique_ptr<INetChannel> channel)
	{
		mTcpChannel = std::move(channel);
	}
	
	void setUdpChannel(std::unique_ptr<INetChannel> channel)
	{
		mUdpChannel = std::move(channel);
	}
	
	// Backward compatible aliases
	void setReliableChannel(std::unique_ptr<INetChannel> channel) { setTcpChannel(std::move(channel)); }
	void setUnreliableChannel(std::unique_ptr<INetChannel> channel) { setUdpChannel(std::move(channel)); }
	
	// Get channels
	INetChannel* getTcpChannel() { return mTcpChannel.get(); }
	INetChannel* getUdpChannel() { return mUdpChannel.get(); }
	
	// Backward compatible aliases  
	INetChannel* getReliableChannel() { return getTcpChannel(); }
	INetChannel* getUnreliableChannel() { return getUdpChannel(); }
	
	// Convenience methods
	bool hasTcpChannel() const { return mTcpChannel != nullptr; }
	bool hasUdpChannel() const { return mUdpChannel != nullptr; }
	bool hasReliableChannel() const { return hasTcpChannel(); }
	bool hasUnreliableChannel() const { return hasUdpChannel(); }

	//-------------------------------------------------------------------------
	// Send Operations
	//-------------------------------------------------------------------------
	
	// Send through TCP channel
	size_t sendTcp(IComPacket* packet)
	{
		if (mTcpChannel)
			return mTcpChannel->send(packet);
		return 0;
	}
	
	// Send through UDP (chain) channel
	size_t sendUdp(IComPacket* packet)
	{
		if (mUdpChannel)
			return mUdpChannel->send(packet);
		return 0;
	}
	
	// Backward compatible aliases
	size_t sendReliable(IComPacket* packet) { return sendTcp(packet); }
	size_t sendUnreliable(IComPacket* packet) { return sendUdp(packet); }
	
	// Flush all channels
	void flushAll(long time)
	{
		if (mTcpChannel)
			mTcpChannel->flush(time);
		if (mUdpChannel)
			mUdpChannel->flush(time);
	}
	
	// Clear all buffers
	void clearAll()
	{
		if (mTcpChannel)
			mTcpChannel->clearBuffer();
		if (mUdpChannel)
			mUdpChannel->clearBuffer();
	}

private:
	std::unique_ptr<INetChannel> mTcpChannel;
	std::unique_ptr<INetChannel> mUdpChannel;
};


//=============================================================================
// Channel ID Constants (for backward compatibility)
//=============================================================================
namespace NetChannelID
{
	constexpr int TCP = 1;
	constexpr int UDP_CHAIN = 2;
	constexpr int UDP_RAW = 3;
	
	// Map old DefaultChannel enum values to new constants
	inline int fromLegacy(int legacyChannel)
	{
		// CHANNEL_GAME_NET_TCP = 1, CHANNEL_GAME_NET_UDP_CHAIN = 2
		return legacyChannel;  // Same values, direct mapping
	}
}


//=============================================================================
// NetChannelFactory - Factory for creating channel instances
//=============================================================================
class NetChannelFactory
{
public:
	// Create a TCP channel wrapping a client connection
	static std::unique_ptr<TcpNetChannel> createTcpChannel(TcpClient& client)
	{
		return std::make_unique<TcpNetChannel>(client);
	}
	
	// Create a TCP channel wrapping a server-side client
	static std::unique_ptr<TcpServerClientChannel> createTcpServerChannel(TcpServer::Client& client)
	{
		return std::make_unique<TcpServerClientChannel>(client);
	}
	
	// Create a raw UDP channel
	static std::unique_ptr<UdpNetChannel> createUdpChannel(UdpClient& client)
	{
		return std::make_unique<UdpNetChannel>(client);
	}
	
	// Create a UDP chain channel (client mode)
	static std::unique_ptr<UdpChainChannel> createUdpChainChannel(UdpClient& client)
	{
		return std::make_unique<UdpChainChannel>(client);
	}
	
	// Create a UDP chain channel (server-client mode)
	static std::unique_ptr<UdpChainChannel> createUdpChainChannel(
		UdpServer::Client& serverClient, 
		NetSocket& socket, 
		NetAddress& addr)
	{
		return std::make_unique<UdpChainChannel>(serverClient, socket, addr);
	}
	
	// Create a complete channel group for a client connection
	static NetChannelGroup createClientChannelGroup(TcpClient& tcp, UdpClient& udp)
	{
		NetChannelGroup group;
		group.setReliableChannel(createTcpChannel(tcp));
		group.setUnreliableChannel(createUdpChainChannel(udp));
		return group;
	}
};


//=============================================================================
// Utility functions for channel-based operations
//=============================================================================

// Send a packet through the appropriate channel based on channel ID
// This provides backward compatibility with existing code using channel IDs
inline size_t SendToChannel(INetChannel* channel, IComPacket* packet)
{
	if (channel)
		return channel->send(packet);
	return 0;
}

// Get channel type from legacy channel ID
inline ENetChannelType GetChannelTypeFromID(int channelId)
{
	switch (channelId)
	{
	case NetChannelID::TCP:
		return ENetChannelType::Tcp;
	case NetChannelID::UDP_CHAIN:
		return ENetChannelType::UdpChain;
	case NetChannelID::UDP_RAW:
		return ENetChannelType::Udp;
	default:
		return ENetChannelType::Tcp;  // Default to reliable
	}
}

// Check if a channel ID represents a reliable channel
inline bool IsChannelReliable(int channelId)
{
	switch (channelId)
	{
	case NetChannelID::TCP:
	case NetChannelID::UDP_CHAIN:
		return true;
	case NetChannelID::UDP_RAW:
		return false;
	default:
		return true;
	}
}

#endif // NetChannel_H_5A8C3E2F_B1D4_4E9A_8F7C_D2E5A6B8C9F1
