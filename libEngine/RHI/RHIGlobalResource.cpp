#include "RHIGlobalResource.h"

#include "RHICommand.h"
#include "Material.h"
#include "ShaderManager.h"

//#TODO: Remove Me
#include "OpenGLCommon.h"



namespace Render
{

#if CORE_SHARE_CODE
	TGlobalRHIResource<RHITexture2D>   GDefaultMaterialTexture2D;
	TGlobalRHIResource<RHITexture1D>   GWhiteTexture1D;
	TGlobalRHIResource<RHITexture1D>   GBlackTexture1D;
	TGlobalRHIResource<RHITexture2D>   GWhiteTexture2D;
	TGlobalRHIResource<RHITexture2D>   GBlackTexture2D;
	TGlobalRHIResource<RHITexture3D>   GWhiteTexture3D;
	TGlobalRHIResource<RHITexture3D>   GBlackTexture3D;
	TGlobalRHIResource<RHITextureCube> GWhiteTextureCube;
	TGlobalRHIResource<RHITextureCube> GBlackTextureCube;

	MaterialMaster* GDefalutMaterial = nullptr;

	MaterialShaderProgram GSimpleBasePass;

	bool InitGlobalRHIResource()
	{

		GlobalRHIResourceBase::RestoreAllResource();

		uint32 colorW[] = { 0xffffffff , 0xffffffff , 0xffffffff , 0xffffffff , 0xffffffff , 0xffffffff , 0xffffffff , 0xffffffff };
		uint32 colorB[] = { 0, 0 , 0 , 0 , 0 , 0 , 0 , 0 };

		VERIFY_RETURN_FALSE(GWhiteTexture1D.initialize( RHICreateTexture1D(Texture::eRGBA8, 2, 1, BCF_DefalutValue, colorW) ) );
		VERIFY_RETURN_FALSE(GBlackTexture1D.initialize( RHICreateTexture1D(Texture::eRGBA8, 2, 1, BCF_DefalutValue, colorB) ) );
		VERIFY_RETURN_FALSE(GWhiteTexture2D.initialize( RHICreateTexture2D(Texture::eRGBA8, 2, 2, 1, 1, BCF_DefalutValue, colorW)) );
		VERIFY_RETURN_FALSE(GBlackTexture2D.initialize( RHICreateTexture2D(Texture::eRGBA8, 2, 2, 1, 1, BCF_DefalutValue, colorB)) );
		VERIFY_RETURN_FALSE(GWhiteTexture3D.initialize( RHICreateTexture3D(Texture::eRGBA8, 2, 2, 2, 1, 1, BCF_DefalutValue, colorW)) );
		VERIFY_RETURN_FALSE(GBlackTexture3D.initialize( RHICreateTexture3D(Texture::eRGBA8, 2, 2, 2, 1, 1, BCF_DefalutValue, colorB)) );
		void* cubeWData[] = { colorW , colorW , colorW , colorW , colorW , colorW };
		VERIFY_RETURN_FALSE(GWhiteTextureCube.initialize( RHICreateTextureCube(Texture::eRGBA8, 2, 0, BCF_DefalutValue, cubeWData) ) );
		void* cubeBData[] = { colorB , colorB , colorB , colorB , colorB , colorB };
		VERIFY_RETURN_FALSE(GBlackTextureCube.initialize( RHICreateTextureCube(Texture::eRGBA8, 2, 0, BCF_DefalutValue, cubeBData) ) );


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

		GDefaultMaterialTexture2D.initialize( RHIUtility::LoadTexture2DFromFile("Texture/Gird.png") );
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
		GDefalutMaterial->releaseRHI();
		delete GDefalutMaterial;
		GDefalutMaterial = nullptr;

		GSimpleBasePass.releaseRHI();

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

