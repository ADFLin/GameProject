#include "TinyGamePCH.h"
#include "NetChannel.h"
#include "GameWorker.h"  // For FNetCommand

//=============================================================================
// TcpNetChannel Implementation
//=============================================================================

TcpNetChannel::TcpNetChannel(TcpClient& client)
	: mClient(client)
{
}

bool TcpNetChannel::isConnected() const
{
	return mClient.isConnected();
}

size_t TcpNetChannel::send(IComPacket* packet)
{
	return FNetCommand::Write(mClient.getSendCtrl(), packet);
}

void TcpNetChannel::flush(long time)
{
	// TCP sends data automatically when the socket is sendable
	// The actual sending is handled by TcpClient::onSendable
}

void TcpNetChannel::clearBuffer()
{
	mClient.clearBuffer();
}

NetBufferOperator& TcpNetChannel::getSendCtrl()
{
	return mClient.getSendCtrl();
}


//=============================================================================
// TcpServerClientChannel Implementation
//=============================================================================

TcpServerClientChannel::TcpServerClientChannel(TcpServer::Client& client)
	: mClient(client)
{
}

bool TcpServerClientChannel::isConnected() const
{
	return mClient.isConnected();
}

size_t TcpServerClientChannel::send(IComPacket* packet)
{
	return FNetCommand::Write(mClient.getSendCtrl(), packet);
}

void TcpServerClientChannel::flush(long time)
{
	// TCP sends data automatically when the socket is sendable
}

void TcpServerClientChannel::clearBuffer()
{
	mClient.clearBuffer();
}

NetBufferOperator& TcpServerClientChannel::getSendCtrl()
{
	return mClient.getSendCtrl();
}


//=============================================================================
// UdpNetChannel Implementation
//=============================================================================

UdpNetChannel::UdpNetChannel(UdpClient& client)
	: mClient(client)
{
}

size_t UdpNetChannel::send(IComPacket* packet)
{
	return FNetCommand::Write(mClient.getSendCtrl(), packet);
}

void UdpNetChannel::flush(long time)
{
	// UDP client handles sending via onSendable callback
}

void UdpNetChannel::clearBuffer()
{
	mClient.clearBuffer();
}

NetBufferOperator& UdpNetChannel::getSendCtrl()
{
	return mClient.getSendCtrl();
}


//=============================================================================
// UdpChainChannel Implementation
//=============================================================================

UdpChainChannel::UdpChainChannel(UdpClient& client)
	: mMode(Mode::Client)
	, mClient(&client)
{
}

UdpChainChannel::UdpChainChannel(UdpServer::Client& serverClient, NetSocket& socket, NetAddress& addr)
	: mMode(Mode::ServerClient)
	, mServerClient(&serverClient)
	, mSocket(&socket)
	, mAddr(&addr)
{
}

size_t UdpChainChannel::send(IComPacket* packet)
{
	if (mMode == Mode::Client)
	{
		return FNetCommand::Write(mClient->getSendCtrl(), packet);
	}
	else
	{
		return FNetCommand::Write(mServerClient->getSendCtrl(), packet);
	}
}

void UdpChainChannel::flush(long time)
{
	if (mMode == Mode::Client)
	{
		// UdpClient handles the chain sending internally
	}
	else
	{
		// Server client needs to process send data
		if (mSocket && mAddr)
		{
			mServerClient->processSendData(time, *mSocket, *mAddr);
		}
		else
		{
			mServerClient->processSendData(time);
		}
	}
}

void UdpChainChannel::clearBuffer()
{
	if (mMode == Mode::Client)
	{
		mClient->clearBuffer();
	}
	else
	{
		mServerClient->clearBuffer();
	}
}

NetBufferOperator& UdpChainChannel::getSendCtrl()
{
	if (mMode == Mode::Client)
	{
		return mClient->getSendCtrl();
	}
	else
	{
		return mServerClient->getSendCtrl();
	}
}
