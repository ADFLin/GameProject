#include "RHICommand.h"

#include "RHICommon.h"

#include "OpenGLCommand.h"
#include "D3D11Command.h"

#include "ShaderCompiler.h"

#include <cassert>
#include "stb/stb_image.h"

namespace Render
{
#if CORE_SHARE_CODE
	RHISystem* gRHISystem = nullptr;
#endif


#define EXECUTE_RHIFUN( CODE ) gRHISystem->CODE

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
				InitGlobalRHIResource();
			}		
		}
		
		return gRHISystem != nullptr;
	}


	void RHISystemShutdown()
	{	
		ShaderManager::Get().clearnupRHIResouse();

		ReleaseGlobalRHIResource();

		gRHISystem->shutdown();
		gRHISystem = nullptr;
	}


	bool RHIBeginRender()
	{
		return EXECUTE_RHIFUN( RHIBeginRender() );
	}

	void RHIEndRender(bool bPresent)
	{
		EXECUTE_RHIFUN( RHIEndRender(bPresent) );
	}

	RHIRenderWindow* RHICreateRenderWindow(PlatformWindowInfo const& info)
	{
		return EXECUTE_RHIFUN( RHICreateRenderWindow(info) );
	}

	RHITexture1D* RHICreateTexture1D(Texture::Format format, int length, int numMipLevel, uint32 creationFlags, void* data)
	{
		return EXECUTE_RHIFUN( RHICreateTexture1D(format, length, numMipLevel, creationFlags , data) );
	}

	RHITexture2D* RHICreateTexture2D(Texture::Format format, int w, int h, int numMipLevel, uint32 creationFlags, void* data , int dataAlign)
	{
		return EXECUTE_RHIFUN( RHICreateTexture2D(format, w, h, numMipLevel, creationFlags , data, dataAlign) );
	}

	RHITexture3D* RHICreateTexture3D(Texture::Format format, int sizeX ,int sizeY , int sizeZ , int numMipLevel, uint32 creationFlags , void* data)
	{
		return EXECUTE_RHIFUN( RHICreateTexture3D(format, sizeX, sizeY, sizeZ, numMipLevel ,creationFlags, data) );
	}

	RHITextureCube* RHICreateTextureCube(Texture::Format format, int size, int numMipLevel, uint32 creationFlags, void* data[])
	{
		return EXECUTE_RHIFUN(RHICreateTextureCube(format, size, numMipLevel, creationFlags, data));
	}

	RHITextureDepth* RHICreateTextureDepth(Texture::DepthFormat format, int w, int h)
	{
		return EXECUTE_RHIFUN( RHICreateTextureDepth(format, w, h) );
	}

	RHIVertexBuffer* RHICreateVertexBuffer(uint32 vertexSize, uint32 numVertices, uint32 creationFlags, void* data)
	{
		return EXECUTE_RHIFUN( RHICreateVertexBuffer(vertexSize, numVertices, creationFlags, data) );
	}

	RHIIndexBuffer* RHICreateIndexBuffer(uint32 nIndices, bool bIntIndex, uint32 creationFlags, void* data)
	{
		return EXECUTE_RHIFUN( RHICreateIndexBuffer(nIndices, bIntIndex, creationFlags, data) );
	}

	RHIUniformBuffer* RHICreateUniformBuffer(uint32 elementSize, uint32 numElement, uint32 creationFlags, void* data)
	{
		return EXECUTE_RHIFUN( RHICreateUniformBuffer(elementSize , numElement, creationFlags, data) );
	}

	void* RHILockBuffer(RHIVertexBuffer* buffer, ELockAccess access, uint32 offset, uint32 size)
	{
		return EXECUTE_RHIFUN(RHILockBuffer(buffer, access , offset, size));
	}

	void RHIUnlockBuffer(RHIVertexBuffer* buffer)
	{
		return EXECUTE_RHIFUN(RHIUnlockBuffer(buffer));
	}

	void* RHILockBuffer(RHIIndexBuffer* buffer, ELockAccess access, uint32 offset, uint32 size)
	{
		return EXECUTE_RHIFUN(RHILockBuffer(buffer, access, offset, size));
	}

	void RHIUnlockBuffer(RHIIndexBuffer* buffer)
	{
		return EXECUTE_RHIFUN(RHIUnlockBuffer(buffer));
	}

	void* RHILockBuffer(RHIUniformBuffer* buffer, ELockAccess access, uint32 offset /*= 0*/, uint32 size /*= 0*/)
	{
		return EXECUTE_RHIFUN(RHILockBuffer(buffer, access, offset, size));
	}

	void RHIUnlockBuffer(RHIUniformBuffer* buffer)
	{
		return EXECUTE_RHIFUN(RHIUnlockBuffer(buffer));
	}

	RHIFrameBuffer* RHICreateFrameBuffer()
	{
		return EXECUTE_RHIFUN( RHICreateFrameBuffer() );
	}

	RHIInputLayout* RHICreateInputLayout(InputLayoutDesc const& desc)
	{
		return EXECUTE_RHIFUN(RHICreateInputLayout(desc));
	}

	RHIRasterizerState* RHICreateRasterizerState(RasterizerStateInitializer const& initializer)
	{
		return EXECUTE_RHIFUN(RHICreateRasterizerState(initializer));
	}

	RHISamplerState* RHICreateSamplerState(SamplerStateInitializer const& initializer)
	{
		return EXECUTE_RHIFUN(RHICreateSamplerState(initializer));
	}

	RHIDepthStencilState* RHICreateDepthStencilState(DepthStencilStateInitializer const& initializer)
	{
		return EXECUTE_RHIFUN(RHICreateDepthStencilState(initializer));
	}

	RHIBlendState* RHICreateBlendState(BlendStateInitializer const& initializer)
	{
		return EXECUTE_RHIFUN(RHICreateBlendState(initializer));
	}

	void RHISetRasterizerState(RHIRasterizerState& rasterizerState)
	{
		EXECUTE_RHIFUN(RHISetRasterizerState(rasterizerState));
	}
	void RHISetBlendState(RHIBlendState& blendState)
	{
		EXECUTE_RHIFUN(RHISetBlendState(blendState));
	}
	void RHISetDepthStencilState(RHIDepthStencilState& depthStencilState, uint32 stencilRef)
	{
		EXECUTE_RHIFUN(RHISetDepthStencilState(depthStencilState, stencilRef));
	}

	void RHISetViewport(int x, int y, int w, int h)
	{
		EXECUTE_RHIFUN( RHISetViewport(x, y, w, h) );
	}

	void RHISetScissorRect(bool bEnable, int x, int y, int w, int h)
	{
		EXECUTE_RHIFUN( RHISetScissorRect(bEnable, x, y, w, h));
	}

	void RHIDrawPrimitive(PrimitiveType type , int start, int nv)
	{
		EXECUTE_RHIFUN(RHIDrawPrimitive(type, start, nv));
	}

	void RHIDrawIndexedPrimitive(PrimitiveType type, ECompValueType indexType , int indexStart, int nIndex)
	{
		EXECUTE_RHIFUN(RHIDrawIndexedPrimitive(type, indexType, indexStart, nIndex));
	}

	void RHIDrawPrimitiveIndirect(PrimitiveType type, RHIVertexBuffer* commandBuffer, int offset, int numCommand, int commandStride)
	{
		EXECUTE_RHIFUN(RHIDrawPrimitiveIndirect(type, commandBuffer, offset, numCommand, commandStride));
	}

	void RHIDrawIndexedPrimitiveIndirect(PrimitiveType type, ECompValueType indexType, RHIVertexBuffer* commandBuffer, int offset, int numCommand, int commandStride)
	{
		EXECUTE_RHIFUN(RHIDrawIndexedPrimitiveIndirect(type, indexType, commandBuffer, offset , numCommand, commandStride));
	}

	void RHIDrawPrimitiveInstanced(PrimitiveType type, int vStart, int nv, int numInstance)
	{
		EXECUTE_RHIFUN(RHIDrawPrimitiveInstanced(type, vStart, nv, numInstance));
	}

	void RHIDrawPrimitiveUP(PrimitiveType type, int numPrimitive, void* pVertices, int numVerex, int vetexStride)
	{
		EXECUTE_RHIFUN(RHIDrawPrimitiveUP(type, numPrimitive, pVertices, numVerex, vetexStride));
	}

	void RHIDrawIndexedPrimitiveUP(PrimitiveType type, int numPrimitive, void* pVertices, int numVerex, int vetexStride, int* pIndices, int numIndex)
	{
		EXECUTE_RHIFUN(RHIDrawIndexedPrimitiveUP(type, numPrimitive, pVertices, numVerex, vetexStride, pIndices, numIndex));
	}

	void RHISetupFixedPipelineState(Matrix4 const& matModelView, Matrix4 const& matProj, int numTexture /*= 0*/, RHITexture2D** textures /*= nullptr*/)
	{
		EXECUTE_RHIFUN(RHISetupFixedPipelineState(matModelView, matProj, numTexture, textures));
	}

	void RHISetFrameBuffer(RHIFrameBuffer& frameBuffer, RHITextureDepth* overrideDepthTexture /*= nullptr*/)
	{
		EXECUTE_RHIFUN(RHISetFrameBuffer(frameBuffer, overrideDepthTexture));
	}

	void RHISetIndexBuffer(RHIIndexBuffer* indexBuffer)
	{
		EXECUTE_RHIFUN(RHISetIndexBuffer(indexBuffer));
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

		return RHICreateTexture2D(option.getFormat(imageData.numComponent), imageData.width, imageData.height, option.numMipLevel, option.creationFlags, imageData.data, 1);
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

	Render::Texture::Format TextureLoadOption::getFormat(int numComponent) const
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
