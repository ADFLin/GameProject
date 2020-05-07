#include "GPUDeviceQuery.h"

#include "nvapi/nvapi.h"
#include "WindowsHeader.h"

#include <vector>

#define NVAPI_MAX_CLOCKS_PER_GPU              0x120  

typedef struct _NV_CLOCKS_INFO
{
	NvU32 version;                              //!< Structure version
	NvU32 clocks[NVAPI_MAX_CLOCKS_PER_GPU];
} NV_CLOCKS_INFO;

#define NV_CLOCKS_INFO_VER  MAKE_NVAPI_VERSION(NV_CLOCKS_INFO, 2)


typedef struct _NvPState
{
	NvU32  bPresent;
	NvS32  percentage;
}NvPState;

#define NVAPI_MAX_PSTATES_PER_GPU             8  

typedef struct _NvPStates
{
	NvU32 version;
	NvU32 Flags;
	NvPState PStates[NVAPI_MAX_PSTATES_PER_GPU];
}NvPStates;

#define NV_PS_STATES_VER  MAKE_NVAPI_VERSION(NvPStates, 1)

#define NVAPI_MAX_USAGES_PER_GPU              33  

typedef struct _NvUsages
{
	NvU32 version;
	NvU32 usages[NVAPI_MAX_USAGES_PER_GPU];
}NvUsages;

#define NV_USAGES_VER  MAKE_NVAPI_VERSION(NvUsages, 1)


struct DyanmicNvAPI
{
public:

	enum FunId
	{
		ID_NvAPI_Initialize = 0x150E828,
		ID_NvAPI_GetErrorMessage = 0x6C2D048C,
		ID_NvAPI_GetInterfaceVersionString = 0x1053FA5,
		ID_NvAPI_GPU_GetEDID = 0x37D32E69,
		ID_NvAPI_SetView = 0x957D7B6,
		ID_NvAPI_SetViewEx = 0x6B89E68,
		ID_NvAPI_GetDisplayDriverVersion = 0xF951A4D1,
		ID_NvAPI_SYS_GetDriverAndBranchVersion = 0x2926AAAD,
		ID_NvAPI_GPU_GetMemoryInfo = 0x7F9B368,
		ID_NvAPI_OGL_ExpertModeSet = 0x3805EF7A,
		ID_NvAPI_OGL_ExpertModeGet = 0x22ED9516,
		ID_NvAPI_OGL_ExpertModeDefaultsSet = 0xB47A657E,
		ID_NvAPI_OGL_ExpertModeDefaultsGet = 0xAE921F12,
		ID_NvAPI_EnumPhysicalGPUs = 0xE5AC921F,
		ID_NvAPI_EnumTCCPhysicalGPUs = 0xD9930B07,
		ID_NvAPI_EnumLogicalGPUs = 0x48B3EA59,
		ID_NvAPI_GetPhysicalGPUsFromDisplay = 0x34EF9506,
		ID_NvAPI_GetPhysicalGPUFromUnAttachedDisplay = 0x5018ED61,
		ID_NvAPI_GetLogicalGPUFromDisplay = 0xEE1370CF,
		ID_NvAPI_GetLogicalGPUFromPhysicalGPU = 0xADD604D1,
		ID_NvAPI_GetPhysicalGPUsFromLogicalGPU = 0xAEA3FA32,
		ID_NvAPI_GPU_GetShaderSubPipeCount = 0xBE17923,
		ID_NvAPI_GPU_GetGpuCoreCount = 0xC7026A87,
		ID_NvAPI_GPU_GetAllOutputs = 0x7D554F8E,
		ID_NvAPI_GPU_GetConnectedOutputs = 0x1730BFC9,
		ID_NvAPI_GPU_GetConnectedSLIOutputs = 0x680DE09,
		ID_NvAPI_GPU_GetConnectedDisplayIds = 0x78DBA2,
		ID_NvAPI_GPU_GetAllDisplayIds = 0x785210A2,
		ID_NvAPI_GPU_GetConnectedOutputsWithLidState = 0xCF8CAF39,
		ID_NvAPI_GPU_GetConnectedSLIOutputsWithLidState = 0x96043CC7,
		ID_NvAPI_GPU_GetSystemType = 0xBAAABFCC,
		ID_NvAPI_GPU_GetActiveOutputs = 0xE3E89B6F,
		ID_NvAPI_GPU_SetEDID = 0xE83D6456,
		ID_NvAPI_GPU_GetOutputType = 0x40A505E4,
		ID_NvAPI_GPU_ValidateOutputCombination = 0x34C9C2D4,
		ID_NvAPI_GPU_GetFullName = 0xCEEE8E9F,
		ID_NvAPI_GPU_GetPCIIdentifiers = 0x2DDFB66E,
		ID_NvAPI_GPU_GetGPUType = 0xC33BAEB1,
		ID_NvAPI_GPU_GetBusType = 0x1BB18724,
		ID_NvAPI_GPU_GetBusId = 0x1BE0B8E5,
		ID_NvAPI_GPU_GetBusSlotId = 0x2A0A350F,
		ID_NvAPI_GPU_GetIRQ = 0xE4715417,
		ID_NvAPI_GPU_GetVbiosRevision = 0xACC3DA0A,
		ID_NvAPI_GPU_GetVbiosOEMRevision = 0x2D43FB31,
		ID_NvAPI_GPU_GetVbiosVersionString = 0xA561FD7D,
		ID_NvAPI_GPU_GetAGPAperture = 0x6E042794,
		ID_NvAPI_GPU_GetCurrentAGPRate = 0xC74925A0,
		ID_NvAPI_GPU_GetCurrentPCIEDownstreamWidth = 0xD048C3B1,
		ID_NvAPI_GPU_GetPhysicalFrameBufferSize = 0x46FBEB03,
		ID_NvAPI_GPU_GetVirtualFrameBufferSize = 0x5A04B644,
		ID_NvAPI_GPU_GetQuadroStatus = 0xE332FA47,
		ID_NvAPI_GPU_GetBoardInfo = 0x22D54523,
		ID_NvAPI_GPU_GetAllClockFrequencies = 0xDCB616C3,
		ID_NvAPI_GPU_GetPStatesInfoEx = 0x843C0256,
		ID_NvAPI_GPU_GetPStates20 = 0x6FF81213,
		ID_NvAPI_GPU_GetCurrentPstate = 0x927DA4F6,
		ID_NvAPI_GPU_GetDynamicPStatesInfoEx = 0x60DED2ED,
		ID_NvAPI_GPU_GetThermalSettings = 0xE3640A56,
		ID_NvAPI_I2CRead = 0x2FDE12C5,
		ID_NvAPI_I2CWrite = 0xE812EB07,
		ID_NvAPI_GPU_WorkstationFeatureSetup = 0x6C1F3FE4,
		ID_NvAPI_GPU_WorkstationFeatureQuery = 0x4537DF,
		ID_NvAPI_GPU_GetHDCPSupportStatus = 0xF089EEF5,
		ID_NvAPI_GPU_GetTachReading = 0x5F608315,
		ID_NvAPI_GPU_GetECCStatusInfo = 0xCA1DDAF3,
		ID_NvAPI_GPU_GetECCErrorInfo = 0xC71F85A6,
		ID_NvAPI_GPU_ResetECCErrorInfo = 0xC02EEC20,
		ID_NvAPI_GPU_GetECCConfigurationInfo = 0x77A796F3,
		ID_NvAPI_GPU_SetECCConfiguration = 0x1CF639D9,
		ID_NvAPI_GPU_SetScanoutIntensity = 0xA57457A4,
		ID_NvAPI_GPU_GetScanoutIntensityState = 0xE81CE836,
		ID_NvAPI_GPU_SetScanoutWarping = 0xB34BAB4F,
		ID_NvAPI_GPU_GetScanoutWarpingState = 0x6F5435AF,
		ID_NvAPI_GPU_SetScanoutCompositionParameter = 0xF898247D,
		ID_NvAPI_GPU_GetScanoutCompositionParameter = 0x58FE51E6,
		ID_NvAPI_GPU_GetScanoutConfiguration = 0x6A9F5B63,
		ID_NvAPI_GPU_GetScanoutConfigurationEx = 0xE2E1E6F0,
		ID_NvAPI_GPU_GetPerfDecreaseInfo = 0x7F7F4600,
		ID_NvAPI_GPU_QueryIlluminationSupport = 0xA629DA31,
		ID_NvAPI_GPU_GetIllumination = 0x9A1B9365,
		ID_NvAPI_GPU_SetIllumination = 0x254A187,
		ID_NvAPI_EnumNvidiaDisplayHandle = 0x9ABDD40D,
		ID_NvAPI_EnumNvidiaUnAttachedDisplayHandle = 0x20DE9260,
		ID_NvAPI_CreateDisplayFromUnAttachedDisplay = 0x63F9799E,
		ID_NvAPI_GetAssociatedNvidiaDisplayHandle = 0x35C29134,
		ID_NvAPI_DISP_GetAssociatedUnAttachedNvidiaDisplayHandle = 0xA70503B2,
		ID_NvAPI_GetAssociatedNvidiaDisplayName = 0x22A78B05,
		ID_NvAPI_GetUnAttachedAssociatedDisplayName = 0x4888D790,
		ID_NvAPI_EnableHWCursor = 0x2863148D,
		ID_NvAPI_DisableHWCursor = 0xAB163097,
		ID_NvAPI_GetVBlankCounter = 0x67B5DB55,
		ID_NvAPI_SetRefreshRateOverride = 0x3092AC32,
		ID_NvAPI_GetAssociatedDisplayOutputId = 0xD995937E,
		ID_NvAPI_GetDisplayPortInfo = 0xC64FF367,
		ID_NvAPI_SetDisplayPort = 0xFA13E65A,
		ID_NvAPI_GetHDMISupportInfo = 0x6AE16EC3,
		ID_NvAPI_Disp_InfoFrameControl = 0x6067AF3F,
		ID_NvAPI_Disp_ColorControl = 0x92F9D80D,
		ID_NvAPI_Disp_GetHdrCapabilities = 0x84F2A8DF,
		ID_NvAPI_Disp_HdrColorControl = 0x351DA224,
		ID_NvAPI_DISP_GetTiming = 0x175167E9,
		ID_NvAPI_DISP_GetMonitorCapabilities = 0x3B05C7E1,
		ID_NvAPI_DISP_GetMonitorColorCapabilities = 0x6AE4CFB5,
		ID_NvAPI_DISP_EnumCustomDisplay = 0xA2072D59,
		ID_NvAPI_DISP_TryCustomDisplay = 0x1F7DB630,
		ID_NvAPI_DISP_DeleteCustomDisplay = 0x552E5B9B,
		ID_NvAPI_DISP_SaveCustomDisplay = 0x49882876,
		ID_NvAPI_DISP_RevertCustomDisplayTrial = 0xCBBD40F0,
		ID_NvAPI_GetView = 0xD6B99D89,
		ID_NvAPI_GetViewEx = 0xDBBC0AF4,
		ID_NvAPI_GetSupportedViews = 0x66FB7FC0,
		ID_NvAPI_DISP_GetDisplayIdByDisplayName = 0xAE457190,
		ID_NvAPI_DISP_GetGDIPrimaryDisplayId = 0x1E9D8A31,
		ID_NvAPI_DISP_GetDisplayConfig = 0x11ABCCF8,
		ID_NvAPI_DISP_SetDisplayConfig = 0x5D8CF8DE,
		ID_NvAPI_Mosaic_GetSupportedTopoInfo = 0xFDB63C81,
		ID_NvAPI_Mosaic_GetTopoGroup = 0xCB89381D,
		ID_NvAPI_Mosaic_GetOverlapLimits = 0x989685F0,
		ID_NvAPI_Mosaic_SetCurrentTopo = 0x9B542831,
		ID_NvAPI_Mosaic_GetCurrentTopo = 0xEC32944E,
		ID_NvAPI_Mosaic_EnableCurrentTopo = 0x5F1AA66C,
		ID_NvAPI_Mosaic_GetDisplayViewportsByResolution = 0xDC6DC8D3,
		ID_NvAPI_Mosaic_SetDisplayGrids = 0x4D959A89,
		ID_NvAPI_Mosaic_ValidateDisplayGrids = 0xCF43903D,
		ID_NvAPI_Mosaic_EnumDisplayModes = 0x78DB97D7,
		ID_NvAPI_Mosaic_EnumDisplayGrids = 0xDF2887AF,
		ID_NvAPI_GetSupportedMosaicTopologies = 0x410B5C25,
		ID_NvAPI_GetCurrentMosaicTopology = 0xF60852BD,
		ID_NvAPI_SetCurrentMosaicTopology = 0xD54B8989,
		ID_NvAPI_EnableCurrentMosaicTopology = 0x74073CC9,
		ID_NvAPI_GSync_EnumSyncDevices = 0xD9639601,
		ID_NvAPI_GSync_QueryCapabilities = 0x44A3F1D1,
		ID_NvAPI_GSync_GetTopology = 0x4562BC38,
		ID_NvAPI_GSync_SetSyncStateSettings = 0x60ACDFDD,
		ID_NvAPI_GSync_GetControlParameters = 0x16DE1C6A,
		ID_NvAPI_GSync_SetControlParameters = 0x8BBFF88B,
		ID_NvAPI_GSync_AdjustSyncDelay = 0x2D11FF51,
		ID_NvAPI_GSync_GetSyncStatus = 0xF1F5B434,
		ID_NvAPI_GSync_GetStatusParameters = 0x70D404EC,
		ID_NvAPI_D3D_GetCurrentSLIState = 0x4B708B54,
		ID_NvAPI_D3D9_RegisterResource = 0xA064BDFC,
		ID_NvAPI_D3D9_UnregisterResource = 0xBB2B17AA,
		ID_NvAPI_D3D9_AliasSurfaceAsTexture = 0xE5CEAE41,
		ID_NvAPI_D3D9_StretchRectEx = 0x22DE03AA,
		ID_NvAPI_D3D9_ClearRT = 0x332D3942,
		ID_NvAPI_D3D_GetObjectHandleForResource = 0xFCEAC864,
		ID_NvAPI_D3D_SetResourceHint = 0x6C0ED98C,
		ID_NvAPI_D3D_BeginResourceRendering = 0x91123D6A,
		ID_NvAPI_D3D_EndResourceRendering = 0x37E7191C,
		ID_NvAPI_D3D9_GetSurfaceHandle = 0xF2DD3F2,
		ID_NvAPI_D3D9_VideoSetStereoInfo = 0xB852F4DB,
		ID_NvAPI_D3D11_CreateDevice = 0x6A16D3A0,
		ID_NvAPI_D3D11_CreateDeviceAndSwapChain = 0xBB939EE5,
		ID_NvAPI_D3D11_SetDepthBoundsTest = 0x7AAF7A04,
		ID_NvAPI_D3D11_IsNvShaderExtnOpCodeSupported = 0x5F68DA40,
		ID_NvAPI_D3D11_SetNvShaderExtnSlot = 0x8E90BB9F,
		ID_NvAPI_D3D11_BeginUAVOverlapEx = 0xBA08208A,
		ID_NvAPI_D3D11_BeginUAVOverlap = 0x65B93CA8,
		ID_NvAPI_D3D11_EndUAVOverlap = 0x2216A357,
		ID_NvAPI_D3D_SetFPSIndicatorState = 0xA776E8DB,
		ID_NvAPI_D3D11_CreateRasterizerState = 0xDB8D28AF,
		ID_NvAPI_D3D_ConfigureANSEL = 0x341C6C7F,
		ID_NvAPI_D3D11_AliasMSAATexture2DAsNonMSAA = 0xF1C54FC9,
		ID_NvAPI_D3D11_CreateGeometryShaderEx_2 = 0x99ED5C1C,
		ID_NvAPI_D3D11_CreateVertexShaderEx = 0xBEAA0B2,
		ID_NvAPI_D3D11_CreateHullShaderEx = 0xB53CAB00,
		ID_NvAPI_D3D11_CreateDomainShaderEx = 0xA0D7180D,
		ID_NvAPI_D3D11_CreateFastGeometryShaderExplicit = 0x71AB7C9C,
		ID_NvAPI_D3D11_CreateFastGeometryShader = 0x525D43BE,
		ID_NvAPI_D3D12_CreateGraphicsPipelineState = 0x2FC28856,
		ID_NvAPI_D3D12_CreateComputePipelineState = 0x2762DEAC,
		ID_NvAPI_D3D12_SetDepthBoundsTestValues = 0xB9333FE9,
		ID_NvAPI_D3D12_IsNvShaderExtnOpCodeSupported = 0x3DFACEC8,
		ID_NvAPI_D3D_IsGSyncCapable = 0x9C1EED78,
		ID_NvAPI_D3D_IsGSyncActive = 0xE942B0FF,
		ID_NvAPI_D3D1x_DisableShaderDiskCache = 0xD0CBCA7D,
		ID_NvAPI_D3D11_MultiGPU_GetCaps = 0xD2D25687,
		ID_NvAPI_D3D11_MultiGPU_Init = 0x17BE49E,
		ID_NvAPI_D3D11_CreateMultiGPUDevice = 0xBDB20007,
		ID_NvAPI_D3D_QuerySinglePassStereoSupport = 0x6F5F0A6D,
		ID_NvAPI_D3D_SetSinglePassStereoMode = 0xA39E6E6E,
		ID_NvAPI_D3D12_QuerySinglePassStereoSupport = 0x3B03791B,
		ID_NvAPI_D3D12_SetSinglePassStereoMode = 0x83556D87,
		ID_NvAPI_D3D_QueryModifiedWSupport = 0xCBF9F4F5,
		ID_NvAPI_D3D_SetModifiedWMode = 0x6EA4BF4,
		ID_NvAPI_D3D12_QueryModifiedWSupport = 0x51235248,
		ID_NvAPI_D3D12_SetModifiedWMode = 0xE1FDABA7,
		ID_NvAPI_D3D_RegisterDevice = 0x8C02C4D0,
		ID_NvAPI_D3D11_MultiDrawInstancedIndirect = 0xD4E26BBF,
		ID_NvAPI_D3D11_MultiDrawIndexedInstancedIndirect = 0x59E890F9,
		ID_NvAPI_D3D_ImplicitSLIControl = 0x2AEDE111,
		ID_NvAPI_VIO_GetCapabilities = 0x1DC91303,
		ID_NvAPI_VIO_Open = 0x44EE4841,
		ID_NvAPI_VIO_Close = 0xD01BD237,
		ID_NvAPI_VIO_Status = 0xE6CE4F1,
		ID_NvAPI_VIO_SyncFormatDetect = 0x118D48A3,
		ID_NvAPI_VIO_GetConfig = 0xD34A789B,
		ID_NvAPI_VIO_SetConfig = 0xE4EEC07,
		ID_NvAPI_VIO_SetCSC = 0xA1EC8D74,
		ID_NvAPI_VIO_GetCSC = 0x7B0D72A3,
		ID_NvAPI_VIO_SetGamma = 0x964BF452,
		ID_NvAPI_VIO_GetGamma = 0x51D53D06,
		ID_NvAPI_VIO_SetSyncDelay = 0x2697A8D1,
		ID_NvAPI_VIO_GetSyncDelay = 0x462214A9,
		ID_NvAPI_VIO_GetPCIInfo = 0xB981D935,
		ID_NvAPI_VIO_IsRunning = 0x96BD040E,
		ID_NvAPI_VIO_Start = 0xCDE8E1A3,
		ID_NvAPI_VIO_Stop = 0x6BA2A5D6,
		ID_NvAPI_VIO_IsFrameLockModeCompatible = 0x7BF0A94D,
		ID_NvAPI_VIO_EnumDevices = 0xFD7C5557,
		ID_NvAPI_VIO_QueryTopology = 0x869534E2,
		ID_NvAPI_VIO_EnumSignalFormats = 0xEAD72FE4,
		ID_NvAPI_VIO_EnumDataFormats = 0x221FA8E8,
		ID_NvAPI_Stereo_CreateConfigurationProfileRegistryKey = 0xBE7692EC,
		ID_NvAPI_Stereo_DeleteConfigurationProfileRegistryKey = 0xF117B834,
		ID_NvAPI_Stereo_SetConfigurationProfileValue = 0x24409F48,
		ID_NvAPI_Stereo_DeleteConfigurationProfileValue = 0x49BCEECF,
		ID_NvAPI_Stereo_Enable = 0x239C4545,
		ID_NvAPI_Stereo_Disable = 0x2EC50C2B,
		ID_NvAPI_Stereo_IsEnabled = 0x348FF8E1,
		ID_NvAPI_Stereo_GetStereoSupport = 0x296C434D,
		ID_NvAPI_Stereo_CreateHandleFromIUnknown = 0xAC7E37F4,
		ID_NvAPI_Stereo_DestroyHandle = 0x3A153134,
		ID_NvAPI_Stereo_Activate = 0xF6A1AD68,
		ID_NvAPI_Stereo_Deactivate = 0x2D68DE96,
		ID_NvAPI_Stereo_IsActivated = 0x1FB0BC30,
		ID_NvAPI_Stereo_GetSeparation = 0x451F2134,
		ID_NvAPI_Stereo_SetSeparation = 0x5C069FA3,
		ID_NvAPI_Stereo_DecreaseSeparation = 0xDA044458,
		ID_NvAPI_Stereo_IncreaseSeparation = 0xC9A8ECEC,
		ID_NvAPI_Stereo_GetConvergence = 0x4AB00934,
		ID_NvAPI_Stereo_SetConvergence = 0x3DD6B54B,
		ID_NvAPI_Stereo_DecreaseConvergence = 0x4C87E317,
		ID_NvAPI_Stereo_IncreaseConvergence = 0xA17DAABE,
		ID_NvAPI_Stereo_GetFrustumAdjustMode = 0xE6839B43,
		ID_NvAPI_Stereo_SetFrustumAdjustMode = 0x7BE27FA2,
		ID_NvAPI_Stereo_CaptureJpegImage = 0x932CB140,
		ID_NvAPI_Stereo_InitActivation = 0xC7177702,
		ID_NvAPI_Stereo_Trigger_Activation = 0xD6C6CD2,
		ID_NvAPI_Stereo_CapturePngImage = 0x8B7E99B5,
		ID_NvAPI_Stereo_ReverseStereoBlitControl = 0x3CD58F89,
		ID_NvAPI_Stereo_SetNotificationMessage = 0x6B9B409E,
		ID_NvAPI_Stereo_SetActiveEye = 0x96EEA9F8,
		ID_NvAPI_Stereo_SetDriverMode = 0x5E8F0BEC,
		ID_NvAPI_Stereo_GetEyeSeparation = 0xCE653127,
		ID_NvAPI_Stereo_IsWindowedModeSupported = 0x40C8ED5E,
		ID_NvAPI_Stereo_SetSurfaceCreationMode = 0xF5DCFCBA,
		ID_NvAPI_Stereo_GetSurfaceCreationMode = 0x36F1C736,
		ID_NvAPI_Stereo_Debug_WasLastDrawStereoized = 0xED4416C5,
		ID_NvAPI_Stereo_SetDefaultProfile = 0x44F0ECD1,
		ID_NvAPI_Stereo_GetDefaultProfile = 0x624E21C2,
		ID_NvAPI_D3D1x_CreateSwapChain = 0x1BC21B66,
		ID_NvAPI_D3D9_CreateSwapChain = 0x1A131E09,
		ID_NvAPI_DRS_CreateSession = 0x694D52E,
		ID_NvAPI_DRS_DestroySession = 0xDAD9CFF8,
		ID_NvAPI_DRS_LoadSettings = 0x375DBD6B,
		ID_NvAPI_DRS_SaveSettings = 0xFCBC7E14,
		ID_NvAPI_DRS_LoadSettingsFromFile = 0xD3EDE889,
		ID_NvAPI_DRS_SaveSettingsToFile = 0x2BE25DF8,
		ID_NvAPI_DRS_CreateProfile = 0xCC176068,
		ID_NvAPI_DRS_DeleteProfile = 0x17093206,
		ID_NvAPI_DRS_SetCurrentGlobalProfile = 0x1C89C5DF,
		ID_NvAPI_DRS_GetCurrentGlobalProfile = 0x617BFF9F,
		ID_NvAPI_DRS_GetProfileInfo = 0x61CD6FD6,
		ID_NvAPI_DRS_SetProfileInfo = 0x16ABD3A9,
		ID_NvAPI_DRS_FindProfileByName = 0x7E4A9A0B,
		ID_NvAPI_DRS_EnumProfiles = 0xBC371EE0,
		ID_NvAPI_DRS_GetNumProfiles = 0x1DAE4FBC,
		ID_NvAPI_DRS_CreateApplication = 0x4347A9DE,
		ID_NvAPI_DRS_DeleteApplicationEx = 0xC5EA85A1,
		ID_NvAPI_DRS_DeleteApplication = 0x2C694BC6,
		ID_NvAPI_DRS_GetApplicationInfo = 0xED1F8C69,
		ID_NvAPI_DRS_EnumApplications = 0x7FA2173A,
		ID_NvAPI_DRS_FindApplicationByName = 0xEEE566B2,
		ID_NvAPI_DRS_SetSetting = 0x577DD202,
		ID_NvAPI_DRS_GetSetting = 0x73BF8338,
		ID_NvAPI_DRS_EnumSettings = 0xAE3039DA,
		ID_NvAPI_DRS_EnumAvailableSettingIds = 0xF020614A,
		ID_NvAPI_DRS_EnumAvailableSettingValues = 0x2EC39F90,
		ID_NvAPI_DRS_GetSettingIdFromName = 0xCB7309CD,
		ID_NvAPI_DRS_GetSettingNameFromId = 0xD61CBE6E,
		ID_NvAPI_DRS_DeleteProfileSetting = 0xE4A26362,
		ID_NvAPI_DRS_RestoreAllDefaults = 0x5927B094,
		ID_NvAPI_DRS_RestoreProfileDefault = 0xFA5F6134,
		ID_NvAPI_DRS_RestoreProfileDefaultSetting = 0x53F0381E,
		ID_NvAPI_DRS_GetBaseProfile = 0xDA8466A0,
		ID_NvAPI_SYS_GetChipSetInfo = 0x53DABBCA,
		ID_NvAPI_SYS_GetLidAndDockInfo = 0xCDA14D8A,
		ID_NvAPI_SYS_GetDisplayIdFromGpuAndOutputId = 0x8F2BAB4,
		ID_NvAPI_SYS_GetGpuAndOutputIdFromDisplayId = 0x112BA1A5,
		ID_NvAPI_SYS_GetPhysicalGpuFromDisplayId = 0x9EA74659,
		ID_NvAPI_Unload = 0xD22BDD7E,


		ID_NvAPI_GetPhysicalGPUFromDisplay = 0x1890E8DA,
		ID_NvAPI_GetPhysicalGPUFromGPUID = 0x5380AD1A,
		ID_NvAPI_GetGPUIDfromPhysicalGPU = 0x6533EA3E,
		ID_NvAPI_GetInfoFrameStatePvt = 0x7FC17574,
		ID_NvAPI_LoadMicrocode = 0x3119F36E,
		ID_NvAPI_GetLoadedMicrocodePrograms = 0x919B3136,
		ID_NvAPI_GetDisplayDriverBuildTitle = 0x7562E947,
		ID_NvAPI_GetDisplayDriverCompileType = 0x988AEA78,
		ID_NvAPI_GetDisplayDriverSecurityLevel = 0x9D772BBA,
		ID_NvAPI_AccessDisplayDriverRegistry = 0xF5579360,
		ID_NvAPI_GetDisplayDriverRegistryPath = 0x0E24CEEE,
		ID_NvAPI_GetUnAttachedDisplayDriverRegistryPath = 0x633252D8,
		ID_NvAPI_GPU_GetRawFuseData = 0xE0B1DCE9,
		ID_NvAPI_GPU_GetFoundry = 0x5D857A00,
		ID_NvAPI_GPU_GetVPECount = 0xD8CBF37B,
		ID_NvAPI_GPU_GetTargetID = 0x35B5FD2F,
		ID_NvAPI_GPU_GetShortName = 0xD988F0F3,
		ID_NvAPI_GPU_GetVbiosMxmVersion = 0xE1D5DABA,
		ID_NvAPI_GPU_GetVbiosImage = 0xFC13EE11,
		ID_NvAPI_GPU_GetMXMBlock = 0xB7AB19B9,
		ID_NvAPI_GPU_SetCurrentPCIEWidth = 0x3F28E1B9,
		ID_NvAPI_GPU_SetCurrentPCIESpeed = 0x3BD32008,
		ID_NvAPI_GPU_GetPCIEInfo = 0xE3795199,
		ID_NvAPI_GPU_ClearPCIELinkErrorInfo = 0x8456FF3D,
		ID_NvAPI_GPU_ClearPCIELinkAERInfo = 0x521566BB,
		ID_NvAPI_GPU_GetFrameBufferCalibrationLockFailures = 0x524B9773,
		ID_NvAPI_GPU_SetDisplayUnderflowMode = 0x387B2E41,
		ID_NvAPI_GPU_GetDisplayUnderflowStatus = 0xED9E8057,
		ID_NvAPI_GPU_GetBarInfo = 0xE4B701E3,
		ID_NvAPI_GPU_GetPSFloorSweepStatus = 0xDEE047AB,
		ID_NvAPI_GPU_GetVSFloorSweepStatus = 0xD4F3944C,
		ID_NvAPI_GPU_GetSerialNumber = 0x14B83A5F,
		ID_NvAPI_GPU_GetManufacturingInfo = 0xA4218928,
		ID_NvAPI_GPU_GetRamConfigStrap = 0x51CCDB2A,
		ID_NvAPI_GPU_GetRamBusWidth = 0x7975C581,
		ID_NvAPI_GPU_GetRamBankCount = 0x17073A3C,
		ID_NvAPI_GPU_GetArchInfo = 0xD8265D24,
		ID_NvAPI_GPU_GetExtendedMinorRevision = 0x25F17421,
		ID_NvAPI_GPU_GetSampleType = 0x32E1D697,
		ID_NvAPI_GPU_GetHardwareQualType = 0xF91E777B,
		ID_NvAPI_GPU_GetAllClocks = 0x1BD69F49,
		ID_NvAPI_GPU_SetClocks = 0x6F151055,
		ID_NvAPI_GPU_SetPerfHybridMode = 0x7BC207F8,
		ID_NvAPI_GPU_GetPerfHybridMode = 0x5D7CCAEB,
		ID_NvAPI_RestartDisplayDriver = 0xB4B26B65,
		ID_NvAPI_GPU_GetAllGpusOnSameBoard = 0x4DB019E6,

		ID_NvAPI_SetTopologyDisplayGPU = 0xF409D5E5,
		ID_NvAPI_GetTopologyDisplayGPU = 0x813D89A8,
		ID_NvAPI_SYS_GetSliApprovalCookie = 0xB539A26E,

		ID_NvAPI_CreateUnAttachedDisplayFromDisplay = 0xA0C72EE4,
		ID_NvAPI_GetDriverModel = 0x25EEB2C4,
		ID_NvAPI_GPU_CudaEnumComputeCapableGpus = 0x5786CC6E,
		ID_NvAPI_GPU_PhysxSetState = 0x4071B85E,
		ID_NvAPI_GPU_PhysxQueryRecommendedState = 0x7A4174F4,
		ID_NvAPI_GPU_GetDeepIdleState = 0x1AAD16B4,
		ID_NvAPI_GPU_SetDeepIdleState = 0x568A2292,
		ID_NvAPI_GetScalingCaps = 0x8E875CF9,
		ID_NvAPI_GPU_GetThermalTable = 0xC729203C,
		ID_NvAPI_GPU_GetHybridControllerInfo = 0xD26B8A58,
		ID_NvAPI_SYS_SetPostOutput = 0xD3A092B1,

		ID_NvAPI_GPU_GetPStates = 0x60DED2ED,
		ID_NvAPI_GPU_GetUsages = 0x189A1FDF,
		ID_NvAPI_GPU_GetCoolerSettings = 0xDA141340,
		ID_NvAPI_GPU_SetCoolerLevels = 0x891FA0AE,
		//ID_NvAPI_GPU_GetMemoryInfo = 0x774AA982,
	};

	bool initialize()
	{
		mDllHandle = LoadLibraryA("nvapi.dll");
		if( mDllHandle == nullptr )
			return false;

		if( !getFuncAddress("nvapi_QueryInterface", nvapi_QueryInterface) )
			return false;

#define QUERY_INTERFACE( NAME )\
	if ( !queryInterface(ID_##NAME , NAME) )\
		return false;

		QUERY_INTERFACE(NvAPI_Initialize);
		QUERY_INTERFACE(NvAPI_Unload);
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

		if( !checkNvStatus(NvAPI_Initialize()) )
			return false;

		bInitialized = true;
		return true;
	}


	void release()
	{
		if( bInitialized )
		{
			NvAPI_Unload();
			if( mDllHandle )
			{
				FreeLibrary(mDllHandle);
				mDllHandle = NULL;
			}
			bInitialized = false;
		}
	}

	bool checkNvStatus(NvAPI_Status status)
	{
		if( status != NVAPI_OK )
		{
			return false;
		}
		return true;
	}

	template< class TFunc >
	bool getFuncAddress(char const* name, TFunc& func)
	{
		func = (TFunc) ::GetProcAddress(mDllHandle, name);
		return func != nullptr;
	}
	template< class TFunc >
	bool queryInterface(unsigned int interfaceId, TFunc& func)
	{
		func = (TFunc)nvapi_QueryInterface(interfaceId);
		return func != nullptr;
	}

	void* (*nvapi_QueryInterface)(unsigned int uiInterfaceID);
	NvAPI_Status(__cdecl *NvAPI_Initialize)();
	NvAPI_Status(__cdecl *NvAPI_Unload)();
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

	HMODULE mDllHandle;
	bool    bInitialized = false;
};

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
		std::vector< NvPhysicalGpuHandle > GpuHandles;
	};

	std::vector< DisplayHandle > mDisplays;
	std::vector< NvPhysicalGpuHandle > mGpuHandles;
};

GPUDeviceQuery* GPUDeviceQuery::Create()
{
	auto result = new NvGpuDeviceQuery;
	if( result && !result->initialize() )
	{
		delete result;
		return nullptr;
	}
	return result;
}
