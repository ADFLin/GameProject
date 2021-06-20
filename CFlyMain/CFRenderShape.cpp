#include "CFlyPCH.h"
#include "CFRenderShape.h"

#include "CFBuffer.h"
#include "CFMaterial.h"
#include "CFVertexUtility.h"
#include "CFGeomUtility.h"
#include "CFRenderSystem.h"

namespace CFly
{

	MeshBase::MeshBase()
	{
		mVertexDecl   = nullptr;
		mNumVertex  = 0;
		mNumElement = 0;
	}

	MeshBase::~MeshBase()
	{
		if ( mVertexDecl )
			mVertexDecl->Release();
		if ( mDeclFormat == VDF_DECL && mDecl )
			mDecl->decRef();
	}

	MeshBase::MeshBase( MeshBase& rhs ) 
		:mPrimitiveType( rhs.mPrimitiveType )
		,mIndexBuffer( rhs.mIndexBuffer )
		,mNumElement( rhs.mNumElement )
	{
		_shareVertexData( rhs );
	}

	MeshBase* MeshBase::clone()
	{
		MeshBase* cMesh = new MeshBase(*this);

		for( int i = 0; i < NUM_STREAM_GROUP ; ++i )
		{
			if ( cMesh->mVertexBuffer[i] )
				cMesh->mVertexBuffer[i] = mVertexBuffer[i]->clone();
		}

		if ( cMesh->mIndexBuffer )
			cMesh->mIndexBuffer = mIndexBuffer->clone();

		return cMesh;
	}


	void MeshBase::setupVertexDecl( VertexType type , VertexInfo& info , VertexDeclFormat suggestFormat )
	{
		mVertexType = type;
		VertexUtility::calcVertexInfo( type , info );
		if ( suggestFormat == VDF_FVF )
		{
			mFVF = VertexUtility::evalFVF( type , info );
			if ( mFVF == 0 )
				suggestFormat = VDF_TYPE_DEF;
		}
		mDeclFormat = suggestFormat;
	}

	void MeshBase::destroyThis()
	{
		delete this;
	}

	void MeshBase::addNormal()
	{
		if ( mVertexType & CFV_NORMAL )
			return;

		if ( mDeclFormat == VDF_FVF )
		{
			addNormalFVF();
			return;
		}







	}

	void MeshBase::addNormalFVF()
	{
		assert( ( mVertexType & CFV_NORMAL ) == 0 );

		bool useTempIndex = false;
		int* tempIndex;

		if ( mPrimitiveType != D3DPT_TRIANGLELIST )
		{
			//FIXME
			assert(0);
			return;
		}

		if ( !mIndexBuffer || !mVertexBuffer[0] )
			return;

		D3DDevice* d3dDevice;
		mVertexBuffer[0]->getSystemImpl()->GetDevice( &d3dDevice );

		assert( d3dDevice );

		unsigned vtxSize = mVertexBuffer[0]->getVertexSize();

		VertexBuffer::RefCountPtr newVBuffer = new VertexBuffer;
		if ( !newVBuffer->create( d3dDevice , vtxSize + 4 * 3  , mNumVertex  , mFVF | D3DFVF_NORMAL ) )
		{
			return;
		}

		float* vtx = static_cast< float*>( mVertexBuffer[0]->lock() );

		
		int* idx;
		if ( !mIndexBuffer->isUseInt32() )
		{
			idx = new int[ mIndexBuffer->getIndexNum() ];
			short* oldIdx = static_cast< short*>( mIndexBuffer->lock( CFBL_READ_ONLY ) );
			std::copy( oldIdx , oldIdx + mIndexBuffer->getIndexNum() , idx );
		}
		else
		{
			idx = static_cast< int*>( mIndexBuffer->lock( CFBL_READ_ONLY ) );
		}
		

		std::vector< Vector3 > normal( mNumVertex );
		VertexUtility::calcVertexNormal( vtx , vtxSize / 4 , mNumVertex , idx , mNumElement * 3 , &normal[0] );

		mIndexBuffer->unlock();
		if ( !mIndexBuffer->isUseInt32() )
		{
			delete [] idx;
		}

		float* newVtx = static_cast< float*>( newVBuffer->lock() );

		int offset = VertexUtility::getComponentOffsetFVF( mFVF , D3DFVF_NORMAL );
		VertexUtility::mergeVertex( newVtx , mNumVertex ,offset , vtx , vtxSize / 4 , (float*) &normal[0] , 3 );

		newVBuffer->unlock();
		mVertexBuffer[0]->unlock();

		mFVF |= D3DFVF_NORMAL;
		mVertexType |= CFV_NORMAL;
		if ( mVertexDecl )
		{
			mVertexDecl->Release();
			mVertexDecl = NULL;
		}
		mVertexBuffer[0] = newVBuffer;

	}

	void MeshBase::_setupDevice( D3DDevice* d3dDevice , unsigned& restOptionBit )
	{
		DWORD const BlendMatrixMap[] = { D3DVBF_0WEIGHTS , D3DVBF_1WEIGHTS , D3DVBF_2WEIGHTS , D3DVBF_3WEIGHTS };
		
		if ( mVertexType & ( CFV_BLENDWEIGHT | CFV_BLENDINDICES ) )
		{
			int numBlendWeight = ( mVertexType & CFV_BLENDWEIGHT ) ? CFVF_BLENDWEIGHTSIZE_GET( mVertexType ) : 0;
			d3dDevice->SetRenderState( D3DRS_VERTEXBLEND , BlendMatrixMap[ numBlendWeight ] );
		}
		else
		{
			d3dDevice->SetRenderState( D3DRS_VERTEXBLEND , D3DVBF_DISABLE );
		}

		if ( mVertexType & CFV_BLENDINDICES )
		{
			d3dDevice->SetRenderState( D3DRS_INDEXEDVERTEXBLENDENABLE , TRUE );
		}
		else
		{
			d3dDevice->SetRenderState( D3DRS_INDEXEDVERTEXBLENDENABLE , FALSE );
		}

	}

	void  MeshBase::_renderPrimitive( RenderSystem* renderSystem )
	{
		renderSystem->renderPrimitive( mPrimitiveType , mNumElement , mNumVertex , mIndexBuffer );
	}

	void MeshBase::_setupStream( RenderSystem* renderSystem , bool useShader )
	{
		
		D3DDevice* d3dDevice = renderSystem->getD3DDevice();
		{
			CF_PROFILE("SetupVertexBuffer");

			renderSystem->setVertexBlend( mVertexType );

			if ( mDeclFormat == VDF_FVF )
			{
				if ( useShader )
				{
					if ( mVertexDecl )
					{		
						D3DVERTEXELEMENT9 decl[MAX_FVF_DECL_SIZE];
						D3DXDeclaratorFromFVF( mFVF , decl );
						d3dDevice->CreateVertexDeclaration( decl , &mVertexDecl );
					}
					d3dDevice->SetVertexDeclaration( mVertexDecl );
				}
				else
				{
					d3dDevice->SetFVF( mFVF );
				}
				mVertexBuffer[0]->setupDevice( d3dDevice , 0 );
			}
			else
			{
				assert( mVertexDecl );
				d3dDevice->SetVertexDeclaration( mVertexDecl );
				for(int i = 0;i< NUM_STREAM_GROUP;++i)
				{
					if ( mVertexBuffer[i] )
						mVertexBuffer[i]->setupDevice( d3dDevice , i );
				}
			}
		}
	}


	void MeshBase::calcBoundSphere( BoundSphere& sphere )
	{
		sphere.center.setValue( 0,0,0 );

		float* v = reinterpret_cast< float*>( mVertexBuffer[0]->lock(  0 , 0  , CFBL_READ_ONLY ) );
		float* pos = v;
		int offset = mVertexBuffer[0]->getVertexSize() / 4;

		for( int i = 0 ; i < mNumVertex ; ++i )
		{
			sphere.center += Vector3(pos);
			pos += offset;
		}

		sphere.center *= ( 1.0f / mNumVertex );
		sphere.radius = 0.0f;

		pos = v;
		for( int i = 0 ; i < mNumVertex ; ++i )
		{
			sphere.addPoint( Vector3(pos) );
			pos += offset;
		}

		mVertexBuffer[0]->unlock();
	}

	int MeshBase::getTriangleNum()
	{
		switch( mPrimitiveType )
		{
		case CFPT_TRIANGLELIST :
			return mNumElement / 3;
		case CFPT_TRIANGLESTRIP :
		case CFPT_TRIANGLEFAN :
			return mNumElement - 2;
		}
		return 0;
	}

	VertexBuffer* MeshBase::getVertexElement( VertexElementSemantic element , unsigned& offset )
	{
		switch( mDeclFormat )
		{
		case VDF_FVF:
			{
				int off = VertexUtility::getVertexElementOffsetFVF( mFVF , element );
				if ( off == -1 )
					return nullptr;
				offset = 4 * off;
				return mVertexBuffer[0];
			}
			break;
		case VDF_TYPE_DEF:
			{
				offset = 0;
				unsigned colorSize = 3 * sizeof( float );
				if ( mVertexType & CFVF_COLOR_ALPHA )
					colorSize += sizeof( float );
				if ( mVertexType & CFVF_COLOR_FMT_BYTE )
					colorSize = 4;

				switch( element )
				{
				case CFV_TEXCOORD :
					if ( mVertexType & CFV_COLOR2 )
						offset += colorSize;
				case CFV_COLOR2 :
					if ( mVertexType & CFV_COLOR )
						offset += colorSize;
				case CFV_COLOR :
					return mVertexBuffer[ VSG_COLOR_TEX ];
					////////////////////////////////////////////
				case CFV_PSIZE :
					if ( mVertexType & CFV_BINORMAL )
						offset += 3 * sizeof( float );
				case CFV_BINORMAL:
					if ( mVertexType & CFV_TANGENT )
						offset += 3 * sizeof( float );
				case CFV_TANGENT:
					if ( mVertexType & CFV_NORMAL )
						offset += 3 * sizeof( float );
				case CFV_NORMAL:
					return mVertexBuffer[ VSG_GEOM ];
					////////////////////////////////////////////
				case CFV_BLENDINDICES :
					if ( mVertexType & CFV_BLENDWEIGHT )
						offset += CFVF_BLENDWEIGHTSIZE_GET( mVertexType ) * sizeof( float );
				case CFV_BLENDWEIGHT:
					if ( mVertexType == CFV_XYZ )
						offset += 3 * sizeof( float );
					else if ( mVertexType == CFV_XYZRHW || mVertexType == CFV_XYZW )
						offset += 4 * sizeof( float );
				case CFV_XYZ :
				case CFV_XYZW :
				case CFV_XYZRHW:
					return mVertexBuffer[ VSG_VERTEX ];
					////////////////////////////////////////////
				}
			}
			break;
		case VDF_TYPE_DEF_SINGLE_STREAM:
			{
				offset = VertexUtility::getVertexElementOffset( mVertexType , element );
				if ( offset == -1 )
					return nullptr;
				return mVertexBuffer[0];
			}
			break;
		case VDF_DECL:
			{


			}
			break;
		}

		return nullptr;
	}

	void MeshBase::createVertexDeclFVF( D3DDevice* d3dDevice )
	{
		assert( mFVF );

		D3DVERTEXELEMENT9 decl[MAX_FVF_DECL_SIZE];
		D3DXDeclaratorFromFVF( mFVF , decl );
		d3dDevice->CreateVertexDeclaration( decl , &mVertexDecl );
	}

	MeshBase* MeshBase::xform( Matrix4 const& trans , bool beAllInstance )
	{
		MeshBase* result = this;
		if ( !beAllInstance && getRefCount() > 1 )
		{
			result = clone();	
		}

		unsigned offset;
		VertexBuffer* buffer = result->getVertexElement( CFV_XYZ , offset );

		if ( buffer )
		{
			uint8* data = static_cast< uint8* >( buffer->lock( 0 , 0 , 0 ) );
			if ( data )
			{
				data += offset;

				for( int i = 0 ; i < result->getVertexNum() ; ++i )
				{
					Vector3& v = *( reinterpret_cast< Vector3*>( data ) );
					v = v * trans;
					data += buffer->getVertexSize();
				}
				buffer->unlock();
			}
		}

		return result;
	}

	void MeshBase::_shareVertexData( MeshBase& other )
	{
		mDeclFormat = other.mDeclFormat;
		mVertexType = other.mVertexType;
		mNumVertex  = other.mNumVertex;

		switch( mDeclFormat )
		{
		case VDF_FVF: 
			mFVF = other.mFVF; 
			break;
		case VDF_TYPE_DEF:
		case VDF_TYPE_DEF_SINGLE_STREAM:
			break;
		case VDF_DECL:
			mDecl = other.mDecl;
			mDecl->incRef();
			break;
		}

		mVertexDecl = other.mVertexDecl;
		if ( mVertexDecl )
			mVertexDecl->AddRef();
		for( int i = 0; i < NUM_STREAM_GROUP ; ++i )
			mVertexBuffer[i] = other.mVertexBuffer[i];
	}

	static void setVertexElement( D3DVERTEXELEMENT9& decl ,
		WORD stream , WORD offset , BYTE type , BYTE usage , BYTE usageIndex )
	{
		decl.Stream = stream;
		decl.Offset = offset;
		decl.Type = type;
		decl.Method = D3DDECLMETHOD_DEFAULT;
		decl.Usage = usage;
		decl.UsageIndex = usageIndex;
	}

	MeshCreator::MeshCreator( D3DDevice* d3dDevice ) 
		:mD3dDevice(d3dDevice)
	{
		mUsageSoftwareVertexProcess = false;
	}

	inline D3DVERTEXELEMENT9* setupDecl_Vertex( D3DVERTEXELEMENT9* ele , WORD group , WORD& offset  , VertexType type )
	{
		switch ( type & CFVF_POSITION_MASK )
		{
		case CFV_XYZ: 
			setVertexElement( *(ele++) , group , offset , D3DDECLTYPE_FLOAT3 , D3DDECLUSAGE_POSITION , 0 );
			offset += 3 * sizeof( float );
			break;
		case CFV_XYZW: 
			setVertexElement( *(ele++) , group , offset , D3DDECLTYPE_FLOAT4 , D3DDECLUSAGE_POSITION , 0 );
			offset += 4 * sizeof( float ); 
			break;
		case CFV_XYZRHW: 
			setVertexElement( *(ele++) , group , offset , D3DDECLTYPE_FLOAT4 , D3DDECLUSAGE_POSITIONT , 0 );
			offset += 4 * sizeof( float );
			break;
		}

		if ( type & CFV_BLENDWEIGHT )
		{
			BYTE const FloatTypeMap[] = 
			{
				0,
				D3DDECLTYPE_FLOAT1  ,
				D3DDECLTYPE_FLOAT2  ,
				D3DDECLTYPE_FLOAT3  , 
				D3DDECLTYPE_FLOAT4  ,
			};

			int numBW = CFVF_BLENDWEIGHTSIZE_GET( type );
			if ( numBW )
			{
				setVertexElement( *(ele++) , group , offset , FloatTypeMap[ numBW ] , D3DDECLUSAGE_BLENDWEIGHT , 0 );
				offset += numBW * sizeof( float );
			}
		}

		if ( type & CFV_BLENDINDICES )
		{
			setVertexElement( *(ele++) , group , offset , D3DDECLTYPE_UBYTE4 , D3DDECLUSAGE_BLENDINDICES , 0 );
			offset += 4;
		}
		return ele;
	}


	inline D3DVERTEXELEMENT9* setupDecl_Goem( D3DVERTEXELEMENT9* ele , WORD group , WORD& offset  , VertexType type )
	{
		if ( type & CFV_NORMAL )
		{
			setVertexElement( *(ele++) , group , offset , D3DDECLTYPE_FLOAT3 , D3DDECLUSAGE_NORMAL , 0 );
			offset += 3 * sizeof( float );
		}
		if ( type & CFV_TANGENT )
		{
			setVertexElement( *(ele++) , group , offset , D3DDECLTYPE_FLOAT3 , D3DDECLUSAGE_TANGENT , 0 );
			offset += 3 * sizeof( float );
		}
		if ( type & CFV_BINORMAL )
		{
			setVertexElement( *(ele++) , group , offset , D3DDECLTYPE_FLOAT3 , D3DDECLUSAGE_BINORMAL , 0 );
			offset += 3 * sizeof( float );
		}
		if ( type & CFV_PSIZE )
		{
			setVertexElement( *(ele++) , group , offset , D3DDECLTYPE_FLOAT1 , D3DDECLUSAGE_PSIZE , 0 );
			offset += sizeof( float );
		}
		return ele;
	}

	inline D3DVERTEXELEMENT9* setupDecl_ColorTex( D3DVERTEXELEMENT9* ele , WORD group , WORD& offset  , VertexType type )
	{
		if ( type & CFV_COLOR )
		{
			int  size;
			BYTE d3dType;
			if ( type & CFVF_COLOR_FMT_BYTE )
			{
				d3dType = D3DDECLTYPE_UBYTE4;
				size = 4;
			}
			else if ( type & CFVF_COLOR_ALPHA )
			{
				d3dType = D3DDECLTYPE_FLOAT4;
				size = 4 * sizeof( float );
			}
			else
			{
				d3dType = D3DDECLTYPE_FLOAT3;
				size = 3 * sizeof( float );
			}

			setVertexElement( *(ele++) , group , offset , d3dType , D3DDECLUSAGE_COLOR , 0 );
			offset += size;
			if ( type & CFV_COLOR2 )
			{
				setVertexElement( *(ele++) , group , offset , d3dType , D3DDECLUSAGE_COLOR , 1 );
				offset += size;
			}
		}

		if ( type & CFV_TEXCOORD )
		{
			int numTex = CFVF_TEXCOUNT_GET( type );
			for( int i = 0 ; i < numTex ; ++i )
			{
				int size = CFVF_TEXSIZE_GET( type , i );
				setVertexElement( *(ele++) , group , offset , size - 1 , D3DDECLUSAGE_TEXCOORD , i );
				offset += size * 4;
			}
		}

		return ele;
	}


	D3DVertexDecl* MeshCreator::createVertexDeclSingleStream( VertexType type , int& vertexSize )
	{
		WORD offset = 0;

		D3DVERTEXELEMENT9 decl[MAX_FVF_DECL_SIZE];
		D3DVERTEXELEMENT9* ele = decl;

		ele = setupDecl_Vertex( ele , 0 , offset , type );
		ele = setupDecl_Goem( ele , 0 , offset , type );
		ele = setupDecl_ColorTex( ele , 0 , offset , type );

		setVertexElement( *ele , 0xff , 0 , D3DDECLTYPE_UNUSED , 0 , 0 );

		D3DVertexDecl* vertexDecl = nullptr;
		mD3dDevice->CreateVertexDeclaration( decl , &vertexDecl );

		vertexSize = offset;
		return vertexDecl;

	}

	D3DVertexDecl* MeshCreator::createVertexDeclMultiStream( VertexType type , int groupPos[] , int groupSize[] )
	{
		WORD offset[ NUM_STREAM_GROUP ];
		int vertexPos = 0;
		for( int i = 0 ; i < NUM_STREAM_GROUP ; ++i )
		{
			offset[i] = 0;
			groupPos[0] = 0;
			groupSize[0] = 0;
		}

		StreamGroup group;
		D3DVERTEXELEMENT9  decl[MAX_FVF_DECL_SIZE];
		D3DVERTEXELEMENT9* ele = decl;
		
		group = VSG_VERTEX;
		ele = setupDecl_Vertex( ele , group , offset[group] , type );
		groupPos[ group ] = vertexPos;
		groupSize[ group ] = offset[ group ];
		vertexPos += offset[ group ];

		group= VSG_GEOM;
		ele = setupDecl_Goem( ele , group , offset[group] , type );
		groupPos[ group ] = vertexPos;
		groupSize[ group ] = offset[ group ];
		vertexPos += offset[ group ];

		group = VSG_COLOR_TEX;
		ele = setupDecl_ColorTex( ele , group , offset[group] , type );
		groupPos[ group ] = vertexPos;
		groupSize[ group ] = offset[ group ];
		vertexPos += offset[ group ];


		setVertexElement( *ele , 0xff , 0 , D3DDECLTYPE_UNUSED , 0 , 0 );

		D3DVertexDecl* vertexDecl = nullptr;
		mD3dDevice->CreateVertexDeclaration( decl , &vertexDecl );
		return vertexDecl;
	}

	VertexBuffer* MeshCreator::createVertexBuffer( unsigned vertexSize , int numVertices )
	{
		VertexBuffer* vtxBuffer = new VertexBuffer;
		if ( !vtxBuffer->create( getD3DDevice() , vertexSize , numVertices , mUsageSoftwareVertexProcess ) )
		{
			delete vtxBuffer;
			return nullptr;
		}
		return vtxBuffer;
	}

	VertexBuffer* MeshCreator::createVertexBuffer( unsigned vertexSize , int numVertices , void* vertex , unsigned offset , unsigned stride )
	{
		VertexBuffer* vtxBuffer = createVertexBuffer( vertexSize , numVertices );
		if ( !vtxBuffer )
			return nullptr;

		char* dest = static_cast< char * >( vtxBuffer->lock() );
		if ( !dest )
		{
			delete vtxBuffer;
			return nullptr;
		}

		VertexUtility::fillData( dest , vertexSize , numVertices , static_cast< char*>( vertex ) + offset ,  stride );
		vtxBuffer->unlock();
		return vtxBuffer;
	}

	VertexBuffer* MeshCreator::createVertexBufferFVF( DWORD FVF , unsigned vertexSize , unsigned numVertices )
	{
		VertexBuffer* vtxBuffer = new VertexBuffer;
		if ( !vtxBuffer->createFVF( getD3DDevice() , FVF , vertexSize , numVertices , mUsageSoftwareVertexProcess ) )
		{
			delete vtxBuffer;
			return nullptr;
		}
		return vtxBuffer;
	}

	VertexBuffer* MeshCreator::createVertexBufferFVF( DWORD FVF , void* v , int nV , VertexInfo const& info  , int nAddV /*= 0 */ )
	{
		int vertexSize = info.size;

		if ( info.type & CFV_COLOR )
		{
			if ( !( info.type & CFVF_COLOR_FMT_BYTE ) )
			{
				int colorSize = 3 * sizeof( float );
				if ( info.type & CFVF_COLOR_ALPHA )
					colorSize += 1 * sizeof( float );

				vertexSize -= colorSize;
				vertexSize += 4;
				if ( info.type & CFV_COLOR2 )
				{
					vertexSize -= colorSize;
					vertexSize += 4;
				}
			}
		}

		VertexBuffer* vtxBuffer = createVertexBufferFVF( FVF , vertexSize , nV + nAddV );
		if ( !vtxBuffer )
			return nullptr;

		char* buf =  static_cast< char* >( vtxBuffer->lock() );
		if ( !buf )
		{
			delete vtxBuffer;
			return nullptr;
		}

		VertexUtility::fillVertexFVF( buf , v , nV , info );

		vtxBuffer->unlock();
		return vtxBuffer;
	}

	IndexBuffer* MeshCreator::createIndexBuffer( int* idx , unsigned numIdx , bool useIntIndex )
	{
		return createIndexBuffer( idx , numIdx , sizeof( int ) , useIntIndex );
	}
	IndexBuffer* MeshCreator::createIndexBuffer( short* idx , unsigned numIdx , bool useIntIndex )
	{
		return createIndexBuffer( idx , numIdx , sizeof( short ) , useIntIndex );
	}

	IndexBuffer* MeshCreator::createIndexBuffer( void* idx , unsigned numIdx , unsigned idxSize , bool useIntIndex )
	{
		IndexBuffer* idxBuffer = new IndexBuffer;
		if ( !idxBuffer->create( getD3DDevice() , numIdx , useIntIndex , mUsageSoftwareVertexProcess ) )
		{
			delete idxBuffer;
			return nullptr;
		}

		unsigned size = ( useIntIndex ) ? 4 : 2;

		void* buf = idxBuffer->lock( 0 );
		if ( !buf )
		{
			delete idxBuffer;
			return nullptr;
		}

		if ( idxSize == size )
		{
			memcpy( buf , idx , numIdx * size );
		}
		else
		{
			if ( useIntIndex )
			{
				assert( idxSize == 2 );
				int*   dest = static_cast< int* >( buf );
				short* src  = static_cast< short* >( idx );
				std::copy( src , src + numIdx , dest );
			}
			else
			{
				assert( idxSize == 4 );

				short* dest = static_cast< short* >( buf );
				int*   src  = static_cast< int* >( idx );
				std::copy( src , src + numIdx , dest );
			}
		}
		return idxBuffer;
	}

	MeshBase* MeshCreator::createMesh( EPrimitive primitive , MeshBase* shapeVertexShared , int* idx , int numIdx  )
	{
		assert( idx );

		MeshBase* mesh = new MeshBase;
		mesh->_shareVertexData( *shapeVertexShared );
		mesh->mIndexBuffer   = createIndexBuffer( idx , numIdx , 4 , true );
		mesh->mPrimitiveType = primitive;

		switch( primitive )
		{
		case CFPT_POINTLIST:
			mesh->mNumElement = numIdx;
			break;
		case CFPT_LINELIST:
			mesh->mNumElement = numIdx / 2;
			break;
		case CFPT_LINESTRIP:
			mesh->mNumElement = numIdx - 1;
			break;
		case CFPT_TRIANGLELIST:
			mesh->mNumElement = numIdx / 3;
			break;
		case CFPT_TRIANGLESTRIP:
		case CFPT_TRIANGLEFAN:
			mesh->mNumElement = numIdx - 2;
			break;
		}
		return mesh;
	}


	MeshBase* MeshCreator::createMesh( MeshInfo const& info )
	{
		MeshBase* mesh = new MeshBase;

		mUsageSoftwareVertexProcess = ( info.flag & CFVD_SOFTWARE_PROCESSING ) != 0;

		VertexDeclFormat suggestFormat = VDF_TYPE_DEF;
		if ( info.flag & CFVD_FVF_FORMAT )
			suggestFormat = VDF_FVF;
		else if ( info.flag & CFVD_SINGLE_STREAM )
			suggestFormat = VDF_TYPE_DEF_SINGLE_STREAM;

		suggestFormat = VDF_TYPE_DEF;

		bool usageFVF = ( info.flag & CFVD_FVF_FORMAT ) != 0;

		VertexInfo vInfo;
		mesh->setupVertexDecl( info.vertexType , vInfo , suggestFormat );

		if ( mUsageSoftwareVertexProcess )
			getD3DDevice()->SetSoftwareVertexProcessing( TRUE );

		switch( mesh->mDeclFormat )
		{
		case VDF_FVF:
			{
				mesh->mVertexBuffer[0] = createVertexBufferFVF( mesh->mFVF , info.pVertex , info.numVertices , vInfo , 0 );
			}
			break;
		case VDF_TYPE_DEF_SINGLE_STREAM:
			{
				int vtxSize;
				mesh->mVertexDecl      = createVertexDeclSingleStream( info.vertexType , vtxSize );
				mesh->mVertexBuffer[0] = createVertexBuffer( vtxSize , info.numVertices );
			}
			break;
		case VDF_TYPE_DEF:
			{
				int groupPos[NUM_STREAM_GROUP];
				int groupSize[NUM_STREAM_GROUP];
				mesh->mVertexDecl = createVertexDeclMultiStream( info.vertexType , groupPos , groupSize );

				mesh->mVertexBuffer[ VSG_VERTEX ] = createVertexBuffer( groupSize[ VSG_VERTEX ] , info.numVertices , info.pVertex , groupPos[ VSG_VERTEX ] , vInfo.size );

				if ( groupSize[ VSG_GEOM ] )
					mesh->mVertexBuffer[ VSG_GEOM ] = createVertexBuffer( groupSize[ VSG_GEOM ] , info.numVertices , info.pVertex , groupPos[ VSG_GEOM ] , vInfo.size );

				if ( groupSize[ VSG_COLOR_TEX ] )
					mesh->mVertexBuffer[ VSG_COLOR_TEX ] = createVertexBuffer( groupSize[ VSG_COLOR_TEX ] , info.numVertices , info.pVertex , groupPos[ VSG_COLOR_TEX ] , vInfo.size );
			}
			break;
		}

		int numIndices;
		if ( info.numIndices )
		{
			numIndices = info.numIndices;
			int sizeIndex = ( info.isIntIndexType ) ? 4 : 2;
			mesh->mIndexBuffer   = createIndexBuffer( info.pIndex , info.numIndices , sizeIndex , true );
		}
		else
		{
			numIndices = info.numVertices;
		}

		if ( mUsageSoftwareVertexProcess )
			getD3DDevice()->SetSoftwareVertexProcessing( FALSE );

		mUsageSoftwareVertexProcess = false;

		mesh->mPrimitiveType = info.primitiveType;
		mesh->mNumVertex     = info.numVertices;

		switch( info.primitiveType )
		{
		case CFPT_POINTLIST:
			mesh->mNumElement = numIndices;
			break;
		case CFPT_LINELIST:
			mesh->mNumElement = numIndices / 2;
			break;
		case CFPT_LINESTRIP:
			mesh->mNumElement = numIndices - 1;
			break;
		case CFPT_TRIANGLELIST:
			mesh->mNumElement = numIndices / 3;
			break;
		case CFPT_TRIANGLESTRIP:
		case CFPT_TRIANGLEFAN:
			mesh->mNumElement = numIndices - 2;
			break;
		}


		return mesh;
	}


	MeshBase* MeshCreator::createLines( LineType type , VertexType vType , float* v , int nV )
	{
		MeshBase* mesh = new MeshBase;

		mesh->mNumVertex = nV;
		VertexInfo vInfo;
		mesh->setupVertexDecl( vType , vInfo , VDF_FVF );

		if ( mesh->mDeclFormat != VDF_FVF )
			goto fail;

		if ( type == CF_CLOSE_POLYLINE )
		{
			mesh->mVertexBuffer[0] = createVertexBufferFVF( mesh->mFVF , v , nV , vInfo , 1 );

			++mesh->mNumVertex;

			if ( mesh->mVertexBuffer[0] )
			{
				float* data = static_cast< float*>( mesh->mVertexBuffer[0]->lock( nV , 1 ) );
				if ( data )
				{
					data[0] = v[0];
					data[1] = v[1];
					data[2] = v[2];
					mesh->mVertexBuffer[0]->unlock();
				}
			}
		}
		else
		{
			mesh->mVertexBuffer[0] = createVertexBufferFVF( mesh->mFVF , v , nV , vInfo );
		}

		if ( !mesh->mVertexBuffer[0] )
			goto fail;


		switch ( type )
		{
		case CF_CLOSE_POLYLINE:
			mesh->mPrimitiveType = CFPT_LINESTRIP;
			mesh->mNumElement = nV + 1;
			break;
		case CF_OPEN_POLYLINE:
			mesh->mPrimitiveType = CFPT_LINESTRIP;
			mesh->mNumElement = nV;
			break;
		case CF_LINE_SEGMENTS:
			mesh->mPrimitiveType = CFPT_LINELIST;
			mesh->mNumElement = nV / 2;
			break;
		}

		return mesh;


fail:
		delete mesh;
		return nullptr;

	}


}//namespace CFly