#ifndef GameMenuStage_h__
#define GameMenuStage_h__

#include "StageBase.h"
#include "DrawEngine.h"

#include "SysMsg.h" // TVec2D
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
	bool onMouse( MouseMsg const& msg );


	void changeGroup( StageGroupID group )
	{
		mCurGroup = group;
		doChangeGroup( group );
	}

	void pushGroup( StageGroupID group )
	{
		mGroupStack.push_back( mCurGroup );
		mCurGroup = group;
		doChangeGroup( group );
		
	}
	void popGroup()
	{
		if ( mGroupStack.empty() )
			return;
		mCurGroup = mGroupStack.back();
		mGroupStack.pop_back();
		doChangeGroup( mCurGroup );
	}

	virtual void doChangeGroup( StageGroupID group ){}

	void     fadeoutGroup( int dDelay );
	GWidget* createButton( int delay , int id , char const* title , Vec2i const& pos , Vec2i const& size );

	void onTaskMessage( TaskBase* task , TaskMsg const& msg );

protected:
	typedef std::list< TaskBase* > TaskList;
	typedef std::list< GWidget* > UIList;

	TaskList      mSkipTasks;
	UIList        mGroupUI;
	StageGroupID  mCurGroup;
	std::vector< StageGroupID > mGroupStack;


};


#endif // GameMenuStage_h__
