#include "MVWidget.h"

#include "MVRenderEngine.h"

#include "RenderGL/GLUtility.h"

namespace MV
{
	using namespace RenderGL;

	MeshViewPanel::MeshViewPanel(int id , Vec2i const& pos , Vec2i const& size , GWidget* parent) 
		:BaseClass( id , pos , size , parent )
	{
		setRenderType( eRectType );
		idMesh = -1;
	}

	void MeshViewPanel::onRender()
	{
		BaseClass::onRender();

		if ( idMesh == -1 )
			return;

		Vec2i wPos = getWorldPos();
		Vec2i size = getSize();
		Vec2i screenSize = ::Global::getDrawEngine()->getScreenSize();
		glPushAttrib( GL_VIEWPORT_BIT );

		glEnable( GL_DEPTH_TEST );
		glViewport( wPos.x , screenSize.y - size.y , size.x , size.y );

		RenderEngine& re = getRenderEngine();
		{
			float width = 2;
			float height = width * size.y / size.x;
			Mat4 matProj = OrthoMatrix( width , width , -10 , 10 );
			Mat4 matView = LookAtMatrix( Vec3f(0,0,0) , -Vec3f( FDir::ParallaxOffset(0) ) , Vector3(0,0,1) );
			RenderGL::MatrixSaveScope Scope( matProj , matView );
			
			re.beginRender( matView );
			glColor3f(1,1,1);
			re.renderMesh( idMesh , Vec3f(0,0,0) , Roataion::Identity() );
			re.endRender();
		}
		glDisable( GL_DEPTH_TEST );
		glPopAttrib();
	}

}//namespace MV
