#include "CFlyPCH.h"
#include "CFActor.h"

#include "CFScene.h"
#include "CFWorld.h"
#include "CFMaterial.h"
#include "CFRenderSystem.h"

#include <algorithm>

namespace CFly
{
	Actor::Actor( Scene* scene ) 
		:SceneNode( scene )
		//,mBoneTransDirty( true )
	{
		mSkeleton = new Skeleton;
		mBoneLine = createSkin();
	}

	Actor::Actor( Actor const& actor ) 
		:BaseClass( actor )
		,BlendAnimation( actor )
		,mSkeleton( actor.mSkeleton )
		,mAttachVec( actor.mAttachVec )
		,mBoneTransVec( actor.mBoneTransVec )
		//,mBoneTransDirty( actor.mBoneTransDirty )
	{
		for( SkinVec::const_iterator iter = actor.mSkinVec.begin();
			iter != actor.mSkinVec.end() ; ++iter )
		{
			mSkinVec.push_back( getScene()->_cloneObject( *iter , this ) );
		}
		for( AttachInfoVec::iterator iter = mAttachVec.begin();
			iter != mAttachVec.end() ; ++iter )
		{
			iter->obj = getScene()->_cloneObject( iter->obj , this );
		}
	}


	Actor::~Actor()
	{

	}

	Object* Actor::createSkin()
	{
		Object* skin = getScene()->createObject( this );
		skin->addFlagBits( SkinFlagBits );
		mSkinVec.push_back( skin );

		return skin;
	}

	void Actor::updateBoneMatrix( float frame )
	{
		int nFrame = (int)frame;
		getSkeleton()->generateBoneTransform( 
			&mBoneTransVec[0] ,
			//getWorldTransform() ,
			Matrix4::Identity() ,
		    nFrame , frame - nFrame );
		//updateBoneLine( frame );
	}

	struct BoneLineVertex
	{
		static uint32 const Type = CFVT_XYZ | CFV_BLENDINDICES;
		Vector3 pos;
		uint32  boneIndex;
	};

	void Actor::updateBoneLine( float frame )
	{
		RefPtrT< Material > mat;

		Object::Element* ele = mBoneLine->getElement(0);
		if ( ele )
		{
			mat = ele->getMaterial();
			mBoneLine->removeElement(0);
		}

		if ( !mat )
		{
			mat = getScene()->getWorld()->createMaterial( NULL , NULL , NULL , 1.0 , Color3f(1,1,1) );
		}

		std::vector< Vector3 > lineVec;
		getSkeleton()->generateBoneLine( lineVec , frame );
		mBoneLine->createLines( mat , CF_LINE_SEGMENTS , (float*) &lineVec[0] , lineVec.size() );

	}

	void Actor::updateSkeleton( bool genInvLocalTrans )
	{
		if ( genInvLocalTrans )
		{
			mSkeleton->generateInvLoacalTransform();
		}
		mBoneTransVec.resize( mSkeleton->getBoneNum() );
		updateBoneMatrix( 0 );
	}

	void Actor::renderImpl( Matrix4 const& trans )
	{

		RenderSystem* renderSys = getScene()->_getRenderSystem();
		D3DDevice* d3dDevice = renderSys->getD3DDevice();
	
		HRESULT hr = d3dDevice->SetSoftwareVertexProcessing( TRUE );

		if ( checkFlag( NODE_DISABLE_SKELETON ) )
		{
			for (int i = 0 ; i < mBoneTransVec.size() ; ++i )
			{
				Matrix4 worldTrans;
				TransformUntility::mul( worldTrans , mBoneTransVec[i] , trans );
				renderSys->setWorldMatrix( i , worldTrans );
			}
		}
		else
		{
			getSkeleton()->setupBoneWorldTransform( renderSys , trans , getBoneTransMatrix() );
		}
		for( SkinVec::iterator iter = mSkinVec.begin();
			iter != mSkinVec.end(); ++iter )
		{
			(*iter)->_renderInternal();
		}
		d3dDevice->SetSoftwareVertexProcessing( FALSE );

		if ( &trans == &getWorldTransform() )
		{
			for( int i = 0 ; i < mAttachVec.size(); ++i )
			{
				AttachInfo& info = mAttachVec[i];
				if ( !info.obj )
					continue;

				info.obj->renderAll();
			}
		}
		else
		{
			for( int i = 0 ; i < mAttachVec.size(); ++i )
			{
				AttachInfo& info = mAttachVec[i];
				if ( !info.obj )
					continue;

				BoneNode* bone = static_cast< BoneNode* >( info.obj->getSubParent() );
				Matrix4 newTrans =  mBoneTransVec[ bone->id ] * trans;
				info.obj->renderAll( newTrans );
			}
		}

		//boneLine->render();
	}


	int Actor::applyAttachment( Object* obj , BoneNode* bone , TransOp op )
	{
		obj->setParent( this );
		obj->addFlagBits( AttachFlagBits );
		obj->setLinkTransOperator( op );
		obj->_setSubParent( bone );

		int slot = getEmptyAttchSlot();

		AttachInfo& info = mAttachVec[slot];
		info.obj    = obj;

		return slot;
	}

	int Actor::applyAttachment( Object* obj , char const* boneName , TransOp op )
	{
		BoneNode* bone = getSkeleton()->getBone( boneName );

		if ( !bone )
			return CF_FAIL_ID;

		return applyAttachment( obj , bone , op );
	}

	int Actor::applyAttachment( Object* obj , int boneID , TransOp op  )
	{
		BoneNode* bone = getSkeleton()->getBone( boneID );
		if ( !bone )
			return CF_FAIL_ID;

		return applyAttachment( obj , bone , op );
	}

	int Actor::getEmptyAttchSlot()
	{
		for( int i = 0; i < mAttachVec.size();++i)
		{
			if ( mAttachVec[i].obj == NULL )
				return i;
		}
		mAttachVec.push_back( AttachInfo() );
		return mAttachVec.size() - 1;
	}

	void Actor::takeOffAttachment( Object* obj )
	{
		AttachInfo* info = findAttachment( obj );
		if ( info )
		{
			info->obj->setParent( getScene()->getRoot() );
			removeAttachment( *info );
		}
	}

	void Actor::removeAttachment( AttachInfo& info )
	{
		info.obj->removeFlagBits( AttachFlagBits );
		info.obj->_setSubParent( nullptr );
		info.obj = nullptr;
	}

	Actor::AttachInfo* Actor::findAttachment( Object* obj )
	{
		for( AttachInfoVec::iterator iter = mAttachVec.begin();
			iter != mAttachVec.end() ; ++iter )
		{
			AttachInfo& info = *iter;
			if ( obj == info.obj )
				return &info;
		}
		return nullptr;
	}

	int Actor::getAttachmentParentBoneID( int idx )
	{
		if ( idx >= mAttachVec.size() )
			return CF_FAIL_ID;

		if ( mAttachVec[ idx ].obj == NULL )
			return CF_FAIL_ID;

		BoneNode* bone = static_cast< BoneNode* >( mAttachVec[ idx ].obj->getSubParent() );
		return bone->id;
	}

	void Actor::_cloneShareData()
	{
		mSkeleton = mSkeleton->clone();

		for( SkinVec::const_iterator iter = mSkinVec.begin();
			iter != mSkinVec.end() ; ++iter )
		{
			(*iter)->_cloneShareData();
		}
		for( AttachInfoVec::iterator iter = mAttachVec.begin();
			iter != mAttachVec.end() ; ++iter )
		{
			AttachInfo& info = *iter;
			if ( !info.obj )
				continue;
			info.obj->_cloneShareData();
		}
	}

	void Actor::changeOwnedChildrenRenderGroup( int group )
	{
		for( SkinVec::iterator iter = mSkinVec.begin();
			iter != mSkinVec.end() ; ++iter )
		{
			(*iter)->setRenderGroup( group );
		}

		for( AttachInfoVec::iterator iter = mAttachVec.begin();
			iter != mAttachVec.end() ; ++iter )
		{
			AttachInfo& info = *iter;
			if ( !info.obj )
				continue;
			info.obj->setRenderGroup( group );
		}
	}

	void Actor::doUpdateAnimation( float* frames , float* weights , int num )
	{
		getSkeleton()->generateBoneTransform( 
			&mBoneTransVec[0] , 
			Matrix4::Identity() ,
			frames , weights , num );

		_updateAllAttachTransform();
	}

	void Actor::updateSubChildTransform( SpatialNode* obj )
	{
		assert( obj->checkFlag( NODE_SUB_CHILD ) && obj->getParent() == this );
		Matrix4 const& worldTrans = getWorldTransform();
		BoneNode* bone = static_cast< BoneNode* >( static_cast< SceneNode* >( obj )->getSubParent() );
		obj->_updateWorldTransformOnBone( mBoneTransVec[ bone->id ] * worldTrans );
	}

	void Actor::_updateAllAttachTransform()
	{
		Matrix4 const& worldTrans = getWorldTransform();
		for( AttachInfoVec::iterator iter = mAttachVec.begin();
			 iter != mAttachVec.end() ; ++iter )
		{
			AttachInfo& info = *iter;
			if ( !info.obj )
				continue;
			BoneNode* bone = static_cast< BoneNode* >( info.obj->getSubParent() );
			info.obj->_updateWorldTransformOnBone( mBoneTransVec[ bone->id ] * worldTrans );
		}
	}

	void Actor::onModifyTransform( bool beLocal )
	{
		BaseClass::onModifyTransform( beLocal );
		_updateAllAttachTransform();
	}

	void Actor::onChangeScene( Scene* newScene )
	{
		for( SkinVec::iterator iter = mSkinVec.begin();
			iter != mSkinVec.end(); ++iter )
		{
			(*iter)->changeScene( newScene , false );
			(*iter)->setParent( this );
		}

		for( AttachInfoVec::iterator iter = mAttachVec.begin();
			iter != mAttachVec.end() ; ++iter )
		{
			AttachInfo& info = *iter;
			if ( !info.obj )
				continue;

			info.obj->changeScene( newScene , false );
			info.obj->setParent( this );
		}
	}

	void Actor::onRemoveChild( SceneNode* node )
	{
		if ( checkFlag( NODE_DESTROYING ) )
			return;

		if ( node->checkFlag( NODE_SKIN_MARK ) )
		{
			for( SkinVec::iterator iter = mSkinVec.begin();
				iter != mSkinVec.end() ; ++iter )
			{
				if ( node != *iter )
					continue;
				mSkinVec.erase( iter );
				return;
			}
		}
		else if ( node->checkFlag( NODE_ATTACHMENT_MARK ) )
		{
			AttachInfo* info = findAttachment( static_cast< Object* >( node ) );
			if ( info )
				removeAttachment( *info );
		}
	}

	Actor* Actor::instance( bool beCopy )
	{
		Actor* cActor = getScene()->_cloneActor( this , getParent() );

		if ( !beCopy )
			return cActor;

		cActor->_cloneShareData();
		return cActor;
	}

	void Actor::release()
	{
		getScene()->_destroyActor( this );
	}

	int Actor::getBoneID( char const* name )
	{
		BoneNode* bone = getSkeleton()->getBone( name );
		if ( bone )
			return bone->id;

		return CF_FAIL_ID;
	}

	Object* Actor::getAttachment( int idx )
	{
		if ( idx >= mAttachVec.size() )
			return nullptr;

		return mAttachVec[ idx ].obj;

	}

	void Actor::setOpacity( float val )
	{
		for( AttachInfoVec::iterator iter = mAttachVec.begin();
			iter != mAttachVec.end() ; ++iter )
		{
			AttachInfo& info = *iter;
			info.obj->setOpacity( val );
		}
		for( SkinVec::iterator iter = mSkinVec.begin();
			iter != mSkinVec.end(); ++iter )
		{
			(*iter)->setOpacity( val );
		}

	}

	void Actor::setBoneTransSize( int size )
	{
		mBoneTransVec.resize( size , Matrix4::Identity() );
	}

	void Actor::releaseOwnedChildren()
	{
		BaseClass::releaseOwnedChildren();

		for( int i = 0 ; i < mSkinVec.size(); ++i )
		{
			mSkinVec[i]->release();
		}
		mSkinVec.clear();

		for( int i = 0 ; i < mAttachVec.size(); ++i )
		{
			mAttachVec[i].obj->release();
		}
		mAttachVec.clear();
	}

	BlendAnimation::BlendAnimation()
	{

	}

	BlendAnimation::BlendAnimation( BlendAnimation const& ba )
	{
		for( AnimationStateMap::const_iterator iter = ba.mAnimationMap.begin();
			iter != ba.mAnimationMap.end(); ++iter )
		{
			AnimationState* copyState = iter->second->clone( this );
			mAnimationMap.insert( std::make_pair( copyState->getName() , copyState ) );	
		}
	}

	BlendAnimation::~BlendAnimation()
	{
		AnimationStateMap::iterator itEnd = mAnimationMap.end();
		for ( AnimationStateMap::iterator iter = mAnimationMap.begin();
			iter != itEnd ; ++iter )
		{
			delete iter->second;
		}
	}

	void BlendAnimation::updateAnimation( float offset  )
	{
		float frames[ MaxBlendAnimateNum ];
		float weights[ MaxBlendAnimateNum ];
		int num = 0;

		for( AnimationStateList::iterator iter( mActiveAnimList.begin() ) , itEnd( mActiveAnimList.end() );
			iter != itEnd ; ++iter )
		{
			AnimationState* state = *iter;
			frames [ num ] = state->updateTimePos( offset );
			weights[ num ] = state->getWeight();
			++num;
			if ( num >= MaxBlendAnimateNum )
				break;
		}

		if ( num )
			doUpdateAnimation( frames , weights , num );
	}

	void BlendAnimation::_updateStateActive( AnimationState* state )
	{
		if ( state->isEnable() )
		{
			mActiveAnimList.push_back( state );		
		}
		else
		{
			mActiveAnimList.remove( state );
		}
	}

	AnimationState* BlendAnimation::getAnimationState( char const* name )
	{
		AnimationStateMap::iterator iter = mAnimationMap.find( name );
		if ( iter == mAnimationMap.end() )
			return nullptr;
		return iter->second;
	}

	AnimationState* BlendAnimation::createAnimationState( char const* name , int start ,int end )
	{
		AnimationState* state = new AnimationState( this , name , start , end );
		mAnimationMap.insert( std::make_pair( state->getName() , state ) );
		return state;
	}

	AnimationState::AnimationState( BlendAnimation* animatable , char const* name , int start , int end ) 
		:mManager(animatable)
		,mName( name )
		,mStart( start )
		,mEnd( end )
		,mTimeScale( 1.0f )
	{
		mLength = end - start;
		mWeight = 0.0f;
		mTimePos = 0.0f;
		mBeLoop = false;
		mEnable = false;
	}

	void AnimationState::enable( bool beE )
	{
		if ( mEnable == beE )
			return;

		mEnable = beE;
		mManager->_updateStateActive( this );
	}

	float AnimationState::updateTimePos( float offset )
	{
		mTimePos += mTimeScale * offset;

		if ( mBeLoop )
		{
			mTimePos = Math::Fmod( mTimePos , mLength );
			if ( mTimePos < 0.0f )
				mTimePos += mLength;

			assert( mStart + mTimePos <= mEnd  );
		}
		else if ( mTimePos < 0 )
		{
			return mStart;
		}
		else if ( mTimePos > mLength )
		{
			return mEnd;
		}
		return mStart + mTimePos;
	}

	AnimationState* AnimationState::clone( BlendAnimation* animatable ) const
	{
		AnimationState* cloneAS = new AnimationState( animatable , mName.c_str() , mStart , mEnd );

		cloneAS->setTimeScale( mTimeScale );
		cloneAS->setWeight( mWeight );
		cloneAS->enable( mEnable );
		cloneAS->enableLoop( mBeLoop );
		return cloneAS;
	}

	void AnimationState::assignSetting( AnimationState& other )
	{
		mBeLoop    = other.mBeLoop;
		mStart     = other.mStart;
		mEnd       = other.mEnd;
		mLength    = other.mLength;
		mTimeScale = other.mTimeScale;
		mWeight    = other.mWeight;
		mTimePos    = other.mTimePos;
	}


}//namespace CFly