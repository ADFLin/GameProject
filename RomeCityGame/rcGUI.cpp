#include "rcPCH.h"
#include "rcGUI.h"

#include "rcRenderSystem.h"
#include "rcGameData.h"
#include "rcGUIData.h"
#include "rcControl.h"
#include "rcMapLayerTag.h"

void rcWidget::procEvent( int evtID , void* data )
{
	if ( mFuncEvt )
		mFuncEvt( this , evtID , data );

	rcWidget* cur = this;
	do
	{
		if ( cur->isTop() )
		{
			rcGUISystem::Get().procUIEvent( this , evtID , data );
			break;
		}
		rcWidget* parent = static_cast< rcWidget* >(cur->getParent());
		if ( !parent->onChildEvent( this , evtID , data ) )
			break;
		cur = parent;
	}
	while( cur != nullptr );
}

rcWidgetInfo const& rcWidget::getWidgetInfo()
{
	return rcDataManager::Get().getWidgetInfo( mID );
}

Vec2i rcWidget::calcUISize( unsigned texID , int yNum )
{
	rcTexture* tex = getRenderSystem()->getTexture( texID );
	return Vec2i( tex->getWidth() , tex->getHeight() / yNum );
}


void rcButtonTex3::onRender()
{
	rcRenderSystem* rs = getRenderSystem();
	Graphics2D& g = rs->getGraphics2D();

	rcTexture* tex = rs->getTexture( getWidgetInfo().texBase );

	int frame = 1;
	switch ( getButtonState() )
	{
	case BS_NORMAL:    frame = 3; break;
	case BS_PRESS:     frame = 2; break;
	case BS_HIGHLIGHT: frame = 1; break;
	}

	Vec2i const& size = getSize();
	g.drawTexture( *tex , getWorldPos() , Vec2i( 0 , size.y * frame ) , size );
}

void rcButtonTex4::onRender()
{
	rcRenderSystem* rs = getRenderSystem();
	Graphics2D& g = rs->getGraphics2D();

	rcTexture* tex = rs->getTexture( getWidgetInfo().texBase );

	int frame = 1;
	switch ( getButtonState() )
	{
	case BS_NORMAL:    frame = isEnable() ? 0 : 3 ; break;
	case BS_PRESS:     frame = 2; break;
	case BS_HIGHLIGHT: frame = 1; break;
	}

	Vec2i const& size = getSize();
	g.drawTexture( *tex , getWorldPos() , Vec2i( 0 , size.y * frame ) , size );
}




rcWidget* rcGUISystem::loadUI( int uiID , rcWidget* parent )
{
	rcWidgetInfo const& info = rcDataManager::Get().getWidgetInfo( uiID );

	rcWidget* widget = NULL;
	switch( info.group )
	{
	case UG_BUILD_BTN:
		widget = new rcButtonTex4( info.id , info.texBase , info.pos , parent );
		break;
	case UG_PANEL:
		widget = new rcPanel( info.id , info.texBase , info.pos , parent );
		break;
	}

	if ( parent == NULL )
		mManager.addWidget( widget );

	return widget;
}


void rcGUISystem::procUIEvent( rcWidget* widget , int evtID , void* data )
{
	switch( widget->getID() )
	{
	case  UI_BLD_HOUSE_BTN:
		mControl->changeBuildTag( BT_HOUSE_SIGN ); 
		break;
	case  UI_BLD_ROAD_BTN :
		mControl->changeBuildTag( BT_ROAD ); 
		break;
	case  UI_BLD_CLEAR_BTN: 
		mControl->setMode( rcControl::eClear ); 
		break;
	case  UI_BLD_WATER_BTN:
	case  UI_BLD_GOV_BTN:
	case  UI_BLD_ENT_BTN:
	case  UI_BLD_EDU_BTN:
		break;
	}
}

void rcPanel::onRender()
{
	rcRenderSystem* rs = getRenderSystem();
	Graphics2D& g = rs->getGraphics2D();

	rcTexture* tex = rs->getTexture( getWidgetInfo().texBase );
	g.drawTexture( *tex , getWorldPos() );
}
