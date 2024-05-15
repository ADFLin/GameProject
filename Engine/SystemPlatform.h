#pragma once
#ifndef SystemPlatform_H_8E107E72_296E_43B2_AAF2_09A1A3C6D4AB
#define SystemPlatform_H_8E107E72_296E_43B2_AAF2_09A1A3C6D4AB

#include "PlatformConfig.h"
#include "Template/ArrayView.h"
#include "Core/IntegerType.h"

struct DateTime;

struct OpenFileFilterInfo
{
	char const* desc;
	char const* pattern;
};

class SystemPlatform
{
public:
	static int    GetProcessorNumber();
	static void   MemoryStoreFence();
	static void   Sleep( uint32 millionSecond );

	static double  GetHighResolutionTime();

	static std::string GetUserLocaleName();

	static char const* GetEnvironmentVariable( char const* key );
	static int32    AtomExchange(volatile int32* ptr, int32 value);
	static int64    AtomExchange(volatile int64* ptr, int64 value);
	static int32    AtomExchangeAdd(volatile int32* ptr, int32 value);
	static int64    AtomExchangeAdd(volatile int64* ptr, int64 value);
	static int32    AtomCompareExchange(volatile int32* ptr, int32 exchange, int32 comperand);
	static int64    AtomCompareExchange(volatile int64* ptr, int64 exchange , int64 comperand);
	static int32    AtomAdd(volatile int32* ptr, int32 value);
	static int64    AtomAdd(volatile int64* ptr, int64 value);
	static int32    AtomIncrement(volatile int32* ptr);
	static int64    AtomIncrement(volatile int64* ptr);
	static int32    AtomDecrement(volatile int32* ptr);
	static int64    AtomDecrement(volatile int64* ptr);
	static int32    AtomicRead(volatile int32* ptr);
	static int64    AtomicRead(volatile int64* ptr);

	static DateTime GetUTCTime();
	static DateTime GetLocalTime();

	static bool OpenFileName(char inoutPath[] , int pathSize, TArrayView< OpenFileFilterInfo const > filters, char const* initDir = nullptr, char const* title = nullptr , void* windowOwner = nullptr);
	static bool OpenDirectoryName(char outPath[], int pathSize, char const* initDir = nullptr , char const* title = nullptr, void* windowOwner = nullptr);
	static int64 GetTickCount();
};


struct ScopeTickCount
	{
	ScopeTickCount(int64& time):mTime(time)
	{
		mTime = SystemPlatform::GetTickCount();
	}
	~ScopeTickCount()
	{
		mTime = SystemPlatform::GetTickCount() - mTime;
	}

	int64& mTime;
};
#endif // SystemPlatform_H_8E107E72_296E_43B2_AAF2_09A1A3C6D4AB
