#pragma once
#ifndef DrawUtility_H_AB9DF39A_A67B_496E_A2F9_E6C58BD2572C
#define DrawUtility_H_AB9DF39A_A67B_496E_A2F9_E6C58BD2572C

#include "RHI/RHIGlobalResource.h"
#include "RHI/RHICommand.h"
//#REMOVE_ME
#include "OpenGLCommon.h"
#include "GlobalShader.h"

#ifndef BIT
#define BIT( n ) ( 1 << ( n ) )
#endif

#define USE_SEPARATE_SHADER 0

namespace Render
{
	class RHICommandList;

	enum RenderRTSemantic
	{
		RTS_Position = 0,
		RTS_Color,
		RTS_Normal ,
		RTS_Texcoord ,

		RTS_MAX ,
	};

#define RTS_ELEMENT_MASK 0x7
#define RTS_ELEMENT_BIT_OFFSET 3
#define RTS_ELEMENT( S , SIZE )\
	( uint32( (SIZE) & RTS_ELEMENT_MASK ) << ( RTS_ELEMENT_BIT_OFFSET * S ) )

	static_assert(RTS_ELEMENT_BIT_OFFSET * (RTS_MAX - 1) <= sizeof(uint32) * 8, "RenderRTSemantic Can't Support ");

	enum RenderRTVertexFormat
	{
		RTVF_XY      = RTS_ELEMENT(RTS_Position, 2),
		RTVF_XYZ     = RTS_ELEMENT(RTS_Position, 3),
		RTVF_XYZW    = RTS_ELEMENT(RTS_Position, 4),
		RTVF_C       = RTS_ELEMENT(RTS_Color, 3),
		RTVF_CA      = RTS_ELEMENT(RTS_Color, 4),
		RTVF_N       = RTS_ELEMENT(RTS_Normal, 3),
		RTVF_TEX_UV  = RTS_ELEMENT(RTS_Texcoord, 2),
		RTVF_TEX_UVW = RTS_ELEMENT(RTS_Texcoord, 3),

		RTVF_XYZ_C       = RTVF_XYZ | RTVF_C,
		RTVF_XYZ_C_N     = RTVF_XYZ | RTVF_C | RTVF_N,
		RTVF_XYZ_C_N_T2  = RTVF_XYZ | RTVF_C | RTVF_N | RTVF_TEX_UV,
		RTVF_XYZ_C_T2    = RTVF_XYZ | RTVF_C | RTVF_TEX_UV,

		RTVF_XYZ_CA      = RTVF_XYZ | RTVF_CA,
		RTVF_XYZ_CA_N    = RTVF_XYZ | RTVF_CA | RTVF_N,
		RTVF_XYZ_CA_N_T2 = RTVF_XYZ | RTVF_CA | RTVF_N | RTVF_TEX_UV,
		RTVF_XYZ_CA_T2   = RTVF_XYZ | RTVF_CA | RTVF_TEX_UV,

		RTVF_XYZ_N       = RTVF_XYZ | RTVF_N,
		RTVF_XYZ_N_T2    = RTVF_XYZ | RTVF_N | RTVF_TEX_UV,
		RTVF_XYZ_T2      = RTVF_XYZ | RTVF_TEX_UV,

		RTVF_XYZW_T2     = RTVF_XYZW | RTVF_TEX_UV,

		RTVF_XY_C        = RTVF_XY | RTVF_C,
		RTVF_XY_CA       = RTVF_XY | RTVF_CA ,
		RTVF_XY_T2       = RTVF_XY | RTVF_TEX_UV,
		RTVF_XY_CA_T2    = RTVF_XY | RTVF_CA | RTVF_TEX_UV,
	};

	template< uint32 VF, uint32 SEMANTIC >
	struct VertexElementOffset
	{
		enum { Result = sizeof(float) * (VF & RTS_ELEMENT_MASK) + VertexElementOffset< (VF >> RTS_ELEMENT_BIT_OFFSET), SEMANTIC - 1 >::Result };
	};

	template< uint32 VF >
	struct VertexElementOffset< VF, 0 >
	{
		enum { Result = 0 };
	};


#define USE_SEMANTIC( VF , S ) ( ( VF ) & RTS_ELEMENT( S , RTS_ELEMENT_MASK ) )
#define VERTEX_ELEMENT_SIZE( VF , S ) ( USE_SEMANTIC( VF , S ) >> ( RTS_ELEMENT_BIT_OFFSET * S ) )
#define VETEX_ELEMENT_OFFSET( VF , S ) VertexElementOffset< VF , S >::Result


#define  ENCODE_VECTOR_FORAMT( TYPE , NUM ) (( TYPE << 2 ) | ( NUM - 1 ) )
	template < uint32 VertexFormat , uint32 SkipVertexFormat = 0 >
	inline void SetupRenderRTInputLayoutDesc(int indexStream , InputLayoutDesc& desc )
	{
		if constexpr ( USE_SEMANTIC(VertexFormat, RTS_Position) )
			desc.addElement(indexStream, USE_SEMANTIC(SkipVertexFormat, RTS_Position) ? EVertex::ATTRIBUTE_UNUSED : EVertex::ATTRIBUTE_POSITION, EVertex::GetFormat(CVT_Float, VERTEX_ELEMENT_SIZE(VertexFormat, RTS_Position)), false);
		if constexpr ( USE_SEMANTIC(VertexFormat, RTS_Color) )
			desc.addElement(indexStream, USE_SEMANTIC(SkipVertexFormat, RTS_Color) ? EVertex::ATTRIBUTE_UNUSED : EVertex::ATTRIBUTE_COLOR, EVertex::GetFormat(CVT_Float, VERTEX_ELEMENT_SIZE(VertexFormat, RTS_Color)), false);
		if constexpr ( USE_SEMANTIC(VertexFormat, RTS_Normal) )
			desc.addElement(indexStream, USE_SEMANTIC(SkipVertexFormat, RTS_Normal) ? EVertex::ATTRIBUTE_UNUSED : EVertex::ATTRIBUTE_NORMAL, EVertex::GetFormat(CVT_Float, VERTEX_ELEMENT_SIZE(VertexFormat, RTS_Normal)), false);
		if constexpr ( USE_SEMANTIC(VertexFormat, RTS_Texcoord) )
			desc.addElement(indexStream, USE_SEMANTIC(SkipVertexFormat, RTS_Texcoord) ? EVertex::ATTRIBUTE_UNUSED : EVertex::ATTRIBUTE_TEXCOORD, EVertex::GetFormat(CVT_Float, VERTEX_ELEMENT_SIZE(VertexFormat, RTS_Texcoord)), false);
	}


	template < uint32 VertexFormat0 , uint32 VertexFormat1 = 0 >
	class TStaticRenderRTInputLayout : public StaticRHIResourceT< TStaticRenderRTInputLayout< VertexFormat0, VertexFormat1> , RHIInputLayout >
	{
	public:
		static RHIInputLayoutRef CreateRHI()
		{
			InputLayoutDesc desc;
			SetupRenderRTInputLayoutDesc< VertexFormat0 , VertexFormat1 >(0, desc);
			if constexpr (VertexFormat1)
			{
				SetupRenderRTInputLayoutDesc< VertexFormat1 >(1, desc);
			}
			return RHICreateInputLayout(desc);
		}
	};

	template < uint32 VertexFormat >
	class TRenderRT
	{
	public:
		FORCEINLINE static void Draw(RHICommandList& commandList, EPrimitive type, RHIVertexBuffer& buffer , int numVertices, int vertexStride = GetVertexSize())
		{
			InputStreamInfo inputStream;
			inputStream.buffer = &buffer;
			inputStream.offset = 0;
			inputStream.stride = vertexStride;
			RHISetInputStream(commandList, &TStaticRenderRTInputLayout<VertexFormat>::GetRHI(), &inputStream, 1);
			RHIDrawPrimitive(commandList, type, 0, numVertices);
		}

		FORCEINLINE static void Draw(RHICommandList& commandList, EPrimitive type, void const* vetrices, int numVertices, int vertexStride = GetVertexSize())
		{
			RHISetInputStream(commandList, &TStaticRenderRTInputLayout<VertexFormat>::GetRHI(), nullptr, 0);
			RHIDrawPrimitiveUP(commandList, type, vetrices , numVertices, vertexStride);
		}


		FORCEINLINE static void DrawIndexed(RHICommandList& commandList, EPrimitive type, void const* vetrices, int numVertices, uint32 const* indices, int nIndex, int vertexStride = GetVertexSize())
		{
			RHISetInputStream(commandList, &TStaticRenderRTInputLayout<VertexFormat>::GetRHI(), nullptr, 0);
			RHIDrawIndexedPrimitiveUP(commandList, type, vetrices, numVertices, vertexStride , indices , nIndex );
		}

		FORCEINLINE static void Draw(RHICommandList& commandList, EPrimitive type, void const* vetrices, int numVertices, LinearColor const& color, int vertexStride = GetVertexSize())
		{
			RHISetInputStream(commandList, &TStaticRenderRTInputLayout<VertexFormat , RTVF_CA >::GetRHI(), nullptr, 0);
			VertexDataInfo infos[2];
			infos[0].ptr = vetrices;
			infos[0].size = numVertices * vertexStride;
			infos[0].stride = vertexStride;
			infos[1].ptr = &color;
			infos[1].size = sizeof(LinearColor);
			infos[1].stride = 0;
			RHIDrawPrimitiveUP(commandList, type, numVertices, infos , 2 );
		}

		FORCEINLINE static void DrawIndexed(RHICommandList& commandList, EPrimitive type, void const* vetrices, int numVertices, uint32 const* indices, int nIndex, LinearColor const& color, int vertexStride = GetVertexSize())
		{
			RHISetInputStream(commandList, &TStaticRenderRTInputLayout<VertexFormat, RTVF_CA >::GetRHI(), nullptr, 0);
			VertexDataInfo infos[2];
			infos[0].ptr = vetrices;
			infos[0].size = numVertices * vertexStride;
			infos[0].stride = vertexStride;
			infos[1].ptr = &color;
			infos[1].size = sizeof(LinearColor);
			infos[1].stride = 0;
			RHIDrawIndexedPrimitiveUP( commandList, type, numVertices, infos , 2 , indices , nIndex );
		}

		FORCEINLINE static int GetVertexSize()
		{
			return (int)VertexElementOffset< VertexFormat , RTS_MAX >::Result;
		}
	};


#undef VETEX_ELEMENT_OFFSET
#undef VERTEX_ELEMENT_SIZE
#undef USE_SEMANTIC


	enum class EScreenRenderMethod : uint8
	{
		Rect ,
		OptimisedTriangle ,
	};

	class DrawUtility
	{
	public:
		//draw (0,0,0) - (1,1,1) cube
		static void CubeLine(RHICommandList& commandList);
		//draw (0,0,0) - (1,1,1) cube
		static void CubeMesh(RHICommandList& commandList);

		static void AixsLine(RHICommandList& commandList);

		static void Rect(RHICommandList& commandList, float width, float height);
		static void Rect(RHICommandList& commandList, float x, float y, float width, float height);
		static void Rect(RHICommandList& commandList, float x, float y, float width, float height, LinearColor const& color);
		
		static void ScreenRect(RHICommandList& commandList, EScreenRenderMethod method = EScreenRenderMethod::Rect );
		static void ScreenRect(RHICommandList& commandList, float uSize, float vSize);

		static void Sprite(RHICommandList& commandList, Vector2 const& pos, Vector2 const& size, Vector2 const& pivot);
		static void Sprite(RHICommandList& commandList, Vector2 const& pos, Vector2 const& size, Vector2 const& pivot, Vector2 const& texPos, Vector2 const& texSize, IntVector2 const& framePos, IntVector2 const& frameDim);
		static void Sprite(RHICommandList& commandList, Vector2 const& pos, Vector2 const& size, Vector2 const& pivot, IntVector2 const& framePos, IntVector2 const& frameDim);
		static void Sprite(RHICommandList& commandList, Vector2 const& pos, Vector2 const& size, Vector2 const& pivot, Vector2 const& texPos, Vector2 const& texSize);

		static void Sprite(RHICommandList& commandList, Vector2 const& pos, Vector2 const& size, Vector2 const& pivot, LinearColor const& color);
		static void Sprite(RHICommandList& commandList, Vector2 const& pos, Vector2 const& size, Vector2 const& pivot, LinearColor const& color, IntVector2 const& framePos, IntVector2 const& frameDim);
		static void Sprite(RHICommandList& commandList, Vector2 const& pos, Vector2 const& size, Vector2 const& pivot, LinearColor const& color, Vector2 const& texPos, Vector2 const& texSize);

		static void DrawTexture(RHICommandList& commandList, Matrix4 const& XForm, RHITexture2D& texture, Vector2 const& pos, Vector2 const& size, LinearColor const& color = LinearColor(1, 1, 1, 1));
		static void DrawTexture(RHICommandList& commandList, Matrix4 const& XForm, RHITexture2D& texture, RHISamplerState& sampler , Vector2 const& pos, Vector2 const& size , LinearColor const& color = LinearColor(1,1,1,1));
		static void DrawCubeTexture(RHICommandList& commandList, Matrix4 const& XForm, RHITextureCube& texCube, Vector2 const& pos, float length);

	};

	struct MatrixSaveScope
	{
		MatrixSaveScope(Matrix4 const& projMat, Matrix4 const& viewMat)
		{
			glMatrixMode(GL_PROJECTION);
			glPushMatrix();
			glLoadMatrixf( AdjProjectionMatrixForRHI(projMat) );
			glMatrixMode(GL_MODELVIEW);
			glPushMatrix();
			glLoadMatrixf(viewMat);
		}

		MatrixSaveScope(Matrix4 const& projMat)
		{
			glMatrixMode(GL_PROJECTION);
			glPushMatrix();
			glLoadMatrixf( AdjProjectionMatrixForRHI(projMat) );
			glMatrixMode(GL_MODELVIEW);
			glPushMatrix();
			glLoadIdentity();
		}

		MatrixSaveScope()
		{
			glMatrixMode(GL_PROJECTION);
			glPushMatrix();
			glMatrixMode(GL_MODELVIEW);
			glPushMatrix();
		}

		~MatrixSaveScope()
		{
			glMatrixMode(GL_MODELVIEW);
			glPopMatrix();
			glMatrixMode(GL_PROJECTION);
			glPopMatrix();
			glMatrixMode(GL_MODELVIEW);
		}
	};

	struct ViewportSaveScope
	{
		ViewportSaveScope(RHICommandList& commandList)
			:mCommandList(commandList)
		{
			glGetIntegerv(GL_VIEWPORT, value);
		}
		~ViewportSaveScope()
		{
			RHISetViewport(mCommandList, value[0], value[1], value[2], value[3]);
		}

		int operator[](int idx) const { return value[idx]; }
		ViewportInfo mValue;
		int value[4];
		RHICommandList& mCommandList;
	};

	class ScreenVS : public GlobalShader
	{
		DECLARE_EXPORTED_SHADER(ScreenVS, Global , CORE_API);

		static char const* GetShaderFileName()
		{
			return "Shader/ScreenVertexShader";
		}
	};

	class ShaderHelper
	{
	public:

		CORE_API static ShaderHelper& Get();

		bool init();
		void releaseRHI();

		void copyTextureToBuffer(RHICommandList& commandList, RHITexture2D& copyTexture);
		void copyTextureMaskToBuffer(RHICommandList& commandList, RHITexture2D& copyTexture, Vector4 const& colorMask);
		void copyTextureBiasToBuffer(RHICommandList& commandList, RHITexture2D& copyTexture, float colorBais[2]);
		void mapTextureColorToBuffer(RHICommandList& commandList, RHITexture2D& copyTexture, Vector4 const& colorMask, float valueFactor[2]);
		void copyTexture(RHICommandList& commandList, RHITexture2D& destTexture, RHITexture2D& srcTexture);

		void clearBuffer(RHICommandList& commandList, RHITexture2D& texture, float clearValue[]);
		void clearBuffer(RHICommandList& commandList, RHITexture2D& texture, uint32 clearValue[]);
		void clearBuffer(RHICommandList& commandList, RHITexture2D& texture, int32 clearValue[]);

		void reload();

#if USE_SEPARATE_SHADER
		class ScreenVS*          mScreenVS;
		class CopyTexturePS*     mCopyTexturePS;
		class CopyTextureMaskPS* mCopyTextureMaskPS;
		class CopyTextureBiasPS* mCopyTextureBiasPS;
#else
		class CopyTextureProgram*         mProgCopyTexture;
		class CopyTextureMaskProgram*     mProgCopyTextureMask;
		class CopyTextureBiasProgram*     mProgCopyTextureBias;
#endif
		class MappingTextureColorProgram* mProgMappingTextureColor;


		RHIFrameBufferRef mFrameBuffer;

	};

}//namespace Render

#endif // DrawUtility_H_AB9DF39A_A67B_496E_A2F9_E6C58BD2572C
