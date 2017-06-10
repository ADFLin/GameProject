#ifndef TUICommon_hpp__
#define TUICommon_hpp__

#include "WidgetCommon.h"
#include "PlatformConfig.h"

#ifdef SYS_PLATFORM_WIN
#include "Win32Header.h"
#endif

template < class Impl ,class CoreImpl >
bool  WButtonT< Impl , CoreImpl >::onMouseMsg( MouseMsg const& msg )
{
	CoreImpl::onMouseMsg( msg );

	if ( msg.onLeftUp() && isFocus() )
	{
		_this()->onClick();
	}

	if (  msg.onLeftDown() )
	{
		setButtonState( BS_PRESS );
	}
	else if ( msg.onLeftUp() )
	{
		setButtonState( BS_HIGHLIGHT );
	}
	return false;
}

template < class Impl ,class CoreImpl >
void WButtonT< Impl , CoreImpl >::mouseOverlap( bool beInside )
{
	CoreImpl::mouseOverlap( beInside );

	if ( beInside )
		setButtonState( BS_HIGHLIGHT );
	else
		setButtonState( BS_NORMAL );
}

template < class Impl ,class CoreImpl >
void WButtonT< Impl , CoreImpl >::setButtonState( ButtonState state )
{
	if ( mState == state )
		return;
	_this()->onChangeState( state );
	mState = state;
}

template < class Impl, class CoreImpl >
unsigned TItemOwnerUI<Impl, CoreImpl>::appendItem( char const* str )
{ 
	mItemList.push_back( Item( str ) );
	unsigned pos = unsigned( mItemList.size() - 1 );
	_this()->onAddItem( mItemList.back() );
	return pos; 
}

template < class Impl, class CoreImpl >
void  TItemOwnerUI<Impl, CoreImpl>::removeItem( unsigned pos )
{
	if ( pos >= mItemList.size() )
		return;

	ItemVec::iterator iter = mItemList.begin() + pos;

	_this()->onRemoveItem( *iter );
	mItemList.erase( iter );
	if ( mCurSelect == (int)pos )
		mCurSelect = -1;
	else if ( mCurSelect > (int)pos )
		--mCurSelect;
}


template < class Impl, class CoreImpl >
void  TItemOwnerUI<Impl, CoreImpl>::removeAllItem()
{
	for( int i = 0 ; i < (int)mItemList.size() ; ++i )
	{
		_this()->onRemoveItem( mItemList[i] );
	}
	mItemList.clear();
	mCurSelect = -1;
}

template < class Impl, class CoreImpl >
void TItemOwnerUI<Impl, CoreImpl>::removeItem( char const* str )
{
	struct FindValue
	{
		FindValue( char const* str ):str( str ){}
		bool operator()( Item const& item ) const { return item.value == str;  }
		char const* str;
	};

	assert( str );
	ItemVec::iterator iter = std::find_if( 
		mItemList.begin() , mItemList.end() , FindValue( str ) );

	if ( iter != mItemList.end() )
	{
		if ( mCurSelect != -1 && iter == mItemList.begin() + mCurSelect )
			mCurSelect = -1;
		_this()->onRemoveItem( *iter );
		mItemList.erase( iter );
	}
}

template < class Impl, class CoreImpl >
void TItemOwnerUI<Impl, CoreImpl>::tryMoveSelect( bool beNext )
{
	if ( mCurSelect == -1 )
	{
		if ( mItemList.size() )
		{
			mCurSelect = ( beNext ) ? 0 : (int)mItemList.size() - 1;
			_this()->onItemSelect( mCurSelect );
		}
		return;
	}

	if ( beNext )
	{
		if ( mCurSelect + 1 >= (int)mItemList.size() )
			return;
		mCurSelect += 1;
	}
	else
	{
		if ( mCurSelect - 1 < 0 )
			return;
		mCurSelect -= 1;
	}
	_this()->onItemSelect( mCurSelect );
}

template < class Impl, class CoreImpl >
bool TItemOwnerUI<Impl, CoreImpl>::onKeyMsg( unsigned key , bool isDown )
{
	if ( !isDown )
		return true;

	switch( key )
	{
	case Keyboard::eUP: 
		tryMoveSelect( false );
		return false;
	case Keyboard::eDOWN:
		tryMoveSelect( true );
		return false;
	}
	return true;
}

template < class Impl, class CoreImpl >
bool WChoiceT<Impl, CoreImpl>::onMouseMsg( MouseMsg const& msg )
{
	CoreImpl::onMouseMsg( msg );

	if ( msg.onDown() )
	{
		Vec2i pos = getWorldPos();
		Vec2i size = getSize();
		pos.y += getSize().y;

		if ( mItemList.size() )
			size.y = _this()->getMenuItemHeight() * int( mItemList.size() );
		else
			size.y = _this()->getMenuItemHeight();

		mLightSelect = mCurSelect;

		Menu* menu = new Menu( pos , size , this );
		getManager()->addWidget( menu );
		//_addFlag( UF_HITTEST_CHILDREN );
		menu->setTop();
		menu->makeFocus();
	}
	return false;
}

template < class Impl, class CoreImpl >
bool WChoiceT<Impl, CoreImpl>::_onMenuMouseMsg( Menu* menu , MouseMsg const& msg )
{
	Vec2i menuSize = menu->getSize();
	int posSelect = ( msg.getPos().y - menu->getWorldPos().y ) / _this()->getMenuItemHeight();
	if ( posSelect < (int)mItemList.size() )
	{
		mLightSelect = posSelect;
		if ( msg.onLeftUp() )
		{
			if ( mItemList.size() )
			{
				if ( mCurSelect != mLightSelect )
				{
					mCurSelect = mLightSelect;
					_this()->onItemSelect( mCurSelect );
				}
			}
			destroyMenu( menu );
		}
	}
	return false;
}

template < class Impl, class CoreImpl >
void WChoiceT<Impl, CoreImpl>::_onRenderMenu( Menu* menu )
{
	_this()->doRenderMenuBG( menu );
	Vec2i pos = menu->getWorldPos();
	for( int i = 0 ; i < (int)mItemList.size() ; ++i )
	{
		_this()->doRenderItem( pos , mItemList[i] , mLightSelect == i );
		pos.y += _this()->getMenuItemHeight();
	}
}

template < class Impl , class CoreImpl >
bool WTextCtrlT<Impl, CoreImpl>::isDoubleChar( int pos )
{
	if ( pos < 0 )
		return false;
	char const* c = mValue.c_str();

	int p = 0;
	int offset = 1;
	while ( p <= pos )
	{
		offset = ( *c & 0x80 ) ? 2 : 1;

		c += offset;
		p += offset;
	}
	return offset == 2;
}

template < class Impl , class CoreImpl >
bool WTextCtrlT<Impl, CoreImpl>::onKeyMsg( unsigned key , bool isDown )
{
	if ( isDown )
	{
		switch( key )
		{
		case Keyboard::eBACK:
			if ( mKeyInPos > 0 && mValue.size() > 0 )
			{
				int offset = isDoubleChar( mKeyInPos - 1 ) ? 2 : 1;
				mValue.erase( mKeyInPos - offset , offset );
				mKeyInPos -= offset;
				_this()->onEditText();
			}
			break;
		case Keyboard::eRIGHT:
			mKeyInPos += isDoubleChar( mKeyInPos ) ? 2 : 1;
			if ( mKeyInPos > int( mValue.size() ) )
				mKeyInPos = int( mValue.size() );
			break;
		case Keyboard::eLEFT:
			mKeyInPos -= isDoubleChar( mKeyInPos - 1 ) ? 2 : 1;
			if ( mKeyInPos < 0 )
				mKeyInPos = 0;
			break;
		case Keyboard::eRETURN:
			_this()->onPressEnter();
			getManager()->focusWidget( NULL );
			break;

		case Keyboard::eV:
#ifdef SYS_PLATFORM_WIN
			if ( ::GetKeyState( VK_CONTROL ) < 0 )
			{
				if ( ::OpenClipboard( NULL ) )
				{
					HANDLE hData = ::GetClipboardData( CF_TEXT );
					struct Locker 
					{
						Locker( HANDLE  hData ) : hData( hData ){ ::GlobalLock( hData ); }
						~Locker(){ ::GlobalUnlock( hData ); }
						HANDLE hData;
					};
					if ( hData )
					{
						char* str = ( char* )::GlobalLock( hData );
						if ( str )
						{
							int len = (int)strlen( str );
							mValue.insert( mValue.begin() + mKeyInPos , str , str + len );
							mKeyInPos += len;

							_this()->onEditText();
						}
						::GlobalUnlock( hData );
					}
					::CloseClipboard();
				}
			}
#endif //SYS_PLATFORM_WIN
			break;

		}
	}
	return false;
}

template < class Impl , class CoreImpl >
bool WTextCtrlT<Impl, CoreImpl>::onCharMsg( unsigned code )
{
	if ( ( code & 0x80 ) || isprint( code ) )
	{
		mValue.insert( mKeyInPos , 1 , char( code ) );
		++mKeyInPos;
		_this()->onEditText();
	}
	return false;
}

template < class Impl , class CoreImpl >
void WSliderT<Impl, CoreImpl >::updateTipPos()
{
	Vec2i  const& size = mTipWidget->getSize();
	float ratio = float( mCurValue - mMinValue) / ( mMaxValue - mMinValue);
	if ( mbHorizontal )
		mTipWidget->setPos( Vec2i( int( ratio * ( this->getSize().x - size.x ) ) , 0 ) );
	else
		mTipWidget->setPos( Vec2i( 0 , int ( ratio *( this->getSize().y - size.y ) ) ) );
}

template < class Impl , class CoreImpl >
void WSliderT<Impl, CoreImpl >::correctTipPos( Vec2i& pos )
{
	Vec2i  const& size = mTipWidget->getSize();
	if ( mbHorizontal )
	{
		pos.y = 0;
		pos.x = std::max( 0 , std::min( pos.x , this->getSize().x - size.x) );
	}
	else
	{
		pos.x = 0;
		pos.y = std::max( 0 , std::min( pos.y , this->getSize().y - size.y ) );
	}
}

template < class Impl , class CoreImpl >
void WSliderT<Impl, CoreImpl>::updateValue()
{
	Vec2i const& pos = mTipWidget->getPos();
	Vec2i const& size = mTipWidget->getSize();

	if ( mbHorizontal )
		mCurValue =  mMinValue + ( mMaxValue - mMinValue) * pos.x / ( this->getSize().x - size.x ) ;
	else
		mCurValue =  mMinValue + ( mMaxValue - mMinValue) * pos.y / ( this->getSize().y - size.y );

	_this()->onScrollChange( mCurValue );
}

template < class Impl , class CoreImpl >
bool WSliderT<Impl, CoreImpl>::TipWidget::onMouseMsg( MouseMsg const& msg )
{
	static int x , y;

	CoreImpl::onMouseMsg( msg );

	if ( msg.onLeftDown() )
	{
		getManager()->captureMouse( this );
		x = msg.x();
		y = msg.y();
	}
	else if ( msg.onLeftUp() )
	{
		getManager()->releaseMouse();
	}
	if ( msg.isLeftDown() && msg.isDraging() )
	{
		Vec2i pos = getPos() + Vec2i(msg.x() - x , msg.y() - y);
		getSlider()->correctTipPos( pos );
		setPos( pos );
		getSlider()->updateValue();

		x = msg.x();
		y = msg.y();
	}
	return false;
}



template < class Impl , class CoreImpl  >
WNoteBookT<Impl, CoreImpl>::WNoteBookT( Vec2i const& pos , Vec2i const& size , CoreImpl* parent ) 
	:CoreImpl(  pos , size , parent )
{
	mCurPageButton = NULL;

	mPageSize = size;
	mPageSize.y -= _this()->getButtonSize().y + 2;
}


template < class Impl , class CoreImpl  >
void WNoteBookT<Impl, CoreImpl>::updatePageButtonPos()
{
	int pid = 0;
	for( ButtonVec::iterator iter = mPageButtons.begin();
		iter != mPageButtons.end() ; ++iter )
	{
		PageButton* button = *iter;
		button->setPos( _this()->calcPageButtonPos( pid ) );
		++pid;
	}
}


template < class Impl , class CoreImpl  >
void WNoteBookT< Impl , CoreImpl >::render()
{
	CoreImpl::render();

	_this()->doRenderBackground();
	if ( mCurPageButton )
		_this()->doRenderPage( getCurPage() );
}

template < class Impl , class CoreImpl  >
void WNoteBookT<Impl, CoreImpl >::changePage( int pID )
{
	if ( mCurPageButton )
	{
		if ( mCurPageButton->pID == pID )
			return;

		mCurPageButton->page->_unlinkInternal(true);
	}
	PageButton* pb = mPageButtons[ pID ];
	addChild( pb->page );

	mCurPageButton = pb;
}




template < class Impl , class CoreImpl  >
typename WNoteBookT<Impl, CoreImpl>::Page* 
WNoteBookT<Impl, CoreImpl >::addPage( char const* title )
{
	Page* page = new Page( _this()->getPagePos() , mPageSize , NULL );

	int pID = (int)mPageButtons.size();

	Vec2i pos = _this()->calcPageButtonPos( pID );

	PageButton* button = new PageButton( pID , page , pos , _this()->getButtonSize() , this );
	button->title = title;

	mPageButtons.push_back( button );

	if ( mCurPageButton == NULL )
		_this()->changePage( pID );

	return page;
}

template < class Impl , class CoreImpl  >
void WListCtrlT< Impl , CoreImpl >::onRender()
{
	Vec2i pos = getWorldPos();
	Vec2i size = getSize();
	_this()->doRenderBackground( pos , size );
	int numLineShow = size.y / _this()->getItemHeight();
	for( int i = 0 ; i < numLineShow ; ++i )
	{
		int posItem = i + mIndexShowStart;
		if ( posItem >= (int)mItemList.size() )
			break;
		_this()->doRenderItem( pos , mItemList[posItem] , posItem == mCurSelect );
		pos.y += _this()->getItemHeight();
	}
}

template < class Impl , class CoreImpl  >
bool WListCtrlT< Impl , CoreImpl >::onMouseMsg( MouseMsg const& msg )
{
	unsigned posItem = mIndexShowStart + ( msg.getPos().y - getWorldPos().y ) / _this()->getItemHeight(); 
	if ( posItem < mItemList.size() )
	{
		if ( msg.onLeftDown() )
		{
			mCurSelect = posItem;
			_this()->onItemSelect( posItem );
		}
		else if ( msg.onLeftDClick() )
		{
			mCurSelect = posItem;
			_this()->onItemLDClick( posItem );
		}
	}
	return false;
}


template < class Impl , class CoreImpl  >
void WListCtrlT< Impl , CoreImpl >::ensureVisible( unsigned pos )
{
	int diff = int(pos) - mIndexShowStart;
	if ( diff < 0 )
	{
		mIndexShowStart = pos;
	}
	else
	{
		int numItemShow = getSize().y / _this()->getItemHeight();
		if ( numItemShow <= diff )
		{
			mIndexShowStart = std::min( pos - numItemShow , mItemList.size() - 1 );
		}
	}

}

#endif // TUICommon_hpp__