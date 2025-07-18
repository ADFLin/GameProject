#ifndef WidgetCommon_h__
#define WidgetCommon_h__

#include "WidgetCore.h"
#include "DataStructure/Array.h"

#include <algorithm>


enum ButtonState
{
	BS_NORMAL,
	BS_PRESS,
	BS_HIGHLIGHT ,
	BS_NUM_STATE ,
};

template < class Impl , class CoreImpl >
class WButtonT : public CoreImpl
{
private:
	Impl* _this(){ return static_cast< Impl*>(this); }

public:
	WButtonT( Vec2i const& pos , Vec2i const& size  , CoreImpl* parent )
		:CoreImpl(  pos , size , parent )
		,mState( BS_NORMAL ){}

	ButtonState getButtonState() const { return mState; }
	///////// override function ////////
public:
	void onClick(){}
	void onChangeState( ButtonState state ){}
	///////////////////////////////////
protected:
	void mouseOverlap( bool beInside );
	MsgReply onMouseMsg( MouseMsg const& msg );

	void setButtonState( ButtonState state );
	ButtonState mState;
};


template < class Impl , class CoreImpl >
class WPanelT : public CoreImpl
{
	Impl* _this(){ return static_cast< Impl*>(this); }
public:
	WPanelT( Vec2i const& pos , Vec2i const& size  , CoreImpl* parent )
		:CoreImpl(  pos , size , parent ){}

};


enum UISide
{
	SIDE_TOP ,
	SIDE_BOTTON ,
	SIDE_LEFT ,
	SIDE_RIGHT ,
};

template < class Impl , class CoreImpl >
class WNoteBookT : public CoreImpl
{
	Impl* _this(){ return static_cast< Impl*>(this); }

public:
	WNoteBookT( Vec2i const& pos , Vec2i const& size  , CoreImpl* parent );

	struct NullData {};

	class Page : public WPanelT< Page , CoreImpl >
	{
	public:
		Page( Vec2i const& pos , Vec2i const& size  , CoreImpl* parent )
			:WPanelT< Page , CoreImpl >( pos , size , parent ){}
	};

	class PageButton  : public WButtonT< PageButton , CoreImpl >
	{
	public:
		PageButton( int _pID , Page* _page, Vec2i const& pos , Vec2i const& size , CoreImpl* parent )
			:WButtonT< PageButton , CoreImpl >( pos , size , parent )
		{
			pID  = _pID;
			page = _page;
		}

		WNoteBookT* getBook(){ return static_cast< WNoteBookT* >( getParent() ); }
		void onClick(){  getBook()->changePage( pID );  }
		void render() {  getBook()->_this()->doRenderButton( this ); }

		std::string title;
		int   pID;
		Page* page;
	};
	///////// override function ////////
	void doRenderButton( PageButton* button ){}
	void doRenderPage( Page* page ){}
	void doRenderBackground(){}

	Vec2i getButtonSize(){ return Vec2i(0,0); }
	Vec2i getButtonOffset(){ return Vec2i(0,0); }
	Vec2i getPagePos(){ return Vec2i(0,0); }
	Vec2i getBaseButtonPos(){ return Vec2i(0,0); }

	void onChangePage( int pID ){}

	Vec2i  calcPageButtonPos( int pID )
	{
		return _this()->getBaseButtonPos() + pID * _this()->getButtonOffset();
	}
	////////////////////////////////////////////////////////////////

	void   changePage( int pID );
	Page*  addPage( char const* title );
	void   updatePageButtonPos();

	Vec2i const& getPageSize(){ return mPageSize; }
	Page*  getCurPage()        { return mCurPageButton->page; }

protected:
	//virtual 
	void   render();

	Vec2i       mPageSize;
	PageButton* mCurPageButton;
	typedef TArray< PageButton* > ButtonVec;
	ButtonVec   mPageButtons;
};

template < class Impl , class CoreImpl >
class WSliderT : public CoreImpl
{
	Impl* _this(){ return static_cast< Impl*>(this); }

public:
	WSliderT( Vec2i const& pos , int length , Vec2i const& sizeTip  ,
		bool beH  , int minRange , int maxRange , CoreImpl* parent )
		:CoreImpl(  pos , ( beH ) ? Vec2i( length , sizeTip.y ) : Vec2i( sizeTip.x , length ) , parent )
		,mMinValue( minRange )
		,mMaxValue( maxRange )
		,mbHorizontal( beH )
	{
		mTipWidget = new TipWidget( sizeTip , this );
	}

	class TipWidget : public CoreImpl
	{
	public:
		TipWidget( Vec2i const& size , CoreImpl* parent )
			:CoreImpl( Vec2i(0,0) , size , parent ){}
		MsgReply onMouseMsg( MouseMsg const& msg );
		Impl* getSlider(){ return  static_cast<Impl*>( getParent() ); }
	};


	///////// override function ////////
	void onScrollChange( int value ){}
	void doRenderTip( TipWidget* ui ){}
	void doRenderBackground(){}

	void onDragTipStart(){}
	void onDragTipEnd(){}
	/////////////////////////////////////////

	void  updateValue();

	void  correctTipPos( Vec2i& pos );

	int   getValue() const { return mCurValue; }
	int   getMaxValue() const { return mMaxValue; }
	int   getMinValue() const { return mMinValue; }

	void  setValue( int val )
	{
		mCurValue = std::max( mMinValue , std::min( val , mMaxValue ) );
		updateTipPos();
	}

	void setRange( int min , int max )
	{
		mMinValue = min;
		mMaxValue = max;
	}

	TipWidget* getTipWidget() { return mTipWidget; }

protected:
	void onRender()
	{
		_this()->doRenderBackground();
		_this()->doRenderTip( mTipWidget );
	}

	void  updateTipPos();

	int    mCurValue;
	int    mMinValue;
	int    mMaxValue;
	TipWidget* mTipWidget;
	bool   mbHorizontal;
};

template < class Impl , class CoreImpl >
class WTextCtrlT : public CoreImpl
{
private:
	Impl* _this(){ return static_cast< Impl*>(this); }

public:
	WTextCtrlT( Vec2i const& pos , Vec2i const& size  , CoreImpl* parent )
		:CoreImpl(  pos , size , parent ){ mKeyInPos = 0; }

	char const* getValue() const { return mValue.c_str(); }
	void clearValue()
	{ 
		mValue.clear(); 
		mKeyInPos = 0; 
		_this()->onModifyValue();
	}
	void setValue( char const* str )
	{ 
		mValue = str; 
		mKeyInPos = (int)mValue.size();
		_this()->onModifyValue();
	}

	void appendValue(char const* str)
	{
		mValue.append(str);
		mKeyInPos = (int)mValue.size();
		_this()->onModifyValue();
	}
	///////// override function ////////
public:
	void onPressEnter(){}
	void onEditText(){}
	void onModifyValue(){}
	/////////////////////////////////////

public:
	//virtual 
	MsgReply onKeyMsg(KeyMsg const& msg);
	//virtual 
	MsgReply onCharMsg(unsigned code);

protected:

	bool isDoubleChar( int pos );


	int mKeyInPos;
	std::string mValue;

};


template < class Impl, class CoreImpl >
class TItemOwnerUI : public CoreImpl
{
private:
	Impl* _this(){ return static_cast< Impl*>(this); }

	struct EmptyData {};
	typedef EmptyData ExtraData;
public:

	TItemOwnerUI( Vec2i const& pos , Vec2i const& size, CoreImpl* parent )
		:CoreImpl( pos , size , parent )
	{
		mCurSelect = INDEX_NONE;
	}

	~TItemOwnerUI()
	{
		removeAllItem();
	}

	unsigned addItem( char const* str );
	void     removeItem( char const* str );
	void     removeItem( unsigned pos );
	void     removeAllItem();

	size_t   getItemNum(){ return mItemList.size(); }
	void*    getSelectedItemData()
	{ 
		if ( mCurSelect != INDEX_NONE )
			return getItemData( mCurSelect );
		return nullptr;
	}

	int      getSelection() const { return mCurSelect; }
	void     setSelection( unsigned select )
	{ 
		assert( select < mItemList.size() );
		mCurSelect = select; 
	}

	int     findItem( char const* value ) const
	{
		for( int i = 0 ; i < (int)mItemList.size() ; ++i )
		{
			if ( mItemList[i].value == value )
				return i;
		}
		return INDEX_NONE;
	}

	void    setItemData( unsigned pos , void* data )
	{
		if ( pos >= mItemList.size() )
			return;
		mItemList[pos].userData = data;
	}
	void*  getItemData( unsigned pos ) const
	{
		return mItemList[pos].userData;
	}

	void    insertValue( unsigned pos , char const* str )
	{
		mItemList.insert( mItemList.begin() + pos , Item(str) );
		_this()->onAddItem( mItemList[pos] );
	}

	char const* getSelectValue() const
	{ 
		if ( mCurSelect == INDEX_NONE)
			return nullptr;
		return mItemList[ mCurSelect ].value.c_str();
	}

	void   modifySelection( unsigned pos )
	{
		setSelection( pos );
		_this()->onItemSelect( pos );
	}


protected:

	struct Item : public Impl::ExtraData
	{
		Item( char const* val )
			:value( val ),userData( 0 ){}
		std::string value;
		void*       userData;
	};

	///////// override function ////////
	void onItemSelect( unsigned select ){}
	void onAddItem( Item& item ){}
	void onRemoveItem( Item& item ){}
protected:
	void tryMoveSelect( bool beNext );
	MsgReply onKeyMsg(KeyMsg const& msg);


	using ItemVec = TArray< Item >;
	ItemVec    mItemList;
	int        mCurSelect;
};



template < class Impl, class CoreImpl >
class WChoiceT : public TItemOwnerUI< Impl , CoreImpl >
{
private:
	Impl* _this(){ return static_cast< Impl*>(this); }

public:

	WChoiceT( Vec2i const& pos , Vec2i const& size, CoreImpl* parent )
		:TItemOwnerUI< Impl , CoreImpl >( pos , size , parent )
	{
		mLightSelect = INDEX_NONE;
	}


	class Menu : public CoreImpl
	{
	public:
		Menu( Vec2i const& pos , Vec2i const& size, WChoiceT* owner )
			:CoreImpl( pos , size , NULL )
			,mOwner( owner )
		{

		}

		~Menu()
		{

		}

		virtual void focus( bool beF )
		{
			CoreImpl::focus( beF ); 
			if (!beF && getOwner())
			{
				getOwner()->destroyMenu();
			}
		}
		WChoiceT* getOwner(){ return mOwner; }
		virtual void render(){  getOwner()->notifyRenderMenu( this );  }
		virtual MsgReply onMouseMsg( MouseMsg const& msg ){  return getOwner()->notifyMenuMouseMsg( this , msg ); }

		WChoiceT* mOwner;
	};

	///////// override function ////////
	void onItemSelect( unsigned select ){}
	void doRenderItem( Vec2i const& pos , Item& item , bool bSelected){}
	void doRenderMenuBG( Menu* menu ){}
	int  getMenuItemHeight(){ return 15; }
	/////////////////////////////////////////////////////////////////

	MsgReply onMouseMsg( MouseMsg const& msg );
protected:
	void destroyMenu();
	void notifyRenderMenu( Menu* menu );
	MsgReply notifyMenuMouseMsg( Menu* menu , MouseMsg const& msg );

	int        mLightSelect;
	Menu*      mMenu = nullptr;
};


template < class Impl, class CoreImpl >
class WListCtrlT : public TItemOwnerUI< Impl , CoreImpl >
{
private:
	Impl* _this(){ return static_cast< Impl*>(this); }
public:
	WListCtrlT( Vec2i const& pos , Vec2i const& size, CoreImpl* parent )
		:TItemOwnerUI< Impl , CoreImpl >( pos , size , parent )
	{
		mIndexShowStart = 0;
	}

	void  ensureVisible( unsigned pos );

public:
	///////// override function ////////
	void onItemSelect( unsigned pos ){}
	void onItemLDClick( unsigned pos ){}
	void doRenderItem( Vec2i const& pos , Item& item , bool bSelected){}
	void doRenderBackground( Vec2i const& pos , Vec2i const& size ){}
	int  getItemHeight(){ return 15; }
	/////////////////////////////////////
protected:
	void onRender();
	MsgReply onMouseMsg( MouseMsg const& msg );

	int  mIndexShowStart;
};

template < class CoreImpl >
class TWidgetLibrary
{
public:
	using Widget = CoreImpl;
	using Core = WidgetCoreT< CoreImpl >;
	using Manager = TWidgetManager< CoreImpl >;  

#define DEFINE_UI_CLASS( Class , BaseClass )\
	template< class Impl >\
	using Class = BaseClass< Impl , CoreImpl >;

	DEFINE_UI_CLASS(SliderT, WSliderT)
	DEFINE_UI_CLASS(PanelT, WPanelT)
	DEFINE_UI_CLASS(ButtonT, WButtonT)
	DEFINE_UI_CLASS(TextCtrlT, WTextCtrlT)
	DEFINE_UI_CLASS(ChoiceT, WChoiceT)
	DEFINE_UI_CLASS(NoteBookT, WNoteBookT)
	DEFINE_UI_CLASS(ListCtrlT, WListCtrlT)

#undef DEFINE_UI_CLASS

	template < class T >
	static T* CastFast( CoreImpl* ui )
	{
#if _DEBUG
		return dynamic_cast< T* >( ui );
#else
		return static_cast< T* >( ui );
#endif
	}


};

#include "WidgetCommon.hpp"


#endif // WidgetCommon_h__
