#include "ZumaPCH.h"
#include "ZDraw.h"
#include "IRenderSystem.h"

#include "ResID.h"
#include "ZBallConGroup.h"
#include "ZFont.h"
#include "ZPath.h"
#include "ZLevel.h"
#include "ZBall.h"
#include "ZStage.h"

#include "ZLevelManager.h"
#include "ZLevelStage.h"
#include "ZTask.h"

#include <functional>

namespace
{
	using namespace Zuma;
	struct BallData
	{
		int         frame;
		ResID       idBase;
		ResID       idTool[ TOOL_NUM_TOOLS ];
		ResID       idLight;
	};

	BallData gBallData[ zColorNum ];
	ResID    gIdToolBomb[ zColorNum ];
	ResID    gIdToolLight[ TOOL_NUM_TOOLS ];
}


namespace Zuma
{
	bool ZRenderUtility::init()
	{
#define BALL_DATA( id ,  _frame , _color , _colorName  )\
	gBallData[id].frame = _frame;\
	gBallData[id].idBase  = IMAGE_##_colorName##_BALL;\
	gBallData[id].idLight = IMAGE_##_colorName##_LIGHT;\
	gBallData[id].idTool[ TOOL_BOMB      ] = IMAGE_##_colorName##_BOMB; \
	gBallData[id].idTool[ TOOL_BACKWARDS ] = IMAGE_##_colorName##_BACKWARDS;\
	gBallData[id].idTool[ TOOL_ACCURACY  ] = IMAGE_##_colorName##_ACCURACY;\
	gBallData[id].idTool[ TOOL_SLOW      ] = IMAGE_##_colorName##_SLOW;\

		BALL_DATA( zBlue   , 47 , RGB( 0 , 0 , 255 ) , BLUE  )
		BALL_DATA( zYellow , 50 , RGB( 255 , 255 , 0 ), YELLOW )
		BALL_DATA( zRed    , 50 , RGB( 255 , 0 , 0 ) , RED )
		BALL_DATA( zGreen  , 50 , RGB( 0 , 255 , 0 ) , GREEN )
		BALL_DATA( zPurple , 51 , RGB( 255 , 0 , 255 ), PURPLE )
		BALL_DATA( zWhite  , 50 , RGB( 255 , 255 , 255 ), WHITE )

#undef BALL_DATA

		gIdToolLight[ TOOL_BACKWARDS ] = IMAGE_BACKWARDS_LIGHT ;
		gIdToolLight[ TOOL_ACCURACY  ] = IMAGE_ACCURACY_LIGHT ;
		gIdToolLight[ TOOL_SLOW      ] = IMAGE_SLOW_LIGHT;

		return true;
	}

	void ZRenderUtility::drawAimArrow( IRenderSystem& RDSystem , ZFrog const& frog , Vector2 const& aimPos , ZColor aimColor )
	{
		Vector2 normal( frog.getDir().y , -frog.getDir().x );

		normal *= 10;
		Vector2 pos = frog.getPos() + 50 * frog.getDir();

		RDSystem.setBlendFun( BLEND_SRCALPHA , BLEND_INV_SRCALPHA );
		RDSystem.enableBlend( true );
		RDSystem.setWorldIdentity();

		switch ( aimColor )
		{
		case zRed   : RDSystem.setColor( 255 , 0 , 0 , 100 ); break;
		case zBlue  : RDSystem.setColor( 50 , 50 , 255 , 100 ); break;
		case zGreen : RDSystem.setColor( 50 , 255 , 50 , 100 ); break;
		case zYellow: RDSystem.setColor( 255 , 255 , 50 , 100 ); break;
		case zPurple: RDSystem.setColor( 180 , 50 , 255 , 100 ); break;
		case zWhite : 
		default:
			RDSystem.setColor( 255 , 255 , 255 , 100 ); break;
		}

		Vector2 v[3];
		v[0] = pos + normal;
		v[1] = pos - normal;
		v[2] = aimPos;
		RDSystem.drawPolygon( v , 3 );
		RDSystem.setColor( 255 , 255 , 255 , 255 );
		RDSystem.enableBlend( false );
	}

	void ZRenderUtility::drawQuakeMap( IRenderSystem& RDSystem , ZQuakeMap& map , ITexture2D* texLvBG , Vector2 const pos[] )
	{
		for( int i = 0 ; i < map.numBGAlapha ; ++i )
		{
			ZBGAlpha& bgAlpha = map.bgAlpha[i];

			RDSystem.setWorldIdentity();
			RDSystem.translateWorld( pos[i] );
			RDSystem.drawBitmapWithinMask( *texLvBG , *bgAlpha.image , bgAlpha.pos );
		}
	}

	void ZRenderUtility::drawLevelMask( IRenderSystem& RDSystem , ZLevelInfo const& info )
	{
		for( int i = 0 ; i < info.numCutout ;++i )
		{
			ITexture2D const* tex = info.cutout[i].image.get();
			Vector2 const& pos = info.cutout[i].pos;

			RDSystem.setWorldIdentity();
			RDSystem.translateWorld( pos );
			RDSystem.drawBitmapWithinMask( *info.BGImage , *tex , pos );
		}
	}

	void ZRenderUtility::drawLevelBG( IRenderSystem& RDSystem , ZLevelInfo& info )
	{
		RDSystem.setWorldIdentity();
		RDSystem.drawBitmap( *info.BGImage );
	}

	void ZRenderUtility::drawPath( IRenderSystem& RDSystem , ZPath& path , float head , float step )
	{
		ITexture2D* texSparkle = RDSystem.getTexture( IMAGE_SPARKLE );

		int w = texSparkle->getWidth() / texSparkle->col;

		RDSystem.setColor( 255 , 255 , 125 , 255 );

		RDSystem.setBlendFun( BLEND_ONE , BLEND_ONE );

		for( int i = 0 ; i < texSparkle->col ; ++i )
		{
			if ( head > path.getPathLength() )
				break;

			Vector2 const& pos = path.getLocation( head );

			RDSystem.setWorldIdentity();
			RDSystem.translateWorld( pos );

			RDSystem.drawBitmap( *texSparkle , 
				Vector2( w * i  , 0 ) , Vector2( w , texSparkle->getHeight() ) ,
				TBF_OFFSET_CENTER | TBF_USE_BLEND_PARAM | TBF_ENABLE_ALPHA_BLEND );

			head -= step;
		}
		RDSystem.setColor( 255 , 255 , 255 , 255 );
	}

	void ZRenderUtility::drawHole( IRenderSystem& RDSystem , int frame )
	{
		ITexture2D* texHole = RDSystem.getTexture( IMAGE_HOLE );
		ITexture2D* texHoleCover = RDSystem.getTexture( IMAGE_HOLE_COVER );

		RDSystem.pushWorldTransform();
		RDSystem.drawBitmap( *texHole , TBF_OFFSET_CENTER );
		RDSystem.popWorldTransform();

		RDSystem.drawBitmap( *texHoleCover , 
			Vector2( 0 ,  frame * texHoleCover->getWidth() ), 
			Vector2( texHoleCover->getWidth() , texHoleCover->getWidth() ) , 
			TBF_OFFSET_CENTER );
	}

	void ZRenderUtility::drawBall( IRenderSystem& RDSystem , ZConBall& ball )
	{
		int frame;

		if ( ball.getToolProp() == TOOL_NORMAL )
		{
			int totalFrame = gBallData[ ball.getColor() ].frame;
			frame = int( 0.5 * ( ball.getPathPos() + ball.getPathPosOffset() ) ) % totalFrame;
			if ( frame < 0 )
				frame += totalFrame;
		}
		else
		{
			frame = ball.getLifeTime();

			float cycle = float( ball.getLifeTime() ) / 2000;

			int const maxC = 170;
			frame = 2 * maxC * ( cycle - (int)cycle );
			if ( frame >= maxC ) 
				frame = 2 * maxC - frame ;
		}


		Vector2 const& pos = ball.getPos();
		Vector2 gBallShadowOffset( -3 , 4 );

		RDSystem.setWorldIdentity();
		RDSystem.translateWorld( pos.x - 3 , pos.y + 4 );

		ITexture2D* tex = RDSystem.getTexture( IMAGE_BALL_SHADOW );

		RDSystem.setColor( 25 , 25 , 25 , 150 );
		RDSystem.drawBitmap( *tex , TBF_OFFSET_CENTER | TBF_ENABLE_ALPHA_BLEND /*| TBF_USE_BLEND_PARAM*/ );
		RDSystem.setColor( 255 , 255 , 255 , 255 );
		drawBall( RDSystem , ball , frame , ball.getToolProp() );
	}

	void ZRenderUtility::drawBall( IRenderSystem& RDSystem , ZColor color , int frame )
	{
		BallData& data = gBallData[color];

		ITexture2D* tex = RDSystem.getTexture( data.idBase );

		int h = tex->getHeight() / tex->row;

		RDSystem.drawBitmap( *tex , Vector2(0 , frame * h ) , 
			Vector2( tex->getWidth() , h ) , TBF_OFFSET_CENTER );
	}

	void ZRenderUtility::drawBall( IRenderSystem& RDSystem , ZObject& ball , int frame , ToolProp prop /*= TOOL_NORMAL */ )
	{
		Vector2 const& pos = ball.getPos();
		Vector2 const& dir = ball.getDir();

		RDSystem.loadWorldMatrix( pos , dir );
		RDSystem.rotateWorld( -90 );
		//RDSystem.setMatrix( pos , 1 , 0 );

		BallData& data = gBallData[ball.getColor()];

		if ( prop == TOOL_NORMAL )
		{
			drawBall( RDSystem , ball.getColor() , frame );
		}
		else
		{
			ITexture2D* tex = RDSystem.getTexture( data.idTool[ prop ] ) ;

			RDSystem.rotateWorld( -90 );

			RDSystem.pushWorldTransform();
			RDSystem.drawBitmap( *tex , TBF_OFFSET_CENTER );
			RDSystem.popWorldTransform();

			ITexture2D* texlight;
			if ( prop == TOOL_BOMB )
				texlight = RDSystem.getTexture( data.idLight );
			else
				texlight = RDSystem.getTexture( gIdToolLight[ prop ] );

			RDSystem.translateWorld( -1 , 1 );

			RDSystem.setBlendFun( BLEND_ONE , BLEND_ONE );
			RDSystem.setColor( frame , frame , frame , 255 );
			RDSystem.drawBitmap( *texlight , 
				TBF_ENABLE_ALPHA_BLEND | TBF_USE_BLEND_PARAM | TBF_OFFSET_CENTER );
			RDSystem.setColor( 255 , 255 , 255 , 255 );
		}
	}

	void ZConBallGroup::render( IRenderSystem& RDSystem , bool beMask )
	{
		float pathLen = getFollowPath()->getPathLength();

		ZBallNode iter = mBallList.begin();
		ZBallNode end = mBallList.end();
		for( ; iter != end ; ++iter )
		{
			ZConBall* ball = *iter;

			if ( ball->getConState() == CS_INSERTING )
			{
				ZConBall* prev = getPrevConBall( *ball );
				if (  prev && ( prev->getPos() - ball->getPos() ).length2() > 3000 )
				{
					ball->setDir( Vector2(1,0));
				}
			}

			if ( ball->getPathPos() < pathLen && ball->getPathPos() >= 0 && beMask == ball->isMask() )
			{
				ZRenderUtility::drawBall( RDSystem , *ball );
			}
		}
	}

	void ZFrog::render( IRenderSystem& RDSystem )
	{

		ITexture2D* tex;
		ITexture2D* texFrog = RDSystem.getTexture( IMAGE_SHOOTER_BOTTOM );

		RDSystem.loadWorldMatrix( getPos() , getDir() );
		RDSystem.rotateWorld( -90 );

		RDSystem.pushWorldTransform();
		{
			RDSystem.drawBitmap( *texFrog , TBF_OFFSET_CENTER );
		}
		RDSystem.popWorldTransform();

		if ( getNextColor() != zNull )
		{
			tex = RDSystem.getTexture( IMAGE_NEXT_BALL );
			int w = tex->getWidth() / tex->col;

			RDSystem.pushWorldTransform();
			{
				RDSystem.translateWorld( 0 , - 25 - mBallOffset );
				RDSystem.drawBitmap( *tex , 
					Vector2(  w * getNextColor() , 0) , 
					Vector2(  w , tex->getHeight() ) , TBF_OFFSET_CENTER );
			}
			RDSystem.popWorldTransform();
		}

		tex = RDSystem.getTexture( IMAGE_SHOOTER_TONGUE );

		RDSystem.pushWorldTransform();
		{
			RDSystem.translateWorld( 0 , 28 + mBallOffset );
			RDSystem.drawBitmap( *tex , TBF_OFFSET_CENTER );
		}
		RDSystem.popWorldTransform();


		if ( getColor() != zNull )
		{
			RDSystem.pushWorldTransform();
			{
				RDSystem.translateWorld( 0  , 25 + mBallOffset );
				ZRenderUtility::drawBall( RDSystem , getColor() , 0 );
			}
			RDSystem.popWorldTransform();
		}


		tex = RDSystem.getTexture( IMAGE_SHOOTER_TOP );

		RDSystem.pushWorldTransform();
		{
			RDSystem.translateWorld( 0 , -22 + 12 );
			RDSystem.drawBitmapWithinMask( *texFrog , *tex ,
				Vector2( (texFrog->getWidth() - tex->getWidth() ) /2.0 , 12 ) , 
				TBF_OFFSET_CENTER );
		}
		RDSystem.popWorldTransform();

		if ( mFrameEye != 0 )
		{
			tex = RDSystem.getTexture( IMAGE_EYE_BLINK );
			int h = tex->getHeight() / 2;
			RDSystem.pushWorldTransform();
			{
				RDSystem.translateWorld( 0 , 0 );
				RDSystem.drawBitmap(  *tex , 
					Vector2( 0 , ( mFrameEye - 1 ) * h) , 
					Vector2( tex->getWidth() , h ) , TBF_OFFSET_CENTER  );
			}
			RDSystem.popWorldTransform();
		}
	}


	void MenuStage::drawSky( IRenderSystem& RDSystem , ResID skyID )
	{
		ITexture2D* tex = RDSystem.getTexture( skyID );	

		if ( skyPos > tex->getWidth() )
			skyPos -= tex->getWidth();

		RDSystem.setWorldIdentity();
		RDSystem.translateWorld( skyPos - tex->getWidth() , 0  );
		RDSystem.drawBitmap( *tex );
		RDSystem.translateWorld( tex->getWidth() , 0  );
		RDSystem.drawBitmap( *tex );
		RDSystem.translateWorld( tex->getWidth() , 0 );
		RDSystem.drawBitmap( *tex );
	}


	void LoadingStage::onRender( IRenderSystem& RDSystem )
	{
		ITexture2D* tex = RDSystem.getTexture( IMAGE_LOADING_SCREEN );

		RDSystem.setWorldIdentity();
		RDSystem.drawBitmap( *tex );

		float persent = float( numTotal - numCur ) / numTotal;

		tex = RDSystem.getTexture( IMAGE_LOADERBAR );

		RDSystem.translateWorld( 128 , 350 );
		RDSystem.drawBitmap( *tex , Vector2( 0 , 0 ) , 
			Vector2( persent * tex->getWidth()  , tex->getHeight() ) );
	}

	void ZLevel::render( IRenderSystem& RDSystem , RenderParam const& param )
	{
		float const PathPosWarning = 300;

		ZRenderUtility::drawLevelBG( RDSystem , getInfo() );

		for ( int i = 0 ; i < getBallGroupNum() ; ++i )
			getBallGroup(i)->render( RDSystem , true );

		ZRenderUtility::drawLevelMask( RDSystem , getInfo() );

		for ( int i = 0 ; i < getBallGroupNum() ; ++i )
		{
			ZPath* path = getBallGroup(i)->getFollowPath();

			int frame = 0;
			if ( ZConBall* ball = getBallGroup(i)->getLastBall() )
			{
				float len = path->getPathLength() - ball->getPathPos();
				if ( len < 0 )
					frame = 11;
				else if ( len < PathPosWarning )
					frame = 11 - int( len * 12 / PathPosWarning );
			}

			RDSystem.loadWorldMatrix( path->getEndLocation() , param.holeDir[i] );
			RDSystem.rotateWorld( 90 );

			ZRenderUtility::drawHole( RDSystem , frame );
			getBallGroup(i)->render( RDSystem , false );
		}

		if ( mTimeAccuracy > g_GameTime.curTime && getFrog().getColor() != zNull )
		{
			ZRenderUtility::drawAimArrow( RDSystem , 
				getFrog(), param.aimPos ,
				( param.aimBall )? param.aimBall->getColor() : zWhite  );
		}

		RDSystem.setCurOrder( LRO_QUAKE_MAP );

		if ( getFrog().isShow() )
			getFrog().render( RDSystem );


		for( MoveBallList::iterator iter = mShootBallList.begin();
			iter != mShootBallList.end(); ++iter )
		{
			ZMoveBall* ball = *iter;
			ZRenderUtility::drawBall(  RDSystem , *ball , 0 , TOOL_NORMAL );
		}


	}

	void LevelStage::onRender( IRenderSystem& RDSystem )
	{

		mLevel.render( RDSystem , mRenderParam );

#if 0
		RDSystem.setCurOrder( LRO_TEXT );

		if ( mPauseGame )
		{
			static Vec2D const screenPos[] = 
			{
				Vec2D( 0 , 0 ), Vec2D( g_ScreenWidth ,0 ) ,
				Vec2D( g_ScreenWidth , g_ScreenHeight ) ,Vec2D( 0 , g_ScreenHeight ) ,
			};

			RDSystem.setWorldIdentity();
			RDSystem.setColor( 0 , 0 , 0 , 100 );
			RDSystem.setBlendFun( BLEND_SRCALPHA , BLEND_INV_SRCALPHA );
			RDSystem.enableBlend( true );
			RDSystem.drawPolygon( screenPos , 4 );
			RDSystem.enableBlend( false );
			RDSystem.setColor( 255 , 255 , 255 , 255 );

			ZFont* font = RDSystem.getFontRes( FONT_HUGE );
			font->print( Vec2D( g_ScreenWidth / 2 , g_ScreenHeight / 2 ) , 
				"Pause" , FF_ALIGN_VCENTER | FF_ALIGN_HCENTER );
		}

		ITexture2D* tex;
		RDSystem.setWorldIdentity();
		tex = RDSystem.getTexture( IMAGE_MENU_BAR );
		RDSystem.drawBitmap( *tex );
		//
		RDSystem.setWorldIdentity();
		RDSystem.translateWorld( 409 , 2 );
		if ( mLevel.getCurComboGauge() >= mLevel.getMaxComboGauge() )
		{
			tex = RDSystem.getTexture( IMAGE_ZUMA_BAR_DONE );
			RDSystem.drawBitmap( *tex );
		}
		else
		{
			tex = RDSystem.getTexture( IMAGE_ZUMA_BAR  );
			float factor = float( mLevel.getCurComboGauge() ) / mLevel.getMaxComboGauge();
			RDSystem.drawBitmap( *tex , Vec2D(0 , 0 ), 
				Vec2D( tex->getWidth() * factor , tex->getHeight() ) );
		}

		tex = RDSystem.getTexture( IMAGE_FROG_LIVES );
		RDSystem.setWorldIdentity();
		RDSystem.translateWorld( 26 , 2 );
		for ( int i = 0 ; i < numLive ; ++i )
		{
			RDSystem.drawBitmap( *tex );
			RDSystem.translateWorld( tex->getWidth() + 3 , 0 );
		}


		ZFont* font = RDSystem.getFontRes( FONT_MAIN10 );
		char str[128];
		sprintf( str , "%d" , showSorce );
		RDSystem.setColor( 255 , 255 , 0 , 255 );
		font->print( Vec2D( 365 , 12 ), str , FF_ALIGN_RIGHT | FF_ALIGN_VCENTER );
		RDSystem.setColor( 255 , 255 , 255 , 255 );
#endif

	}

	void ZMainMenuStage::onRender( IRenderSystem& RDSystem )
	{
		ITexture2D* tex;

		drawSky( RDSystem , IMAGE_MM_SKY );

		tex = RDSystem.getTexture( IMAGE_MM_BACK );

		RDSystem.setWorldIdentity();
		RDSystem.drawBitmap( *tex );

		tex = RDSystem.getTexture( IMAGE_SUNGLOW );

		RDSystem.translateWorld( 40 , 40  );
		RDSystem.rotateWorld( angleSunGlow );

		float s = 0.9 + 0.2 * sin( 0.001* angleSunGlow );

		RDSystem.scaleWorld( s , s );

		RDSystem.setColor( 50 , 50 , 10 ,  60 * s  );
		RDSystem.setBlendFun( BLEND_ONE , BLEND_ONE );
		RDSystem.drawBitmap( *tex , TBF_OFFSET_CENTER | TBF_USE_BLEND_PARAM | TBF_ENABLE_ALPHA_BLEND );
		RDSystem.setColor( 255 , 255 , 255 , 255 );


		tex = RDSystem.getTexture( IMAGE_MM_SUN );
		RDSystem.setWorldIdentity();
		RDSystem.translateWorld( 40 , 40 );
		RDSystem.drawBitmap( *tex , TBF_OFFSET_CENTER );

		ZFont* font = RDSystem.getFontRes( FONT_PLAIN );
		InlineString< 32 > str;
		str.format(" Loading Time = %d " );
		font->print( Vec2i(20,20) , str );
	}


	void AdvMenuStage::onRender( IRenderSystem& RDSystem )
	{
		ITexture2D* tex;

		drawSky( RDSystem , IMAGE_ADVSKY );

		tex = RDSystem.getTexture( IMAGE_ADVBACK );

		RDSystem.setWorldIdentity();
		RDSystem.drawBitmap( *tex );
		RDSystem.translateWorld( 0  , 72  );
		tex = RDSystem.getTexture( IMAGE_ADVMIDDLE );
		RDSystem.drawBitmap( *tex );

		if ( finishTemp >= 1 )
			tex = RDSystem.getTexture( IMAGE_ADVTEMPLE2 );
		else
			tex = RDSystem.getTexture( IMAGE_ADVTEMPLE2V );
		RDSystem.setWorldIdentity();
		RDSystem.translateWorld( 0  , 106  );
		RDSystem.drawBitmap( *tex );

		if ( finishTemp >= 2 )
			tex = RDSystem.getTexture( IMAGE_ADVTEMPLE3 );
		else
			tex = RDSystem.getTexture( IMAGE_ADVTEMPLE3V );

		RDSystem.setWorldIdentity();
		RDSystem.translateWorld( g_ScreenWidth - tex->getWidth() , 106  );
		RDSystem.drawBitmap( *tex );


		RDSystem.setWorldIdentity();
		tex = RDSystem.getTexture( IMAGE_ADVTEMPLE1 );
		RDSystem.translateWorld( 0  , g_ScreenHeight - tex->getHeight() );
		RDSystem.drawBitmap( *tex );
	}

#include "ZUISystem.h"

	void ZUISystem::drawTex( ResID idTex , Vector2 const& pos , int frame )
	{
		IRenderSystem& GLSystem = *mRDSystem;

		ITexture2D* tex = getTexture( idTex );

		GLSystem.setWorldIdentity();
		GLSystem.translateWorld( pos );

		if ( frame == -1 )
		{
			GLSystem.drawBitmap( *tex );	
		}
		else
		{
			int w = tex->getWidth() / tex->col;
			GLSystem.drawBitmap( *tex , Vector2( frame * w , 0 ) , Vector2( w , tex->getHeight() ) );
		}

	}


	Vec2i ZDialogButton::calcUISize( int len )
	{
		ITexture2D* tex = getUISystem()->getTexture( IMAGE_DIALOG_BUTTON );
		return Vec2i( len , tex->getHeight() - downGap );
	}

	void ZDialogButton::onRender( )
	{
		IRenderSystem& RDSystem = getUISystem()->getRenderSystem();

		Vec2i size = getSize();
		Vec2i pos  = getWorldPos();
		int bodySize = size.x - 2 * headSize;

		ITexture2D* tex = getTexture();
		int h = tex->getHeight() - downGap;
		int w = tex->getWidth() / 3 - ( frontGap + backGap );
		int wBody = w - 2 * headSize;

		RDSystem.setWorldIdentity();
		RDSystem.translateWorld( pos );
		RDSystem.translateWorld( -frontGap , 0  );

		int x = 0;

		switch( getButtonState() )
		{
		case BS_NORMAL:    x += 0; break;
		case BS_HIGHLIGHT: x += 1 * tex->getWidth() / 3; break;
		case BS_PRESS:     x += 2 * tex->getWidth() / 3; break;
		}

		int tw = headSize + frontGap;
		RDSystem.drawBitmap( *tex , Vector2( x , 0 ), Vector2(tw  , tex->getHeight()) );

		RDSystem.translateWorld( tw , 0 );
		x+= tw ;

		for(;bodySize > wBody ; bodySize-=wBody )
		{
			RDSystem.drawBitmap( *tex , Vector2(x , 0) , Vector2(wBody , tex->getHeight()) );
			RDSystem.translateWorld(  wBody ,0 );
		}

		RDSystem.drawBitmap( *tex , Vector2(x , 0) , Vector2(bodySize , tex->getHeight()) );

		RDSystem.translateWorld(  bodySize  ,0 );
		x += wBody;

		RDSystem.drawBitmap( *tex , Vector2(x, 0) , Vector2(headSize + backGap , tex->getHeight()) );

		ZFont* font = RDSystem.getFontRes( FONT_MAIN10 );

		RDSystem.setWorldIdentity();

		pos += ( size / 2 );
		if ( getButtonState() == BS_NORMAL )
			RDSystem.setColor( 255 , 255 , 0 , 255 );
		else if ( getButtonState() == BS_PRESS )
		{
			RDSystem.setColor( 255 , 255 , 255 , 255 );
			pos += Vec2i(2,2);
		}

		font->print( 
			pos , title.c_str() , 
			FF_ALIGN_HCENTER | FF_ALIGN_VCENTER );

		RDSystem.setColor( 255 , 255 , 255 , 255 );

	}


	void ZStaticText::onRender()
	{
		IRenderSystem& RDSystem = getUISystem()->getRenderSystem();

		ZFont* font = RDSystem.getFontRes( fontType );

		RDSystem.setWorldIdentity();
		RDSystem.setColor( fontColor );
		font->print( 
			getWorldPos() + getSize() / 2  , title.c_str() , 
			FF_ALIGN_HCENTER | FF_ALIGN_VCENTER );
	}


	void ZCheckButton::onRender()
	{
		IRenderSystem& RDSystem = getUISystem()->getRenderSystem();

		ITexture2D* tex = getTexture();
		int w =  tex->getWidth() / tex->col;

		Vec2i pos = getWorldPos();
		Vec2i size = getSize();

		RDSystem.setWorldIdentity();

		RDSystem.translateWorld( pos.x , pos.y  );
		RDSystem.pushWorldTransform();
		RDSystem.translateWorld( -frontGap , 0 );
		RDSystem.drawBitmap( *tex , 
			Vector2( isCheck()? 0 : w , 0 ),
			Vector2( w , tex->getHeight() ) );
		RDSystem.popWorldTransform();


		tex = getUISystem()->getTexture( IMAGE_DIALOG_CHECKBOXLINE );
		int lineLen = titleLen - size.x - 10;
		RDSystem.pushWorldTransform();
		RDSystem.translateWorld( size.x , size.y - 9 );
		RDSystem.drawBitmap( *tex , Vector2(0 , 0 ) , 
			Vector2( lineLen , tex->getHeight() ) );

		RDSystem.translateWorld( lineLen , 0 );
		tex = getUISystem()->getTexture( IMAGE_DIALOG_CHECKBOXCAP );
		RDSystem.drawBitmap( *tex );
		RDSystem.popWorldTransform();

		ZFont* font = RDSystem.getFontRes( FONT_MAIN10 );

		font->print( 
			pos + Vec2i( size.x + lineLen / 2 , size.y / 2 ) , 
			title.c_str() , FF_ALIGN_VCENTER | FF_ALIGN_HCENTER );
	}

	void ZPanel::onRender()
	{
		IRenderSystem& RDSystem = getUISystem()->getRenderSystem();

		int const frontGap = 11;
		int const backGap  = 2;
		int const downGap  = 9;

		int const cornerLen = 70;
		int const centerOffset = 10;

		ITexture2D* tex = getTexture();

		int  wSide = tex->getWidth() - 2* cornerLen - ( frontGap + backGap );
		int  hSide = tex->getHeight() - 2* cornerLen - ( downGap  );

		int  lx = frontGap;
		int  mx = lx + cornerLen;
		int  rx = mx + wSide;
		int  ty = 0;
		int  my = ty + cornerLen;
		int  by = my + hSide;

		Vec2i const& size = getSize();

		int wBody = size.x - 2 * cornerLen;
		int hBody = size.y - 2 * cornerLen;

		int offsetX = cornerLen + wBody;
		int offsetY = cornerLen + hBody;
		RDSystem.setWorldIdentity();

		Vec2i pos = getWorldPos();
		RDSystem.translateWorld( pos );

		RDSystem.pushWorldTransform();
		RDSystem.drawBitmap( *tex , Vector2(lx , ty) , Vector2(cornerLen , cornerLen) );
		RDSystem.translateWorld( 0 , offsetY );
		RDSystem.drawBitmap( *tex , Vector2(lx , by) , Vector2(cornerLen , cornerLen) );
		RDSystem.popWorldTransform();

		RDSystem.pushWorldTransform();
		RDSystem.translateWorld( offsetX , 0 );
		RDSystem.drawBitmap( *tex , Vector2(rx , ty) , Vector2(cornerLen , cornerLen) );
		RDSystem.translateWorld( 0 , offsetY );
		RDSystem.drawBitmap( *tex , Vector2(rx , by) , Vector2(cornerLen , cornerLen) );
		RDSystem.popWorldTransform();

		int body;
		RDSystem.pushWorldTransform();
		RDSystem.translateWorld( cornerLen , 0 );
		body = wBody;
		for( ;body > wSide ; body-=wSide )
		{
			RDSystem.pushWorldTransform();
			RDSystem.drawBitmap( *tex , Vector2(mx , ty) , Vector2(wSide , cornerLen) );
			RDSystem.translateWorld( 0 , offsetY );
			RDSystem.drawBitmap( *tex , Vector2(mx , by) , Vector2(wSide , cornerLen) );
			RDSystem.popWorldTransform();
			RDSystem.translateWorld( wSide , 0 );
		}
		RDSystem.drawBitmap( *tex , Vector2(mx , ty) , Vector2(body , cornerLen) );
		RDSystem.translateWorld( 0 , offsetY );
		RDSystem.drawBitmap( *tex , Vector2(mx , by) , Vector2(body , cornerLen) );
		RDSystem.popWorldTransform();


		RDSystem.pushWorldTransform();
		RDSystem.translateWorld( 0 , cornerLen );
		body = hBody;
		for( ;body > hSide ; body-= hSide )
		{
			RDSystem.pushWorldTransform();
			RDSystem.drawBitmap( *tex , Vector2(lx , my) , Vector2(cornerLen , hSide) );
			RDSystem.translateWorld( offsetX , 0 );
			RDSystem.drawBitmap( *tex , Vector2(rx , my) , Vector2(cornerLen , hSide) );
			RDSystem.popWorldTransform();
			RDSystem.translateWorld( 0 , hSide );
		}
		RDSystem.drawBitmap( *tex , Vector2(lx , my), Vector2(cornerLen , body) );
		RDSystem.translateWorld( offsetX , 0 );
		RDSystem.drawBitmap( *tex , Vector2(rx , my) , Vector2(cornerLen , body) );
		RDSystem.popWorldTransform();


		RDSystem.pushWorldTransform();
		RDSystem.translateWorld( cornerLen , cornerLen );
		int yBody;
		for( body = wBody ; body > 0 ; body-=wSide )
		{
			RDSystem.pushWorldTransform();
			for( yBody = hBody ; yBody > hSide ; yBody-= hSide )
			{
				RDSystem.drawBitmap( *tex , Vector2(mx , my ), Vector2(wSide , hSide) );
				RDSystem.translateWorld( 0 , hSide );
			}

			RDSystem.drawBitmap( *tex , Vector2( mx , my ) , Vector2(wSide , yBody) );
			RDSystem.popWorldTransform();
			RDSystem.translateWorld( wSide , 0  );
		}

		for( yBody = hBody ; yBody > hSide ; yBody-= hSide )
		{
			RDSystem.drawBitmap( *tex , Vector2( mx , my ) , Vector2(body , hSide) );
			RDSystem.translateWorld( 0 , body );
		}
		RDSystem.drawBitmap( *tex , Vector2( mx , my ) , Vector2( body , yBody ) );
		RDSystem.popWorldTransform();

	}

	void LevelTitleTask::onRender( IRenderSystem& RDSystem )
	{
		float step = 15.0f;

		ZRenderUtility::drawPath( RDSystem , 
			*curPath , int( pathPos / step ) * step , step );

		ZLevelInfo const& info = mLevelStage->getLevel().getInfo();
		ZFont* font;
		font = RDSystem.getFontRes( FONT_HUGE );

		int w = font->calcStringWidth( mLevelStage->getLevelTitle().c_str() );
		RDSystem.setWorldIdentity();
		RDSystem.translateWorld( g_ScreenWidth - 20 - w / 2, g_ScreenHeight - 100 );
		RDSystem.scaleWorld( scaleFontT , scaleFontT );
		font->print( Vector2( 0 , 0 ) , mLevelStage->getLevelTitle().c_str() , 
			FF_ALIGN_VCENTER | FF_ALIGN_HCENTER | FF_LOCAL_POS );

		font = RDSystem.getFontRes( FONT_BROWNTITLE );

		w = font->calcStringWidth( info.nameDisp.c_str() );
		RDSystem.setWorldIdentity();
		RDSystem.translateWorld( g_ScreenWidth - 25 - w / 2, g_ScreenHeight - 50  );
		RDSystem.scaleWorld( scaleFontB , scaleFontB );

		font->print( Vector2( 0 , 0 ) , info.nameDisp.c_str() , 
			FF_ALIGN_VCENTER | FF_ALIGN_HCENTER | FF_LOCAL_POS );
	}


	void ShowTextTask::onRender( IRenderSystem& RDSystem )
	{
		ZFont* font = RDSystem.getFontRes( fontType );

		RDSystem.setColor( color );
		font->print( pos , str.c_str() , FF_ALIGN_VCENTER | FF_ALIGN_HCENTER );
		RDSystem.setColor( 255 , 255 , 255 , 255 );
	}

	void LevelQuakeTask::onRender( IRenderSystem& RDSystem )
	{
		ZRenderUtility::drawQuakeMap( RDSystem , mQMap , mPreLvInfo.BGImage.get() , bgAlphaPos );
	}

	class ScaleEffect : public IRenderEffect
	{
	public:
		virtual void preRenderEffect( IRenderSystem& RDSystem )
		{
			RDSystem.scaleWorld( factor , factor );
		}
		virtual void postRenderEffect( IRenderSystem& RDSystem ) = 0;
		float factor;
	};

}//namespace Zuma
