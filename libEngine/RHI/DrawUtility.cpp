#include "DrawUtility.h"

#include "ShaderManager.h"
#include "ProfileSystem.h"

#include "RHI/RHICommand.h"

namespace Render
{
	struct VertexXYZ_T1
	{
		Vector3 pos;
		Vector2 uv;
	};

	struct VertexXYZW_T1
	{
		Vector4 pos;
		Vector2 uv;
	};

	struct VertexXY_T1
	{
		Vector2 pos;
		Vector2 tex;
	};

	struct VertexXY_CA_T1
	{
		Vector2 pos;
		LinearColor color;
		Vector2 tex;
	};

	static const VertexXYZW_T1 GScreenVertices_Rect[] =
	{
		{ Vector4(-1 , -1 , 0 , 1) , Vector2(0,0) },
		{ Vector4(1 , -1 , 0 , 1) , Vector2(1,0) },
		{ Vector4(1, 1 , 0 , 1) , Vector2(1,1) },
		{ Vector4(-1, 1, 0 , 1) , Vector2(0,1) },
	};

	static const VertexXYZW_T1 GScreenVertices_OptimisedTriangle[] =
	{
		{ Vector4(-1 , -1 , 0 , 1) , Vector2(0,0) },
		{ Vector4(3 , -1 , 0 , 1) , Vector2(2,0) },
		{ Vector4(-1, 3 , 0 , 1) , Vector2(0,2) },
	};

	void DrawUtility::CubeLine(RHICommandList& commandList)
	{
		static Vector3 const v[] =
		{
			Vector3(1,0,0),Vector3(1,1,0),Vector3(1,1,0),Vector3(1,1,1),
			Vector3(1,1,1),Vector3(1,0,1),Vector3(1,0,1),Vector3(1,0,0),
			Vector3(0,0,0),Vector3(0,1,0),Vector3(0,1,0),Vector3(0,1,1),
			Vector3(0,1,1),Vector3(0,0,1),Vector3(0,0,1),Vector3(0,0,0),
			Vector3(0,0,0),Vector3(1,0,0),Vector3(0,1,0),Vector3(1,1,0),
			Vector3(0,1,1),Vector3(1,1,1),Vector3(0,0,1),Vector3(1,0,1),
		};
		TRenderRT< RTVF_XYZ >::Draw(commandList, EPrimitive::LineList, v, 4 * 6, sizeof(Vector3));
	}

	void DrawUtility::CubeMesh(RHICommandList& commandList)
	{
		static Vector3 const v[] =
		{
			//+x
			Vector3(1,1,1),Vector3(1,0,0),Vector3(1,0,1),Vector3(1,0,0),
			Vector3(1,0,0),Vector3(1,0,0),Vector3(1,1,0),Vector3(1,0,0),
			//-x
			Vector3(0,1,0),Vector3(-1,0,0),Vector3(0,0,0),Vector3(-1,0,0),
			Vector3(0,0,1),Vector3(-1,0,0),Vector3(0,1,1),Vector3(-1,0,0),
			//+y
			Vector3(1,1,1),Vector3(0,1,0),Vector3(1,1,0),Vector3(0,1,0),
			Vector3(0,1,0),Vector3(0,1,0),Vector3(0,1,1),Vector3(0,1,0),
			//-y
			Vector3(0,0,1),Vector3(0,-1,0),Vector3(0,0,0),Vector3(0,-1,0),
			Vector3(1,0,0),Vector3(0,-1,0),Vector3(1,0,1),Vector3(0,-1,0),
			//+z
			Vector3(1,1,1),Vector3(0,0,1),Vector3(0,1,1),Vector3(0,0,1),
			Vector3(0,0,1),Vector3(0,0,1),Vector3(1,0,1),Vector3(0,0,1),
			//-z
			Vector3(1,0,0),Vector3(0,0,-1),Vector3(0,0,0),Vector3(0,0,-1),
			Vector3(0,1,0),Vector3(0,0,-1),Vector3(1,1,0),Vector3(0,0,-1),
		};
		TRenderRT< RTVF_XYZ_N >::Draw(commandList, EPrimitive::Quad, v, 4 * 6, 2 * sizeof(Vector3));
	}

	void DrawUtility::AixsLine(RHICommandList& commandList)
	{
		static Vector3 const v[12] =
		{
			Vector3(0,0,0),Vector3(1,0,0), Vector3(1,0,0),Vector3(1,0,0),
			Vector3(0,0,0),Vector3(0,1,0), Vector3(0,1,0),Vector3(0,1,0),
			Vector3(0,0,0),Vector3(0,0,1), Vector3(0,0,1),Vector3(0,0,1),
		};
		TRenderRT< RTVF_XYZ_C >::Draw(commandList, EPrimitive::LineList, v, 6, 2 * sizeof(Vector3));
	}

	void DrawUtility::Rect(RHICommandList& commandList, int x, int y, int width, int height , LinearColor const& color )
	{
		float x2 = x + width;
		float y2 = y + height;
		VertexXY_CA_T1 vertices[] =
		{
			{ Vector2(x , y) , color , Vector2(0,0) },
			{ Vector2(x2 , y) , color , Vector2(1,0) },
			{ Vector2(x2 , y2) , color, Vector2(1,1) },
			{ Vector2(x , y2) , color , Vector2(0,1) },
		};

		TRenderRT< RTVF_XY_CA_T2 >::Draw(commandList, EPrimitive::Quad, vertices, 4);
	}

	void DrawUtility::Rect(RHICommandList& commandList, int x, int y, int width, int height)
	{
		float x2 = x + width;
		float y2 = y + height;
		VertexXY_T1 vertices[] =
		{
			{ Vector2(x , y ) , Vector2(0,0) },
			{ Vector2(x2 , y ) , Vector2(1,0) },
			{ Vector2(x2 , y2 ) , Vector2(1,1) },
			{ Vector2(x , y2 ) , Vector2(0,1) },
		};

		TRenderRT< RTVF_XY_T2 >::Draw(commandList, EPrimitive::Quad, vertices, 4);
	}

	void DrawUtility::Rect(RHICommandList& commandList, int width, int height)
	{
		VertexXY_T1 vertices[] =
		{
			{ Vector2(0 , 0) , Vector2(0,0) },
			{ Vector2(width , 0) , Vector2(1,0) },
			{ Vector2(width , height) , Vector2(1,1) },
			{ Vector2(0 , height) , Vector2(0,1) },
		};
		TRenderRT< RTVF_XY_T2 >::Draw(commandList, EPrimitive::Quad, vertices, 4);
	}

	void DrawUtility::ScreenRect(RHICommandList& commandList, EScreenRenderMethod method)
	{
		switch (method)
		{
		case Render::Rect:
			TRenderRT< RTVF_XYZW_T2 >::Draw(commandList, EPrimitive::Quad, GScreenVertices_Rect, 4);
			break;
		case Render::OptimisedTriangle:
			TRenderRT< RTVF_XYZW_T2 >::Draw(commandList, EPrimitive::TriangleList, GScreenVertices_OptimisedTriangle, 3);
			break;
		}
	}

	void DrawUtility::ScreenRect(RHICommandList& commandList, int with, int height)
	{
		VertexXYZW_T1 screenVertices[] =
		{
			{ Vector4(-1 , -1 , 0 , 1) , Vector2(0,0) },
			{ Vector4(1 , -1 , 0 , 1) , Vector2(with,0) },
			{ Vector4(1, 1 , 0 , 1) , Vector2(with,height) },
			{ Vector4(-1, 1, 0 , 1) , Vector2(0,height) },
		};
		TRenderRT< RTVF_XYZW_T2 >::Draw(commandList, EPrimitive::Quad, screenVertices, 4);
	}

	void DrawUtility::Sprite(RHICommandList& commandList, Vector2 const& pos, Vector2 const& size, Vector2 const& pivot)
	{
		Sprite(commandList, pos, size, pivot, Vector2(0, 0), Vector2(1, 1));
	}

	void DrawUtility::Sprite(RHICommandList& commandList, Vector2 const& pos, Vector2 const& size, Vector2 const& pivot, IntVector2 const& framePos, IntVector2 const& frameDim)
	{
		Vector2 dtex = Vector2(1.0 / frameDim.x, 1.0 / frameDim.y);
		Vector2 texLT = Vector2(framePos).mul(dtex);

		Sprite(commandList, pos, size, pivot, texLT, dtex);
	}

	void DrawUtility::Sprite(RHICommandList& commandList, Vector2 const& pos, Vector2 const& size, Vector2 const& pivot, Vector2 const& texPos, Vector2 const& texSize)
	{
		Vector2 posLT = pos - size.mul(pivot);
		Vector2 posRB = posLT + size;

		Vector2 texLT = texPos;
		Vector2 texRB = texLT + texSize;

		VertexXY_T1 vertices[4] =
		{
			{ posLT , texLT },
			{ Vector2(posRB.x, posLT.y) , Vector2(texRB.x, texLT.y) },
			{ posRB , texRB },
			{ Vector2(posLT.x, posRB.y) , Vector2(texLT.x, texRB.y) },
		};

		TRenderRT< RTVF_XY_T2 >::Draw(commandList, EPrimitive::Quad, vertices, 4);
	}

	void DrawUtility::Sprite(RHICommandList& commandList, Vector2 const& pos, Vector2 const& size, Vector2 const& pivot, LinearColor const& color)
	{
		Sprite(commandList, pos, size, pivot, color, Vector2(0, 0), Vector2(1, 1));
	}

	void DrawUtility::Sprite(RHICommandList& commandList, Vector2 const& pos, Vector2 const& size, Vector2 const& pivot, LinearColor const& color, IntVector2 const& framePos, IntVector2 const& frameDim)
	{
		Vector2 dtex = Vector2(1.0 / frameDim.x, 1.0 / frameDim.y);
		Vector2 texLT = Vector2(framePos).mul(dtex);

		Sprite(commandList, pos, size, pivot, color, texLT, dtex);
	}

	void DrawUtility::Sprite(RHICommandList& commandList, Vector2 const& pos, Vector2 const& size, Vector2 const& pivot, LinearColor const& color, Vector2 const& texPos, Vector2 const& texSize)
	{
		Vector2 posLT = pos - size.mul(pivot);
		Vector2 posRB = posLT + size;

		Vector2 texLT = texPos;
		Vector2 texRB = texLT + texSize;

		VertexXY_CA_T1 vertices[4] =
		{
			{ posLT , color ,texLT },
			{ Vector2(posRB.x, posLT.y) , color ,Vector2(texRB.x, texLT.y) },
			{ posRB , color ,texRB },
			{ Vector2(posLT.x, posRB.y) , color , Vector2(texLT.x, texRB.y) },
		};

		TRenderRT< RTVF_XY_CA_T2 >::Draw(commandList, EPrimitive::Quad, vertices, 4);
	}

	void DrawUtility::DrawTexture(RHICommandList& commandList, RHITexture2D& texture, IntVector2 const& pos, IntVector2 const& size, LinearColor const& color)
	{
		glEnable(GL_TEXTURE_2D);
		{
			GL_BIND_LOCK_OBJECT(texture);
			DrawUtility::Rect(commandList, pos.x, pos.y, size.x, size.y, color);
		}
		glDisable(GL_TEXTURE_2D);
	}

	void DrawUtility::DrawTexture(RHICommandList& commandList, RHITexture2D& texture, RHISamplerState& sampler, IntVector2 const& pos, IntVector2 const& size, LinearColor const& color)
	{
		glEnable(GL_TEXTURE_2D);
		{
			GL_BIND_LOCK_OBJECT(texture);
			glBindSampler( 0 , OpenGLCast::GetHandle(sampler) );
			DrawUtility::Rect(commandList, pos.x, pos.y, size.x, size.y, color);
		}
		glDisable(GL_TEXTURE_2D);
	}

	void DrawUtility::DrawCubeTexture(RHICommandList& commandList, RHITextureCube& texCube, IntVector2 const& pos, int length)
	{
		int offset = 10;

		if( length == 0 )
			length = 100;

		struct MyVertex
		{
			Vector2 pos;
			Vector3 uvw;
		};

		static Vector2 const posOffset[] =
		{
			Vector2(1 , 1),
			Vector2(2 , 0),
			Vector2(-1 ,0),
			Vector2(-2 ,0),
			Vector2(1 , 1),
			Vector2(0 ,-2),
		};
		MyVertex vertices[] =
		{
			//x
			{ Vector2(0,0) , Vector3(1,-1,-1) },
			{ Vector2(1,0) , Vector3(1,1,-1) },
			{ Vector2(1,1) , Vector3(1,1,1) },
			{ Vector2(0,1) , Vector3(1,-1,1) },
			//-x
			{ Vector2(0,0) , Vector3(-1,1,-1) },
			{ Vector2(1,0) , Vector3(-1,-1,-1) },
			{ Vector2(1,1) , Vector3(-1,-1,1) },
			{ Vector2(0,1) , Vector3(-1,1,1) },
			//y
			{ Vector2(0,0) , Vector3(1, 1, -1) },
			{ Vector2(1,0) , Vector3(-1, 1, -1) },
			{ Vector2(1,1) , Vector3(-1, 1, 1) },
			{ Vector2(0,1) , Vector3(1, 1, 1) },
			//-y
			{ Vector2(0,0) , Vector3(-1, -1, -1) },
			{ Vector2(1,0) , Vector3(1, -1, -1) },
			{ Vector2(1,1) , Vector3(1, -1, 1) },
			{ Vector2(0,1) , Vector3(-1, -1, 1) },
			//z
			{ Vector2(0,0) , Vector3(1, -1, 1) },
			{ Vector2(1,0) , Vector3(1, 1, 1) },
			{ Vector2(1,1) , Vector3(-1, 1, 1) },
			{ Vector2(0,1) , Vector3(-1, -1, 1) },
			//-z
			{ Vector2(0,0) , Vector3(-1, -1, -1) },
			{ Vector2(1,0) , Vector3(-1, 1, -1) },
			{ Vector2(1,1) , Vector3(1, 1, -1) },
			{ Vector2(0,1) , Vector3(1, -1, -1) },
		};

		Vector2 curPos = pos;
		for( int i = 0; i < 6; ++i )
		{
			curPos += length * posOffset[i];
			MyVertex* v = vertices + 4 * i;
			for( int j = 0; j < 4; ++j )
			{
				v[j].pos = curPos + length * v[j].pos;
			}
		}

		glEnable(GL_TEXTURE_CUBE_MAP);
		{
			GL_BIND_LOCK_OBJECT(texCube);
			glColor3f(1, 1, 1);
			TRenderRT< RTVF_XY | RTVF_TEX_UVW >::Draw(commandList, EPrimitive::Quad, vertices, ARRAY_SIZE(vertices));
		}
		glDisable(GL_TEXTURE_CUBE_MAP);
	}


	class CopyTextureProgram : public GlobalShaderProgram
	{
		DECLARE_SHADER_PROGRAM(CopyTextureProgram, Global)

		static void SetupShaderCompileOption(ShaderCompileOption&) {}
		static char const* GetShaderFileName()
		{
			return "Shader/CopyTexture";
		}
		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] =
			{
				{ Shader::eVertex , SHADER_ENTRY(ScreenVS) },
				{ Shader::ePixel  , SHADER_ENTRY(CopyTexturePS) },
			};
			return entries;
		}
	public:
		void bindParameters(ShaderParameterMap const& parameterMap)
		{
			mParamCopyTexture.bind(parameterMap, SHADER_PARAM(CopyTexture));
		}

		void setParameters(RHICommandList& commandList, RHITexture2D& copyTexture)
		{
			setTexture(commandList, mParamCopyTexture, copyTexture);
		}

		ShaderParameter mParamCopyTexture;

	};


	class CopyTextureMaskProgram : public GlobalShaderProgram
	{
		DECLARE_SHADER_PROGRAM(CopyTextureMaskProgram, Global)
		static void SetupShaderCompileOption(ShaderCompileOption&) {}
		static char const* GetShaderFileName()
		{
			return "Shader/CopyTexture";
		}
		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] =
			{
				{ Shader::eVertex , SHADER_ENTRY(ScreenVS) },
				{ Shader::ePixel  , SHADER_ENTRY(CopyTextureMaskPS) },
			};
			return entries;
		}
	public:
		void bindParameters(ShaderParameterMap const& parameterMap)
		{
			mParamCopyTexture.bind(parameterMap, SHADER_PARAM(CopyTexture));
			mParamColorMask.bind(parameterMap, SHADER_PARAM(ColorMask));
		}
		void setParameters(RHICommandList& commandList, RHITexture2D& copyTexture, Vector4 const& colorMask)
		{
			setTexture(commandList, mParamCopyTexture, copyTexture);
			setParam(commandList, mParamColorMask, colorMask);
		}

		ShaderParameter mParamColorMask;
		ShaderParameter mParamCopyTexture;
	};


	class CopyTextureBiasProgram : public GlobalShaderProgram
	{
		DECLARE_SHADER_PROGRAM(CopyTextureBiasProgram, Global)

		static void SetupShaderCompileOption(ShaderCompileOption&) {}
		static char const* GetShaderFileName()
		{
			return "Shader/CopyTexture";
		}
		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] =
			{
				{ Shader::eVertex , SHADER_ENTRY(ScreenVS) },
				{ Shader::ePixel  , SHADER_ENTRY(CopyTextureBaisPS) },
			};
			return entries;
		}
	public:


		void bindParameters(ShaderParameterMap const& parameterMap)
		{
			mParamCopyTexture.bind(parameterMap, SHADER_PARAM(CopyTexture));
			mParamColorBais.bind(parameterMap, SHADER_PARAM(ColorBais));
		}
		void setParameters(RHICommandList& commandList, RHITexture2D& copyTexture, float colorBais[2])
		{
			setTexture(commandList, mParamCopyTexture, copyTexture);
			setParam(commandList, mParamColorBais, Vector2(colorBais[0], colorBais[1]));
		}

		ShaderParameter mParamColorBais;
		ShaderParameter mParamCopyTexture;

	};

	class MappingTextureColorProgram : public GlobalShaderProgram
	{
		DECLARE_SHADER_PROGRAM(MappingTextureColorProgram, Global)

		static void SetupShaderCompileOption(ShaderCompileOption&) {}
		static char const* GetShaderFileName()
		{
			return "Shader/CopyTexture";
		}
		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] =
			{
				{ Shader::eVertex , SHADER_ENTRY(ScreenVS) },
				{ Shader::ePixel  , SHADER_ENTRY(MappingTextureColorPS) },
			};
			return entries;
		}
	public:
		void bindParameters(ShaderParameterMap const& parameterMap)
		{
			mParamCopyTexture.bind(parameterMap, SHADER_PARAM(CopyTexture));
			mParamColorMask.bind(parameterMap, SHADER_PARAM(ColorMask));
			mParamValueFactor.bind(parameterMap, SHADER_PARAM(ValueFactor));
		}

		void setParameters(RHICommandList& commandList, RHITexture2D& copyTexture, Vector4 const& colorMask, float valueFactor[2])
		{
			setTexture(commandList, mParamCopyTexture, copyTexture);
			setParam(commandList, mParamColorMask, colorMask);
			setParam(commandList, mParamValueFactor, Vector2(valueFactor[0], valueFactor[1]));
		}

		ShaderParameter mParamCopyTexture;
		ShaderParameter mParamColorMask;
		ShaderParameter mParamValueFactor;
	};

	IMPLEMENT_SHADER_PROGRAM(CopyTextureProgram);
	IMPLEMENT_SHADER_PROGRAM(CopyTextureMaskProgram);
	IMPLEMENT_SHADER_PROGRAM(CopyTextureBiasProgram);
	IMPLEMENT_SHADER_PROGRAM(MappingTextureColorProgram);


	class SimplePipelineProgram : public GlobalShaderProgram
	{
		DECLARE_SHADER_PROGRAM(SimplePipelineProgram, Global)

		static void SetupShaderCompileOption(ShaderCompileOption&) {}
		static char const* GetShaderFileName()
		{
			return "Shader/SimplePipelineShader";
		}
		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] =
			{
				{ Shader::eVertex , SHADER_ENTRY(MainVS) },
				{ Shader::ePixel  , SHADER_ENTRY(MainPS) },
			};
			return entries;
		}
	public:
		void bindParameters(ShaderParameterMap const& parameterMap)
		{
			mParamTexture.bind(parameterMap, SHADER_PARAM(Texture));
			mParamColor.bind(parameterMap, SHADER_PARAM(Color));
			mParamXForm.bind(parameterMap, SHADER_PARAM(XForm));
		}

		void setParameters(RHICommandList& commandList, Matrix4 const& transform, Vector4 const& color, RHITexture2D* copyTexture )
		{
			setTexture(commandList, mParamTexture, copyTexture ? *copyTexture : *GWhiteTexture2D );
			setParam(commandList, mParamColor, color);
			setParam(commandList, mParamXForm, transform);
		}

		ShaderParameter mParamTexture;
		ShaderParameter mParamColor;
		ShaderParameter mParamXForm;
	};

	IMPLEMENT_SHADER_PROGRAM(SimplePipelineProgram)

	bool ShaderHelper::init()
	{
		TIME_SCOPE("ShaderHelper Init");
		ShaderCompileOption option;
		mProgCopyTexture = ShaderManager::Get().getGlobalShaderT<CopyTextureProgram>(true);
		if( mProgCopyTexture == nullptr )
			return false;
		mProgCopyTextureMask = ShaderManager::Get().getGlobalShaderT<CopyTextureMaskProgram>(true);
		if( mProgCopyTextureMask == nullptr )
			return false;
		mProgCopyTextureBias = ShaderManager::Get().getGlobalShaderT<CopyTextureBiasProgram>(true);
		if( mProgCopyTextureBias == nullptr )
			return false;
		mProgMappingTextureColor = ShaderManager::Get().getGlobalShaderT<MappingTextureColorProgram>(true);
		if( mProgMappingTextureColor == nullptr )
			return false;
		mProgSimplePipeline = ShaderManager::Get().getGlobalShaderT<SimplePipelineProgram>(true);
		if( mProgSimplePipeline == nullptr )
			return false;

		mFrameBuffer = RHICreateFrameBuffer();
		return true;
	}

	void ShaderHelper::clearBuffer(RHICommandList& commandList, RHITexture2D& texture, float clearValue[])
	{
		mFrameBuffer->setTexture(0, texture);
		RHISetFrameBuffer(commandList, mFrameBuffer);
		glClearBufferfv(GL_COLOR, 0, (float const*)clearValue);
		RHISetFrameBuffer(commandList, nullptr);
	}

	void ShaderHelper::clearBuffer(RHICommandList& commandList, RHITexture2D& texture, uint32 clearValue[])
	{
		mFrameBuffer->setTexture(0, texture);
		RHISetFrameBuffer(commandList, mFrameBuffer);
		glClearBufferuiv(GL_COLOR, 0, clearValue);
		RHISetFrameBuffer(commandList, nullptr);
	}

	void ShaderHelper::clearBuffer(RHICommandList& commandList, RHITexture2D& texture, int32 clearValue[])
	{
		mFrameBuffer->setTexture(0, texture);
		RHISetFrameBuffer(commandList, mFrameBuffer);
		glClearBufferiv(GL_COLOR, 0, clearValue);
		RHISetFrameBuffer(commandList, nullptr);
	}


	void ShaderHelper::copyTextureToBuffer(RHICommandList& commandList, RHITexture2D& copyTexture)
	{
		RHISetShaderProgram(commandList, mProgCopyTexture->getRHIResource());
		mProgCopyTexture->setParameters(commandList, copyTexture);
		DrawUtility::ScreenRect(commandList);
		RHISetShaderProgram(commandList, nullptr);
	}

	void ShaderHelper::copyTextureMaskToBuffer(RHICommandList& commandList, RHITexture2D& copyTexture, Vector4 const& colorMask)
	{
		RHISetShaderProgram(commandList, mProgCopyTextureMask->getRHIResource());
		mProgCopyTextureMask->setParameters(commandList, copyTexture, colorMask);
		DrawUtility::ScreenRect(commandList);
		RHISetShaderProgram(commandList, nullptr);
	}

	void ShaderHelper::copyTextureBiasToBuffer(RHICommandList& commandList, RHITexture2D& copyTexture, float colorBais[2])
	{
		RHISetShaderProgram(commandList, mProgCopyTextureBias->getRHIResource());
		mProgCopyTextureBias->setParameters(commandList, copyTexture, colorBais);
		DrawUtility::ScreenRect(commandList);
		RHISetShaderProgram(commandList, nullptr);
	}

	void ShaderHelper::mapTextureColorToBuffer(RHICommandList& commandList, RHITexture2D& copyTexture, Vector4 const& colorMask, float valueFactor[2])
	{
		RHISetShaderProgram(commandList, mProgMappingTextureColor->getRHIResource());
		mProgMappingTextureColor->setParameters(commandList, copyTexture, colorMask, valueFactor);
		DrawUtility::ScreenRect(commandList);
		RHISetShaderProgram(commandList, nullptr);
	}

	void ShaderHelper::copyTexture(RHICommandList& commandList, RHITexture2D& destTexture, RHITexture2D& srcTexture)
	{
		mFrameBuffer->setTexture(0, destTexture);
		RHISetFrameBuffer(commandList, mFrameBuffer);
		copyTextureToBuffer(commandList, srcTexture);
		RHISetFrameBuffer(commandList, nullptr);
	}

	void ShaderHelper::reload()
	{
		ShaderManager::Get().reloadShader(*mProgCopyTexture);
		ShaderManager::Get().reloadShader(*mProgCopyTextureMask);
		ShaderManager::Get().reloadShader(*mProgMappingTextureColor);
		ShaderManager::Get().reloadShader(*mProgCopyTextureBias);
	}

}

