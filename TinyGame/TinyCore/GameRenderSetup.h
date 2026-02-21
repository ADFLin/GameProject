#pragma once
#ifndef GameRenderSetup_H_5ED8A370_2B22_4CA0_A906_B67D32EE1FB9
#define GameRenderSetup_H_5ED8A370_2B22_4CA0_A906_B67D32EE1FB9

enum class ERenderSystem
{
	None,
	OpenGL,
	D3D11,
	D3D12,
	Vulkan,
};

struct RenderSystemConfigs
{
	int32 screenWidth;
	int32 screenHeight;
	int32 numSamples;
	bool  bVSyncEnable;
	bool  bDebugMode;
	bool  bWasUsedPlatformGraphics;
	bool  bUseRenderThread;

	RenderSystemConfigs()
	{
		screenWidth = 0;
		screenHeight = 0;
		numSamples = 1;
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
