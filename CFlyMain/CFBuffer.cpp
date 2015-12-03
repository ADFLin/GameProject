#include "CFlyPCH.h"
#include "CFBuffer.h"


namespace CFly
{

	VertexBuffer::VertexBuffer()
	{
		mVtxSize = 0;
		mNumVtx  = 0;
		mVtxBuffer = nullptr;
	}

	VertexBuffer::~VertexBuffer()
	{
		if ( mVtxBuffer )
			mVtxBuffer->Release();
	}

	bool VertexBuffer::createImpl( D3DDevice* device , unsigned vtxSize  , unsigned numVtx , DWORD FVF , bool beSoftwareProcess )
	{
		if ( mVtxBuffer )
		{
			mVtxBuffer->Release();
			mVtxBuffer = nullptr;
		}

		mVtxSize = vtxSize;
		mNumVtx  = numVtx;

		//HRESULT hr = device->CreateVertexBuffer( vtxSize * numVtx , D3DUSAGE_WRITEONLY , FVF , D3DPOOL_DEFAULT , &mVtxBuffer , NULL );
		HRESULT hr = device->CreateVertexBuffer( 
			vtxSize * numVtx , 
			beSoftwareProcess ? D3DUSAGE_SOFTWAREPROCESSING : 0 , 
			FVF , D3DPOOL_MANAGED , &mVtxBuffer , NULL );

		if ( hr != D3D_OK )
		{
			switch( hr )
			{
			case D3DERR_INVALIDCALL:
				break;
			case D3DERR_OUTOFVIDEOMEMORY:
				break;
			case E_OUTOFMEMORY:
				break;
			}
			return false;
		}

		return true;
	}

	void* VertexBuffer::lock( unsigned num , unsigned offset , unsigned flag )
	{
		if ( !num )
			num = mNumVtx;

		unsigned d3dFlag = 0;
		if ( flag & CFBL_DISCARD )
			d3dFlag |= D3DLOCK_DISCARD;
		if ( flag & CFBL_READ_ONLY )
			d3dFlag |= D3DLOCK_READONLY;

		void* vtx;
		HRESULT hr = mVtxBuffer->Lock( mVtxSize * offset , mVtxSize * num , (void**)&vtx , d3dFlag );

		if ( hr != D3D_OK )
		{
			return nullptr;
		}

		return vtx;
	}

	void VertexBuffer::unlock()
	{
		mVtxBuffer->Unlock();
	}

	VertexBuffer* VertexBuffer::clone()
	{
		VertexBuffer* cBuffer = new VertexBuffer;

		D3DVERTEXBUFFER_DESC desc;
		getSystemImpl()->GetDesc( &desc );
		bool bSoftwareProc = ( desc.Usage & D3DUSAGE_SOFTWAREPROCESSING ) != 0;

		D3DDevice* d3dDevice;
		getSystemImpl()->GetDevice( &d3dDevice );
		if ( !cBuffer->createImpl(  d3dDevice , mVtxSize , mNumVtx  , desc.FVF , bSoftwareProc ) )
			return nullptr;

		float* src = static_cast< float*>( lock( CFBL_READ_ONLY ) );
		float* dest = static_cast< float*>( cBuffer->lock() );

		if ( src && dest )
		{
			memcpy(  dest , src , mNumVtx * mVtxSize );
		}

		unlock();
		cBuffer->unlock();

		return cBuffer;
	}

	void VertexBuffer::setupDevice( D3DDevice* device , UINT streamID )
	{
		device->SetStreamSource( streamID , mVtxBuffer , 0 , mVtxSize );
	}

	IndexBuffer::IndexBuffer() 
		:mIndexBuffer( nullptr )
		,mNumIdx(0)
	{

	}

	IndexBuffer::~IndexBuffer()
	{
		if ( mIndexBuffer )
			mIndexBuffer->Release();
	}

	void* IndexBuffer::lock( unsigned flag )
	{
		if ( !mIndexBuffer )
			return nullptr;

		unsigned d3dFlag = 0;
		if ( flag & CFBL_DISCARD )
			d3dFlag |= D3DLOCK_DISCARD;
		if ( flag & CFBL_READ_ONLY )
			d3dFlag |= D3DLOCK_READONLY;


		void* idx;
		HRESULT hr = mIndexBuffer->Lock( 0 , mNumIdx , (void**)&idx , d3dFlag );
		if ( hr != D3D_OK )
		{
			return nullptr;
		}
		return idx;
	}

	bool IndexBuffer::create( D3DDevice* device , unsigned numIdx , bool useInt32 , bool beSoftwareProcess )
	{
		if ( mIndexBuffer )
			mIndexBuffer->Release();

		mNumIdx   = numIdx;
		mUseInt32 = useInt32;

		unsigned len = numIdx * ( ( useInt32 ) ? 4 : 2 );
		HRESULT hr = device->CreateIndexBuffer( 
			len , 
			beSoftwareProcess ? D3DUSAGE_SOFTWAREPROCESSING : 0  , 
			( useInt32 )? D3DFMT_INDEX32 : D3DFMT_INDEX16 , 
			D3DPOOL_DEFAULT , &mIndexBuffer , NULL );

		if ( hr != D3D_OK )
		{
			return false;
		}

		return true;
	}

	IndexBuffer* IndexBuffer::clone()
	{
		IndexBuffer* cBuffer = new IndexBuffer;

		D3DINDEXBUFFER_DESC desc;
		getSystemImpl()->GetDesc( &desc );

		bool bSoftwareProc = ( desc.Usage & D3DUSAGE_SOFTWAREPROCESSING ) != 0;

		D3DDevice* d3dDevice;
		getSystemImpl()->GetDevice( &d3dDevice );
		cBuffer->create( d3dDevice , mNumIdx , mUseInt32 , bSoftwareProc );

		unsigned len = mNumIdx;
		len *= ( mUseInt32 ) ? 4 : 2;

		void* dest = cBuffer->lock( );
		void* src  = lock( CFBL_READ_ONLY );

		if ( dest && src )
		{
			memcpy( dest , src , len );
		}
		else
		{

		}

		cBuffer->unlock();
		unlock();

		return cBuffer;
	}


}//namespace CFly