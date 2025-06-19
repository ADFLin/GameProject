#include "GPUDeviceQuery.h"
#include "PlatformConfig.h"

#if SYS_PLATFORM_WIN
#include "DynamicNvAPI.h"

class NvGpuDeviceQuery : public GPUDeviceQuery 
	                   , public DyanmicNvAPI
{
public:
	void release()
	{
		DyanmicNvAPI::release();
		delete this;
	}
	bool initialize()
	{
		if( !DyanmicNvAPI::initialize() )
			return false;

#define QUERY_INTERFACE( NAME )\
	if ( !queryInterface(ID_##NAME , NAME) )\
		return false;

		QUERY_INTERFACE(NvAPI_GPU_GetFullName);
		QUERY_INTERFACE(NvAPI_GPU_GetThermalSettings);
		QUERY_INTERFACE(NvAPI_EnumNvidiaDisplayHandle);
		QUERY_INTERFACE(NvAPI_GetPhysicalGPUsFromDisplay);
		QUERY_INTERFACE(NvAPI_EnumPhysicalGPUs);
		QUERY_INTERFACE(NvAPI_GPU_GetTachReading);
		QUERY_INTERFACE(NvAPI_GPU_GetMemoryInfo);
		QUERY_INTERFACE(NvAPI_SYS_GetDriverAndBranchVersion);
		QUERY_INTERFACE(NvAPI_GetInterfaceVersionString);
		QUERY_INTERFACE(NvAPI_GPU_GetPCIIdentifiers);

		//QUERY_INTERFACE(NvAPI_GPU_GetCoolerSettings);
		//QUERY_INTERFACE(NvAPI_GPU_SetCoolerLevels);
		QUERY_INTERFACE(NvAPI_GPU_GetAllClocks);
		QUERY_INTERFACE(NvAPI_GPU_GetPStates);
		QUERY_INTERFACE(NvAPI_GPU_GetUsages);

#undef QUERY_INTERFACE

		{
			NvU32 gpuCount;
			NvPhysicalGpuHandle nvGpuHandles[NVAPI_MAX_PHYSICAL_GPUS];
			if( !checkNvStatus(NvAPI_EnumPhysicalGPUs(nvGpuHandles, &gpuCount)) )
				return false;

			mGpuHandles.assign(nvGpuHandles, nvGpuHandles + gpuCount);
		}

		mDisplays.clear();
		for( NvU32 nIndex = 0; nIndex < NVAPI_MAX_DISPLAYS; ++nIndex )
		{
			NvDisplayHandle nvDisplayCardHandle;
			if( !checkNvStatus(NvAPI_EnumNvidiaDisplayHandle(nIndex, &nvDisplayCardHandle)) )
				break;

			DisplayHandle info;
			info.displayHandle = nvDisplayCardHandle;

			NvU32 gpuCount;
			NvPhysicalGpuHandle nvHandles[NVAPI_MAX_PHYSICAL_GPUS];
			if( !checkNvStatus(NvAPI_GetPhysicalGPUsFromDisplay(info.displayHandle, nvHandles, &gpuCount)) )
				break;

			info.GpuHandles.assign(nvHandles, nvHandles + gpuCount);
			mDisplays.push_back(info);
		}

		return true;
	}

	bool getGPUInfo(int idx, GPUInfo& info)
	{
		NvAPI_ShortString gpuName;
		if( !checkNvStatus(NvAPI_GPU_GetFullName(mGpuHandles[idx], gpuName)) )
			return false;

		info.name = gpuName;
		return true;
	}

	bool getGPUStatus(int idx, GPUStatus& status, GPUStatus::QueryMask mask )
	{
		NvPhysicalGpuHandle handle = mGpuHandles[idx];
		if ( mask & GPUStatus::eTemperature )
		{
			NV_GPU_THERMAL_SETTINGS setting;
			setting.version = NV_GPU_THERMAL_SETTINGS_VER;
			if( !checkNvStatus(NvAPI_GPU_GetThermalSettings(handle, 0, &setting)) )
				return false;
			status.temperature = setting.sensor[0].currentTemp;
		}

		if( mask & GPUStatus::eMemory )
		{
			NV_DISPLAY_DRIVER_MEMORY_INFO info;
			info.version = NV_DISPLAY_DRIVER_MEMORY_INFO_VER;

			if( !checkNvStatus(NvAPI_GPU_GetMemoryInfo(handle, &info)) )
				return false;
			status.freeMemory = info.curAvailableDedicatedVideoMemory;
			status.totalMemory = info.dedicatedVideoMemory;
		}

		if( mask & GPUStatus::eUsage )
		{
			NvUsages nvUsages;
			nvUsages.version = NV_USAGES_VER;
			if( !checkNvStatus(NvAPI_GPU_GetUsages(handle, &nvUsages)) )
				return false;

			status.usage = nvUsages.usages[2];
		}
		return true;
	}
	int getDisplyNum() { return mDisplays.size(); }

	struct DisplayHandle
	{
		NvDisplayHandle displayHandle;
		TArray< NvPhysicalGpuHandle > GpuHandles;
	};

	NvAPI_Status(__cdecl *NvAPI_GPU_GetFullName)(NvPhysicalGpuHandle hPhysicalGpu, NvAPI_ShortString szName);
	NvAPI_Status(__cdecl *NvAPI_GPU_GetThermalSettings)(NvPhysicalGpuHandle hPhysicalGpu, NvU32 sensorIndex, NV_GPU_THERMAL_SETTINGS *pThermalSettings);
	NvAPI_Status(__cdecl *NvAPI_EnumNvidiaDisplayHandle)(NvU32 thisEnum, NvDisplayHandle *pNvDispHandle);
	NvAPI_Status(__cdecl *NvAPI_GetPhysicalGPUsFromDisplay)(NvDisplayHandle hNvDisp, NvPhysicalGpuHandle nvGPUHandle[NVAPI_MAX_PHYSICAL_GPUS], NvU32 *pGpuCount);
	NvAPI_Status(__cdecl *NvAPI_EnumPhysicalGPUs)(NvPhysicalGpuHandle nvGPUHandle[NVAPI_MAX_PHYSICAL_GPUS], NvU32 *pGpuCount);
	NvAPI_Status(__cdecl *NvAPI_GPU_GetTachReading)(NvPhysicalGpuHandle hPhysicalGPU, NvU32 *pValue);

	//NvAPI_Status (__cdecl *NvAPI_GPU_GetCoolerSettings)(const NvPhysicalGpuHandle gpuHandle, int coolerIndex, NvGPUCoolerSettings *pnvGPUCoolerSettings);
	//NvAPI_Status (__cdecl *NvAPI_GPU_SetCoolerLevels)(const NvPhysicalGpuHandle gpuHandle, int coolerIndex, NvGPUCoolerLevels *pnvGPUCoolerLevels);
	NvAPI_Status(__cdecl *NvAPI_GPU_GetMemoryInfo)(NvPhysicalGpuHandle hPhysicalGpu, NV_DISPLAY_DRIVER_MEMORY_INFO *pMemoryInfo);
	NvAPI_Status(__cdecl *NvAPI_SYS_GetDriverAndBranchVersion)(NvU32* pDriverVersion, NvAPI_ShortString szBuildBranchString);
	NvAPI_Status(__cdecl *NvAPI_GetInterfaceVersionString)(NvAPI_ShortString szDesc);
	NvAPI_Status(__cdecl *NvAPI_GPU_GetPCIIdentifiers)(NvPhysicalGpuHandle hPhysicalGpu, NvU32 *pDeviceId, NvU32 *pSubSystemId, NvU32 *pRevisionId, NvU32 *pExtDeviceId);

	NvAPI_Status(__cdecl *NvAPI_GPU_GetAllClocks)(__in NvPhysicalGpuHandle hPhysicalGpu, __inout NV_CLOCKS_INFO* pClocksInfo);
	NvAPI_Status(__cdecl *NvAPI_GPU_GetPStates)(NvPhysicalGpuHandle gpuHandle, NvPStates *pnvPStates);
	NvAPI_Status(__cdecl *NvAPI_GPU_GetUsages)(NvPhysicalGpuHandle gpuHandle, NvUsages *pnvUsages);

	TArray< DisplayHandle > mDisplays;
	TArray< NvPhysicalGpuHandle > mGpuHandles;
};

#endif

GPUDeviceQuery* GPUDeviceQuery::Create()
{
#if SYS_PLATFORM_WIN
	auto result = new NvGpuDeviceQuery;
	if( result && !result->initialize() )
	{
		delete result;
		return nullptr;
	}
	return result;
#else
	return nullptr;
#endif
}
