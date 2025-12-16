# Network Channel Abstraction Layer

## Overview

The `NetChannel` abstraction provides a unified interface for network communication,
regardless of the underlying transport protocol (TCP, UDP, or UDP with reliability).

## Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                     INetChannel (Interface)                      │
│  - send(IComPacket*)   - flush(long time)                       │
│  - isReliable()        - isConnected()                          │
│  - clearBuffer()       - getSendCtrl()                          │
└──────────────────────────────┬──────────────────────────────────┘
                               │
       ┌───────────────────────┼───────────────────────┐
       ▼                       ▼                       ▼
┌──────────────┐     ┌─────────────────┐     ┌────────────────┐
│ TcpNetChannel│     │  UdpNetChannel  │     │ UdpChainChannel│
│  (Reliable)  │     │  (Unreliable)   │     │  (Reliable UDP)│
└──────────────┘     └─────────────────┘     └────────────────┘
       │                       │                       │
       ▼                       ▼                       ▼
┌──────────────┐     ┌─────────────────┐     ┌────────────────┐
│  TcpClient   │     │   UdpClient     │     │ UdpServer::    │
│(existing)    │     │   (existing)    │     │ Client(existing│
└──────────────┘     └─────────────────┘     └────────────────┘
```

## Channel Types

| Channel Type         | Reliable | Ordered | Use Case                    |
|---------------------|----------|---------|------------------------------|
| TcpNetChannel       | Yes      | Yes     | Critical game state, login   |
| UdpNetChannel       | No       | No      | Raw UDP, discovery           |
| UdpChainChannel     | Yes      | Yes     | Game actions, sync frames    |
| TcpServerClientChannel | Yes   | Yes     | Server-side TCP to clients   |

## Usage Examples

### Creating Channels (New Code)

```cpp
#include "NetChannel.h"

// Client side - create channel group
TcpClient tcpClient;
UdpClient udpClient;

auto tcpChannel = NetChannelFactory::createTcpChannel(tcpClient);
auto udpChannel = NetChannelFactory::createUdpChainChannel(udpClient);

// Or create a complete group
NetChannelGroup channelGroup = NetChannelFactory::createClientChannelGroup(tcpClient, udpClient);

// Send packets
IComPacket* packet = ...;
channelGroup.sendReliable(packet);
channelGroup.sendUnreliable(packet);
```

### Server-Side Channels

```cpp
// Wrap server client connections
NetClientData* clientData = ...;

auto tcpChannel = std::make_unique<TcpServerClientChannel>(clientData->tcpChannel);
auto udpChannel = std::make_unique<UdpChainChannel>(
    clientData->udpChannel, 
    serverSocket, 
    clientData->udpAddr
);

// Send to client
tcpChannel->send(packet);
```

### Migrating Existing Code

**Before (Old Code):**
```cpp
void SNetPlayer::sendCommand(int channel, IComPacket* cp)
{
    switch(channel)
    {
    case CHANNEL_GAME_NET_TCP:
        FNetCommand::Write(mClient->tcpChannel.getSendCtrl(), cp);
        break;
    case CHANNEL_GAME_NET_UDP_CHAIN:
        FNetCommand::Write(mClient->udpChannel.getSendCtrl(), cp);
        break;
    }
}
```

**After (New Code):**
```cpp
class SNetPlayer : public ServerPlayer
{
    std::unique_ptr<INetChannel> mTcpChannel;
    std::unique_ptr<INetChannel> mUdpChannel;
    
public:
    void initChannels(NetClientData* client, NetSocket& serverSocket)
    {
        mTcpChannel = std::make_unique<TcpServerClientChannel>(client->tcpChannel);
        mUdpChannel = std::make_unique<UdpChainChannel>(
            client->udpChannel, serverSocket, client->udpAddr);
    }
    
    void sendCommand(int channel, IComPacket* cp) override
    {
        INetChannel* targetChannel = (channel == CHANNEL_GAME_NET_TCP) 
            ? mTcpChannel.get() 
            : mUdpChannel.get();
        
        if (targetChannel)
            targetChannel->send(cp);
    }
    
    void sendTcpCommand(IComPacket* cp) override
    {
        if (mTcpChannel)
            mTcpChannel->send(cp);
    }
    
    void sendUdpCommand(IComPacket* cp) override
    {
        if (mUdpChannel)
            mUdpChannel->send(cp);
    }
};
```

## Backward Compatibility

The channel system is designed for gradual migration:

1. **Channel IDs are preserved**: `CHANNEL_GAME_NET_TCP` = `NetChannelID::TCP` = 1
2. **Helper functions available**: `GetChannelTypeFromID()`, `IsChannelReliable()`
3. **Existing code continues working**: `FNetCommand::Write()` still works

## NetChannelGroup

For managing multiple channels per connection:

```cpp
NetChannelGroup group;
group.setReliableChannel(tcpChannel);
group.setUnreliableChannel(udpChannel);

// Check availability
if (group.hasReliableChannel()) { ... }

// Send through appropriate channel
group.sendReliable(criticalPacket);
group.sendUnreliable(gameStatePacket);

// Flush all pending data
group.flushAll(currentTime);

// Clean up
group.clearAll();
```

## Migration Steps

1. Include `NetChannel.h` in your code
2. Create channel instances using `NetChannelFactory`
3. Replace `FNetCommand::Write(ctrl, packet)` with `channel->send(packet)`
4. Use `NetChannelGroup` for managing multiple channels
5. Eventually remove direct buffer access

## Benefits

- **Unified Interface**: Same API for all transport types
- **Testability**: Easy to mock channels for unit testing
- **Flexibility**: Can swap transport without changing business logic
- **Type Safety**: Channel type is explicit, not an integer
- **Encapsulation**: Implementation details hidden from consumers
