#pragma once
#ifndef DrawUtility_H_AB9DF39A_A67B_496E_A2F9_E6C58BD2572C
#define DrawUtility_H_AB9DF39A_A67B_496E_A2F9_E6C58BD2572C

#include "RHI/MeshUtility.h"
#include "RHI/RHIGlobalResource.h"
#include "RHI/RHICommand.h"
//#REMOVE_ME
#include "OpenGLCommon.h"

#include "Singleton.h"
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
		if( USE_SEMANTIC(VertexFormat, RTS_Position) )
			desc.addElement(indexStream, USE_SEMANTIC(SkipVertexFormat, RTS_Position) ? Vertex::ATTRIBUTE_UNUSED : Vertex::ATTRIBUTE_POSITION, Vertex::GetFormat(CVT_Float, VERTEX_ELEMENT_SIZE(VertexFormat, RTS_Position)), false);
		if( USE_SEMANTIC(VertexFormat, RTS_Color) )
			desc.addElement(indexStream, USE_SEMANTIC(SkipVertexFormat, RTS_Color) ? Vertex::ATTRIBUTE_UNUSED : Vertex::ATTRIBUTE_COLOR, Vertex::GetFormat(CVT_Float, VERTEX_ELEMENT_SIZE(VertexFormat, RTS_Color)), false);
		if( USE_SEMANTIC(VertexFormat, RTS_Normal) )
			desc.addElement(indexStream, USE_SEMANTIC(SkipVertexFormat, RTS_Normal) ? Vertex::ATTRIBUTE_UNUSED : Vertex::ATTRIBUTE_NORMAL, Vertex::GetFormat(CVT_Float, VERTEX_ELEMENT_SIZE(VertexFormat, RTS_Normal)), false);
		if( USE_SEMANTIC(VertexFormat, RTS_Texcoord) )
			desc.addElement(indexStream, USE_SEMANTIC(SkipVertexFormat, RTS_Texcoord) ? Vertex::ATTRIBUTE_UNUSED : Vertex::ATTRIBUTE_TEXCOORD, Vertex::GetFormat(CVT_Float, VERTEX_ELEMENT_SIZE(VertexFormat, RTS_Texcoord)), false);
	}


	template < uint32 VertexFormat0 , uint32 VertexFormat1 = 0 >
	class TStaticRenderRTInputLayout : public StaticRHIResourceT< TStaticRenderRTInputLayout< VertexFormat0, VertexFormat1> , RHIInputLayout >
	{
	public:
		static RHIInputLayoutRef CreateRHI()
		{
			InputLayoutDesc desc;
			SetupRenderRTInputLayoutDesc< VertexFormat0 , VertexFormat1 >(0, desc);
			SetupRenderRTInputLayoutDesc< VertexFormat1 >(1, desc);
			return RHICreateInputLayout(desc);
		}
	};

	template < uint32 VertexFormat0 >
	class TStaticRenderRTInputLayout< VertexFormat0 , 0 > : public StaticRHIResourceT< TStaticRenderRTInputLayout< VertexFormat0, 0 >, RHIInputLayout >
	{
	public:
		static RHIInputLayoutRef CreateRHI()
		{
			InputLayoutDesc desc;
			SetupRenderRTInputLayoutDesc< VertexFormat0 >(0, desc);
			return RHICreateInputLayout(desc);
		}
	};

	template < uint32 VertexFormat >
	class TRenderRT
	{
	public:
		FORCEINLINE static void Draw(RHICommandList& commandList, EPrimitive type, RHIVertexBuffer& buffer , int numVertex, int vertexStride = GetVertexSize())
		{
			InputStreamInfo inputStream;
			inputStream.buffer = &buffer;
			inputStream.offset = 0;
			inputStream.stride = vertexStride;
			RHISetInputStream(commandList, &TStaticRenderRTInputLayout<VertexFormat>::GetRHI(), &inputStream, 1);
			RHIDrawPrimitive(commandList, type, 0, numVertex);
		}

		FORCEINLINE static void Draw(RHICommandList& commandList, EPrimitive type, void const* vetrices, int numVertex, int vertexStride = GetVertexSize())
		{
			RHISetInputStream(commandList, &TStaticRenderRTInputLayout<VertexFormat>::GetRHI(), nullptr, 0);
			RHIDrawPrimitiveUP(commandList, type, vetrices , numVertex, vertexStride);
		}


		FORCEINLINE static void DrawIndexed(RHICommandList& commandList, EPrimitive type, void const* vetrices, int numVertex, int const* indices, int nIndex, int vertexStride = GetVertexSize())
		{
			RHISetInputStream(commandList, &TStaticRenderRTInputLayout<VertexFormat>::GetRHI(), nullptr, 0);
			RHIDrawIndexedPrimitiveUP(commandList, type, vetrices, numVertex, vertexStride , indices , nIndex );
		}

		FORCEINLINE static void Draw(RHICommandList& commandList, EPrimitive type, void const* vetrices, int numVertex, LinearColor const& color, int vertexStride = GetVertexSize())
		{
			RHISetInputStream(commandList, &TStaticRenderRTInputLayout<VertexFormat , RTVF_CA >::GetRHI(), nullptr, 0);
			VertexDataInfo infos[2];
			infos[0].ptr = vetrices;
			infos[0].size = numVertex * vertexStride;
			infos[0].stride = vertexStride;
			infos[1].ptr = &color;
			infos[1].size = sizeof(LinearColor);
			infos[1].stride = 0;
			RHIDrawPrimitiveUP(commandList, type, numVertex, infos , 2 );
		}

		FORCEINLINE static void DrawIndexed(RHICommandList& commandList, EPrimitive type, void const* vetrices, int numVertex, int const* indices, int nIndex, LinearColor const& color, int vertexStride = GetVertexSize())
		{
			RHISetInputStream(commandList, &TStaticRenderRTInputLayout<VertexFormat, RTVF_CA >::GetRHI(), nullptr, 0);
			VertexDataInfo infos[2];
			infos[0].ptr = vetrices;
			infos[0].size = numVertex * vertexStride;
			infos[0].stride = vertexStride;
			infos[1].ptr = &color;
			infos[1].size = sizeof(LinearColor);
			infos[1].stride = 0;
			RHIDrawIndexedPrimitiveUP( commandList, type, numVertex, infos , 2 , indices , nIndex );
		}

		FORCEINLINE static int GetVertexSize()
		{
			return (int)VertexElementOffset< VertexFormat , RTS_MAX >::Result;
		}
	};


#undef VETEX_ELEMENT_OFFSET
#undef VERTEX_ELEMENT_SIZE
#undef USE_SEMANTIC


	enum EScreenRenderMethod : uint8
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

		static void Rect(RHICommandList& commandList, int x , int y , int width, int height);
		static void Rect(RHICommandList& commandList, int x, int y, int width, int height, LinearColor const& color);
		static void Rect(RHICommandList& commandList, int width, int height);
		static void ScreenRect(RHICommandList& commandList, EScreenRenderMethod method = EScreenRenderMethod::Rect );
		static void ScreenRect(RHICommandList& commandList, int with, int height);

		static void Sprite(RHICommandList& commandList, Vector2 const& pos, Vector2 const& size, Vector2 const& pivot);
		static void Sprite(RHICommandList& commandList, Vector2 const& pos, Vector2 const& size, Vector2 const& pivot, IntVector2 const& framePos, IntVector2 const& frameDim);
		static void Sprite(RHICommandList& commandList, Vector2 const& pos, Vector2 const& size, Vector2 const& pivot, Vector2 const& texPos, Vector2 const& texSize);

		static void Sprite(RHICommandList& commandList, Vector2 const& pos, Vector2 const& size, Vector2 const& pivot, LinearColor const& color);
		static void Sprite(RHICommandList& commandList, Vector2 const& pos, Vector2 const& size, Vector2 const& pivot, LinearColor const& color, IntVector2 const& framePos, IntVector2 const& frameDim);
		static void Sprite(RHICommandList& commandList, Vector2 const& pos, Vector2 const& size, Vector2 const& pivot, LinearColor const& color, Vector2 const& texPos, Vector2 const& texSize);

		static void DrawTexture(RHICommandList& commandList, RHITexture2D& texture, IntVector2 const& pos, IntVector2 const& size, LinearColor const& color = LinearColor(1, 1, 1, 1));
		static void DrawTexture(RHICommandList& commandList, RHITexture2D& texture, RHISamplerState& sampler , IntVector2 const& pos, IntVector2 const& size , LinearColor const& color = LinearColor(1,1,1,1));
		static void DrawCubeTexture(RHICommandList& commandList, RHITextureCube& texCube, IntVector2 const& pos, int length);

	};


	class SimpleCamera
	{
	public:
		SimpleCamera()
			:mPos(0, 0, 0)
			, mYaw(0)
			, mPitch(0)
			, mRoll(0)
		{
			updateInternal();
		}
		void setPos(Vector3 const& pos) { mPos = pos; }
		void lookAt(Vector3 const& pos, Vector3 const& posTarget, Vector3 const& upDir)
		{
			mPos = pos;
			setViewDir(posTarget - pos, upDir);
		}
		void setViewDir(Vector3 const& forwardDir, Vector3 const& upDir)
		{
			LookAtMatrix mat(forwardDir, upDir);
			mRotation.setMatrix(mat);
			mRotation = mRotation.inverse();
			Vector3 angle = mRotation.getEulerZYX();
			mYaw = angle.z;
			mRoll = angle.y;
			mPitch = angle.x;
		}
		void setRotation(float yaw, float pitch, float roll)
		{
			mYaw = yaw;
			mPitch = pitch;
			mRoll = roll;
			updateInternal();
		}

		Matrix4 getViewMatrix() const { return LookAtMatrix(mPos, getViewDir(), getUpDir()); }
		Matrix4 getTransform() const { return Matrix4(mPos, mRotation); }
		Vector3 const& getPos() const { return mPos; }

		static Vector3 LocalViewDir() { return Vector3(0, 0, -1); }
		static Vector3 LocalUpDir() { return Vector3(0, 1, 0); }

		Vector3 getViewDir() const { return mRotation.rotate(LocalViewDir()); }
		Vector3 getUpDir() const { return mRotation.rotate(LocalUpDir()); }
		Vector3 getRightDir() const { return mRotation.rotate(Vector3(1, 0, 0)); }

		void    moveRight(float dist) { mPos += mRotation.rotate(Vector3(dist, 0, 0)); }
		void    moveFront(float dist) { mPos += dist * getViewDir(); }
		void    moveUp(float dist) { mPos += dist * getUpDir(); }


		Vector3 toWorld(Vector3 const& p) const { return mPos + mRotation.rotate(p); }
		Vector3 toLocal(Vector3 const& p) const { return mRotation.rotateInverse(p - mPos); }

		void    rotateByMouse(float dx, float dy)
		{
			mYaw -= dx;
			mPitch -= dy;
			//float const f = 0.00001;
			//if ( mPitch < f )
			//	mPitch = f;
			//else if ( mPitch > Deg2Rad(180) - f )
			//	mPitch = Deg2Rad(180) - f;
			updateInternal();
		}

	private:
		void updateInternal()
		{
			mRotation.setEulerZYX(mYaw, mRoll, mPitch);
		}

		Quaternion mRotation;
		Vector3 mPos;
		float   mYaw;
		float   mPitch;
		float   mRoll;
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
		DECLARE_SHADER(ScreenVS, Global);

		static char const* GetShaderFileName()
		{
			return "Shader/ScreenVertexShader";
		}
	};

	class ShaderHelper : public SingletonT< ShaderHelper >
	{
	public:
		bool init();

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
