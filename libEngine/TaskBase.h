#ifndef TaskBase_h__
#define TaskBase_h__

#include <list>
#include <cassert>
#include "EasingFunction.h"

class TaskBase;

#ifndef BIT
#define BIT( n ) ( 1 << ( n ) )
#endif 

enum
{
	TF_STEP_START   = BIT(0),
	TF_STEP_END     = BIT(1),
	TF_STEP_DESTROY = BIT(2),

	TF_STEP_MASK    = BIT(5) - 1,

	TF_HAVE_NEXT_TASK = BIT(6),
	TF_NOT_COMPLETE   = BIT(7),
	TF_NEVER_START    = BIT(8),
};

class TaskMsg
{
public:
	TaskMsg( int inGroup , unsigned inFlag ):groupId(inGroup),flag(inFlag){}
	int   getGroup()     const { return groupId; }
	bool  onStart()   const { return ( flag & TF_STEP_MASK ) == TF_STEP_START; }
	bool  onEnd()     const { return ( flag & TF_STEP_MASK ) == TF_STEP_END; }
	bool  onDestroy() const { return ( flag & TF_STEP_MASK ) == TF_STEP_DESTROY; }
	bool  checkFlag( unsigned bit ) const { return ( flag & bit ) != 0; }
private:
	int      groupId;
	unsigned flag;
};

#define  DEFAULT_TASK_GROUP 0

class TaskListener
{
public:
	virtual ~TaskListener(){}
	virtual void onTaskMessage( TaskBase* task , TaskMsg const& msg ){};
};

enum TaskUpdatePolicy
{
	TUP_HANDLER_DEFAULT = 0 ,
	TUP_SYSTEM_MASK     = BIT(0) ,
	TUP_GAME_MASK       = BIT(1) ,
	TUP_LEVEL_MASK      = BIT(2) ,
	TUP_ALL_MASK        = -1 ,
};

class TaskHandler
{
public:
	TaskHandler() { setDefaultUpdatePolicy( TUP_SYSTEM_MASK ); }
	~TaskHandler(){ clearTask(); }

	void    clearTask();
	void    runTask( long time , unsigned updateMask = TUP_ALL_MASK );
	void    addTask( TaskBase* task , TaskListener* listener = NULL , int taskGroup = DEFAULT_TASK_GROUP );
	void    removeTask( int id );
	bool    removeTask( TaskBase* task , bool beAll = false );
	void    setDefaultUpdatePolicy( TaskUpdatePolicy policy ){ mDefaultPolicy = policy; }

protected:

	struct TaskNode
	{
		int           groupId;
		TaskBase*     task;
		TaskListener* listenser;
	};

	void   startNode( TaskNode& node );

	typedef std::list< TaskNode > TaskList;
	bool    removeTaskInternal( TaskList::iterator iter , bool beAll );
	void    sendMessage( TaskNode const& node , unsigned flag );
	
	TaskList         mRunList;
	TaskUpdatePolicy mDefaultPolicy;

};



class TaskBase
{
public:
	TaskBase();
	virtual ~TaskBase(){}

	void   skip() {  if ( mHandler ) mHandler->removeTask( this , false ); }
	void   stop() {  if ( mHandler ) mHandler->removeTask( this , true );  }

	TaskHandler* getHandler(){ return mHandler; }
	TaskBase*    setNextTask( TaskBase* task );
	void         setUpdatePolicy( TaskUpdatePolicy policy ){ mUpdatePolicy = policy; }
public:
	virtual void onStart(){}
	virtual bool onUpdate( long time ){ return false; }
	virtual void onEnd( bool beComplete ){}
	virtual void release() = 0;

private:
	bool   update( long time );
	long             mDelay;
	TaskUpdatePolicy mUpdatePolicy;
	TaskBase*        mNextTask;
	TaskHandler*     mHandler;
	friend class TaskHandler;
	friend class ParallelTask;
};

class LifeTimeTask : public TaskBase
{
public:
	LifeTimeTask( long lifeTime )
		:mLifeTime( lifeTime ){}

	void  release(){ delete this; }
	bool  onUpdate( long time )
	{
		mLifeTime -= time;
		return  mLifeTime > 0;
	}

	void   setLiftTime( long time ){ mLifeTime = time ; }
	long   getLifeTime(){ return mLifeTime; }
	

private:
	long   mLifeTime;	
};

typedef LifeTimeTask DelayTask;

class ParallelTask  : public TaskBase
	                , public TaskHandler
{
public:
	ParallelTask(){}
	~ParallelTask()
	{
		clearTask();
	}

	bool onUpdate( long time );
	void release(){ delete this; }
};


struct CVFade
{
	template < class T > inline
	static void changeValue( T& val , T const& from , T const& to , long lifeTime , long totalTime )
	{
		val = from + ( to - from ) * ( totalTime - lifeTime ) / totalTime ;
	}
};


struct CVFloor
{
	template < class T > inline
	static void changeValue( T& val , T const& from , T const& to , long lifeTime , long totalTime )
	{
		val = from;
	}
};

template < class T , class ModifyPolicy = CVFade >
class ValueModifyTask : public LifeTimeTask
{
public:
	ValueModifyTask( T& val , T const& from , T const& to  , long time )
		:LifeTimeTask( time ),val(val),from(from),to(to){}

	void release(){ delete this; }
	void onStart(){ val = from; totalTime = getLifeTime(); }
	void onEnd( bool beComplete ){ if ( beComplete ) val = to; }
	bool onUpdate( long time )
	{ 
		bool result = LifeTimeTask::onUpdate( time );
		ModifyPolicy::changeValue( val , from , to , getLifeTime() , totalTime ); 
		return result; 
	}

	long totalTime;
	T  diff ,from , to;
	T& val;
};

#include <functional>
class DelegateTask : public TaskBase
{
public:
	typedef std::function< bool(long) > DelegateFun;

	template< class FunType >
	DelegateTask(FunType fun)
		:mFun(fun)
	{

	}
	void release() { delete this; }
	bool onUpdate(long time) { return mFun( time ); }
	DelegateFun mFun;
};

template < class T , class MemFun >
class MemberFunTask : public TaskBase
{
	MemFun mFun;
	T*     mPtr;
public:
	MemberFunTask( T* ptr , MemFun fun )
		:mPtr(ptr), mFun( fun ){}
	void release(){ delete this; }
	bool onUpdate( long time ){ return (mPtr->*mFun)( time ); }
};


class TaskUtility
{
public:
	template < class T , class CV >
	static ValueModifyTask< T , CV >* ChangeVal( T& val , T const& from , T const& to , long time )
	{
		return new ValueModifyTask< T , CV >( val , from , to , time );
	}
	template < class T, class MemFun >
	static MemberFunTask< T , MemFun >* MemberFun( T* ptr , MemFun fun )
	{
		return new MemberFunTask< T , MemFun >( ptr , fun );
	}

	template < class T, class EasingFun >
	static TaskBase* ValueEasing( T& val , T const& from, T const& to , long time )
	{
		return nullptr;
	}

	template < class FunType >
	static DelegateTask* DelegateFun( FunType fun )
	{
		return new DelegateTask(fun);
	}
};

#endif // TaskBase_h__
