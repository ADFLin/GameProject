#ifndef WidgetCore_h__
#define WidgetCore_h__

#include <cstdlib>
#include <cassert>
#include <vector>

#include "SystemMessage.h"
#include "Rect.h"

#include "DataStructure/IntrList.h"
#include "StdUtility.h"

//#include "ProfileSystem.h"
#define  WIDGET_PROFILE_ENTRY(...)


#ifndef BIT
#define BIT( n ) ( 1 << ( n ) )
#endif 

#define UI_NO_PARENT (void*)(~0)

enum WidgetInternalFlag
{
	WIF_PERSISTENT           = BIT(0),
	WIF_STAY_TOP             = BIT(1),
	WIF_WORLD_POS_CACHED     = BIT(2),
	WIF_DISABLE              = BIT(3),
	WIF_HAD_HIDDEN           = BIT(4),
	WIF_HITTEST_CHILDREN     = BIT(5),
	WIF_DEFERRED_DESTROY     = BIT(6),

	WIF_MARK_DESTROY         = BIT(7),
	//WIF_      = BIT(8),
	WIF_EFFECT_DISABLE_CHILD = BIT(9),
	WIF_MANAGER_REF          = BIT(10),

	WIF_REROUTE_MOUSE_EVENT_UNHANDLED = BIT(11),
	WIF_REROUTE_KEY_EVENT_UNHANDLED   = BIT(12),
	WIF_REROUTE_CHAR_EVENT_UNHANDLED  = BIT(13),
	WIF_REROUTE_MOUSE_EVENT           = BIT(14),
	WIF_REROUTE_KEY_EVENT             = BIT(15),
	WIF_REROUTE_CHAR_EVENT            = BIT(16),
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
	LinkHook       mLinkHook;
};

template< class T >
class WidgetCoreT : public WidgetCoreBase
{
	T* _this() { return static_cast<T*>(this); }
	using Rect = TRect< int >;
	using WidgetCore = WidgetCoreT<T>;

public:
	
	WidgetCoreT(Vec2i const& pos, Vec2i const& size, T* parent);
	virtual ~WidgetCoreT();

	auto           createChildrenIterator() { return MakeIterator(mChildren); }
	T*             getChild() { return (mChildren.empty()) ? nullptr : &mChildren.front(); }

	T*             getParent() { return static_cast<T*>( mParent ); }
	TWidgetManager<T>* getManager() { return mManager; }
	int            getChildrenNum() { return mNumChild; }
	int            getLevel();
	int            getOrder();

	Vec2i const&  getWorldPos();
	Vec2i const&  getPos() const { return mBoundRect.min; }
	Vec2i         getSize() const { return mBoundRect.getSize(); }
	Rect const&   getBoundRect() const { return mBoundRect; }

	bool           isFocus();
	bool           isEnable() const;
	bool           isShow() const;
	bool           isTop();

	T&             setPos(Vec2i const& pos);
	T&             setSize(Vec2i const& size) { mBoundRect.max = mBoundRect.min + size;  return *_this(); }
	T&             setTop(bool beAlways = false);
	T&             makeFocus();
	T&             clearFocus(bool bSubWidgetsIncluded = true);
	T&             show(bool beS = true);
	T&             enable(bool beE = true);

	T&             addChild(WidgetCoreT* ui);
	bool           isSubWidgetOf(WidgetCoreT* ui) const;
	void           destroy();

	T&             setGlobal() { addFlagInternal(WIF_PERSISTENT); return *_this(); }


public:
	void          unlinkInternal(bool bRemove);
	bool          checkFlag(unsigned flag) const { return !!(mFlag & flag); }

private:
	void          linkChildInternal(WidgetCoreT* ui);

protected:
	void    onLink() {}
	void    onUnlink() {}
	void    onChangeOrder() {}
	void    onChangeChildrenOrder() {}
	void    onEnable(bool beE) {}
	void    onUpdateUI() {}
	void    onChangePos(Vec2i const& pos, bool bParentMove) {}
	void    onShow(bool beS) {}
	void    onMouse(bool beIn) {}

	void    onFocus(bool beF) {}
	void    onResize(Vec2i const& size) {}
	bool    doHitTest(Vec2i const& pos) { return mBoundRect.hitTest(pos); }

	void    doRenderAll();
	void    onRender() {}
	void    onPrevRender() {}
	void    onPostRender() {}
	void    onPostRenderChildren() {}
	bool    haveChildClipTest() { return false; }
	bool    doClipTest() { return true; }

public:

	virtual void  deleteThis() { delete this; }
	virtual MsgReply onKeyMsg(KeyMsg const& msg) { (void)msg; return MsgReply::Unhandled(); }
	virtual MsgReply onCharMsg(unsigned code) { (void)code; return MsgReply::Unhandled(); }
	virtual MsgReply onMouseMsg(MouseMsg const& msg) { (void)msg; return MsgReply::Handled(); }

private:
	void    deleteChildren_R();

	void          setTopChild(WidgetCoreT* ui, bool beAlways);

	void          setFlag(unsigned flag) { mFlag = flag; }
	unsigned      getFlag() { return mFlag; }


	void          addChildFlag(unsigned flag);
	void          removeChildFlag(unsigned flag);
	void          enableFlag(unsigned flag , bool bEnable)
	{
		if( bEnable ) addFlagInternal(flag);
		else removeFlagInternal(flag);
	}

	void          setManager(TWidgetManager<T>* mgr);

	bool          hitTest(Vec2i const& testPos) { return _this()->doHitTest(testPos); }
	bool          clipTest() { return _this()->doClipTest(); }
	WidgetCoreT*  hitTestChildren(Vec2i const& testPos);

public:
	void          setRerouteMouseMsgUnhandled() { addFlagInternal(WIF_REROUTE_MOUSE_EVENT_UNHANDLED); }
	void          setRerouteKeyMsgUnhandled() { addFlagInternal(WIF_REROUTE_KEY_EVENT_UNHANDLED); }
	void          setRerouteCharMsgUnhandled() { addFlagInternal(WIF_REROUTE_CHAR_EVENT_UNHANDLED); }

	void          setRerouteMouseMsg() { addFlagInternal(WIF_REROUTE_MOUSE_EVENT); }
	void          setRerouteKeyMsg() { addFlagInternal(WIF_REROUTE_KEY_EVENT); }
	void          setRerouteCharMsg(){ addFlagInternal(WIF_REROUTE_CHAR_EVENT); }

protected:
	void          markDeferredDestroy() { addFlagInternal(WIF_DEFERRED_DESTROY); }
	void          unmarkDeferredDestroy() { removeFlagInternal(WIF_DEFERRED_DESTROY); }
	

	void          addFlagInternal( unsigned flag ){ mFlag |= flag; }
	void          removeFlagInternal( unsigned flag ){ mFlag &= ~flag; }

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

	using WidgetList = TIntrList< T, MemberHook< WidgetCoreBase, &WidgetCoreBase::mLinkHook > >;
	static void SetTopInternal(WidgetList& list, WidgetCoreT* ui , bool bAlwaysTop );

	static WidgetCoreT*  hitTestInternal(Vec2i const& testPos, WidgetList& Widgetlist);
	WidgetList        mChildren;


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
	using WidgetCore = WidgetCoreT< T >;
public:
	TWidgetManager();
	~TWidgetManager();
	void      addWidget( WidgetCore* ui );
	
	void      update();
	void      render();

	void      destroyWidget(WidgetCore* ui );
	void      cleanupWidgets( bool bPersistentIncluded = false);

	bool      isTopWidget( WidgetCore* ui );
	MouseMsg& getLastMouseMsg(){ return mLastMouseMsg; }

	T*        hitTest( Vec2i const& testPos );

public:
	T*        getModalWidget()       { return static_cast< T* >( mNamedSlots[ ESlotName::Modal ] ); }
	T*        getFocusWidget()       { return static_cast< T* >( mNamedSlots[ ESlotName::Focus ]); }
	T*        getLastMouseMsgWidget(){ return static_cast< T* >( mNamedSlots[ ESlotName::LastMouseMsg ] ); }
	T*        getMouseWidget()       { return static_cast< T* >( mNamedSlots[ ESlotName::Mouse ]); }

	auto      createTopWidgetIterator()
	{
		return MakeIterator(mTopWidgetList);
	}
	void      startModal(WidgetCore* ui );
	void      endModal(WidgetCore* ui );

	MsgReply  procMouseMsg( MouseMsg const& msg );
	MsgReply  procKeyMsg( KeyMsg const& msg );
	MsgReply  procCharMsg( unsigned code );

	void      focusWidget(WidgetCore* ui );
	void      clearFocusWidget(WidgetCore* ui, bool bSubWidgetsIncluded);

	void      captureMouse(WidgetCore* ui){ mNamedSlots[ESlotName::Capture] = ui;}
	void      releaseMouse(){ mNamedSlots[ESlotName::Capture] = NULL; }

	void      cleanupPaddingKillWidgets();

	template< class Visitor >
	void      visitWidgets( Visitor visitor );


	bool isProcessingMsg() const { return mbProcessingMsg; }

protected:
	void        destroyWidgetChildren_R(WidgetCore* ui);
	void        destroyWidgetActually(WidgetCore* ui);
	void        removeWidgetReference(WidgetCore* ui);
	void        updateInternal(WidgetCore& ui);

	template< class Visitor >
	void        visitWidgetInternal( Visitor visitor , WidgetCore& ui );

	void        prevProcMsg();
	void        postProcMsg();

	template< class TFunc >
	MsgReply processMessage(WidgetCore* ui, WidgetInternalFlag flag, WidgetInternalFlag unhandledFlag, TFunc func);

	struct ProcMsgScope
	{
		ProcMsgScope( TWidgetManager* inManager )
			:manager(inManager)
		{
			manager->prevProcMsg();
		}
		~ProcMsgScope()
		{
			manager->postProcMsg();
		}
		TWidgetManager* manager;
	};

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
		mNamedSlots[name]->addFlagInternal(WIF_MANAGER_REF);
	}
	void      clearNamedSlot(ESlotName name)
	{
		assert(mNamedSlots[name]);
		//mNamedSlots[name]->removeFlagInternal(UF_MANAGER_REF);
		mNamedSlots[name] = nullptr;
	}

	
	WidgetCore*    getKeyInputWidget();

	using WidgetList = typename WidgetCore::WidgetList;
	WidgetList   mTopWidgetList;
	WidgetList   mDeferredDestroyWidgets;

	bool         mbProcessingMsg;
	MouseMsg     mLastMouseMsg;

	friend class WidgetCore;
private:
	TWidgetManager( TWidgetManager const& ){}
	TWidgetManager& operator = (TWidgetManager const&);

};

#include "WidgetCore.hpp"


#endif // WidgetCore_h__
