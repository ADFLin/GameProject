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
		TRenderRT< RTVF_XYZ >::Draw(PrimitiveType::eLineList, v, 4 * 6, sizeof(Vector3));
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
		TRenderRT< RTVF_XYZ_N >::Draw(PrimitiveType::eQuad, v, 4 * 6, 2 * sizeof(Vector3));
	}

	void RenderGL::DrawUtility::AixsLine()
	{
		static Vector3 const v[12] =
		{
			Vector3(0,0,0),Vector3(1,0,0), Vector3(1,0,0),Vector3(1,0,0),
			Vector3(0,0,0),Vector3(0,1,0), Vector3(0,1,0),Vector3(0,1,0),
			Vector3(0,0,0),Vector3(0,0,1), Vector3(0,0,1),Vector3(0,0,1),
		};
		TRenderRT< RTVF_XYZ_C >::Draw(PrimitiveType::eLineList, v, 6, 2 * sizeof(Vector3));
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

		TRenderRT< RTVF_XY_T2 >::Draw(PrimitiveType::eQuad, vertices, 4);
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
		TRenderRT< RTVF_XY_T2 >::Draw(PrimitiveType::eQuad, vertices, 4);
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
		TRenderRT< RTVF_XY_T2 >::DrawShader(PrimitiveType::eQuad, vertices, 4);
	}

	void DrawUtility::ScreenRect()
	{
		TRenderRT< RTVF_XYZW_T2 >::Draw(PrimitiveType::eQuad, GScreenVertices, 4);
	}

	void DrawUtility::ScreenRectShader()
	{
		TRenderRT< RTVF_XYZW_T2 >::DrawShader(PrimitiveType::eQuad, GScreenVertices, 4);
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

		TRenderRT< RTVF_XY_T2 >::Draw(PrimitiveType::eQuad, vertices, 4);
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
			TRenderRT< RTVF_XY | RTVF_TEX_UVW >::Draw(PrimitiveType::eQuad, vertices, ARRAY_SIZE(vertices));
		}
		glDisable(GL_TEXTURE_CUBE_MAP);
	}

}

