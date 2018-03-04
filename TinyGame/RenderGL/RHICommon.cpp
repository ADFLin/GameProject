#include "RHICommon.h"

//#TODO : remove
#include "Material.h"

#include "DrawUtility.h"
#include "ShaderCompiler.h"
#include "VertexFactory.h"
#include "RHICommand.h"

namespace RenderGL
{

	RHITexture2DRef GDefaultMaterialTexture2D;
	RHITexture2DRef GWhiteTexture2D;
	RHITexture2DRef GBlackTexture2D;
	RHITextureCubeRef GWhiteTextureCube;
	RHITextureCubeRef GBlackTextureCube;

	MaterialMaster* GDefalutMaterial = nullptr;

	MaterialShaderProgram GSimpleBasePass;

	bool InitGlobalRHIResource()
	{
		uint32 colorW[4] = { 0xffffffff , 0xffffffff , 0xffffffff , 0xffffffff };
		uint32 colorB[4] = { 0, 0 , 0 , 0 };

		GWhiteTexture2D = RHICreateTexture2D();
		GWhiteTexture2D = RHICreateTexture2D();
		GWhiteTextureCube = RHICreateTextureCube();
		GBlackTextureCube = RHICreateTextureCube();
		if( !GWhiteTexture2D->create(Texture::eRGBA8, 2, 2, colorW) ||
		   !GWhiteTexture2D->create(Texture::eRGBA8, 2, 2, colorB) ||
		   !GWhiteTextureCube->create(Texture::eRGBA8, 2, 2, colorW) ||
		   !GBlackTextureCube->create(Texture::eRGBA8, 2, 2, colorB) )
			return false;

		GWhiteTextureCube->bind();
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		GWhiteTextureCube->unbind();

		GWhiteTexture2D->bind();
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		GWhiteTexture2D->unbind();

		GDefalutMaterial = new MaterialMaster;
		if( !GDefalutMaterial->loadFile("EmptyMaterial") )
			return false;

		GDefaultMaterialTexture2D = RHICreateTexture2D();
		if( !GDefaultMaterialTexture2D->loadFromFile("Texture/Gird.png") )
			return false;

		ShaderCompileOption option;
		option.version = 430;
		VertexFarcoryType::DefaultType->getCompileOption(option);

		if( !ShaderManager::Get().loadFile( 
			GSimpleBasePass ,
			"Shader/SimpleBasePass",
			SHADER_ENTRY(BassPassVS), SHADER_ENTRY(BasePassPS), option , nullptr) )
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
	}

	bool ShaderHelper::init()
	{
		ShaderCompileOption option;
		option.version = 430;

		if( !ShaderManager::Get().loadFile(
			mProgCopyTexture, "Shader/CopyTexture",
			SHADER_ENTRY(CopyTextureVS), SHADER_ENTRY(CopyTexturePS), option) )
			return false;

		if( !ShaderManager::Get().loadFile(
			mProgCopyTextureMask,"Shader/CopyTexture",
			SHADER_ENTRY(CopyTextureVS), SHADER_ENTRY(CopyTextureMaskPS), option) )
			return false;

		if( !ShaderManager::Get().loadFile(
			mProgCopyTextureBias, "Shader/CopyTexture",
			SHADER_ENTRY(CopyTextureVS), SHADER_ENTRY(CopyTextureBaisPS), option) )
			return false;

		if( !ShaderManager::Get().loadFile(
			mProgMappingTextureColor, "Shader/CopyTexture",
			SHADER_ENTRY(CopyTextureVS), SHADER_ENTRY(MappingTextureColorPS), option) )
			return false;

		if( !mFrameBuffer.create() )
			return false;

		mFrameBuffer.addTexture(*GWhiteTexture2D);
		return true;
	}

	void ShaderHelper::clearBuffer(RHITexture2D& texture, float clearValue[])
	{
		mFrameBuffer.setTexture(0, texture);
		GL_BIND_LOCK_OBJECT(mFrameBuffer);
		glClearBufferfv(GL_COLOR, 0, (float const*)clearValue);
	}

	void ShaderHelper::clearBuffer(RHITexture2D& texture, uint32 clearValue[])
	{
		mFrameBuffer.setTexture(0, texture);
		GL_BIND_LOCK_OBJECT(mFrameBuffer);
		glClearBufferuiv(GL_COLOR, 0, clearValue);
	}

	void ShaderHelper::clearBuffer(RHITexture2D& texture, int32 clearValue[])
	{
		mFrameBuffer.setTexture(0, texture);
		GL_BIND_LOCK_OBJECT( mFrameBuffer );
		glClearBufferiv(GL_COLOR, 0, clearValue);
	}

	void ShaderHelper::DrawCubeTexture(RHITextureCube& texCube, Vec2i const& pos, int length)
	{
		glEnable(GL_TEXTURE_CUBE_MAP);
		GL_BIND_LOCK_OBJECT(texCube);
		glColor3f(1, 1, 1);
		int offset = 10;

		if( length == 0 )
			length = 100;

		glPushMatrix();

		glTranslatef(pos.x, pos.y, 0);
		glTranslatef(1 * length, 1 * length, 0);
		glBegin(GL_QUADS); //x
		glTexCoord3f(1, -1, -1); glVertex2f(0, 0);
		glTexCoord3f(1, 1, -1);  glVertex2f(length, 0);
		glTexCoord3f(1, 1, 1);  glVertex2f(length, length);
		glTexCoord3f(1, -1, 1);  glVertex2f(0, length);
		glEnd();
		glTranslatef(2 * length, 0, 0);
		glBegin(GL_QUADS); //-x
		glTexCoord3f(-1, 1, -1); glVertex2f(0, 0);
		glTexCoord3f(-1, -1, -1);  glVertex2f(length, 0);
		glTexCoord3f(-1, -1, 1); glVertex2f(length, length);
		glTexCoord3f(-1, 1, 1);  glVertex2f(0, length);
		glEnd();
		glTranslatef(-1 * length, 0, 0);
		glBegin(GL_QUADS); //y
		glTexCoord3f(1, 1, -1); glVertex2f(0, 0);
		glTexCoord3f(-1, 1, -1); glVertex2f(length, 0);
		glTexCoord3f(-1, 1, 1); glVertex2f(length, length);
		glTexCoord3f(1, 1, 1);  glVertex2f(0, length);
		glEnd();
		glTranslatef(-2 * length, 0, 0);
		glBegin(GL_QUADS); //-y
		glTexCoord3f(-1, -1, -1); glVertex2f(0, 0);
		glTexCoord3f(1, -1, -1); glVertex2f(length, 0);
		glTexCoord3f(1, -1, 1); glVertex2f(length, length);
		glTexCoord3f(-1, -1, 1); glVertex2f(0, length);
		glEnd();
		glTranslatef(1 * length, 1 * length, 0);
		glBegin(GL_QUADS); //z
		glTexCoord3f(1, -1, 1); glVertex2f(0, 0);
		glTexCoord3f(1, 1, 1); glVertex2f(length, 0);
		glTexCoord3f(-1, 1, 1);  glVertex2f(length, length);
		glTexCoord3f(-1, -1, 1);  glVertex2f(0, length);
		glEnd();
		glTranslatef(0 * length, -2 * length, 0);
		glBegin(GL_QUADS); //-z
		glTexCoord3f(-1, -1, -1);  glVertex2f(0, 0);
		glTexCoord3f(-1, 1, -1);  glVertex2f(length, 0);
		glTexCoord3f(1, 1, -1);  glVertex2f(length, length);
		glTexCoord3f(1, -1, -1); glVertex2f(0, length);
		glEnd();

		glPopMatrix();
		glDisable(GL_TEXTURE_CUBE_MAP);
	}


	void ShaderHelper::DrawTexture(RHITexture2D& texture, Vec2i const& pos, Vec2i const& size)
	{
		glLoadIdentity();
		glEnable(GL_TEXTURE_2D);
		texture.bind();
		glColor3f(1, 1, 1);
		DrawUtility::Rect(pos.x, pos.y, size.x, size.y);
		texture.unbind();
		glDisable(GL_TEXTURE_2D);
	}

	void ShaderHelper::copyTextureToBuffer(RHITexture2D& copyTexture)
	{
		GL_BIND_LOCK_OBJECT(mProgCopyTexture);
		mProgCopyTexture.setParameters(copyTexture);
		DrawUtility::ScreenRect();
	}

	void ShaderHelper::copyTextureMaskToBuffer(RHITexture2D& copyTexture, Vector4 const& colorMask)
	{
		GL_BIND_LOCK_OBJECT(mProgCopyTextureMask);
		mProgCopyTextureMask.setParameters( copyTexture , colorMask );
		DrawUtility::ScreenRect();
	}

	void ShaderHelper::copyTextureBiasToBuffer(RHITexture2D& copyTexture, float colorBais[2])
	{
		GL_BIND_LOCK_OBJECT(mProgCopyTextureBias);
		mProgCopyTextureBias.setParameters(copyTexture, colorBais);
		DrawUtility::ScreenRect();
	}

	void ShaderHelper::mapTextureColorToBuffer(RHITexture2D& copyTexture, Vector4 const& colorMask, float valueFactor[2])
	{
		GL_BIND_LOCK_OBJECT(mProgMappingTextureColor);
		mProgMappingTextureColor.setParameters(copyTexture, colorMask, valueFactor);
		DrawUtility::ScreenRect();
	}

	void ShaderHelper::copyTexture(RHITexture2D& destTexture, RHITexture2D& srcTexture)
	{
		mFrameBuffer.setTexture(0, destTexture);
		mFrameBuffer.bind();
		copyTextureToBuffer(srcTexture);
		mFrameBuffer.unbind();
	}

	void ShaderHelper::reload()
	{
		ShaderManager::Get().reloadShader(mProgCopyTexture);
		ShaderManager::Get().reloadShader(mProgCopyTextureMask);
		ShaderManager::Get().reloadShader(mProgMappingTextureColor);
		ShaderManager::Get().reloadShader(mProgCopyTextureBias);
	}


	void CopyTextureMaskProgram::bindParameters()
	{
		mParamCopyTexture.bind(*this, SHADER_PARAM(CopyTexture));
		mParamColorMask.bind(*this, SHADER_PARAM(ColorMask));
	}

	void CopyTextureMaskProgram::setParameters(RHITexture2D& copyTexture, Vector4 const& colorMask)
	{
		setTexture(mParamCopyTexture, copyTexture);
		setParam(mParamColorMask, colorMask);
	}

	void CopyTextureProgram::bindParameters()
	{
		mParamCopyTexture.bind(*this, SHADER_PARAM(CopyTexture));
	}

	void CopyTextureProgram::setParameters(RHITexture2D& copyTexture)
	{
		setTexture(mParamCopyTexture, copyTexture);
	}


	void CopyTextureBiasProgram::bindParameters()
	{
		mParamCopyTexture.bind(*this, SHADER_PARAM(CopyTexture));
		mParamColorBais.bind(*this, SHADER_PARAM(ColorBais));
	}

	void CopyTextureBiasProgram::setParameters(RHITexture2D& copyTexture, float colorBais[2])
	{
		setTexture(mParamCopyTexture, copyTexture);
		setParam(mParamColorBais, colorBais[0], colorBais[1]);
	}

	void MappingTextureColorProgram::bindParameters()
	{
		mParamCopyTexture.bind(*this, SHADER_PARAM(CopyTexture));
		mParamColorMask.bind(*this, SHADER_PARAM(ColorMask));
		mParamValueFactor.bind(*this, SHADER_PARAM(ValueFactor));
	}

	void MappingTextureColorProgram::setParameters(RHITexture2D& copyTexture, Vector4 const& colorMask, float valueFactor[2])
	{
		setTexture(mParamCopyTexture, copyTexture);
		setParam(mParamColorMask, colorMask);
		setParam(mParamValueFactor, valueFactor[0], valueFactor[1]);
	}

}//namespace RenderGL
