#ifndef TUICore_hpp__
#define TUICore_hpp__

#include "WidgetCore.h"
#include <cassert>

template< class T >
WidgetCoreT<T>::WidgetCoreT( Vec2i const& pos , Vec2i const& size , T* parent ) 
	:mBoundRect( pos , pos + size )
{
	init();

	if ( parent && parent != UI_NO_PARENT )
		parent->addChild( this );
}


template< class T >
WidgetCoreT<T>::WidgetCoreT()
	:mBoundRect( Vec2i(0,0) , Vec2i(0,0))
{
	init();
}

template< class T >
void WidgetCoreT< T >::init()
{
	mParent = NULL;
#if UI_CORE_USE_INTRLIST

#else
	mChild = NULL;
	mNext = NULL;
	mPrev = NULL;
#endif
	mNumChild = 0;
	mManager = NULL;
	mFlag = 0;
}


template< class T >
WidgetCoreT<T>::~WidgetCoreT()
{
	if ( mParent )
		_unlinkInternal( true );
}

#if UI_CORE_USE_INTRLIST
template< class T >
WidgetCoreT<T>* WidgetCoreT<T>::hitTestInternal(Vec2i const& testPos , WidgetList& Widgetlist)
{
	WidgetCoreT* result = NULL;

	Vec2i  pos = testPos;
	WidgetList* curList = &Widgetlist;
	auto childiter = curList->rbegin();

	while( childiter != curList->rend() )
	{
		WidgetCoreT* ui = &(*childiter);

		bool testFail = true;
		if( !ui->checkFlag(WIF_DISABLE | WIF_BE_HIDDEN) )
			// ui->isEnable() && ui->isShow()
		{
			if( ui->checkFlag(WIF_HITTEST_CHILDREN) )
			{
				Vec2i tPos = pos - ui->getPos();
				result = hitTestInternal( tPos , ui->mChildren );
				if( result )
					return result;
			}
			else if( static_cast<T*>(ui)->doHitTest(pos) )
			{
				result = ui;
				curList = &ui->mChildren;
				childiter = curList->rbegin();
				if( childiter == curList->rend() )
					return result;

				pos -= ui->getPos();
				testFail = false;
			}
		}

		if( testFail )
		{
			++childiter;
		}
	}

	return result;
}
#endif

template< class T >
WidgetCoreT<T>* WidgetCoreT<T>::hitTestChildren(Vec2i const& testPos)
{
#if UI_CORE_USE_INTRLIST
	return hitTestInternal(testPos, mChildren);
#else
	WidgetCoreT* result = NULL;

	if( !mChild )
		return NULL;

	Vec2i  pos = testPos;

	WidgetCoreT* child = mChild;
	WidgetCoreT* ui = child->mPrev;

	while( 1 )
	{
		bool testFail = true;
		if( !ui->checkFlag(WIF_DISABLE | WIF_BE_HIDDEN) )
			// ui->isEnable() && ui->isShow()
		{
			if( ui->checkFlag(WIF_HITTEST_CHILDREN) )
			{
				Vec2i tPos = pos - ui->getPos();
				result = ui->hitTestChildren(tPos);
				if( result )
					return result;
			}
			else if( static_cast<T*>(ui)->doHitTest(pos) )
			{
				result = ui;
				child = ui->mChild;

				if( !child )
					return result;

				pos -= ui->getPos();
				ui = child->mPrev;

				testFail = false;
			}
		}

		if( testFail )
		{
			if( ui == child )
				return result;

			ui = ui->mPrev;
		}
	}
	return result;
#endif
}


template< class T >
void WidgetCoreT<T>::_unlinkInternal( bool bRemove)
{
	

#if UI_CORE_USE_INTRLIST
	mLinkHook.unlink();
#else
	assert(mParent);
	if ( mParent->mChild == this )
	{
		mParent->mChild = mNext;

		if ( mNext )
			mNext->mPrev = mPrev;
	}
	else
	{
		mPrev->mNext = mNext;

		if ( mNext )
			mNext->mPrev = mPrev;
		else
			mParent->mChild->mPrev = mPrev;
	}

	mNext = NULL;
	mPrev = NULL;
#endif

	if ( !isTop() )
		static_cast< T* >( mParent )->onChangeChildrenOrder();

	if( mParent )
	{
		--mParent->mNumChild;
	}

	mParent = NULL;
	if ( bRemove )
		mManager = NULL;
}


template< class T >
void WidgetCoreT<T>::linkChildInternal( WidgetCoreT* ui )
{
	assert( ui->mParent == NULL );

#if UI_CORE_USE_INTRLIST
	mChildren.push_back(*static_cast<T*>(ui));
#else
	if ( mChild )
	{
		WidgetCoreT* lastUI = mChild->mPrev;
		lastUI->mNext = ui;
		ui->mPrev = lastUI;
	}
	else
	{
		mChild = ui;
	}

	mChild->mPrev = ui;
	ui->mNext = NULL;
#endif

	ui->mParent = this;

	if ( mManager )
		ui->setManager( mManager );

	++mNumChild;
}
template< class T >
T& WidgetCoreT<T>::addChild( WidgetCoreT* ui )
{
	if ( ui->mParent )
		ui->_unlinkInternal(true);

	linkChildInternal( ui );

	ui->removeChildFlag( WIF_WORLD_POS_CACHED);

	static_cast< T* >( ui )->onLink();

	return *_this();
}

template< class T >
bool WidgetCoreT<T>::isFocus()
{
	assert( getManager() );
	return getManager()->getFocusWidget() == this;
}

template< class T >
T&  WidgetCoreT<T>::show( bool beS )
{
	if ( isShow() != beS )
	{
		if ( beS )
			removeFlagInternal( WIF_BE_HIDDEN );
		else
			addFlagInternal( WIF_BE_HIDDEN );

		_this()->onShow( beS );

		for( auto ui = createChildrenIterator() ; ui ; ++ui )
		{
			ui->show( beS );
		}
	}
	return *_this();
}

template< class T >
T&  WidgetCoreT<T>::enable( bool beE = true )
{ 
	if ( isEnable() != beE )
	{
		if ( beE )
			removeFlagInternal( WIF_DISABLE );
		else
			addFlagInternal( WIF_DISABLE );

		_this()->onEnable( beE );
	}
	return *_this();
}


template< class T >
int WidgetCoreT<T>::getLevel()
{
	WidgetCoreT* ui = mParent;
	int result = 0;
	while ( ui ){ ++result; ui = ui->mParent; }
	//have not linked manger
	if ( !mManager && mParent )
		++result;

	return result;
}

template< class T >
int WidgetCoreT<T>::getOrder()
{
	assert( mParent );
	int count = 0;
	for( auto ui = mParent->createChildrenIterator(); ui; ++ui )
	{
		if ( &(*ui) == this )
			break;
		++count;
	}
	return count;
}


template< class T >
Vec2i const& WidgetCoreT<T>::getWorldPos()
{
	if ( !checkFlag( WIF_WORLD_POS_CACHED ) )
	{
		mCacheWorldPos = getPos();

		if ( !isTop() )
			mCacheWorldPos += mParent->getWorldPos();

		addFlagInternal( WIF_WORLD_POS_CACHED );
	}
	return mCacheWorldPos;
}

template< class T >
T& WidgetCoreT<T>::setPos( Vec2i const& pos )
{
	Vec2i offset = pos - mBoundRect.min;

	mBoundRect.min = pos;
	mBoundRect.max += offset;

	removeFlagInternal( WIF_WORLD_POS_CACHED );
	removeChildFlag( WIF_WORLD_POS_CACHED);
	_this()->onChangePos( pos , true );

	return *_this();
}

template< class T >
void WidgetCoreT<T>::setManager( TWidgetManager<T>* mgr )
{
	mManager = mgr;
	for( auto child = createChildrenIterator(); child; ++child )
	{
		child->setManager( mgr );
	}
}

#if UI_CORE_USE_INTRLIST
template< class T >
void WidgetCoreT<T>::SetTopInternal(WidgetList& list, WidgetCoreT* ui, bool bAlwaysTop)
{
	//TODO
	ui->mLinkHook.unlink();
	list.push_back(*static_cast<T*>(ui));
}
#endif

template< class T >
void WidgetCoreT<T>::setTopChild( WidgetCoreT* ui , bool beAlways )
{
#if UI_CORE_USE_INTRLIST
	SetTopInternal(mChildren, ui, beAlways);
#else
	assert( ui->mNext );

	if ( mChild == ui )
	{
		mChild = ui->mNext;
		mChild->mPrev = ui->mPrev;
	}
	else
	{
		ui->mPrev->mNext = ui->mNext;
		ui->mNext->mPrev = ui->mPrev;
	}

	assert( mChild );

	WidgetCoreT* lastUI = mChild->mPrev;
	assert( lastUI );

	lastUI->mNext = ui;
	ui->mPrev = lastUI;
	ui->mNext = NULL;

	mChild->mPrev = ui;
#endif

#if !UI_CORE_USE_INTRLIST
	if ( getManager()->getRoot() != this )
		_this()->onChangeChildrenOrder();
#endif
}

template< class T >
T& WidgetCoreT<T>::setTop( bool beAlways )
{
	//return *_this();
	if ( beAlways )
		addFlagInternal( WIF_STAY_TOP );

#if UI_CORE_USE_INTRLIST
	if( getParent() )
	{
		getParent()->setTopChild(this, beAlways);

	}
	else
	{
		SetTopInternal(getManager()->mTopWidgetList ,  this, beAlways);
	}
	_this()->onChangeOrder();
#else
	if ( mNext )
	{
		getParent()->setTopChild( this , beAlways );
		_this()->onChangeOrder();
	}
#endif

	return *_this();
}

template< class T >
bool WidgetCoreT<T>::isTop()
{
	return getManager()->isTopWidget( this );
}

template< class T >
void WidgetCoreT<T>::addChildFlag( unsigned flag )
{
	for( auto child = createChildrenIterator(); child; ++child )
	{
		child->addFlagInternal( flag );
		child->addChildFlag( flag );
	}
}

template< class T >
void WidgetCoreT<T>::removeChildFlag( unsigned flag )
{
	for( auto child = createChildrenIterator(); child; ++child )
	{
		child->removeFlagInternal( flag );
		child->removeChildFlag( flag );
	}
}


template< class T >
T& WidgetCoreT<T>::makeFocus()
{
	getManager()->focusWidget( this );
	return *_this();
}

template< class T >
void WidgetCoreT<T>::destroyChildren_R()
{
	auto childIter = createChildrenIterator();
	while( childIter )
	{
		WidgetCoreT* ui = &(*childIter);
		ui->destroyChildren_R();

		if ( getManager() )
			getManager()->removeWidgetReference( ui );

		++childIter;
		ui->deleteThis();
	}
}

template< class T >
void WidgetCoreT<T>::doRenderAll()
{
	prevRender();
	render();
	postRender();

	bool bClipTest = _this()->haveChildClipTest();

	if ( bClipTest )
	{
		for( auto child = createChildrenIterator(); child; ++child )
		{
			if ( !child->isShow() )
				continue;
			if ( !child->clipTest() )
				continue;
			child->renderAll();
		}
	}
	else
	{
		for (auto child = createChildrenIterator(); child; ++child)
		{
			if ( !child->isShow() )
				continue;
			child->renderAll();
		}
	}
	postRenderChildren();
}

template< class T >
void WidgetCoreT<T>::destroy()
{
	if( getManager() )
	{
		getManager()->destroyWidget(this);
	}
	else
	{
		destroyChildren_R();
		deleteThis();
	}
}

template< class T >
TWidgetManager<T>::TWidgetManager()
	:mProcessingMsg(false)
#if !UI_CORE_USE_INTRLIST
	,mRoot()
	, mRemoveUI()
#endif
{
	std::fill_n(mNamedSlots, (int)ESlotName::Num, nullptr);
#if !UI_CORE_USE_INTRLIST
	mRoot.setManager( this );
	mRemoveUI.setManager(this);
#endif
}

template < class T >
TWidgetManager<T>::~TWidgetManager()
{
	cleanupWidgets();
	cleanupPaddingKillWidgets();
}

template< class T >
void TWidgetManager<T>::prevProcMsg()
{
	mProcessingMsg = true;
}
template< class T >
void TWidgetManager<T>::postProcMsg()
{
	mProcessingMsg = false;
	cleanupPaddingKillWidgets();
}

template< class T >
bool TWidgetManager<T>::procMouseMsg( MouseMsg const& msg )
{
	PROFILE_ENTRY("UI System");

	ProcMsgScope scope(this);

	bool result = true;

	mMouseMsg = msg;

	WidgetCore* ui = nullptr;
	if ( mNamedSlots[ESlotName::Capture] )
	{
		ui = mNamedSlots[ESlotName::Capture];
	}
	else if( mNamedSlots[ESlotName::Modal] )
	{
		if( mNamedSlots[ESlotName::Modal]->hitTest(mMouseMsg.getPos()) )
		{
			ui = mNamedSlots[ESlotName::Modal]->hitTestChildren(mMouseMsg.getPos() - mNamedSlots[ESlotName::Modal]->getPos());
		}		
	}
	else
	{
		ui = hitTest( mMouseMsg.getPos() );
	}

	if( mNamedSlots[ESlotName::LastMouseMsg] )
		clearNamedSlot(ESlotName::LastMouseMsg);
	if ( ui )
		setNamedSlot(ESlotName::LastMouseMsg , *ui);

	{
		PROFILE_ENTRY("Msg Process");

		if (  msg.onLeftDown() )
		{
			
			if ( mNamedSlots[ESlotName::Focus] != ui )
			{
				if( mNamedSlots[ESlotName::Focus] )
				{
					WidgetCore* temp = mNamedSlots[ESlotName::Focus];
					clearNamedSlot(ESlotName::Focus);
					temp->focus(false);
				}

				if ( ui )
				{
					setNamedSlot(ESlotName::Focus, *ui);
					ui->focus(true);
				}
			}
		}
		else if ( msg.onMoving() )
		{
			
			if ( mNamedSlots[ESlotName::Mouse] != ui )
			{
				if ( mNamedSlots[ESlotName::Mouse] )
				{
					WidgetCore* temp = mNamedSlots[ESlotName::Mouse];
					clearNamedSlot(ESlotName::Mouse);
					temp->mouseOverlap(false);
				}

				if ( ui )
				{
					setNamedSlot(ESlotName::Mouse, *ui);
					ui->mouseOverlap(true);
				}
			}
		}


		if ( ui )
		{
			if ( msg.isDraging() && ui != mNamedSlots[ESlotName::Focus] )
			{
				result = (mNamedSlots[ESlotName::Focus] == NULL ) ;
			}
			else
			{
				result = ui->onMouseMsg( mMouseMsg );
				while ( result && ui->checkFlag( WIF_PARENT_MOUSE_EVENT ) )
				{
					if ( ui->isTop() )
						break;
					ui->removeFlagInternal( WIF_PARENT_MOUSE_EVENT );

					ui = static_cast< T* >( ui->getParent() );
					result = ui->onMouseMsg( mMouseMsg );

					if( mNamedSlots[ESlotName::LastMouseMsg] )
						clearNamedSlot(ESlotName::LastMouseMsg);

					setNamedSlot(ESlotName::LastMouseMsg, *ui);
				}
			}

			//while ( bool still = ui->onMouseMsg( mMouseMsg ) )
			//{
			//	ui = ui->getParent();
			//	if ( !still || ui == getRoot() )
			//		return still;
			//}
		}
	}

	return result;
}

template < class T >
WidgetCoreT<T>* TWidgetManager<T>::getKeyInputWidget()
{
	if( mNamedSlots[ESlotName::Focus] )
	{
		return mNamedSlots[ESlotName::Focus]->isEnable() ? mNamedSlots[ESlotName::Focus] : nullptr;
	}
	else if( mNamedSlots[ESlotName::Modal] )
	{
		return mNamedSlots[ESlotName::Modal]->isEnable() ? mNamedSlots[ESlotName::Modal] : nullptr;
	}
	return nullptr;
}

template< class T >
bool TWidgetManager<T>::procCharMsg( unsigned code )
{
	ProcMsgScope scope(this);

	bool result = true;
	WidgetCore* ui = getKeyInputWidget();
	if( ui )
	{
		result = ui->onCharMsg(code);
	}
	return result;
}

template< class T >
bool TWidgetManager<T>::procKeyMsg( unsigned key , bool isDown )
{
	ProcMsgScope scope(this);

	bool result = true;
	WidgetCore* ui = getKeyInputWidget();
	if( ui )
	{
		result = ui->onKeyMsg(key, isDown);
	}
	return result;
}

template< class T >
T* TWidgetManager<T>::hitTest( Vec2i const& testPos )
{
	PROFILE_ENTRY("UI Hit Test");
#if UI_CORE_USE_INTRLIST
	return static_cast< T*>( WidgetCore::hitTestInternal( testPos , mTopWidgetList ) );
#else
	return static_cast< T* >(mRoot.hitTestChildren(testPos));
#endif
	
}

template< class T >
void TWidgetManager<T>::update()
{
	PROFILE_ENTRY("UI System");

	{
		PROFILE_ENTRY("UpdateUI");
#if UI_CORE_USE_INTRLIST
		for( auto ui = createTopWidgetIterator(); ui; ++ui )
#else
		for( auto ui = mRoot.createChildrenIterator(); ui; ++ui )
#endif
		{
			updateInternal(*ui);
		}
		
	}
}

template< class T >
void TWidgetManager<T>::updateInternal( WidgetCore& ui )
{
	if( ui.isEnable() )
	{
		ui.update();
	}
	if( ui.isEnable() || ui.checkFlag(WIF_EFFECT_DISABLE_CHILD) == false )
	{
		for( auto child = ui.createChildrenIterator(); child; ++child )
		{
			updateInternal(*child);
		}
	}
}

template< class T >
void TWidgetManager<T>::render()
{
#if UI_CORE_USE_INTRLIST
	for( auto ui = createTopWidgetIterator(); ui; ++ui )
#else
	for( auto ui = mRoot.createChildrenIterator(); ui; ++ui )
#endif
	{
		if ( !ui->isShow() )
			continue;
		ui->renderAll();
	}
}


template< class T >
template< class Visitor >
void  TWidgetManager<T>::visitWidgets( Visitor visitor )
{
	PROFILE_ENTRY("UI System");
	{
		PROFILE_ENTRY("visitUI");
#if UI_CORE_USE_INTRLIST
		for( auto ui = createTopWidgetIterator(); ui; ++ui )
#else
		for( auto ui = mRoot.createChildrenIterator(); ui; ++ui )
#endif
		{
			visitWidgetInternal(visitor , *ui);
		}
	}
}

template< class T >
template< class Visitor >
void  TWidgetManager<T>::visitWidgetInternal( Visitor visitor , WidgetCore& ui )
{
	visitor(static_cast<T*>(&ui));
	for( auto child = ui.createChildrenIterator(); child; ++child )
	{
		visitWidgetInternal(visitor, *child);
	}
}

template< class T >
void TWidgetManager<T>::removeWidgetReference( WidgetCore* ui )
{
	if( !ui->checkFlag(WIF_MANAGER_REF) )
		return;

	if ( mNamedSlots[ESlotName::Focus] == ui )
	{
		clearNamedSlot(ESlotName::Focus);
		ui->focus( false );
	}
	if ( mNamedSlots[ESlotName::Mouse] == ui )
	{
		clearNamedSlot(ESlotName::Mouse);
		ui->mouseOverlap( false );
	}
	for( int i = 0; i < (int)ESlotName::Num; ++i )
	{
		if( mNamedSlots[i] == ui )
		{
			clearNamedSlot(ESlotName(i));
		}	
	}
}

template< class T >
void TWidgetManager<T>::destroyWidget( WidgetCore* ui )
{
	if ( ui == NULL || ui->checkFlag( WIF_MARK_DESTROY ) )
		return;

	ui->addFlagInternal(WIF_MARK_DESTROY);
	if ( mProcessingMsg || ui->checkFlag( WIF_BLOCK_DESTROY ))
	{
		ui->_unlinkInternal(false);
#if UI_CORE_USE_INTRLIST
		mRemoveWidgetList.push_back(*static_cast<T*>(ui));
#else
		mRemoveUI.linkChildInternal( ui );
#endif

	}
	else
	{
		destroyNoCheck( ui );
	}
}

template< class T >
void TWidgetManager<T>::cleanupPaddingKillWidgets()
{
#if UI_CORE_USE_INTRLIST
	while( !mRemoveWidgetList.empty() )
	{
		destroyNoCheck( &mRemoveWidgetList.front() );
	}
#else
	WidgetCore* ui = mRemoveUI.getChild();
	while( ui )
	{
		destroyNoCheck(ui);
		ui = mRemoveUI.getChild();
	}
#endif
}

template< class T >
void TWidgetManager<T>::destroyNoCheck( WidgetCore* ui )
{
	removeWidgetReference( ui );
	ui->destroyChildren_R();
	ui->_unlinkInternal(true);
	ui->deleteThis();
}

template< class T >
void TWidgetManager<T>::cleanupWidgets()
{
#if UI_CORE_USE_INTRLIST
	while( !mTopWidgetList.empty() )
	{
		destroyWidget(&mTopWidgetList.front());
	}
#else
	WidgetCore* child = getRoot()->getChild();
	while ( child )
	{
		destroyWidget( child );
		child = getRoot()->getChild();
	}
#endif
}

template< class T >
void TWidgetManager<T>::addWidget( WidgetCore* ui )
{
#if UI_CORE_USE_INTRLIST
	mTopWidgetList.push_back(*static_cast<T*>(ui));
	ui->setManager(this);
#else
	assert( ui && ui->getParent() == NULL );
	mRoot.addChild( ui );
#endif
}

template< class T >
void TWidgetManager<T>::startModal( WidgetCore* ui )
{
	if ( mNamedSlots[ESlotName::Modal] )
		return;
	setNamedSlot(ESlotName::Modal, *ui);
}

template< class T >
void TWidgetManager<T>::endModal( WidgetCore* ui )
{
	if( ui == mNamedSlots[ESlotName::Modal] )
		clearNamedSlot(ESlotName::Modal);
}

template< class T >
bool TWidgetManager<T>::isTopWidget( WidgetCore* ui )
{
#if UI_CORE_USE_INTRLIST
	return ui->getParent() == nullptr;
#else
	WidgetCore* parent = ui->getParent();
	return parent == getRoot() ||
		   parent == &mRemoveUI;
#endif
}

template< class T >
void TWidgetManager<T>::focusWidget( WidgetCore* ui )
{
	if ( ui == mNamedSlots[ESlotName::Focus] )
		return;
	if ( mNamedSlots[ESlotName::Focus] )
	{
		WidgetCore* temp = mNamedSlots[ESlotName::Focus];
		clearNamedSlot(ESlotName::Focus);
		temp->focus(false);
	}

	if ( ui )
	{
		setNamedSlot(ESlotName::Focus, *ui);
		ui->focus(true);
	}
}

#endif // TUICore_hpp__
