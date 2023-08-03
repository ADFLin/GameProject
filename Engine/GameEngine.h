#ifndef GameEngine_h__
#define GameEngine_h__



class Stage
{
public:
	virtual ~Stage(){}

	virtual void onEnter( StageEvent& event );
	virtual void onExit( StageEvent& event );
	virtual void onUpdate( long time );


private:
	void update( long time )
	{
		onUpdate( time );
	}
};

class StageEvent
{
	virtual ~StageEvent(){}
};






class IStageManage
{
public:
	IStageManage()
	{
		mNextStage = NULL;
		mCurStage  = NULL;
	}

	void  manageStage( long time )
	{
		if ( mNextStage )
		{
			mCurStage->onExit( mNextStage );

			mCurStage = mNextStage;
			mNextStage = NULL;

			mNextStage->onEnter();
		}
	}

private:
	Stage* mNextStage;
	Stage* mCurStage;
};

template < class Impl >
class StageDesigner : public IStageManage
{
	void setupStageFunc( Stage& newStage , StageEvent& event ){}


	class IRule
	{
		void doSetupFunc( Impl& impl , Stage& newStage , StageEvent& event ) = 0;
	};

	template < class SrcStage , class DstStage , class Event , void (Impl::*setupFunc)( event& ) >
	class Rule : public IRule
	{
		void changeStage( Impl& impl)
		{

		}
		void doSetupFunc( Impl& impl , Stage& newStage , StageEvent& event )
		{
			(impl.*setupFunc)( newStage , event );
		}
	};



	Stage* mStageMap[];
};


template< class PlatformPolicy >
class Root
{
public:
	GuiSystem*    getGuiSystem();
	RenderSystem* getRenderSystem();
	Game*         getGame();
}
#include <list>
class Game
{
public:

	bool init()
	{

	}

	long updateLoop( unsigned time )
	{
		int numFrame = time / getUpdateRate();
		long updateTime = numFrame * getUpdateRate();

		for( SystemList::iterator iter = mSystemList.begin(),
			iterEnd = mSystemList.end();
			iter != iterEnd ; ++iter )
		{
			System* system = *iter;
			system->update( updateTime );
		}

		onUpdate( updateTime );
		//StageManagePolicy::manageStage( time );
		getCurStage()->update( updateTime );


		return updateTime;
	}

	void  render()
	{

	}

	long       getUpdateRate();

	Stage*     getCurStage();

/////////////////////////////
	bool onInit();
	void onUpdate( long time );
/////////////////////////////


	typedef std::list< System* > SystemList;
	SystemList mSystemList;

};

class Input
{

};

class System
{
public:
	bool onInit();
	void onTick();
	void onUpdate( long time );
	void setTickUpdateTime( long time ){ mTimeTick = time; }
private:
	void update( long time )
	{
		mTimer += time;
		if ( mTimer > mTimeTick )
		{
			onTick();
			mTimer -= mTimeTick;
		}
		onUpdate( time );
	}

	long mTimeTick;
	long mTimer;

	friend class Game;
};


class EventSystem ; public System
{

};

class Entity
{

};

class Engine
{

};

class RenderSystem : public System
{

	void preRender();
	void postRender();

};


class GuiSystem : public System
{

};

class Scene
{





};


extern int main( int argc , char* argv[] );

#endif // GameEngine_h__