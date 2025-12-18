#pragma once
#ifndef GameShare_H_4538DC2D_5D0A_46A7_9909_D2EDD14191B4
#define GameShare_H_4538DC2D_5D0A_46A7_9909_D2EDD14191B4

#include "GameConfig.h"
#include "LogSystem.h"

TINY_API bool IsInGameThead();
TINY_API extern class PacketFactory GGamePacketFactory;

#if TINY_USE_NET_THREAD
TINY_API bool IsInNetThread();
#else
FORCEINLINE bool IsInNetThread() { return IsInGameThead(); }
#endif
#endif // GameShare_H_4538DC2D_5D0A_46A7_9909_D2EDD14191B4

