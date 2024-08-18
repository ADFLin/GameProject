#include "RHICommand.h"

#include "RHICommon.h"
#include "RHIGlobalResource.h"

#include "ShaderManager.h"
#include "GpuProfiler.h"

#if RHI_USE_RESOURCE_TRACE
#include "RHITraceScope.h"
#endif
#include "Launch/CommandlLine.h"

namespace Render
{

	char const* ToString(RHISystemName name)
	{
		switch (name)
		{
		case RHISystemName::D3D11: return "D3D11";
		case RHISystemName::D3D12: return "D3D12";
		case RHISystemName::OpenGL: return "OpenGL";
		case RHISystemName::Vulkan: return "Vulkan";
		}
		return "Unknown";
	}


#if CORE_SHARE_CODE
	RHISystem* GRHISystem = nullptr;
	float GRHIClipZMin = 0;
	float GRHIProjectionYSign = 1;
	float GRHIVericalFlip = 1;
	bool GRHISupportMeshShader = false;
	bool GRHISupportRayTracing = false;
	bool GRHIPrefEnabled = false;
	bool GRHISupportVPAndRTArrayIndexFromAnyShaderFeedingRasterizer = false;

#define EXECUTE_RHI_FUNC( CODE ) GRHISystem->CODE

#define RHI_USE_FACTORY_LIST 1

#if RHI_USE_FACTORY_LIST
	struct RHIFactoryInfo
	{
		RHISystemName name;
		RHISystemFactory* factory;
	};
	TArray< RHIFactoryInfo > GRHISystemFactoryList;

	RHISystemFactory* RHIGetSystemFactory(RHISystemName name)
	{
		int index = GRHISystemFactoryList.findIndexPred(
			[name](auto const& info) { return info.name == name; });

		if (index == INDEX_NONE)
			return nullptr;

		return GRHISystemFactoryList[index].factory;
	}


	void RHIRegisterSystem(RHISystemName name, RHISystemFactory* factory)
	{
#if 0
		if (!VERIFY(RHIGetSystemFactory( name ) == nullptr))
		{
			LogWarning(0, "RHISystem %s is registered already !!", ToString(name));
			return;
		}
#endif
		GRHISystemFactoryList.push_back({ name , factory });
	}
	void RHIUnregisterSystem(RHISystemName name)
	{
		GRHISystemFactoryList.removePred([name](auto const& info)
		{
			return info.name == name;
		});
	}

#else
	std::unordered_map< RHISystemName, RHISystemFactory*> GRHISystemFactoryMap;

	RHISystemFactory* RHIGetSystemFactory(RHISystemName name)
	{
		auto iter = GRHISystemFactoryMap.find(name);
		if (iter == GRHISystemFactoryMap.end())
			return nullptr;
		return iter->second;
	}

	void RHIRegisterSystem(RHISystemName name, RHISystemFactory* factory)
	{
		GRHISystemFactoryMap.emplace(name, factory);
	}
	void RHIUnregisterSystem(RHISystemName name)
	{
		RemoveValue(GRHISystemFactoryMap, name);
	}

#endif

	bool RHISystemIsSupported(RHISystemName name)
	{
		return RHIGetSystemFactory(name) != nullptr;
	}

	TArray< RHISystemEvent > GInitEventList;
	TArray< RHISystemEvent > GShutdownEventList;
	void RHIRegisterSystemInitializeEvent(RHISystemEvent event)
	{
		GInitEventList.push_back(std::move(event));
	}
	void RHIRegisterSystemShutdownEvent(RHISystemEvent event)
	{
		GShutdownEventList.push_back(std::move(event));
	}

	bool RHISystemInitialize(RHISystemName name, RHISystemInitParams const& initParam)
	{
		if (GRHISystem != nullptr)
		{
			LogWarning(0, "RHISystem is Initialized!");
			return true;
		}

		RHISystemFactory* factory = RHIGetSystemFactory(name);
		if (factory)
		{
			GRHISystem = factory->createRHISystem(name);
		}
		else
		{
			LogError("RHI System %s can't found", ToString(name));
			return false;
		}

		LogMsg("===== Init RHI System : %s ====", ToString(name));

		GRHIDeviceVendorName = DeviceVendorName::Unknown;
		GRHISupportRayTracing = false;
		GRHISupportMeshShader = false;
		GRHISupportVPAndRTArrayIndexFromAnyShaderFeedingRasterizer = false;

		TChar const* cmdLine = FCommandLine::Get();
		GRHIPrefEnabled = FCString::StrStr(cmdLine, "-RHIPerf");

		FRHIResourceTable::Initialize();

		if (GRHISystem && !GRHISystem->initialize(initParam))
		{
			delete GRHISystem;
			GRHISystem = nullptr;
		}

		if (GRHISystem)
		{
			ShaderFormat* shaderFormat = GRHISystem->createShaderFormat();
			if (shaderFormat == nullptr)
			{
				LogError("Can't create shader format for %d system", (int)GRHISystem->getName());
				return false;
			}

			if (!ShaderManager::Get().initialize(*shaderFormat))
			{
				LogError("ShaderManager can't initialize");
				return false;
			}


			IGlobalRenderResource::RestoreAllResources();

			InitGlobalRenderResource();

			for (auto& event : GInitEventList)
			{
				if (event)
				{
					event();
				}
			}
			GInitEventList.clear();
		}

		return GRHISystem != nullptr;
	}

	void RHIClearResourceReference()
	{
		GRHISystem->clearResourceReference();
	}

	void RHISystemShutdown()
	{	
		GpuProfiler::Get().releaseRHIResource();
		for (auto& event : GShutdownEventList)
		{
			if (event)
			{
				event();
			}
		}
		GShutdownEventList.clear();
		ReleaseGlobalRenderResource();

		ShaderManager::Get().clearnupRHIResouse();
		IGlobalRenderResource::ReleaseAllResources();

		GRHISystem->shutdown();
		delete GRHISystem;
		GRHISystem = nullptr;

		FRHIResourceTable::Release();
		RHIResource::DumpResource();
	}

	bool RHIBeginRender()
	{
		return EXECUTE_RHI_FUNC( RHIBeginRender() );
	}

	void RHIEndRender(bool bPresent)
	{
		EXECUTE_RHI_FUNC( RHIEndRender(bPresent) );
	}

	RHISwapChain* RHICreateSwapChain(SwapChainCreationInfo const& info)
	{
		return EXECUTE_RHI_FUNC( RHICreateSwapChain(info) );
	}

	RHITexture1D* RHI_TRACE_FUNC(RHICreateTexture1D, ETexture::Format format, int length, int numMipLevel, int numSamples, TextureCreationFlags creationFlags, void* data)
	{
		TextureDesc desc = TextureDesc::Type1D(format, length).MipLevel(numMipLevel).Samples(numSamples).Flags(creationFlags);
		RHI_TRACE_CODE( EXECUTE_RHI_FUNC( RHICreateTexture1D(desc, data) ) );
	}

	RHITexture1D* RHI_TRACE_FUNC(RHICreateTexture1D, TextureDesc const& desc, void* data)
	{
		RHI_TRACE_CODE(EXECUTE_RHI_FUNC(RHICreateTexture1D(desc, data)));
	}

	RHITexture2D* RHI_TRACE_FUNC(RHICreateTexture2D , ETexture::Format format, int w, int h, int numMipLevel, int numSamples, TextureCreationFlags creationFlags, void* data , int dataAlign)
	{
		TextureDesc desc = TextureDesc::Type2D(format, w, h).MipLevel(numMipLevel).Samples(numSamples).Flags(creationFlags);
		RHI_TRACE_CODE( EXECUTE_RHI_FUNC( RHICreateTexture2D(desc, data, dataAlign) ) );
	}
	RHITexture2D* RHI_TRACE_FUNC(RHICreateTexture2D, TextureDesc const& desc, void* data, int dataAlign)
	{
		RHI_TRACE_CODE(EXECUTE_RHI_FUNC(RHICreateTexture2D(desc, data, dataAlign)));
	}

	RHITexture3D* RHI_TRACE_FUNC(RHICreateTexture3D ,ETexture::Format format, int sizeX, int sizeY, int sizeZ, int numMipLevel, int numSamples, TextureCreationFlags creationFlags, void* data)
	{
		TextureDesc desc = TextureDesc::Type3D(format, sizeX, sizeY, sizeZ).MipLevel(numMipLevel).Samples(numSamples).Flags(creationFlags);
		RHI_TRACE_CODE( EXECUTE_RHI_FUNC(RHICreateTexture3D(desc, data)) );
	}
	RHITexture3D* RHI_TRACE_FUNC(RHICreateTexture3D, TextureDesc const& desc, void* data)
	{
		RHI_TRACE_CODE(EXECUTE_RHI_FUNC(RHICreateTexture3D(desc, data)));
	}

	RHITextureCube* RHI_TRACE_FUNC(RHICreateTextureCube, ETexture::Format format, int size, int numMipLevel, TextureCreationFlags creationFlags, void* data[])
	{
		TextureDesc desc = TextureDesc::TypeCube(format, size).MipLevel(numMipLevel).Flags(creationFlags);
		RHI_TRACE_CODE( EXECUTE_RHI_FUNC(RHICreateTextureCube(desc, data)) );
	}
	RHITextureCube* RHI_TRACE_FUNC(RHICreateTextureCube, TextureDesc const& desc, void* data[])
	{
		RHI_TRACE_CODE(EXECUTE_RHI_FUNC(RHICreateTextureCube(desc, data)));
	}

	RHITexture2DArray* RHI_TRACE_FUNC(RHICreateTexture2DArray, ETexture::Format format, int w, int h, int layerSize, int numMipLevel, int numSamples, TextureCreationFlags creationFlags, void* data)
	{
		TextureDesc desc = TextureDesc::Type2DArray(format, w, h, layerSize).MipLevel(numMipLevel).Samples(numSamples).Flags(creationFlags);
		RHI_TRACE_CODE( EXECUTE_RHI_FUNC(RHICreateTexture2DArray(desc, data)) );
	}
	RHI_API RHITexture2DArray* RHI_TRACE_FUNC(RHICreateTexture2DArray, TextureDesc const& desc, void* data)
	{
		RHI_TRACE_CODE(EXECUTE_RHI_FUNC(RHICreateTexture2DArray(desc, data)));
	}

	RHITexture2D* RHI_TRACE_FUNC(RHICreateTextureDepth, ETexture::Format format, int w, int h, int numMipLevel, int numSamples, TextureCreationFlags creationFlags)
	{
		TextureDesc desc = TextureDesc::Type2D(format, w, h).MipLevel(numMipLevel).Samples(numSamples).Flags(creationFlags);
		RHI_TRACE_CODE( EXECUTE_RHI_FUNC(RHICreateTextureDepth(desc)) );
	}
	RHITexture2D* RHI_TRACE_FUNC(RHICreateTextureDepth, TextureDesc const& desc)
	{
		RHI_TRACE_CODE(EXECUTE_RHI_FUNC(RHICreateTextureDepth(desc)));
	}

	RHIBuffer* RHI_TRACE_FUNC(RHICreateBuffer, uint32 elementSize, uint32 numElements, BufferCreationFlags creationFlags, void* data)
	{
		if (elementSize == 0 || numElements == 0)
			return nullptr;

		BufferDesc desc;
		desc.elementSize = elementSize;
		desc.numElements = numElements;
		desc.creationFlags = creationFlags;
		RHI_TRACE_CODE(EXECUTE_RHI_FUNC(RHICreateBuffer(desc, data)));
	}

	RHIBuffer* RHI_TRACE_FUNC(RHICreateBuffer, BufferDesc const& desc, void* data)
	{
		if (desc.elementSize == 0 || desc.numElements == 0)
			return nullptr;

		RHI_TRACE_CODE(EXECUTE_RHI_FUNC(RHICreateBuffer(desc, data)));
	}

	RHIBuffer* RHI_TRACE_FUNC(RHICreateVertexBuffer, uint32 vertexSize, uint32 numVertices, BufferCreationFlags creationFlags, void* data)
	{
		if (vertexSize == 0 || numVertices == 0)
			return nullptr;

		BufferDesc desc;
		desc.elementSize = vertexSize;
		desc.numElements = numVertices;
		desc.creationFlags = creationFlags | BCF_UsageVertex;
		RHI_TRACE_CODE( EXECUTE_RHI_FUNC(RHICreateBuffer(desc, data) ) );
	}

	RHIBuffer* RHI_TRACE_FUNC(RHICreateIndexBuffer, uint32 nIndices, bool bIntIndex, BufferCreationFlags creationFlags, void* data)
	{
		if (nIndices == 0)
			return nullptr;

		BufferDesc desc;
		desc.elementSize = bIntIndex ? sizeof(uint32) : sizeof(uint16);
		desc.numElements = nIndices;
		desc.creationFlags = creationFlags | BCF_UsageIndex;
		RHI_TRACE_CODE( EXECUTE_RHI_FUNC(RHICreateBuffer(desc, data)) );
	}

	void* RHILockBuffer(RHIBuffer* buffer, ELockAccess access, uint32 offset, uint32 size)
	{
		return EXECUTE_RHI_FUNC(RHILockBuffer(buffer, access, offset, size));
	}

	void RHIUnlockBuffer(RHIBuffer* buffer)
	{
		return EXECUTE_RHI_FUNC(RHIUnlockBuffer(buffer));
	}

	void RHIReadTexture(RHITexture2D& texture, ETexture::Format format, int level, TArray< uint8 >& outData)
	{
		EXECUTE_RHI_FUNC(RHIReadTexture(texture, format, level, outData));
	}

	void RHIReadTexture(RHITextureCube& texture, ETexture::Format format, int level, TArray< uint8 >& outData)
	{
		EXECUTE_RHI_FUNC(RHIReadTexture(texture, format, level, outData));
	}

	bool  RHIUpdateTexture(RHITexture2D& texture, int ox, int oy, int w, int h, void* data, int level, int dataWidth)
	{
		return EXECUTE_RHI_FUNC(RHIUpdateTexture(texture, ox, oy, w, h, data, level, dataWidth));
	}

	//void* RHILockTexture(RHITextureBase* texture, ELockAccess access, uint32 offset /*= 0*/, uint32 size /*= 0*/)
	//{
	//	return EXECUTE_RHI_FUNC(RHILockTexture(texture, access, offset, size));
	//}

	//void RHIUnlockTexture(RHITextureBase* texture)
	//{
	//	return EXECUTE_RHI_FUNC(RHIUnlockTexture(texture));
	//}

	RHIFrameBuffer* RHICreateFrameBuffer()
	{
		return EXECUTE_RHI_FUNC( RHICreateFrameBuffer() );
	}

	RHIInputLayout* RHI_TRACE_FUNC(RHICreateInputLayout, InputLayoutDesc const& desc)
	{
		RHI_TRACE_CODE(EXECUTE_RHI_FUNC(RHICreateInputLayout(desc)));
	}

	RHIRasterizerState* RHI_TRACE_FUNC(RHICreateRasterizerState, RasterizerStateInitializer const& initializer)
	{
		RHI_TRACE_CODE(EXECUTE_RHI_FUNC(RHICreateRasterizerState(initializer)));
	}

	RHISamplerState* RHI_TRACE_FUNC(RHICreateSamplerState, SamplerStateInitializer const& initializer)
	{
		RHI_TRACE_CODE(EXECUTE_RHI_FUNC(RHICreateSamplerState(initializer)));
	}

	RHIDepthStencilState* RHI_TRACE_FUNC(RHICreateDepthStencilState, DepthStencilStateInitializer const& initializer)
	{
		RHI_TRACE_CODE(EXECUTE_RHI_FUNC(RHICreateDepthStencilState(initializer)));
	}

	RHIBlendState* RHI_TRACE_FUNC(RHICreateBlendState, BlendStateInitializer const& initializer)
	{
		RHI_TRACE_CODE(EXECUTE_RHI_FUNC(RHICreateBlendState(initializer)));
	}

	RHIShader* RHICreateShader(EShader::Type type)
	{
		return EXECUTE_RHI_FUNC( RHICreateShader(type) );
	}

	RHIShaderProgram* RHI_TRACE_FUNC(RHICreateShaderProgram)
	{
		RHI_TRACE_CODE(EXECUTE_RHI_FUNC(RHICreateShaderProgram()));
	}

	RHIRasterizerState& GetStaticRasterizerState(ECullMode cullMode, EFillMode fillMode)
	{
#define SWITCH_CULL_MODE( FILL_MODE )\
		switch( cullMode )\
		{\
		case ECullMode::Front: return TStaticRasterizerState< ECullMode::Front , FILL_MODE >::GetRHI();\
		case ECullMode::Back: return TStaticRasterizerState< ECullMode::Back , FILL_MODE >::GetRHI();\
		case ECullMode::None: \
		default:\
			return TStaticRasterizerState< ECullMode::None , FILL_MODE >::GetRHI();\
		}

		switch( fillMode )
		{
		case EFillMode::Point:
			SWITCH_CULL_MODE(EFillMode::Point);
			break;
		case EFillMode::Wireframe:
			SWITCH_CULL_MODE(EFillMode::Wireframe);
			break;
		}

		SWITCH_CULL_MODE(EFillMode::Solid);

#undef SWITCH_CULL_MODE
	}

	RHICommandList& RHICommandList::GetImmediateList()
	{
		return GRHISystem->getImmediateCommandList();
	}


#endif //CORE_SHARE_CODE

#if RHI_USE_RESOURCE_TRACE
#include "RHITraceScope.h"
#endif
}


