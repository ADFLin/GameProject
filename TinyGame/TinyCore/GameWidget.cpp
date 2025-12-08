#include "TinyGamePCH.h"
#include "GameWidget.h"

#include "GameGUISystem.h"
#include "GameGlobal.h"
#include "StageBase.h"
#include "RenderUtility.h"

#include "FileSystem.h"

#include <cmath>
#include "ProfileSystem.h"

WidgetRenderer  gRenderer;


void WidgetRenderer::drawButton( IGraphics2D& g , Vec2i const& pos , Vec2i const& size , ButtonState state , WidgetColor const& color, bool beEnable )
{
	Vec2i renderPos = pos;

	//switch ( state )
	//{
	//case BS_HIGHLIGHT:
	//case BS_NORMAL:
	//case BS_PRESS:
	//	RenderUtility::setPen( g , EColor::Gray );
	//	break;
	//	//case BS_PRESS:
	//	//	color = YELLOW;
	//	//	RenderUtility::setPen( g , GRAY );
	//	//	break;
	//}
	if ( state == BS_PRESS )
		renderPos += Vec2i( 1 , 1 );

	if ( state == BS_HIGHLIGHT )
		RenderUtility::SetPen( g , EColor::White );
	else
		RenderUtility::SetPen( g , EColor::Black );

	if( beEnable )
	{
		color.setupBrush(g, COLOR_NORMAL);
	}
	else
	{
		RenderUtility::SetBrush(g,EColor::Gray, COLOR_NORMAL);
	}
#define DRAW_CAPSULE 1

#if DRAW_CAPSULE
	RenderUtility::DrawCapsuleX( g , renderPos , size );
#else
	g.drawRect(renderPos, size);
#endif
	RenderUtility::SetPen( g , EColor::Null );
	RenderUtility::SetBrush( g ,  EColor::White , COLOR_NORMAL );
#if DRAW_CAPSULE
	RenderUtility::DrawCapsuleX( g , renderPos + Vec2i( 3 , 2 ) , size - Vec2i( 4 , 6 ) );
#else
	g.drawRect(renderPos + Vec2i(3, 2), size - Vec2i(4, 6));
#endif
	if( beEnable )
	{
		color.setupBrush(g, COLOR_DEEP);
	}
	else
	{
		RenderUtility::SetBrush(g, EColor::White, COLOR_DEEP);
	}
#if DRAW_CAPSULE
	RenderUtility::DrawCapsuleX( g , renderPos + Vec2i( 3 , 4  ) , size - Vec2i( 4 , 6 ) );
#else
	g.drawRect(renderPos + Vec2i(3, 4), size - Vec2i(4, 6));
#endif
}

void WidgetRenderer::drawButton2( IGraphics2D& g , Vec2i const& pos , Vec2i const& size , ButtonState state , WidgetColor const& color, bool beEnable /*= true */ )
{

}



void WidgetRenderer::drawPanel( IGraphics2D& g , Vec2i const& pos , Vec2i const& size , WidgetColor const& color)
{
	RenderUtility::SetBrush( g ,  EColor::White  );
	RenderUtility::SetPen( g , EColor::Black );
	g.drawRoundRect( pos , size , Vec2i( 10 , 10 ) );
	color.setupBrush(g);
	g.drawRoundRect( pos + Vec2i( 2 , 2 ) ,size - Vec2i( 4, 4 ) , Vec2i( 20 , 20 ) );

}

void WidgetRenderer::drawPanel( IGraphics2D& g , Vec2i const& pos , Vec2i const& size , WidgetColor const& color, float alpha )
{
	g.beginBlend( pos , size , alpha );
	drawPanel( g , pos , size , color );
	g.endBlend();
}

void WidgetRenderer::drawPanel2( IGraphics2D& g , Vec2i const& pos , Vec2i const& size  , WidgetColor const& color)
{

	int border = 2;
	RenderUtility::SetPen( g , EColor::Black );
	RenderUtility::SetBrush( g , EColor::White );
	g.drawRect( pos , size );
	color.setupBrush(g);
	g.drawRect( pos + Vec2i( border ,border) , size - 2 * Vec2i( border , border ) );
}

void WidgetRenderer::drawSilder( IGraphics2D& g , Vec2i const& pos , Vec2i const& size , Vec2i const& tipPos , Vec2i const& tipSize )
{
	RenderUtility::SetPen(g, EColor::Black);
	RenderUtility::SetBrush( g , EColor::Blue );
	g.drawRoundRect( pos ,size , Vec2i( 3 , 3 ) );
	RenderUtility::SetBrush( g , EColor::Yellow , COLOR_DEEP );
	g.drawRoundRect( tipPos , tipSize , Vec2i( 3 , 3 ) );
	RenderUtility::SetBrush( g , EColor::Yellow );
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
	if (!preSendEvent(eventID))
		return;

	bool bKeep = true;
	if( onEvent )
	{
		bKeep = onEvent(eventID, this);
	}

	if ( mID != UI_ANY && bKeep )
	{
		GUI::Core* ui = this;
		while( ui != nullptr )
		{
			if( ui->isTop() )
				break;

			ui = ui->getParent();
			GWidget* widget = static_cast<GWidget*> (ui);
			if( !widget->onChildEvent(eventID, mID, this) )
				return;
		}

		if( mID < UI_WIDGET_ID )
			::Global::GUI().sendMessage(eventID, mID, this);
	}

}

void GWidget::removeMotionTask()
{
	if ( motionTask )
		motionTask->stop();
}

void GWidget::refresh()
{
	if (canRefresh())
	{
		if (onRefresh)
			onRefresh(this);

		for (auto child = createChildrenIterator(); child; ++child)
		{
			child->refresh();
		}
	}
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

GMsgBox::GMsgBox( int _id , Vec2i const& pos , GWidget* parent , EMessageButton::Type buttonType)
	:GPanel( _id , pos , BoxSize , parent )
{
	GButton* button;

	switch( buttonType )
	{
	case EMessageButton::YesNo:
		button = new GButton( UI_NO , BoxSize - Vec2i( 50 , 30 ) , Vec2i( 40 , 20 ) , this );
		button->setTitle( LOCTEXT("No") );
		button->setHotkey( ACT_BUTTON0 );
		button = new GButton( UI_YES , BoxSize - Vec2i( 100 , 30 ) , Vec2i( 40 , 20 ) , this );
		button->setTitle( LOCTEXT("Yes") );
		button->setHotkey( ACT_BUTTON1 );
		break;
	case EMessageButton::Ok:
		button = new GButton( UI_OK , BoxSize - Vec2i( 50 , 30 ) , Vec2i( 40 , 20 ) , this );
		button->setTitle( LOCTEXT("OK") );
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

	IGraphics2D& g = Global::GetIGraphics2D();

	Vec2i pos = getWorldPos();
	RenderUtility::SetFont( g , FONT_S12 );
	g.setTextColor( Color3ub(255 , 255 , 0) );
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
	setColorName( EColor::Blue );
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
	PROFILE_FUNCTION();

	IGraphics2D& g = Global::GetIGraphics2D();

	Vec2i pos  = getWorldPos();
	Vec2i size = getSize();

	{
		PROFILE_ENTRY("Button");
		//assert( !useColorKey );
		gRenderer.drawButton(g, pos, size, getButtonState(), mColor, isEnable());
	}

	if ( !mTitle.empty() )
	{
		RenderUtility::SetFont( g , mFontType );
		g.setBrush(Color3ub(255, 255, 255));

		if ( getButtonState() == BS_PRESS )
		{
			g.setTextColor( Color3ub(255 , 0 , 0) );
			pos.x += 2;
			pos.y += 2;
		}
		else
		{
			g.setTextColor( Color3ub(255 , 255 , 0) );
		}
		{
			PROFILE_ENTRY("Text");
			g.drawText(pos, size, mTitle.c_str(), false);
		}
		
	}
}

GNoteBook::GNoteBook( int id , Vec2i const& pos , Vec2i const& size , GWidget* parent ) 
	:GUI::NoteBookT< GNoteBook >(  pos , size , parent )
{
	mID = id;
}

void GNoteBook::doRenderButton( PageButton* button )
{
	Graphics2D&  g = Global::GetGraphics2D();

	Vec2i pos = button->getWorldPos();
	//g.drawRect( pos , button->getSize() );

	int color = button->page == getCurPage() ? ( EColor::Green ) : ( EColor::Blue );
	RenderUtility::DrawBlock( g , pos , button->getSize() , color );
	//uiSys._drawButton( pos , button->getSize()  , button->getButtonState() , color );

	if ( !button->title.empty() )
	{
		RenderUtility::SetFont( g , FONT_S8 );

		if ( button->page == getCurPage() )
		{
			g.setTextColor(Color3ub(255 , 255 , 255) );
		}
		else
		{
			g.setTextColor(Color3ub(255 , 255 , 0) );
		}

		g.drawText( pos , button->getSize() , button->title.c_str() );
	}
}

void GNoteBook::doRenderPage( Page* page )
{
	Vec2i pos = page->getWorldPos();
	Vec2i size = page->getSize();

	Graphics2D& g = Global::GetGraphics2D();

	//g.beginBlend( pos , size  , 0.6 );
	{
		RenderUtility::SetBrush( g , EColor::Null );
		g.setPen( Color3ub(255,255,255) , 0 );
		g.drawLine( pos , pos + Vec2i( size.x - 1, 0 ) );
		//g.drawRect( pos  , size  );
	}
	//g.endBlend();
	//page->postRender();

}

void GNoteBook::doRenderBackground()
{
	IGraphics2D& g = Global::GetIGraphics2D();
	WidgetColor color;
	color.setValue(Color3ub(50, 100, 150));
	gRenderer.drawPanel( g , getWorldPos() , getSize() , color , 0.9f );
}

GTextCtrl::GTextCtrl( int id , Vec2i const& pos , int length , GWidget* parent ) 
	:BaseClass( pos , Vec2i( length , UI_Height ) , parent )
{
	mID = id;
}

void GTextCtrl::onRender()
{
	Vec2i pos = getWorldPos();

	IGraphics2D& g = Global::GetIGraphics2D();

	RenderUtility::SetPen(g, EColor::Black);

	Vec2i size = getSize();
	RenderUtility::SetBrush( g , EColor::Gray  );
	g.drawRect( pos , size );
	RenderUtility::SetBrush( g , EColor::Gray , COLOR_LIGHT );

	Vec2i inPos = pos + Vec2i( 2 , 2 );
	Vec2i inSize = size - Vec2i( 4 , 4 );
	g.drawRect( inPos , inSize );

	if ( isEnable() )
		g.setTextColor( Color3ub(0 , 0 , 0) );
	else
		g.setTextColor( Color3ub(125 , 125 , 125) );

	RenderUtility::SetFont( g , FONT_S10 );

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
	IGraphics2D& g = Global::GetIGraphics2D();

	Vec2i pos = getWorldPos();
	Vec2i size = getSize();

	//assert( !useColorKey );

	int border = 2;
	RenderUtility::SetPen( g , EColor::Black );
	RenderUtility::SetBrush( g , EColor::White );
	g.drawRoundRect( pos , size ,Vec2i( 5 , 5 ) );
	mColor.setupBrush( g , COLOR_DEEP );
	g.drawRoundRect( pos + Vec2i( border ,border) , size - 2 * Vec2i( border , border ) , Vec2i( 2 , 2 ) );

	//::Global::getGUI()._drawPanel( g , pos , size , BS_NORMAL , mColor , true );
	if ( getSelectValue() )
	{
		RenderUtility::SetFont( g , FONT_S8 );
		g.setTextColor( Color3ub(255 , 255 , 0) );
		g.drawText( pos , size , getSelectValue() );
	}
}

void GChoice::doRenderItem( Vec2i const& pos , Item& item , bool bSelected )
{
	IGraphics2D& g = Global::GetIGraphics2D();

	Vec2i size( getSize().x , getMenuItemHeight() );

	g.beginBlend( pos , size , 0.8f );
	RenderUtility::SetPen( g , EColor::Black );
	RenderUtility::SetBrush( g , EColor::Blue , COLOR_LIGHT );
	g.drawRoundRect( pos + Vec2i(4,1), size - Vec2i(8,2) , Vec2i( 5 , 5 ) );
	g.endBlend();

	if ( bSelected )
		g.setTextColor(Color3ub(255 , 255 , 255) );
	else
		g.setTextColor(Color3ub(0, 0 , 0) );

	RenderUtility::SetFont( g , FONT_S8 );
	g.drawText( pos , size , item.value.c_str() );
}

void GChoice::doRenderMenuBG( Menu* menu )
{
	IGraphics2D& g = Global::GetIGraphics2D();

	Vec2i pos =  menu->getWorldPos();
	Vec2i size = menu->getSize();
	g.beginBlend( pos , size , 0.8f );
	RenderUtility::SetPen( g , EColor::Black );
	RenderUtility::SetBrush( g , EColor::Blue  );
	g.drawRoundRect( pos + Vec2i(2,1), size - Vec2i(4,2) , Vec2i( 5 , 5 ) );
	g.endBlend();
}

GChoice::GChoice( int id , Vec2i const& pos , Vec2i const& size , GWidget* parent ) 
	: BaseClass( pos , size , parent )
{
	mID = id;
	setColorName( EColor::Blue );
}

GPanel::GPanel( int id , Vec2i const& pos , Vec2i const& size , GWidget* parent ) :BaseClass( pos , size , parent )
	,mAlpha( 0.5f )
	,mRenderType( eRoundRectType )
{
	mID = id;
	setColor( Color3ub( 50 , 100 , 150 ) );
}

GPanel::~GPanel()
{

}

void GPanel::onRender()
{
	Vec2i pos = getWorldPos();

	IGraphics2D& g = Global::GetIGraphics2D();
	Vec2i size = getSize();

	switch ( mRenderType )
	{
	case eRoundRectType:
		g.beginBlend( pos , size , mAlpha );
		gRenderer.drawPanel( g , pos , size  , mColor );
		g.endBlend();
		break;
	case eRectType:
		g.beginBlend( pos , size , mAlpha );
		gRenderer.drawPanel2( g , pos , size , mColor );
		g.endBlend();
		break;
	}
}

MsgReply GPanel::onMouseMsg(MouseMsg const& msg)
{
	if( msg.onLeftDown() )
	{
		setTop();
	}
	return BaseClass::onMouseMsg(msg);
}

GListCtrl::GListCtrl( int id , Vec2i const& pos , Vec2i const& size , GWidget* parent ) 
	:BaseClass( pos , size , parent )
{
	mID = id;
	setColorName( EColor::Blue );
}

void GListCtrl::doRenderItem( Vec2i const& pos , Item& item , bool bSelected )
{
	IGraphics2D& g = Global::GetIGraphics2D();

	Vec2i size( getSize().x , getItemHeight() );

	if (bSelected)
	{
		g.beginBlend( pos , size , 0.8f );
		RenderUtility::SetPen( g , EColor::Black );
		mColor.setupBrush( g , COLOR_NORMAL );
		g.drawRoundRect( pos + Vec2i(4,1), size - Vec2i(8,2) , Vec2i( 5 , 5 ) );
		g.endBlend();

		g.setTextColor(Color3ub(255 , 255 , 255) );
	}
	else
		g.setTextColor(Color3ub(255 , 255 , 0) );

	RenderUtility::SetFont( g , FONT_S10 );
	g.drawText( pos , size , item.value.c_str() );
}

void GListCtrl::doRenderBackground( Vec2i const& pos , Vec2i const& size )
{
	IGraphics2D& g = Global::GetIGraphics2D();
	RenderUtility::SetPen( g , EColor::Black );
	mColor.setupBrush(g, COLOR_LIGHT);
	g.drawRoundRect( pos , size , Vec2i( 12 , 12 ) );
}

GFrame::GFrame( int id , Vec2i const& pos , Vec2i const& size , GWidget* parent ) 
	:BaseClass( id , pos , size , parent )
{

}

// Helper enum for resize edge detection
enum ResizeEdge
{
	RESIZE_NONE   = 0,
	RESIZE_LEFT   = BIT(0),
	RESIZE_RIGHT  = BIT(1),
	RESIZE_TOP    = BIT(2),
	RESIZE_BOTTOM = BIT(3),
	RESIZE_TOP_LEFT = RESIZE_TOP | RESIZE_LEFT,
	RESIZE_TOP_RIGHT = RESIZE_TOP | RESIZE_RIGHT,
	RESIZE_BOTTOM_LEFT = RESIZE_BOTTOM | RESIZE_LEFT,
	RESIZE_BOTTOM_RIGHT = RESIZE_BOTTOM | RESIZE_RIGHT,
};

SUPPORT_ENUM_FLAGS_OPERATION(ResizeEdge);

#ifdef SYS_PLATFORM_WIN
#include "WindowsHeader.h"
#endif


enum class ECursorIcon
{
	SizeWE,
	SizeNS,
	SizeNWSE,
	sizeNESW,
};

class CursorManager
{
public:
	static void SetCursor(ECursorIcon icon)
	{
#ifdef SYS_PLATFORM_WIN
		LPCTSTR cursorName = IDC_ARROW;
		switch (icon)
		{
		case ECursorIcon::SizeWE:
			cursorName = IDC_SIZEWE;
			break;
		case ECursorIcon::SizeNS:
			cursorName = IDC_SIZENS;
			break;
		case ECursorIcon::SizeNWSE:
			cursorName = IDC_SIZENWSE;
			break;
		case ECursorIcon::sizeNESW:
			cursorName = IDC_SIZENESW;
			break;
		}
		::SetCursor(::LoadCursor(NULL, cursorName));
#endif
	}
};

struct ResizeOperation
{
	static const int BorderWidth = 8;

	static ECursorIcon GetCursorIcon(ResizeEdge edge)
	{
		switch (edge)
		{
		case RESIZE_LEFT:
		case RESIZE_RIGHT:
			return ECursorIcon::SizeWE;
		case RESIZE_TOP:
		case RESIZE_BOTTOM:
			return ECursorIcon::SizeNS;
		case RESIZE_TOP_LEFT:
		case RESIZE_BOTTOM_RIGHT:
			return ECursorIcon::SizeNWSE;
		case RESIZE_TOP_RIGHT:
		case RESIZE_BOTTOM_LEFT:
			return ECursorIcon::sizeNESW;
		}
		return ECursorIcon::SizeWE;
	}
	
	ResizeEdge detectEdge(Vec2i const& localPos, Vec2i const& widgetSize)
	{
		ResizeEdge edge = RESIZE_NONE;
		
		if (localPos.x < BorderWidth)
			edge |= RESIZE_LEFT;
		else if (localPos.x >= widgetSize.x - BorderWidth)
			edge |= RESIZE_RIGHT;
			
		if (localPos.y < BorderWidth)
			edge |= RESIZE_TOP;
		else if (localPos.y >= widgetSize.y - BorderWidth)
			edge |= RESIZE_BOTTOM;
			
		return edge;
	}
	
	void start(ResizeEdge edge, Vec2i const& startWorldPos, Vec2i const& widgetPos, Vec2i const& widgetSize)
	{
		resizingEdge = edge;
		startMouseWorldPos = startWorldPos;
		startWidgetPos = widgetPos;
		startWidgetSize = widgetSize;
	}
	
	void end()
	{
		resizingEdge = RESIZE_NONE;
	}
	
	bool isResizing() const
	{
		return resizingEdge != RESIZE_NONE;
	}
	
	void updateResize(Vec2i const& currentWorldPos, Vec2i& outPos, Vec2i& outSize)
	{
		if (!isResizing())
			return;
			
		Vec2i delta = currentWorldPos - startMouseWorldPos;
		Vec2i newPos = startWidgetPos;
		Vec2i newSize = startWidgetSize;

		const int minWidth = 50;
		const int minHeight = 30;
		
		// Handle horizontal resizing
		if (resizingEdge & RESIZE_LEFT)
		{
			int newWidth = startWidgetSize.x - delta.x;
			// Clamp to minimum width
			if (newWidth < minSize.x)
			{
				newWidth = minSize.x;
				// Adjust position to maintain right edge
				newPos.x = startWidgetPos.x + startWidgetSize.x - minSize.x;
			}
			else
			{
				newPos.x = startWidgetPos.x + delta.x;
			}
			newSize.x = newWidth;
		}
		else if (resizingEdge & RESIZE_RIGHT)
		{
			int newWidth = startWidgetSize.x + delta.x;
			// Clamp to minimum width
			newSize.x = (newWidth >= minSize.x) ? newWidth : minSize.x;
		}
		
		// Handle vertical resizing
		if (resizingEdge & RESIZE_TOP)
		{
			int newHeight = startWidgetSize.y - delta.y;
			// Clamp to minimum height
			if (newHeight < minSize.y)
			{
				newHeight = minSize.y;
				// Adjust position to maintain bottom edge
				newPos.y = startWidgetPos.y + startWidgetSize.y - minSize.y;
			}
			else
			{
				newPos.y = startWidgetPos.y + delta.y;
			}
			newSize.y = newHeight;
		}
		else if (resizingEdge & RESIZE_BOTTOM)
		{
			int newHeight = startWidgetSize.y + delta.y;
			// Clamp to minimum height
			newSize.y = (newHeight >= minSize.y) ? newHeight : minSize.y;
		}
		
		outPos = newPos;
		outSize = newSize;
	}
	
	ResizeEdge resizingEdge = RESIZE_NONE;
	Vec2i startMouseWorldPos;  // Use world coordinates instead of local
	Vec2i startWidgetPos;
	Vec2i startWidgetSize;

	Vec2i minSize = Vec2i(50, 30);
};

struct DragOperation
{

	void start(Vec2i const& startPos)
	{
		bStart = true;
		pos = startPos;
	}
	void end()
	{
		bStart = false;
	}
	bool update(Vec2i const& inPos , Vec2i& outOffset )
	{
		if( bStart )
		{
			outOffset = inPos - pos;
			pos = inPos;
		}
		return bStart;
	}

	Vec2i pos;
	bool  bStart;
};

MsgReply GFrame::onMouseMsg( MouseMsg const& msg )
{
	BaseClass::onMouseMsg( msg );

	static ResizeOperation resizeOp;
	static DragOperation dragOp;



	if ( msg.onLeftUp() )
	{
		getManager()->releaseMouse();
		resizeOp.end();
		dragOp.end();
	}
	else if ( msg.onLeftDown() )
	{
		setTop();
		
		ResizeEdge edge = RESIZE_NONE;
		if (bCanResize)
		{
			Vec2i localPos = msg.getPos() - getWorldPos();
			edge = resizeOp.detectEdge(localPos, getSize());
		}
		if (edge != RESIZE_NONE)
		{
			getManager()->captureMouse( this );
			resizeOp.start(edge, msg.getPos(), getPos(), getSize());
			resizeOp.minSize = mMinSize;
			CursorManager::SetCursor(ResizeOperation::GetCursorIcon(edge));
		}
		else
		{
			getManager()->captureMouse( this );
			dragOp.start(msg.getPos());
		}
	}	
	else if (msg.onMoving())
	{
		if (bCanResize)
		{
			if (resizeOp.isResizing())
			{
				CursorManager::SetCursor(ResizeOperation::GetCursorIcon(resizeOp.resizingEdge));
			}
			else if (!dragOp.bStart)
			{
				Vec2i localPos = msg.getPos() - getWorldPos();
				ResizeEdge edge = resizeOp.detectEdge(localPos, getSize());
				if (edge != RESIZE_NONE)
				{
					CursorManager::SetCursor(ResizeOperation::GetCursorIcon(edge));
				}
			}
		}
	}


	if ( msg.isLeftDown() && msg.isDraging() )
	{
		if (resizeOp.isResizing())
		{
			Vec2i newPos, newSize;
			resizeOp.updateResize(msg.getPos(), newPos, newSize);
			setPos(newPos);
			setSize(newSize);
		}
		else if (dragOp.bStart)
		{
			Vec2i offset;
			if( dragOp.update(msg.getPos(),offset) )
			{
				setPos(getPos() + offset);
			}
		}
	}

	return MsgReply::Handled();
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
	Vec2i size = getSize();
	IGraphics2D& g = Global::GetIGraphics2D();

	if (onGetShowValue)
	{
		std::string value = onGetShowValue();
		g.drawText(pos, size, value.c_str());
	}
	else
	{
		InlineString< 256 > str;
		str.format("%d", getValue());
		RenderUtility::SetFont(g, FONT_S10);
		g.drawText(pos, size, str);

	}
}

void GSlider::showValue()
{
	setRenderCallback( RenderCallBack::Create( this , &GSlider::renderValue ) );
}

void GSlider::onRender()
{
	IGraphics2D& g = Global::GetIGraphics2D();
	gRenderer.drawSilder( g , getWorldPos() , getSize() , getTipWidget()->getWorldPos() , TipSize );
}

GCheckBox::GCheckBox(int id , Vec2i const& pos , Vec2i const& size , GWidget* parent) 
	:BaseClass( id , pos , size , parent )
{
	bChecked = false;
	mID = id;
}

void GCheckBox::onClick()
{
	bChecked = !bChecked; 
	sendEvent(EVT_CHECK_BOX_CHANGE);
	updateMotionState(true);
}

void GCheckBox::onMouse(bool beInside)
{
	BaseClass::onMouse(beInside);
	updateMotionState(beInside);
}

void GCheckBox::onRender()
{
	IGraphics2D& g = Global::GetIGraphics2D();
	Vec2i pos  = getWorldPos();
	Vec2i size = getSize();

	int brushColor;
	int colorType[2];
	if ( bChecked )
	{
		brushColor = EColor::Red;
		colorType[0] = COLOR_NORMAL;
		colorType[1] = COLOR_DEEP;
		RenderUtility::SetPen( g , (getButtonState() == BS_HIGHLIGHT) ?  EColor::Yellow : EColor::White );
	}
	else
	{
		brushColor = EColor::Blue;
		colorType[0] = COLOR_NORMAL;
		colorType[1] = COLOR_DEEP;
		RenderUtility::SetPen( g , (getButtonState() == BS_HIGHLIGHT) ? EColor::Yellow : EColor::Black );
	}
	RenderUtility::SetBrush(g, brushColor, colorType[0]);
	g.drawRect( pos , size );

	RenderUtility::SetBrush(g, brushColor, colorType[1]);
	RenderUtility::SetPen(g, brushColor);
	g.drawRect(pos + Vec2i(3, 3), size - 2 * Vec2i(3, 3));

	RenderUtility::SetFont( g , FONT_S8 );
	g.setTextColor(Color3ub(255 , 255 , 0) );
	g.drawText((bChecked) ? pos : pos + Vec2i(1,1) , size , mTitle.c_str() );
}



void GCheckBox::updateMotionState( bool bInside )
{
	if( bInside && bChecked == false )
	{
		if( !motionTask )
		{
			TaskBase* task = new UIMotionTask(this, getPos() + Vec2i(0, 2), 500, WMT_SHAKE_MOTION);
			::Global::GUI().addTask(task);
		}
	}
	else
	{
		UIMotionTask* task = dynamic_cast<UIMotionTask*>(motionTask);
		if( task  && task->mType == WMT_SHAKE_MOTION )
		{
			motionTask->stop();
			motionTask = NULL;
		}
	}
}

GFileListCtrl::GFileListCtrl(int id, Vec2i const& pos, Vec2i const& size, GWidget* parent) 
	:BaseClass(id, pos, size, parent)
{

}

String GFileListCtrl::getSelectedFilePath() const
{
	char const* name = getSelectValue();
	if( name )
		return mCurDir + "/" + name;

	return String();
}

void GFileListCtrl::deleteSelectdFile()
{
	if( getSelection() == INDEX_NONE )
		return;
	String path = getSelectedFilePath();
	FFileSystem::DeleteFile(path.c_str());
	removeItem(getSelection());
}

void GFileListCtrl::setDir(char const* dir)
{
	mCurDir = dir;
	refreshFiles();
}

void GFileListCtrl::refreshFiles()
{
	removeAllItem();

	FileIterator fileIter;
	if( !FFileSystem::FindFiles(mCurDir.c_str(), mSubFileName.c_str(), fileIter) )
		return;

	for( ; fileIter.haveMore(); fileIter.goNext() )
	{
		addItem(fileIter.getFileName());
	}
}

GText::GText( Vec2i const& pos, Vec2i const& size, GWidget* parent) 
	:BaseClass(  pos, size, parent)
{
	mID = UI_ANY;
	setRerouteMouseMsgUnhandled();
}

void GText::onRender()
{
	IGraphics2D& g = Global::GetIGraphics2D();
	Vec2i pos = getWorldPos();
	Vec2i size = getSize();

	RenderUtility::SetFont(g, FONT_S8);
	g.setTextColor(Color3ub(255, 255, 0));
	g.drawText(pos, size, mText.c_str());
}

// GScrollBar Implementation

GScrollBar::GScrollBar(int id, Vec2i const& pos, int length, bool beH, GWidget* parent)
	:BaseClass(pos, beH ? Vec2i(length, 16) : Vec2i(16, length), parent)
{
	mID = id;
	mbHorizontal = beH;

	// No child buttons created

	updateThumbSize();
	updateThumbPos();
}

void GScrollBar::setRange(int range, int pageSize)
{
	mRange = range;
	mPageSize = pageSize;
	updateThumbSize();
	setValue(mValue);
}

void GScrollBar::setValue(int value)
{
	int maxVal = std::max(0, mRange);
	int oldVal = mValue;
	mValue = std::max(0, std::min(value, maxVal));
	
	if (oldVal != mValue)
	{
		updateThumbPos();
		onScrollChange(mValue);
	}
}

void GScrollBar::updateThumbSize()
{
	int btnSize = 16;
	int trackLen = (mbHorizontal ? getSize().x : getSize().y) - 2 * btnSize;
	
	if (trackLen <= 0) 
		return;

	int total = mRange + mPageSize;
	if (total <= 0) 
		total = 1;

	int thumbLen = Math::Clamp(trackLen * mPageSize / total, 10, trackLen);
	mThumbSize = mbHorizontal ? Vec2i(thumbLen, 16) : Vec2i(16, thumbLen);
}

void GScrollBar::updateThumbPos()
{
	int btnSize = 16;
	int trackLen = (mbHorizontal ? getSize().x : getSize().y) - 2 * btnSize;
	int thumbLen = mbHorizontal ? mThumbSize.x : mThumbSize.y;
	int movableLen = trackLen - thumbLen;

	if (movableLen <= 0)
	{
		if (mbHorizontal) mThumbPos = Vec2i(btnSize, 0);
		else mThumbPos = Vec2i(0, btnSize);
		return;
	}

	int offset = 0;
	if (mRange > 0)
	{
		offset = mValue * movableLen / mRange;
	}

	if (mbHorizontal)
		mThumbPos = Vec2i(btnSize + offset, 0);
	else
		mThumbPos = Vec2i(0, btnSize + offset);
}

int GScrollBar::calcValueFromPos(Vec2i const& pos)
{
	int btnSize = 16;
	int trackLen = (mbHorizontal ? getSize().x : getSize().y) - 2 * btnSize;
	int thumbLen = mbHorizontal ? mThumbSize.x : mThumbSize.y;
	int movableLen = trackLen - thumbLen;

	if (movableLen <= 0) return 0;

	// Calculate center of thumb relative to track start
	int relativePos = (mbHorizontal ? pos.x : pos.y) - btnSize - thumbLen / 2;
	
	int val = relativePos * mRange / movableLen;
	return Math::Clamp(val, 0, mRange);
}

void GScrollBar::onRender()
{
	IGraphics2D& g = Global::GetIGraphics2D();
	Vec2i pos = getWorldPos();
	Vec2i size = getSize();
	int btnSize = 16;

	// Colors
	Color3ub colorTrack(45, 45, 48);
	Color3ub colorThumb(62, 62, 66);
	Color3ub colorThumbHover(104, 104, 104);
	Color3ub colorThumbPress(150, 150, 150); // Or active color
	Color3ub colorArrow(200, 200, 200);
	Color3ub colorBtnHover(62, 62, 66);
	Color3ub colorBtnPress(0, 122, 204);

	// Draw Track
	g.setBrush(colorTrack);
	RenderUtility::SetPen(g, EColor::Null);
	g.drawRect(pos, size);

	// Draw Buttons
	Vec2i btn1Pos = pos;
	Vec2i btn2Pos = mbHorizontal ? pos + Vec2i(size.x - btnSize, 0) : pos + Vec2i(0, size.y - btnSize);
	Vec2i btnSizeVec(btnSize, btnSize);

	// Button 1 (Minus)
	if (mPressPart == PART_MINUS) g.setBrush(colorBtnPress);
	else if (mHoverPart == PART_MINUS) g.setBrush(colorBtnHover);
	else g.setBrush(colorTrack); // Transparent/Track color
	g.drawRect(btn1Pos, btnSizeVec);

	// Button 2 (Plus)
	if (mPressPart == PART_PLUS) g.setBrush(colorBtnPress);
	else if (mHoverPart == PART_PLUS) g.setBrush(colorBtnHover);
	else g.setBrush(colorTrack);
	g.drawRect(btn2Pos, btnSizeVec);

	// Draw Arrows
	g.setBrush(colorArrow);
	// ... draw triangles ...
	// Helper to draw arrow
	auto drawArrow = [&](Vec2i p, int dir) 
	{
		// dir: 0=left, 1=right, 2=up, 3=down
		Vector2 center = Vector2(p) + Vector2(btnSize / 2.0f, btnSize / 2.0f);
		float s = 4.0f;
		Vector2 v[3];
		if (dir == 0) { v[0] = center + Vector2(-s, 0); v[1] = center + Vector2(s, -s); v[2] = center + Vector2(s, s); }
		else if (dir == 1) { v[0] = center + Vector2(s, 0); v[1] = center + Vector2(-s, -s); v[2] = center + Vector2(-s, s); }
		else if (dir == 2) { v[0] = center + Vector2(0, -s); v[1] = center + Vector2(-s, s); v[2] = center + Vector2(s, s); }
		else if (dir == 3) { v[0] = center + Vector2(0, s); v[1] = center + Vector2(-s, -s); v[2] = center + Vector2(s, -s); }
		g.drawPolygon(v, 3);
	};

	if (mbHorizontal) { drawArrow(btn1Pos, 0); drawArrow(btn2Pos, 1); }
	else { drawArrow(btn1Pos, 2); drawArrow(btn2Pos, 3); }

	// Draw Thumb
	Vec2i thumbWorldPos = pos + mThumbPos;
	if (mPressPart == PART_THUMB) g.setBrush(colorThumbPress);
	else if (mHoverPart == PART_THUMB) g.setBrush(colorThumbHover);
	else g.setBrush(colorThumb);

	// Rounded rect for thumb
	g.drawRoundRect(thumbWorldPos, mThumbSize, Vector2(4, 4));



	//g.enablePen(true);
}

MsgReply GScrollBar::onMouseMsg(MouseMsg const& msg)
{
	Vec2i localPos = msg.getPos() - getWorldPos();
	Vec2i size = getSize();
	int btnSize = 16;

	// Hit testing
	EPart hitPart = PART_NONE;
	if (localPos.x >= 0 && localPos.y >= 0 && localPos.x < size.x && localPos.y < size.y)
	{
		if (mbHorizontal)
		{
			if (localPos.x < btnSize) hitPart = PART_MINUS;
			else if (localPos.x >= size.x - btnSize) hitPart = PART_PLUS;
			else if (localPos.x >= mThumbPos.x && localPos.x < mThumbPos.x + mThumbSize.x) hitPart = PART_THUMB;
			else hitPart = PART_TRACK;
		}
		else
		{
			if (localPos.y < btnSize) hitPart = PART_MINUS;
			else if (localPos.y >= size.y - btnSize) hitPart = PART_PLUS;
			else if (localPos.y >= mThumbPos.y && localPos.y < mThumbPos.y + mThumbSize.y) hitPart = PART_THUMB;
			else hitPart = PART_TRACK;
		}
	}

	if (msg.onMoving())
	{
		if (mHoverPart != hitPart)
		{
			mHoverPart = hitPart;
			// Redraw? GWidget doesn't have invalidate. But onRender is called every frame usually?
			// If not, we might need to trigger update.
		}
	}
	else if (msg.onLeftDown() || msg.onLeftDClick())
	{
		mPressPart = hitPart;
		getManager()->captureMouse(this);

		if (hitPart == PART_THUMB)
		{
			mbDragging = true;
			mDragStartPos = msg.getPos();
			mDragStartValue = mValue;
		}
		else if (hitPart == PART_MINUS)
		{
			setValue(mValue - 1);
		}
		else if (hitPart == PART_PLUS)
		{
			setValue(mValue + 1);
		}
		else if (hitPart == PART_TRACK)
		{
			setValue(calcValueFromPos(localPos));
		}
		return MsgReply::Handled();
	}
	else if (msg.onLeftUp())
	{
		if (mPressPart != PART_NONE)
		{
			mPressPart = PART_NONE;
			mbDragging = false;
			getManager()->releaseMouse();
			return MsgReply::Handled();
		}
	}
	
	if (msg.isLeftDown() && mbDragging)
	{
		// Drag logic (same as before)
		int trackLen = (mbHorizontal ? getSize().x : getSize().y) - 2 * btnSize;
		int thumbLen = mbHorizontal ? mThumbSize.x : mThumbSize.y;
		int movableLen = trackLen - thumbLen;

		if (movableLen > 0)
		{
			int deltaPix = mbHorizontal ? (msg.x() - mDragStartPos.x) : (msg.y() - mDragStartPos.y);
			int deltaVal = deltaPix * mRange / movableLen;
			setValue(mDragStartValue + deltaVal);
		}
		return MsgReply::Handled();
	}

	return BaseClass::onMouseMsg(msg);
}

bool GScrollBar::onChildEvent(int event, int id, GWidget* ui)
{
	if (event == EVT_BUTTON_CLICK)
	{
		if (id == 0) // Minus
		{
			setValue(mValue - 1);
			return false;
		}
		else if (id == 1) // Plus
		{
			setValue(mValue + 1);
			return false;
		}
	}
	return true;
}

void GScrollBar::onResize(Vec2i const& size)
{
	BaseClass::onResize(size);
	updateThumbSize();
	updateThumbPos();
}
