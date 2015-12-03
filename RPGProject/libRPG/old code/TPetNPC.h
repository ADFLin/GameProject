#ifndef TPetNPC_h__
#define TPetNPC_h__

#include "TNPCBase.h"


class TPetNPC : public TNPCBase
{
	DESCRIBLE_CLASS( TPetNPC , TNPCBase )
public:
	TPetNPC( TActor* owner ,unsigned roleID , XForm const& trans );

	enum 
	{
		//
		SCHE_FOLLOW_OWNER   = BaseClass::SCHE_NUM ,
		SCHE_RAND_WALK ,

		TASK_TARGET_OWNER  = BaseClass::TASK_NUM , 
	};

	virtual void        gatherCondition();
	virtual void        startTask( TTask const& task );
	virtual void        runTask( TTask const& task );
	virtual TSchedule*  getScheduleByType( int sche );
	virtual int         selectSchedule();
	virtual void        OnEvent( TEvent& event );


	TActor* m_owmer;
};

#endif // TPetNPC_h__