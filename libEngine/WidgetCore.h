#ifndef WidgetCore_h__
#define WidgetCore_h__

#include <cstdlib>
#include <cassert>
#include <vector>

#include "SystemMessage.h"
#include "Rect.h"

#define UI_CORE_USE_INTRLIST 1

#if UI_CORE_USE_INTRLIST
#include "IntrList.h"
#include "StdUtility.h"
#endif

//#include "ProfileSystem.h"
#define  PROFILE_ENTRY(...)


#ifndef BIT
#define BIT( n ) ( 1 << ( n ) )
#endif 

#define UI_NO_PARENT (void*)(~0)



enum WidgetInternalFlag
{
	WIF_STAY_TOP            = BIT(1) ,
	WIF_WORLD_POS_     = BIT(2) ,
	WIF_DISABLE             = BIT(3) ,
	WIF_BE_HIDDEN           = BIT(4) ,
	WIF_HITTEST_CHILDREN    = BIT(5) ,
	WIF_BLOCK_DESTROY       = BIT(6) ,
	WIF_PARENT_MOUSE_EVENT  = BIT(7) ,
	WIF_MARK_DESTROY        = BIT(8) ,
	WIF_EFFECT_DISABLE_CHILD= BIT(9) ,
	WIF_MANAGER_REF         = BIT(10),
};

#ifdef max
	#undef max
#endif
#ifdef min
	#undef min
#endif

template< class T >
class TWidgetManager;

class WidgetCoreBase
{
public:
	virtual ~WidgetCoreBase(){}
#if UI_CORE_USE_INTRLIST
	HookNode       mLinkHook;
#endif
};

template< class T >
class WidgetCoreT : public WidgetCoreBase
{
	T* _this(){ return static_cast< T* >( this );  }
	typedef TRect< int > Rect;
public:
	WidgetCoreT( Vec2i const& pos , Vec2i const& size , T* parent );
	virtual ~WidgetCoreT();

#if UI_CORE_USE_INTRLIST
	auto           createChildrenIterator() { return MakeIterator( mChildren ); }
	T*             getChild() { return ( mChildren.empty() ) ? nullptr : &mChildren.front(); }
#else
	class Iterator
	{
	public:
		Iterator(WidgetCoreT* c) :cur(c){}
		operator bool()  const { return !!cur; }
		T& operator *() { return *static_cast< T*>( cur ); }
		T* operator->() { return static_cast< T*>( cur ); }
		Iterator& operator ++() { cur = cur->mNext; return *this; }
		Iterator  operator ++(int) { Iterator temp = *this;  cur = cur->mNext; return temp; }
	private:
		WidgetCoreT* cur;
	};

	auto           createChildrenIterator() { return Iterator(mChild);  }
	T*             getChild(){ return static_cast< T*>( mChild );  }
#endif
	
	WidgetCoreT*       getParent(){ return mParent; }
	TWidgetManager<T>* getManager(){ return mManager; }
	int            getChildrenNum(){ return mNumChild; }
	int            getLevel();
	int            getOrder();

	Vec2i const&  getWorldPos();
	Vec2i const&  getPos() const { return mBoundRect.min; }
	Vec2i         getSize() const { return mBoundRect.getSize(); }
	Rect const&   getBoundRect() const { return mBoundRect; }

	bool           isFocus();
	bool           isEnable() const { return !checkFlag( WIF_DISABLE ); }
	bool           isShow() const { return !checkFlag( WIF_BE_HIDDEN ); }
	bool           isTop();

	T&             setPos( Vec2i const& pos );
	T&             setSize( Vec2i const& size ){  mBoundRect.max = mBoundRect.min + size;  return *_this(); }
	T&             setTop( bool beAlways = false );
	T&             makeFocus();
	T&             show( bool beS );
	T&             enable( bool beE = true );

	T&             addChild( WidgetCoreT* ui );
	void           destroy();

public:
	void          _unlinkInternal(bool bRemove);
private:
	void          linkChildInternal( WidgetCoreT* ui );

protected:
	void    onLink(){}
	void    onUnlink(){}
	void    onChangeOrder(){}
	void    onChangeChildrenOrder(){}
	void    onEnable( bool beE ){}
	void    onUpdateUI(){}
	void    onChangePos( Vec2i const& pos , bool beLocal ){}
	void    onShow( bool beS ){}
	void    onMouse( bool beIn ){}

	void    onFocus( bool beF ){}
	void    onResize( Vec2i const& size ){}
	bool    doHitTest( Vec2i const& pos ){ return mBoundRect.hitTest( pos ); }

	void    doRenderAll();
	void    onRender(){}
	void    onPrevRender(){}
	void    onPostRender(){}
	void    onPostRenderChildren(){}
	bool    haveChildClipTest(){ return false; }
	bool    doClipTest(){ return true; }

public:

	virtual void  deleteThis(){ delete this; }
	virtual bool  onKeyMsg( unsigned key , bool isDown ){ (void)key; (void)isDown; return true; }
	virtual bool  onCharMsg( unsigned code ){ (void)code; return true; }
	virtual bool  onMouseMsg( MouseMsg const& msg){ (void)msg; return false; }

private:


	void          setTopChild( WidgetCoreT* ui , bool beAlways );
	void          _destroyChildren();

	void          setFlag( unsigned flag ){	mFlag = flag;	}
	unsigned      getFlag(){ return mFlag; }

	bool          checkFlag( unsigned flag ) const { return ( mFlag & flag ) != 0; }

	void          addChildFlag( unsigned flag );
	void          removeChildFlag( unsigned flag );

	void          setManager( TWidgetManager<T>* mgr );

	bool          hitTest( Vec2i const& testPos ){ return _this()->doHitTest( testPos ); }
	bool          clipTest(){ return _this()->doClipTest(); }
	WidgetCoreT*  hitTestChildren(Vec2i const& testPos);

protected:
	void          skipMouseMsg(){ _addFlag( WIF_PARENT_MOUSE_EVENT ); }
	void          lockDestroy()   { _addFlag( WIF_BLOCK_DESTROY );  }
	void          unlockDestroy() { _removeFlag( WIF_BLOCK_DESTROY ); }



	void          _addFlag( unsigned flag ){ mFlag |= flag; }
	void          _removeFlag( unsigned flag ){ mFlag &= ~flag; }

	virtual void  mouseOverlap( bool beOverlap ){  _this()->onMouse( beOverlap ); }
	virtual void  render(){  _this()->onRender();  }
	virtual void  focus( bool beF ){ _this()->onFocus( beF ); }
	void    update()    {  _this()->onUpdateUI();   }

private:

	void    renderAll()         { _this()->doRenderAll(); }
	void    prevRender()        {  _this()->onPrevRender();  }
	void    postRender()        {  _this()->onPostRender();  }
	void    postRenderChildren(){  _this()->onPostRenderChildren(); }


private:
	WidgetCoreT();

	void          init();
	friend class TWidgetManager< T >;

protected:

#if UI_CORE_USE_INTRLIST
	typedef TIntrList< T, MemberHook< WidgetCoreBase, &WidgetCoreBase::mLinkHook > > WidgetList;
	static WidgetCoreT*  hitTestInternal(Vec2i const& testPos, WidgetList& Widgetlist);
	WidgetList        mChildren;
#else
	WidgetCoreT*       mNext;
	WidgetCoreT*       mPrev;
	WidgetCoreT*       mChild;

#endif

	TWidgetManager<T>* mManager;
	WidgetCoreT*       mParent;

	Vec2i          mCacheWorldPos;
	unsigned       mFlag;
	int            mNumChild;
	Rect           mBoundRect;
};


template < class T >
class TWidgetManager
{
	typedef WidgetCoreT< T > WidgetCore;
public:
	TWidgetManager();
	~TWidgetManager();
	void      addWidget( WidgetCore* ui );
	
	void      update();
	void      render();

	void      destroyWidget(WidgetCore* ui );
	void      cleanupWidgets();

	bool      isTopWidget( WidgetCore* ui );
	MouseMsg& getLastMouseMsg(){ return mMouseMsg; }

	T*        hitTest( Vec2i const& testPos );

public:
	T*        getModalWidget()       { return static_cast< T* >( mNamedSlots[ ESlotName::Modal ] ); }
	T*        getFocusWidget()       { return static_cast< T* >( mNamedSlots[ ESlotName::Focus ]); }
	T*        getLastMouseMsgWidget(){ return static_cast< T* >( mNamedSlots[ ESlotName::LastMouseMsg ] ); }
	T*        getMouseWidget()       { return static_cast< T* >( mNamedSlots[ ESlotName::Mouse ]); }

	auto      createTopWidgetIterator()
	{
#if UI_CORE_USE_INTRLIST
		return MakeIterator(mTopWidgetList);
#else
		return mRoot.createChildrenIterator(); 
#endif
	}
	void      startModal(WidgetCore* ui );
	void      endModal(WidgetCore* ui );

	bool      procMouseMsg( MouseMsg const& msg );
	bool      procKeyMsg( unsigned key , bool isDown );
	bool      procCharMsg( unsigned code );

	void      focusWidget(WidgetCore* ui );

	void      captureMouse(WidgetCore* ui){ mNamedSlots[ESlotName::Capture] = ui;}
	void      releaseMouse(){ mNamedSlots[ESlotName::Capture] = NULL; }

	void      cleanupPaddingKillWidgets();

	template< class Visitor >
	void      visitWidgets( Visitor visitor );

protected:
	void        destroyNoCheck(WidgetCore* ui );
	void        removeWidgetReference(WidgetCore* ui );
	void        updateInternal(WidgetCore& ui );

	template< class Visitor >
	void        visitWidgetInternal( Visitor visitor , WidgetCore& ui );

	void        prevProcMsg();
	void        postProcMsg();

private:
	enum ESlotName
	{
		LastMouseMsg,
		Capture,
		Modal,
		Focus,
		Mouse,
		Num,
	};
	WidgetCore*   mNamedSlots[(int)ESlotName::Num];
	void      setNamedSlot(ESlotName name, WidgetCore& ui)
	{
		assert( mNamedSlots[name] == nullptr );
		mNamedSlots[name] = &ui;
		mNamedSlots[name]->_addFlag(WIF_MANAGER_REF);
	}
	void      clearNamedSlot(ESlotName name)
	{
		assert(mNamedSlots[name]);
		//mNamedSlots[name]->_removeFlag(UF_MANAGER_REF);
		mNamedSlots[name] = nullptr;
	}

	
	WidgetCore*    getKeyInputWidget();

#if UI_CORE_USE_INTRLIST
	typedef typename WidgetCore::WidgetList WidgetList;
	WidgetList   mTopWidgetList;
	WidgetList   mRemoveWidgetList;
#else
	WidgetCore*  getRoot() { return &mRoot; }
	WidgetCore   mRoot;
	WidgetCore   mRemoveUI;
#endif

	bool         mProcessingMsg;
	MouseMsg     mMouseMsg;

	friend class WidgetCore;
private:
	TWidgetManager( TWidgetManager const& ){}
	TWidgetManager& operator = (TWidgetManager const&);
};


#include "WidgetCore.hpp"


#endif // WidgetCore_h__
