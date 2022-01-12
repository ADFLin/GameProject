#include "TinyGamePCH.h"
#include "GameGUISystem.h"

#include "RenderUtility.h"
#include "RHI/RHIGraphics2D.h"

#include "StageBase.h"
#include "TaskBase.h"

GUISystem::GUISystem()
{
	mGuiDelegate  = nullptr;
	mHideWidgets  = false;
	mbSkipCharEvent = false;
	mbSkipKeyEvent  = false;
	mbSkipMouseEvent = false;
}

bool GUISystem::initialize(IGUIDelegate&  guiDelegate)
{
	mGuiDelegate = &guiDelegate;
	return true;
}

void GUISystem::finalize()
{

}

Vec2i GUISystem::calcScreenCenterPos( Vec2i const& size )
{
	Vec2i pos;
	pos.x = ( Global::GetScreenSize().x - size.x ) / 2;
	pos.y = ( Global::GetScreenSize().y - size.y ) / 2;
	return pos;
}

void GUISystem::addTask( TaskBase* task , bool beGlobal /*= false */ )
{
	mGuiDelegate->addGUITask(task, beGlobal);
}

void GUISystem::sendMessage( int event , int id , GWidget* ui )
{
	mGuiDelegate->dispatchWidgetEvent(event, id, ui);
}

void GUISystem::removeHotkey( GWidget* ui )
{
	for( HotkeyList::iterator iter = mHotKeys.begin();
		iter != mHotKeys.end() ; )
	{
		if ( iter->ui == ui )
		{
			iter = mHotKeys.erase( iter );
		}
		else
		{
			++iter;
		}
	}
}

void GUISystem::addHotkey( GWidget* ui , ControlAction key )
{
	HotkeyInfo info;
	info.ui  = ui;
	info.keyAction = key;
	mHotKeys.push_back( info );
}

void GUISystem::scanHotkey( InputControl& inputConrol )
{
	//for ( HotkeyList::iterator iter = mHotKeys.begin();
	//	iter != mHotKeys.end() ; ++iter )
	//{
	//	iter->ui->markDestroyDefferred();
	//	if ( inputConrol.checkKey( 0 , iter->keyAction ) )
	//	{
	//		iter->ui->onHotkey( iter->keyAction );
	//	}
	//}

	//mUIManager.cleanupDestroyUI();

	//for ( HotkeyList::iterator iter = mHotKeys.begin();
	//	iter != mHotKeys.end() ; ++iter )
	//{
	//	iter->ui->unmarkDestroyDefferred();
	//}
}

GWidget* GUISystem::showMessageBox( int id , char const* msg , EMessageButton::Type buttonType)
{
	Vec2i pos = calcScreenCenterPos( GMsgBox::BoxSize );
	GMsgBox* box = new GMsgBox( id , pos , NULL , buttonType);
	box->setTitle( msg );
	box->setAlpha( 0.8f );
	addWidget( box );
	box->doModal();

	return box;
}

int GUISystem::getModalId()
{
	GWidget* ui = getManager().getModalWidget();
	if ( ui )
		return ui->getID();
	return UI_ANY;
}

void GUISystem::updateFrame( int frame , long tickTime )
{
	struct UpdateFrameVisitor
	{
		void operator()( GWidget* w )
		{
			w->updateFrame( frame );
		}
		int frame;
	} visitor;

	visitor.frame = frame;
	getManager().visitWidgets( visitor );
	mTweener.update( frame * tickTime );
}

MsgReply GUISystem::procMouseMsg(MouseMsg const& msg)
{
	if( mHideWidgets || mbSkipMouseEvent )
		return MsgReply::Unhandled();
	return getManager().procMouseMsg(msg);
}

MsgReply GUISystem::procKeyMsg(KeyMsg const& msg)
{
	if( mHideWidgets || mbSkipKeyEvent )
		return MsgReply::Unhandled();
	return getManager().procKeyMsg(msg);
}

MsgReply GUISystem::procCharMsg(unsigned code)
{
	if( mHideWidgets || mbSkipCharEvent )
		return MsgReply::Unhandled();
	return getManager().procCharMsg(code);
}

void GUISystem::addWidget( GWidget* widget )
{
	mUIManager.addWidget( widget );
}

void GUISystem::cleanupWidget(bool bForceCleanup , bool bPersistentIncluded)
{
	mUIManager.cleanupWidgets(bPersistentIncluded);
	mTweener.clear();
	if( bForceCleanup )
		mUIManager.cleanupPaddingKillWidgets();
}

void GUISystem::render()
{
	if ( mHideWidgets )
		return;

	mUIManager.render();
}

void GUISystem::update()
{
	mUIManager.update();
}

GWidget* GUISystem::findTopWidget(int id)
{
	for( auto child = mUIManager.createTopWidgetIterator() ; child ; ++child )
	{
		if ( child->getID() == id )
			return &(*child);
	}
	return nullptr;
}

