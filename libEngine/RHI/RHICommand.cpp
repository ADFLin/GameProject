#include "RHICommand.h"

#include "RHICommon.h"

#include "OpenGLCommand.h"
#include "D3D11Command.h"

#include "ShaderManager.h"

#include <cassert>
#include "stb/stb_image.h"

namespace Render
{
#if CORE_SHARE_CODE
	RHISystem* gRHISystem = nullptr;
#endif
	float gRHIClipZMin = 0;
	float gRHIProjectYSign = 1;
#define EXECUTE_RHI_FUNC( CODE ) gRHISystem->CODE

	bool RHISystemInitialize(RHISytemName name, RHISystemInitParam const& initParam)
	{
		if( gRHISystem == nullptr )
		{
			switch( name )
			{
			case RHISytemName::D3D11: gRHISystem = new D3D11System; break;
			case RHISytemName::Opengl:gRHISystem = new OpenGLSystem; break;
			}

			if( gRHISystem && !gRHISystem->initialize(initParam) )
			{
				delete gRHISystem;
				gRHISystem = nullptr;
			}

			if( gRHISystem )
			{
				ShaderFormat* shaderFormat = gRHISystem->createShaderFormat();
				ShaderManager::Get().mShaderFormat = shaderFormat;

				//#FIXME
				if( gRHISystem->getName() != RHISytemName::D3D11 )
					InitGlobalRHIResource();
			}		
		}
		
		return gRHISystem != nullptr;
	}


	void RHISystemShutdown()
	{	
		ShaderManager::Get().clearnupRHIResouse();

		//#FIXME
		if( gRHISystem->getName() != RHISytemName::D3D11 )
			ReleaseGlobalRHIResource();

		gRHISystem->shutdown();
		gRHISystem = nullptr;


		if( 1 )
		{
			::LogMsg("%d RHI Resource not release" , RHIResource::TotalCount );
		}
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

	RHITexture2D* RHICreateTexture2D(Texture::Format format, int w, int h, int numMipLevel, int numSamples, uint32 creationFlags, void* data , int dataAlign)
	{
		return EXECUTE_RHI_FUNC( RHICreateTexture2D(format, w, h, numMipLevel, numSamples, creationFlags , data, dataAlign) );
	}

	RHITexture3D* RHICreateTexture3D(Texture::Format format, int sizeX ,int sizeY , int sizeZ , int numMipLevel, int numSamples, uint32 creationFlags , void* data)
	{
		return EXECUTE_RHI_FUNC( RHICreateTexture3D(format, sizeX, sizeY, sizeZ, numMipLevel , numSamples, creationFlags, data) );
	}

	RHITextureCube* RHICreateTextureCube(Texture::Format format, int size, int numMipLevel, uint32 creationFlags, void* data[])
	{
		return EXECUTE_RHI_FUNC(RHICreateTextureCube(format, size, numMipLevel, creationFlags, data));
	}

	RHITexture2DArray* RHICreateTexture2DArray(Texture::Format format, int w, int h, int layerSize, int numMipLevel, int numSamples, uint32 creationFlags, void* data)
	{
		return EXECUTE_RHI_FUNC(RHICreateTexture2DArray(format, w, h, layerSize, numMipLevel, numSamples , creationFlags, data) );
	}

	RHITextureDepth* RHICreateTextureDepth(Texture::DepthFormat format, int w, int h, int numMipLevel, int numSamples)
	{
		return EXECUTE_RHI_FUNC( RHICreateTextureDepth(format, w, h, numMipLevel, numSamples) );
	}

	RHIVertexBuffer* RHICreateVertexBuffer(uint32 vertexSize, uint32 numVertices, uint32 creationFlags, void* data)
	{
		return EXECUTE_RHI_FUNC( RHICreateVertexBuffer(vertexSize, numVertices, creationFlags, data) );
	}

	RHIIndexBuffer* RHICreateIndexBuffer(uint32 nIndices, bool bIntIndex, uint32 creationFlags, void* data)
	{
		return EXECUTE_RHI_FUNC( RHICreateIndexBuffer(nIndices, bIntIndex, creationFlags, data) );
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

	RHIInputLayout* RHICreateInputLayout(InputLayoutDesc const& desc)
	{
		return EXECUTE_RHI_FUNC(RHICreateInputLayout(desc));
	}

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

	RHIShader* RHICreateShader(Shader::Type type)
	{
		return EXECUTE_RHI_FUNC( RHICreateShader(type) );
	}

	RHIShaderProgram* RHICreateShaderProgram()
	{
		return EXECUTE_RHI_FUNC( RHICreateShaderProgram() );
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

	struct STBImageData
	{
		STBImageData()
		{
			data = nullptr;
		}

		~STBImageData()
		{
			if( data )
			{
				stbi_image_free(data);
			}
		}
		int   width;
		int   height;
		int   numComponent;
		void* data;

		bool load(char const* path, TextureLoadOption const& option)
		{
			stbi_set_flip_vertically_on_load(option.bReverseH);
			if( option.bHDR )
			{
				data = stbi_loadf(path, &width, &height, &numComponent, STBI_default);
			}
			else
			{
				data = stbi_load(path, &width, &height, &numComponent, STBI_default);
			}
			stbi_set_flip_vertically_on_load(false);
			return data != nullptr;
		}
	};

	RHITexture2D* RHIUtility::LoadTexture2DFromFile(char const* path , TextureLoadOption const& option )
	{
		STBImageData imageData;
		if( !imageData.load(path, option) )
			return false;

		return RHICreateTexture2D(option.getFormat(imageData.numComponent), imageData.width, imageData.height, option.numMipLevel, 1 , option.creationFlags, imageData.data, 1);
	}

	RHITextureCube* RHIUtility::LoadTextureCubeFromFile(char const* paths[], TextureLoadOption const& option)
	{
		STBImageData imageDatas[Texture::FaceCount];

		void* data[Texture::FaceCount];
		for( int i = 0; i < Texture::FaceCount; ++i )
		{
			if( !imageDatas[i].load(paths[i], option) )
				return false;

			data[i] = imageDatas[i].data;
		}

		return RHICreateTextureCube(option.getFormat(imageDatas[0].numComponent), imageDatas[0].width, option.numMipLevel, option.creationFlags, data);
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

	RHICommandList& RHICommandList::GetImmediateList()
	{
		return gRHISystem->getImmediateCommandList();
	}

}
