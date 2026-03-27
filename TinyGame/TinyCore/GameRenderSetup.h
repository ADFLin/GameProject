#pragma once

#ifndef GameRenderSetup_H_5ED8A370_2B22_4CA0_A906_B67D32EE1FB9
#define GameRenderSetup_H_5ED8A370_2B22_4CA0_A906_B67D32EE1FB9

#include "CString.h"

enum class ERenderSystem
{
	None,
	OpenGL,
	D3D11,
	D3D12,
	Vulkan,
};

FORCEINLINE char const* ToString(ERenderSystem system)
{
	switch (system)
	{
	case ERenderSystem::OpenGL: return "OpenGL";
	case ERenderSystem::D3D11:  return "D3D11";
	case ERenderSystem::D3D12: return "D3D12";
	case ERenderSystem::Vulkan: return "Vulkan";
	}
	return "None";
}

FORCEINLINE ERenderSystem StringToRenderSystem(char const* str)
{
	if (FCString::CompareIgnoreCase(str, "OpenGL") == 0)
	{
		return ERenderSystem::OpenGL;
	}
	else if (FCString::CompareIgnoreCase(str, "D3D11") == 0)
	{
		return ERenderSystem::D3D11;
	}
	else if (FCString::CompareIgnoreCase(str, "D3D12") == 0)
	{
		return ERenderSystem::D3D12;
	}
	else if (FCString::CompareIgnoreCase(str, "Vulkan") == 0)
	{
		return ERenderSystem::Vulkan;
	}
	return ERenderSystem::None;
}

struct RenderSystemConfigs
{
	int32 screenWidth;
	int32 screenHeight;
	int32 numSamples;
	int32 bufferCount;
	bool  bVSyncEnable;
	bool  bDebugMode;
	bool  bWasUsedPlatformGraphics;
	bool  bUseRenderThread;

	RenderSystemConfigs()
	{
		screenWidth = 0;
		screenHeight = 0;
		numSamples = 1;
		bufferCount = 0;
		bVSyncEnable = false;
		bDebugMode = true;
		bWasUsedPlatformGraphics = false;
		bUseRenderThread = false;
	}
};


class IGameRenderSetup
{
public:
	virtual ~IGameRenderSetup(){}

	virtual ERenderSystem getDefaultRenderSystem()
	{
		return ERenderSystem::None;
	}

	virtual void configRenderSystem(ERenderSystem systenName, RenderSystemConfigs& systemConfigs)
	{

	}

	virtual bool isRenderSystemSupported(ERenderSystem systemName)
	{
		return true;
	}
	virtual bool setupRenderResource(ERenderSystem systemName)
	{
		return true;
	}

	virtual void preShutdownRenderSystem(bool bReInit = false)
	{

	}

	virtual void setupPlatformGraphic()
	{

	}
};


class LegacyPlatformRenderSetup : public IGameRenderSetup
{
public:
	virtual void configRenderSystem(ERenderSystem systenName, RenderSystemConfigs& systemConfigs)
	{
		systemConfigs.bWasUsedPlatformGraphics = true;
	}
};

#endif // GameRenderSetup_H_5ED8A370_2B22_4CA0_A906_B67D32EE1FB9
