#include "TinyGamePCH.h"
#include "GameGUISystem.h"

#include "RenderUtility.h"
#include "GLGraphics2D.h"

#include "StageBase.h"
#include "TaskBase.h"

GUISystem::GUISystem()
{
	mStageManager = NULL;
	mSkipRender   = false;
}

bool GUISystem::init( StageManager* stageManager )
{
	mStageManager = stageManager;
	return true;
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
	TaskHandler* handler = ( beGlobal ) ? static_cast< TaskHandler* >( mStageManager ) : mStageManager->getCurStage();
	handler->addTask( task );
}

void GUISystem::sendMessage( int event , int id , GWidget* ui )
{
	if ( !mStageManager->getCurStage()->onEvent( event , id , ui ) )
		return ;
	if ( id > UI_STAGE_ID )
		return;
	mStageManager->onEvent( event , id , ui );
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
	box->setAlpha( 0.9f );
	addWidget( box );
	box->doModal();

	return box;
}

int GUISystem::getModalID()
{
	GWidget* ui = getManager().getModalUI();
	if ( ui )
		return ui->getID();
	return UI_ANY;
}

UserProfile& GUISystem::getUserProfile()
{
	return mStageManager->getUserProfile();
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
	getManager().visitUI( visitor );
	mTweener.update( frame * tickTime );
}

void GUISystem::addWidget( GWidget* widget )
{
	mUIManager.addUI( widget );
}

void GUISystem::cleanupWidget()
{
	mUIManager.cleanupUI();
	mTweener.clear();
}

void GUISystem::render()
{
	if ( mSkipRender )
		return;

	mUIManager.render();

}

void GUISystem::update()
{
	mUIManager.update();
}

GWidget* GUISystem::findTopWidget(int id , GWidget* start /*= NULL */)
{
	GUI::Core* root = mUIManager.getRoot();
	GWidget* child = ( start ) ? root->nextChild( start ) : root->getChild();
	while( child )
	{
		if ( child->getID() == id )
			return child;
		child = root->nextChild( child );
	}
	return NULL;
}

