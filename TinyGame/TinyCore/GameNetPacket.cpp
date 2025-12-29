#include "TinyGamePCH.h"
#include "GameNetPacket.h"

// ========================================
// 註冊所有 TinyGame 核心網絡封包
// 使用 REGISTER_GAME_PACKET 宏在程序啟動時自動註冊到 GGamePacketFactory
// ========================================

// Game Data Protocol - Frame Stream Packets
REGISTER_GAME_PACKET(GDPFrameStream)

// Game Data Protocol - Stream Packets
REGISTER_GAME_PACKET(GDPStream)

// Client Packets
REGISTER_GAME_PACKET(CPLogin)
REGISTER_GAME_PACKET(CPEcho)
REGISTER_GAME_PACKET(CPUdpCon)
REGISTER_GAME_PACKET(CPNetControlReply)

// Client-Server Packets
REGISTER_GAME_PACKET(CSPRawData)
REGISTER_GAME_PACKET(CSPMsg)
REGISTER_GAME_PACKET(CSPComMsg)
REGISTER_GAME_PACKET(CSPPlayerState)
REGISTER_GAME_PACKET(CSPClockSynd)

// Server Packets
REGISTER_GAME_PACKET(SPSlotState)
REGISTER_GAME_PACKET(SPServerInfo)
REGISTER_GAME_PACKET(SPLevelInfo)
REGISTER_GAME_PACKET(SPConnectMsg)
REGISTER_GAME_PACKET(SPPlayerStatus)

// Gateway Packets
REGISTER_GAME_PACKET(GPGameSvList)
