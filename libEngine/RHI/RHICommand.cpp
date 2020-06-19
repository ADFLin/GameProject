#include "RHICommand.h"

#include "RHICommon.h"

#include "OpenGLCommand.h"
#include "D3D11Command.h"
#include "VulkanCommand.h"

#include "ShaderManager.h"

#include <cassert>
#include "DataCacheInterface.h"
#include "Serialize/DataStream.h"
#include "Image/ImageData.h"

#if USE_RHI_RESOURCE_TRACE
#include "RHITraceScope.h"
#endif


namespace Render
{
#if CORE_SHARE_CODE
	RHISystem* gRHISystem = nullptr;
	float gRHIClipZMin = 0;
	float gRHIProjectYSign = 1;

#define EXECUTE_RHI_FUNC( CODE ) gRHISystem->CODE

	bool RHISystemInitialize(RHISytemName name, RHISystemInitParams const& initParam)
	{
		if( gRHISystem == nullptr )
		{
			switch( name )
			{
			case RHISytemName::D3D11: gRHISystem = new D3D11System; break;
			case RHISytemName::Opengl:gRHISystem = new OpenGLSystem; break;
			case RHISytemName::Vulkan:gRHISystem = new VulkanSystem; break;
			}

			if( gRHISystem && !gRHISystem->initialize(initParam) )
			{
				delete gRHISystem;
				gRHISystem = nullptr;
			}

			if( gRHISystem )
			{
				//#FIXME
				if (gRHISystem->getName() != RHISytemName::D3D12 )
				{
					ShaderFormat* shaderFormat = gRHISystem->createShaderFormat();
					if (shaderFormat == nullptr)
					{
						LogError("Can't create shader format for %d system", (int)gRHISystem->getName());
						return false;
					}

					if (!ShaderManager::Get().initialize(*shaderFormat))
					{
						LogError("ShaderManager can't initialize");
						return false;
					}
				}

				GlobalRHIResourceBase::ReleaseAllResource();

				//#FIXME
				if( gRHISystem->getName() != RHISytemName::D3D11 &&
					gRHISystem->getName() != RHISytemName::D3D12 &&
					gRHISystem->getName() != RHISytemName::Vulkan )
				{
					InitGlobalRHIResource();
				}
			}		
		}
		
		return gRHISystem != nullptr;
	}


	void RHISystemShutdown()
	{	
		ShaderManager::Get().clearnupRHIResouse();

		//#FIXME
		if( gRHISystem->getName() != RHISytemName::D3D11 ||
			gRHISystem->getName() != RHISytemName::D3D12 )
			ReleaseGlobalRHIResource();

		GlobalRHIResourceBase::ReleaseAllResource();

		gRHISystem->shutdown();
		delete gRHISystem;
		gRHISystem = nullptr;

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


	RHIRenderWindow* RHICreateRenderWindow(PlatformWindowInfo const& info)
	{
		return EXECUTE_RHI_FUNC( RHICreateRenderWindow(info) );
	}

	RHITexture1D* RHICreateTexture1D(Texture::Format format, int length, int numMipLevel, uint32 creationFlags, void* data)
	{
		return EXECUTE_RHI_FUNC( RHICreateTexture1D(format, length, numMipLevel, creationFlags , data) );
	}

	RHITexture2D* RHI_TRACE_FUNC(RHICreateTexture2D , Texture::Format format, int w, int h, int numMipLevel, int numSamples, uint32 creationFlags, void* data , int dataAlign)
	{
		RHI_TRACE_CODE( EXECUTE_RHI_FUNC( RHICreateTexture2D(format, w, h, numMipLevel, numSamples, creationFlags , data, dataAlign) ) );
	}

	RHITexture3D* RHI_TRACE_FUNC(RHICreateTexture3D ,Texture::Format format, int sizeX, int sizeY, int sizeZ, int numMipLevel, int numSamples, uint32 creationFlags, void* data)
	{
		RHI_TRACE_CODE( EXECUTE_RHI_FUNC(RHICreateTexture3D(format, sizeX, sizeY, sizeZ, numMipLevel, numSamples, creationFlags, data)) );
	}

	RHITextureCube* RHI_TRACE_FUNC(RHICreateTextureCube, Texture::Format format, int size, int numMipLevel, uint32 creationFlags, void* data[])
	{
		RHI_TRACE_CODE( EXECUTE_RHI_FUNC(RHICreateTextureCube(format, size, numMipLevel, creationFlags, data)) );
	}

	RHITexture2DArray* RHI_TRACE_FUNC(RHICreateTexture2DArray, Texture::Format format, int w, int h, int layerSize, int numMipLevel, int numSamples, uint32 creationFlags, void* data)
	{
		RHI_TRACE_CODE( EXECUTE_RHI_FUNC(RHICreateTexture2DArray(format, w, h, layerSize, numMipLevel, numSamples, creationFlags, data)) );
	}

	RHITextureDepth* RHI_TRACE_FUNC(RHICreateTextureDepth, Texture::DepthFormat format, int w, int h, int numMipLevel, int numSamples)
	{
		RHI_TRACE_CODE( EXECUTE_RHI_FUNC(RHICreateTextureDepth(format, w, h, numMipLevel, numSamples)) );
	}

	RHIVertexBuffer* RHI_TRACE_FUNC(RHICreateVertexBuffer, uint32 vertexSize, uint32 numVertices, uint32 creationFlags, void* data)
	{
		RHI_TRACE_CODE( EXECUTE_RHI_FUNC( RHICreateVertexBuffer(vertexSize, numVertices, creationFlags, data) ) );
	}

	RHIIndexBuffer* RHI_TRACE_FUNC(RHICreateIndexBuffer, uint32 nIndices, bool bIntIndex, uint32 creationFlags, void* data)
	{
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

	RHIFrameBuffer* RHICreateFrameBuffer()
	{
		return EXECUTE_RHI_FUNC( RHICreateFrameBuffer() );
	}

#if USE_RHI_RESOURCE_TRACE
	RHIInputLayout* RHICreateInputLayoutTrace(ResTraceInfo const& traceInfo, InputLayoutDesc const& desc)
	{
		auto result = EXECUTE_RHI_FUNC(RHICreateInputLayout(desc));
		if (result)
		{
			result->mTrace = traceInfo;
		}
		return result;
	}
#else
	RHIInputLayout* RHICreateInputLayout(InputLayoutDesc const& desc)
	{
		return EXECUTE_RHI_FUNC(RHICreateInputLayout(desc));
	}
#endif

	RHIRasterizerState* RHICreateRasterizerState(RasterizerStateInitializer const& initializer)
	{
		return EXECUTE_RHI_FUNC(RHICreateRasterizerState(initializer));
	}

	RHISamplerState* RHICreateSamplerState(SamplerStateInitializer const& initializer)
	{
		return EXECUTE_RHI_FUNC(RHICreateSamplerState(initializer));
	}

	RHIDepthStencilState* RHICreateDepthStencilState(DepthStencilStateInitializer const& initializer)
	{
		return EXECUTE_RHI_FUNC(RHICreateDepthStencilState(initializer));
	}

	RHIBlendState* RHICreateBlendState(BlendStateInitializer const& initializer)
	{
		return EXECUTE_RHI_FUNC(RHICreateBlendState(initializer));
	}

	RHIShader* RHICreateShader(EShader::Type type)
	{
		return EXECUTE_RHI_FUNC( RHICreateShader(type) );
	}

#if USE_RHI_RESOURCE_TRACE
	RHIShaderProgram* RHICreateShaderProgramTrace(ResTraceInfo const& traceInfo)
	{
		RHIShaderProgram* result = EXECUTE_RHI_FUNC(RHICreateShaderProgram());
		if (result)
		{
			result->mTrace = traceInfo;
		}
		return result;
	}
#else
	RHIShaderProgram* RHICreateShaderProgram()
	{
		return EXECUTE_RHI_FUNC(RHICreateShaderProgram());
	}
#endif


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
		return gRHISystem->getImmediateCommandList();
	}


#endif //CORE_SHARE_CODE

#if USE_RHI_RESOURCE_TRACE
#include "RHITraceScope.h"
#endif

	bool IsSupportRGB8ComponentTexture()
	{
		return gRHISystem->getName() != RHISytemName::Vulkan;
	}

	RHITexture2D* RHIUtility::LoadTexture2DFromFile(DataCacheInterface& dataCache, char const* path, TextureLoadOption const& option)
	{
		DataCacheKey cacheKey;
		cacheKey.typeName = "TEXTURE";
		cacheKey.version = "8AE15F61-E1DC-4639-B7D8-409CF17933F0";
		cacheKey.keySuffix.add(path, option.bHDR, option.bReverseH, option.bSRGB, option.creationFlags, option.numMipLevel);


		void* pData;
		ImageData imageData;
		std::vector< uint8 > cachedImageData;
		auto LoadCache = [&imageData, &cachedImageData](IStreamSerializer& serializer)-> bool
		{
			serializer >> imageData.width;
			serializer >> imageData.height;
			serializer >> imageData.numComponent;
			serializer >> imageData.dataSize;

			cachedImageData.resize(imageData.dataSize);
			serializer.read(cachedImageData.data(), imageData.dataSize);
			return true;
		};
		if( !dataCache.loadDelegate(cacheKey, LoadCache) )
		{
			if( !imageData.load(path, option.bHDR, option.bReverseH , option.bHDR ? false : !IsSupportRGB8ComponentTexture()) )
				return false;

			pData = imageData.data;
			dataCache.saveDelegate(cacheKey, [&imageData](IStreamSerializer& serializer)-> bool
			{
				serializer << imageData.width;
				serializer << imageData.height;
				serializer << imageData.numComponent;
				serializer << imageData.dataSize;
				serializer.write(imageData.data, imageData.dataSize);
				return true;
			});
		}
		else
		{

			pData = cachedImageData.data();
		}

		return RHICreateTexture2D(option.getFormat(imageData.numComponent), imageData.width, imageData.height, option.numMipLevel, 1, option.creationFlags, pData, 1);
	}

	RHITexture2D* RHIUtility::LoadTexture2DFromFile(char const* path , TextureLoadOption const& option )
	{
		ImageData imageData;
		if( !imageData.load(path, option.bHDR , option.bReverseH, option.bHDR ? false : !IsSupportRGB8ComponentTexture()) )
			return false;

		return RHICreateTexture2D(option.getFormat(imageData.numComponent), imageData.width, imageData.height, option.numMipLevel, 1 , option.creationFlags, imageData.data, 1);
	}

	RHITextureCube* RHIUtility::LoadTextureCubeFromFile(char const* paths[], TextureLoadOption const& option)
	{
		ImageData imageDatas[Texture::FaceCount];

		void* data[Texture::FaceCount];
		for( int i = 0; i < Texture::FaceCount; ++i )
		{
			if( !imageDatas[i].load(paths[i], option.bHDR, option.bReverseH, option.bHDR ? false : !IsSupportRGB8ComponentTexture()) )
				return false;

			data[i] = imageDatas[i].data;
		}

		return RHICreateTextureCube(option.getFormat(imageDatas[0].numComponent), imageDatas[0].width, option.numMipLevel, option.creationFlags, data);
	}

	RHITexture2D* RHIUtility::CreateTexture2D(ImageData const& imageData, TextureLoadOption const& option)
	{
		return RHICreateTexture2D(option.getFormat(imageData.numComponent), imageData.width, imageData.height, option.numMipLevel, 1, option.creationFlags, imageData.data, 1);
	}

	Texture::Format TextureLoadOption::getFormat(int numComponent) const
	{
		if( bHDR )
		{
			switch( numComponent )
			{
			case 1: return Texture::eR32F;
			case 3: return Texture::eRGB16F;
			case 4: return Texture::eRGBA16F;
			}
		}
		else
		{
			//#TODO
			switch( numComponent )
			{
			case 3: return bSRGB ? Texture::eSRGB : Texture::eRGB8;
			case 4: return bSRGB ? Texture::eSRGBA : Texture::eRGBA8;
			}
		}
		return Texture::eR8;
	}


}


