#ifndef CFScene_h__
#define CFScene_h__

#include "CFBase.h"
#include "CFNodeBase.h"
#include "CFRenderOption.h"
#include "CFGeomUtility.h"

namespace CFly
{
	class SceneNode;
	class MeshCreator;
	class ShaderEffect;
	class ShaderParamSet;
	class MeshBase;
	class BillBoard;
	class ILoadListener;
	class IRenderable;
	class RenderNode;

	extern bool g_useObjFrushumClip;

	enum RenderFlag
	{
		CFRF_CLEAR_Z              = BIT(0),
		CFRF_CLEAR_BG_COLOR       = BIT(1),
		CFRF_CLEAR_STENCIL        = BIT(2),
		CFRF_LIGHTING_DISABLE     = BIT(3),
		CFRF_FRUSTUM_CLIP_DISABLE = BIT(4),
		CFRF_SAVE_RENDER_TARGET   = BIT(5),
		CFRF_DEFULT = CFRF_CLEAR_Z | CFRF_CLEAR_BG_COLOR,
	};

	class RenderQueue
	{
	public:
		//void classifyRenderGroup( IGeomShape* geom , bool isTrans , unsigned usageROption )
		//{
		//	IMaterial* material = geom->getMaterial();
		//	if( material->useShader() )
		//	{

		//	}
		//}
	};

	class ISceneRenderListener
	{
	public:
		virtual void onRenderNodeStart(){}
		virtual void onRenderNodeEnd(){}
	};


	class SceneNodeVisitor
	{
	public:
		virtual bool visit( SceneNode* node ) = 0;
	};

	class IRenderSceneManager
	{
	public:
		IRenderSceneManager( Scene* scene )
			:mScene( scene )
		{

		}
		virtual bool needRefreshListRequire( Camera* cam , Viewport* view );

		bool testVisible( Camera* cam );
		
		Scene* mScene;
	};

	class Scene : public Entity
		        , public NodeManager
	{
	public:
		Scene( World* world , int numGroup );
		~Scene();

		void       release();

		Object*    createObject( SceneNode* parent = nullptr );
		Camera*    createCamera( SceneNode* parent = nullptr );
		BillBoard* createBillBoard( SceneNode* parent = nullptr );
		Actor*     createActor( SceneNode* parent = nullptr );
		Light*     createLight( SceneNode* parent = nullptr );
		Sprite*    createSprite( SceneNode* parent = nullptr );


		Actor*     createActorFromFile( char const* fileName , char const* loaderName = nullptr );
		Object*    createSkyBox( char const* name , float size , bool isCubeMap = false );
		

		void       render( Camera* camera , Viewport* viewport , unsigned flag = CFRF_DEFULT  );
		void       render2D( Viewport* viewport , unsigned flag = CFRF_DEFULT  );
		void       renderNode( SceneNode* node , Matrix4 const& worldTrans , Camera* camera , Viewport* viewport , unsigned flag = CFRF_DEFULT );
		void       renderObject( Object* obj , Camera* camera , Viewport* viewport , unsigned flag = CFRF_DEFULT  );
		//void renderShape( PrimitiveType priType , VertexType vtxType , float* vtxData , int numVtx , void* idxData , int numIdx , bool UsageIntIndex , unsigned flag = CFRF_DEFULT )
		//{

		//}

		void  setSpriteWorldSize( int w , int h , int depth );
		void  destroyAllSceneNode();
		
		World*         getWorld(){ return mWorld; }

		D3DDevice*     getD3DDevice();
		RenderSystem*  _getRenderSystem();

		static SceneNode*  findSceneNode( unsigned id );

		Light*  findLightByName( char const* name );
		Camera* findCameraByName( char const* name );

		bool load( char const* fileName , char const* loaderName = nullptr , ILoadListener* listener = nullptr );
		void setAmbientLight( float* rgb );
		Color4f const& getAmbientLight(){ return mAmbientColor; }

		

		SceneNode*  getRoot(){ return mRoot; }

		void  visitSceneNode( SceneNodeVisitor& visitor );
		void  setRenderListener( ISceneRenderListener* listener ){ mRenderListener = listener;  }
	public:
		void  destroyNodeImpl( SceneNode* node );

		void  _destroyObject( Object* obj );
		void  _destroySprite( Sprite* spr );
		void  _destroyActor( Actor* actor );
		void  _destroyCamera( Camera* cam );
		void  _destroyLight( Light* light );

		void  _renderShadowMap( )
		{


		}

		void      _setupMaterial( Material* material , RenderMode mode , float opacity , unsigned& restOptionBit );
		void      _renderMesh( MeshBase* mesh , Material* mat , RenderMode mode , float opacity , unsigned& restOptionBit );
		void      _setupDefultRenderOption( unsigned renderOptionBit );
		void      _setupRenderOption( DWORD* renderOption , unsigned renderOptionBit );


		bool checkRenderGroup( SceneNode* node );

		

		Camera*   _getRenderCamera()
		{ 
			assert( mRenderCamera );
			return mRenderCamera; 
		}
		Viewport* _getRenderViewport()
		{ 
			assert( mRenderViewport ); 
			return mRenderViewport; 
		}

		Object*       _cloneObject( Object* obj , SceneNode* parent );
		Actor*        _cloneActor( Actor* actor , SceneNode* parent );

		void          _unlinkSceneNode( SceneNode* node );
		void          _linkSceneNode( SceneNode* node , SceneNode* parent );

		ShaderParamSet* getShaderParamSet(){ return mShaderParamSet; }
	private:
		template< class T >
		T*            cloneNode( T* ptr , SceneNode* parent );

		Material*     mReplaceMat;


		struct RenderGeom
		{
			RenderNode*  owner;
			IRenderable* unit;
			unsigned     priority;
		};
		typedef std::vector< RenderGeom > RenderGeomVec;
		RenderGeomVec mSortedRenderGeomVec;

		unsigned mSuppressRenderOptionBits;


		enum RenderGroup
		{
			RG_OPTICY_GROUP ,
			RG_TRANS_GROUP ,
			RG_ALL_GROUP ,
		};

		Viewport*             mRenderViewport;
		Camera*               mRenderCamera;
		ShaderParamSet*       mShaderParamSet;

		RenderGroup           mRenderGroup;
		typedef std::list< NodeBase* > NodeList;
		NodeList              mTransNodeList;
		ISceneRenderListener* mRenderListener;
		
		bool   prepareRender( Camera* camera , Viewport* viewport , unsigned flag );
		void   finishRender();


		typedef std::list< Light* > LightList;

		IDirect3DSurface9* mPrevRenderTarget;
		int                mNumSkin;
		int                mNumActor;
		int                mNumObject;

		Color4f            mAmbientColor;

	private:

		int            mNumLightUse;
		Light*         mUseLight[8];
		LightList      mLightList;


		bool           mRenderSprite;
		Camera*        mRender2DCam;

		int            mNumGroup;
		SceneNode*     mRoot;
		MeshCreator*   mGeomCreator;		
		World*         mWorld;

	};



	DEFINE_ENTITY_TYPE( Scene , ET_SCENE )

}//namespace CFly

#endif // CFScene_h__

