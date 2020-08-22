#include "DrawUtility.h"

#include "RHI/RHICommand.h"
#include "RHI/RHIGlobalResource.h"

#include "ShaderManager.h"
#include "ProfileSystem.h"
#include "ConsoleSystem.h"

#include "CoreShare.h"

namespace Render
{
	extern CORE_API TConsoleVariable< bool > CVarDrawScreenUseDynamicVertexData;
	extern CORE_API TConsoleVariable< bool > CVarDrawScreenUseOptimisedTriangle;
#if CORE_SHARE_CODE
	CORE_API TConsoleVariable< bool > CVarDrawScreenUseDynamicVertexData(false, "r.DrawScreenUseDynamicVertexData", CVF_TOGGLEABLE);
	CORE_API TConsoleVariable< bool > CVarDrawScreenUseOptimisedTriangle(false, "r.DrawScreenUseOptimisedTriangle", CVF_TOGGLEABLE);
#endif

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


	static const VertexXYZW_T1 GScreenVertices[] =
	{
		//Rect
		{ Vector4(-1 , -1 , 0 , 1) , Vector2(0,0) },
		{ Vector4(1 , -1 , 0 , 1) , Vector2(1,0) },
		{ Vector4(1, 1 , 0 , 1) , Vector2(1,1) },
		{ Vector4(-1, 1, 0 , 1) , Vector2(0,1) },

		//OptimisedTriangle
		{ Vector4(-1 , -1 , 0 , 1) , Vector2(0,0) },
		{ Vector4(3 , -1 , 0 , 1) , Vector2(2,0) },
		{ Vector4(-1, 3 , 0 , 1) , Vector2(0,2) },
	};

	class ScreenRenderResoure : public GlobalRenderResourceBase
	{
	public:
		virtual void restoreRHI() override
		{ 
			mRectVertexBuffer = RHICreateVertexBuffer(sizeof(VertexXYZW_T1), 7, BCF_DefalutValue, const_cast<VertexXYZW_T1*>( &GScreenVertices[0] ));
			uint16 indices[] = { 0, 1 , 2 , 0 , 2 , 3 };
			mQuadIndexBuffer = RHICreateIndexBuffer(6, false, BCF_DefalutValue, indices);

			InputLayoutDesc desc;
			desc.addElement(0, Vertex::ATTRIBUTE_POSITION, Vertex::eFloat4);
			desc.addElement(0, Vertex::ATTRIBUTE_TEXCOORD, Vertex::eFloat2);
			mRectVertexInputLayout = RHICreateInputLayout(desc);
		}


		virtual void releaseRHI() override
		{
			mRectVertexBuffer.release();
			mQuadIndexBuffer.release();
			mRectVertexInputLayout.release();
		}

		void drawRect(RHICommandList& commandList)
		{
			InputStreamInfo inputStream;
			inputStream.buffer = mRectVertexBuffer;
			RHISetInputStream(commandList, mRectVertexInputLayout, &inputStream, 1);

			RHISetIndexBuffer(commandList, mQuadIndexBuffer);
			RHIDrawIndexedPrimitive(commandList, EPrimitive::TriangleList, 0, 6);
		}

		void drawOptimisedTriangle(RHICommandList& commandList)
		{
			InputStreamInfo inputStream;
			inputStream.buffer = mRectVertexBuffer;
			RHISetInputStream(commandList, mRectVertexInputLayout, &inputStream, 1);

			RHISetIndexBuffer(commandList, mQuadIndexBuffer);
			RHIDrawIndexedPrimitive(commandList, EPrimitive::TriangleList, 0, 3, 4);
		}


		RHIInputLayoutRef  mRectVertexInputLayout;
		RHIVertexBufferRef mRectVertexBuffer;
		RHIIndexBufferRef  mQuadIndexBuffer;

	};
	ScreenRenderResoure GScreenRenderResoure;

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

	void DrawUtility::Rect(RHICommandList& commandList, float x, float y, float width, float height , LinearColor const& color )
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

	void DrawUtility::Rect(RHICommandList& commandList, float x, float y, float width, float height)
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

	void DrawUtility::Rect(RHICommandList& commandList, float width, float height)
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
		if ( CVarDrawScreenUseOptimisedTriangle )
			method = EScreenRenderMethod::OptimisedTriangle;
		switch (method)
		{
		case EScreenRenderMethod::Rect:
			if (CVarDrawScreenUseDynamicVertexData)
			{
				TRenderRT< RTVF_XYZW_T2 >::Draw(commandList, EPrimitive::Quad, GScreenVertices, 4);
			}
			else
			{
				GScreenRenderResoure.drawRect(commandList);
			}
			break;
		case EScreenRenderMethod::OptimisedTriangle:
			if (CVarDrawScreenUseDynamicVertexData)
			{
				TRenderRT< RTVF_XYZW_T2 >::Draw(commandList, EPrimitive::TriangleList, GScreenVertices + 4, 3);
			}
			else
			{
				GScreenRenderResoure.drawOptimisedTriangle(commandList);
			}
			break;
		}
	}

	void DrawUtility::ScreenRect(RHICommandList& commandList, float uSize, float vSize)
	{
		VertexXYZW_T1 screenVertices[] =
		{
			{ Vector4(-1 , -1 , 0 , 1) , Vector2(0,0) },
			{ Vector4(1 , -1 , 0 , 1) , Vector2(uSize,0) },
			{ Vector4(1, 1 , 0 , 1) , Vector2(uSize,vSize) },
			{ Vector4(-1, 1, 0 , 1) , Vector2(0,vSize) },
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

	void DrawUtility::Sprite(RHICommandList& commandList, Vector2 const& pos, Vector2 const& size, Vector2 const& pivot, Vector2 const& texPos, Vector2 const& texSize, IntVector2 const& framePos, IntVector2 const& frameDim)
	{
		Vector2 dtex = texSize.div(frameDim);
		Vector2 texLT = texPos + Vector2(framePos).mul(dtex);

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

	void DrawUtility::DrawTexture(RHICommandList& commandList, Matrix4 const& XForm, RHITexture2D& texture, Vector2 const& pos, Vector2 const& size, LinearColor const& color)
	{
		RHISetFixedShaderPipelineState(commandList, XForm, color, &texture);
		DrawUtility::Rect(commandList, pos.x, pos.y, size.x, size.y);
	}

	void DrawUtility::DrawTexture(RHICommandList& commandList, Matrix4 const& XForm, RHITexture2D& texture, RHISamplerState& sampler, Vector2 const& pos, Vector2 const& size, LinearColor const& color)
	{
		RHISetFixedShaderPipelineState(commandList, XForm, color, &texture, &sampler);
		DrawUtility::Rect(commandList, pos.x, pos.y, size.x, size.y);
	}

	void DrawUtility::DrawCubeTexture(RHICommandList& commandList, Matrix4 const& XForm, RHITextureCube& texCube, Vector2 const& pos, float length)
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

		//RHISetFixedShaderPipelineState(commandList, XForm, color, &texture, &sampler);
		//DrawUtility::Rect(commandList, pos.x, pos.y, size.x, size.y);

		glEnable(GL_TEXTURE_CUBE_MAP);
		{
			GL_SCOPED_BIND_OBJECT(texCube);
			glColor3f(1, 1, 1);
			TRenderRT< RTVF_XY | RTVF_TEX_UVW >::Draw(commandList, EPrimitive::Quad, vertices, ARRAY_SIZE(vertices));
		}
		glDisable(GL_TEXTURE_CUBE_MAP);
	}

	IMPLEMENT_SHADER(ScreenVS, EShader::Vertex, SHADER_ENTRY(ScreenVS));


	template< class TShaderType >
	class TCopyTextureBase : public TShaderType
	{
	public:
		static char const* GetShaderFileName()
		{
			return "Shader/CopyTexture";
		}
		void bindParameters(ShaderParameterMap const& parameterMap)
		{
			BIND_TEXTURE_PARAM(parameterMap, CopyTexture);
		}
		void setParameters(RHICommandList& commandList, RHITexture2D& copyTexture)
		{
			RHISamplerState& sampler = TStaticSamplerState< Sampler::eBilinear, Sampler::eClamp, Sampler::eClamp >::GetRHI();
			SET_SHADER_TEXTURE_AND_SAMPLER(commandList, *this , CopyTexture, copyTexture, sampler);
		}
		DEFINE_TEXTURE_PARAM(CopyTexture);
	};

	class CopyTexturePS : public TCopyTextureBase< GlobalShader >
	{
		using BaseClass = TCopyTextureBase< GlobalShader >;
		DECLARE_SHADER(CopyTexturePS, Global)
	};
	IMPLEMENT_SHADER(CopyTexturePS, EShader::Pixel, SHADER_ENTRY(CopyTexturePS));

	class CopyTextureProgram : public TCopyTextureBase< GlobalShaderProgram >
	{
		using BaseClass = TCopyTextureBase< GlobalShaderProgram >;
		DECLARE_SHADER_PROGRAM(CopyTextureProgram, Global)

		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] =
			{
				{ EShader::Vertex , SHADER_ENTRY(ScreenVS) },
				{ EShader::Pixel  , SHADER_ENTRY(CopyTexturePS) },
			};
			return entries;
		}
	};


	class CopyTextureMaskProgram : public TCopyTextureBase< GlobalShaderProgram >
	{
		using BaseClass = TCopyTextureBase< GlobalShaderProgram >;

		DECLARE_SHADER_PROGRAM(CopyTextureMaskProgram, Global)

		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] =
			{
				{ EShader::Vertex , SHADER_ENTRY(ScreenVS) },
				{ EShader::Pixel  , SHADER_ENTRY(CopyTextureMaskPS) },
			};
			return entries;
		}
	public:
		void bindParameters(ShaderParameterMap const& parameterMap)
		{
			BaseClass::bindParameters(parameterMap);
			BIND_SHADER_PARAM(parameterMap, ColorMask);
		}
		void setParameters(RHICommandList& commandList, RHITexture2D& copyTexture, Vector4 const& colorMask)
		{
			BaseClass::setParameters(commandList, copyTexture);
			SET_SHADER_PARAM(commandList, *this, ColorMask, colorMask);
		}
		DEFINE_SHADER_PARAM(ColorMask);
	};

	class CopyTextureMaskPS : public TCopyTextureBase< GlobalShader >
	{
		using BaseClass = TCopyTextureBase< GlobalShader >;
		DECLARE_SHADER(CopyTextureMaskPS, Global);

	public:
		void bindParameters(ShaderParameterMap const& parameterMap)
		{
			BaseClass::bindParameters(parameterMap);
			BIND_SHADER_PARAM(parameterMap, ColorMask);
		}
		void setParameters(RHICommandList& commandList, RHITexture2D& copyTexture, Vector4 const& colorMask, RHISamplerState& sampler = TStaticSamplerState< Sampler::eBilinear, Sampler::eClamp, Sampler::eClamp >::GetRHI())
		{
			BaseClass::setParameters(commandList, copyTexture);
			setParam(commandList, mParamColorMask, colorMask);
		}

		DEFINE_SHADER_PARAM(ColorMask);
	};

	IMPLEMENT_SHADER(CopyTextureMaskPS, EShader::Pixel, SHADER_ENTRY(CopyTextureMaskPS));

	class CopyTextureBiasProgram : public TCopyTextureBase< GlobalShaderProgram >
	{
		using BaseClass = TCopyTextureBase< GlobalShaderProgram >;
		DECLARE_SHADER_PROGRAM(CopyTextureBiasProgram, Global);

		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] =
			{
				{ EShader::Vertex , SHADER_ENTRY(ScreenVS) },
				{ EShader::Pixel  , SHADER_ENTRY(CopyTextureBiasPS) },
			};
			return entries;
		}
	public:


		void bindParameters(ShaderParameterMap const& parameterMap)
		{
			BaseClass::bindParameters(parameterMap);
			BIND_SHADER_PARAM(parameterMap, ColorBais);
		}
		void setParameters(RHICommandList& commandList, RHITexture2D& copyTexture, float colorBais[2], RHISamplerState& sampler = TStaticSamplerState< Sampler::eBilinear, Sampler::eClamp, Sampler::eClamp >::GetRHI())
		{
			BaseClass::setParameters(commandList, copyTexture);
			setParam(commandList, mParamColorBais, Vector2(colorBais[0], colorBais[1]));
		}

		ShaderParameter mParamColorBais;
		ShaderParameter mParamCopyTexture;

	};


	class CopyTextureBiasPS : public TCopyTextureBase< GlobalShader >
	{
		using BaseClass = TCopyTextureBase< GlobalShader >;
		DECLARE_SHADER(CopyTextureBiasPS, Global)

	public:

		void bindParameters(ShaderParameterMap const& parameterMap)
		{
			BaseClass::bindParameters(parameterMap);
			BIND_SHADER_PARAM(parameterMap, ColorBais);
		}
		void setParameters(RHICommandList& commandList, RHITexture2D& copyTexture, float colorBais[2], RHISamplerState& sampler = TStaticSamplerState< Sampler::eBilinear, Sampler::eClamp, Sampler::eClamp >::GetRHI())
		{
			BaseClass::setParameters(commandList, copyTexture);
			SET_SHADER_PARAM(commandList, *this, ColorBais, Vector2(colorBais[0], colorBais[1]));
		}

		DEFINE_SHADER_PARAM(ColorBais);
	};

	IMPLEMENT_SHADER(CopyTextureBiasPS, EShader::Pixel, SHADER_ENTRY(CopyTextureBiasPS));

	class MappingTextureColorProgram : public TCopyTextureBase< GlobalShaderProgram >
	{
		using BaseClass = TCopyTextureBase< GlobalShaderProgram >;
		DECLARE_SHADER_PROGRAM(MappingTextureColorProgram, Global)
		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] =
			{
				{ EShader::Vertex , SHADER_ENTRY(ScreenVS) },
				{ EShader::Pixel  , SHADER_ENTRY(MappingTextureColorPS) },
			};
			return entries;
		}
	public:
		void bindParameters(ShaderParameterMap const& parameterMap)
		{
			BaseClass::bindParameters(parameterMap);
			BIND_SHADER_PARAM(parameterMap, ColorMask);
			BIND_SHADER_PARAM(parameterMap, ValueFactor);
		}

		void setParameters(RHICommandList& commandList, RHITexture2D& copyTexture, Vector4 const& colorMask, float valueFactor[2])
		{
			BaseClass::setParameters(commandList, copyTexture);
			SET_SHADER_PARAM(commandList, *this, ColorMask, colorMask);
			SET_SHADER_PARAM(commandList, *this, ValueFactor, Vector2(valueFactor[0], valueFactor[1]));
		}

		DEFINE_SHADER_PARAM(ColorMask);
		DEFINE_SHADER_PARAM(ValueFactor);
	};

	IMPLEMENT_SHADER_PROGRAM(CopyTextureProgram);
	IMPLEMENT_SHADER_PROGRAM(CopyTextureMaskProgram);
	IMPLEMENT_SHADER_PROGRAM(CopyTextureBiasProgram);
	IMPLEMENT_SHADER_PROGRAM(MappingTextureColorProgram);


	bool ShaderHelper::init()
	{
		TIME_SCOPE("ShaderHelper Init");
#if USE_SEPARATE_SHADER
		VERIFY_RETURN_FALSE(mScreenVS = ShaderManager::Get().getGlobalShaderT<ScreenVS>(true));
		VERIFY_RETURN_FALSE(mCopyTexturePS = ShaderManager::Get().getGlobalShaderT<CopyTexturePS>(true));
		VERIFY_RETURN_FALSE(mCopyTextureMaskPS = ShaderManager::Get().getGlobalShaderT<CopyTextureMaskPS>(true));
		VERIFY_RETURN_FALSE(mCopyTextureBiasPS = ShaderManager::Get().getGlobalShaderT<CopyTextureBiasPS>(true));
#else
		VERIFY_RETURN_FALSE(mProgCopyTexture = ShaderManager::Get().getGlobalShaderT<CopyTextureProgram>(true));
		VERIFY_RETURN_FALSE(mProgCopyTextureMask = ShaderManager::Get().getGlobalShaderT<CopyTextureMaskProgram>(true));
		VERIFY_RETURN_FALSE(mProgCopyTextureBias = ShaderManager::Get().getGlobalShaderT<CopyTextureBiasProgram>(true));
#endif
		
		VERIFY_RETURN_FALSE(mProgMappingTextureColor = ShaderManager::Get().getGlobalShaderT<MappingTextureColorProgram>(true));
		mFrameBuffer = RHICreateFrameBuffer();
		return true;
	}

	void ShaderHelper::releaseRHI()
	{
		mFrameBuffer.release();
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
#if USE_SEPARATE_SHADER
		GraphicShaderBoundState state;
		state.vertexShader = mScreenVS->getRHIResource();
		state.pixelShader = mCopyTexturePS->getRHIResource();
		RHISetGraphicsShaderBoundState(commandList, state);
		mCopyTexturePS->setParameters(commandList, copyTexture);
#else
		RHISetShaderProgram(commandList, mProgCopyTexture->getRHIResource());
		mProgCopyTexture->setParameters(commandList, copyTexture);
#endif

		DrawUtility::ScreenRect(commandList);
		RHISetShaderProgram(commandList, nullptr);
	}

	void ShaderHelper::copyTextureMaskToBuffer(RHICommandList& commandList, RHITexture2D& copyTexture, Vector4 const& colorMask)
	{
#if USE_SEPARATE_SHADER
		GraphicShaderBoundState state;
		state.vertexShader = mScreenVS->getRHIResource();
		state.pixelShader = mCopyTextureMaskPS->getRHIResource();
		RHISetGraphicsShaderBoundState(commandList, state);
		mCopyTextureMaskPS->setParameters(commandList, copyTexture, colorMask);
#else
		RHISetShaderProgram(commandList, mProgCopyTextureMask->getRHIResource());
		mProgCopyTextureMask->setParameters(commandList, copyTexture, colorMask);
#endif

		DrawUtility::ScreenRect(commandList);
		RHISetShaderProgram(commandList, nullptr);
	}

	void ShaderHelper::copyTextureBiasToBuffer(RHICommandList& commandList, RHITexture2D& copyTexture, float colorBais[2])
	{
#if USE_SEPARATE_SHADER
		GraphicShaderBoundState state;
		state.vertexShader = mScreenVS->getRHIResource();
		state.pixelShader = mCopyTextureBiasPS->getRHIResource();
		RHISetGraphicsShaderBoundState(commandList, state);
		mCopyTextureBiasPS->setParameters(commandList, copyTexture, colorBais);
#else
		RHISetShaderProgram(commandList, mProgCopyTextureBias->getRHIResource());
		mProgCopyTextureBias->setParameters(commandList, copyTexture, colorBais);
#endif
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
#if USE_SEPARATE_SHADER
		ShaderManager::Get().reloadShader(*mScreenVS);
		ShaderManager::Get().reloadShader(*mCopyTexturePS);
		ShaderManager::Get().reloadShader(*mCopyTextureMaskPS);
		ShaderManager::Get().reloadShader(*mCopyTextureBiasPS);
#else
		ShaderManager::Get().reloadShader(*mProgCopyTexture);
		ShaderManager::Get().reloadShader(*mProgCopyTextureMask);
		ShaderManager::Get().reloadShader(*mProgCopyTextureBias);
#endif
		ShaderManager::Get().reloadShader(*mProgMappingTextureColor);
	}

}

