#ifndef WidgetCommon_h__
#define WidgetCommon_h__

#include "WidgetCore.h"
#include "DataStructure/Array.h"

#include <algorithm>
#include <type_traits>


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


template < class T >
struct TUIValueTraits
{
    static char const* GetString( T const& val ) { return val.c_str(); }
};

template <>
struct TUIValueTraits< char const* >
{
	static char const* GetString(char const* val) { return val; }
};

template < class Impl, class CoreImpl, class T = std::string >
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

	template < typename Q, typename = typename std::enable_if< !std::is_integral<Q>::value >::type >
	unsigned addItem( Q const& val );
	template < typename Q, typename = typename std::enable_if< !std::is_integral<Q>::value >::type >
	void     removeItem( Q const& val );
	void     removeItem( int pos );
	void     removeAllItem();

	size_t   getItemNum() const { return mItemList.size(); }
	void*    getSelectedItemData()
	{ 
		if ( mCurSelect != INDEX_NONE )
			return getItemData( mCurSelect );
		return nullptr;
	}

	int      getSelection() const { return mCurSelect; }
	void     setSelection( int select )
	{ 
		assert( mItemList.empty() || (select < (int)mItemList.size() && select >= INDEX_NONE) );
		mCurSelect = select; 
	}

	template < typename Q, typename = typename std::enable_if< !std::is_integral<Q>::value >::type >
	int     findItem( Q const& value ) const;

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

	void    insertValue( unsigned pos , T const& val )
	{
		mItemList.insert( mItemList.begin() + pos , Item(val) );
		_this()->onAddItem( mItemList[pos] );
	}

	char const* getSelectValue() const
	{ 
		if ( mCurSelect == INDEX_NONE)
			return nullptr;
		return TUIValueTraits<T>::GetString( mItemList[ mCurSelect ].value );
	}

	T const&    getSelectData() const
	{
		static T StaticValue = T();
		if ( mCurSelect == INDEX_NONE )
			return StaticValue;
		return mItemList[ mCurSelect ].value;
	}

	void   modifySelection( unsigned pos )
	{
		setSelection( pos );
		_this()->onItemSelect( pos );
	}


protected:

	struct Item : public Impl::ExtraData
	{
		Item( T const& val )
			:value( val ),userData( 0 ){}
		template < typename Q >
		Item( Q const& val )
			:value( val ),userData( 0 ){}
		T value;
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



template < class Impl, class CoreImpl, class T = std::string >
class WChoiceT : public TItemOwnerUI< Impl , CoreImpl, T >
{
private:
	Impl* _this(){ return static_cast< Impl*>(this); }

public:

	WChoiceT( Vec2i const& pos , Vec2i const& size, CoreImpl* parent )
		:TItemOwnerUI< Impl , CoreImpl, T >( pos , size , parent )
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


template < class Impl, class CoreImpl, class T = std::string >
class WListCtrlT : public TItemOwnerUI< Impl , CoreImpl , T >
{
private:
	Impl* _this(){ return static_cast< Impl*>(this); }
public:
	WListCtrlT( Vec2i const& pos , Vec2i const& size, CoreImpl* parent )
		:TItemOwnerUI< Impl , CoreImpl, T >( pos , size , parent )
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

template < typename CoreImpl >
class TWidgetLibrary
{
public:
	using Widget = CoreImpl;
	using Core = WidgetCoreT< CoreImpl >;

#define DEFINE_UI_CLASS( Class , BaseClass )\
	template< class Impl >\
	using Class = BaseClass< Impl , CoreImpl >;

	DEFINE_UI_CLASS(SliderT, WSliderT)
	DEFINE_UI_CLASS(PanelT, WPanelT)
	DEFINE_UI_CLASS(ButtonT, WButtonT)
	DEFINE_UI_CLASS(TextCtrlT, WTextCtrlT)
	
	template< class Impl , class T = std::string >
	using ChoiceT = WChoiceT< Impl , CoreImpl , T >;
	template< class Impl , class T = std::string >
	using ListCtrlT = WListCtrlT< Impl , CoreImpl , T >;

	DEFINE_UI_CLASS(NoteBookT, WNoteBookT)

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
