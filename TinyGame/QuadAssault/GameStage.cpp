#include "GameStage.h"

#include "GameInterface.h"
#include "RenderUtility.h"
#include "RHI/RHIGraphics2D.h"

SrceenFade::SrceenFade()
{
	mState     = eNONE;
	mFadeSpeed = 1.0f;
	mColor     = 0;
}

void SrceenFade::render(RHIGraphics2D& g)
{
	if ( mColor != 1.0 )
	{
		using namespace Render;
		g.setBlendState(ESimpleBlendMode::Multiply);
		g.setBrush(Color3f(mColor, mColor, mColor));
		g.drawRect(Vec2f(0.0, 0.0), Vec2f(getGame()->getScreenSize()));
		g.setBlendState(ESimpleBlendMode::None);
	}
}

void SrceenFade::update( float dt )
{
	if ( mState == eNONE )
		return;
	switch( mState )
	{
	case eIN:
		mColor += mFadeSpeed * dt;
		if( mColor > 1.0f )
		{
			mState = eNONE;
			mColor=1.0f;
		}
		break;
	case eOUT:
		mColor -= mFadeSpeed * dt;
		if( mColor < 0.0f)
		{
			mState = eNONE;
			mColor = 0.0f;
		}
		break;
	}

	if ( mState == eNONE )
	{
		if ( mFunFisish )
			mFunFisish();
	}
}

GameStage::GameStage()
{
	mNeedStop = false;
}
