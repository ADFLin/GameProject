#include "DrawUtility.h"

namespace RenderGL
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


	void RenderGL::DrawUtility::CubeLine()
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
		TRenderRT< RTVF_XYZ >::Draw(PrimitiveType::LineList, v, 4 * 6, sizeof(Vector3));
	}

	void RenderGL::DrawUtility::CubeMesh()
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
		TRenderRT< RTVF_XYZ_N >::Draw(PrimitiveType::Quad, v, 4 * 6, 2 * sizeof(Vector3));
	}

	void RenderGL::DrawUtility::AixsLine()
	{
		static Vector3 const v[12] =
		{
			Vector3(0,0,0),Vector3(1,0,0), Vector3(1,0,0),Vector3(1,0,0),
			Vector3(0,0,0),Vector3(0,1,0), Vector3(0,1,0),Vector3(0,1,0),
			Vector3(0,0,0),Vector3(0,0,1), Vector3(0,0,1),Vector3(0,0,1),
		};
		TRenderRT< RTVF_XYZ_C >::Draw(PrimitiveType::LineList, v, 6, 2 * sizeof(Vector3));
	}

	void RenderGL::DrawUtility::Rect(int x, int y, int width, int height)
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

		TRenderRT< RTVF_XY_T2 >::Draw(PrimitiveType::Quad, vertices, 4);
	}

	void RenderGL::DrawUtility::Rect(int width, int height)
	{
		VertexXY_T1 vertices[] =
		{
			{ Vector2(0 , 0 ) , Vector2(0,0) },
			{ Vector2(width , 0 ) , Vector2(1,0) },
			{ Vector2(width , height ) , Vector2(1,1) },
			{ Vector2(0 , height ) , Vector2(0,1) },
		};
		TRenderRT< RTVF_XY_T2 >::Draw(PrimitiveType::Quad, vertices, 4);
	}

	void DrawUtility::RectShader(int width, int height)
	{
		VertexXY_T1 vertices[] =
		{
			{ Vector2(0 , 0) , Vector2(0,0) },
			{ Vector2(width , 0) , Vector2(1,0) },
			{ Vector2(width , height) , Vector2(1,1) },
			{ Vector2(0 , height) , Vector2(0,1) },
		};
		TRenderRT< RTVF_XY_T2 >::DrawShader(PrimitiveType::Quad, vertices, 4);
	}

	void DrawUtility::ScreenRect()
	{
		TRenderRT< RTVF_XYZW_T2 >::Draw(PrimitiveType::Quad, GScreenVertices, 4);
	}

	void DrawUtility::ScreenRectShader()
	{
		TRenderRT< RTVF_XYZW_T2 >::DrawShader(PrimitiveType::Quad, GScreenVertices, 4);
	}

	void DrawUtility::Sprite(Vector2 const& pos, Vector2 const& size, Vector2 const& pivot)
	{
		Sprite( pos, size, pivot, Vector2(0, 0), Vector2(1, 1));
	}

	void DrawUtility::Sprite(Vector2 const& pos, Vector2 const& size, Vector2 const& pivot, Vec2i const& framePos, Vec2i const& frameDim)
	{
		Vector2 dtex = Vector2(1.0 / frameDim.x, 1.0 / frameDim.y);
		Vector2 texLT = Vector2(framePos).mul(dtex);

		Sprite( pos, size, pivot, texLT, dtex);
	}

	void DrawUtility::Sprite(Vector2 const& pos, Vector2 const& size, Vector2 const& pivot, Vector2 const& texPos, Vector2 const& texSize)
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

		TRenderRT< RTVF_XY_T2 >::Draw(PrimitiveType::Quad, vertices, 4);
	}

	void DrawUtility::DrawTexture(RHITexture2D& texture, Vec2i const& pos, Vec2i const& size)
	{
		glEnable(GL_TEXTURE_2D);
		{
			GL_BIND_LOCK_OBJECT(texture);
			glColor3f(1, 1, 1);
			DrawUtility::Rect(pos.x, pos.y, size.x, size.y);
		}
		glDisable(GL_TEXTURE_2D);
	}

	void DrawUtility::DrawTexture(RHITexture2D& texture, RHISamplerState& sampler, Vec2i const& pos, Vec2i const& size)
	{
		glEnable(GL_TEXTURE_2D);
		{
			GL_BIND_LOCK_OBJECT(texture);
			glBindSampler( 0 , sampler.mHandle );
			glColor3f(1, 1, 1);
			DrawUtility::Rect(pos.x, pos.y, size.x, size.y);
		}
		glDisable(GL_TEXTURE_2D);
	}

	void DrawUtility::DrawCubeTexture(RHITextureCube& texCube, Vec2i const& pos, int length)
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
			TRenderRT< RTVF_XY | RTVF_TEX_UVW >::Draw(PrimitiveType::Quad, vertices, ARRAY_SIZE(vertices));
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

}

