#include "TinyGamePCH.h"
#include "GameMenuStage.h"

#include "GameGUISystem.h"
#include "TaskBase.h"

class DestroyUITask : public TaskBase
{
public:
	DestroyUITask( GWidget* ui ):ui( ui ){}
	virtual void onStart(){ ui->destroy(); }
	virtual bool onUpdate( long time ){ return false; }
	void release(){ delete this; }
	GWidget* ui;
};

GameMenuStage::GameMenuStage()
{

}

bool GameMenuStage::onInit()
{
	return true;
}

GWidget* GameMenuStage::createButton( int delay , int id , char const* title , Vec2i const& pos , Vec2i const& size )
{
	GButton* button;
	button = new GButton( id , Vec2i( -150 , pos.y ), size , NULL );
	button->setTitle( title );

	::Global::GUI().addWidget( button );

	TaskBase* task = new DelayTask( delay );
	TaskBase* task2 = new UIMotionTask( button , pos , 200 , WMT_LINE_MOTION );
	task->setNextTask( task2 );
	addTask( task , this );

	mSkipTasks.push_back( task );
	mSkipTasks.push_back( task2 );

	mGroupUI.push_back( button );
	return button;
}


MsgReply GameMenuStage::onMouse( MouseMsg const& msg )
{
	if ( msg.onLeftDown() &&
		::Global::GUI().getManager().getLastMouseMsgWidget() == NULL )
	{
		for( TaskList::iterator iter = mSkipTasks.begin();
			iter != mSkipTasks.end() ; ++iter )
		{
			TaskBase* task = *iter;
			task->skip();
		}
		mSkipTasks.clear();
	}

	return BaseClass::onMouse(msg);
}


MsgReply GameMenuStage::onKey(KeyMsg const& msg)
{
	if (msg.isDown())
	{
		switch (msg.getCode())
		{
		case EKeyCode::Escape:
			popWidgetGroup();
			return MsgReply::Handled();
		}
	}

	return BaseClass::onKey(msg);
}

void GameMenuStage::fadeoutGroup( int dDelay )
{
	int delay = 0;
	for ( UIList::iterator iter = mGroupUI.begin();
		iter != mGroupUI.end() ; ++iter )
	{
		GWidget* ui = *iter;
		Vec2i pos = ui->getWorldPos() + Vec2i( 400 , 0 );

		TaskBase* task  = new DelayTask( delay );
		TaskBase* task2 = new UIMotionTask( ui , pos , 200 , WMT_LINE_MOTION );
		TaskBase* task3 = new DestroyUITask( ui );
		task->setNextTask( task2 );
		task2->setNextTask( task3 );
		addTask( task , this );

		mSkipTasks.push_back( task );
		mSkipTasks.push_back( task2 );
		delay += dDelay;
	}
}

void GameMenuStage::onUpdate(GameTimeSpan deltaTime)
{
	::Global::GUI().updateFrame(long(deltaTime) / gDefaultTickTime , gDefaultTickTime );
}

void GameMenuStage::onTaskMessage( TaskBase* task , TaskMsg const& msg )
{
	if ( msg.onEnd() && !msg.checkFlag( TF_NOT_COMPLETE ) )
	{
		auto iter = std::find(mSkipTasks.begin(), mSkipTasks.end(), task);
		if ( iter != mSkipTasks.end() )
			mSkipTasks.erase( iter );
	}
}
