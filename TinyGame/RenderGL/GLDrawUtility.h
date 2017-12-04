#ifndef GLDrawUtility_h__
#define GLDrawUtility_h__

#include "GLUtility.h"

#ifndef BIT
#define BIT( n ) ( 1 << ( n ) )
#endif

namespace RenderGL
{
	enum RenderRTSemantic
	{
		RTS_Position = 0,
		RTS_Normal ,
		RTS_Color ,
		RTS_Texcoord ,

		RTS_MAX ,
	};

#define RTS_ELEMENT( S , SIZE )\
	( uint32( (SIZE) & 0x7 ) << ( 3 * S ) )

	template < uint32 value >
	class TRenderRTVertexDecl
	{
	};
	class RenderRT
	{
	public:

		enum 
		{
			USAGE_XY   = RTS_ELEMENT(RTS_Position, 2),
			USAGE_XYZ  = RTS_ELEMENT(RTS_Position, 3),
			USAGE_XYZW = RTS_ELEMENT(RTS_Position, 4),
			USAGE_N    = RTS_ELEMENT(RTS_Normal, 3),
			USAGE_C    = RTS_ELEMENT(RTS_Color, 3),
			USAGE_CA   = RTS_ELEMENT(RTS_Color, 4),
			USAGE_TEX_UV  = RTS_ELEMENT(RTS_Texcoord, 2),
			USAGE_TEX_UVW = RTS_ELEMENT(RTS_Texcoord, 3),
		};

		enum VertexFormat
		{
			eXYZ   = USAGE_XYZ,
			eXYZ_C = USAGE_XYZ | USAGE_C,
			eXYZ_N_C = USAGE_XYZ | USAGE_N | USAGE_C,
			eXYZ_N_C_T2 = USAGE_XYZ | USAGE_N | USAGE_C | USAGE_TEX_UV,
			eXYZ_C_T2 = USAGE_XYZ | USAGE_C | USAGE_TEX_UV,
			eXYZ_N = USAGE_XYZ | USAGE_N,
			eXYZ_N_T2 = USAGE_XYZ | USAGE_N | USAGE_TEX_UV,
			eXYZ_T2 = USAGE_XYZ | USAGE_TEX_UV,

			eXYZW_T2 = USAGE_XYZW | USAGE_TEX_UV,
			eXY_T2 = USAGE_XY | USAGE_TEX_UV,
			eXY_CA_T2 = USAGE_XY | USAGE_CA | USAGE_TEX_UV,
		};


		template < uint32 VF >
		FORCEINLINE static void Draw(PrimitiveType type, void const* vtx, int nV, int vertexStride)
		{
			bindArray< VF >((float const*)vtx, vertexStride);
			glDrawArrays( GLConvert::To(type), 0, nV);
			unbindArray< VF >();
		}

		template < uint32 VF >
		FORCEINLINE static void Draw(PrimitiveType type, void const* vtx, int nV)
		{
			Draw< VF >(type, vtx, nV, (int)VertexElementOffset< VF , RTS_MAX >::Result * sizeof(float));
		}

#define USE_SEMANTIC( VF , S ) ( ( VF ) & RTS_ELEMENT( S , 0x7 ) )
#define VERTEX_ELEMENT_SIZE( VF , S ) ( USE_SEMANTIC( VF , S ) >> ( 3 * S ) )


		template< uint32 VF, uint32 SEMANTIC >
		struct VertexElementOffset
		{
			enum { Result = (VF & 0x7) + VertexElementOffset< (VF >> 3), SEMANTIC - 1 >::Result };
		};

		template< uint32 VF >
		struct VertexElementOffset< VF, 0 >
		{
			enum { Result = 0 };
		};

		template< uint32 VF >
		FORCEINLINE static void bindArray(float const* v, uint32 vertexStride)
		{

#define VETEX_ELEMENT_OFFSET( VF , S ) VertexElementOffset< VF , S >::Result

			if( USE_SEMANTIC( VF , RTS_Position) )
			{
				glEnableClientState(GL_VERTEX_ARRAY);
				glVertexPointer( VERTEX_ELEMENT_SIZE( VF , RTS_Position ) , GL_FLOAT, vertexStride, v + VETEX_ELEMENT_OFFSET( VF , RTS_Position ));
			}
			if( USE_SEMANTIC(VF, RTS_Normal) )
			{
				assert(VERTEX_ELEMENT_SIZE(VF, RTS_Normal) == 3);
				glEnableClientState(GL_NORMAL_ARRAY);
				glNormalPointer( GL_FLOAT , vertexStride, v + VETEX_ELEMENT_OFFSET(VF , RTS_Normal ));
			}
			if( USE_SEMANTIC(VF, RTS_Color) )
			{
				glEnableClientState(GL_COLOR_ARRAY);
				glColorPointer( VERTEX_ELEMENT_SIZE(VF, RTS_Color) , GL_FLOAT, vertexStride, v + VETEX_ELEMENT_OFFSET(VF , RTS_Color));
			}

			if( USE_SEMANTIC(VF, RTS_Texcoord) )
			{
				glClientActiveTexture(GL_TEXTURE0);
				glEnableClientState(GL_TEXTURE_COORD_ARRAY);
				glTexCoordPointer(VERTEX_ELEMENT_SIZE(VF, RTS_Texcoord), GL_FLOAT, vertexStride, v + VETEX_ELEMENT_OFFSET(VF , RTS_Texcoord));
			}


		}


		template< uint32 VF >
		FORCEINLINE static void unbindArray()
		{
			if( USE_SEMANTIC(VF, RTS_Position) )
			{
				glDisableClientState(GL_VERTEX_ARRAY);
			}
			if( USE_SEMANTIC(VF, RTS_Normal) )
			{
				glDisableClientState(GL_NORMAL_ARRAY);
			}
			if( USE_SEMANTIC(VF, RTS_Color) )
			{
				glDisableClientState(GL_COLOR_ARRAY);

			}
			if( USE_SEMANTIC(VF, RTS_Texcoord) )
			{
				glClientActiveTexture(GL_TEXTURE0);
				glDisableClientState(GL_TEXTURE_COORD_ARRAY);
			}
		}
	};
#undef VETEX_ELEMENT_OFFSET
#undef VERTEX_ELEMENT_SIZE
#undef USE_SEMANTIC
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


		static void Sprite(RHITexture2D& texture, Vector2 const& pos, Vector2 const& size, Vector2 const& pivot);
		static void Sprite(RHITexture2D& texture, Vector2 const& pos, Vector2 const& size, Vector2 const& pivot, Vec2i const& framePos, Vec2i const& frameDim);
		static void Sprite(RHITexture2D& texture, Vector2 const& pos, Vector2 const& size, Vector2 const& pivot, Vector2 const& texPos, Vector2 const& texSize);

	};



}//namespace GL

#endif // GLDrawUtility_h__
