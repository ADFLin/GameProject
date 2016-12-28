#ifndef GLDrawUtility_h__
#define GLDrawUtility_h__

#include "GLUtility.h"

namespace GL
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

	};



}//namespace GL

#endif // GLDrawUtility_h__
