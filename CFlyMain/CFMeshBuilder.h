#ifndef CFMeshBuilder_h__
#define CFMeshBuilder_h__

#include "CFBase.h"
#include "CFObject.h"

namespace CFly
{

	class MeshBase;


	enum VertexComponent
	{
		CFCM_POSITION ,
		CFCM_BLENDWEIGHT ,
		CFCM_BLENDINDICES ,
		CFCM_NORMAL ,
		CFCM_COLOR  ,
		CFCM_COLOR2 ,
		CFCM_TEX0 ,
		CFCM_TEX1 ,
		CFCM_TEX2 ,
		CFCM_TEX3 ,
		CFCM_TEX4 ,
		CFCM_TEX5 ,
		CFCM_TEX6 ,
		CFCM_TEX7 ,

		CFCM_NUM_COMPONENT ,
	};

	class MeshBuilder
	{

	public:
		MeshBuilder( VertexType type );
		~MeshBuilder();

		void setVertexType( VertexType type );
		void reserveVexterBuffer( size_t size ){ mVtxBuffer.reserve( mVtxSize * size ); }
		void reserveIndexBuffer( size_t size ) { mIdxBuffer.reserve( size ); }


		float* getVertexComponent( int idx , VertexComponent component )
		{
			assert( mPosComponent[component] != -1 );
			return &mVtxBuffer[0] + ( mVtxSize * idx + mPosComponent[ component] );
		}

		void setPosition( Vector3 const& pos )          {  fillVertex( CFCM_POSITION , pos ); }
		void setPosition( Vector3 const& pos , float w ){  fillVertex( CFCM_POSITION , pos , w );  }
		void setBlendIndices( uint32 value )   { fillVertex( CFCM_BLENDINDICES , value ); }
		void setNormal( Vector3 const& normal ){ fillVertex( CFCM_NORMAL  , normal ); }
		void setColor( uint32 color , int idx = 0 )
		{ 
			assert( mVtxType & CFVF_COLOR_FMT_BYTE ); 
			fillVertex( VertexComponent( CFCM_COLOR + idx ) , color );
		}
		void setColor( Color3f const& color , int idx = 0 )
		{ 
			assert( ( mVtxType & CFVF_COLOR_FMT_BYTE ) == 0 ); 
			fillVertex( VertexComponent( CFCM_COLOR + idx ) , color );  
		}
		void setColor( float* rgb , int idx = 0 )
		{ 
			assert( ( mVtxType & CFVF_COLOR_FMT_BYTE ) == 0 ); 
			fillVertex( VertexComponent( CFCM_COLOR + idx ) , Color3f( rgb[0] , rgb[1], rgb[2] ) );  
		}

		void setTexCoord( int idx , float u , float v )
		{
			assert( mTexLen[ idx ] == 2 );
			fillVertex( VertexComponent( CFCM_TEX0 + idx ) , u , v );
		}

		void setTexCoord( int idx , Vector3 const& coord )
		{
			assert( mTexLen[ idx ] == 3 );
			fillVertex( VertexComponent( CFCM_TEX0 + idx ) , coord );
		}

		void setTexCoord( int idx , float u , float v , float r )
		{
			assert( mTexLen[ idx ] == 3 );
			fillVertex( VertexComponent( CFCM_TEX0 + idx ) , Vector3( u , v , r ) );
		}
		void addVertex()
		{
			mVtxBuffer.insert( mVtxBuffer.end() , &mTempVertex[0] , &mTempVertex[0] + mVtxSize );
			++mNumVertex;
		}

		void addQuad( int i0 , int i1 , int i2 , int i3 )
		{
			addTriangle( i0 , i1 , i2 );
			addTriangle( i0 , i2 , i3 );
		}
		void addTriangle( int i0 , int i1 , int i2 )
		{
			mIdxBuffer.push_back( i0 );
			mIdxBuffer.push_back( i1 );
			mIdxBuffer.push_back( i2 );
		}

		VertexBuffer* createBuffer( World* world );

		int  createIndexTrangle( Object* obj , Material* mat );
		void clearTrangles(){ mIdxBuffer.clear(); }

		void resetBuffer()
		{
			mVtxBuffer.clear();
			mNumVertex = 0;
			clearTrangles();
		}

		void setFlag( unsigned flag ){ mFlag = flag; }

		int  getVertexNum(){ return mNumVertex; }

	private:
		void fillVertex( VertexComponent pos , Color3f const& v );
		void fillVertex( VertexComponent pos , float v );
		void fillVertex( VertexComponent pos , float v0 , float v1 );
		void fillVertex( VertexComponent pos , Vector3 const& v );
		void fillVertex( VertexComponent pos , Vector3 const& v , float w );
		void fillVertex( VertexComponent pos , uint32 value );
		

		std::vector< float > mVtxBuffer;
		std::vector< int >   mIdxBuffer;

		int     mPosComponent[ CFCM_NUM_COMPONENT ];
		std::vector< float > mTempVertex;

		VertexType mVtxType;
		int        mNumVertex;
		int        mVtxSize;
		unsigned   mFlag;

		int      mTexLen[8];
		int      mNumTex;

	};


	namespace Utility
	{
		inline void fill( float* vtx , Vector3 const& v )
		{
			vtx[0] = v.x; vtx[1] = v.y; vtx[2] = v.z;
		}
		inline void fill( float* vtx , float v0 , float v1 )
		{
			vtx[0] = v0; vtx[1] = v1;
		}
		inline void fill( float* vtx , float v0 , float v1 , float v2 )
		{
			vtx[0] = v0; vtx[1] = v1; vtx[2] = v2;
		}
	}

}//namespace CFly

#endif // CFMeshBuilder_h__

