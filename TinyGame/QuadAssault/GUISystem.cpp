#include "GUISystem.h"

#include "Dependence.h"
#include "GameInterface.h"
#include "GameInput.h"

#include "RenderSystem.h"
#include "RenderUtility.h"
#include "Texture.h"


#include "RHI/RHIGraphics2D.h"
#include "TinyCore/RenderUtility.h"

class ScissorClipStack
{
public:
	
	void push( Vec2i const& pos , Vec2i const& size , bool bOverlapPrev );
	void pop();

	typedef TRect< int > SRect;
	bool mPrevEnable;
	std::vector< SRect > mStack;
};

ScissorClipStack gClipStack;

void ScissorClipStack::push( Vec2i const& pos , Vec2i const& size , bool bOverlapPrev )
{
	SRect rect;
	rect.min = pos;
	rect.max = pos + size;

#if 1
	auto& g = IGame::Get().getGraphics2D();
	if (mStack.empty())
	{
		g.beginClip(pos, size);
	}
	else if (bOverlapPrev)
	{
		if (!rect.overlap(mStack.back()))
			rect.max = rect.min;
		Vec2i sizeRect = rect.getSize();
		g.setClipRect(pos, sizeRect);
	}
#else
	if ( mStack.empty() )
	{
		mPrevEnable = glIsEnabled( GL_SCISSOR_TEST );
		if ( !mPrevEnable )
			glEnable( GL_SCISSOR_TEST );
	}
	else if ( bOverlapPrev )
	{
		if ( !rect.overlap( mStack.back() ) )
			rect.max = rect.min;
	}
	Vec2i sizeRect = rect.getSize();
	glScissor( rect.min.x , rect.min.y , sizeRect.x , sizeRect.y );
	
#endif

	mStack.push_back(rect);
}

void ScissorClipStack::pop()
{
	mStack.pop_back();
#if 1
	auto& g = IGame::Get().getGraphics2D();
	if (mStack.empty())
	{	
		g.endClip();
	}
	else
	{
		SRect& rect = mStack.back();
		Vec2i size = rect.getSize();
		g.setClipRect(rect.min, rect.getSize());
	}
#else
	if ( mStack.empty() )
	{
		if ( !mPrevEnable )
			glDisable( GL_SCISSOR_TEST );
	}
	else
	{
		SRect& rect = mStack.back();
		Vec2i size = rect.getSize();
		glScissor( rect.min.x , rect.min.y , size.x , size.y );
	}
#endif
}

void GUISystem::sendMessage( int event , int id , QWidget* sender )
{
	IGame::Get().procWidgetEvent( event , id , sender );
}

QWidget* GUISystem::findTopWidget( int id , QWidget* start )
{
	for( auto iter = mManager.createTopWidgetIterator(); iter; ++iter )
	{
		if( iter->getID() == id )
			return &*iter;

	}
	return NULL;
}

void GUISystem::render()
{
	mManager.render();
}

void GUISystem::addWidget( QWidget* widget )
{
	mManager.addWidget( widget );
}

void GUISystem::cleanupWidget()
{
	mManager.cleanupWidgets();
}

QWidget::QWidget( Vec2i const& pos , Vec2i const& size , QWidget* parent ) 
	:WidgetCoreT< QWidget >( pos , size , parent )
{
	mId = UI_ANY;
	mClipEnable = false;
	mUserData = 0;
}

QWidget::~QWidget()
{

}

void QWidget::sendEvent( int eventID )
{
	GUI::Core* ui = this;
	while( ui != nullptr )
	{
		if( ui->isTop() )
			break;

		ui = ui->getParent();
		QWidget* widget = static_cast<QWidget*> (ui);
		if( !widget->onChildEvent(eventID, mId, this) )
			return;
	}
	GUISystem::Get().sendMessage( eventID , mId , this );
}

QWidget* QWidget::findChild( int id , QWidget* start )
{

	for( auto child = createChildrenIterator(); child; ++child )
	{
		if( child->mId == id )
			return &(*child);
	}
	return NULL;
}

void QWidget::doRenderAll()
{
	if ( mClipEnable )
	{
		Vec2i pos  = getWorldPos();
		Vec2i size = getSize();
		pos.y = getGame()->getScreenSize().y - ( pos.y + size.y );
		gClipStack.push( pos , size , true );
	}

	WidgetCoreT< QWidget >::doRenderAll();
	QWidget* ui = getChild();

	for( auto child = createChildrenIterator(); child; ++child )
	{
		child->onRenderSiblingsEnd();
	}

	if ( mClipEnable )
	{
		gClipStack.pop();
	}
}


QTextButton::QTextButton( int id , Vec2i const& pos , Vec2i const& size , QWidget* parent ) 
	:BaseClass( id , pos , size , parent )
{
	text.reset( IText::create( getGame()->getFont(0) , 24 , Color4ub( 255 , 255 , 0 ) ) );
}

QTextButton::~QTextButton()
{

}

void QTextButton::onRender()
{
	Vec2i const& pos  = getWorldPos();
	Vec2i const& size = getSize();
#if 1
	auto& g = IGame::Get().getGraphics2D();
	g.beginBlend(1, ESimpleBlendMode::Add);

	if (isEnable())
	{
		if (getButtonState() == BS_HIGHLIGHT)
		{
			g.setBrush(Color3f(0.0, 0.5, 0.0));
			g.setPen(Color3f(0.0, 1.0, 0.0));
		}
		else
		{
			g.setBrush(Color3f(0.0, 0.25, 0.0));
			g.setPen(Color3f(0.0, 0.5, 0.0));
		}
	}
	else
	{
		g.setBrush(Color3f(0.05, 0.05, 0.05));
		g.setPen(Color3f(0.1, 0.1, 0.1));
	}

	g.drawRect(pos, size);
	g.endBlend();

#else
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);
	if( isEnable() )
	{
		if( getButtonState() == BS_HIGHLIGHT )
			glColor3f(0.0, 0.5, 0.0);
		else
			glColor3f(0.0, 0.25, 0.0);
	}
	else
	{
		glColor3f(0.05, 0.05, 0.05);
	}

	drawRect( pos , size );

	if( isEnable() )
	{
		if( getButtonState() == BS_HIGHLIGHT )
			glColor3f(0.0, 1.0, 0.0);
		else
			glColor3f(0.0, 0.5, 0.0);
	}
	else
	{
		glColor3f(0.1, 0.1, 0.1);
	}

	drawRectLine( pos , size );

	glColor3f(1.0, 1.0, 1.0);

	glDisable(GL_BLEND);
#endif

	if( isEnable() )
		text->setColor(Color4ub(50,255,50));
	else
		text->setColor(Color4ub(150,150,150));

	getRenderSystem()->drawText( text , pos  + size / 2  );



}

int const TopSideHeight = 16;
QFrame::QFrame( int id , Vec2i const& pos , Vec2i const& size , QWidget* parent ) 
	:BaseClass( pos , size , parent )
{
	mId = id;
	mTile = NULL;
	mbMiniSize = false;
}

QFrame::~QFrame()
{
	if ( mTile )
		mTile->release();
}

bool QFrame::onMouseMsg( MouseMsg const& msg )
{
	BaseClass::onMouseMsg( msg );

	static int x , y;
	static bool needMove = false;
	if ( msg.onLeftDown() )
	{
		setTop();

		if ( msg.getPos().y > getWorldPos().y &&
			 msg.getPos().y < getWorldPos().y + TopSideHeight )
		{
			x = msg.x();
			y = msg.y();

			getManager()->captureMouse( this );

			needMove = true;
		}
	}
	else if ( msg.onRightDown() )
	{
		if ( msg.getPos().y > getWorldPos().y &&
			 msg.getPos().y < getWorldPos().y + TopSideHeight )
		{ 
			mbMiniSize = !mbMiniSize;
			if ( mbMiniSize )
			{
				mPrevSize = getSize();
				setSize( Vec2i( mPrevSize.x , TopSideHeight ) );
				mClipEnable = true;
			}
			else
			{
				setSize( mPrevSize );
				mClipEnable = false;
			}
		}

	}
	else if ( msg.onLeftUp() )
	{
		needMove = false;
		getManager()->releaseMouse();
	}
	if ( needMove )
	{
		if ( msg.isLeftDown() && msg.isDraging() )
		{
			Vec2i pos = getPos() +Vec2i( msg.x() - x , msg.y() - y );
			setPos( pos );
			x = msg.x();
			y = msg.y();
		}
	}
	return false;
}

void QFrame::onRender()
{
	Vec2i pos = getWorldPos();
	Vec2i size = getSize();

#if 1
	//TIJELO PROZORA
	auto& g = IGame::Get().getGraphics2D();
	g.beginBlend(0.4f);
	g.enablePen(false);
	g.setBrush(Color3f(0.0, 0.25, 0.0));
	g.drawRect(pos, size);
	g.enablePen(true);
	g.endBlend();

#else

	//TIJELO PROZORA
	glEnable(GL_BLEND);
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	glColor4f(0.0, 0.25, 0.0 , 0.4 );
	drawRect( pos , size );
	glDisable(GL_BLEND);

	//NASLOVNICA (TRAKA)
	glBegin(GL_QUADS);
	glColor3f(0.0, 1.0, 0.0); glVertex2f(pos.x, pos.y);
	glColor3f(0.0, 1.0, 0.0); glVertex2f(pos.x+size.x, pos.y);
	glColor3f(0.0, 0.0, 0.0); glVertex2f(pos.x+size.x, pos.y+TopSideHeight);
	glColor3f(0.0, 0.0, 0.0); glVertex2f(pos.x, pos.y+TopSideHeight);
	glEnd();
	glColor3f(1.0, 1.0, 1.0);

#endif

	if ( mTile )
	{
		getRenderSystem()->drawText( mTile , pos + Vec2i( 10 , 0 ) , TEXT_SIDE_LEFT | TEXT_SIDE_TOP );
	}
}

void QFrame::setTile( char const* name )
{
	if ( !mTile )
	{
		mTile = IText::create( getGame()->getFont(0) , 20 , Color4ub( 255 , 255 , 0 ) );
	}
	mTile->setString( name );
}

void QPanel::onRender()
{
	Vec2i pos = getWorldPos();
	Vec2i size = getSize();
	//TIJELO PROZORA
#if 1
	auto& g = IGame::Get().getGraphics2D();
	g.beginBlend(0.8f);
	g.enablePen(false);
	g.setBrush(Color3f(0.0, 0.25, 0.0));
	g.drawRect(pos, size);
	g.enablePen(true);
	g.endBlend();
#else
	glEnable(GL_BLEND);
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	glColor4f( 0.0 , 0.25, 0.0 , 0.8 );
	drawRect( pos , size );
	glDisable(GL_BLEND);
#endif
}

QPanel::QPanel( int id , Vec2i const& pos , Vec2i const& size , QWidget* parent ) 
	:BaseClass( pos , size , parent )
{
	mId = id;
}

QImageButton::QImageButton( int id , Vec2i const& pos , Vec2i const& size , QWidget* parent ) 
	:BaseClass( id , pos , size , parent )
{
	mHelpText = NULL;
}

QImageButton::~QImageButton()
{
	if ( mHelpText )
		mHelpText->release();
}

void QImageButton::onRender()
{
#if 1
	auto& g = IGame::Get().getGraphics2D();

	g.enablePen(false);

	Vec2f pos = getWorldPos();
	Vec2f size = getSize();

	if (getButtonState() == BS_HIGHLIGHT)
		g.setBrush(Color3f(0.25, 0.25, 0.25));
	else
		g.setBrush(Color3f(1.0, 1.0, 1.0));

	g.drawTexture(*texImag->resource, pos, size);
	g.enablePen(true);
#else

	if( getButtonState() == BS_HIGHLIGHT )
		glColor3f(0.25, 0.25, 0.25);
	else
		glColor3f(1.0, 1.0, 1.0);

	Vec2f pos;
	pos.x = getWorldPos().x; 
	pos.y = getWorldPos().y; 
	Vec2f size;
	size.x = getSize().x;
	size.y = getSize().y;

	drawSprite( pos , size ,texImag );
	glColor3f(1.0, 1.0, 1.0);
#endif

}

void QImageButton::setHelpText( char const* str )
{
	if ( !mHelpText )
	{
		mHelpText = IText::create( getGame()->getFont(0) , 24 , Color4ub( 255 , 255 , 255 ) );
	}
	mHelpText->setString( str );
}

void QImageButton::onRenderSiblingsEnd()
{
	if( getButtonState() == BS_HIGHLIGHT && mHelpText )
	{
		Vec2f sizeHelp = mHelpText->getBoundSize() + 2 * Vec2f( 4 , 4 );
		Vec2f posHelp = getGame()->getMousePos();
#if 1
		auto& g = IGame::Get().getGraphics2D();
		g.beginBlend(0.4f);
		g.enablePen(false);
		g.setBrush(Color3f(0.0, 0.25, 0.0));
		g.drawRect(posHelp, sizeHelp);
		g.enablePen(true);
		g.endBlend();

#else
		glEnable(GL_BLEND);
		glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
		glColor4f(0.0, 0.25, 0.0 , 0.4 );
		drawRect( posHelp , sizeHelp );
		glDisable(GL_BLEND);

#endif

		getRenderSystem()->drawText(mHelpText, posHelp + sizeHelp / 2);
	}
}

QTextCtrl::QTextCtrl( int id , Vec2i const& pos , Vec2i const& size , QWidget* parent ) 
	:BaseClass( pos , size , parent )
{
	mId = id;
	text = IText::create();
	mClipEnable = true;
}


QTextCtrl::~QTextCtrl()
{
	text->release();
}

void QTextCtrl::onRender()
{
	Vec2i pos = getWorldPos();
	Vec2i size = getSize();

#if 1
	auto& g = IGame::Get().getGraphics2D();
	g.beginBlend(0.8f);
	g.enablePen(false);
	g.setBrush(Color3f(0.1, 0.1, 0.1));
	g.drawRect(pos, size);
	g.enablePen(true);
	g.endBlend();

#else

	glEnable(GL_BLEND);
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	glColor4f( 0.1, 0.1 , 0.1 , 0.8f );
	drawRect( pos , size );
	glColor3f( 1,1,1);
	glDisable(GL_BLEND);

#endif
	if( isFocus() )
		text->setColor(Color4ub(50,255,50));
	else
		text->setColor(Color4ub(150,150,150));

	getRenderSystem()->drawText( text , pos + size / 2 );

}

void QTextCtrl::setFontSize( unsigned size )
{
	text->setCharSize( size );
}

void QTextCtrl::onModifyValue()
{
	text->setString( getValue() );
}

void QTextCtrl::onEditText()
{
	text->setString( getValue() );
	sendEvent( EVT_TEXTCTRL_VALUE_CHANGED );
}

void QTextCtrl::onFocus( bool beF )
{
	Input::sKeyBlocked = beF;
}

QChoice::QChoice( int id , Vec2i const& pos , Vec2i const& size , QWidget* parent ) 
	:BaseClass(  pos , size , parent )
{
	mId = id;
}

void QChoice::onAddItem( Item& item )
{
	item.text = IText::create( getGame()->getFont( 0 ) , 20 , Color4ub( 150,150,150 ) );
	item.text->setString( item.value.c_str() );
}

void QChoice::onRemoveItem( Item& item )
{
	item.text->release();
}

void QChoice::doRenderItem( Vec2i const& pos , Item& item , bool beLight )
{
	Vec2i size( getSize().x , getMenuItemHeight() );
	if ( beLight )
	{
#if 1
		auto& g = IGame::Get().getGraphics2D();
		g.beginBlend(0.8f);
		g.enablePen(false);
		g.setBrush(Color3f(0.1, 0.1, 0.1));
		g.drawRect(pos + Vec2i(2, 2), size - Vec2i(4, 4));
		g.enablePen(true);
		g.endBlend();
#else
		glEnable(GL_BLEND);
		glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
		glColor4f( 0.1, 0.1 , 0.1 , 0.8f );
		drawRect( pos + Vec2i( 2 , 2 ) , size - Vec2i( 4 , 4 ) );
		glColor3f( 1,1,1);
		glDisable(GL_BLEND);
#endif
	}
	getRenderSystem()->drawText( item.text , pos + size / 2 );
}

void QChoice::onRender()
{
	Vec2i pos = getWorldPos();
	Vec2i size = getSize();

#if 1
	auto& g = IGame::Get().getGraphics2D();
	g.beginBlend(0.8f);
	g.enablePen(false);
	g.setBrush(Color3f(0.1, 0.1, 0.1));
	g.drawRect(pos, size);
	g.enablePen(true);
	g.endBlend();
#else
	glEnable(GL_BLEND);
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	glColor4f( 0.1, 0.1 , 0.1 , 0.8f );
	drawRect( pos , size );
	glColor3f( 1,1,1);
	glDisable(GL_BLEND);
#endif

	if ( mCurSelect != -1 )
	{
		Item& item = mItemList[ mCurSelect ];
		getRenderSystem()->drawText( item.text , pos + size / 2 );
	}

}

void QChoice::doRenderMenuBG( Menu* menu )
{
	Vec2i pos =  menu->getWorldPos();
	Vec2i size = menu->getSize();
#if 1
	auto& g = IGame::Get().getGraphics2D();
	g.beginBlend(0.8f);
	g.enablePen(false);
	g.setBrush(Color3f(0.5, 0.5, 0.5));
	g.drawRect(pos, size);
	g.enablePen(true);
	g.endBlend();
#else
	glEnable(GL_BLEND);
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	glColor4f( 0.5, 0.5 , 0.5 , 0.8f );
	drawRect( pos , size );
	glColor3f( 1,1,1);
	glDisable(GL_BLEND);
#endif
}

QListCtrl::QListCtrl( int id , Vec2i const& pos , Vec2i const& size , QWidget* parent ) 
	:BaseClass(  pos , size , parent )
{
	mId = id;
}


void QListCtrl::onAddItem( Item& item )
{
	item.text = IText::create( getGame()->getFont( 0 ) , 20 , Color4ub( 255 , 255 , 255 ) );
	item.text->setString( item.value.c_str() );
}

void QListCtrl::onRemoveItem( Item& item )
{
	item.text->release();
}

void QListCtrl::doRenderItem( Vec2i const& pos , Item& item , bool beSelected )
{
	Vec2i size( getSize().x , getItemHeight() );
	if ( beSelected )
	{
#if 1
		auto& g = IGame::Get().getGraphics2D();
		g.beginBlend(0.8f);
		g.enablePen(false);
		g.setBrush(Color3f(0.1, 0.1, 0.1));
		g.drawRect(pos + Vec2i(2, 2), size - Vec2i(4, 4));
		g.enablePen(true);
		g.endBlend();
#else
		glEnable(GL_BLEND);
		glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
		glColor4f( 0.1, 0.1 , 0.1 , 0.8f );
		drawRect( pos + Vec2i( 2 , 2 ) , size - Vec2i( 4 , 4 ) );
		glColor3f( 1,1,1);
		glDisable(GL_BLEND);
#endif
	}
	getRenderSystem()->drawText( item.text , pos + size / 2 );
}

void QListCtrl::doRenderBackground( Vec2i const& pos , Vec2i const& size )
{
#if 1
	auto& g = IGame::Get().getGraphics2D();
	g.beginBlend(0.8f);
	g.enablePen(false);
	g.setBrush(Color3f(0.5, 0.5, 0.5));
	g.drawRect(pos, size);
	g.enablePen(true);
	g.endBlend();
#else
	glEnable(GL_BLEND);
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	glColor4f( 0.5, 0.5 , 0.5 , 0.8f );
	drawRect( pos , size );
	glColor3f( 1,1,1);
	glDisable(GL_BLEND);
#endif
}
