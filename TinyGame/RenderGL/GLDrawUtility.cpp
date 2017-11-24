#include "GLDrawUtility.h"

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
		RenderRT::Draw< RenderRT::eXYZ >(GL_LINES, v, 4 * 6, sizeof(Vector3));
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
		RenderRT::Draw< RenderRT::eXYZ_N >(GL_QUADS, v, 4 * 6, 2 * sizeof(Vector3));
	}

	void RenderGL::DrawUtiltiy::AixsLine()
	{
		static Vector3 const v[12] =
		{
			Vector3(0,0,0),Vector3(1,0,0), Vector3(1,0,0),Vector3(1,0,0),
			Vector3(0,0,0),Vector3(0,1,0), Vector3(0,1,0),Vector3(0,1,0),
			Vector3(0,0,0),Vector3(0,0,1), Vector3(0,0,1),Vector3(0,0,1),
		};
		RenderRT::Draw< RenderRT::eXYZ_C >(GL_LINES, v, 6, 2 * sizeof(Vector3));
	}

	void RenderGL::DrawUtiltiy::Rect(int x, int y, int width, int height)
	{
		float x2 = x + width;
		float y2 = y + height;
		VertexXYZ_T1 vertices[] =
		{
			{ Vector3(x , y , 0) , Vector2(0,0) },
			{ Vector3(x2 , y , 0) , Vector2(1,0) },
			{ Vector3(x2 , y2 , 0) , Vector2(1,1) },
			{ Vector3(x , y2 , 0) , Vector2(0,1) },
		};

		RenderRT::Draw< RenderRT::eXYZ_T2 >(GL_QUADS, vertices, 4);
	}

	void RenderGL::DrawUtiltiy::Rect(int width, int height)
	{
		VertexXYZ_T1 vertices[] =
		{
			{ Vector3(0 , 0 , 0) , Vector2(0,0) },
			{ Vector3(width , 0 , 0) , Vector2(1,0) },
			{ Vector3(width , height , 0) , Vector2(1,1) },
			{ Vector3(0 , height , 0) , Vector2(0,1) },
		};
		RenderRT::Draw< RenderRT::eXYZ_T2 >(GL_QUADS, vertices, 4);
	}

	void DrawUtiltiy::ScreenRect()
	{
		RenderRT::Draw< RenderRT::eXYZW_T2 >(GL_QUADS, GScreenVertices, 4);
	}

}

