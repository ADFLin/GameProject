#ifndef CActorModel_h__
#define CActorModel_h__

#include "common.h"
#include "CRenderComponent.h"
#include "CFActor.h"

enum AnimationType
{
	ANIM_WAIT = 0   ,
	ANIM_WALK       ,
	ANIM_RUN        ,
	ANIM_DEFENSE    ,
	ANIM_HURT       ,
	ANIM_DYING      ,
	ANIM_BUF_SKILL  ,
	ANIM_CAST_SKILL ,
	ANIM_ATTACK     ,

	ANIM_FUN_1 ,
	ANIM_FUN_2 ,
	ANIM_FUN_3 ,
	ANIM_FUN_4 ,
	ANIM_FUN_5 ,
	ANIM_FUN_6 ,
	ANIM_FUN_7 ,
	ANIM_FUN_8 ,
	ANIM_FUN_9 ,

	ANIM_TYPE_NUM ,
	ANIM_NO_PLAY ,
};


enum AnimationFlag
{
	AF_CHANGE_IMMEDIATELY = BIT(0),
	AF_PLAY_LOOP          = BIT(1),
	AF_PLAY_WHEN_SETTING  = BIT(2),
	AF_TRANSFER_IDLE_AUTO = BIT(3),
	AF_BLOCK_NEXT_ANIM    = BIT(4),
};


class CActorModel : public IRenderEntity
{
public:

	struct AnimInfo
	{
		char     blendMap;
		unsigned flag;
	};


	enum BlendType
	{
		CROSS_BLEND ,
		CONNECTED_BLEND ,
		NO_BLEND ,
	};

	CActorModel( CFly::Actor* actor );

	//virtual 
	CFActor* getRenderNode(){ return mActor; }
	//virtual 
	void changeScene( CFScene* scene ){  mActor->changeScene( scene );  }
	//virtual
	void release(){ mActor->release();  }

	void     setAnimationState( char const* name , AnimationType type );
	void     restoreAnim();
	void     setupAnimState( AnimationType type );
	void     updateAnim( long time );

	void     setupBlendAnim( AnimationType type , BlendType blendType , float maxBlendFrame );
	void     changeAction( AnimationType anim , bool beForce = false );

	char const* getPoseName( AnimationType amin );

	int      getBlendActionType( AnimationType fAnim ,AnimationType bAnim );
	float    getAnimTimeLength( AnimationType anim );
	void     setAnimFlag( int anim , unsigned flag ){	mAnimInfo[ anim ].flag = flag;	}
	void     removeAnimFlagBit( AnimationType anim , unsigned bit ){  mAnimInfo[ anim ].flag &= ~bit;  }
	void     addAnimFlagBit( AnimationType anim , unsigned bit ){  mAnimInfo[ anim ].flag |= bit;  }

	BlendType getBlendType( AnimationType fAnim ,AnimationType bAnim );

	CFActor*  getCFActor(){ return mActor; }
protected:
	bool     canChangeImmediately( AnimationType amin );
	void     updateBlendState( float dFrame );
	void     setupActionInternal( AnimationType anim , bool needBlend );
	BlendType mBlendType;

	float    mChangeAnimDelay;
	float    mTotalBlendFrame;
	float    mBlendFrame;

	CFActor*   mActor;
	CFly::AnimationState* mAnimState[ ANIM_TYPE_NUM ];

	CFly::AnimationState* mCurState;
	CFly::AnimationState* mBlendState;
	CFly::AnimationState* mCopyState;

	AnimationType mCurAnim;
	AnimationType mIdleAnim;
	AnimationType mNextAnim;

	AnimInfo mAnimInfo[ ANIM_TYPE_NUM ]; 
};

#endif // CActorModel_h__


