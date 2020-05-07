#ifndef CFRenderShape_h__
#define CFRenderShape_h__

#include "CFBase.h"
#include "CFTypeDef.h"
#include "CFVertexDecl.h"
#include "CFGeomUtility.h"

namespace CFly
{
	class VertexBuffer;
	class IndexBuffer;
	class MeshCreator;
	struct VertexInfo;

	enum LineType
	{
		CF_LINE_SEGMENTS ,
		CF_OPEN_POLYLINE ,
		CF_CLOSE_POLYLINE ,
	};

	enum VertexDataFlag
	{
		CFVD_SOFTWARE_PROCESSING = BIT(0),
		CFVD_SINGLE_STREAM       = BIT(1),
		CFVD_FVF_FORMAT          = BIT(2),
	};

	struct MeshInfo
	{
		MeshInfo()
		{
			primitiveType = CFPT_UNKNOWN;
			vertexType = CFVT_UNKONWN;
			pVertex = nullptr;
			pIndex  = nullptr;
			isIntIndexType = false;
			flag   = 0;
			numVertices = 0;
			numIndices  = 0;
		}
		EPrimitive    primitiveType;
		VertexType    vertexType;
		void*         pVertex;
		int           numVertices;
		void*         pIndex;
		int           numIndices;
		bool          isIntIndexType;
		unsigned      flag;
	};


	enum StreamGroup
	{
		VSG_VERTEX         ,
		VSG_GEOM           ,
		VSG_COLOR_TEX      ,

		NUM_STREAM_GROUP
	};

	enum VertexDeclFormat
	{
		VDF_FVF ,
		VDF_TYPE_DEF ,
		VDF_TYPE_DEF_SINGLE_STREAM ,
		VDF_DECL ,
	};

	class IRenderable
	{
	public:
		//virtual void renderImpl( Matrix const& trans ) = 0;
	};

	class VertexData
	{







	};


	class MeshBase : public RefCountedObjectT< MeshBase >
		           //, public IRenderable
	{
	public:
		MeshBase();
		~MeshBase();

		MeshBase*  clone();
		MeshBase*  xform( Matrix4 const& trans , bool beAllInstance );

		EPrimitive getPrimitiveType(){ return mPrimitiveType; }
		void          calcBoundSphere( BoundSphere& sphere );
		unsigned      getVertexNum(){ return mNumVertex; }
		int           getTriangleNum();
		int           getElementNum(){  return mNumElement; }

		IndexBuffer*  getIndexBuffer(){ return mIndexBuffer; }
		VertexBuffer* getVertexElement( VertexElementSemantic element , unsigned& offset );

		void         addNormal();
		void         addTangentData( bool )
		{

		}
		void         addGemoData( bool haveNormal , bool haveTangent , bool haveBinormal )
		{

		}
		
		VertexType   getVertexType(){ return mVertexType; }

	public:
		void          _shareVertexData( MeshBase& other );
		void          _setupStream( RenderSystem* renderSystem , bool useShader );
		void          _renderPrimitive( RenderSystem* renderSystem );
		void          _setupDevice( D3DDevice* d3dDevice , unsigned& restOptionBit );
		VertexBuffer* _getVertexBuffer( int idx = 0 ){ return mVertexBuffer[idx]; }
		void          destroyThis();
	private:
		void          setupVertexDecl( VertexType type , VertexInfo& info , VertexDeclFormat suggestFormat );
		void          addNormalFVF();
		void          createVertexDeclFVF( D3DDevice* d3dDevice );

		TRefCountPtr<VertexBuffer> mVertexBuffer[ NUM_STREAM_GROUP ];
		TRefCountPtr<IndexBuffer>  mIndexBuffer;
		
		EPrimitive         mPrimitiveType;
		uint32                mNumElement;
		uint32                mNumVertex;


		VertexDeclFormat      mDeclFormat;
		D3DVertexDecl*        mVertexDecl;
		VertexType            mVertexType;
		union
		{
			VertexDecl*  mDecl;
			DWORD        mFVF;
		};
		MeshBase( MeshBase& rhs );
		
		friend class MeshCreator; 
	};

	

	class MeshCreator
	{
	public:
		MeshCreator( D3DDevice* d3dDevice);

		MeshBase*       createMesh( MeshInfo const& info );
		MeshBase*       createMesh( EPrimitive primitive , MeshBase* shapeVertexShared , int* idx , int numIdx );
		MeshBase*       createLines( LineType type , VertexType vType , float* v , int nV );
		
		VertexBuffer*   createVertexBufferFVF( DWORD FVF , unsigned vertexSize , unsigned numVertex );
		VertexBuffer*   createVertexBufferFVF( DWORD FVF , void* v , int nV , VertexInfo const& info , int nAddV = 0 );

		VertexBuffer*   createVertexBuffer( unsigned vertexSize , int numVertex );
		VertexBuffer*   createVertexBuffer( unsigned vertexSize , int numVertex , void* vertex , unsigned offset , unsigned stride  );

		IndexBuffer*    createIndexBuffer( void* idx , unsigned numIdx , unsigned  stride , bool useIntIndex );
		IndexBuffer*    createIndexBuffer( short* idx , unsigned numIdx , bool useIntIndex );
		IndexBuffer*    createIndexBuffer( int* idx , unsigned numIdx , bool useIntIndex );

	protected:
		D3DVertexDecl*  createVertexDeclSingleStream( VertexType type , int& vertexSize );
		D3DVertexDecl*  createVertexDeclMultiStream( VertexType type , int groupPos[] , int groupSize[] );
		D3DDevice*      getD3DDevice(){ return mD3dDevice; }
		
		bool          mUsageSoftwareVertexProcess;
		RenderSystem* mSystem;
		D3DDevice*    mD3dDevice;
	};


}//namespace CFly

#endif // CFRenderShape_h__
