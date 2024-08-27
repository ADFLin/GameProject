#ifndef CFActor_h__
#define CFActor_h__

#include "CFBase.h"
#include "CFAnimatable.h"
#include "CFObject.h"
#include "CFSkeleton.h"

#include "DataStructure/IntrList.h"

namespace CFly
{

	class BlendAnimation;

	class AnimationState
	{
	public:

		void  enable( bool beE );
		void  enableLoop( bool beE ){ mBeLoop = beE; }
		void  setWeight( float w ){  mWeight = w; }
		void  setTimePos( float pos ){ mTimePos = pos; }
		void  setTimeScale( float scale ) { mTimeScale = scale; }
		
		float getTimePos()    const { return mTimePos; }
		float getWeight()     const { return mWeight; }
		float getTimeScale()  const { return mTimeScale; }
		int   getStartFrame() const { return mStart; }
		int   getEndFrame()   const { return mEnd; }
		int   getLength()     const { return mLength; }
		bool  isEnable()      const { return mEnable; }
		bool  isLoop()        const { return mBeLoop; }

		char const* getName(){ return mName.c_str(); }

		void  assignSetting( AnimationState& other );
		AnimationState*  clone( BlendAnimation* animatable ) const;

	private:
		float updateTimePos( float offset );
		AnimationState( BlendAnimation* animatable , char const* name , int start , int end );

		bool   mBeLoop;
		bool   mEnable;
		int    mStart;
		int    mEnd;
		int    mLength;
		float  mTimeScale;
		float  mTimePos;
		float  mWeight;
		String mName;

		LinkHook mActiveHook;
		BlendAnimation* mManager;
		friend class BlendAnimation;
	};


	class BlendAnimation : public IAnimatable
	{
	public:
		static int const MaxBlendAnimateNum = 8;
		BlendAnimation();

		BlendAnimation( BlendAnimation const& ba );
		~BlendAnimation();
		AnimationState* getAnimationState( char const* name );
		AnimationState* createAnimationState( char const* name , int start ,int end );
		void updateAnimation( float offset );

	public:
		void _updateStateActive( AnimationState* state );

	protected:
		virtual void doUpdateAnimation( float* frames , float* weights , int num ){}
		
		struct StrCmp
		{
			bool operator()( char const* s1 , char const* s2 ) const
			{
				return strcmp( s1 , s2 ) < 0;
			}
		};

		typedef std::map< char const* , AnimationState* , StrCmp > AnimationStateMap;
		typedef TIntrList< 
			AnimationState , 
			MemberHook< AnimationState , &AnimationState::mActiveHook > ,
			PointerType
		> AnimationStateList;

		AnimationStateMap  mAnimationMap;
		AnimationStateList mActiveAnimList;
	};


	enum SkinBlendMethod
	{
		SBM_SOFTWARE_VERTEX_PROC ,
		SBM_SHADR ,
	};

	class Skin
	{



		Skeleton* mSkeleton;
	};


	class Actor : public SceneNode
		        , public BlendAnimation
	{
		typedef SceneNode BaseClass;
	public:
		enum
		{
			NODE_DISABLE_SKELETON = BaseClass::NEXT_NODE_FLAG ,
			NODE_SKIN_MARK ,
			NODE_ATTACHMENT_MARK ,
			NEXT_NODE_FLAG ,
		};


		Actor( Scene* scene );
		Actor( Actor const& actor );
		~Actor();

		Actor*    instance( bool beCopy );
		void      performSkinDeformation(){  updateAnimation( 0 );  }
		Skeleton* getSkeleton(){ return mSkeleton; }
		void      updateBoneMatrix( float frame );
		void      updateSkeleton( bool genInvLocalTrans );

		Object*   getAttachment( int idx );
		int       applyAttachment( Object* obj , BoneNode* bone , TransOp op = CFTO_GLOBAL );
		int       applyAttachment( Object* obj , char const* boneName , TransOp op = CFTO_GLOBAL );
		int       applyAttachment( Object* obj , int boneID , TransOp op = CFTO_GLOBAL );
		int       getAttachmentParentBoneID( int idx );
		int       getBoneID( char const* name );
		void      takeOffAttachment( Object* obj );
		int       getEmptyAttchSlot();
		int       getMaxAttachmentIndex(){ return (int)mAttachVec.size(); }

		Object*   createSkin();
		int       getSkinNum(){ return (int)mSkinVec.size(); }
		Object*   getSkin( int idx ){ return mSkinVec[ idx ]; }


		void      updateBoneLine( float frame );
		void      setOpacity( float val );

		void      setBoneTransSize( int size );
		Matrix4*  getBoneTransMatrix(){ return mBoneTransVec.empty() ? NULL : &mBoneTransVec[0]; }
		
		void      release();

	public:
		void  _updateAllAttachTransform();
		void  updateSubChildTransform( SpatialNode* obj );
	protected:
		//BlendAnimation
		void  doUpdateAnimation( float* frames , float* weights , int num );
		
		//SceneNode
		void  onRemoveChild( SceneNode* node );
		void  onChangeScene( Scene* newScene );
		void  onModifyTransform( bool beLocal );
		void  renderImpl( Matrix4 const& trans );
		
		void  _cloneShareData();
		virtual void releaseOwnedChildren();

		typedef std::vector< Object* > SkinVec;

		static uint32 const SkinFlagBits = BIT(NODE_BLOCK_RENDER) | BIT( NODE_SKIN_MARK );
		static uint32 const AttachFlagBits =  BIT(NODE_BLOCK_RENDER) | BIT(NODE_SUB_CHILD) | BIT( NODE_ATTACHMENT_MARK );


		SkinVec          mSkinVec;
		Skeleton::RefCountPtr mSkeleton;

		struct AttachInfo
		{
			Object* obj;
		};
		AttachInfo* findAttachment( Object* obj );
		void        removeAttachment( AttachInfo& info );
		void        changeOwnedChildrenRenderGroup( int group );
		typedef std::vector< Matrix4 >   MatrixVec;
		typedef std::vector< AttachInfo >  AttachInfoVec;

		struct MeshGroup
		{
			Skeleton::RefCountPtr mSkeleton;
			SkinVec          mSkinVec;
		};
		AttachInfoVec mAttachVec;
		//bool          mBoneTransDirty;
		MatrixVec     mBoneTransVec;
		Object*       mBoneLine;
	};

	DEFINE_ENTITY_TYPE( Actor , ET_ACTOR )


}//namespace CFly

#endif // CFActor_h__
