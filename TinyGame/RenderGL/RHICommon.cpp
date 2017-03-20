#include "RHICommon.h"

//#TODO : remove
#include "Material.h"

#include "GLDrawUtility.h"
#include "ShaderCompiler.h"

namespace RenderGL
{

	RHITexture2DRef GDefaultMaterialTexture2D;
	RHITexture2DRef GWhiteTexture2D;
	RHITexture2DRef GBlackTexture2D;
	RHITextureCubeRef GWhiteTextureCube;
	RHITextureCubeRef GBlackTextureCube;

	MaterialMaster* GDefalutMaterial = nullptr;

	ShaderProgram GSimpleBasePass;

	bool InitGlobalRHIResource()
	{
		uint32 colorW[4] = { 0xffffffff , 0xffffffff , 0xffffffff , 0xffffffff };
		uint32 colorB[4] = { 0, 0 , 0 , 0 };

		GWhiteTexture2D = new RHITexture2D;
		GWhiteTexture2D = new RHITexture2D;
		GWhiteTextureCube = new RHITextureCube;
		GBlackTextureCube = new RHITextureCube;
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

		GDefaultMaterialTexture2D = new RHITexture2D;
		if( !GDefaultMaterialTexture2D->loadFile("Texture/Gird.png") )
			return false;

		if( !ShaderManager::getInstance().loadFile( 
			GSimpleBasePass ,
			"Shader/SimpleBasePass",
			SHADER_ENTRY(BassPassVS), SHADER_ENTRY(BasePassPS), "") )
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
		if( !ShaderManager::getInstance().loadFile(
			mProgCopyTexture ,
			"Shader/CopyTexture",
			SHADER_ENTRY(CopyTextureVS), SHADER_ENTRY(CopyTexturePS)) )
			return false;

		if( !ShaderManager::getInstance().loadFile(
			mProgCopyTextureMask ,
			"Shader/CopyTexture",
			SHADER_ENTRY(CopyTextureVS), SHADER_ENTRY(CopyTextureMaskPS)) )
			return false;

		if( !ShaderManager::getInstance().loadFile(
			mProgCopyTextureBias ,
			"Shader/CopyTexture",
			SHADER_ENTRY(CopyTextureVS), SHADER_ENTRY(CopyTextureBaisPS)) )
			return false;

		if( !ShaderManager::getInstance().loadFile(
			mProgMappingTextureColor ,
			"Shader/CopyTexture",
			SHADER_ENTRY(CopyTextureVS), SHADER_ENTRY(MappingTextureColorPS)) )
			return false;

		return true;
	}

	void ShaderHelper::drawCubeTexture(RHITextureCube& texCube, Vec2i const& pos, int length)
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


	void ShaderHelper::drawTexture(RHITexture2D& texture, Vec2i const& pos, Vec2i const& size)
	{
		glLoadIdentity();
		glEnable(GL_TEXTURE_2D);
		texture.bind();
		glColor3f(1, 1, 1);
		DrawUtiltiy::Rect(pos.x, pos.y, size.x, size.y);
		texture.unbind();
		glDisable(GL_TEXTURE_2D);
	}

	void ShaderHelper::copyTextureToBuffer(RHITexture2D& copyTexture)
	{
		GL_BIND_LOCK_OBJECT(mProgCopyTexture);
		mProgCopyTexture.setTexture(SHADER_PARAM(CopyTexture), copyTexture);
		DrawUtiltiy::ScreenRect();
	}

	void ShaderHelper::copyTextureMaskToBuffer(RHITexture2D& copyTexture, Vector4 const& colorMask)
	{
		GL_BIND_LOCK_OBJECT(mProgCopyTextureMask);
		mProgCopyTextureMask.setTexture(SHADER_PARAM(CopyTexture), copyTexture);
		mProgCopyTextureMask.setParam(SHADER_PARAM(ColorMask), colorMask);
		DrawUtiltiy::ScreenRect();
	}

	void ShaderHelper::copyTextureBiasToBuffer(RHITexture2D& copyTexture, float colorBais[2])
	{
		GL_BIND_LOCK_OBJECT(mProgCopyTextureBias);
		mProgCopyTextureBias.setTexture(SHADER_PARAM(CopyTexture), copyTexture);
		mProgCopyTextureBias.setParam(SHADER_PARAM(ColorBais), colorBais[0] , colorBais[1] );
		DrawUtiltiy::ScreenRect();
	}

	void ShaderHelper::mapTextureColorToBuffer(RHITexture2D& copyTexture, Vector4 const& colorMask, float valueFactor[2])
	{
		GL_BIND_LOCK_OBJECT(mProgMappingTextureColor);
		mProgMappingTextureColor.setTexture(SHADER_PARAM(CopyTexture), copyTexture);
		mProgMappingTextureColor.setParam(SHADER_PARAM(ColorMask), colorMask);
		mProgMappingTextureColor.setParam(SHADER_PARAM(ValueFactor), valueFactor[0], valueFactor[1]);
		DrawUtiltiy::ScreenRect();
	}

	void ShaderHelper::copyTexture(RHITexture2D& destTexture, RHITexture2D& srcTexture)
	{
		static FrameBuffer frameBuffer;
		static bool bInit = false;
		if( bInit == false )
		{
			frameBuffer.create();
			frameBuffer.addTexture(destTexture);
			bInit = true;
		}
		else
		{
			frameBuffer.setTexture(0, destTexture);
		}

		frameBuffer.bind();
		copyTextureToBuffer(srcTexture);
		frameBuffer.unbind();
	}

	void ShaderHelper::reload()
	{
		ShaderManager::getInstance().reloadShader(mProgCopyTexture);
		ShaderManager::getInstance().reloadShader(mProgCopyTextureMask);
		ShaderManager::getInstance().reloadShader(mProgMappingTextureColor);
		ShaderManager::getInstance().reloadShader(mProgCopyTextureBias);
	}


}//namespace RenderGL
