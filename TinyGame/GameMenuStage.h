#ifndef GameMenuStage_h__
#define GameMenuStage_h__

#include "StageBase.h"
#include "DrawEngine.h"

#include "SystemMessage.h" // TVec2D
#include <list>

class TaskBase;

typedef unsigned StageGroupID;

class GameMenuStage : public StageBase
	                , public TaskListener
{
	typedef StageBase BaseClass;
public:

	enum
	{
		UI_BACK_GROUP = BaseClass::NEXT_UI_ID ,
		NEXT_UI_ID ,
	};

	GameMenuStage();
	bool onInit();
	void onUpdate( long time );
	bool onMouse( MouseMsg const& msg ) override;
	bool onKey(unsigned key, bool isDown) override;

	void changeWidgetGroup( StageGroupID group )
	{
		mCurGroup = group;
		doChangeWidgetGroup( group );
	}

	void pushWidgetGroup( StageGroupID group )
	{
		if( group == mCurGroup )
			return;

		mGroupStack.push_back( mCurGroup );
		mCurGroup = group;
		doChangeWidgetGroup( group );
		
	}
	void popWidgetGroup()
	{
		if ( mGroupStack.empty() )
			return;
		mCurGroup = mGroupStack.back();
		mGroupStack.pop_back();
		doChangeWidgetGroup( mCurGroup );
	}

	virtual void doChangeWidgetGroup( StageGroupID group ){}

	void     fadeoutGroup( int dDelay );
	GWidget* createButton( int delay , int id , char const* title , Vec2i const& pos , Vec2i const& size );

	void onTaskMessage( TaskBase* task , TaskMsg const& msg );




protected:
	typedef std::vector< TaskBase* > TaskList;
	typedef std::vector< GWidget* > UIList;

	TaskList      mSkipTasks;
	UIList        mGroupUI;
	StageGroupID  mCurGroup;
	std::vector< StageGroupID > mGroupStack;


};


#endif // GameMenuStage_h__
