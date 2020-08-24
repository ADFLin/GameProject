#pragma once

#ifndef BasePassRendering_H_2CFE6D0C_5DFA_446A_B560_66C63B1ADBBF
#define BasePassRendering_H_2CFE6D0C_5DFA_446A_B560_66C63B1ADBBF
#include "RHI/MaterialShader.h"

namespace Render
{

	namespace EGBufferId
	{
		enum Type
		{
			A, //xyz : Noraml
			B, //xyz : BaseColor
			C,
			D,

			Count,
		};
	};
	struct GBufferResource
	{
		RHITexture2DRef textures[EGBufferId::Count];
		RHITextureDepthRef depthTexture;

		bool initializeRHI(IntVector2 const& size, int numSamples);
		void releaseRHI()
		{
			for (auto& texture : textures)
			{
				texture.release();
			}
			depthTexture.release();
		}

		void drawTextures(RHICommandList& commandList, Matrix4 const& XForm, IntVector2 const& size, IntVector2 const& gapSize);
		void drawTexture(RHICommandList& commandList, Matrix4 const& XForm, int x, int y, int width, int height, int idxBuffer);
		void drawTexture(RHICommandList& commandList, Matrix4 const& XForm, int x, int y, int width, int height, int idxBuffer, Vector4 const& colorMask);
	};

	struct RenderTargetResource
	{
		RHITexture2DRef renderTargetTexture;
		RHITexture2DRef resolvedTexture;
	};

	class FrameRenderTargets
	{
	public:

		bool prepare(IntVector2 const& size, int numSamples = 1);

		bool createBufferRHIResource(IntVector2 const& size, int numSamples = 1);
		void releaseBufferRHIResource();

		bool initializeRHI(IntVector2 const& size, int numSamples = 1);
		void releaseRHI();

		RHITexture2D&  getFrameTexture() { return *mFrameTextures[mIdxRenderFrameTexture]; }
		RHITexture2D&  getPrevFrameTexture() { return *mFrameTextures[1 - mIdxRenderFrameTexture]; }
		void swapFrameTexture()
		{
			mIdxRenderFrameTexture = 1 - mIdxRenderFrameTexture;
			mFrameBuffer->setTexture(0, getFrameTexture());
		}

		GBufferResource&  getGBuffer() { return mGBuffer; }
		RHITextureDepth&  getDepthTexture() { return *mDepthTexture; }



		RHIFrameBufferRef& getFrameBuffer() { return mFrameBuffer; }

		void attachDepthTexture() { mFrameBuffer->setDepth(*mDepthTexture); }
		void detachDepthTexture() { mFrameBuffer->removeDepth(); }


		void drawDepthTexture(RHICommandList& commandList, int x, int y, int width, int height);


		IntVector2         mSize = IntVector2(0, 0);
		int                mNumSamples = 0;

		GBufferResource    mGBuffer;
		RHITexture2DRef    mFrameTextures[2];
		int                mIdxRenderFrameTexture;


		RHIFrameBufferRef  mFrameBuffer;
		RHITextureDepthRef mDepthTexture;
		RHITextureDepthRef mResolvedDepthTexture;
	};


	class GBufferShaderParameters
	{
	public:
		void bindParameters(ShaderParameterMap const& parameterMap, bool bUseDepth = false);

		void setParameters(RHICommandList& commandList, ShaderProgram& program, GBufferResource& GBufferData, RHISamplerState& sampler);
		void setParameters(RHICommandList& commandList, ShaderProgram& program, FrameRenderTargets& sceneRenderTargets);


		DEFINE_TEXTURE_PARAM(GBufferTextureA);
		DEFINE_TEXTURE_PARAM(GBufferTextureB);
		DEFINE_TEXTURE_PARAM(GBufferTextureC);
		DEFINE_TEXTURE_PARAM(GBufferTextureD);

		ShaderParameter mParamFrameDepthTexture;
	};

	class DeferredBasePassProgram : public MaterialShaderProgram
	{
	public:
		using BaseClass = MaterialShaderProgram;
		DECLARE_EXPORTED_SHADER_PROGRAM(DeferredBasePassProgram, Material, CORE_API);

		static void SetupShaderCompileOption(ShaderCompileOption& option)
		{
			BaseClass::SetupShaderCompileOption(option);
		}

		static char const* GetShaderFileName()
		{
			return "Shader/BasePass";
		}
		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] =
			{
				{ EShader::Vertex , SHADER_ENTRY(BassPassVS) },
				{ EShader::Pixel  , SHADER_ENTRY(BasePassPS) },
			};
			return entries;
		}
	};

}//namespace Render



#endif // BasePassRendering_H_2CFE6D0C_5DFA_446A_B560_66C63B1ADBBF
