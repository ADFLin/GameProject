#pragma once
#ifndef WidgetCore_H_0D21F7C5_C362_437A_8BC5_D4C98EDB3024
#define WidgetCore_H_0D21F7C5_C362_437A_8BC5_D4C98EDB3024

#include "WidgetCore.h"
#include "LogSystem.h"
#include <cassert>

template< class T >
WidgetCoreT<T>::WidgetCoreT( Vec2i const& pos , Vec2i const& size , T* parent ) 
{
	mBoundRect.min = pos;
	mBoundRect.max = pos + size;
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
	mParent = nullptr;
	mManager = nullptr;
	mFlag = 0;
}


template< class T >
WidgetCoreT<T>::~WidgetCoreT()
{
	if ( mParent )
		unlinkInternal( true );
}

template< class T >
WidgetCoreT<T>* WidgetCoreT<T>::HitTestInternal(Vec2i const& testPos , WidgetList& Widgetlist)
{
	WidgetCoreT* result = NULL;

	Vec2i  pos = testPos;
	WidgetList* curList = &Widgetlist;
	auto childiter = curList->rbegin();

	while( childiter != curList->rend() )
	{
		WidgetCoreT* ui = &(*childiter);

		bool testFail = true;
		if( !ui->checkFlag(WIF_DISABLE | WIF_HAD_HIDDEN) )
			// ui->isEnable() && ui->isShow()
		{
			if( ui->checkFlag(WIF_HITTEST_CHILDREN) )
			{
				Vec2i tPos = pos - ui->getPos();
				result = HitTestInternal( tPos , ui->mChildren );
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
	return HitTestInternal(testPos, mChildren);
}


template< class T >
void WidgetCoreT<T>::unlinkInternal( bool bRemove )
{
	mLinkHook.unlink();

	if ( !isTop() )
		static_cast< T* >( mParent )->onChangeChildrenOrder();

	if( mParent )
	{
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
bool WidgetCoreT<T>::isSubWidgetOf(WidgetCoreT* ui) const
{
	WidgetCoreT const* curWidget = this;
	do
	{
		if (curWidget->mParent == ui)
			return true;

		curWidget = curWidget->mParent;

	}
	while (curWidget);
	return false;
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
		enableFlag(WIF_HAD_HIDDEN, !beS);
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
	return !checkFlag(WIF_HAD_HIDDEN);
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
	//#TODO
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
T& WidgetCoreT<T>::clearFocus(bool bSubWidgetsIncluded)
{
	getManager()->clearFocusWidget(this, bSubWidgetsIncluded);
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
		_this()->onDestroy();
		destroyChildren_R();
		deleteThis();
	}
}

template< class T >
void WidgetCoreT<T>::destroyChildren_R()
{
	auto iterChildren = createChildrenIterator();

	while( iterChildren )
	{
		WidgetCoreT* child = &(*iterChildren);
		static_cast<T*>(child)->onDestroy();
		child->destroyChildren_R();
		++iterChildren;
		child->deleteThis();
	}
}

template < typename T>
TWidgetManager<T>::TWidgetManager()
	:mbProcessingMsg(false)
{
	std::fill_n(mNamedSlots, (int)ESlotName::Num, nullptr);
}

template < typename T>
TWidgetManager<T>::~TWidgetManager()
{
	cleanupWidgets();
	cleanupPaddingKillWidgets();
}

template < typename T>
void TWidgetManager<T>::prevProcMsg()
{
	assert(mbProcessingMsg == false);
	mbProcessingMsg = true;
}

template < typename T>
void TWidgetManager<T>::postProcMsg()
{
	mbProcessingMsg = false;
	cleanupPaddingKillWidgets();
}

template < typename T>
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

template < typename T>
template< class TFunc >
MsgReply TWidgetManager<T>::processMessage(WidgetCore* ui, WidgetInternalFlag flag, WidgetInternalFlag unhandledFlag, TFunc func)
{
	MsgReply reply(false);
	while (ui != nullptr)
	{
		if (ui->checkFlag(flag | unhandledFlag))
		{
			if (ui->checkFlag(unhandledFlag))
			{
				reply = func(ui);
				if (reply.isHandled())
					break;
			}

			ui = ui->getParent();
		}
		else
		{
			reply = func(ui);
			break;
		}
	}

	return reply;
}

template < typename T>
MsgReply TWidgetManager<T>::procCharMsg( unsigned code )
{
	//System maybe send callback
	if (mbProcessingMsg)
		return MsgReply::Handled();

	ProcMsgScope scope(this);
	MsgReply reply = processMessage(getKeyInputWidget(), WIF_REROUTE_CHAR_EVENT, WIF_REROUTE_CHAR_EVENT_UNHANDLED, [code](WidgetCore* ui)
	{
		return ui->onCharMsg(code);
	});
	return reply;
}

template < typename T>
MsgReply TWidgetManager<T>::procKeyMsg(KeyMsg const& msg)
{
	//System maybe send callback
	if (mbProcessingMsg)
		return MsgReply::Handled();

	ProcMsgScope scope(this);
	MsgReply reply = processMessage(getKeyInputWidget(), WIF_REROUTE_KEY_EVENT, WIF_REROUTE_KEY_EVENT_UNHANDLED, [msg](WidgetCore* ui)
	{
		return ui->onKeyMsg(msg);
	});
	return reply;
}

template < typename T>
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

template < typename T>
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

template < typename T>
void TWidgetManager<T>::render()
{
	for( auto ui = createTopWidgetIterator(); ui; ++ui )
	{
		if ( !ui->isShow() )
			continue;
		ui->renderAll();
	}
}


template < typename T >
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

template < typename T>
template< class Visitor >
void  TWidgetManager<T>::visitWidgetInternal( Visitor visitor , WidgetCore& ui )
{
	visitor(static_cast<T*>(&ui));
	for( auto child = ui.createChildrenIterator(); child; ++child )
	{
		visitWidgetInternal(visitor, *child);
	}
}

template < typename T>
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

template < typename T>
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

template < typename T>
void TWidgetManager<T>::destroyWidgetChildren_R(WidgetCore* ui)
{
	auto childIter = ui->createChildrenIterator();
	while( childIter )
	{
		WidgetCore* child = &(*childIter);
		static_cast<T*>(child)->onDestroy();
		destroyWidgetChildren_R(child);
		++childIter;
		destroyWidget(child);
	}
}


template < typename T>
void TWidgetManager<T>::destroyWidgetActually( WidgetCore* ui )
{
	static_cast<T*>(ui)->onDestroy();
	removeWidgetReference( ui );
	destroyWidgetChildren_R( ui );
	ui->unlinkInternal(true);
	ui->deleteThis();
}

template < typename T>
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

template < typename T>
void TWidgetManager<T>::cleanupWidgets(bool bPersistentIncluded)
{
	if(bPersistentIncluded)
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
			if( !widget.checkFlag(WIF_PERSISTENT) )
			{
				destroyWidget(&widget);
			}
		}
	}
}

template < typename T>
void TWidgetManager<T>::addWidget( WidgetCore* ui )
{
	mTopWidgetList.push_back(*static_cast<T*>(ui));
	ui->setManager(this);
}

template < typename T>
void TWidgetManager<T>::startModal( WidgetCore* ui )
{
	if ( mNamedSlots[ESlotName::Modal] )
		return;
	setNamedSlot(ESlotName::Modal, *ui);
}

template < typename T>
void TWidgetManager<T>::endModal( WidgetCore* ui )
{
	if( ui == mNamedSlots[ESlotName::Modal] )
		clearNamedSlot(ESlotName::Modal);
}

template < typename T>
bool TWidgetManager<T>::isTopWidget( WidgetCore* ui )
{
	return ui->getParent() == nullptr;
}

template < typename T>
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

template < typename T>
void TWidgetManager<T>::clearFocusWidget(WidgetCore* ui, bool bSubWidgetsIncluded)
{
	if (mNamedSlots[ESlotName::Focus] == nullptr)
		return;

	if (ui == mNamedSlots[ESlotName::Focus])
	{
		clearNamedSlot(ESlotName::Focus);
		ui->focus(false);
	}
	else if (bSubWidgetsIncluded && mNamedSlots[ESlotName::Focus]->isSubWidgetOf(ui))
	{
		ui = mNamedSlots[ESlotName::Focus];
		clearNamedSlot(ESlotName::Focus);
		ui->focus(false);
	}
}

template < typename T, typename TWidgetImpl >
MsgReply WidgetManagerT<T, TWidgetImpl>::procMouseMsg( MouseMsg const& msg )
{
	//System maybe send callback
	if (mbProcessingMsg)
		return MsgReply::Handled();

	WIDGET_PROFILE_ENTRY("UI System");

	ProcMsgScope scope(this);

	MsgReply result = MsgReply::Unhandled();

	mLastMouseMsg = msg;
	Vec2i worldPos;

	WidgetCore* ui = nullptr;
	if ( mNamedSlots[ESlotName::Capture] )
	{
		ui = mNamedSlots[ESlotName::Capture];
		worldPos = _this()->transformToWorld((TWidgetImpl*)ui, mLastMouseMsg.getPos());
	}
	else if( mNamedSlots[ESlotName::Modal] )
	{
		worldPos = _this()->transformToWorld((TWidgetImpl*)mNamedSlots[ESlotName::Modal], mLastMouseMsg.getPos());
		if( mNamedSlots[ESlotName::Modal]->hitTest(worldPos) )
		{
			ui = mNamedSlots[ESlotName::Modal]->hitTestChildren(worldPos - mNamedSlots[ESlotName::Modal]->getPos());
		}		
	}
	else if (mLastMouseMsg.getMsg() & MBS_WHEEL)
	{
		ui = mNamedSlots[ESlotName::Focus];
		if (ui)
		{
			worldPos = _this()->transformToWorld((TWidgetImpl*)ui, mLastMouseMsg.getPos());
		}
	}
	else 
	{	
		ui = hitWidgetTest( mLastMouseMsg.getPos(), worldPos);
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
				result = MsgReply(mNamedSlots[ESlotName::Focus] != NULL) ;
			}
			else
			{
				mLastMouseMsg.setPos(worldPos);
				bool bSetSlot = true;
				while (ui)
				{
					if (!ui->checkFlag(WIF_REROUTE_MOUSE_EVENT))
					{
						result = ui->onMouseMsg(mLastMouseMsg);

						if (mNamedSlots[ESlotName::LastMouseMsg])
							clearNamedSlot(ESlotName::LastMouseMsg);

						setNamedSlot(ESlotName::LastMouseMsg, *ui);

						if (ui->checkFlag(WIF_SEND_PARENT_MOUSE_EVENT))
						{
							ui = static_cast<TWidgetImpl*>(ui->getParent());
							auto parentResult = ui->onMouseMsg(mLastMouseMsg);
							if (parentResult.isHandled())
							{
								result.bHandled = true;
							}
						}

						if (result.isHandled() || !ui->checkFlag(WIF_REROUTE_MOUSE_EVENT_UNHANDLED))
						{
							break;
						}
					}

					ui = static_cast<TWidgetImpl*>(ui->getParent());
				}

			}
		}
	}

	return result;
}

template < typename T, typename TWidgetImpl >
TWidgetImpl* WidgetManagerT<T, TWidgetImpl>::hitWidgetTest( Vec2i const& testPos, Vec2i& outWorldPos)
{
	WIDGET_PROFILE_ENTRY("UI Hit Test")

	WidgetCore* result = NULL;

	for (auto it = createTopWidgetIterator(); it; ++it)
	{
		auto ui = &(*it);

		if (ui->checkFlag(WIF_DISABLE | WIF_HAD_HIDDEN))
			continue;

		Vec2i pos = _this()->transformToWorld(ui, testPos);

		if (ui->checkFlag(WIF_HITTEST_CHILDREN))
		{
			Vec2i tPos = pos - ui->getPos();
			result = WidgetCore::HitTestInternal(tPos, ui->mChildren);
			if (result)
			{
				outWorldPos = pos;
				break;
			}
		}
		else if (static_cast<TWidgetImpl*>(ui)->doHitTest(pos))
		{
			result = ui;
			outWorldPos = pos;
			Vec2i tPos = pos - ui->getPos();
			auto childHit = WidgetCore::HitTestInternal(tPos, ui->mChildren);
			if (childHit)
			{
				result = childHit;
			}
			break;
		}
	}

	return static_cast<TWidgetImpl*>(result);
}

#endif // WidgetCore_H_0D21F7C5_C362_437A_8BC5_D4C98EDB3024
