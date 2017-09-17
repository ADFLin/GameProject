#ifndef GLUtility_h__
#define GLUtility_h__

#include "GLCommon.h"
#include "CommonMarco.h"
#include <string>

#ifndef BIT
#define BIT( n ) ( 1 << ( n ) )
#endif

namespace RenderGL
{

	inline bool isNormalize(Vector3 const& v)
	{
		return Math::Abs(1.0 - v.length2()) < 1e-6;
	}

	class LookAtMatrix : public Matrix4
	{
	public:
		LookAtMatrix( Vector3 const& eyePos , Vector3 const& lookDir , Vector3 const& upDir )
		{
			Vector3 zAxis = -Normalize( lookDir );
			Vector3 xAxis = Normalize( upDir.cross( zAxis ) );
			Vector3 yAxis = zAxis.cross( xAxis );
			setValue( 
				xAxis.x , yAxis.x , zAxis.x ,
				xAxis.y , yAxis.y , zAxis.y ,
				xAxis.z , yAxis.z , zAxis.z , 
				-eyePos.dot( xAxis ) , -eyePos.dot( yAxis ) , -eyePos.dot( zAxis ) );
		}
	};

	class PerspectiveMatrix : public Matrix4
	{
	public:
		PerspectiveMatrix( float yFov , float aspect , float zNear , float zFar )
		{
			float f = 1.0 / Math::Tan( yFov / 2 );
			float dz = zNear - zFar;
			setValue(
		       f / aspect , 0 , 0 , 0 ,
			   0 , f , 0 , 0 ,
			   0 , 0 , ( zFar + zNear ) / dz , -1 ,
			   0 , 0 , 2 * zFar * zNear / dz , 0 );
		}

		PerspectiveMatrix( float left, float right, float bottom, float top,float zNear, float zFar )
		{
			float dz = zNear - zFar;
			float xFactor = 1 / ( right - left );
			float yFactor = 1 / ( top - bottom );
			float n2 = 2 * zNear;
			setValue(
				n2 * xFactor , 0 , 0 , 0 ,
				0 , n2 * yFactor , 0 , 0 ,
				( right + left ) * xFactor , ( top + bottom ) * yFactor , ( zFar + zNear ) / dz , -1 ,
				0 , 0 , 2 * zFar * zNear / dz , 0 );
		}
	};
	class OrthoMatrix : public Matrix4
	{
	public:
		OrthoMatrix( float left , float right , float bottom , float top  , float zNear , float zFar )
		{
			float xFactor = 1 / ( right - left );
			float yFactor = 1 / ( top - bottom );
			float zFactor = 1 / ( zFar - zNear );

			setValue( 
				2 * xFactor , 0 , 0 , 0 ,
				0  , 2 * yFactor , 0 , 0 ,
				0 , 0 , -2 * zFactor , 0 ,
				-( left + right ) * xFactor , -( top + bottom ) * yFactor ,-( zFar + zNear ) * zFactor ,  1 );
		}

		OrthoMatrix( float width , float height , float zNear , float zFar )
		{
			float xFactor = 2 / ( width );
			float yFactor = 2 / ( height );
			float zFactor = 1 / ( zFar - zNear );

			setValue( xFactor , 0 , 0 , 0 ,
				      0  , yFactor , 0 , 0 ,
				      0 , 0 , -2 * zFactor , 0 ,
				      0 ,0 , -( zFar + zNear ) * zFactor , 1 );
		}

	};

	class BasisMaterix : public Matrix4
	{
	public:
		static Matrix4 FromX(Vector3 const& axis)
		{
			Vector3 x0 = axis;
			Vector3 x1 = AnyOrthoVector(x0);
			Vector3 x2 = x0.cross(x1);
			return BasisMaterix(x0, x1, x2);
		}
		static Matrix4 FromY(Vector3 const& axis)
		{
			Vector3 x0 = axis;
			Vector3 x1 = AnyOrthoVector(x0);
			Vector3 x2 = x0.cross(x1);
			return BasisMaterix(x2, x0, x1);
		}
		static Matrix4 FromZ(Vector3 const& axis)
		{
			Vector3 x0 = axis;
			Vector3 x1 = AnyOrthoVector(x0);
			Vector3 x2 = x0.cross(x1);
			return BasisMaterix(x1, x2, x0);
		}
		static Matrix4 FromXY(Vector3 const& xAxis, Vector3 const& yAxis)
		{
			Vector3 x0 = xAxis;
			Vector3 x1 = yAxis;
			x1 -= Porjection(x1, x0);
			Vector3 x2 = x0.cross(x1);
			return BasisMaterix(x0, x1, x2);
		}

		static Matrix4 FromYZ(Vector3 const& yAxis, Vector3 const& zAxis)
		{
			Vector3 x0 = yAxis;
			Vector3 x1 = zAxis;
			x1 -= Porjection(x1, x0);
			Vector3 x2 = x0.cross(x1);
			return BasisMaterix(x2, x0, x1);
		}

		static Matrix4 FromZX(Vector3 const& zAxis, Vector3 const& xAxis)
		{
			Vector3 x0 = zAxis;
			Vector3 x1 = xAxis;
			x1 -= Porjection(x1, x0);
			Vector3 x2 = x0.cross(x1);
			return BasisMaterix(x1, x2, x0);
		}
	private:
		//#MOVE
		static Vector3 Porjection(Vector3 v, Vector3 dir)
		{
			return v - ( dir.dot(v) / dir.length2() ) * dir;
		}

		static Vector3 AnyOrthoVector(Vector3 v)
		{
			Vector3 temp = v.cross(Vector3(0, 0, 1));
			if( temp.length2() > 1e-5 )
				return temp;
			return v.cross(Vector3(1,0,0));
		}
		BasisMaterix(Vector3 xAxis, Vector3 yAxis, Vector3 zAxis)
		{
			float s;
			s = xAxis.normalize(); assert(s != 0);
			s = yAxis.normalize(); assert(s != 0);
			s = zAxis.normalize(); assert(s != 0);
			setValue(xAxis.x, xAxis.y, xAxis.z,
					 yAxis.x, yAxis.y, yAxis.z,
					 zAxis.x, zAxis.y, zAxis.z);
		}
	};


	class ReflectMatrix : public Matrix4
	{
	public:
		//plane: r * n + d = 0
		ReflectMatrix( Vector3 const& n , float d )
		{
			assert( isNormalize( n ) );
			// R = I - 2 n * nt
			// [  I 0 ] * [ R 0 ] * [ I 0 ] = [  R 0 ]
			// [ -D 1 ]   [ 0 1 ]   [ D 1 ]   [-2D 1 ]
			float xx = -2 * n.x * n.x;
			float yy = -2 * n.y * n.y;
			float zz = -2 * n.z * n.z;
			float xy = -2 * n.x * n.y;
			float xz = -2 * n.x * n.z;
			float yz = -2 * n.y * n.z;

			Vector3 off = -2 * d * n;

			setValue( 
				1 + xx , xy , xz ,
				xy , 1 + yy , yz ,
				xz , yz , 1 + zz ,
				off.x , off.y , off.z );
		}
	};

	class Camera
	{
	public:
		Camera()
			:mPos(0,0,0)
			,mYaw( 0 )
			,mPitch( 0 )
			,mRoll( 0 )
		{
			updateInternal();
		}
		void setPos( Vector3 const& pos ){ mPos = pos; }
		void setViewDir(Vector3 const& forwardDir, Vector3 const& upDir)
		{
			LookAtMatrix mat(Vector3::Zero(), forwardDir, upDir);
			mRotation.setMatrix(mat);
			mRotation = mRotation.inverse();
			Vector3 angle = mRotation.getEulerZYX();
			mYaw = angle.z;
			mRoll = angle.y;
			mPitch = angle.x;
		}
		void setRotation( float yaw , float pitch , float roll )
		{ 
			mYaw = yaw;
			mPitch = pitch; 
			mRoll = roll;
			updateInternal(); 
		}

		Matrix4 getViewMatrix() const { return LookAtMatrix( mPos , getViewDir() , getUpDir() ); }
		Matrix4 getTransform() const { return Matrix4( mPos , mRotation ); }
		Vector3 const& getPos() const { return mPos; }

		static Vector3 LocalViewDir(){ return Vector3( 0 , 0 , -1 ); }
		static Vector3 LocalUpDir(){ return Vector3( 0 , 1 , 0 ); }

		Vector3 getViewDir() const {  return mRotation.rotate( LocalViewDir() );  }
		Vector3 getUpDir() const {  return mRotation.rotate( LocalUpDir() );  }
		Vector3 getRightDir() const { return mRotation.rotate(Vector3(1, 0, 0)); }

		void    moveRight( float dist ){  mPos += mRotation.rotate( Vector3( dist , 0 , 0 ) );  }
		void    moveFront( float dist ){  mPos += dist * getViewDir();  }
		void    moveUp( float dist )   {  mPos += dist * getUpDir();  }


		Vector3 toWorld( Vector3 const& p ) const { return mPos + mRotation.rotate( p ); }
		Vector3 toLocal( Vector3 const& p ) const { return mRotation.rotateInverse( p - mPos ); }

		void    rotateByMouse( float dx , float dy )
		{
			mYaw   -=  dx;
			mPitch -=  dy;
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
			mRotation.setEulerZYX( mYaw  , mRoll , mPitch );
		}

		Quaternion mRotation;
		Vector3 mPos;
		float   mYaw;
		float   mPitch;
		float   mRoll;
	};

	struct OBJMaterialInfo
	{
		char const* name;

		Vector3 ambient;
		Vector3 diffuse;
		Vector3 specular;
		Vector3 transmittance;
		Vector3 emission;
		float shininess;
		float ior;
		float dissolve;
		int   illum;
		char const* ambientTextureName;
		char const* diffuseTextureName;
		char const* specularTextureName;
		char const* normalTextureName;
		std::vector < std::pair< char const*, char const* > > unknownParameters;

		char const* findParam(char const* name) const
		{
			for( auto& pair : unknownParameters )
			{
				if( strcmp(name, pair.first) == 0 )
					return pair.second;
			}
			return nullptr;
		}
	};

	class OBJMaterialBuildListener
	{
	public:
		virtual void build(int idxSection, OBJMaterialInfo const* mat) = 0;
	};

	class MeshUtility
	{
	public:
		static bool createTile( Mesh& mesh , int tileSize , float len );
		static bool createUVSphere( Mesh& mesh , float radius , int rings, int sectors);
		static bool createIcoSphere( Mesh& mesh , float radius , int numDiv );
		static bool createSkyBox( Mesh& mesh );
		static bool createCube( Mesh& mesh , float halfLen = 1.0f );
		
		static bool createCone(Mesh& mesh, float height, int numSide);
		static bool createDoughnut(Mesh& mesh , float radius , float ringRadius , int rings , int sectors);
		static bool createPlaneZ( Mesh& mesh , float len, float texFactor );
		static bool createPlane( Mesh& mesh , Vector3 const& offset , Vector3 const& normal , Vector3 const& dir , float len , float texFactor);


		static bool createSimpleSkin(Mesh& mesh, float width, float height, int nx, int ny);

		static bool createLightSphere(Mesh& mesh);
		static bool createLightCone(Mesh& mesh);

		struct MeshData
		{
			float* position;
			int    numPosition;
			float* normal;
			int    numNormal;
			int*   indices;
			int    numIndex;
		};
		static bool createMesh(Mesh& mesh, MeshData& data);



		static bool createFromObjectFile(
			Mesh& mesh, char const* path , 
			Matrix4* pTransform = nullptr , 
			OBJMaterialBuildListener* listener = nullptr ,
			int * skip = nullptr );

	};


	class Font
	{
	public:
		Font(){  base = 0;  }
		explicit Font( int size , HDC hDC ){ buildFontImage( size , hDC ); }
		~Font();
		void create( int size , HDC hDC );
		void printf( char const* fmt, ...);
		void print( char const* str );
	private:
		void buildFontImage( int size , HDC hDC );
		GLuint	base;
	};

	class RenderRT
	{
	public:
		enum 
		{
			USAGE_P = BIT(0) ,
			USAGE_N = BIT(1) ,
			USAGE_C = BIT(2) ,
			USAGE_TEX0 = BIT(3) ,
		};

		enum VertexFormat
		{
			eXYZ = USAGE_P  ,
			eXYZ_C = USAGE_P | USAGE_C ,
			eXYZ_N_C = USAGE_P | USAGE_N | USAGE_C ,
			eXYZ_N_C_T2 = USAGE_P | USAGE_N | USAGE_C | USAGE_TEX0 ,
			eXYZ_C_T2 = USAGE_P | USAGE_C | USAGE_TEX0,
			eXYZ_N = USAGE_P | USAGE_N ,
			eXYZ_N_T2 = USAGE_P | USAGE_N | USAGE_TEX0 ,
			eXYZ_T2 = USAGE_P | USAGE_TEX0 ,
		};


		template < uint32 VF >
		inline static void Draw( GLuint type , void const* vtx , int nV , int vertexStride )
		{
			bindArray< VF >( (float const*)vtx , vertexStride );
			glDrawArrays( type , 0 , nV );
			unbindArray< VF >();
		}

		template < uint32 VF >
		inline static void Draw( GLuint type , void const* vtx , int nV )
		{
			Draw< VF >( type , vtx , nV , (int) CountSize< VF >::Result * sizeof( float) );
		}

		template< uint32 BIT >
		struct MaskSize { enum { Result = 3 }; };
		template<>
		struct MaskSize< 0 > { enum { Result = 0 }; };
		template<>
		struct MaskSize< USAGE_TEX0 > { enum { Result = 2 }; };

		template< uint32 BITS >
		struct CountSize
		{
			enum { Result = MaskSize<( BITS & 0x1 )>::Result + CountSize< ( BITS >> 1 ) >::Result  };
		};
		template<>
		struct CountSize< 0 >
		{
			enum { Result = 0 };
		};

		template< uint32 VF > 
		inline static void bindArray( float const* v , int vertexStride )
		{
#define COUNT_MASK( V ) ( V - 1 )
			if ( VF & USAGE_P )
			{
				glEnableClientState( GL_VERTEX_ARRAY );
				glVertexPointer( 3 , GL_FLOAT , vertexStride , v + CountSize< VF & COUNT_MASK( USAGE_P ) >::Result );
			}
			if ( VF & USAGE_N )
			{
				glEnableClientState( GL_NORMAL_ARRAY );
				glNormalPointer( GL_FLOAT , vertexStride, v + CountSize< VF & COUNT_MASK( USAGE_N ) >::Result );
			}
			if ( VF & USAGE_C )
			{
				glEnableClientState( GL_COLOR_ARRAY );
				int offset = CountSize< VF & COUNT_MASK( USAGE_C ) >::Result;
				glColorPointer( 3 , GL_FLOAT , vertexStride , v + offset );
			}

			if ( VF & USAGE_TEX0 )
			{
				glClientActiveTexture( GL_TEXTURE0 );
				glEnableClientState( GL_TEXTURE_COORD_ARRAY );
				glTexCoordPointer( 2 , GL_FLOAT , vertexStride , v + CountSize< VF & COUNT_MASK( USAGE_TEX0 ) >::Result );
			}
#undef COUNT_MASK
		}

		template< uint32 VF > 
		inline static void unbindArray()
		{
			if ( VF & USAGE_P )
			{
				glDisableClientState( GL_VERTEX_ARRAY );
			}
			if ( VF & USAGE_N )
			{
				glDisableClientState( GL_NORMAL_ARRAY );
			}
			if ( VF & USAGE_C )
			{
				glDisableClientState( GL_COLOR_ARRAY );

			}
			if ( VF & USAGE_TEX0 )
			{
				glClientActiveTexture( GL_TEXTURE0 );
				glDisableClientState( GL_TEXTURE_COORD_ARRAY );
			}
		}
	};


	struct MatrixSaveScope
	{
		MatrixSaveScope( Matrix4 const& projMat , Matrix4 const& viewMat )
		{
			glMatrixMode( GL_PROJECTION );
			glPushMatrix();
			glLoadMatrixf( projMat );
			glMatrixMode( GL_MODELVIEW );
			glPushMatrix();
			glLoadMatrixf( viewMat );
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
			glMatrixMode( GL_PROJECTION );
			glPopMatrix();
			glMatrixMode( GL_MODELVIEW );
		}
	};

	struct ViewportSaveScope
	{
		ViewportSaveScope()
		{
			glGetIntegerv(GL_VIEWPORT, value);
		}
		~ViewportSaveScope()
		{
			glViewport(value[0], value[1], value[2], value[3]);
		}

		int operator[](int idx) const { return value[idx]; }
		int value[4];
	};


}//namespace GL

#endif // GLUtility_h__