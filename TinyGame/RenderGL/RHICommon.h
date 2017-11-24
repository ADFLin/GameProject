#pragma once
#ifndef RHICommon_H_F71942CB_2583_4990_B63B_D7B4FC78E1DB
#define RHICommon_H_F71942CB_2583_4990_B63B_D7B4FC78E1DB

#include "GLCommon.h"
#include "GLUtility.h"
#include "TVector2.h"
#include "Singleton.h"

namespace RenderGL
{
	using namespace RenderGL;
	typedef TVector2<int> Vec2i;

	class ShaderCompileOption;
	class MaterialShaderProgram;

	extern RHITexture2DRef  GDefaultMaterialTexture2D;
	extern RHITexture2DRef  GWhiteTexture2D;
	extern RHITexture2DRef  GBlackTexture2D;
	extern RHITextureCubeRef  GWhiteTextureCube;
	extern RHITextureCubeRef  GBlackTextureCube;
	extern class MaterialMaster* GDefalutMaterial;

	extern MaterialShaderProgram GSimpleBasePass;

	bool InitGlobalRHIResource();
	void ReleaseGlobalRHIResource();


	class CopyTextureProgram : public ShaderProgram
	{
	public:
		void bindParameters();
		void setParameters(RHITexture2D& copyTexture);

		ShaderParameter mParamCopyTexture;

	};

	class CopyTextureMaskProgram : public ShaderProgram
	{
	public:
		void bindParameters();
		void setParameters(RHITexture2D& copyTexture, Vector4 const& colorMask);

		ShaderParameter mParamColorMask;
		ShaderParameter mParamCopyTexture;
	};


	class CopyTextureBiasProgram : public ShaderProgram
	{
	public:
		void bindParameters();
		void setParameters(RHITexture2D& copyTexture, float colorBais[2]);

		ShaderParameter mParamColorBais;
		ShaderParameter mParamCopyTexture;

	};

	class MappingTextureColorProgram : public ShaderProgram
	{
	public:
		void bindParameters();
		void setParameters(RHITexture2D& copyTexture, Vector4 const& colorMask, float valueFactor[2]);

		ShaderParameter mParamCopyTexture;
		ShaderParameter mParamColorMask;
		ShaderParameter mParamValueFactor;
	};

	class ShaderHelper : public SingletonT< ShaderHelper >
	{
	public:
		bool init();

		void copyTextureToBuffer(RHITexture2D& copyTexture);
		void copyTextureMaskToBuffer(RHITexture2D& copyTexture, Vector4 const& colorMask);
		void copyTextureBiasToBuffer(RHITexture2D& copyTexture, float colorBais[2]);
		void mapTextureColorToBuffer(RHITexture2D& copyTexture, Vector4 const& colorMask, float valueFactor[2]);
		void copyTexture(RHITexture2D& destTexture, RHITexture2D& srcTexture);

		void clearBuffer(RHITexture2D& texture, float clearValue[]);
		void clearBuffer(RHITexture2D& texture, uint32 clearValue[]);
		void clearBuffer(RHITexture2D& texture, int32 clearValue[]);

		static void DrawCubeTexture(RHITextureCube& texCube, Vec2i const& pos, int length);
		static void DrawTexture(RHITexture2D& texture, Vec2i const& pos, Vec2i const& size);

		void reload();

		CopyTextureProgram mProgCopyTexture;
		CopyTextureMaskProgram mProgCopyTextureMask;
		CopyTextureBiasProgram mProgCopyTextureBias;
		MappingTextureColorProgram mProgMappingTextureColor;
		FrameBuffer mFrameBuffer;


	};

}//namespace RenderGL

#endif // RHICommon_H_F71942CB_2583_4990_B63B_D7B4FC78E1DB