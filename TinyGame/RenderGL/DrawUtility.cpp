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


	void RenderGL::DrawUtiltiy::CubeLine()
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
		RenderRT::Draw< RenderRT::eXYZ >(PrimitiveType::eLineList, v, 4 * 6, sizeof(Vector3));
	}

	void RenderGL::DrawUtiltiy::CubeMesh()
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
		RenderRT::Draw< RenderRT::eXYZ_N >(PrimitiveType::eQuad, v, 4 * 6, 2 * sizeof(Vector3));
	}

	void RenderGL::DrawUtiltiy::AixsLine()
	{
		static Vector3 const v[12] =
		{
			Vector3(0,0,0),Vector3(1,0,0), Vector3(1,0,0),Vector3(1,0,0),
			Vector3(0,0,0),Vector3(0,1,0), Vector3(0,1,0),Vector3(0,1,0),
			Vector3(0,0,0),Vector3(0,0,1), Vector3(0,0,1),Vector3(0,0,1),
		};
		RenderRT::Draw< RenderRT::eXYZ_C >(PrimitiveType::eLineList, v, 6, 2 * sizeof(Vector3));
	}

	void RenderGL::DrawUtiltiy::Rect(int x, int y, int width, int height)
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

		RenderRT::Draw< RenderRT::eXY_T2 >(PrimitiveType::eQuad, vertices, 4);
	}

	void RenderGL::DrawUtiltiy::Rect(int width, int height)
	{
		VertexXY_T1 vertices[] =
		{
			{ Vector2(0 , 0 ) , Vector2(0,0) },
			{ Vector2(width , 0 ) , Vector2(1,0) },
			{ Vector2(width , height ) , Vector2(1,1) },
			{ Vector2(0 , height ) , Vector2(0,1) },
		};
		RenderRT::Draw< RenderRT::eXY_T2 >(PrimitiveType::eQuad, vertices, 4);
	}

	void DrawUtiltiy::ScreenRect()
	{
		RenderRT::Draw< RenderRT::eXYZW_T2 >(PrimitiveType::eQuad, GScreenVertices, 4);
	}

	void DrawUtiltiy::Sprite(Vector2 const& pos, Vector2 const& size, Vector2 const& pivot)
	{
		Sprite( pos, size, pivot, Vector2(0, 0), Vector2(1, 1));
	}

	void DrawUtiltiy::Sprite(Vector2 const& pos, Vector2 const& size, Vector2 const& pivot, Vec2i const& framePos, Vec2i const& frameDim)
	{
		Vector2 dtex = Vector2(1.0 / frameDim.x, 1.0 / frameDim.y);
		Vector2 texLT = Vector2(framePos).mul(dtex);

		Sprite( pos, size, pivot, texLT, dtex);
	}

	void DrawUtiltiy::Sprite(Vector2 const& pos, Vector2 const& size, Vector2 const& pivot, Vector2 const& texPos, Vector2 const& texSize)
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

		RenderRT::Draw< RenderRT::eXY_T2 >(PrimitiveType::eQuad, vertices, 4);


	}

}

