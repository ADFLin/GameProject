#include "RHIGlobalResource.h"

#include "RHICommand.h"
#include "Material.h"
#include "ShaderManager.h"

//#TODO: Remove Me
#include "OpenGLCommon.h"



namespace Render
{



	void SimplePipelineProgram::setParameters(RHICommandList& commandList, Matrix4 const& transform, LinearColor const& color, RHITexture2D* texture, RHISamplerState* sampler)
	{
		if (mParamTexture.isBound())
		{
			setTexture(commandList, mParamTexture, texture ? *texture : *GWhiteTexture2D, mParamTextureSampler, sampler ? *sampler : TStaticSamplerState<>::GetRHI());
		}
		setParam(commandList, mParamColor, color);
		setParam(commandList, mParamXForm, transform);
	}

	class ShaderCompileOption;
	struct ShaderEntryInfo;


#if CORE_SHARE_CODE

	IMPLEMENT_SHADER_PROGRAM(SimplePipelineProgram);

	SimplePipelineProgram* SimplePipelineProgram::Get(uint32 attributeMask , bool bHaveTexture)
	{
		SimplePipelineProgram::PermutationDomain permutationVector;
		permutationVector.set< SimplePipelineProgram::HaveVertexColor >(attributeMask & BIT(EVertex::ATTRIBUTE_COLOR));
		permutationVector.set< SimplePipelineProgram::HaveTexcoord >((attributeMask & BIT(EVertex::ATTRIBUTE_TEXCOORD)) && bHaveTexture);
		return ShaderManager::Get().getGlobalShaderT< SimplePipelineProgram >(permutationVector);
	}

	TGlobalRenderResource<RHITexture2D>   GDefaultMaterialTexture2D;
	TGlobalRenderResource<RHITexture1D>   GWhiteTexture1D;
	TGlobalRenderResource<RHITexture1D>   GBlackTexture1D;
	TGlobalRenderResource<RHITexture2D>   GWhiteTexture2D;
	TGlobalRenderResource<RHITexture2D>   GBlackTexture2D;
	TGlobalRenderResource<RHITexture3D>   GWhiteTexture3D;
	TGlobalRenderResource<RHITexture3D>   GBlackTexture3D;
	TGlobalRenderResource<RHITextureCube> GWhiteTextureCube;
	TGlobalRenderResource<RHITextureCube> GBlackTextureCube;

	CORE_API MaterialMaster* GDefalutMaterial = nullptr;
	CORE_API MaterialShaderProgram GSimpleBasePass;

	bool InitGlobalRenderResource()
	{
		if (GRHISystem->getName() == RHISystemName::Vulkan)
			return true;

		TRACE_RESOURCE_TAG_SCOPE("GlobalRHIResource");

		if (GRHISystem->getName() == RHISystemName::OpenGL ||
			GRHISystem->getName() == RHISystemName::D3D11)
		{

		uint32 colorW[] = { 0xffffffff , 0xffffffff , 0xffffffff , 0xffffffff , 0xffffffff , 0xffffffff , 0xffffffff , 0xffffffff };
		uint32 colorB[] = { 0, 0 , 0 , 0 , 0 , 0 , 0 , 0 };

		{
			TIME_SCOPE("GlobalTexture");

			VERIFY_RETURN_FALSE(GWhiteTexture2D.initialize(RHICreateTexture2D(ETexture::RGBA8, 2, 2, 1, 1, BCF_DefalutValue, colorW)));
			VERIFY_RETURN_FALSE(GBlackTexture2D.initialize(RHICreateTexture2D(ETexture::RGBA8, 2, 2, 1, 1, BCF_DefalutValue, colorB)));

			VERIFY_RETURN_FALSE(GWhiteTexture1D.initialize(RHICreateTexture1D(ETexture::RGBA8, 2, 1, BCF_DefalutValue, colorW)));
			VERIFY_RETURN_FALSE(GBlackTexture1D.initialize(RHICreateTexture1D(ETexture::RGBA8, 2, 1, BCF_DefalutValue, colorB)));

			VERIFY_RETURN_FALSE(GWhiteTexture3D.initialize(RHICreateTexture3D(ETexture::RGBA8, 2, 2, 2, 1, 1, BCF_DefalutValue, colorW)));
			VERIFY_RETURN_FALSE(GBlackTexture3D.initialize(RHICreateTexture3D(ETexture::RGBA8, 2, 2, 2, 1, 1, BCF_DefalutValue, colorB)));
			void* cubeWData[] = { colorW , colorW , colorW , colorW , colorW , colorW };
			VERIFY_RETURN_FALSE(GWhiteTextureCube.initialize(RHICreateTextureCube(ETexture::RGBA8, 2, 0, BCF_DefalutValue, cubeWData)));
			void* cubeBData[] = { colorB , colorB , colorB , colorB , colorB , colorB };
			VERIFY_RETURN_FALSE(GBlackTextureCube.initialize(RHICreateTextureCube(ETexture::RGBA8, 2, 0, BCF_DefalutValue, cubeBData)));

			if( GRHISystem->getName() == RHISystemName::OpenGL )
			{
				OpenGLCast::To(GWhiteTextureCube.getResource())->bind();
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
				OpenGLCast::To(GWhiteTextureCube.getResource())->unbind();

				OpenGLCast::To(GWhiteTexture2D.getResource())->bind();
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				OpenGLCast::To(GWhiteTexture2D.getResource())->unbind();
			}
		}

		if (GRHISystem->getName() == RHISystemName::OpenGL)
		{
			TIME_SCOPE("EmptyMaterial");
			GDefalutMaterial = new MaterialMaster;
			if( !GDefalutMaterial->loadFile("EmptyMaterial") )
				return false;
		}

		{
			TIME_SCOPE("EmptyTexture");
			GDefaultMaterialTexture2D.initialize(RHIUtility::LoadTexture2DFromFile("Texture/Gird.png"));
			if( !GDefaultMaterialTexture2D.isValid() )
				return false;
		}

			TIME_SCOPE("SimpleBasePass");
			ShaderCompileOption option;
			VertexFactoryType::DefaultType->getCompileOption(option);

			if( !ShaderManager::Get().loadFile(
				GSimpleBasePass,
				"Shader/SimpleBasePass",
				SHADER_ENTRY(BasePassVS), SHADER_ENTRY(BasePassPS), option, nullptr) )
				return false;
		}

		for (uint32 i = 0; i < SimplePipelineProgram::PermutationDomain::GetPermutationCount(); ++i)
		{
			ShaderManager::Get().getGlobalShaderT< SimplePipelineProgram >(i);
		}
		return true;
	}

	void ReleaseGlobalRenderResource()
	{
		if ( GDefalutMaterial )
		{
			GDefalutMaterial->releaseRHI();
			delete GDefalutMaterial;
			GDefalutMaterial = nullptr;
		}

		GSimpleBasePass.releaseRHI();
	}


	GlobalRenderResourceBase* GStaticResourceHead = nullptr;

	GlobalRenderResourceBase::GlobalRenderResourceBase()
	{
		mNext = GStaticResourceHead;
		GStaticResourceHead = this;
	}


	void GlobalRenderResourceBase::ReleaseAllResource()
	{
		GlobalRenderResourceBase* cur = GStaticResourceHead;

		while( cur )
		{
			cur->releaseRHI();
			cur = cur->mNext;
		}
	}

	void GlobalRenderResourceBase::RestoreAllResource()
	{
		GlobalRenderResourceBase* cur = GStaticResourceHead;

		while( cur )
		{
			cur->restoreRHI();
			cur = cur->mNext;
		}
	}

#endif

}

