#pragma once
#ifndef WidgetCore_H_0D21F7C5_C362_437A_8BC5_D4C98EDB3024
#define WidgetCore_H_0D21F7C5_C362_437A_8BC5_D4C98EDB3024

#include "WidgetCore.h"
#include "LogSystem.h"
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
	mNumChild = 0;
	mManager = NULL;
	mFlag = 0;
}


template< class T >
WidgetCoreT<T>::~WidgetCoreT()
{
	if ( mParent )
		unlinkInternal( true );
}

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

template< class T >
WidgetCoreT<T>* WidgetCoreT<T>::hitTestChildren(Vec2i const& testPos)
{
	return hitTestInternal(testPos, mChildren);
}


template< class T >
void WidgetCoreT<T>::unlinkInternal( bool bRemove )
{
	mLinkHook.unlink();

	if ( !isTop() )
		static_cast< T* >( mParent )->onChangeChildrenOrder();

	if( mParent )
	{
		--mParent->mNumChild;
		mParent = nullptr;
	}

	if ( bRemove )
		mManager = nullptr;
}

template< class T >
void WidgetCoreT<T>::linkChildInternal( WidgetCoreT* ui )
{
	assert( ui->mParent == nullptr );

	mChildren.push_back(*static_cast<T*>(ui));

	ui->mParent = this;

	if ( mManager )
		ui->setManager( mManager );

	++mNumChild;
}

template< class T >
T& WidgetCoreT<T>::addChild( WidgetCoreT* ui )
{
	if ( ui->mParent )
		ui->unlinkInternal(true);

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
	bool bShown = isShow();
	if ( bShown != beS )
	{
		enableFlag(WIF_BE_HIDDEN, !beS);
		_this()->onShow( beS );

		for( auto ui = createChildrenIterator() ; ui ; ++ui )
		{
			ui->show( beS );
		}
	}
	return *_this();
}

template< class T >
T&  WidgetCoreT<T>::enable( bool beE )
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
bool WidgetCoreT<T>::isEnable() const
{
	return !checkFlag(WIF_DISABLE);
}


template< class T >
bool WidgetCoreT<T>::isShow() const
{
	return !checkFlag(WIF_BE_HIDDEN);
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
	_this()->onChangePos( pos , false );

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

template< class T >
void WidgetCoreT<T>::SetTopInternal(WidgetList& list, WidgetCoreT* ui, bool bAlwaysTop)
{
	//TODO
	ui->mLinkHook.unlink();
	list.push_back(*static_cast<T*>(ui));
}


template< class T >
void WidgetCoreT<T>::setTopChild( WidgetCoreT* ui , bool beAlways )
{
	SetTopInternal(mChildren, ui, beAlways);
}

template< class T >
T& WidgetCoreT<T>::setTop( bool beAlways )
{
	//return *_this();
	if ( beAlways )
		addFlagInternal( WIF_STAY_TOP );

	if( getParent() )
	{
		getParent()->setTopChild(this, beAlways);
	}
	else
	{
		SetTopInternal(getManager()->mTopWidgetList ,  this, beAlways);
	}
	_this()->onChangeOrder();


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
void WidgetCoreT<T>::doRenderAll()
{
	prevRender();
	render();
	postRender();

	bool const bClipTest = _this()->haveChildClipTest();

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
		deleteChildren_R();
		deleteThis();
	}
}

template< class T >
void WidgetCoreT<T>::deleteChildren_R()
{
	auto iterChildren = createChildrenIterator();

	while( iterChildren )
	{
		WidgetCoreT* child = &(*iterChildren);
		child->deleteChildren_R();
		++iterChildren;
		child->deleteThis();
	}
}

template< class T >
TWidgetManager<T>::TWidgetManager()
	:mbProcessingMsg(false)
{
	std::fill_n(mNamedSlots, (int)ESlotName::Num, nullptr);
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
	assert(mbProcessingMsg == false);
	mbProcessingMsg = true;
}

template< class T >
void TWidgetManager<T>::postProcMsg()
{
	mbProcessingMsg = false;
	cleanupPaddingKillWidgets();
}

template< class T >
bool TWidgetManager<T>::procMouseMsg( MouseMsg const& msg )
{
	//System maybe send callback
	if (mbProcessingMsg)
		return false;
	WIDGET_PROFILE_ENTRY("UI System");

	ProcMsgScope scope(this);

	bool result = true;

	mLastMouseMsg = msg;

	WidgetCore* ui = nullptr;
	if ( mNamedSlots[ESlotName::Capture] )
	{
		ui = mNamedSlots[ESlotName::Capture];
	}
	else if( mNamedSlots[ESlotName::Modal] )
	{
		if( mNamedSlots[ESlotName::Modal]->hitTest(mLastMouseMsg.getPos()) )
		{
			ui = mNamedSlots[ESlotName::Modal]->hitTestChildren(mLastMouseMsg.getPos() - mNamedSlots[ESlotName::Modal]->getPos());
		}		
	}
	else
	{
		ui = hitTest( mLastMouseMsg.getPos() );
	}

	if (mNamedSlots[ESlotName::LastMouseMsg])
	{
		clearNamedSlot(ESlotName::LastMouseMsg);
	}
	if (ui)
	{
		setNamedSlot(ESlotName::LastMouseMsg, *ui);
	}

	{
		WIDGET_PROFILE_ENTRY("Msg Process");

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
				result = ui->onMouseMsg( mLastMouseMsg );
				while ( result && ui->checkFlag( WIF_REROUTE_MOUSE_EVENT_UNHANDLED ) )
				{
					if ( ui->isTop() )
						break;

					ui = static_cast< T* >( ui->getParent() );
					result = ui->onMouseMsg( mLastMouseMsg );

					if( mNamedSlots[ESlotName::LastMouseMsg] )
						clearNamedSlot(ESlotName::LastMouseMsg);

					setNamedSlot(ESlotName::LastMouseMsg, *ui);
				}
			}

			//while ( bool still = ui->onMouseMsg( mLastMouseMsg ) )
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
template< class Func >
bool TWidgetManager<T>::processMessage(WidgetCore* ui, WidgetInternalFlag flag , WidgetInternalFlag unhandledFlag , Func func)
{
	bool result = true;
	while( ui != nullptr )
	{
		if( ui->checkFlag(flag | unhandledFlag) )
		{
			if( ui->checkFlag(unhandledFlag) )
			{
				result = func(ui);
				if( !result )
					break;
			}

			ui = ui->getParent();
		}
		else
		{
			result = func(ui);
			break;
		}
	}

	return result;
}

template< class T >
bool TWidgetManager<T>::procCharMsg( unsigned code )
{
	//System maybe send callback
	if (mbProcessingMsg)
		return false;
	ProcMsgScope scope(this);
	bool result = processMessage(getKeyInputWidget(), WIF_REROUTE_CHAR_EVENT, WIF_REROUTE_CHAR_EVENT_UNHANDLED, [code](WidgetCore* ui)
	{
		return ui->onCharMsg(code);
	});
	return result;
}

template< class T >
bool TWidgetManager<T>::procKeyMsg(KeyMsg const& msg)
{
	//System maybe send callback
	if (mbProcessingMsg)
		return false;

	ProcMsgScope scope(this);
	bool result = processMessage(getKeyInputWidget(), WIF_REROUTE_KEY_EVENT, WIF_REROUTE_KEY_EVENT_UNHANDLED, [msg](WidgetCore* ui)
	{
		return ui->onKeyMsg(msg);
	});
	return result;
}

template< class T >
T* TWidgetManager<T>::hitTest( Vec2i const& testPos )
{
	WIDGET_PROFILE_ENTRY("UI Hit Test");
	return static_cast< T*>( WidgetCore::hitTestInternal( testPos , mTopWidgetList ) );
}

template< class T >
void TWidgetManager<T>::update()
{
	WIDGET_PROFILE_ENTRY("UI System");

	{
		WIDGET_PROFILE_ENTRY("UpdateUI");

		for( auto ui = createTopWidgetIterator(); ui; ++ui )
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
	for( auto ui = createTopWidgetIterator(); ui; ++ui )
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
	WIDGET_PROFILE_ENTRY("UI System");
	{
		WIDGET_PROFILE_ENTRY("visitUI");

		for( auto ui = createTopWidgetIterator(); ui; ++ui )
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
	if ( ui == nullptr || ui->checkFlag( WIF_MARK_DESTROY ) )
		return;

	ui->addFlagInternal(WIF_MARK_DESTROY);
	if ( mbProcessingMsg || ui->checkFlag( WIF_DEFERRED_DESTROY ))
	{
		ui->unlinkInternal(false);
		mDeferredDestroyWidgets.push_back(*static_cast<T*>(ui));
	}
	else
	{
		destroyWidgetActually( ui );
	}
}

template < class T >
void TWidgetManager<T>::destroyWidgetChildren_R(WidgetCore* ui)
{
	auto childIter = ui->createChildrenIterator();
	while( childIter )
	{
		WidgetCore* child = &(*childIter);
		destroyWidgetChildren_R(child);
		++childIter;
		destroyWidget(child);
	}
}


template< class T >
void TWidgetManager<T>::destroyWidgetActually( WidgetCore* ui )
{
	removeWidgetReference( ui );
	destroyWidgetChildren_R( ui );

	ui->unlinkInternal(true);
	ui->deleteThis();
}

template< class T >
void TWidgetManager<T>::cleanupPaddingKillWidgets()
{
	int loopCount = 0;

	while( !mDeferredDestroyWidgets.empty() )
	{
		WidgetList processingList;
		processingList.swap(mDeferredDestroyWidgets);

		while( !processingList.empty() )
		{
			WidgetCore* ui = &processingList.front();
			ui->removeFlagInternal(WIF_MARK_DESTROY);
			destroyWidget(ui);
		}

		++loopCount;
		if( loopCount > 10 )
		{
			LogWarning(0, "Can't Destroy Padding Widgets!!");
			break;
		}
	}
}

template< class T >
void TWidgetManager<T>::cleanupWidgets(bool bGlobalIncluded )
{
	if( bGlobalIncluded )
	{
		while( !mTopWidgetList.empty() )
		{
			destroyWidget(&mTopWidgetList.front());
		}
	}
	else
	{
		for( auto iter = mTopWidgetList.begin(); iter != mTopWidgetList.end();  )
		{
			WidgetCore& widget = *iter;
			++iter;
			if( !widget.checkFlag(WIF_GLOBAL) )
			{
				destroyWidget(&widget);
			}
		}
	}
}

template< class T >
void TWidgetManager<T>::addWidget( WidgetCore* ui )
{
	mTopWidgetList.push_back(*static_cast<T*>(ui));
	ui->setManager(this);
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
	return ui->getParent() == nullptr;
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

#endif // WidgetCore_H_0D21F7C5_C362_437A_8BC5_D4C98EDB3024
