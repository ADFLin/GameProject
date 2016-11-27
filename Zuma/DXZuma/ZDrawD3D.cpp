#include "ZumaPCH.h"
#include "D3DRenderSystem.h"

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
	//ZBallBase ball( White );

	//RDSystem.drawLevel();
	//RDSystem.enableBlend( true );

	//RDSystem.setBlendFun( BLEND_ONE , BLEND_ONE );

	//glLineWidth( 5 );

	//for( int i = 0; i < splineVec.size(); ++i )
	//{
	//	axCRSpline& spline = splineVec[i];

	//	Vec2D pos = spline.getPoint( 0 );

	//	glBegin( GL_LINE_STRIP );
	//	glVertex3f( pos.x , pos.y  , 0 );

	//	for ( int i = 1 ; i <= 20 ; ++i )
	//	{
	//		Vec2D pos = spline.getPoint( 0.05f * i );
	//		glVertex3f( pos.x , pos.y  , 0 );
	//	}
	//	glEnd();
	//}

	//glDisable(GL_BLEND);				

	//for( int i = 0; i < vtxVec.size(); ++i )
	//{
	//	Vec2D& pt = vtxVec[i].pos;
	//	ball.setPos( pt );

	//	if ( vtxVec[i].flag & FG_MASK )
	//		ball.setColor( Red );
	//	else
	//		ball.setColor( White );

	//	RDSystem.drawBall( ball , 0 );
	//}
}

}