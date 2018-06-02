#include "TinyGamePCH.h"
#include "GameGUISystem.h"

#include "RenderUtility.h"
#include "GLGraphics2D.h"

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
	pos.x = ( Global::getDrawEngine()->getScreenWidth() - size.x ) / 2;
	pos.y = ( Global::getDrawEngine()->getScreenHeight() - size.y ) / 2;
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

void GUISystem::scanHotkey( GameController& controller )
{
	//for ( HotkeyList::iterator iter = mHotKeys.begin();
	//	iter != mHotKeys.end() ; ++iter )
	//{
	//	iter->ui->lockDestroy();
	//	if ( controller.checkKey( 0 , iter->keyAction ) )
	//	{
	//		iter->ui->onHotkey( iter->keyAction );
	//	}
	//}

	//mUIManager.cleanupDestroyUI();

	//for ( HotkeyList::iterator iter = mHotKeys.begin();
	//	iter != mHotKeys.end() ; ++iter )
	//{
	//	iter->ui->unlockDestroy();
	//}
}

GWidget* GUISystem::showMessageBox( int id , char const* msg , unsigned flag /*= GMB_YESNO */ )
{
	Vec2i pos = calcScreenCenterPos( GMsgBox::BoxSize );
	GMsgBox* box = new GMsgBox( id , pos , NULL , flag );
	box->setTitle( msg );
	box->setAlpha( 0.8f );
	addWidget( box );
	box->doModal();

	return box;
}

int GUISystem::getModalID()
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

bool GUISystem::procMouseMsg(MouseMsg const& msg)
{
	if( mHideWidgets || mbSkipMouseEvent )
		return true;
	return getManager().procMouseMsg(msg);
}

bool GUISystem::procKeyMsg(unsigned key, bool isDown)
{
	if( mHideWidgets || mbSkipKeyEvent )
		return true;
	return getManager().procKeyMsg(key, isDown);
}

bool GUISystem::procCharMsg(unsigned code)
{
	if( mHideWidgets || mbSkipCharEvent )
		return true;
	return getManager().procCharMsg(code);
}

void GUISystem::addWidget( GWidget* widget )
{
	mUIManager.addWidget( widget );
}

void GUISystem::cleanupWidget(bool bForceCleanup)
{
	mUIManager.cleanupWidgets();
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

