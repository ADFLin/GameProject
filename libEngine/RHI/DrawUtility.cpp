#include "DrawUtility.h"

#include "ShaderManager.h"

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

	static const VertexXYZW_T1 GScreenVertices[] =
	{
		{ Vector4(-1 , -1 , 0 , 1) , Vector2(0,0) },
		{ Vector4(1 , -1 , 0 , 1) , Vector2(1,0) },
		{ Vector4(1, 1 , 0 , 1) , Vector2(1,1) },
		{ Vector4(-1, 1, 0 , 1) , Vector2(0,1) },
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
		TRenderRT< RTVF_XYZ >::Draw(commandList, PrimitiveType::LineList, v, 4 * 6, sizeof(Vector3));
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
		TRenderRT< RTVF_XYZ_N >::Draw(commandList, PrimitiveType::Quad, v, 4 * 6, 2 * sizeof(Vector3));
	}

	void DrawUtility::AixsLine(RHICommandList& commandList)
	{
		static Vector3 const v[12] =
		{
			Vector3(0,0,0),Vector3(1,0,0), Vector3(1,0,0),Vector3(1,0,0),
			Vector3(0,0,0),Vector3(0,1,0), Vector3(0,1,0),Vector3(0,1,0),
			Vector3(0,0,0),Vector3(0,0,1), Vector3(0,0,1),Vector3(0,0,1),
		};
		TRenderRT< RTVF_XYZ_C >::Draw(commandList, PrimitiveType::LineList, v, 6, 2 * sizeof(Vector3));
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

		TRenderRT< RTVF_XY_T2 >::Draw(commandList, PrimitiveType::Quad, vertices, 4);
	}

	void Render::DrawUtility::Rect(RHICommandList& commandList, int width, int height)
	{
		VertexXY_T1 vertices[] =
		{
			{ Vector2(0 , 0 ) , Vector2(0,0) },
			{ Vector2(width , 0 ) , Vector2(1,0) },
			{ Vector2(width , height ) , Vector2(1,1) },
			{ Vector2(0 , height ) , Vector2(0,1) },
		};
		TRenderRT< RTVF_XY_T2 >::Draw(commandList, PrimitiveType::Quad, vertices, 4);
	}

	void DrawUtility::RectShader(RHICommandList& commandList, int width, int height)
	{
		VertexXY_T1 vertices[] =
		{
			{ Vector2(0 , 0) , Vector2(0,0) },
			{ Vector2(width , 0) , Vector2(1,0) },
			{ Vector2(width , height) , Vector2(1,1) },
			{ Vector2(0 , height) , Vector2(0,1) },
		};
		TRenderRT< RTVF_XY_T2 >::DrawShader(commandList, PrimitiveType::Quad, vertices, 4);
	}

	void DrawUtility::ScreenRect(RHICommandList& commandList)
	{
		TRenderRT< RTVF_XYZW_T2 >::Draw(commandList, PrimitiveType::Quad, GScreenVertices, 4);
	}

	void DrawUtility::ScreenRectShader(RHICommandList& commandList)
	{
		TRenderRT< RTVF_XYZW_T2 >::DrawShader(commandList, PrimitiveType::Quad, GScreenVertices, 4);
	}

	void DrawUtility::ScreenRectShader(RHICommandList& commandList, int with, int height)
	{
		VertexXYZW_T1 screenVertices[] =
		{
			{ Vector4(-1 , -1 , 0 , 1) , Vector2(0,0) },
			{ Vector4(1 , -1 , 0 , 1) , Vector2(with,0) },
			{ Vector4(1, 1 , 0 , 1) , Vector2(with,height) },
			{ Vector4(-1, 1, 0 , 1) , Vector2(0,height) },
		};
		TRenderRT< RTVF_XYZW_T2 >::DrawShader(commandList, PrimitiveType::Quad, screenVertices, 4);
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

		TRenderRT< RTVF_XY_T2 >::Draw(commandList, PrimitiveType::Quad, vertices, 4);
	}

	void DrawUtility::DrawTexture(RHICommandList& commandList, RHITexture2D& texture, IntVector2 const& pos, IntVector2 const& size, LinearColor const& color)
	{
		glEnable(GL_TEXTURE_2D);
		{
			GL_BIND_LOCK_OBJECT(texture);
			glColor4fv(color);
			DrawUtility::Rect(commandList, pos.x, pos.y, size.x, size.y);
		}
		glDisable(GL_TEXTURE_2D);
	}

	void DrawUtility::DrawTexture(RHICommandList& commandList, RHITexture2D& texture, RHISamplerState& sampler, IntVector2 const& pos, IntVector2 const& size, LinearColor const& color)
	{
		glEnable(GL_TEXTURE_2D);
		{
			GL_BIND_LOCK_OBJECT(texture);
			glBindSampler( 0 , OpenGLCast::GetHandle(sampler) );
			glColor4fv(color);
			DrawUtility::Rect(commandList, pos.x, pos.y, size.x, size.y);
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
			TRenderRT< RTVF_XY | RTVF_TEX_UVW >::Draw(commandList, PrimitiveType::Quad, vertices, ARRAY_SIZE(vertices));
		}
		glDisable(GL_TEXTURE_CUBE_MAP);
	}

	void Font::buildFontImage(int size, HDC hDC)
	{
		HFONT	font;										// Windows Font ID
		HFONT	oldfont;									// Used For Good House Keeping

		base = glGenLists(96);								// Storage For 96 Characters

		int height = -(int)(fabs((float)10 * size *GetDeviceCaps(hDC, LOGPIXELSY) / 72) / 10.0 + 0.5);

		font = CreateFont(
			height,					    // Height Of Font
			0,								// Width Of Font
			0,								// Angle Of Escapement
			0,								// Orientation Angle
			FW_BOLD,						// Font Weight
			FALSE,							// Italic
			FALSE,							// Underline
			FALSE,							// Strikeout
			ANSI_CHARSET,					// Character Set Identifier
			OUT_TT_PRECIS,					// Output Precision
			CLIP_DEFAULT_PRECIS,			// Clipping Precision
			ANTIALIASED_QUALITY,			// Output Quality
			FF_DONTCARE | DEFAULT_PITCH,		// Family And Pitch
			TEXT("²Ó©úÅé"));			    // Font Name

		oldfont = (HFONT)SelectObject(hDC, font);           // Selects The Font We Want
		wglUseFontBitmaps(hDC, 32, 96, base);				// Builds 96 Characters Starting At Character 32
		SelectObject(hDC, oldfont);							// Selects The Font We Want
		DeleteObject(font);									// Delete The Font
	}

	void Font::printf(const char *fmt, ...)
	{
		if( fmt == NULL )									// If There's No Text
			return;											// Do Nothing

		va_list	ap;
		char    text[512];								// Holds Our String

		va_start(ap, fmt);									// Parses The String For Variables
		vsprintf(text, fmt, ap);						// And Converts Symbols To Actual Numbers
		va_end(ap);											// Results Are Stored In Text

		glPushAttrib(GL_LIST_BIT);							// Pushes The Display List Bits
		glListBase(base - 32);								// Sets The Base Character to 32
		glCallLists(strlen(text), GL_UNSIGNED_BYTE, text);	// Draws The Display List Text
		glPopAttrib();										// Pops The Display List Bits
	}


	void Font::print(char const* str)
	{
		if( str == NULL )									// If There's No Text
			return;											// Do Nothing
		glPushAttrib(GL_LIST_BIT);							// Pushes The Display List Bits
		glListBase(base - 32);								// Sets The Base Character to 32
		glCallLists(strlen(str), GL_UNSIGNED_BYTE, str);	// Draws The Display List Text
		glPopAttrib();										// Pops The Display List Bits
	}


	Font::~Font()
	{
		if( base )
			glDeleteLists(base, 96);
	}

	void Font::create(int size, HDC hDC)
	{
		buildFontImage(size, hDC);
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
		void bindParameters(ShaderParameterMap& parameterMap)
		{
			parameterMap.bind(mParamCopyTexture, SHADER_PARAM(CopyTexture));
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
		void bindParameters(ShaderParameterMap& parameterMap)
		{
			parameterMap.bind(mParamCopyTexture, SHADER_PARAM(CopyTexture));
			parameterMap.bind(mParamColorMask, SHADER_PARAM(ColorMask));
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


		void bindParameters(ShaderParameterMap& parameterMap)
		{
			parameterMap.bind(mParamCopyTexture, SHADER_PARAM(CopyTexture));
			parameterMap.bind(mParamColorBais, SHADER_PARAM(ColorBais));
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
		void bindParameters(ShaderParameterMap& parameterMap)
		{
			parameterMap.bind(mParamCopyTexture, SHADER_PARAM(CopyTexture));
			parameterMap.bind(mParamColorMask, SHADER_PARAM(ColorMask));
			parameterMap.bind(mParamValueFactor, SHADER_PARAM(ValueFactor));
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

	bool ShaderHelper::init()
	{
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

		if( !mFrameBuffer.create() )
			return false;

		mFrameBuffer.addTexture(*GWhiteTexture2D);
		return true;
	}

	void ShaderHelper::clearBuffer(RHICommandList& commandList, RHITexture2D& texture, float clearValue[])
	{
		mFrameBuffer.setTexture(0, texture);
		GL_BIND_LOCK_OBJECT(mFrameBuffer);
		glClearBufferfv(GL_COLOR, 0, (float const*)clearValue);
	}

	void ShaderHelper::clearBuffer(RHICommandList& commandList, RHITexture2D& texture, uint32 clearValue[])
	{
		mFrameBuffer.setTexture(0, texture);
		GL_BIND_LOCK_OBJECT(mFrameBuffer);
		glClearBufferuiv(GL_COLOR, 0, clearValue);
	}

	void ShaderHelper::clearBuffer(RHICommandList& commandList, RHITexture2D& texture, int32 clearValue[])
	{
		mFrameBuffer.setTexture(0, texture);
		GL_BIND_LOCK_OBJECT(mFrameBuffer);
		glClearBufferiv(GL_COLOR, 0, clearValue);
	}


	void ShaderHelper::copyTextureToBuffer(RHICommandList& commandList, RHITexture2D& copyTexture)
	{
		RHISetShaderProgram(commandList, mProgCopyTexture->getRHIResource());
		mProgCopyTexture->setParameters(commandList, copyTexture);
		DrawUtility::ScreenRectShader(commandList);
	}

	void ShaderHelper::copyTextureMaskToBuffer(RHICommandList& commandList, RHITexture2D& copyTexture, Vector4 const& colorMask)
	{
		RHISetShaderProgram(commandList, mProgCopyTextureMask->getRHIResource());
		mProgCopyTextureMask->setParameters(commandList, copyTexture, colorMask);
		DrawUtility::ScreenRectShader(commandList);
	}

	void ShaderHelper::copyTextureBiasToBuffer(RHICommandList& commandList, RHITexture2D& copyTexture, float colorBais[2])
	{
		RHISetShaderProgram(commandList, mProgCopyTextureBias->getRHIResource());
		mProgCopyTextureBias->setParameters(commandList, copyTexture, colorBais);
		DrawUtility::ScreenRectShader(commandList);
	}

	void ShaderHelper::mapTextureColorToBuffer(RHICommandList& commandList, RHITexture2D& copyTexture, Vector4 const& colorMask, float valueFactor[2])
	{
		RHISetShaderProgram(commandList, mProgMappingTextureColor->getRHIResource());
		mProgMappingTextureColor->setParameters(commandList, copyTexture, colorMask, valueFactor);
		DrawUtility::ScreenRectShader(commandList);
	}

	void ShaderHelper::copyTexture(RHICommandList& commandList, RHITexture2D& destTexture, RHITexture2D& srcTexture)
	{
		mFrameBuffer.setTexture(0, destTexture);
		mFrameBuffer.bind();
		copyTextureToBuffer(commandList, srcTexture);
		mFrameBuffer.unbind();
	}

	void ShaderHelper::reload()
	{
		ShaderManager::Get().reloadShader(*mProgCopyTexture);
		ShaderManager::Get().reloadShader(*mProgCopyTextureMask);
		ShaderManager::Get().reloadShader(*mProgMappingTextureColor);
		ShaderManager::Get().reloadShader(*mProgCopyTextureBias);
	}

}

