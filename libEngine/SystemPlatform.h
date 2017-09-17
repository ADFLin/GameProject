#pragma once
#ifndef SystemPlatform_H_8E107E72_296E_43B2_AAF2_09A1A3C6D4AB
#define SystemPlatform_H_8E107E72_296E_43B2_AAF2_09A1A3C6D4AB

#include "PlatformConfig.h"
#include "IntegerType.h"

#undef InterlockedExchange

class SystemPlatform
{
public:
	static int    GetProcessorNumber();
	static void   MemoryBarrier();
	static void   Sleep( uint32 millionSecond );

	static int32  InterlockedExchange(volatile int32* value, int32 exchange);

};
#endif // SystemPlatform_H_8E107E72_296E_43B2_AAF2_09A1A3C6D4AB
