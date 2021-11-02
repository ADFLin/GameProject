#include "RHICommand.h"

#include "RHICommon.h"
#include "RHIGlobalResource.h"


//TODO: Remove
#include "OpenGLCommand.h"
#include "D3D11Command.h"
#include "D3D12Command.h"
#include "VulkanCommand.h"

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
	float GRHIProjectYSign = 1;
	float GRHIVericalFlip = 1;
	bool GRHISupportMeshShader = false;
	bool GRHISupportRayTracing = false;
	bool GRHIPrefEnabled = false;

#define EXECUTE_RHI_FUNC( CODE ) GRHISystem->CODE

	bool RHISystemInitialize(RHISystemName name, RHISystemInitParams const& initParam)
	{
		if( GRHISystem == nullptr )
		{
			switch( name )
			{
			case RHISystemName::D3D11: GRHISystem = new D3D11System; break;
			case RHISystemName::D3D12: GRHISystem = new D3D12System; break;
			case RHISystemName::OpenGL:GRHISystem = new OpenGLSystem; break;
			case RHISystemName::Vulkan:GRHISystem = new VulkanSystem; break;
			}

			LogMsg("===== Init RHI System : %s ====" , ToString(name) );

			GRHIDeviceVendorName = DeviceVendorName::Unknown;
			GRHISupportRayTracing = false;
			GRHISupportMeshShader = false;

			char const* cmdLine = GetCommandLineA();
			GRHIPrefEnabled = FCString::StrStr(cmdLine, "-RHIPerf");

			FRHIResourceTable::Initialize();

			if( GRHISystem && !GRHISystem->initialize(initParam) )
			{
				delete GRHISystem;
				GRHISystem = nullptr;
			}

			if( GRHISystem )
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


				GlobalRenderResourceBase::RestoreAllResource();

				InitGlobalRenderResource();
			}		
		}
		
		return GRHISystem != nullptr;
	}


	void RHISystemShutdown()
	{	
		GRHISystem->preShutdown();
		
		if (GpuProfiler::Get().mCore)
		{
			GpuProfiler::Get().mCore->releaseRHI();
			delete GpuProfiler::Get().mCore;
			GpuProfiler::Get().mCore = nullptr;
		}

		//#FIXME
		if( GRHISystem->getName() != RHISystemName::Vulkan ||
			GRHISystem->getName() != RHISystemName::D3D12 )
			ReleaseGlobalRenderResource();

		ShaderManager::Get().clearnupRHIResouse();
		GlobalRenderResourceBase::ReleaseAllResource();

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

	RHITexture1D* RHICreateTexture1D(ETexture::Format format, int length, int numMipLevel, uint32 creationFlags, void* data)
	{
		return EXECUTE_RHI_FUNC( RHICreateTexture1D(format, length, numMipLevel, creationFlags , data) );
	}

	RHITexture2D* RHI_TRACE_FUNC(RHICreateTexture2D , ETexture::Format format, int w, int h, int numMipLevel, int numSamples, uint32 creationFlags, void* data , int dataAlign)
	{
		RHI_TRACE_CODE( EXECUTE_RHI_FUNC( RHICreateTexture2D(format, w, h, numMipLevel, numSamples, creationFlags , data, dataAlign) ) );
	}

	RHITexture3D* RHI_TRACE_FUNC(RHICreateTexture3D ,ETexture::Format format, int sizeX, int sizeY, int sizeZ, int numMipLevel, int numSamples, uint32 creationFlags, void* data)
	{
		RHI_TRACE_CODE( EXECUTE_RHI_FUNC(RHICreateTexture3D(format, sizeX, sizeY, sizeZ, numMipLevel, numSamples, creationFlags, data)) );
	}

	RHITextureCube* RHI_TRACE_FUNC(RHICreateTextureCube, ETexture::Format format, int size, int numMipLevel, uint32 creationFlags, void* data[])
	{
		RHI_TRACE_CODE( EXECUTE_RHI_FUNC(RHICreateTextureCube(format, size, numMipLevel, creationFlags, data)) );
	}

	RHITexture2DArray* RHI_TRACE_FUNC(RHICreateTexture2DArray, ETexture::Format format, int w, int h, int layerSize, int numMipLevel, int numSamples, uint32 creationFlags, void* data)
	{
		RHI_TRACE_CODE( EXECUTE_RHI_FUNC(RHICreateTexture2DArray(format, w, h, layerSize, numMipLevel, numSamples, creationFlags, data)) );
	}

	RHITexture2D* RHI_TRACE_FUNC(RHICreateTextureDepth, ETexture::Format format, int w, int h, int numMipLevel, int numSamples, uint32 creationFlags)
	{
		RHI_TRACE_CODE( EXECUTE_RHI_FUNC(RHICreateTextureDepth(format, w, h, numMipLevel, numSamples, creationFlags)) );
	}

	RHIVertexBuffer* RHI_TRACE_FUNC(RHICreateVertexBuffer, uint32 vertexSize, uint32 numVertices, uint32 creationFlags, void* data)
	{
		if (numVertices == 0 || vertexSize == 0)
			return nullptr;
		RHI_TRACE_CODE( EXECUTE_RHI_FUNC( RHICreateVertexBuffer(vertexSize, numVertices, creationFlags, data) ) );
	}

	RHIIndexBuffer* RHI_TRACE_FUNC(RHICreateIndexBuffer, uint32 nIndices, bool bIntIndex, uint32 creationFlags, void* data)
	{
		if (nIndices == 0)
			return nullptr;
		RHI_TRACE_CODE( EXECUTE_RHI_FUNC(RHICreateIndexBuffer(nIndices, bIntIndex, creationFlags, data)) );
	}

	void* RHILockBuffer(RHIVertexBuffer* buffer, ELockAccess access, uint32 offset, uint32 size)
	{
		return EXECUTE_RHI_FUNC(RHILockBuffer(buffer, access , offset, size));
	}

	void RHIUnlockBuffer(RHIVertexBuffer* buffer)
	{
		return EXECUTE_RHI_FUNC(RHIUnlockBuffer(buffer));
	}

	void* RHILockBuffer(RHIIndexBuffer* buffer, ELockAccess access, uint32 offset, uint32 size)
	{
		return EXECUTE_RHI_FUNC(RHILockBuffer(buffer, access, offset, size));
	}

	void RHIUnlockBuffer(RHIIndexBuffer* buffer)
	{
		return EXECUTE_RHI_FUNC(RHIUnlockBuffer(buffer));
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
			if( !imageData.load(path, option.bHDR, option.bFlipV, !option.isRGBTextureSupported()) )
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
		if( !imageData.load(path, option.bHDR , option.bFlipV, !option.isRGBTextureSupported()) )
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
			if( !imageDatas[i].load(paths[i], option.bHDR, option.bFlipV, !option.isRGBTextureSupported()) )
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
		if (GRHISystem->getName() == RHISystemName::OpenGL ||
			GRHISystem->getName() == RHISystemName::Vulkan)
			return true;

		if ( GRHISystem->getName() == RHISystemName::D3D11 ||
			 GRHISystem->getName() == RHISystemName::D3D12 )
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
		if (GRHISystem->getName() == RHISystemName::D3D11 ||
			GRHISystem->getName() == RHISystemName::D3D12 ||
			GRHISystem->getName() == RHISystemName::Vulkan)
			return true;

		return false;
	}

}


