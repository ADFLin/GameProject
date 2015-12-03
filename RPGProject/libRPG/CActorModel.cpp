#include "ProjectPCH.h"
#include "CActorModel.h"

float const gActorAnimationFPS = 30;
float const gCrossBlendLength = 5;
float const gConnectedBlendLength = 5;


static CActorModel::AnimInfo sDefultAnimFlag[ANIM_TYPE_NUM] =
{
	//ANIM_WAIT
	{ 1 , AF_PLAY_LOOP | AF_CHANGE_IMMEDIATELY },
	//ANIM_WALK 
	{ 1 , AF_PLAY_LOOP | AF_CHANGE_IMMEDIATELY },
	//ANIM_RUN 
	{ 1 , AF_PLAY_LOOP | AF_CHANGE_IMMEDIATELY },
	//ANIM_DEFENSE
	{ 0 , AF_PLAY_LOOP | AF_CHANGE_IMMEDIATELY },
	//ANIM_HURT
	{ 0 , 0 },
	//ANIM_DYING
	{ 0 , AF_BLOCK_NEXT_ANIM },
	//ANIM_BUF_SKILL
	{ 0 , AF_PLAY_LOOP | AF_CHANGE_IMMEDIATELY },
	//ANIM_CAST_SKILL
	{ 0 , 0 },
	//ANIM_ATTACK
	{ 0 , 0 },

	{ 0 , 0 },{ 0 , 0 },{ 0 , 0 },{ 0 , 0 },{ 0 , 0 },
	{ 0 , 0 },{ 0 , 0 },{ 0 , 0 },{ 0 , 0 },

};

CActorModel::CActorModel( CFly::Actor* actor ) 
{
	mActor = actor;
	mCopyState = actor->createAnimationState( "COPY_STATE" , 1 , 2 );

	mCurAnim  = ANIM_WAIT;	
	mIdleAnim = ANIM_WAIT;

	mBlendType = NO_BLEND;
	mNextAnim = ANIM_NO_PLAY;

	for( int i = 0 ; i < ANIM_TYPE_NUM; ++i )
	{
		mAnimInfo[i] = sDefultAnimFlag[i];
		mAnimState[i] = NULL;
	}
}

void CActorModel::updateAnim( long time )
{
	float dFrame = ( gActorAnimationFPS / 1000 ) * time ;

	if ( mNextAnim != ANIM_NO_PLAY )
	{
		if ( mBlendType != CONNECTED_BLEND )
			mChangeAnimDelay -= dFrame;

		if ( mChangeAnimDelay < 0 )
		{
			setupActionInternal( mNextAnim , true );
			mNextAnim = ANIM_NO_PLAY;
		}
	}
	else if ( !mCurState->isLoop() )
	{
		if ( mCurState->getTimePos() > mCurState->getLength() )
		{
			if ( mIdleAnim != ANIM_NO_PLAY &&( mAnimInfo[ mCurAnim ].flag & AF_TRANSFER_IDLE_AUTO ) )
				setupActionInternal( mIdleAnim , true );
		}
	}

	updateBlendState( dFrame );
	mActor->updateAnimation(( mBlendType != CONNECTED_BLEND ) ? dFrame : 0 );
}

void CActorModel::setupActionInternal( AnimationType anim , bool needBlend )
{
	assert( anim != ANIM_NO_PLAY );

	if ( mBlendType != NO_BLEND )
	{
		mBlendState->enable( false );
		mBlendType = NO_BLEND;
	}

	if ( needBlend )
	{
		BlendType type = getBlendType( mCurAnim , anim );
		if ( type == CROSS_BLEND )
			setupBlendAnim( anim , CROSS_BLEND , gCrossBlendLength );
		else
			setupBlendAnim( anim , CONNECTED_BLEND , gConnectedBlendLength );
	}
	else
	{
		mCurState->enable( false );
		mCurState = mAnimState[ anim ];
		mCurState->enable( true );
		mCurState->setTimePos( 0 );
		mCurState->setWeight( 1.0f );
	}
	mCurAnim     = anim;
}

void CActorModel::changeAction( AnimationType anim , bool beForce )
{
	if ( !mAnimState[anim] )
		return;

	if ( anim == mCurAnim && mCurState->isLoop() )
		return;

	mNextAnim = anim;

	if ( canChangeImmediately( mCurAnim ) || beForce )
		mChangeAnimDelay = 0.0f;
	else 
		mChangeAnimDelay = mCurState->getLength() - mCurState->getTimePos();

}

float CActorModel::getAnimTimeLength( AnimationType anim )
{
	if (!mAnimState[ anim ] )
		return 0;
	return mAnimState[ anim ]->getLength() / gActorAnimationFPS;
}

bool CActorModel::canChangeImmediately( AnimationType amin )
{
	if ( amin == ANIM_NO_PLAY )
		return true;
	return ( mAnimInfo[ amin ].flag & AF_CHANGE_IMMEDIATELY );
}

char const* CActorModel::getPoseName( AnimationType amin )
{
	assert( mAnimState[amin] );
	return mAnimState[amin]->getName();
}

void CActorModel::setupBlendAnim( AnimationType type , BlendType blendType , float maxBlendFrame )
{
	assert( mAnimState[type] );
	assert( blendType != NO_BLEND );

	if ( blendType == CROSS_BLEND )
	{
		if ( !mCurState->isLoop() )
			mTotalBlendFrame = std::max( mCurState->getLength() - mCurState->getTimePos() , maxBlendFrame );
		else
			mTotalBlendFrame = maxBlendFrame;
	}
	else
	{
		mTotalBlendFrame = maxBlendFrame;
	}

	if ( mCurState == mAnimState[type] )
	{
		mBlendState = mCopyState;
		mBlendState->assignSetting( *mCurState );
		mBlendState->enable( true );
	}
	else
	{
		mBlendState = mCurState;
	}
	mBlendState->setWeight(1.0);

	mCurState = mAnimState[type];
	mCurState->setWeight(0);
	mCurState->setTimePos(0);
	mCurState->enable( true );

	mBlendType = blendType;
	mBlendFrame = 0;
}

void CActorModel::updateBlendState( float dFrame )
{
	if ( mBlendType == NO_BLEND )
		return;

	mBlendFrame += dFrame;

	if ( mBlendFrame >= mTotalBlendFrame )
	{
		mBlendState->enable( false );
		mCurState->setWeight( 1.0f );
		mBlendType = NO_BLEND;
		return;
	}

	float fract = mBlendFrame / mTotalBlendFrame;
	mCurState->setWeight( fract );
	mBlendState->setWeight( 1 - fract );
}

void CActorModel::setupAnimState( AnimationType type )
{
	assert( mAnimState[type] );

	CFly::AnimationState* state = mAnimState[type];
	unsigned flag = mAnimInfo[ type ].flag;

	if ( flag & AF_PLAY_LOOP )
		state->enableLoop( true );
}

void CActorModel::setAnimationState( char const* name , AnimationType type )
{
	mAnimState[ type ] = mActor->getAnimationState( name );
	if ( mAnimState[type] )
		setupAnimState( type );
}

void CActorModel::restoreAnim()
{
	mCurState = mAnimState[ mCurAnim ];
	if ( mCurState )
	{
		mCurState->enable( true );
		mCurState->setTimePos( 0 );
		mCurState->setWeight( 1.0f );
	}
}

CActorModel::BlendType CActorModel::getBlendType( AnimationType fAnim ,AnimationType bAnim )
{
	if ( !mAnimInfo[ fAnim ].blendMap || !mAnimInfo[ bAnim ].blendMap )
		return CONNECTED_BLEND;

	if ( mAnimInfo[ fAnim ].blendMap != mAnimInfo[ bAnim ].blendMap )
		return CONNECTED_BLEND;

	return CROSS_BLEND;
}


char const* actionName[] = 
{
	"ANIM_WAIT"       ,
	"ANIM_WALK"       ,
	"ANIM_RUN"        ,
	"ANIM_DEFENSE"    ,
	"ANIM_HURT"       ,
	"ANIM_DYING"      ,
	"ANIM_BUF_SKILL"  ,
	"ANIM_CAST_SKILL" ,
	"ANIM_ATTACK"     ,

	"ANIM_FUN_1" ,
	"ANIM_FUN_2" ,
	"ANIM_FUN_3" ,
	"ANIM_FUN_4" ,
	"ANIM_FUN_5" ,
	"ANIM_FUN_6" ,
	"ANIM_FUN_7" ,
	"ANIM_FUN_8" ,
	"ANIM_FUN_9" ,
};