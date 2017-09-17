#include "ZumaPCH.h"

#include "ZDraw.h"
#include "GLRenderSystem.h"

#include "ZBallConGroup.h"
#include "ResID.h"
#include "ZFont.h"
#include "ZPath.h"

#include "ZLevelManager.h"
#include <functional>

#include "ZStage.h"

namespace Zuma
{

	void ZDevStage::onRender( IRenderSystem& RDSystem )
	{
		ZObject ball( zWhite );

		ZRenderUtility::drawLevelBG( RDSystem , lvInfo );
		RDSystem.enableBlend( true );

		RDSystem.setBlendFun( BLEND_ONE , BLEND_ONE );

		glLineWidth( 5 );

		for( int i = 0; i < splineVec.size(); ++i )
		{
			CRSpline2D& spline = splineVec[i];

			Vec2f pos = spline.getPoint( 0 );

			glBegin( GL_LINE_STRIP );
			glVertex3f( pos.x , pos.y  , 0 );

			for ( int i = 1 ; i <= 20 ; ++i )
			{
				Vec2f pos = spline.getPoint( 0.05f * i );
				glVertex3f( pos.x , pos.y  , 0 );
			}
			glEnd();
		}

		glDisable(GL_BLEND);				

		for( int i = 0; i < vtxVec.size(); ++i )
		{
			Vec2f& pt = vtxVec[i].pos;
			ball.setPos( pt );

			if ( vtxVec[i].flag &  CVData::eMask )
				ball.setColor( zRed );
			else
				ball.setColor( zWhite );

			ZRenderUtility::drawBall( RDSystem , ball , 0 , TOOL_NORMAL );
		}
	}

}//namespace Zuma