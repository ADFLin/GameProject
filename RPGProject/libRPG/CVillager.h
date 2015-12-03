#ifndef CVillager_h__
#define CVillager_h__

#include "common.h"
#include "CNPCBase.h"

class CVillager : public CNPCBase
{
	DESCRIBLE_CLASS( CVillager , CNPCBase )
public:


	enum 
	{
		SELLER,
		TALKER,
	};

	enum 
	{
		//
		SCHE_SELL_ITEMS = BaseClass::SCHE_NUM ,
		SCHE_SAY_HOLLOW,
		SCHE_RAND_WALK ,

		TASK_TALK       = BaseClass::NEXT_TASK ,
		TASK_WAIT_TALK     ,
		TASK_TRADE_ITEMS   ,
		TASK_WAIT_TRADE    ,
	};

	CVillager();
	bool init( GameObject* gameObject , GameObjectInitHelper const& helper );

	//virtual 
	void onInteract( ILevelObject* entity );

	virtual void  startTask( AITask const& task );
	virtual void  runTask( AITask const& task );
	virtual AIActionSchedule* getScheduleByType( int sche );



	virtual int  selectSchedule();
	void         addSellItem( unsigned item );

	void onEvent( TEvent const& event );
	virtual void  onTalkSectionEnd(){}
	virtual void  onTalkEnd(){}

	void setMode( int mode ){ m_mode = mode;}
	
	int m_mode;
	
};

#endif // CVillager_h__
