#include "CFlyPCH.h"
#include "CFMeshBuilder.h"

namespace CFly
{

	MeshBuilder::MeshBuilder( VertexType type )
	{
		mVtxSize = 0;
		mFlag    = 0;
		setVertexType( type );
	}

	MeshBuilder::~MeshBuilder()
	{

	}

	void MeshBuilder::setVertexType( VertexType type )
	{
		mVtxType = type;
		mNumTex  = 0;

		std::fill_n( mPosComponent , (unsigned)CFCM_NUM_COMPONENT , -1 );
		mVtxSize = 0;


		uint32 posType = type & CFVF_POSITION_MASK;
		if ( posType )
		{
			mPosComponent[ CFCM_POSITION ] = mVtxSize;
			if ( posType == CFV_XYZW || posType == CFV_XYZRHW )
			{
				mVtxSize += 4;
			}
			else
			{
				mVtxSize += 3;
			}
		}
#define DEF_COMP( NAME , CFV , SIZE )\
		if ( type & CFV )\
		{\
			mPosComponent[ NAME ] = mVtxSize;\
			mVtxSize += SIZE;\
		}

		DEF_COMP( CFCM_BLENDWEIGHT  , CFV_BLENDWEIGHT  , CFVF_BLENDWEIGHTSIZE_GET( type ) )
		DEF_COMP( CFCM_BLENDINDICES , CFV_BLENDINDICES , 1 )
		DEF_COMP( CFCM_NORMAL       , CFV_NORMAL       , 3 )

		if ( type & CFV_TANGENT )
		{
			mVtxSize += 3;
		}

		if ( type & CFV_BINORMAL )
		{
			mVtxSize += 3;
		}

		assert( ( mVtxSize & CFVF_COLOR_ALPHA ) == 0 );
		int colorSize = ( mVtxSize & CFVF_COLOR_FMT_BYTE ) ? 1 : 3;
		if ( ( mVtxSize & CFVF_COLOR_ALPHA ) && colorSize == 3 )
			colorSize = 4;

		if ( type &  CFV_COLOR  )
		{
			mPosComponent[ CFCM_COLOR ] = mVtxSize;
			mVtxSize += colorSize;

			if ( mVtxType & CFV_COLOR2  )
			{
				mPosComponent[ CFCM_COLOR2 ] = mVtxSize;
				mVtxSize += colorSize;
			}
		}

		if ( type & CFV_TEXCOORD )
		{
			mNumTex = CFVF_TEXCOUNT_GET( mVtxType );
			for ( int i = 0 ; i < mNumTex ; ++i  )
			{
				mTexLen[i] = CFVF_TEXSIZE_GET( type , i );
				mPosComponent[ CFCM_TEX0 + i ] = mVtxSize;
				mVtxSize += mTexLen[i];
			}
		}

		mTempVertex.resize( mVtxSize );

		mVtxBuffer.clear();
		mNumVertex = 0;

#undef DEF_COMP
	}

	void MeshBuilder::fillVertex( VertexComponent pos , Vector3 const& v )
	{
		assert( mPosComponent[pos] != -1 );
		float* temp = &mTempVertex[0] + mPosComponent[pos];
		Utility::fill( temp , v );
	}

	void MeshBuilder::fillVertex( VertexComponent pos , Vector3 const& v , float w )
	{
		assert( mPosComponent[pos] != -1 );
		float* temp = &mTempVertex[0] + mPosComponent[pos];
		Utility::fill( temp , v );
		*(temp + 3 ) = w;
	}

	void MeshBuilder::fillVertex( VertexComponent pos , float v0 , float v1 )
	{
		assert( mPosComponent[pos] != -1 );
		float* temp = &mTempVertex[0] + mPosComponent[pos];
		Utility::fill( temp , v0 , v1 );
	}

	void MeshBuilder::fillVertex( VertexComponent pos , float v )
	{
		assert( mPosComponent[pos] != -1 );
		float* temp = &mTempVertex[0] + mPosComponent[pos];
		temp[0] = v;
	}

	void MeshBuilder::fillVertex( VertexComponent pos , Color3f const& v )
	{
		assert( mPosComponent[pos] != -1 );
		float* temp = &mTempVertex[0] + mPosComponent[pos];
		temp[0] = v.r;
		temp[1] = v.g;
		temp[2] = v.b;
	}

	void MeshBuilder::fillVertex( VertexComponent pos , uint32 value )
	{
		uint32* temp = reinterpret_cast< uint32* >( &mTempVertex[0] + mPosComponent[pos] );
		*temp = value;
	}

	int MeshBuilder::createIndexTrangle( Object* obj , Material* mat )
	{
		MeshInfo info; 
		info.primitiveType = CFPT_TRIANGLELIST;
		info.vertexType  = mVtxType;
		info.pVertex     = &mVtxBuffer[0];
		info.numVertices = mNumVertex;
		info.pIndex      = &mIdxBuffer[0];
		info.numIndices  = mIdxBuffer.size();
		info.isIntIndexType = true;
		info.flag   = mFlag;

		return obj->createMesh( mat , info );
	}

	VertexBuffer* MeshBuilder::createBuffer( World* world )
	{
		//MeshCreator* creator = world->_getMeshCreator();


		return nullptr;
	}

}//namespace CFly