#ifndef GLDrawUtility_h__
#define GLDrawUtility_h__

#include "GLUtility.h"
#include "GLCommon.h"

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

#define RTS_ELEMENT_MASK 0x7
#define RTS_ELEMENT_BIT_OFFSET 3
#define RTS_ELEMENT( S , SIZE )\
	( uint32( (SIZE) & RTS_ELEMENT_MASK ) << ( RTS_ELEMENT_BIT_OFFSET * S ) )

	static_assert(RTS_ELEMENT_BIT_OFFSET * (RTS_MAX - 1) <= sizeof(uint32) * 8, "RenderRTSemantic Can't Support ");

	enum RenderRTElementUsage
	{
		RTEU_XY = RTS_ELEMENT(RTS_Position, 2),
		RTEU_XYZ = RTS_ELEMENT(RTS_Position, 3),
		RTEU_XYZW = RTS_ELEMENT(RTS_Position, 4),
		RTEU_N = RTS_ELEMENT(RTS_Normal, 3),
		RTEU_C = RTS_ELEMENT(RTS_Color, 3),
		RTEU_CA = RTS_ELEMENT(RTS_Color, 4),
		RTEU_TEX_UV = RTS_ELEMENT(RTS_Texcoord, 2),
		RTEU_TEX_UVW = RTS_ELEMENT(RTS_Texcoord, 3),
	};

	enum RenderRTVertexFormat
	{
		RTVF_XYZ = RTEU_XYZ,
		RTVF_XYZ_C = RTEU_XYZ | RTEU_C,
		RTVF_XYZ_N_C = RTEU_XYZ | RTEU_N | RTEU_C,
		RTVF_XYZ_N_C_T2 = RTEU_XYZ | RTEU_N | RTEU_C | RTEU_TEX_UV,
		RTVF_XYZ_C_T2 = RTEU_XYZ | RTEU_C | RTEU_TEX_UV,
		RTVF_XYZ_N = RTEU_XYZ | RTEU_N,
		RTVF_XYZ_N_T2 = RTEU_XYZ | RTEU_N | RTEU_TEX_UV,
		RTVF_XYZ_T2 = RTEU_XYZ | RTEU_TEX_UV,

		RTVF_XYZW_T2 = RTEU_XYZW | RTEU_TEX_UV,
		RTVF_XY = RTEU_XY,
		RTVF_XY_T2 = RTEU_XY | RTEU_TEX_UV,
		RTVF_XY_CA_T2 = RTEU_XY | RTEU_CA | RTEU_TEX_UV,
	};

 
	template < uint32 value >
	class TRenderRTVertexDecl
	{
	};

	template < uint32 VertexFormat >
	class TRenderRT
	{
	public:
		FORCEINLINE static void Draw(PrimitiveType type, void const* vtx, int nV, int vertexStride)
		{
			BindVertexArray((float const*)vtx, vertexStride);
			glDrawArrays( GLConvert::To(type), 0, nV);
			UnbindVertexArray();
		}

		FORCEINLINE static void Draw(PrimitiveType type, void const* vtx, int nV)
		{
			Draw(type, vtx, nV, GetVertexSize());
		}

		FORCEINLINE static void DrawShader(PrimitiveType type, void const* vtx, int nV, int vertexStride)
		{
			BindVertexAttrib((float const*)vtx, vertexStride);
			glDrawArrays(GLConvert::To(type), 0, nV);
			UnbindVertexAttrib();
		}

		FORCEINLINE static void DrawShader(PrimitiveType type, void const* vtx, int nV)
		{
			DrawShader(type, vtx, nV, GetVertexSize());
		}

		FORCEINLINE static int GetVertexSize()
		{
			return (int)VertexElementOffset< VertexFormat , RTS_MAX >::Result * sizeof(float);
		}

	private:
		template< uint32 VF, uint32 SEMANTIC >
		struct VertexElementOffset
		{
			enum { Result = (VF & RTS_ELEMENT_MASK) + VertexElementOffset< (VF >> RTS_ELEMENT_BIT_OFFSET), SEMANTIC - 1 >::Result };
		};

		template< uint32 VF >
		struct VertexElementOffset< VF, 0 >
		{
			enum { Result = 0 };
		};

#define USE_SEMANTIC( S ) ( ( VertexFormat ) & RTS_ELEMENT( S , RTS_ELEMENT_MASK ) )
#define VERTEX_ELEMENT_SIZE( S ) ( USE_SEMANTIC( S ) >> ( RTS_ELEMENT_BIT_OFFSET * S ) )
#define VETEX_ELEMENT_OFFSET( S ) VertexElementOffset< VertexFormat , S >::Result

		FORCEINLINE static void BindVertexArray(float const* v, uint32 vertexStride)
		{
			if( USE_SEMANTIC( RTS_Position) )
			{
				glEnableClientState(GL_VERTEX_ARRAY);
				glVertexPointer( VERTEX_ELEMENT_SIZE(RTS_Position ) , GL_FLOAT, vertexStride, v + VETEX_ELEMENT_OFFSET( RTS_Position ));
			}
			if( USE_SEMANTIC( RTS_Normal) )
			{
				static_assert( !USE_SEMANTIC(RTS_Normal) || VERTEX_ELEMENT_SIZE(RTS_Normal) == 3 , "normal size need equal 3" );
				glEnableClientState(GL_NORMAL_ARRAY);
				glNormalPointer( GL_FLOAT , vertexStride, v + VETEX_ELEMENT_OFFSET(RTS_Normal));
			}
			if( USE_SEMANTIC(RTS_Color) )
			{
				glEnableClientState(GL_COLOR_ARRAY);
				glColorPointer( VERTEX_ELEMENT_SIZE(RTS_Color) , GL_FLOAT, vertexStride, v + VETEX_ELEMENT_OFFSET(RTS_Color));
			}

			if( USE_SEMANTIC( RTS_Texcoord) )
			{
				glClientActiveTexture(GL_TEXTURE0);
				glEnableClientState(GL_TEXTURE_COORD_ARRAY);
				glTexCoordPointer(VERTEX_ELEMENT_SIZE(RTS_Texcoord), GL_FLOAT, vertexStride, v + VETEX_ELEMENT_OFFSET(RTS_Texcoord));
			}
		}

		FORCEINLINE static void UnbindVertexArray()
		{
			if( USE_SEMANTIC(RTS_Position) )
			{
				glDisableClientState(GL_VERTEX_ARRAY);
			}
			if( USE_SEMANTIC(RTS_Normal) )
			{
				glDisableClientState(GL_NORMAL_ARRAY);
			}
			if( USE_SEMANTIC(RTS_Color) )
			{
				glDisableClientState(GL_COLOR_ARRAY);

			}
			if( USE_SEMANTIC(RTS_Texcoord) )
			{
				glClientActiveTexture(GL_TEXTURE0);
				glDisableClientState(GL_TEXTURE_COORD_ARRAY);
			}
		}

		FORCEINLINE static void BindVertexAttrib(float const* v, uint32 vertexStride)
		{
#define VERTEX_ATTRIB_BIND( ATTR , RTS )\
			if( USE_SEMANTIC(RTS ) )\
			{\
				glEnableVertexAttribArray(Vertex::ATTR);\
				glVertexAttribPointer(Vertex::ATTR, VERTEX_ELEMENT_SIZE(RTS), GL_FLOAT, GL_FALSE, vertexStride, v + VETEX_ELEMENT_OFFSET(RTS));\
			}

			VERTEX_ATTRIB_BIND(ATTRIBUTE_POSITION, RTS_Position);
			VERTEX_ATTRIB_BIND(ATTRIBUTE_NORMAL, RTS_Normal);
			VERTEX_ATTRIB_BIND(ATTRIBUTE_COLOR, RTS_Color);
			VERTEX_ATTRIB_BIND(ATTRIBUTE_TEXCOORD, RTS_Texcoord);

#undef VERTEX_ATTRIB_BIND
		}

		FORCEINLINE static void UnbindVertexAttrib()
		{
#define VERTEX_ATTRIB_UNBIND( ATTR , RTS )\
			if( USE_SEMANTIC(RTS) )\
			{\
				glDisableVertexAttribArray(Vertex::ATTR);\
			}

			VERTEX_ATTRIB_UNBIND(ATTRIBUTE_POSITION, RTS_Position);
			VERTEX_ATTRIB_UNBIND(ATTRIBUTE_NORMAL, RTS_Normal);
			VERTEX_ATTRIB_UNBIND(ATTRIBUTE_COLOR, RTS_Color);
			VERTEX_ATTRIB_UNBIND(ATTRIBUTE_TEXCOORD, RTS_Texcoord);

#undef VERTEX_ATTRIB_UNBIND
		}

#undef VETEX_ELEMENT_OFFSET
#undef VERTEX_ELEMENT_SIZE
#undef USE_SEMANTIC

	};

	class DrawUtility
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
		static void ScreenRectShader();

		static void Sprite(Vector2 const& pos, Vector2 const& size, Vector2 const& pivot);
		static void Sprite(Vector2 const& pos, Vector2 const& size, Vector2 const& pivot, Vec2i const& framePos, Vec2i const& frameDim);
		static void Sprite(Vector2 const& pos, Vector2 const& size, Vector2 const& pivot, Vector2 const& texPos, Vector2 const& texSize);

	};



}//namespace GL

#endif // GLDrawUtility_h__
