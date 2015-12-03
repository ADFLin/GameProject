#ifndef TaskBase_h__
#define TaskBase_h__

#include <list>
#include <cassert>

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
	TaskMsg( int _id , unsigned _flag ):id(_id),flag(_flag){}
	int   getID()     const { return id; }
	bool  onStart()   const { return ( flag & TF_STEP_MASK ) == TF_STEP_START; }
	bool  onEnd()     const { return ( flag & TF_STEP_MASK ) == TF_STEP_END; }
	bool  onDestroy() const { return ( flag & TF_STEP_MASK ) == TF_STEP_DESTROY; }
	bool  checkFlag( unsigned bit ) const { return ( flag & bit ) != 0; }
private:
	int      id;
	unsigned flag;
};

#define  DEFAULT_TASK_ID 0

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
	void    addTask( TaskBase* task , TaskListener* listener = NULL , int taskID = DEFAULT_TASK_ID );
	void    removeTask( int id );
	bool    removeTask( TaskBase* task , bool beAll = false );
	void    setDefaultUpdatePolicy( TaskUpdatePolicy policy ){ mDefaultPolicy = policy; }

protected:

	struct TaskNode
	{
		int           id;
		TaskBase*     task;
		TaskListener* listenser;
	};

    struct FindTask
    {
        FindTask( TaskBase* _task): task(_task){}
        bool operator()( TaskNode const& node ) const { return node.task == task; }
        TaskBase* task;
    };

    struct FindTaskByID
    {
        FindTaskByID( int _id): id(_id){}
        bool operator()( TaskNode const& node ) const { return node.id == id; }
        int id;
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


namespace TaskUtility
{
	template < class T , class CV >
	ValueModifyTask< T , CV >* createChangeValTask( T& val , T const& from , T const& to , long time )
	{
		return new ValueModifyTask< T , CV >( val , from , to , time );
	}
	template < class T, class MemFun >
	MemberFunTask< T , MemFun >* createMemberFunTask( T* ptr , MemFun fun )
	{
		return new MemberFunTask< T , MemFun >( ptr , fun );
	}
}
#endif // TaskBase_h__
