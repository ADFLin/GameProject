#include "CFlyPCH.h"
#include "CFSkeleton.h"
#include "CFRenderSystem.h"


namespace CFly
{
	class EmptyBoneControllerManager : public IBoneController
	{
	public:
		//virtual 
		void setParamValue( int idxParam , float* value ){}
	};

	static EmptyBoneControllerManager gEmptyBoneControllerMgr;


	BoneNode::BoneNode() 
		:invLocalTrans( Matrix4::Identity() )
	{
		keyFrames = nullptr;
	}

	BoneNode::BoneNode( BoneNode const& rhs ) :id( rhs.id )
		,parentId( rhs.parentId )
		,keyFrames( rhs.keyFrames )
		,mKeyFormat( rhs.mKeyFormat )
		,mName( rhs.mName )
	{

	}


	void BoneNode::interpolationMotion( Vector3& pos , Quaternion& rotate , int frame  , float t )
	{
		assert( keyFrames );

		MotionKeyFrame& frame0 = keyFrames[ frame ];
		MotionKeyFrame& frame1 = keyFrames[ frame + 1 ];

		if ( t == 0 )
		{
			pos    = frame0.pos;
			rotate = frame0.rotation ;
		}
		else
		{
			pos    = lerp( frame0.pos , frame1.pos , t );
			rotate = slerp( frame0.rotation , frame1.rotation , t );
		}
	}

	void BoneNode::calcFrameTransform( Matrix4& result , int frame )
	{
		MotionKeyFrame& keyFrame = keyFrames[ frame ];
		result.setTransform( keyFrame.pos , keyFrame.rotation );
	}

	void BoneNode::calcFrameTransform( Matrix4& result , int frame0 , int frame1 , float t )
	{
		assert( keyFrames );

		MotionKeyFrame& keyFrame0 = keyFrames[ frame0 ];
		MotionKeyFrame& keyFrame1 = keyFrames[ frame1 ];

		Quaternion q = slerp( keyFrame0.rotation , keyFrame1.rotation , t );
		Vector3    pos = lerp( keyFrame0.pos , keyFrame1.pos , t );

		result.setTransform( pos , q );
	}


	void BoneNode::calcFrameTransform( Matrix4& result , float* frame , float* weights , int num )
	{
		assert( keyFrames );

		assert( num != 0 );
		int curFrame = (int)frame[0];
		Quaternion accRotation;
		Vector3    accPos; 
		interpolationMotion( accPos , accRotation , curFrame , frame[0] - curFrame );

		float accWeight = weights[0];
		for( int i = 1 ; i < num ; ++i )
		{
			Quaternion rotate;
			Vector3    pos; 
			curFrame = (int)frame[i];
			interpolationMotion( pos , rotate , curFrame , frame[i] - curFrame );

			accWeight += weights[i];
			float t = weights[i] / accWeight;
			accRotation = slerp( accRotation , rotate , t  );
			accPos    = lerp( accPos , pos , t );
		}

		result.setTransform( accPos , accRotation );
	}


	void BoneNode::generateBoneLine( std::vector< Vector3 >& lineVec , Matrix4 const& trans , int frame )
	{
#if 0
		Matrix4 nextMat;
		calcFrameTransform( nextMat , trans , frame );

		Vector3 org = motionData[0].pos * trans;

		for( NodeList::iterator iter ( mChildren.begin() ) , end( mChildren.end() );
			iter != end ; ++iter )
		{
			BoneNode* bone = static_cast< BoneNode* >( CONV(iter) );
			Vector3 pos =  bone->motionData[0].pos * nextMat;

			lineVec.push_back( org );
			lineVec.push_back( pos );
		}

		for( NodeList::iterator iter ( mChildren.begin() ) , end( mChildren.end() );
			iter != end ; ++iter )
		{
			castBone( CONV(iter) )->generateBoneLine( lineVec , nextMat , frame );
		}
#endif
	}



	Skeleton::Skeleton()
	{
		mController = &gEmptyBoneControllerMgr;
		mUseInvLocalTrans = true;
		mTotalKeyFrame = 0;
		mBaseBone = createBone( CF_BASE_BONE_NAME , BoneId(0) );
	}

	Skeleton::Skeleton( Skeleton const& rhs ) 
		:mNameMap( rhs.mNameMap )
		,mUseInvLocalTrans( rhs.mUseInvLocalTrans )
		,mTotalKeyFrame( rhs.mTotalKeyFrame )
		,mBaseBone( rhs.mBaseBone )
		,mController( rhs.mController )
		,mBoneVec( rhs.mBoneVec )
	{
		for( BoneMap::iterator iter( mNameMap.begin() ) ,end( mNameMap.end() ); 
			iter != end ; ++iter )
		{
			int id = iter->second;
			mBoneVec[id] = new BoneNode( *rhs.mBoneVec[id] );
			mBoneVec[id]->mName = iter->first.c_str();
		}
		mBaseBone = mBoneVec[0];

		if ( mTotalKeyFrame )
		{
			createMotionData( mTotalKeyFrame );
			std::copy( rhs.mBaseBone->keyFrames , mBaseBone->keyFrames , mBaseBone->keyFrames + ( mTotalKeyFrame * mBoneVec.size() ) );
		}
	}

	Skeleton::~Skeleton()
	{
		delete [] mBaseBone->keyFrames;
		for( int i = 0 ; i < mBoneVec.size() ; ++i )
			delete mBoneVec[i];
	}

	Skeleton* Skeleton::clone()
	{
		Skeleton* result = new Skeleton( *this );
		return result;
	}

	void Skeleton::createMotionData( int numFrame )
	{
		mTotalKeyFrame = numFrame;

		MotionKeyFrame* data = new MotionKeyFrame[ numFrame * mBoneVec.size() ];
		mBaseBone->keyFrames = data;
		for( int i = 0 ; i < mTotalKeyFrame ; ++i )
		{
			data[i].pos.setValue(0,0,0);
			data[i].rotation.setValue(0,0,0,1);
		}

		data += numFrame;
		for( int i = 1 ; i < mBoneVec.size() ; ++i )
		{
			mBoneVec[i]->keyFrames = data;
			data += numFrame;
		}
	}

	BoneNode* Skeleton::createBone( char const* name , char const* parentName )
	{
		if ( !parentName )
			parentName = CF_BASE_BONE_NAME;
		BoneNode* parent = getBone( parentName );
		return createBone( name , parent->id );
	}

	BoneNode* Skeleton::createBone( char const* name , BoneId parentId )
	{
		BoneNode* bone = new BoneNode;

		int id = mBoneVec.size();
		std::pair< BoneMap::iterator, bool > result = mNameMap.insert( std::make_pair( name , id ) );
		assert( result.second );

		bone->mName = result.first->first.c_str();
		bone->id = id;
		bone->parentId = parentId;
		assert( bone->id > parentId || parentId == 0 );

		mBoneVec.push_back( bone );
		return bone;
	}

	BoneNode* Skeleton::getBone( char const* name )
	{
		BoneMap::iterator iter = mNameMap.find( name );
		if ( iter == mNameMap.end() )
			return nullptr;
		return mBoneVec[ iter->second ];
	}


	void Skeleton::setController( IBoneController* manager )
	{
		mController->uninstall( *this );
		mController = manager;
		if ( !mController )
			mController = &gEmptyBoneControllerMgr;
	}

	void Skeleton::generateBoneTransform( Matrix4* matVec , Matrix4 const& baseTrans , int frame , float fract )
	{
		assert( 0 <= fract && fract < 1.0f );

		//M( blend ) = M(motion) * M( base )

		assert( mBaseBone->id == 0 );
		matVec[ 0 ] = baseTrans;

		int size = mBoneVec.size();
		for ( int i = 1 ; i < size ; ++i )
		{
			BoneNode* bone = mBoneVec[i];
			Matrix4& boneTrans = matVec[bone->id];
			bone->calcFrameTransform( boneTrans , frame , frame + 1 , fract );
			TransformUntility::mul( boneTrans , boneTrans , matVec[ bone->parentId ] );
		}
	}


	void Skeleton::generateBoneTransform( Matrix4* matVec , Matrix4 const& baseTrans , float* frames , float* weights , int num )
	{
		assert( mBaseBone->id == 0 );
		matVec[ 0 ] = baseTrans;

		int size = mBoneVec.size();
		for ( int i = 1 ; i < size ; ++i )
		{
			BoneNode* bone = mBoneVec[i];
			Matrix4& boneTrans = matVec[bone->id];
			bone->calcFrameTransform( boneTrans , frames , weights , num );
			TransformUntility::mul( boneTrans , boneTrans , matVec[ bone->parentId ] );
		}
	}

	void Skeleton::generateInvLoacalTransform()
	{
		assert( mUseInvLocalTrans );

		std::vector< Matrix4 > globalTransVec;
		globalTransVec.resize( mBoneVec.size() );
		globalTransVec[0] = Matrix4::Identity();
		mBaseBone->invLocalTrans = Matrix4::Identity();

		assert( mBaseBone->id == 0 );
		int size = mBoneVec.size();
		for ( int i = 1 ; i < size ; ++i )
		{
			BoneNode* bone = mBoneVec[i];
			Matrix4 const& parentTrans = globalTransVec[ bone->parentId ];

			Matrix4& worldTrans = globalTransVec[ bone->id ];
			bone->calcFrameTransform( worldTrans , 0 );
			TransformUntility::mul( worldTrans , worldTrans , parentTrans );

			float det;
			bool result = worldTrans.inverseAffine( bone->invLocalTrans , det );
		}
	}

	void Skeleton::generateBoneLine( std::vector< Vector3 >& lineVec , int frame )
	{
		lineVec.reserve( 2 * mBoneVec.size() );
		mBaseBone->generateBoneLine( lineVec , Matrix4::Identity() , frame );
	}

	void Skeleton::setupBoneWorldTransform( RenderSystem* renderSys , Matrix4 const& baseWorld , Matrix4* boneTrans )
	{
		if ( boneTrans )
		{
			if ( mUseInvLocalTrans )
			{
				for( int i = 0 ; i < getBoneNum(); ++i )
				{
					BoneNode* bone = getBone(i);
					Matrix4 worldTrans;
					//World = InvLocal * Bone * BaseWorld
					TransformUntility::mul( worldTrans , bone->invLocalTrans , boneTrans[i] );
					TransformUntility::mul( worldTrans , worldTrans , baseWorld );
					renderSys->setWorldMatrix( i , worldTrans );
				}
			}
			else
			{
				for( int i = 0 ; i < getBoneNum(); ++i )
				{
					BoneNode* bone = getBone(i);
					Matrix4 worldTrans;
					TransformUntility::mul( worldTrans , boneTrans[i] , baseWorld );
					renderSys->setWorldMatrix( i , worldTrans );
				}
			}
		}
		else
		{
			for( int i = 0 ; i < getBoneNum(); ++i )
			{
				BoneNode* bone = getBone(i);
				Matrix4 worldTrans;
				renderSys->setWorldMatrix( i , baseWorld );
			}
		}
	}

}//namespace CFly
