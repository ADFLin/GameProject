#include "TinyGamePCH.h"
#include "GameWidget.h"

#include "GameGUISystem.h"
#include "GameGlobal.h"
#include "StageBase.h"
#include <cmath>
#include "RenderUtility.h"


WidgetRenderer  gRenderer;


void WidgetRenderer::drawButton( IGraphics2D& g , Vec2i const& pos , Vec2i const& size , ButtonState state , int color , bool beEnable )
{
	Vec2i renderPos = pos;

	//switch ( state )
	//{
	//case BS_HIGHLIGHT:
	//case BS_NORMAL:
	//case BS_PRESS:
	//	RenderUtility::setPen( g , Color::eGray );
	//	break;
	//	//case BS_PRESS:
	//	//	color = YELLOW;
	//	//	RenderUtility::setPen( g , GRAY );
	//	//	break;
	//}
	if ( state == BS_PRESS )
		renderPos += Vec2i( 1 , 1 );

	if ( state == BS_HIGHLIGHT )
		RenderUtility::setPen( g , Color::eWhite );
	else
		RenderUtility::setPen( g , Color::eBlack );

	RenderUtility::setBrush( g , (beEnable) ? color : Color::eGray , COLOR_NORMAL );
	RenderUtility::drawCapsuleX( g , renderPos , size );

	RenderUtility::setPen( g , Color::eNull );

	RenderUtility::setBrush( g ,  Color::eWhite , COLOR_NORMAL );
	RenderUtility::drawCapsuleX( g , renderPos + Vec2i( 3 , 2 ) , size - Vec2i( 4 , 6 ) );

	RenderUtility::setBrush( g , (beEnable) ? color : Color::eWhite , COLOR_DEEP );
	RenderUtility::drawCapsuleX( g , renderPos + Vec2i( 3 , 4  ) , size - Vec2i( 4 , 6 ) );
}

void WidgetRenderer::drawButton2( IGraphics2D& g , Vec2i const& pos , Vec2i const& size , ButtonState state , int color , bool beEnable /*= true */ )
{

}



void WidgetRenderer::drawPanel( IGraphics2D& g , Vec2i const& pos , Vec2i const& size , ColorKey3 const& color )
{
	RenderUtility::setBrush( g ,  Color::eWhite  );
	RenderUtility::setPen( g , Color::eBlack );
	g.drawRoundRect( pos , size , Vec2i( 10 , 10 ) );
	g.setBrush( color );
	g.drawRoundRect( pos + Vec2i( 2 , 2 ) ,size - Vec2i( 4, 4 ) , Vec2i( 20 , 20 ) );

}

void WidgetRenderer::drawPanel( IGraphics2D& g , Vec2i const& pos , Vec2i const& size , ColorKey3 const& color , float alpha )
{
	g.beginBlend( pos , size , alpha );
	drawPanel( g , pos , size , color );
	g.endBlend();
}

void WidgetRenderer::drawPanel2( IGraphics2D& g , Vec2i const& pos , Vec2i const& size  , ColorKey3 const& color  )
{
	int border = 2;
	RenderUtility::setPen( g , Color::eBlack );
	RenderUtility::setBrush( g , Color::eWhite );
	g.drawRect( pos , size );
	g.setBrush( color );
	g.drawRect( pos + Vec2i( border ,border) , size - 2 * Vec2i( border , border ) );
}

void WidgetRenderer::drawSilder( IGraphics2D& g , Vec2i const& pos , Vec2i const& size , Vec2i const& tipPos , Vec2i const& tipSize )
{
	RenderUtility::setBrush( g , Color::eBlue );
	g.drawRoundRect( pos ,size , Vec2i( 3 , 3 ) );
	RenderUtility::setBrush( g , Color::eYellow , COLOR_DEEP );
	g.drawRoundRect( tipPos , tipSize , Vec2i( 3 , 3 ) );
	RenderUtility::setBrush( g , Color::eYellow );
	g.drawRoundRect( tipPos + Vec2i( 2 , 2 ) , tipSize - Vec2i( 4 , 4 ) , Vec2i( 1 , 1 ) );
}

GWidget::GWidget( Vec2i const& pos , Vec2i const& size , GWidget* parent ) 
:WidgetCoreT< GWidget >( pos , size , parent )
{
	mID = UI_ANY;
	callback   = NULL;
	motionTask = NULL; 
	mFontType  = FONT_S8;
	useHotKey  = false;
}

GWidget::~GWidget()
{
	removeMotionTask();
	if ( callback )
		callback->release();

	if( useHotKey )
		::Global::GUI().removeHotkey( this );
}

void GWidget::sendEvent( int eventID )
{
	GUI::Core* ui   = this;
	while ( ui != nullptr )
	{
		if ( ui->isTop())
			break;

		ui = ui->getParent();
		GWidget* widget = static_cast< GWidget* > ( ui );
		if ( !widget->onChildEvent( eventID, mID , this ) )
			return;
	}

	if ( mID < UI_WEIGET_ID )
		::Global::GUI().sendMessage( eventID , mID , this );

}

void GWidget::removeMotionTask()
{
	if ( motionTask )
		motionTask->stop();
}

void GWidget::setRenderCallback( RenderCallBack* cb )
{
	if ( callback ) 
		callback->release(); 

	callback = cb;
}

void GWidget::setHotkey( ControlAction key )
{
	::Global::GUI().addHotkey( this , key );
	useHotKey = true;
}

GWidget* GWidget::findChild( int id , GWidget* start )
{
	for( auto child = createChildrenIterator(); child; ++child )
	{
		if( child->mID == id )
			return &(*child);
	}
	return NULL;
}

void GWidget::onPostRender()
{
	if ( callback )
		callback->postRender( this );
}

bool GWidget::doClipTest()
{
	if ( isTop() )
		return true;
	return true;
}

WidgetRenderer& GWidget::getRenderer()
{
	return gRenderer;
}

Vec2i const GMsgBox::BoxSize( 300 , 150 );

GMsgBox::GMsgBox( int _id , Vec2i const& pos , GWidget* parent , unsigned flag ) 
	:GPanel( _id , pos , BoxSize , parent )
{
	GButton* button;

	switch( flag )
	{
	case GMB_YESNO:
		button = new GButton( UI_NO , BoxSize - Vec2i( 50 , 30 ) , Vec2i( 40 , 20 ) , this );
		button->setTitle( LAN("No") );
		button->setHotkey( ACT_BUTTON0 );
		button = new GButton( UI_YES , BoxSize - Vec2i( 100 , 30 ) , Vec2i( 40 , 20 ) , this );
		button->setTitle( LAN("Yes") );
		button->setHotkey( ACT_BUTTON1 );
		break;
	case GMB_OK :
		button = new GButton( UI_OK , BoxSize - Vec2i( 100 , 30 ) , Vec2i( 40 , 20 ) , this );
		button->setTitle( LAN("OK") );
		button->setHotkey( ACT_BUTTON0 );
		break;
	}
}


bool GMsgBox::onChildEvent( int event , int id , GWidget* ui )
{
	if ( event != EVT_BUTTON_CLICK )
		return true;

	switch ( id )
	{
	case UI_OK  : sendEvent( EVT_BOX_OK  ); destroy(); return true;
	case UI_YES : sendEvent( EVT_BOX_YES ); destroy(); return true;
	case UI_NO :  sendEvent( EVT_BOX_NO  ); destroy(); return true;
	}
	return false;
}

void GMsgBox::onRender()
{
	GPanel::onRender();

	IGraphics2D& g = Global::getIGraphics2D();

	Vec2i pos = getWorldPos();
	RenderUtility::setFont( g , FONT_S12 );
	g.setTextColor(255 , 255 , 0 );
	g.drawText( pos , Vec2i( BoxSize.x , BoxSize.y - 30 ) , mTitle.c_str() );
}



UIMotionTask::UIMotionTask( GWidget* _ui , Vec2i const& _ePos , long time , WidgetMotionType type ) 
	:LifeTimeTask( time )
	,ui(_ui),ePos(_ePos),tParam( time ),mType( type )
{
	sPos = ui->getPos();
}

void UIMotionTask::release()
{
	delete this;
}

void UIMotionTask::LineMotion()
{
	Vec2i dir = ePos - sPos;

	//float len = sqrt( (float)dir.length2() );

	int x = int( ePos.x - float( getLifeTime() ) * dir.x / ( tParam ));
	int y = int( ePos.y - float( getLifeTime() ) * dir.y / ( tParam ));

	ui->setPos( Vec2i(x,y) );
}

void UIMotionTask::ShakeMotion()
{
	float const PI = 3.1415926f;
	if ( getLifeTime() == 0 )
		setLiftTime( tParam );

	Vec2i dir = ePos - sPos;

	float st =  sin( 2 * PI * float( getLifeTime() ) / tParam );

	int x = sPos.x + int( st * dir.x );
	int y = sPos.y + int( st * dir.y );

	ui->setPos( Vec2i(x,y) );
}

void UIMotionTask::onEnd( bool beComplete )
{
	ui->motionTask = NULL;

	switch( mType )
	{
	case WMT_LINE_MOTION: ui->setPos( ePos ); break;
	case WMT_SHAKE_MOTION: ui->setPos( sPos ); break;
	}
}

bool UIMotionTask::onUpdate( long time )
{
	if ( !LifeTimeTask::onUpdate( time ) )
		setLiftTime( 0 );

	switch( mType )
	{
	case WMT_LINE_MOTION: LineMotion(); break;
	case WMT_SHAKE_MOTION: ShakeMotion(); break;
	}

	return getLifeTime() > 0;
}

void UIMotionTask::onStart()
{
	if ( ui->motionTask )
		ui->motionTask->skip();

	 ui->motionTask = this;
}

GButton::GButton( int id , Vec2i const& pos , Vec2i const& size , GWidget* parent ) 
	:BaseClass( id , pos , size , parent )
{
	setColor( Color::eBlue );
}

void GButton::onMouse( bool beInside )
{
	BaseClass::onMouse( beInside );

	if ( beInside )
	{
		if ( !motionTask )
		{
			TaskBase* task = new UIMotionTask( this , getPos() + Vec2i( 0 , 2 ) , 500 , WMT_SHAKE_MOTION );
			::Global::GUI().addTask( task );
		}
	}
	else
	{
		UIMotionTask* task = dynamic_cast< UIMotionTask* >( motionTask );
		if ( task  && task->mType == WMT_SHAKE_MOTION )
		{
			motionTask->stop();
			motionTask = NULL;
		}
	}
}

void GButton::onRender()
{
	IGraphics2D& g = Global::getIGraphics2D();

	Vec2i pos  = getWorldPos();
	Vec2i size = getSize();

	//assert( !useColorKey );
	gRenderer.drawButton( g , pos , size , getButtonState() , mColor  , isEnable() );

	if ( !mTitle.empty() )
	{
		RenderUtility::setFont( g , mFontType );

		if ( getButtonState() == BS_PRESS )
		{
			g.setTextColor(255 , 0 , 0 );
			pos.x += 2;
			pos.y += 2;
		}
		else
		{
			g.setTextColor(255 , 255 , 0 );
		}
		g.drawText( pos , size , mTitle.c_str() , true );
	}
}

GNoteBook::GNoteBook( int id , Vec2i const& pos , Vec2i const& size , GWidget* parent ) 
	:GUI::NoteBookT< GNoteBook >(  pos , size , parent )
{
	mID = id;
}

void GNoteBook::doRenderButton( PageButton* button )
{
	Graphics2D&  g = Global::getGraphics2D();

	Vec2i pos = button->getWorldPos();
	//g.drawRect( pos , button->getSize() );

	int color = button->page == getCurPage() ? ( Color::eGreen ) : ( Color::eBlue );
	RenderUtility::drawBlock( g , pos , button->getSize() , color );
	//uiSys._drawButton( pos , button->getSize()  , button->getButtonState() , color );

	if ( !button->title.empty() )
	{
		RenderUtility::setFont( g , FONT_S8 );

		if ( button->page == getCurPage() )
		{
			g.setTextColor(255 , 255 , 255 );
		}
		else
		{
			g.setTextColor(255 , 255 , 0 );
		}

		g.drawText( pos , button->getSize() , button->title.c_str() );
	}
}

void GNoteBook::doRenderPage( Page* page )
{
	Vec2i pos = page->getWorldPos();
	Vec2i size = page->getSize();

	Graphics2D& g = Global::getGraphics2D();

	//g.beginBlend( pos , size  , 0.6 );
	{
		RenderUtility::setBrush( g , Color::eNull );
		g.setPen( ColorKey3(255,255,255) , 0 );
		g.drawLine( pos , pos + Vec2i( size.x - 1, 0 ) );
		//g.drawRect( pos  , size  );
	}
	//g.endBlend();
	//page->postRender();

}

void GNoteBook::doRenderBackground()
{
	IGraphics2D& g = Global::getIGraphics2D();
	gRenderer.drawPanel( g , getWorldPos() , getSize() , ColorKey3( 50 , 100 , 150 ) , 0.9f );
}

GTextCtrl::GTextCtrl( int id , Vec2i const& pos , int length , GWidget* parent ) 
	:BaseClass( pos , Vec2i( length , UI_Height ) , parent )
{
	mID = id;
}

void GTextCtrl::onRender()
{
	Vec2i pos = getWorldPos();

	IGraphics2D& g = Global::getIGraphics2D();

	Vec2i size = getSize();
	RenderUtility::setBrush( g , Color::eGray  );
	g.drawRect( pos , size );
	RenderUtility::setBrush( g , Color::eGray , COLOR_LIGHT );

	Vec2i inPos = pos + Vec2i( 2 , 2 );
	Vec2i inSize = size - Vec2i( 4 , 4 );
	g.drawRect( inPos , inSize );

	if ( isEnable() )
		g.setTextColor(0 , 0 , 0 );
	else
		g.setTextColor(125 , 125 , 125 );

	RenderUtility::setFont( g , FONT_S10 );

	Vec2i textPos = pos + Vec2i( 6 , 3 );
	g.drawText( textPos , getValue() );


	static int blink = 0;

	if ( isFocus() )
	{
		int const blinkCycleTime = 1400;
		blink += gDefaultTickTime;
		if ( blink > blinkCycleTime )
			blink -= blinkCycleTime;

		if ( blink < blinkCycleTime / 2 )
		{
			Vec2i rSize;
			//FIXME
#if 0
			if ( mKeyInPos )
			{
				rSize = g.calcTextExtentSize( getValue() , mKeyInPos );
			}
			else
			{
				rSize = g.calcTextExtentSize( getValue() , 1 );
				rSize.x = 0;
			}
			RenderUtility::setPen( g , Color::eBlack );
			RenderUtility::setBrush( g , Color::eBlack );
			g.drawRect( textPos + Vec2i( rSize.x , 0 ) , Vec2i( 1 ,rSize.y ) );
#endif
		}
	}
}


void GChoice::onRender()
{
	Graphics2D& g = Global::getGraphics2D();

	Vec2i pos = getWorldPos();
	Vec2i size = getSize();

	//assert( !useColorKey );

	int border = 2;
	RenderUtility::setPen( g , Color::eBlack );
	RenderUtility::setBrush( g , Color::eWhite );
	g.drawRoundRect( pos , size ,Vec2i( 5 , 5 ) );
	RenderUtility::setBrush( g , mColor ,  COLOR_DEEP );
	g.drawRoundRect( pos + Vec2i( border ,border) , size - 2 * Vec2i( border , border ) , Vec2i( 2 , 2 ) );

	//::Global::getGUI()._drawPanel( g , pos , size , BS_NORMAL , mColor , true );
	if ( getSelectValue() )
	{
		RenderUtility::setFont( g , FONT_S8 );
		g.setTextColor(255 , 255 , 0 );
		g.drawText( pos , size , getSelectValue() );
	}
}

void GChoice::doRenderItem( Vec2i const& pos , Item& item , bool beLight )
{
	Graphics2D& g = Global::getGraphics2D();

	Vec2i size( getSize().x , getMenuItemHeight() );

	g.beginBlend( pos , size , 0.8f );
	RenderUtility::setPen( g , Color::eBlack );
	RenderUtility::setBrush( g , Color::eBlue , COLOR_LIGHT );
	g.drawRoundRect( pos + Vec2i(4,1), size - Vec2i(8,2) , Vec2i( 5 , 5 ) );
	g.endBlend();

	if ( beLight )
		g.setTextColor(255 , 255 , 255 );
	else
		g.setTextColor(0, 0 , 0 );

	RenderUtility::setFont( g , FONT_S8 );
	g.drawText( pos , size , item.value.c_str() );
}

void GChoice::doRenderMenuBG( Menu* menu )
{
	Graphics2D& g = Global::getGraphics2D();

	Vec2i pos =  menu->getWorldPos();
	Vec2i size = menu->getSize();
	g.beginBlend( pos , size , 0.8f );
	RenderUtility::setPen( g , Color::eBlack );
	RenderUtility::setBrush( g , Color::eBlue  );
	g.drawRoundRect( pos + Vec2i(2,1), size - Vec2i(4,2) , Vec2i( 5 , 5 ) );
	g.endBlend();
}

GChoice::GChoice( int id , Vec2i const& pos , Vec2i const& size , GWidget* parent ) 
	: BaseClass( pos , size , parent )
{
	mID = id;
	setColor( Color::eBlue );
}

GPanel::GPanel( int id , Vec2i const& pos , Vec2i const& size , GWidget* parent ) :BaseClass( pos , size , parent )
	,mAlpha( 0.5f )
	,mRenderType( eRoundRectType )
{
	mID = id;
	setColorKey( ColorKey3( 50 , 100 , 150 ) );
}

GPanel::~GPanel()
{

}

void GPanel::onRender()
{
	Vec2i pos = getWorldPos();
	assert( useColorKey );

	IGraphics2D& g = Global::getIGraphics2D();
	Vec2i size = getSize();


	switch ( mRenderType )
	{
	case eRoundRectType:
		g.beginBlend( pos , size , mAlpha );
		gRenderer.drawPanel( g , pos , size  , mColorKey );
		g.endBlend();
		break;
	case eRectType:
		g.beginBlend( pos , size , mAlpha );
		gRenderer.drawPanel2( g , pos , size , mColorKey );
		g.endBlend();
		break;
	}
}

GListCtrl::GListCtrl( int id , Vec2i const& pos , Vec2i const& size , GWidget* parent ) 
	:BaseClass( pos , size , parent )
{
	mID = id;
	setColor( Color::eBlue );
}

void GListCtrl::doRenderItem( Vec2i const& pos , Item& item , bool beSelected )
{
	IGraphics2D& g = Global::getIGraphics2D();

	Vec2i size( getSize().x , getItemHeight() );

	if ( beSelected )
	{
		g.beginBlend( pos , size , 0.8f );
		RenderUtility::setPen( g , Color::eBlack );
		RenderUtility::setBrush( g , mColor , COLOR_NORMAL );
		g.drawRoundRect( pos + Vec2i(4,1), size - Vec2i(8,2) , Vec2i( 5 , 5 ) );
		g.endBlend();

		g.setTextColor(255 , 255 , 255 );
	}
	else
		g.setTextColor(255 , 255 , 0 );

	RenderUtility::setFont( g , FONT_S10 );
	g.drawText( pos , size , item.value.c_str() );
}

void GListCtrl::doRenderBackground( Vec2i const& pos , Vec2i const& size )
{
	IGraphics2D& g = Global::getIGraphics2D();
	RenderUtility::setPen( g , Color::eBlack );
	RenderUtility::setBrush( g , mColor , COLOR_LIGHT );
	g.drawRoundRect( pos , size , Vec2i( 12 , 12 ) );
}

GFrame::GFrame( int id , Vec2i const& pos , Vec2i const& size , GWidget* parent ) 
	:BaseClass( id , pos , size , parent )
{

}

bool GFrame::onMouseMsg( MouseMsg const& msg )
{
	BaseClass::onMouseMsg( msg );

	static int x , y;
	if ( msg.onLeftDown() )
	{
		x = msg.x();
		y = msg.y();

		setTop();
		getManager()->captureMouse( this );
	}
	else if ( msg.onLeftUp() )
	{
		getManager()->releaseMouse();
	}
	if ( msg.isLeftDown() && msg.isDraging() )
	{
		Vec2i pos = getPos() +Vec2i( msg.x() - x , msg.y() - y );
		setPos( pos );
		x = msg.x();
		y = msg.y();
	}
	return false;
}

Vec2i GSlider::TipSize( 10, 10 );

GSlider::GSlider( int id , Vec2i const& pos , int length , bool beH , GWidget* parent ) 
	:BaseClass( pos , length , TipSize , beH , 0 , 1000 , parent )
{
	mID = id;
}

void GSlider::renderValue( GWidget* widget )
{
	Vec2i pos = getWorldPos();
	Graphics2D& g = Global::getGraphics2D();

	FixString< 256 > str;
	str.format( "%d" , getValue() );
	RenderUtility::setFont( g , FONT_S10 );
	g.drawText( Vec2i( pos.x + getSize().x + 3 , pos.y ) , str );
}

void GSlider::showValue()
{
	setRenderCallback( RenderCallBack::create( this , &GSlider::renderValue ) );
}

void GSlider::onRender()
{
	IGraphics2D& g = Global::getIGraphics2D();
	gRenderer.drawSilder( g , getWorldPos() , getSize() , m_tipUI->getWorldPos() , TipSize );
}

GCheckBox::GCheckBox(int id , Vec2i const& pos , Vec2i const& size , GWidget* parent) 
	:BaseClass( id , pos , size , parent )
{
	isCheck = false;
	mID = id;
}

void GCheckBox::onRender()
{
	IGraphics2D& g = Global::getIGraphics2D();
	Vec2i pos  = getWorldPos();
	Vec2i size = getSize();

	if ( isCheck )
	{
		RenderUtility::setBrush( g , Color::eRed );
		RenderUtility::setPen( g , Color::eWhite );
	}
	else
	{
		RenderUtility::setBrush( g , Color::eBlue );
		RenderUtility::setPen( g , Color::eBlack );
	}

	g.drawRect( pos , size );

	RenderUtility::setFont( g , FONT_S12 );
	g.setTextColor(255 , 255 , 0 );
	g.drawText( pos , size , mTitle.c_str() );
}
