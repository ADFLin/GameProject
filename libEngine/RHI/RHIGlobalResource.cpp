#include "RHIGlobalResource.h"

#include "RHICommand.h"
#include "Material.h"
#include "ShaderCompiler.h"

//#TODO: Remove Me
#include "OpenGLCommon.h"



namespace Render
{

#if CORE_SHARE_CODE
	RHITexture2DRef GDefaultMaterialTexture2D;
	RHITexture2DRef GWhiteTexture2D;
	RHITexture2DRef GBlackTexture2D;
	RHITextureCubeRef GWhiteTextureCube;
	RHITextureCubeRef GBlackTextureCube;

	MaterialMaster* GDefalutMaterial = nullptr;

	MaterialShaderProgram GSimpleBasePass;

	bool InitGlobalRHIResource()
	{

		GlobalRHIResourceBase::RestoreAllResource();

		uint32 colorW[4] = { 0xffffffff , 0xffffffff , 0xffffffff , 0xffffffff };
		uint32 colorB[4] = { 0, 0 , 0 , 0 };

		GWhiteTexture2D = RHICreateTexture2D(Texture::eRGBA8, 2, 2, 0, 1, BCF_DefalutValue, colorW);
		if( !GWhiteTexture2D.isValid() )
			return false;
		GBlackTexture2D = RHICreateTexture2D(Texture::eRGBA8, 2, 2, 0, 1, BCF_DefalutValue, colorB);
		if( !GBlackTexture2D.isValid() )
			return false;
		void* cubeWData[] = { colorW , colorW , colorW , colorW , colorW , colorW };
		GWhiteTextureCube = RHICreateTextureCube(Texture::eRGBA8, 2, 0, BCF_DefalutValue, cubeWData);
		if( !GWhiteTextureCube.isValid() )
			return false;
		void* cubeBData[] = { colorB , colorB , colorB , colorB , colorB , colorB };
		GBlackTextureCube = RHICreateTextureCube(Texture::eRGBA8, 2, 0, BCF_DefalutValue, cubeBData);
		if( !GBlackTextureCube.isValid() )
			return false;

		OpenGLCast::To(GWhiteTextureCube)->bind();
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		OpenGLCast::To(GWhiteTextureCube)->unbind();

		OpenGLCast::To(GWhiteTexture2D)->bind();
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		OpenGLCast::To(GWhiteTexture2D)->unbind();

		GDefalutMaterial = new MaterialMaster;
		if( !GDefalutMaterial->loadFile("EmptyMaterial") )
			return false;

		GDefaultMaterialTexture2D = RHIUtility::LoadTexture2DFromFile("Texture/Gird.png");
		if( !GDefaultMaterialTexture2D.isValid() )
			return false;

		ShaderCompileOption option;
		option.version = 430;
		VertexFactoryType::DefaultType->getCompileOption(option);

		if( !ShaderManager::Get().loadFile(
			GSimpleBasePass,
			"Shader/SimpleBasePass",
			SHADER_ENTRY(BassPassVS), SHADER_ENTRY(BasePassPS), option, nullptr) )
			return false;


		return true;
	}

	void ReleaseGlobalRHIResource()
	{
		GDefaultMaterialTexture2D.release();
		GWhiteTexture2D.release();
		GBlackTexture2D.release();
		GWhiteTextureCube.release();
		GBlackTextureCube.release();

		GDefalutMaterial->releaseRHI();



		delete GDefalutMaterial;


		GlobalRHIResourceBase::ReleaseAllResource();
	}


	GlobalRHIResourceBase* GStaticResourceHead = nullptr;

	GlobalRHIResourceBase::GlobalRHIResourceBase()
	{
		mNext = GStaticResourceHead;
		GStaticResourceHead = this;
	}


	void GlobalRHIResourceBase::ReleaseAllResource()
	{
		GlobalRHIResourceBase* cur = GStaticResourceHead;

		while( cur )
		{
			cur->releaseRHI();
			cur = cur->mNext;
		}
	}

	void GlobalRHIResourceBase::RestoreAllResource()
	{
		GlobalRHIResourceBase* cur = GStaticResourceHead;

		while( cur )
		{
			cur->restoreRHI();
			cur = cur->mNext;
		}
	}

#endif
}

