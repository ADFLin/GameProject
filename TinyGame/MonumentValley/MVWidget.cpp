#include "MVWidget.h"

#include "MVRenderEngine.h"

#include "RHI/DrawUtility.h"
#include "RHI/RHIGraphics2D.h"
#include "RHI/RHIGlobalResource.h"

namespace MV
{
	using namespace Render;

	static ViewInfo WidgetView;
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

		RHIGraphics2D& g = ::Global::GetRHIGraphics2D();
		RHICommandList& commandList = RHICommandList::GetImmediateList();

		Vec2i wPos = getWorldPos();
		Vec2i size = getSize();
		Vec2i screenSize = ::Global::GetScreenSize();

		GrapthicStateScope scope(g);

		RHISetDepthStencilState(commandList, RenderDepthStencilState::GetRHI());
		RHISetViewport(commandList, wPos.x, screenSize.y - size.y, size.x, size.y);

		RenderEngine& re = getRenderEngine();
		{
			float width = 2;
			float height = width * size.y / size.x;
			Mat4 matProj = ReversedZOrthoMatrix( width , width , -10 , 10 );
			Mat4 matView = LookAtMatrix( Vec3f(0,0,0) , -Vec3f( FDir::ParallaxOffset(0) ) , Vector3(0,0,1) );
			Render::MatrixSaveScope Scope( matProj , matView );
			
			RenderContext context;
			context.mView = &WidgetView;
			context.mView->setupTransform(matView, matProj);
			context.setColor(LinearColor(1, 1, 1, 1));
			re.beginRender();
			re.renderMesh(context ,idMesh , Vec3f(0,0,0) , AxisRoataion::Identity() );
			re.endRender();
		}
	}

}//namespace MV
