#ifndef CFBuffer_h__
#define CFBuffer_h__

#include "CFBase.h"

namespace CFly
{
	enum BufferLock
	{
		CFBL_READ_ONLY   = BIT(1),
		CFBL_DISCARD     = BIT(2),
	};

	class BufferBase
	{
	public:
	};



	class VertexBuffer : public BufferBase
		               , public RefObjectT< VertexBuffer >
	{
	public:
		VertexBuffer();
		~VertexBuffer();
		bool   createFVF( D3DDevice* device , DWORD FVF , unsigned vtxSize , unsigned numVtx , bool beSoftwareProcess = false )
		{  return createImpl( device , vtxSize , numVtx , FVF , beSoftwareProcess );  }
		bool   create( D3DDevice* device , unsigned vtxSize , unsigned numVtx  , bool beSoftwareProcess = false )
		{  return createImpl( device , vtxSize , numVtx , 0 , beSoftwareProcess );  }
		void*  lock( unsigned num = 0 , unsigned offset = 0 , unsigned flag = 0 );
		void   unlock();

		void   setupDevice( D3DDevice* device , UINT streamID );

		VertexBuffer*    clone();
		unsigned         getVertexSize() const { return mVtxSize; }
		unsigned         getVertexNum()  const { return mNumVtx;  }

		D3DVertexBuffer* getSystemImpl(){ return mVtxBuffer; }

	private:
		bool createImpl( D3DDevice* device , unsigned vtxSize , unsigned numVtx , DWORD FVF , bool beSoftwareProcess );
		unsigned          mNumVtx;
		unsigned          mVtxSize;
		D3DVertexBuffer*  mVtxBuffer;
		VertexBuffer( VertexBuffer& buffer );

	};

	class IndexBuffer : public BufferBase
		              , public RefObjectT< IndexBuffer >
	{
	public:
		IndexBuffer();
		~IndexBuffer();

		IndexBuffer* clone();
		bool         create( D3DDevice* device , unsigned numIdx , bool useInt32 , bool beSoftwareProcess = false );
		bool         isUseInt32(){ return mUseInt32; }
		unsigned     getIndexNum() const { return mNumIdx; }
		void*        lock( unsigned flag = 0 );
		void         unlock(){  mIndexBuffer->Unlock();  }

		D3DIndexBuffer*  getSystemImpl() const { return mIndexBuffer; }
		
	private:
		D3DIndexBuffer*  mIndexBuffer;
		unsigned         mNumIdx;
		bool             mUseInt32;
	};

	template< class Buffer >
	class Locker
	{
	public:
		Locker( Buffer* buffer )
			:mBuffer( buffer )
		{
			mData = mBuffer->lock();
		}

		template< class T1 >
		Locker( Buffer* buffer , T1 t1 )
			:mBuffer( buffer )
		{
			mData = mBuffer->lock( t1 );
		}

		template< class T1 , class T2 >
		Locker( Buffer* buffer , T1 t1 , T2 t2 )
			:mBuffer( buffer )
		{
			mData = mBuffer->lock( t1 , t2 );
		}

		template< class T1 , class T2 , class T3 >
		Locker( Buffer* buffer , T1 t1 , T2 t2 , T3 t3 )
			:mBuffer( buffer )
		{
			mData = mBuffer->lock( t1 , t2 , t3 );
		}
		~Locker()
		{
			if ( mData )
				mBuffer->unlock();
		}

		void* getData(){ return mData; }

		Buffer* mBuffer;
		void*   mData;
	};


}//namespace CFly

#endif // CFBuffer_h__