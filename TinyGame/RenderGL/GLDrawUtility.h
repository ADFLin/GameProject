#ifndef GLDrawUtility_h__
#define GLDrawUtility_h__

#include "GLUtility.h"

namespace RenderGL
{
	class DrawUtiltiy
	{
	public:
		//draw (0,0,0) - (1,1,1) cube
		static void CubeLine()
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
			RenderRT::draw< RenderRT::eXYZ >( GL_LINES , v , 4 * 6 , sizeof( Vector3 ) );
		}

		//draw (0,0,0) - (1,1,1) cube
		static void CubeMesh()
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
			RenderRT::draw< RenderRT::eXYZ_N >( GL_QUADS , v , 4 * 6 , 2 * sizeof( Vector3 ) );
		}

		static void AixsLine()
		{
			static Vector3 const v[ 12 ] =
			{
				Vector3(0,0,0),Vector3(1,0,0), Vector3(1,0,0),Vector3(1,0,0),
				Vector3(0,0,0),Vector3(0,1,0), Vector3(0,1,0),Vector3(0,1,0),
				Vector3(0,0,0),Vector3(0,0,1), Vector3(0,0,1),Vector3(0,0,1),
			};
			RenderRT::draw< RenderRT::eXYZ_C >( GL_LINES , v , 6 , 2 * sizeof( Vector3 ) );
		}
		static void Rect(int x , int y , int width, int height)
		{
			int x2 = x + width;
			int y2 = y + height;
			glBegin(GL_QUADS);
			glTexCoord2f(0, 0); glVertex2f(x, y);
			glTexCoord2f(1, 0); glVertex2f(x2, y);
			glTexCoord2f(1, 1); glVertex2f(x2, y2);
			glTexCoord2f(0, 1); glVertex2f(x, y2);
			glEnd();
		}
		static void Rect( int width, int height )
		{
			glBegin(GL_QUADS);
			glTexCoord2f(0, 0); glVertex2f(0, 0);
			glTexCoord2f(1, 0); glVertex2f(width, 0);
			glTexCoord2f(1, 1); glVertex2f(width, height);
			glTexCoord2f(0, 1); glVertex2f(0, height);
			glEnd();
		}
		static void ScreenRect()
		{
			glBegin(GL_QUADS);
			glTexCoord2f(0, 0); glVertex4f(-1, -1, 0, 1);
			glTexCoord2f(1, 0); glVertex4f(1, -1, 0, 1);
			glTexCoord2f(1, 1); glVertex4f(1, 1, 0, 1);
			glTexCoord2f(0, 1); glVertex4f(-1, 1, 0, 1);
			glEnd();
		}

	};



}//namespace GL

#endif // GLDrawUtility_h__
