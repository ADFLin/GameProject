#ifndef CFSceneNode_h__
#define CFSceneNode_h__

#include "CFBase.h"
#include "CFSpatialNode.h"


namespace CFly
{
	class SceneNode;
	class RenderListener
	{
	public:
		virtual bool prevRender( SceneNode* obj ){ return true; }
		virtual void postRender( SceneNode* obj ){}
	};

	class SubNode
	{



	};

	class SceneNode : public SpatialNode
		            , public Entity
	{
		typedef SpatialNode BaseClass;
	public:

		enum
		{
			NODE_BLOCK_RENDER      = BaseClass::NEXT_NODE_FLAG ,
			NODE_TRANSLUCENT_OBJ   ,
			NODE_DISABLE_SHOW      ,
			NODE_FOLLOW_PARENT     ,
			NODE_DESTROYING        ,
			NODE_USE_MAT_OPACITY   ,
			NEXT_NODE_FLAG         ,
		};

		SceneNode( Scene* scene );
		SceneNode( SceneNode const& node );

		SceneNode* getParent() const { return static_cast< SceneNode* >( SpatialNode::getParent() ); }
		Scene*     getScene();
		SubNode*   getSubParent(){ return mSubParent; }

		void    renderAll( Matrix4 const& curMat );
		void    renderAll();

		void    render( Matrix4 const& trans );

		void    renderChildren();

		void    show( bool beV ){ enableFlag( NODE_DISABLE_SHOW , !beV ); }

		bool    isShow()       { return !checkFlag( NODE_DISABLE_SHOW );  }
		bool    isTranslucent(){ return  checkFlag( NODE_TRANSLUCENT_OBJ ); }

		void*   getUserData() const { return mUserData; }
		void    setUserData(void* val) { mUserData = val; }

		void    changeScene( Scene* other , bool beUnlink = true );

		void    setRenderGroup( int group )
		{
			mRenderGroup = group;
			changeOwnedChildrenRenderGroup( group );
		}

		int              getRenderGroup(){ return mRenderGroup; }
		RenderListener*  getRenderListener(){ return mRenderListener; }
		void             setRenderListener( RenderListener* listener ){ mRenderListener = listener; }
		
		//virtual  SceneNode* clone() = 0;

	public:
		void             _setSubParent( SubNode* node ){ mSubParent = node; }
		void             _setupRoot();
		void             _prevDestroy();

		bool             testVisible( Matrix4 const& trans );

		virtual void release(){ assert(0); }
	protected:
		virtual void releaseOwnedChildren(){}
		virtual void changeOwnedChildrenRenderGroup( int group ){}
		virtual void renderImpl( Matrix4 const& trans ){}
		virtual void onChangeScene( Scene* newScene ){}
		virtual void onRemoveChild( SceneNode* node ){}
		virtual bool doTestVisible( Matrix4 const& trans ){  return true;  }

		SubNode*          mSubParent;//
		int               mRenderGroup;
		RenderListener*   mRenderListener;	
		void*             mUserData;
		friend class Scene;
	};
}//namespace CFly

#endif // CFSceneNode_h__
