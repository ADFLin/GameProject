#include "RHICommand.h"

#include "RHICommon.h"
#include "RHIGlobalResource.h"

#include "ShaderManager.h"
#include "GpuProfiler.h"

#include <cassert>
#include "DataCacheInterface.h"
#include "Serialize/DataStream.h"
#include "Image/ImageData.h"


#if USE_RHI_RESOURCE_TRACE
#include "RHITraceScope.h"
#endif
#include "Core/HalfFlot.h"

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

#define EXECUTE_RHI_FUNC( CODE ) GRHISystem->CODE

#define RHI_USE_FACTORY_LIST 1

#if RHI_USE_FACTORY_LIST
	struct RHIFactoryInfo
	{
		RHISystemName name;
		RHISystemFactory* factory;
	};
	std::vector< RHIFactoryInfo > GRHISystemFactoryList;

	RHISystemFactory* RHIGetSystemFactory(RHISystemName name)
	{
		int index = FindIndexPred(GRHISystemFactoryList,
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
		RemovePred(GRHISystemFactoryList, [name](auto const& info)
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

	std::vector< RHISystemEvent > GInitEventList;
	std::vector< RHISystemEvent > GShutdownEventList;
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

		char const* cmdLine = GetCommandLineA();
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

	void RHIPreSystemShutdown()
	{
		GRHISystem->preShutdown();
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

		//#FIXME
		if( GRHISystem->getName() != RHISystemName::Vulkan ||
			GRHISystem->getName() != RHISystemName::D3D12 )
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

	RHITexture1D* RHI_TRACE_FUNC(RHICreateTexture1D, ETexture::Format format, int length, int numMipLevel, uint32 creationFlags, void* data)
	{
		TextureDesc desc = TextureDesc::Type1D(format, length).MipLevel(numMipLevel).Flags(creationFlags);
		RHI_TRACE_CODE( EXECUTE_RHI_FUNC( RHICreateTexture1D(desc, data) ) );
	}

	RHITexture1D* RHI_TRACE_FUNC(RHICreateTexture1D, TextureDesc const& desc, void* data)
	{
		RHI_TRACE_CODE(EXECUTE_RHI_FUNC(RHICreateTexture1D(desc, data)));
	}

	RHITexture2D* RHI_TRACE_FUNC(RHICreateTexture2D , ETexture::Format format, int w, int h, int numMipLevel, int numSamples, uint32 creationFlags, void* data , int dataAlign)
	{
		TextureDesc desc = TextureDesc::Type2D(format, w, h).MipLevel(numMipLevel).Samples(numSamples).Flags(creationFlags);
		RHI_TRACE_CODE( EXECUTE_RHI_FUNC( RHICreateTexture2D(desc, data, dataAlign) ) );
	}
	RHITexture2D* RHI_TRACE_FUNC(RHICreateTexture2D, TextureDesc const& desc, void* data, int dataAlign)
	{
		RHI_TRACE_CODE(EXECUTE_RHI_FUNC(RHICreateTexture2D(desc, data, dataAlign)));
	}

	RHITexture3D* RHI_TRACE_FUNC(RHICreateTexture3D ,ETexture::Format format, int sizeX, int sizeY, int sizeZ, int numMipLevel, int numSamples, uint32 creationFlags, void* data)
	{
		TextureDesc desc = TextureDesc::Type3D(format, sizeX, sizeY, sizeZ).MipLevel(numMipLevel).Samples(numSamples).Flags(creationFlags);
		RHI_TRACE_CODE( EXECUTE_RHI_FUNC(RHICreateTexture3D(desc, data)) );
	}
	RHITexture3D* RHI_TRACE_FUNC(RHICreateTexture3D, TextureDesc const& desc, void* data)
	{
		RHI_TRACE_CODE(EXECUTE_RHI_FUNC(RHICreateTexture3D(desc, data)));
	}

	RHITextureCube* RHI_TRACE_FUNC(RHICreateTextureCube, ETexture::Format format, int size, int numMipLevel, uint32 creationFlags, void* data[])
	{
		TextureDesc desc = TextureDesc::TypeCube(format, size).MipLevel(numMipLevel).Flags(creationFlags);
		RHI_TRACE_CODE( EXECUTE_RHI_FUNC(RHICreateTextureCube(desc, data)) );
	}
	RHITextureCube* RHI_TRACE_FUNC(RHICreateTextureCube, TextureDesc const& desc, void* data[])
	{
		RHI_TRACE_CODE(EXECUTE_RHI_FUNC(RHICreateTextureCube(desc, data)));
	}

	RHITexture2DArray* RHI_TRACE_FUNC(RHICreateTexture2DArray, ETexture::Format format, int w, int h, int layerSize, int numMipLevel, int numSamples, uint32 creationFlags, void* data)
	{
		TextureDesc desc = TextureDesc::Type2DArray(format, w, h, layerSize).MipLevel(numMipLevel).Samples(numSamples).Flags(creationFlags);
		RHI_TRACE_CODE( EXECUTE_RHI_FUNC(RHICreateTexture2DArray(desc, data)) );
	}
	RHI_API RHITexture2DArray* RHI_TRACE_FUNC(RHICreateTexture2DArray, TextureDesc const& desc, void* data)
	{
		RHI_TRACE_CODE(EXECUTE_RHI_FUNC(RHICreateTexture2DArray(desc, data)));
	}

	RHITexture2D* RHI_TRACE_FUNC(RHICreateTextureDepth, ETexture::Format format, int w, int h, int numMipLevel, int numSamples, uint32 creationFlags)
	{
		TextureDesc desc = TextureDesc::Type2D(format, w, h).MipLevel(numMipLevel).Samples(numSamples).Flags(creationFlags);
		RHI_TRACE_CODE( EXECUTE_RHI_FUNC(RHICreateTextureDepth(desc)) );
	}
	RHITexture2D* RHI_TRACE_FUNC(RHICreateTextureDepth, TextureDesc const& desc)
	{
		RHI_TRACE_CODE(EXECUTE_RHI_FUNC(RHICreateTextureDepth(desc)));
	}

	RHIBuffer* RHI_TRACE_FUNC(RHICreateBuffer, uint32 elementSize, uint32 numElements, uint32 creationFlags, void* data)
	{
		if (elementSize == 0 || numElements == 0)
			return nullptr;
		RHI_TRACE_CODE(EXECUTE_RHI_FUNC(RHICreateBuffer(elementSize, numElements, creationFlags, data)));
	}

	RHIBuffer* RHI_TRACE_FUNC(RHICreateVertexBuffer, uint32 vertexSize, uint32 numVertices, uint32 creationFlags, void* data)
	{
		if (vertexSize == 0 || numVertices == 0)
			return nullptr;
		RHI_TRACE_CODE( EXECUTE_RHI_FUNC(RHICreateBuffer(vertexSize, numVertices, creationFlags | BCF_UsageVertex, data) ) );
	}

	RHIBuffer* RHI_TRACE_FUNC(RHICreateIndexBuffer, uint32 nIndices, bool bIntIndex, uint32 creationFlags, void* data)
	{
		if (nIndices == 0)
			return nullptr;
		RHI_TRACE_CODE( EXECUTE_RHI_FUNC(RHICreateBuffer(bIntIndex ? sizeof(uint32) : sizeof(uint16), nIndices, creationFlags | BCF_UsageIndex, data)) );
	}

	void* RHILockBuffer(RHIBuffer* buffer, ELockAccess access, uint32 offset, uint32 size)
	{
		return EXECUTE_RHI_FUNC(RHILockBuffer(buffer, access, offset, size));
	}

	void RHIUnlockBuffer(RHIBuffer* buffer)
	{
		return EXECUTE_RHI_FUNC(RHIUnlockBuffer(buffer));
	}

	void RHIReadTexture(RHITexture2D& texture, ETexture::Format format, int level, std::vector< uint8 >& outData)
	{
		EXECUTE_RHI_FUNC(RHIReadTexture(texture, format, level, outData));
	}

	void RHIReadTexture(RHITextureCube& texture, ETexture::Format format, int level, std::vector< uint8 >& outData)
	{
		EXECUTE_RHI_FUNC(RHIReadTexture(texture, format, level, outData));
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

#if USE_RHI_RESOURCE_TRACE
#include "RHITraceScope.h"
#endif

	int CalcMipmapLevel(int size)
	{
		int result = 0;
		int value = 1;
		while (value <= size) { value *= 2; ++result; }
		return result;
	}

	ImageLoadOption ToImageLoadOption(TextureLoadOption const& option)
	{
		ImageLoadOption result;
		result.FlipV(option.bFlipV).HDR(option.bHDR).UpThreeComponentToFour(!option.isRGBTextureSupported());
		return result;
	}
	RHITexture2D* RHIUtility::LoadTexture2DFromFile(DataCacheInterface& dataCache, char const* path, TextureLoadOption const& option)
	{
		bool bConvToHalf = option.isConvertFloatToHalf();

		DataCacheKey cacheKey;
		cacheKey.typeName = "TEXTURE";
		cacheKey.version = "8AE15F61-E1CF-4639-B7D8-409CF17933F0";
		cacheKey.keySuffix.add(path, option.bHDR, option.bFlipV, option.bSRGB, option.creationFlags, option.numMipLevel, option.bAutoMipMap,
			option.isRGBTextureSupported(), bConvToHalf);

		void* pData;
		ImageData imageData;
		std::vector< uint8 > cachedImageData;
		auto LoadCache = [&imageData, &cachedImageData](IStreamSerializer& serializer)-> bool
		{
			serializer >> imageData.width;
			serializer >> imageData.height;
			serializer >> imageData.numComponent;

			int dataSize;
			serializer >> dataSize;

			cachedImageData.resize(dataSize);
			serializer.read(cachedImageData.data(), dataSize);
			return true;
		};

		if( !dataCache.loadDelegate(cacheKey, LoadCache) )
		{
			if( !imageData.load(path, ToImageLoadOption(option)) )
				return false;

			pData = imageData.data;

			int dataSize = imageData.dataSize;
			
			if (bConvToHalf)
			{
				int num = imageData.width * imageData.height * imageData.numComponent;
				cachedImageData.resize(sizeof(HalfFloat) * num);
				
				HalfFloat* pHalf = (HalfFloat*)cachedImageData.data();
				float* pFloat = (float*)pData;
				for (int i = 0; i < num; ++i)
				{
					*pHalf = *pFloat;
					++pHalf;
					++pFloat;
				}
				pData = cachedImageData.data();
				dataSize /= 2;
			}

			dataCache.saveDelegate(cacheKey, [&imageData, pData , dataSize](IStreamSerializer& serializer)-> bool
			{
				serializer << imageData.width;
				serializer << imageData.height;
				serializer << imageData.numComponent;
				serializer << dataSize;
				serializer.write(pData, dataSize);
				return true;
			});
		}
		else
		{
			pData = cachedImageData.data();
		}

		uint32 flags = option.creationFlags;
		if (bConvToHalf)
		{
			flags |= TCF_HalfData;
		}


		auto format = option.getFormat(imageData.numComponent);

		int numMipLevel = option.numMipLevel;
		if (option.bAutoMipMap)
		{
			numMipLevel = CalcMipmapLevel( Math::Max(imageData.width , imageData.height) );
		}

		if (numMipLevel > 1)
		{
			flags |= TCF_GenerateMips;
		}


		return RHICreateTexture2D(format, imageData.width, imageData.height, numMipLevel, 1, flags, pData, 1);
	}

	RHITexture2D* RHIUtility::LoadTexture2DFromFile(char const* path , TextureLoadOption const& option )
	{
		ImageData imageData;
		if( !imageData.load(path, ToImageLoadOption(option)) )
			return false;

		return CreateTexture2D(imageData, option);
	}

	RHITextureCube* RHIUtility::LoadTextureCubeFromFile(char const* paths[], TextureLoadOption const& option)
	{
		bool bConvToHalf = option.isConvertFloatToHalf();
		ImageData imageDatas[ETexture::FaceCount];

		void* data[ETexture::FaceCount];
		for( int i = 0; i < ETexture::FaceCount; ++i )
		{
			if( !imageDatas[i].load(paths[i], ToImageLoadOption(option)) )
				return false;

			data[i] = imageDatas[i].data;
		}

		uint32 flags = option.creationFlags;
		if (option.bAutoMipMap)
		{
			option.numMipLevel;
		}

		int numMipLevel = option.numMipLevel;
		if (option.bAutoMipMap)
		{
			numMipLevel = CalcMipmapLevel(imageDatas[0].width);
		}
		if (numMipLevel > 1)
		{
			flags |= TCF_GenerateMips;
		}

		return RHICreateTextureCube(option.getFormat(imageDatas[0].numComponent), imageDatas[0].width, numMipLevel, flags, data);
	}

	RHITexture2D* RHIUtility::CreateTexture2D(ImageData const& imageData, TextureLoadOption const& option)
	{
		auto format = option.getFormat(imageData.numComponent);

		bool bConvToHalf = option.isConvertFloatToHalf();


		uint32 flags = option.creationFlags;

		int numMipLevel = option.numMipLevel;
		if (option.bAutoMipMap)
		{
			numMipLevel = CalcMipmapLevel(Math::Max(imageData.width, imageData.height));
		}

		if (option.numMipLevel > 1)
		{
			flags |= TCF_GenerateMips;
		}

		if (bConvToHalf)
		{
			std::vector< HalfFloat > halfData(imageData.width * imageData.height * imageData.numComponent);
			float* pFloat = (float*)imageData.data;
			for (int i = 0; i < halfData.size(); ++i)
			{
				halfData[i] = *pFloat;
				++pFloat;
			}
			return RHICreateTexture2D(format, imageData.width, imageData.height, numMipLevel, 1, flags |TCF_HalfData, halfData.data(), 1);

		}
		else
		{
			return RHICreateTexture2D(format, imageData.width, imageData.height, numMipLevel, 1, flags, imageData.data, 1);
		}
	}

	ETexture::Format TextureLoadOption::getFormat(int numComponent) const
	{
#define FORCE_USE_FLOAT_TYPE 0

		if( bHDR )
		{
			if ( bUseHalf )
			{
				switch (numComponent)
				{
				case 1: return ETexture::R16F;
				case 2: return ETexture::RG16F;
				case 3: return ETexture::RGB16F;
				case 4: return ETexture::RGBA16F;
				}
			}
			else
			{
				switch (numComponent)
				{
				case 1: return ETexture::R32F;
				case 2: return ETexture::RG32F;
				case 3: return ETexture::RGB32F;
				case 4: return ETexture::RGBA32F;
				}
			}

		}
		else
		{
			//#TODO
			switch( numComponent )
			{
			case 1: return ETexture::R8;
			case 3: return bSRGB ? ETexture::SRGB : ETexture::RGB8;
			case 4: return bSRGB ? ETexture::SRGBA : ETexture::RGBA8;
			}
		}
		return ETexture::R8;
	}


	bool TextureLoadOption::isRGBTextureSupported() const
	{
		auto systemName = GRHISystem->getName();
		if (systemName == RHISystemName::OpenGL ||
			systemName == RHISystemName::Vulkan)
			return true;

		if (systemName == RHISystemName::D3D11 ||
			systemName == RHISystemName::D3D12 )
		{
			if (bHDR)
			{
				//no rgb16f format and rgb32f only can use of point sampler in pixel shader
				return false;
			}
			if (bSRGB)
			{
				return false;
			}

			return false;
		}
		return true;
	}

	bool TextureLoadOption::isNeedConvertFloatToHalf() const
	{
		auto systemName = GRHISystem->getName();
		if (systemName == RHISystemName::D3D11 ||
			systemName == RHISystemName::D3D12 ||
			systemName == RHISystemName::Vulkan)
			return true;

		return false;
	}

}


