#ifndef CNPCBase_h__
#define CNPCBase_h__

#include "common.h"
#include "CActor.h"
#include "AICondition.h"

#include "ConsoleSystem.h"

extern ConVar< float > npc_dead_body_save_time;
extern ConVar< float > npc_chest_vanish_time;

class TNavigator;


class ScriptComponent
{

};


enum NPCAIState
{
	NPC_COMBAT = 0,
	NPC_IDLE ,
	NPC_ALERT,
	NPC_DEAD ,
	NPC_SPEAKING ,
	NPC_BUSINESS ,
};

enum TaskFailCode
{
	TF_NO_FAIL   ,
	TF_NO_TARGET ,
	TF_CANT_ATTACK ,
	TF_NO_EMPTY ,
	TF_NO_PATH  ,
	TF_UNKNOW   ,
};

union TTaskData
{
	float       FloatVal;
	long        IntVal;
	void*       PtrVal;
	char const* StrVal;
};

struct AITask
{
	AITask( int type , char const* name )
		:type( type ), name( name ){}
	AITask( int type , char const* name ,float val )
		:type( type ) , name( name ) , FloatVal( val ){}
	AITask( int type , char const* name, int   val )
		:type( type ) , name( name ), IntVal( val ){}
	AITask( int type , char const* name, void* val )
		:type( type ) , name( name ), PtrVal( val ){}
	AITask( int type , char const* name , char const* val )
		:type( type ) , name( name ), StrVal( val ){}

	int  type;
	char const* name;
	union
	{
		float       FloatVal;
		int         IntVal;
		void*       PtrVal;
		char const* StrVal;
	};
};





struct AIActionSchedule
{
	int         type;
	char const* name;
	int         numTask;
	AITask*     taskList;
	int         interruptsCond;
};




class TBehaviorBase
{
public:


};

class CNPCBase;

class CNPCBase : public CActor
{
	DESCRIBLE_GAME_OBJECT_CLASS( CNPCBase , CActor );
public:
	
	enum TaskType
	{
		TASK_WAIT          = 0 ,
		TASK_WAIT_MOVEMENT    ,
		TASK_STOP_MOVE        ,
		TASK_FACE_TARGET      ,
		TASK_SET_EMPTY_TRAGET  ,
		TASK_SET_PLAYER_TARGET ,
		TASK_ATTACK           ,
		TASK_WAIT_CONDITION   ,
		TASK_WAIT_ATTACK      ,
		TASK_NAV_GOAL_TARGET  ,
		TASK_NAV_GOAL_POS     ,
		TASK_NAV_RAND_GOAL_POS ,
		TASK_UPDATE_GOAL_EMPTY_POS  ,
		TASK_CHANGE_TASK       ,
		TASK_SCHEDULE_MAX_TIME ,
		TASK_SAVE_RAND_2D_POS   ,
		TASK_SAVE_SELF_POS     ,
		NEXT_TASK              ,
	};

	enum
	{
		SCHE_NO_SCHEDULE   ,
		SCHE_NO_MOVE_IDLE  ,
		SCHE_MOVE_TO_EMPTY ,
		SCHE_ATTACK        ,
		SCHE_MOVE_TO_TARGET,
		SCHE_GUARD_POSION ,
		SCHE_NUM ,
	};

	CNPCBase();
	~CNPCBase();
	bool init( GameObject* gameObject , GameObjectInitHelper const& helper );
	void update( long time );
public:
	virtual void takeDamage( DamageInfo const& info );

	virtual void onDying();

	bool onAttackTest();
	void onSpawn();


	void clearEmptyCondition();

public:
	virtual void onDead();

	void destoryThink( ThinkContent& content );
	void productChestThink( ThinkContent& content );
protected:
	float    m_deadTime;


public:

	virtual void sceneWorld();

	virtual int selectSchedule();

	virtual void gatherCondition();
	virtual void getherEmptyCondition();
	void clearCondition( ConditionType con );
	void addCondition( ConditionType con );
	bool haveCondition( ConditionType con );

protected:
	CActor*  m_hEmpty;

	struct EmptyConInfo
	{
		float dist;
		float angle;
	} m_emptyConInfo;

public:
	virtual AIActionSchedule* getScheduleByType( int sche );

	virtual void startTask( AITask const& task );
	virtual void runTask( AITask const& task );
	virtual void onChangeSchedule(){}

	void          setAIState( NPCAIState state );
	NPCAIState    getAIState(){ return m_AIState; }
	ILevelObject* getTarget(){  return m_targetPE; }
	void          setTraget( ILevelObject* entity ){ m_targetPE = entity;  }

protected:
	void runAI();
	void taskFail( TaskFailCode code );
	void taskFinish();

	void     maintainSchedule();
	bool     checkScheduleValid();

	enum TaskState
	{
		TKS_NEW     ,
		TKS_RUNNING ,
		TKS_SUCCESS ,
		TKS_FAIL    ,
	};
	struct SchduleStatus
	{
		SchduleStatus()
		{
			data = NULL ;
			taskState = TKS_FAIL;
		}
		AIActionSchedule*   data;
		TaskState    taskState;
		int          curTaskIndex;
		TaskFailCode failCode;
		float        startTime;
	};

	TaskState          getCurTaskState(){ return mSchdlestatus.taskState; }
	AITask const*      getCurTask();
	void               changeNextTask();
	AIActionSchedule*  getNewSchedule();
	AIActionSchedule*  getCurSchedule(){  return mSchdlestatus.data; }
	void               changeSchedule( AIActionSchedule* sche );

	SchduleStatus  mSchdlestatus;

	EHandle        m_targetPE;
	Vec3D          m_savePos;

	int            m_conditionBit;
	NPCAIState     m_AIState; 
	float          m_moveIntervalTime;
	float          mWaitTime;

public:
	TNavigator* getNavigator(){ return mNavigator; }
	void faceTarget( ILevelObject* target );

protected:
	
	TPtrHolder< TNavigator >    mNavigator;
	TPtrHolder< TBehaviorBase > mBehavior;

};


class TScheduleManager
{
public:

	typedef std::map< int , AIActionSchedule* > ScheduleMap;
	ScheduleMap scheMap;
};

template < class T , int TypeVal >
struct TaskNameSpace;

template < class T >
struct ScheduleNameSpace;

#define DEFINE_AI_ACTION_SCHEDULE( Class , Type )\
	template <>\
	struct TaskNameSpace< Class , Class::Type >\
	{\
		typedef Class ThisClass;\
		static  AITask taskData[];\
	};\
	AITask TaskNameSpace< Class , Class::Type >::taskData[] = {

#define ADD_TASK( type , val )\
	AITask( ThisClass::type , #type , val ) ,

#define END_SCHDULE() };

#define CLASS_AI_START( Class )\
	template <>\
	struct ScheduleNameSpace< Class >\
	{\
		typedef Class ThisClass;\
		static  AIActionSchedule scheData[];\
	};\
	AIActionSchedule ScheduleNameSpace< Class >::scheData[] = {

#define TASK_SIZE( arTask ) ( sizeof( arTask )/ sizeof( AITask ) )
#define ADD_SCHDULE( type , val ) \
	{ ThisClass::type ,  #type ,sizeof( TaskNameSpace< ThisClass , ThisClass::type >::taskData ) /sizeof ( AITask )\
     , TaskNameSpace< ThisClass , ThisClass::type >::taskData  ,  val },

#define CLASS_AI_END() };


#define GET_SCHEDULE_DATA( Class )\
	ScheduleNameSpace< Class >::scheData

//
//template< class THIS_NPC , class BASE_NPC >
//class NPCHelper : public BASE_NPC
//{
//
//	AIActionSchedule* CNPCBase::getScheduleByType( int sche )
//	{
//		static int const size = ARRAY_SIZE( GET_SCHEDULE_DATA( ThisClass ) );
//
//		for ( int i = 0 ; i < size ; ++i )
//		{
//			if ( GET_SCHEDULE_DATA( ThisClass )[i].type == sche )
//				return &GET_SCHEDULE_DATA( ThisClass )[i];
//		}
//
//		Msg("No Schedule Define %d" , sche );
//		return NULL;
//	}
//};

#endif // CNPCBase_h__
