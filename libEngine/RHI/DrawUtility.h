#ifndef GLDrawUtility_h__
#define GLDrawUtility_h__

#include "MeshUtility.h"
#include "OpenGLCommon.h"

#include "Singleton.h"

#ifndef BIT
#define BIT( n ) ( 1 << ( n ) )
#endif

namespace Render
{
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
		RTVF_XY = RTS_ELEMENT(RTS_Position, 2),
		RTVF_XYZ = RTS_ELEMENT(RTS_Position, 3),
		RTVF_XYZW = RTS_ELEMENT(RTS_Position, 4),
		RTVF_C = RTS_ELEMENT(RTS_Color, 3),
		RTVF_N = RTS_ELEMENT(RTS_Normal, 3),
		RTVF_CA = RTS_ELEMENT(RTS_Color, 4),
		RTVF_TEX_UV = RTS_ELEMENT(RTS_Texcoord, 2),
		RTVF_TEX_UVW = RTS_ELEMENT(RTS_Texcoord, 3),

		RTVF_XYZ_C = RTVF_XYZ | RTVF_C,
		RTVF_XYZ_C_N = RTVF_XYZ | RTVF_C | RTVF_N,
		RTVF_XYZ_C_N_T2 = RTVF_XYZ | RTVF_C | RTVF_N | RTVF_TEX_UV,
		RTVF_XYZ_C_T2 = RTVF_XYZ | RTVF_C | RTVF_TEX_UV,

		RTVF_XYZ_CA = RTVF_XYZ | RTVF_CA,
		RTVF_XYZ_CA_N = RTVF_XYZ | RTVF_CA | RTVF_N,
		RTVF_XYZ_CA_N_T2 = RTVF_XYZ | RTVF_CA | RTVF_N | RTVF_TEX_UV,
		RTVF_XYZ_CA_T2 = RTVF_XYZ | RTVF_CA | RTVF_TEX_UV,

		RTVF_XYZ_N = RTVF_XYZ | RTVF_N,
		RTVF_XYZ_N_T2 = RTVF_XYZ | RTVF_N | RTVF_TEX_UV,
		RTVF_XYZ_T2 = RTVF_XYZ | RTVF_TEX_UV,

		RTVF_XYZW_T2 = RTVF_XYZW | RTVF_TEX_UV,
		RTVF_XY_T2 = RTVF_XY | RTVF_TEX_UV,
		RTVF_XY_CA_T2 = RTVF_XY | RTVF_CA | RTVF_TEX_UV,
	};

	template < uint32 VertexFormat >
	class TStaticRenderRTInputLayout
	{
	public:
		static InputLayoutDesc const& Get()
		{



		}
	};

	template < uint32 VertexFormat >
	class TRenderRT
	{
	public:
		FORCEINLINE static void Draw(RHICommandList& commandList, PrimitiveType type, RHIVertexBuffer& buffer , int nV, int vertexStride = GetVertexSize())
		{
			BindVertexPointer((uint8 const*)0, vertexStride);
			buffer.bind();
			RHIDrawPrimitive(type, 0, nV);
			buffer.unbind();
			UnbindVertexPointer();
		}

		FORCEINLINE static void Draw(RHICommandList& commandList, PrimitiveType type, RHIVertexBuffer& buffer,  int nV, LinearColor const& color, int vertexStride = GetVertexSize())
		{
			BindVertexPointer((uint8 const*)0, vertexStride ,&color);
			buffer.bind();
			RHIDrawPrimitive(commandList, type, 0, nV);
			buffer.unbind();
			UnbindVertexPointer(&color);
		}

		FORCEINLINE static void DrawShader(RHICommandList& commandList, PrimitiveType type, RHIVertexBuffer& buffer, int nV, int vertexStride = GetVertexSize())
		{
			BindVertexAttrib((uint8 const*)0, vertexStride);
			buffer.bind();
			RHIDrawPrimitive(commandList, type, 0, nV);
			buffer.unbind();
			UnbindVertexAttrib();
		}

		FORCEINLINE static void Draw(RHICommandList& commandList, PrimitiveType type, void const* vetrices, int nV, LinearColor const& color, int vertexStride = GetVertexSize())
		{
			BindVertexPointer((uint8 const*)vetrices, vertexStride , &color );
			RHIDrawPrimitive(commandList, type, 0, nV);
			UnbindVertexPointer(&color);
		}

		FORCEINLINE static void Draw(RHICommandList& commandList, PrimitiveType type, void const* vetrices, int nV, int vertexStride = GetVertexSize())
		{
			BindVertexPointer((uint8 const*)vetrices, vertexStride);
			RHIDrawPrimitive(commandList, type, 0, nV);
			UnbindVertexPointer();
		}

		FORCEINLINE static void DrawShader(RHICommandList& commandList, PrimitiveType type, void const* vetrices, int nV, int vertexStride = GetVertexSize())
		{
			BindVertexAttrib((uint8 const*)vetrices, vertexStride);
			RHIDrawPrimitive(commandList, type, 0, nV);
			UnbindVertexAttrib();
		}

		FORCEINLINE static void DrawIndexed(RHICommandList& commandList, PrimitiveType type, void const* vetrices, int nV, int const* indices, int nIndex, int vertexStride = GetVertexSize())
		{
			BindVertexPointer((uint8 const*)vetrices, vertexStride);
			glDrawElements(GLConvert::To(type), nIndex, GL_UNSIGNED_INT, indices);
			UnbindVertexPointer();
		}

		FORCEINLINE static void DrawIndexed(RHICommandList& commandList, PrimitiveType type, void const* vetrices, int nV, int const* indices, int nIndex, LinearColor const& color , int vertexStride = GetVertexSize())
		{
			BindVertexPointer((uint8 const*)vetrices, vertexStride, &color );
			glDrawElements(GLConvert::To(type), nIndex, GL_UNSIGNED_INT, indices);
			UnbindVertexPointer(&color);
		}

		FORCEINLINE static void DrawIndexedShader(RHICommandList& commandList, PrimitiveType type, void const* vetrices, int nV, int const* indices, int nIndex, int vertexStride = GetVertexSize())
		{
			BindVertexAttrib((uint8 const*)vetrices, vertexStride);
			glDrawElements(GLConvert::To(type), nIndex, GL_UNSIGNED_INT, indices);
			UnbindVertexAttrib();
		}

		FORCEINLINE static int GetVertexSize()
		{
			return (int)VertexElementOffset< VertexFormat , RTS_MAX >::Result;
		}

	//private:
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

#define USE_SEMANTIC( S ) ( ( VertexFormat ) & RTS_ELEMENT( S , RTS_ELEMENT_MASK ) )
#define VERTEX_ELEMENT_SIZE( S ) ( USE_SEMANTIC( S ) >> ( RTS_ELEMENT_BIT_OFFSET * S ) )
#define VETEX_ELEMENT_OFFSET( S ) VertexElementOffset< VertexFormat , S >::Result

		FORCEINLINE static void BindVertexPointer(uint8 const* v, uint32 vertexStride , LinearColor const* overwriteColor = nullptr )
		{
			if( USE_SEMANTIC( RTS_Position) )
			{
				glEnableClientState(GL_VERTEX_ARRAY);
				glVertexPointer( VERTEX_ELEMENT_SIZE(RTS_Position) , GL_FLOAT, vertexStride, v + VETEX_ELEMENT_OFFSET( RTS_Position ));
			}

			if ( overwriteColor )
			{
				glColor4fv(*overwriteColor);
			}
			else if( USE_SEMANTIC(RTS_Color) )
			{
				glEnableClientState(GL_COLOR_ARRAY);
				glColorPointer(VERTEX_ELEMENT_SIZE(RTS_Color), GL_FLOAT, vertexStride, v + VETEX_ELEMENT_OFFSET(RTS_Color));
			}

			if( USE_SEMANTIC( RTS_Normal) )
			{
				static_assert( !USE_SEMANTIC(RTS_Normal) || VERTEX_ELEMENT_SIZE(RTS_Normal) == 3 , "normal size need equal 3" );
				glEnableClientState(GL_NORMAL_ARRAY);
				glNormalPointer( GL_FLOAT , vertexStride, v + VETEX_ELEMENT_OFFSET(RTS_Normal));
			}

			if( USE_SEMANTIC( RTS_Texcoord) )
			{
				glClientActiveTexture(GL_TEXTURE0);
				glEnableClientState(GL_TEXTURE_COORD_ARRAY);
				glTexCoordPointer(VERTEX_ELEMENT_SIZE(RTS_Texcoord), GL_FLOAT, vertexStride, v + VETEX_ELEMENT_OFFSET(RTS_Texcoord));
			}
		}

		FORCEINLINE static void UnbindVertexPointer(LinearColor const* overwriteColor = nullptr )
		{
			if( USE_SEMANTIC(RTS_Position) )
			{
				glDisableClientState(GL_VERTEX_ARRAY);
			}
			if( overwriteColor == nullptr && USE_SEMANTIC(RTS_Color) )
			{
				glDisableClientState(GL_COLOR_ARRAY);
			}
			if( USE_SEMANTIC(RTS_Normal) )
			{
				glDisableClientState(GL_NORMAL_ARRAY);
			}
			if( USE_SEMANTIC(RTS_Texcoord) )
			{
				glClientActiveTexture(GL_TEXTURE0);
				glDisableClientState(GL_TEXTURE_COORD_ARRAY);
			}
		}

		FORCEINLINE static void BindVertexAttrib(uint8 const* v, uint32 vertexStride , LinearColor const* overwriteColor = nullptr )
		{
#define VERTEX_ATTRIB_BIND( ATTR , RTS )\
			if( USE_SEMANTIC(RTS ) )\
			{\
				glEnableVertexAttribArray(Vertex::ATTR);\
				glVertexAttribPointer(Vertex::ATTR, VERTEX_ELEMENT_SIZE(RTS), GL_FLOAT, GL_FALSE, vertexStride, v + VETEX_ELEMENT_OFFSET(RTS));\
			}

			VERTEX_ATTRIB_BIND(ATTRIBUTE_POSITION, RTS_Position);
			if( overwriteColor )
			{
				glVertexAttrib4fv(Vertex::ATTRIBUTE_COLOR, *overwriteColor);
			}
			else
			{
				VERTEX_ATTRIB_BIND(ATTRIBUTE_COLOR, RTS_Color);
			}
			VERTEX_ATTRIB_BIND(ATTRIBUTE_NORMAL, RTS_Normal);		
			VERTEX_ATTRIB_BIND(ATTRIBUTE_TEXCOORD, RTS_Texcoord);

#undef VERTEX_ATTRIB_BIND
		}

		FORCEINLINE static void UnbindVertexAttrib(LinearColor const* overwriteColor = nullptr)
		{
#define VERTEX_ATTRIB_UNBIND( ATTR , RTS )\
			if( USE_SEMANTIC(RTS) )\
			{\
				glDisableVertexAttribArray(Vertex::ATTR);\
			}

			VERTEX_ATTRIB_UNBIND(ATTRIBUTE_POSITION, RTS_Position);
			VERTEX_ATTRIB_UNBIND(ATTRIBUTE_NORMAL, RTS_Normal);
			if ( overwriteColor ){}
			else
			{
				VERTEX_ATTRIB_UNBIND(ATTRIBUTE_COLOR, RTS_Color);
			}
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
		static void CubeLine(RHICommandList& commandList);
		//draw (0,0,0) - (1,1,1) cube
		static void CubeMesh(RHICommandList& commandList);

		static void AixsLine(RHICommandList& commandList);

		static void Rect(RHICommandList& commandList, int x , int y , int width, int height);
		static void Rect(RHICommandList& commandList, int width, int height );
		static void RectShader(RHICommandList& commandList, int width, int height);
		static void ScreenRect(RHICommandList& commandList);
		static void ScreenRectShader(RHICommandList& commandList);
		static void ScreenRectShader(RHICommandList& commandList, int with, int height);

		static void Sprite(RHICommandList& commandList, Vector2 const& pos, Vector2 const& size, Vector2 const& pivot);
		static void Sprite(RHICommandList& commandList, Vector2 const& pos, Vector2 const& size, Vector2 const& pivot, IntVector2 const& framePos, IntVector2 const& frameDim);
		static void Sprite(RHICommandList& commandList, Vector2 const& pos, Vector2 const& size, Vector2 const& pivot, Vector2 const& texPos, Vector2 const& texSize);

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


	class Font
	{
	public:
		Font() { base = 0; }
		explicit Font(int size, HDC hDC) { buildFontImage(size, hDC); }
		~Font();
		void create(int size, HDC hDC);
		void printf(char const* fmt, ...);
		void print(char const* str);
	private:
		void buildFontImage(int size, HDC hDC);
		GLuint	base;
	};


	struct MatrixSaveScope
	{
		MatrixSaveScope(Matrix4 const& projMat, Matrix4 const& viewMat)
		{
			glMatrixMode(GL_PROJECTION);
			glPushMatrix();
			glLoadMatrixf(projMat);
			glMatrixMode(GL_MODELVIEW);
			glPushMatrix();
			glLoadMatrixf(viewMat);
		}

		MatrixSaveScope(Matrix4 const& projMat)
		{
			glMatrixMode(GL_PROJECTION);
			glPushMatrix();
			glLoadMatrixf(projMat);
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
		int value[4];
		RHICommandList& mCommandList;
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

		class CopyTextureProgram* mProgCopyTexture;
		class CopyTextureMaskProgram* mProgCopyTextureMask;
		class CopyTextureBiasProgram* mProgCopyTextureBias;
		class MappingTextureColorProgram* mProgMappingTextureColor;
		OpenGLFrameBuffer mFrameBuffer;

	};
}//namespace GL

#endif // GLDrawUtility_h__
