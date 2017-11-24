#ifndef GLDrawUtility_h__
#define GLDrawUtility_h__

#include "GLUtility.h"

#ifndef BIT
#define BIT( n ) ( 1 << ( n ) )
#endif

namespace RenderGL
{
	class RenderRT
	{
	public:
		enum
		{
			USAGE_XYZ = BIT(0),
			USAGE_W = BIT(1),
			USAGE_N = BIT(2),
			USAGE_C = BIT(3),
			USAGE_TEX_UV = BIT(4),
			USAGE_TEX_W  = BIT(5),
		};

		enum VertexFormat
		{
			eXYZ = USAGE_XYZ,
			eXYZ_C = USAGE_XYZ | USAGE_C,
			eXYZ_N_C = USAGE_XYZ | USAGE_N | USAGE_C,
			eXYZ_N_C_T2 = USAGE_XYZ | USAGE_N | USAGE_C | USAGE_TEX_UV,
			eXYZ_C_T2 = USAGE_XYZ | USAGE_C | USAGE_TEX_UV,
			eXYZ_N = USAGE_XYZ | USAGE_N,
			eXYZ_N_T2 = USAGE_XYZ | USAGE_N | USAGE_TEX_UV,
			eXYZ_T2 = USAGE_XYZ | USAGE_TEX_UV,

			eXYZW_T2 = USAGE_XYZ | USAGE_W | USAGE_TEX_UV,
		};


		template < uint32 VF >
		FORCEINLINE static void Draw(GLuint type, void const* vtx, int nV, int vertexStride)
		{
			bindArray< VF >((float const*)vtx, vertexStride);
			glDrawArrays(type, 0, nV);
			unbindArray< VF >();
		}

		template < uint32 VF >
		FORCEINLINE static void Draw(GLuint type, void const* vtx, int nV)
		{
			Draw< VF >(type, vtx, nV, (int)CountSize< VF >::Result * sizeof(float));
		}

		template< uint32 BIT >
		struct MaskSize { enum { Result = 3 }; };
		template<>
		struct MaskSize< 0 > { enum { Result = 0 }; };
		template<>
		struct MaskSize< USAGE_TEX_UV > { enum { Result = 2 }; };
		template<>
		struct MaskSize< USAGE_W > { enum { Result = 1 }; };
		template<>
		struct MaskSize< USAGE_TEX_W > { enum { Result = 1 }; };

		template< uint32 MASK >
		struct CountSize
		{
			enum { ExtrctBit = MASK & -MASK };
			enum { Result = MaskSize<ExtrctBit>::Result + CountSize< MASK & (~ExtrctBit) >::Result };
		};

		template< >
		struct CountSize< 0 >
		{
			enum { Result = 0 };
		};

		template< uint32 VF >
		FORCEINLINE static void bindArray(float const* v, uint32 vertexStride)
		{

#define VEXTER_OFFSET( VMASK , MASK ) CountSize< VMASK & ((MASK) - 1 )>::Result
			if( VF & USAGE_XYZ )
			{
				glEnableClientState(GL_VERTEX_ARRAY);
				glVertexPointer((VF & USAGE_W) ? 4 : 3, GL_FLOAT, vertexStride, v + VEXTER_OFFSET( VF , USAGE_XYZ));
			}
			if( VF & USAGE_N )
			{
				glEnableClientState(GL_NORMAL_ARRAY);
				glNormalPointer(GL_FLOAT, vertexStride, v + VEXTER_OFFSET(VF , USAGE_N ));
			}
			if( VF & USAGE_C )
			{
				glEnableClientState(GL_COLOR_ARRAY);
				glColorPointer(3, GL_FLOAT, vertexStride, v + VEXTER_OFFSET(VF , USAGE_C));
			}

			if( VF & USAGE_TEX_UV )
			{
				glClientActiveTexture(GL_TEXTURE0);
				glEnableClientState(GL_TEXTURE_COORD_ARRAY);
				glTexCoordPointer(( VF & USAGE_TEX_W )?3:2, GL_FLOAT, vertexStride, v + VEXTER_OFFSET(VF , USAGE_TEX_UV));
			}
#undef VEXTER_OFFSET
		}

		template< uint32 VF >
		FORCEINLINE static void unbindArray()
		{
			if( VF & USAGE_XYZ )
			{
				glDisableClientState(GL_VERTEX_ARRAY);
			}
			if( VF & USAGE_N )
			{
				glDisableClientState(GL_NORMAL_ARRAY);
			}
			if( VF & USAGE_C )
			{
				glDisableClientState(GL_COLOR_ARRAY);

			}
			if( VF & USAGE_TEX_UV )
			{
				glClientActiveTexture(GL_TEXTURE0);
				glDisableClientState(GL_TEXTURE_COORD_ARRAY);
			}
		}
	};

	class DrawUtiltiy
	{
	public:
		//draw (0,0,0) - (1,1,1) cube
		static void CubeLine();
		//draw (0,0,0) - (1,1,1) cube
		static void CubeMesh();

		static void AixsLine();

		static void Rect(int x , int y , int width, int height);
		static void Rect( int width, int height );
		static void ScreenRect();

	};



}//namespace GL

#endif // GLDrawUtility_h__
