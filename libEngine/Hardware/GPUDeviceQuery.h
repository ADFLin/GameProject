#pragma once
#ifndef GPUDeviceQuery_H_73D9B0F7_A077_471E_AA45_F0C93B6DCB4A
#define GPUDeviceQuery_H_73D9B0F7_A077_471E_AA45_F0C93B6DCB4A

#include "Core/IntegerType.h"

#include <string>

struct GPUStatus
{
	int    usage;
	int    temperature;
	uint32 totalMemory;
	uint32 freeMemory;
};

struct GPUInfo
{
	std::string name;
};

struct GPUSystemInfo
{
	uint32 driverVersion;
	uint32 numGPUs;
	uint32 numDisplays;
};

class GPUDeviceQuery
{
public:

	virtual ~GPUDeviceQuery(){}
	virtual void release() = 0;
	virtual bool getGPUInfo(int idx, GPUInfo& info) = 0;
	virtual bool getGPUStatus(int idx, GPUStatus& status) = 0;


	static GPUDeviceQuery* Create();
};

#endif // GPUDeviceQuery_H_73D9B0F7_A077_471E_AA45_F0C93B6DCB4A
