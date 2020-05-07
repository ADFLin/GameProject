#ifndef CFObject_h__
#define CFObject_h__

#include "CFBase.h"
#include "CFSceneNode.h"
#include "CFRenderOption.h"
#include "CFMaterial.h"
#include "CFBuffer.h"
#include "CFGeomUtility.h"
#include "CFAnimatable.h"
#include "CFRenderShape.h"

namespace CFly
{
	class IRenderNode : public SceneNode
	{
	public:
		IRenderNode( Scene* scene );
		IRenderNode( IRenderNode const& node );

		void         setRenderOption( RenderOption option , DWORD val );
		DWORD        getRenderOption( RenderOption option ){ return mRenderOption[ option ]; }
		float        getOpacity() const { return mOpacity; }
		void         setOpacity(float val) { mOpacity = val; }
		
		void         enableVisibleTest( bool beE );
		virtual void updateBoundSphere(){}
		virtual IRenderNode* instance( bool beCopy = false ) = 0;

	public:
		bool       doTestVisible( Matrix4 const& trans );
	protected:


		RenderMode  mRenderMode;
		bool        mUseBoundSphere;
		BoundSphere mBoundSphere;
		float       mOpacity;
		unsigned    mUsageRenderOption;
		DWORD       mRenderOption[ CFRO_OPTION_NUM ];

	};

	struct RenderUnit
	{
		TRefCountPtr< MeshBase > mesh;
		TRefCountPtr< Material > material;
		unsigned  flag;
	};

	class Object : public IRenderNode
		         , public IAnimatable
	{
	public:
		Object( Scene* scene );
		Object( Object const& obj );

		virtual void  release();
		bool load( char const* fileName , char const* loaderName = nullptr );

		int createMesh( Material* mat , MeshInfo const& info );

		int createMesh(
			Material* mat , 
			EPrimitive primitive , 
			MeshBase* shapeVertexShared ,
			int* idx = nullptr , int numIdx = 0 );

		int createIndexedTriangle( 
			Material* mat , VertexType type ,
			void* v , int nV , int* tri ,  int nT , unsigned flag = 0 );

		int createLines( Material* mat , LineType type , VertexType vType , float* v , int nV );
		int createLines( Material* mat , LineType type , float* v , int nV ){ return createLines( mat , type , CFVT_XYZ , v , nV );  }

		int createPlane( Material* mat , float w , float h , float* color = nullptr, 
			            float xTile = 1.0f , float yTile = 1.0f , 
						Vector3 const& normal = Vector3( 0,0,1) , 
						Vector3 const& alignDir = Vector3( 1,0,0 ) ,
						Vector3 const& offset = Vector3(0,0,0) , bool invYDir = false );

		void        updateAnimation( float dt );
		Object*     instance( bool beCopy = false );
		void        xform( bool beAllInstance = false );
		bool        removeElement( int id );
		void        removeAllElement();


		class Element : private RenderUnit
		{
		public:
			MeshBase* getMesh();
			void      setMaterial( Material* mat );
			Material* getMaterial();
		private:
			RenderUnit& getUnit(){ return *this; }
			void clear()
			{
				mesh     = nullptr;
				material = nullptr;
			}
			friend class Object;
			int      mNext;
		};

		Element*     getElement( int id = 0 );
		size_t       getElementNum() const { return mNumElements; }
		void         setRenderMode( RenderMode mode ){ mRenderMode = mode; }
		RenderMode   getRenderMode(){ return mRenderMode; }

		void         addVertexNormal();
		
		void         useMaterialOpacity( bool val ) { enableFlag( NODE_USE_MAT_OPACITY , val ); }
	public:

		float       _evalOpacity( Material* mat );
		
		virtual void _cloneShareData();
		void         _renderInternal();
	protected:
		void         renderImpl( Matrix4 const& trans );
		
		void         updateBoundSphere();

		typedef std::vector< TRefCountPtr<MeshBase> > ShapeVector;
		typedef std::vector< Element > ElementVec;

	public:
		class MeshIterator
		{
		public:
			MeshIterator( ElementVec& elements , int idx )
				:mElements( elements ) , mIdxCur( idx ){}

			Element*   get(){ return ( mIdxCur != -1 ) ? &mElements[ mIdxCur ] : nullptr; }
			bool       haveMore(){ return mIdxCur != -1; }
			void       next(){ mIdxCur = mElements[ mIdxCur ].mNext; }

		private:
			ElementVec& mElements;
			int         mIdxCur;

		};
		MeshIterator getElements(){ return MeshIterator( mElements , mIdxUsed ); }

	protected:

		int addElement( MeshBase* mesh , Material* mat , unsigned flag = 0 );
		
		MotionKeyFrame*   mMotionData;
		ElementVec        mElements;
		int               mIdxUsed;
		int               mIdxUnused;
		int               mNumElements;
	};


	class SkyBox : public Object
	{
	public:
		SkyBox( Scene* scene );
		void renderImpl( Matrix4 const& trans );
	};

	class BillBoard : public Object
	{
	public:
		enum BoardType
		{
			BT_SPHERIAL ,
			BT_CYLINDICAL_X ,
			BT_CYLINDICAL_Z ,
			BT_CYLINDICAL_Y ,
		};

		BillBoard( Scene* scene )
			:Object( scene )
			,mBoardType( BT_SPHERIAL )
		{}

		int createBillBoard( Vector3 const& pos , float w , float h, Color4f const* color , char const* tex , int numTex  , Color3ub const* colorKey , float roll = 0.0f )
		{
			return CF_FAIL_ID;
		}
		void setBoardType( BoardType type ){  mBoardType = type; }
		void renderImpl( Matrix4 const& trans );

		BoardType mBoardType;
	};

	DEFINE_ENTITY_TYPE( Object    , ET_OBJECT )
	DEFINE_ENTITY_TYPE( SkyBox    , ET_OBJECT )
	DEFINE_ENTITY_TYPE( BillBoard , ET_OBJECT )


}//namespace CFly
#endif // CFObject_h__