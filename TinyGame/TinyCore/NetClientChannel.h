#pragma once
#ifndef NetClientChannel_H_A1B2C3D4_E5F6_7890_ABCD_EF1234567890
#define NetClientChannel_H_A1B2C3D4_E5F6_7890_ABCD_EF1234567890

#include "NetChannel.h"
#include "GameServer.h"

//=============================================================================
// NetClientChannels - Channel wrapper for NetClientData
//=============================================================================
// This class provides a convenient way to work with NetClientData using
// the channel abstraction layer. It can be used as a replacement for
// direct buffer access in SNetPlayer and other server-side code.
//=============================================================================
class NetClientChannels
{
public:
	NetClientChannels() = default;
	
	// Initialize channels from NetClientData
	void init(NetClientData* clientData, NetSocket* serverUdpSocket = nullptr)
	{
		if (!clientData)
			return;
		
		mClientData = clientData;
		
		// Create TCP channel
		mTcpChannel = std::make_unique<TcpServerClientChannel>(clientData->tcpChannel);
		
		// Create UDP chain channel
		// If serverUdpSocket is provided, use it for sending (server mode)
		// Otherwise, use the client's own socket (connected UDP mode)
		if (serverUdpSocket)
		{
			mUdpChannel = std::make_unique<UdpChainChannel>(
				clientData->udpChannel,
				*serverUdpSocket,
				clientData->udpAddr
			);
		}
		else
		{
			// For connected UDP, the client has its own socket
			// We still wrap the UdpServer::Client
			mUdpChannelDirect = std::make_unique<UdpChainChannelDirect>(clientData->udpChannel);
		}
	}
	
	// Send through TCP (reliable, ordered)
	size_t sendTcp(IComPacket* packet)
	{
		if (mTcpChannel)
			return mTcpChannel->send(packet);
		return 0;
	}
	
	// Send through UDP chain (reliable UDP)
	size_t sendUdp(IComPacket* packet)
	{
		if (mUdpChannel)
			return mUdpChannel->send(packet);
		if (mUdpChannelDirect)
			return mUdpChannelDirect->send(packet);
		return 0;
	}
	
	// Send through specified channel ID
	size_t sendByChannel(int channelId, IComPacket* packet)
	{
		switch (channelId)
		{
		case NetChannelID::TCP:
			return sendTcp(packet);
		case NetChannelID::UDP_CHAIN:
			return sendUdp(packet);
		default:
			return 0;
		}
	}
	
	// Access channels directly
	INetChannel* getTcpChannel() { return mTcpChannel.get(); }
	INetChannel* getUdpChannel() 
	{ 
		return mUdpChannel ? mUdpChannel.get() 
		                   : static_cast<INetChannel*>(mUdpChannelDirect.get()); 
	}
	
	// Flush pending data
	void flush(long time)
	{
		if (mTcpChannel)
			mTcpChannel->flush(time);
		if (mUdpChannel)
			mUdpChannel->flush(time);
		if (mUdpChannelDirect)
			mUdpChannelDirect->flush(time);
	}
	
	// Clear all buffers
	void clear()
	{
		if (mTcpChannel)
			mTcpChannel->clearBuffer();
		if (mUdpChannel)
			mUdpChannel->clearBuffer();
		if (mUdpChannelDirect)
			mUdpChannelDirect->clearBuffer();
	}

private:
	// Helper class for UdpServer::Client without server socket reference
	class UdpChainChannelDirect : public INetChannel
	{
	public:
		explicit UdpChainChannelDirect(UdpServer::Client& client)
			: mClient(client)
		{}
		
		ENetChannelType getType() const override { return ENetChannelType::UdpChain; }
		bool isReliable() const override { return true; }
		bool isOrdered() const override { return true; }
		bool isConnected() const override { return true; }
		
		size_t send(IComPacket* packet) override
		{
			return FNetCommand::Write(mClient.getSendCtrl(), packet);
		}
		
		void flush(long time) override
		{
			mClient.processSendData(time);
		}
		
		void clearBuffer() override
		{
			mClient.clearBuffer();
		}
		
		NetBufferOperator& getSendCtrl() override
		{
			return mClient.getSendCtrl();
		}
		
	private:
		UdpServer::Client& mClient;
	};

	NetClientData* mClientData = nullptr;
	std::unique_ptr<TcpServerClientChannel> mTcpChannel;
	std::unique_ptr<UdpChainChannel> mUdpChannel;
	std::unique_ptr<UdpChainChannelDirect> mUdpChannelDirect;
};


//=============================================================================
// Example: Refactored SNetPlayer using channels
//=============================================================================
// This demonstrates how to migrate SNetPlayer to use the channel abstraction.
// Uncomment and modify SNetPlayer in GameServer.h/.cpp to use this pattern.
//=============================================================================
/*
class SNetPlayerWithChannels : public ServerPlayer
{
public:
	SNetPlayerWithChannels(ServerWorker* server, NetClientData* client)
		: ServerPlayer(true)
		, mServer(server)
		, mClient(client)
	{
		// Initialize channels with server's UDP socket
		mChannels.init(client, &server->getUdpSocket());
	}
	
	NetClientData& getClient() { return *mClient; }
	
	void sendTcpCommand(IComPacket* cp) override
	{
		mChannels.sendTcp(cp);
	}
	
	void sendUdpCommand(IComPacket* cp) override
	{
		mChannels.sendUdp(cp);
	}
	
	void sendCommand(int channel, IComPacket* cp) override
	{
		mChannels.sendByChannel(channel, cp);
	}

protected:
	ServerWorker* mServer;
	NetClientData* mClient;
	NetClientChannels mChannels;
};
*/

#endif // NetClientChannel_H_A1B2C3D4_E5F6_7890_ABCD_EF1234567890
