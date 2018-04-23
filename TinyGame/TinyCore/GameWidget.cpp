#include "TinyGamePCH.h"
#include "GameWidget.h"

#include "GameGUISystem.h"
#include "GameGlobal.h"
#include "StageBase.h"
#include "RenderUtility.h"

#include "FileSystem.h"

#include <cmath>

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
		RenderUtility::SetPen( g , Color::eWhite );
	else
		RenderUtility::SetPen( g , Color::eBlack );

	RenderUtility::SetBrush( g , (beEnable) ? color : Color::eGray , COLOR_NORMAL );
	RenderUtility::DrawCapsuleX( g , renderPos , size );

	RenderUtility::SetPen( g , Color::eNull );

	RenderUtility::SetBrush( g ,  Color::eWhite , COLOR_NORMAL );
	RenderUtility::DrawCapsuleX( g , renderPos + Vec2i( 3 , 2 ) , size - Vec2i( 4 , 6 ) );

	RenderUtility::SetBrush( g , (beEnable) ? color : Color::eWhite , COLOR_DEEP );
	RenderUtility::DrawCapsuleX( g , renderPos + Vec2i( 3 , 4  ) , size - Vec2i( 4 , 6 ) );
}

void WidgetRenderer::drawButton2( IGraphics2D& g , Vec2i const& pos , Vec2i const& size , ButtonState state , int color , bool beEnable /*= true */ )
{

}



void WidgetRenderer::drawPanel( IGraphics2D& g , Vec2i const& pos , Vec2i const& size , Color3ub const& color )
{
	RenderUtility::SetBrush( g ,  Color::eWhite  );
	RenderUtility::SetPen( g , Color::eBlack );
	g.drawRoundRect( pos , size , Vec2i( 10 , 10 ) );
	g.setBrush( color );
	g.drawRoundRect( pos + Vec2i( 2 , 2 ) ,size - Vec2i( 4, 4 ) , Vec2i( 20 , 20 ) );

}

void WidgetRenderer::drawPanel( IGraphics2D& g , Vec2i const& pos , Vec2i const& size , Color3ub const& color , float alpha )
{
	g.beginBlend( pos , size , alpha );
	drawPanel( g , pos , size , color );
	g.endBlend();
}

void WidgetRenderer::drawPanel2( IGraphics2D& g , Vec2i const& pos , Vec2i const& size  , Color3ub const& color  )
{
	int border = 2;
	RenderUtility::SetPen( g , Color::eBlack );
	RenderUtility::SetBrush( g , Color::eWhite );
	g.drawRect( pos , size );
	g.setBrush( color );
	g.drawRect( pos + Vec2i( border ,border) , size - 2 * Vec2i( border , border ) );
}

void WidgetRenderer::drawSilder( IGraphics2D& g , Vec2i const& pos , Vec2i const& size , Vec2i const& tipPos , Vec2i const& tipSize )
{
	RenderUtility::SetPen(g, Color::eBlack);
	RenderUtility::SetBrush( g , Color::eBlue );
	g.drawRoundRect( pos ,size , Vec2i( 3 , 3 ) );
	RenderUtility::SetBrush( g , Color::eYellow , COLOR_DEEP );
	g.drawRoundRect( tipPos , tipSize , Vec2i( 3 , 3 ) );
	RenderUtility::SetBrush( g , Color::eYellow );
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
		button->setTitle( LOCTEXT("No") );
		button->setHotkey( ACT_BUTTON0 );
		button = new GButton( UI_YES , BoxSize - Vec2i( 100 , 30 ) , Vec2i( 40 , 20 ) , this );
		button->setTitle( LOCTEXT("Yes") );
		button->setHotkey( ACT_BUTTON1 );
		break;
	case GMB_OK :
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

	IGraphics2D& g = Global::getIGraphics2D();

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
		RenderUtility::SetFont( g , mFontType );

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

	Graphics2D& g = Global::getGraphics2D();

	//g.beginBlend( pos , size  , 0.6 );
	{
		RenderUtility::SetBrush( g , Color::eNull );
		g.setPen( Color3ub(255,255,255) , 0 );
		g.drawLine( pos , pos + Vec2i( size.x - 1, 0 ) );
		//g.drawRect( pos  , size  );
	}
	//g.endBlend();
	//page->postRender();

}

void GNoteBook::doRenderBackground()
{
	IGraphics2D& g = Global::getIGraphics2D();
	gRenderer.drawPanel( g , getWorldPos() , getSize() , Color3ub( 50 , 100 , 150 ) , 0.9f );
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
	RenderUtility::SetBrush( g , Color::eGray  );
	g.drawRect( pos , size );
	RenderUtility::SetBrush( g , Color::eGray , COLOR_LIGHT );

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
	IGraphics2D& g = Global::getIGraphics2D();

	Vec2i pos = getWorldPos();
	Vec2i size = getSize();

	//assert( !useColorKey );

	int border = 2;
	RenderUtility::SetPen( g , Color::eBlack );
	RenderUtility::SetBrush( g , Color::eWhite );
	g.drawRoundRect( pos , size ,Vec2i( 5 , 5 ) );
	RenderUtility::SetBrush( g , mColor ,  COLOR_DEEP );
	g.drawRoundRect( pos + Vec2i( border ,border) , size - 2 * Vec2i( border , border ) , Vec2i( 2 , 2 ) );

	//::Global::getGUI()._drawPanel( g , pos , size , BS_NORMAL , mColor , true );
	if ( getSelectValue() )
	{
		RenderUtility::SetFont( g , FONT_S8 );
		g.setTextColor( Color3ub(255 , 255 , 0) );
		g.drawText( pos , size , getSelectValue() );
	}
}

void GChoice::doRenderItem( Vec2i const& pos , Item& item , bool beLight )
{
	IGraphics2D& g = Global::getIGraphics2D();

	Vec2i size( getSize().x , getMenuItemHeight() );

	g.beginBlend( pos , size , 0.8f );
	RenderUtility::SetPen( g , Color::eBlack );
	RenderUtility::SetBrush( g , Color::eBlue , COLOR_LIGHT );
	g.drawRoundRect( pos + Vec2i(4,1), size - Vec2i(8,2) , Vec2i( 5 , 5 ) );
	g.endBlend();

	if ( beLight )
		g.setTextColor(Color3ub(255 , 255 , 255) );
	else
		g.setTextColor(Color3ub(0, 0 , 0) );

	RenderUtility::SetFont( g , FONT_S8 );
	g.drawText( pos , size , item.value.c_str() );
}

void GChoice::doRenderMenuBG( Menu* menu )
{
	IGraphics2D& g = Global::getIGraphics2D();

	Vec2i pos =  menu->getWorldPos();
	Vec2i size = menu->getSize();
	g.beginBlend( pos , size , 0.8f );
	RenderUtility::SetPen( g , Color::eBlack );
	RenderUtility::SetBrush( g , Color::eBlue  );
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
	setColorKey( Color3ub( 50 , 100 , 150 ) );
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

bool GPanel::onMouseMsg(MouseMsg const& msg)
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
	setColor( Color::eBlue );
}

void GListCtrl::doRenderItem( Vec2i const& pos , Item& item , bool beSelected )
{
	IGraphics2D& g = Global::getIGraphics2D();

	Vec2i size( getSize().x , getItemHeight() );

	if ( beSelected )
	{
		g.beginBlend( pos , size , 0.8f );
		RenderUtility::SetPen( g , Color::eBlack );
		RenderUtility::SetBrush( g , mColor , COLOR_NORMAL );
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
	IGraphics2D& g = Global::getIGraphics2D();
	RenderUtility::SetPen( g , Color::eBlack );
	RenderUtility::SetBrush( g , mColor , COLOR_LIGHT );
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
	RenderUtility::SetFont( g , FONT_S10 );
	g.drawText( Vec2i( pos.x + getSize().x + 3 , pos.y ) , str );
}

void GSlider::showValue()
{
	setRenderCallback( RenderCallBack::Create( this , &GSlider::renderValue ) );
}

void GSlider::onRender()
{
	IGraphics2D& g = Global::getIGraphics2D();
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
	IGraphics2D& g = Global::getIGraphics2D();
	Vec2i pos  = getWorldPos();
	Vec2i size = getSize();

	int brushColor;
	int colorType[2];
	if ( bChecked )
	{
		brushColor = Color::eRed;
		colorType[0] = COLOR_NORMAL;
		colorType[1] = COLOR_DEEP;
		RenderUtility::SetPen( g , (getButtonState() == BS_HIGHLIGHT) ?  Color::eYellow : Color::eWhite );
	}
	else
	{
		brushColor = Color::eBlue;
		colorType[0] = COLOR_NORMAL;
		colorType[1] = COLOR_DEEP;
		RenderUtility::SetPen( g , (getButtonState() == BS_HIGHLIGHT) ? Color::eYellow : Color::eBlack );
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
	if( getSelection() == -1 )
		return;
	String path = getSelectedFilePath();
	FileSystem::DeleteFile(path.c_str());
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
	if( !FileSystem::FindFiles(mCurDir.c_str(), mSubFileName.c_str(), fileIter) )
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
	skipMouseMsg();
}

void GText::onRender()
{
	IGraphics2D& g = Global::getIGraphics2D();
	Vec2i pos = getWorldPos();
	Vec2i size = getSize();

	RenderUtility::SetFont(g, FONT_S8);
	g.setTextColor(Color3ub(255, 255, 0));
	g.drawText(pos, size, mText.c_str());
}
