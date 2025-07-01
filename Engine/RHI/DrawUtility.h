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

#define USE_SEPARATE_SHADER 1

namespace Render
{
	class RHICommandList;

	enum RenderRTSemantic
	{
		RTS_Position = 0,
		RTS_Color,
		RTS_Normal ,
		RTS_Texcoord,

		RTS_MAX ,
	};

	enum RenderRTType
	{
		RTT_Float = 0,
		RTT_U8N   = 1,
	};

#define RTS_ELEMENT_COUNT_MASK 0x7
#define RTS_ELEMENT_BIT_OFFSET 4
#define RTS_ELEMENT_( S , SIZE , TYPE )\
	uint32(((((SIZE) & RTS_ELEMENT_COUNT_MASK)<< 1) | TYPE) << (RTS_ELEMENT_BIT_OFFSET * (S)) )


#define RTS_ELEMENT(S , SIZE) RTS_ELEMENT_(S, SIZE , RTT_Float )
#define RTS_ELEMENT_U8N(S , SIZE) RTS_ELEMENT_(S, SIZE , RTT_U8N )

#define USE_SEMANTIC( VF , S )         ( ( VF ) & RTS_ELEMENT_( S , RTS_ELEMENT_COUNT_MASK , 0x1) )
#define VERTEX_ELEMENT_COUNT( VF , S ) ( USE_SEMANTIC( VF , S ) >> ( RTS_ELEMENT_BIT_OFFSET * (S) + 1 ) )
#define VERTEX_ELEMENT_TYPE( VF , S )  ( ( USE_SEMANTIC( VF , S ) >> ( RTS_ELEMENT_BIT_OFFSET * (S) ) ) & 0x1 )

	static_assert(RTS_ELEMENT_BIT_OFFSET * (RTS_MAX - 1) <= sizeof(uint32) * 8, "RenderRTSemantic Can't Support ");

	enum ERenderRTVertexFormat : uint32
	{
		RTVF_XY      = RTS_ELEMENT(RTS_Position, 2),
		RTVF_XYZ     = RTS_ELEMENT(RTS_Position, 3),
		RTVF_XYZW    = RTS_ELEMENT(RTS_Position, 4),
		RTVF_C       = RTS_ELEMENT(RTS_Color, 3),
		RTVF_C8      = RTS_ELEMENT_U8N(RTS_Color, 3),
		RTVF_CA      = RTS_ELEMENT(RTS_Color, 4),
		RTVF_CA8     = RTS_ELEMENT_U8N(RTS_Color, 4),
		RTVF_N       = RTS_ELEMENT(RTS_Normal, 3),
		RTVF_T2      = RTS_ELEMENT(RTS_Texcoord, 2),
		RTVF_T3      = RTS_ELEMENT(RTS_Texcoord, 3),
		RTVF_T4      = RTS_ELEMENT(RTS_Texcoord, 4),

		RTVF_XYZ_C       = RTVF_XYZ | RTVF_C,
		RTVF_XYZ_C8      = RTVF_XYZ | RTVF_C8,
		RTVF_XYZ_C_N     = RTVF_XYZ | RTVF_C | RTVF_N,
		RTVF_XYZ_C_N_T2  = RTVF_XYZ | RTVF_C | RTVF_N | RTVF_T2,
		RTVF_XYZ_C_T2    = RTVF_XYZ | RTVF_C | RTVF_T2,

		RTVF_XYZ_CA      = RTVF_XYZ | RTVF_CA,
		RTVF_XYZ_CA8     = RTVF_XYZ | RTVF_CA8,
		RTVF_XYZ_CA_N    = RTVF_XYZ | RTVF_CA | RTVF_N,
		RTVF_XYZ_CA_N_T2 = RTVF_XYZ | RTVF_CA | RTVF_N | RTVF_T2,
		RTVF_XYZ_CA_T2   = RTVF_XYZ | RTVF_CA | RTVF_T2,

		RTVF_XYZ_N       = RTVF_XYZ | RTVF_N,
		RTVF_XYZ_N_T2    = RTVF_XYZ | RTVF_N | RTVF_T2,
		RTVF_XYZ_T2      = RTVF_XYZ | RTVF_T2,

		RTVF_XYZW_T2     = RTVF_XYZW | RTVF_T2,

		RTVF_XY_C        = RTVF_XY | RTVF_C,
		RTVF_XY_CA       = RTVF_XY | RTVF_CA ,
		RTVF_XY_CA8      = RTVF_XY | RTVF_CA8,
		RTVF_XY_T2       = RTVF_XY | RTVF_T2,
		RTVF_XY_CA_T2    = RTVF_XY | RTVF_CA | RTVF_T2,
		RTVF_XY_CA8_T2   = RTVF_XY | RTVF_CA8 | RTVF_T2,
	};



	template < uint32 VertexFormat , uint32 SkipVertexFormat = 0 >
	inline void SetupRenderRTInputLayoutDesc(int indexStream , InputLayoutDesc& desc )
	{
		auto AddElement = [&desc , indexStream](RenderRTSemantic semantic , int attribute)
		{
			auto type = VERTEX_ELEMENT_TYPE(VertexFormat, semantic) == RTT_Float ? CVT_Float : CVT_UByte;
			bool bNormalized = type != CVT_Float;
			auto format = EVertex::GetFormat(type, VERTEX_ELEMENT_COUNT(VertexFormat, semantic));

			if constexpr (!!SkipVertexFormat)
			{
				desc.addElement(indexStream, (!!USE_SEMANTIC(SkipVertexFormat, semantic)) ? EVertex::ATTRIBUTE_UNUSED : attribute, format, bNormalized);
			}
			else
			{
				desc.addElement(indexStream, attribute, format, bNormalized);
			}
		};

		if constexpr (!!USE_SEMANTIC(VertexFormat, RTS_Position))
			AddElement(RTS_Position, EVertex::ATTRIBUTE_POSITION);
		if constexpr (!!USE_SEMANTIC(VertexFormat, RTS_Color))
			AddElement(RTS_Color, EVertex::ATTRIBUTE_COLOR);
		if constexpr (!!USE_SEMANTIC(VertexFormat, RTS_Normal))
			AddElement(RTS_Normal, EVertex::ATTRIBUTE_NORMAL);
		if constexpr (!!USE_SEMANTIC(VertexFormat, RTS_Texcoord))
			AddElement(RTS_Texcoord, EVertex::ATTRIBUTE_TEXCOORD);
	}

	template< uint32 VF, uint32 SEMANTIC >
	struct TVertexElementOffset
	{
		enum
		{
			ElementSize = (VERTEX_ELEMENT_TYPE(VF, 0) == RTT_Float) ? sizeof(float) : sizeof(uint8),
			Result = ElementSize * VERTEX_ELEMENT_COUNT(VF, 0) + TVertexElementOffset< (VF >> RTS_ELEMENT_BIT_OFFSET), SEMANTIC - 1 >::Result,
		};
	};

	template< uint32 VF >
	struct TVertexElementOffset< VF, 0 >
	{
		enum { Result = 0 };
	};

	template < uint32 VertexFormat0 , uint32 VertexFormat1 = 0 >
	class TStaticRenderRTInputLayout : public StaticRHIResourceT< TStaticRenderRTInputLayout< VertexFormat0, VertexFormat1> , RHIInputLayout >
	{
	public:
		static RHIInputLayoutRef CreateRHI()
		{
			InputLayoutDesc desc = GetSetupValue();
			return RHICreateInputLayout(desc);
		}

		static InputLayoutDesc GetSetupValue()
		{
			InputLayoutDesc desc;
			SetupRenderRTInputLayoutDesc< VertexFormat0, VertexFormat1 >(0, desc);
			if constexpr (!!VertexFormat1)
			{
				SetupRenderRTInputLayoutDesc< VertexFormat1 >(1, desc);
			}
			return desc;
		}

		static uint32 GetHashKey()
		{
			return GetSetupValue().getTypeHash();
		}
	};

	template < uint32 VertexFormat0, uint32 VertexFormat1 = 0 >
	class TStaticRenderRTInputLayoutSkip : public StaticRHIResourceT< TStaticRenderRTInputLayoutSkip< VertexFormat0, VertexFormat1>, RHIInputLayout >
	{
	public:
		static RHIInputLayoutRef CreateRHI()
		{
			InputLayoutDesc desc = GetSetupValue();
			return RHICreateInputLayout(desc);
		}

		static InputLayoutDesc GetSetupValue()
		{
			InputLayoutDesc desc;
			SetupRenderRTInputLayoutDesc< VertexFormat0, VertexFormat1 >(0, desc);
			return desc;
		}

		static uint32 GetHashKey()
		{
			return GetSetupValue().getTypeHash();
		}
	};

	FORCEINLINE void RenderRTVertexFormatTest()
	{
		static_assert(VERTEX_ELEMENT_COUNT(RTVF_CA8, RTS_Color) == 4);
		static_assert(VERTEX_ELEMENT_TYPE(RTVF_CA8, RTS_Color) == RTT_U8N);

		static_assert(TVertexElementOffset< RTVF_CA8, RTS_MAX >::Result == 4);
		static_assert(TVertexElementOffset< RTVF_XY_CA8, RTS_MAX >::Result == 12);
		static_assert(TVertexElementOffset< RTVF_XYZ_C8, RTS_MAX >::Result == 15);
	}

#undef VERTEX_ELEMENT_TYPE
#undef VERTEX_ELEMENT_COUNT
#undef USE_SEMANTIC

	class FRenderRT
	{
	public:
		FORCEINLINE static void Draw(RHICommandList& commandList, RHIInputLayout* inputLayout, EPrimitive type, void const* vetrices, int numVertices, int vertexStride)
		{
			DrawInstanced(commandList, inputLayout, type, vetrices, numVertices, 1, vertexStride);
		}

		FORCEINLINE static void DrawInstanced(RHICommandList& commandList, RHIInputLayout* inputLayout, EPrimitive type, void const* vetrices, int numVertices, uint32 numInstance, int vertexStride)
		{
			RHISetInputStream(commandList, inputLayout, nullptr, 0);
			RHIDrawPrimitiveUP(commandList, type, vetrices, numVertices, vertexStride, numInstance);
		}

		FORCEINLINE static void DrawIndexed(RHICommandList& commandList, RHIInputLayout* inputLayout, EPrimitive type, void const* vetrices, int numVertices, uint32 const* indices, int nIndex, int vertexStride)
		{
			DrawIndexedInstanced(commandList, inputLayout, type, vetrices, numVertices, indices, nIndex, 1, vertexStride);
		}

		FORCEINLINE static void DrawIndexedInstanced(RHICommandList& commandList, RHIInputLayout* inputLayout, EPrimitive type, void const* vetrices, int numVertices, uint32 const* indices, int nIndex, uint32 numInstance, int vertexStride)
		{
			RHISetInputStream(commandList, inputLayout, nullptr, 0);
			RHIDrawIndexedPrimitiveUP(commandList, type, vetrices, numVertices, vertexStride, indices, nIndex, numInstance);
		}

		FORCEINLINE static void Draw(RHICommandList& commandList, RHIInputLayout* inputLayout, EPrimitive type, RHIBuffer& buffer, int numVertices, int vertexStride)
		{
			InputStreamInfo inputStream;
			inputStream.buffer = &buffer;
			inputStream.offset = 0;
			inputStream.stride = vertexStride;
			RHISetInputStream(commandList, inputLayout, &inputStream, 1);
			RHIDrawPrimitive(commandList, type, 0, numVertices);
		}

		FORCEINLINE static void DrawIndexed(RHICommandList& commandList, RHIInputLayout* inputLayout, EPrimitive type, RHIBuffer& vertexbuffer, RHIBuffer& indexbuffer, int nIndex, int vertexStride)
		{
			InputStreamInfo inputStream;
			inputStream.buffer = &vertexbuffer;
			inputStream.offset = 0;
			inputStream.stride = vertexStride;
			RHISetInputStream(commandList, inputLayout, &inputStream, 1);
			RHISetIndexBuffer(commandList, &indexbuffer);
			RHIDrawIndexedPrimitive(commandList, type, 0, nIndex);
		}

	};

	template < uint32 VertexFormat >
	class TRenderRT
	{
	public:

		FORCEINLINE static void Draw(RHICommandList& commandList, EPrimitive type, void const* vetrices, int numVertices, int vertexStride = GetVertexSize())
		{
			FRenderRT::DrawInstanced(commandList, &GetInputLayout(), type, vetrices, numVertices, 1u, vertexStride);
		}

		FORCEINLINE static void DrawInstanced(RHICommandList& commandList, EPrimitive type, void const* vetrices, int numVertices, uint32 numInstance, int vertexStride = GetVertexSize())
		{
			FRenderRT::DrawInstanced(commandList, &GetInputLayout(), type, vetrices, numVertices, numInstance, vertexStride);
		}

		FORCEINLINE static void DrawIndexed(RHICommandList& commandList, EPrimitive type, void const* vetrices, int numVertices, uint32 const* indices, int nIndex, int vertexStride = GetVertexSize())
		{
			FRenderRT::DrawIndexedInstanced(commandList, &GetInputLayout(), type, vetrices, numVertices, indices, nIndex, 1u, vertexStride);
		}

		FORCEINLINE static void DrawIndexedInstanced(RHICommandList& commandList, EPrimitive type, void const* vetrices, int numVertices, uint32 const* indices, int nIndex, uint32 numInstance, int vertexStride = GetVertexSize())
		{
			FRenderRT::DrawIndexedInstanced(commandList, &GetInputLayout(), type, vetrices, numVertices, indices, nIndex, numInstance, vertexStride);
		}

		FORCEINLINE static void Draw(RHICommandList& commandList, EPrimitive type, void const* vetrices, int numVertices, LinearColor const& color, int vertexStride = GetVertexSize())
		{
			DrawInstanced(commandList, type ,vetrices, numVertices, color, 1, vertexStride);
		}

		FORCEINLINE static void DrawInstanced(RHICommandList& commandList, EPrimitive type, void const* vetrices, int numVertices, LinearColor const& color, uint32 numInstance, int vertexStride = GetVertexSize())
		{
			RHISetInputStream(commandList, &TStaticRenderRTInputLayout<VertexFormat, RTVF_CA >::GetRHI(), nullptr, 0);
			VertexDataInfo infos[2];
			infos[0].ptr = vetrices;
			infos[0].size = numVertices * vertexStride;
			infos[0].stride = vertexStride;
			infos[1].ptr = &color;
			infos[1].size = sizeof(LinearColor);
			infos[1].stride = 0;
			RHIDrawPrimitiveUP(commandList, type, numVertices, infos, 2, numInstance);
		}

		FORCEINLINE static void DrawIndexed(RHICommandList& commandList, EPrimitive type, void const* vetrices, int numVertices, uint32 const* indices, int nIndex, LinearColor const& color, int vertexStride = GetVertexSize())
		{
			DrawIndexedInstanced(commandList, type, vetrices, numVertices, indices, nIndex, color, 1, vertexStride);
		}

		FORCEINLINE static void DrawIndexedInstanced(RHICommandList& commandList, EPrimitive type, void const* vetrices, int numVertices, uint32 const* indices, int nIndex, LinearColor const& color, uint32 numInstance, int vertexStride = GetVertexSize())
		{
			RHISetInputStream(commandList, &TStaticRenderRTInputLayout<VertexFormat, RTVF_CA >::GetRHI(), nullptr, 0);
			VertexDataInfo infos[2];
			infos[0].ptr = vetrices;
			infos[0].size = numVertices * vertexStride;
			infos[0].stride = vertexStride;
			infos[1].ptr = &color;
			infos[1].size = sizeof(LinearColor);
			infos[1].stride = 0;
			RHIDrawIndexedPrimitiveUP(commandList, type, numVertices, infos, 2, indices, nIndex, numInstance);
		}

		FORCEINLINE static void Draw(RHICommandList& commandList, EPrimitive type, RHIBuffer& buffer, int numVertices, int vertexStride = GetVertexSize())
		{
			FRenderRT::Draw(commandList, &GetInputLayout(), type, buffer, numVertices, vertexStride);
		}

		FORCEINLINE static void DrawIndexed(RHICommandList& commandList, EPrimitive type, RHIBuffer& vertexbuffer, RHIBuffer& indexbuffer, int nIndex, int vertexStride = GetVertexSize())
		{
			FRenderRT::DrawIndexed(commandList, &GetInputLayout(), type , vertexbuffer , indexbuffer, nIndex, vertexStride);
		}

		FORCEINLINE static int GetVertexSize()
		{
			return (int)TVertexElementOffset< VertexFormat , RTS_MAX >::Result;
		}

		FORCEINLINE static RHIInputLayout& GetInputLayout()
		{
			return TStaticRenderRTInputLayout<VertexFormat>::GetRHI();
		}
	};


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
		static void AixsLine(RHICommandList& commandList, float length);
		static void Rect(RHICommandList& commandList, float width, float height);
		static void Rect(RHICommandList& commandList, float x, float y, float width, float height);
		static void Rect(RHICommandList& commandList, float x, float y, float width, float height, LinearColor const& color);
		
		static void ScreenRect(RHICommandList& commandList, EScreenRenderMethod method = EScreenRenderMethod::Rect );
		static void ScreenRect(RHICommandList& commandList, float uSize, float vSize);
		static void ScreenRect(RHICommandList& commandList, int numInstance, EScreenRenderMethod method = EScreenRenderMethod::Rect);
		static void Sprite(RHICommandList& commandList, Vector2 const& pos, Vector2 const& size, Vector2 const& pivot);
		static void Sprite(RHICommandList& commandList, Vector2 const& pos, Vector2 const& size, Vector2 const& pivot, Vector2 const& texPos, Vector2 const& texSize, IntVector2 const& framePos, IntVector2 const& frameDim);
		static void Sprite(RHICommandList& commandList, Vector2 const& pos, Vector2 const& size, Vector2 const& pivot, IntVector2 const& framePos, IntVector2 const& frameDim);
		static void Sprite(RHICommandList& commandList, Vector2 const& pos, Vector2 const& size, Vector2 const& pivot, Vector2 const& texPos, Vector2 const& texSize);

		static void Sprite(RHICommandList& commandList, Vector2 const& pos, Vector2 const& size, Vector2 const& pivot, LinearColor const& color);
		static void Sprite(RHICommandList& commandList, Vector2 const& pos, Vector2 const& size, Vector2 const& pivot, LinearColor const& color, IntVector2 const& framePos, IntVector2 const& frameDim);
		static void Sprite(RHICommandList& commandList, Vector2 const& pos, Vector2 const& size, Vector2 const& pivot, LinearColor const& color, Vector2 const& texPos, Vector2 const& texSize);

		static void DrawTexture(
			RHICommandList& commandList, Matrix4 const& xForm, 
			RHITexture2D& texture, Vector2 const& pos, Vector2 const& size);
		static void DrawTexture(
			RHICommandList& commandList, Matrix4 const& xForm, 
			RHITexture2D& texture, RHISamplerState& sampler , 
			Vector2 const& pos, Vector2 const& size);
		static void DrawTexture(
			RHICommandList& commandList, Matrix4 const& xForm, 
			RHITexture2D& texture, RHISamplerState& sampler, 
			Vector2 const& pos, Vector2 const& size, 
			LinearColor const* colorMask, Vector3 const* mappingParams = nullptr);

		static void DrawDepthTexture(
			RHICommandList& commandList, Matrix4 const& xForm,
			RHITexture2D& texture, RHISamplerState& sampler,
			Vector2 const& pos, Vector2 const& size, float minDistance, float maxDistance);

		static void DrawCubeTexture(RHICommandList& commandList, Matrix4 const& xForm, RHITextureCube& texCube, Vector2 const& pos, float length);
		static void DrawCubeTexture(RHICommandList& commandList, Matrix4 const& xForm, RHITextureCube& texCube, Vector2 const& pos, Vector2 const& size);

	};

	struct MatrixSaveScope
	{
		MatrixSaveScope(Matrix4 const& projMat, Matrix4 const& viewMat)
		{
			glMatrixMode(GL_PROJECTION);
			glPushMatrix();
			glLoadMatrixf( AdjustProjectionMatrixForRHI(projMat) );
			glMatrixMode(GL_MODELVIEW);
			glPushMatrix();
			glLoadMatrixf(viewMat);
		}

		MatrixSaveScope(Matrix4 const& projMat)
		{
			glMatrixMode(GL_PROJECTION);
			glPushMatrix();
			glLoadMatrixf( AdjustProjectionMatrixForRHI(projMat) );
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
			RHISetViewport(mCommandList, float(value[0]), float(value[1]), float(value[2]), float(value[3]));
		}

		int operator[](int idx) const { return value[idx]; }
		ViewportInfo mValue;
		int value[4];
		RHICommandList& mCommandList;
	};

	class ScreenVS : public GlobalShader
	{
		using BaseClass = GlobalShader;
	public:
		DECLARE_EXPORTED_SHADER(ScreenVS, Global , CORE_API);

		SHADER_PERMUTATION_TYPE_BOOL(UseTexCoord, "USE_SCREEN_TEXCOORD");
		using PermutationDomain = TShaderPermutationDomain<UseTexCoord>;

		static void SetupShaderCompileOption(PermutationDomain const& domain, ShaderCompileOption& option)
		{
			BaseClass::SetupShaderCompileOption(option);
		}
		static char const* GetShaderFileName()
		{
			return "Shader/ScreenVertexShader";
		}
	};

	class ShaderHelper : public IGlobalRenderResource
	{
	public:

		CORE_API static ShaderHelper& Get();

		bool init();

		void copyTextureToBuffer(RHICommandList& commandList, RHITexture2D& copyTexture);
		void copyTextureMaskToBuffer(RHICommandList& commandList, RHITexture2D& copyTexture, Vector4 const& colorMask);
		void copyTextureBiasToBuffer(RHICommandList& commandList, RHITexture2D& copyTexture, float colorBais[2]);
		void mapTextureColorToBuffer(RHICommandList& commandList, RHITexture2D& copyTexture, Vector4 const& colorMask, float valueFactor[2]);
		void copyTexture(RHICommandList& commandList, RHITexture2D& destTexture, RHITexture2D& srcTexture, bool bComputeShader = false);

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
	private:
		void restoreRHI() override;
		void releaseRHI() override;

	};

}//namespace Render

#endif // DrawUtility_H_AB9DF39A_A67B_496E_A2F9_E6C58BD2572C
