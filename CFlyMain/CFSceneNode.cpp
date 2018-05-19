#include "CFlyPCH.h"
#include "CFSceneNode.h"

#include "CFScene.h"

#include "CFActor.h"

namespace CFly
{
	SceneNode::SceneNode( Scene* scene ) 
		:SpatialNode( scene )
		,mUserData( nullptr )
		,mRenderListener( nullptr )
		,mSubParent( nullptr )
	{
		addFlag( NODE_WORLD_TRANS_DIRTY );
	}

	SceneNode::SceneNode( SceneNode const& node )
		:SpatialNode( node )
		,Entity()
		,mUserData( node.mUserData )
		,mRenderListener( node.mRenderListener )
		,mSubParent( node.mSubParent )
	{}

	void SceneNode::renderChildren()
	{
		for( NodeList::iterator iter = mChildren.begin();
			iter != mChildren.end() ; ++iter )
		{
			SceneNode* node = static_cast< SceneNode* >( *iter );
			if ( node->checkFlag( NODE_BLOCK_RENDER ) )
				continue;
			node->renderAll();
		}
	}

	void SceneNode::renderAll()
	{
		//CF_PROFILE( "renderSceneNode" );
		render( _getWorldTransformInternal( true ) );
		renderChildren();
	}

	void SceneNode::renderAll( Matrix4 const& curMat )
	{
		Matrix4 trans = getLocalTransform();
		FTransform::Transform( trans , curMat , mLinkTransOp );

		render( trans );

		for( NodeList::iterator iter = mChildren.begin();
			iter != mChildren.end() ; ++iter )
		{
			SceneNode* node = static_cast< SceneNode* >( *iter );

			if ( node->checkFlag( NODE_BLOCK_RENDER ) )
				continue;

			node->renderAll( trans );
		}
	}

	void SceneNode::render( Matrix4 const& trans )
	{
		{
			CF_PROFILE("TestGroupAndVisible");
			if ( !getScene()->checkRenderGroup( this ) )
				return;
			if ( !testVisible( trans ) )
				return;
		}

		RenderListener* listener = getRenderListener();

		if ( listener )
		{
			if ( listener->prevRender( this ) )
			{
				renderImpl( trans );
				listener->postRender( this );
			}
		}
		else
		{
			renderImpl( trans );
		}
	}


	Scene* SceneNode::getScene()
	{
		return static_cast< Scene* >( getManager() );
	}

	

	void SceneNode::changeScene( Scene* other , bool beUnlink )
	{
		if ( beUnlink )
		{
			getScene()->_unlinkSceneNode( this );
			other->addNode( this );
			other->_linkSceneNode( this , nullptr );
		}
		else
		{
			getScene()->removeNode( this );
			other->addNode( this );
		}
		onChangeScene( other );
	}

	void SceneNode::_setupRoot()
	{
		removeFlag( NODE_WORLD_TRANS_DIRTY );
		mCacheWorldTrans.setIdentity();
	}

	bool SceneNode::testVisible( Matrix4 const& trans )
	{
		if ( checkFlag( NODE_DISABLE_SHOW ) )
			return false;
		return doTestVisible( trans );
	}

	void SceneNode::_prevDestroy()
	{
		addFlag( NODE_DESTROYING );
		releaseOwnedChildren();
	}

}//namespace CFly