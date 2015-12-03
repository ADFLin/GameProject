#include "ProjectPCH.h"
#include "CBloodBar.h"

#include "UtilityGlobal.h"
#include "UtilityFlyFun.h"
#include "CUISystem.h"

#include "Thinkable.h"


#define BLOOD_BAR_TEX_NAME "panel_titlebar_center"

#include "RenderSystem.h"

CBloodBar::CBloodBar( Thinkable* thinkObj , CFObject* obj , float maxValue , Vec3D const& pos , Vec2i const& size , float const* color )
{
	mHostObj = obj;

	mMaxValue = maxValue;
	mCurValue = maxValue;
	mNewVal   = maxValue;
	mDeltaValue = 0;

	mLength = size.x;

	CUISystem::getInstance().setTextureDir( "Data\\UI" );

	Material* mat = gEnv->renderSystem->createEmptyMaterial();
	mat->addTexture( 0 , 0 , BLOOD_BAR_TEX_NAME );
	//mat->setOpacity( 0.000001f );

	unsigned id = UF_CreateSquare3D( mHostObj , pos , size.x , size.y , Vec3D(color[0] , color[1] , color[2] ) , mat );

	CFly::MeshBase* geom = mHostObj->getElement( id )->getMesh();

	posBuffer = geom->getVertexElement( CFly::CFV_XYZ , posOffset );
	texBuffer = geom->getVertexElement( CFly::CFV_TEXCOORD , texOffset );

	assert( posBuffer && texBuffer );

	if ( thinkObj )
		mContent = thinkObj->setupThink( this , &CBloodBar::changeLifeThink );
	else
		mContent = nullptr;
}

CBloodBar::~CBloodBar()
{

}



void CBloodBar::setMaxVal( float val )
{
	if ( mMaxValue != val )
	{
		mMaxValue = val; 
		doSetLife( getLife() );
	}
	else mMaxValue = val;
}

void CBloodBar::changeLifeThink( ThinkContent& content )
{
	doSetLife( getLife()+ mDeltaValue );

	if ( ( mDeltaValue > 0 && getLife() < mNewVal ) ||
		( mDeltaValue < 0 && getLife() > mNewVal ) )
	{
		content.nextTime = g_GlobalVal.nextTime;
	}
}

void CBloodBar::setLife( float val , bool isFade )
{
	if ( val == getLife() )
		return;

	if ( mContent && isFade && abs( val - getLife() ) > 1 )
	{
		mNewVal = val;
		mDeltaValue = ( mNewVal - getLife() ) / 3 ;
		mContent->nextTime = g_GlobalVal.curtime;
	}
	else
	{
		doSetLife( val );
	}
}

void CBloodBar::doSetLife( float val )
{
	char* data;

	assert( posBuffer );
	data = reinterpret_cast< char* >( posBuffer->lock(4) );

	if ( !data )
		return;

	char* posData = data + posOffset;

	float* v0 = reinterpret_cast< float* >( posData );
	float* v1 = reinterpret_cast< float* >( posData + posBuffer->getVertexSize() );
	float* v2 = reinterpret_cast< float* >( posData + 2 * posBuffer->getVertexSize() );

	if ( texBuffer != posBuffer )
	{
		data = reinterpret_cast< char* >( texBuffer->lock(4) );
		if ( !data )
		{
			posBuffer->unlock();
			return;
		}
	}

	char* texData = data + texOffset;

	float* tex1 = reinterpret_cast< float*>( texData + texBuffer->getVertexSize() );
	float* tex2 = reinterpret_cast< float*>( texData + 2 * texBuffer->getVertexSize() );

	mCurValue = TClamp( val , 0.0f , mMaxValue );
	float rate = mCurValue / mMaxValue;

	//x
	v1[0] = v2[0] = v0[0] + mLength * rate;
	tex1[0] = tex2[0] = rate;

	posBuffer->unlock();

	if ( texBuffer != posBuffer )
		texBuffer->unlock();
}

