#ifndef rcWorkerData_h__
#define rcWorkerData_h__


struct WorkerInfo
{
	int      randMoveDist;
	int      moveSpeed;
	unsigned animBit;
	unsigned texAnim[ 5 ];
};


enum
{
	WKA_WALK       ,
	WKA_DIE        ,
	WAK_ADDTIONNAL ,

	WAK_ANIM_NUM ,
};

enum WorkerTag
{
	WT_FIND_MAN ,
	WT_BATH_WORKER ,

	WT_ACTOR ,
	WT_LOIN_TAMER ,
	WT_TAX_COLLECTOR ,
	WT_SCHOOL_CHILD ,
	WT_MARKET_TRADER ,
	WT_DELIVERY_MAN , 
	WT_SETTLER ,
	WT_ENGINNER ,

	WT_LION ,



	WT_PATRICIAN ,
	WT_DOCTOR ,

	WT_PREFECT ,
	
	WT_UNKNOWN ,
	WT_NUM_WORKER ,
};

#endif // rcWorkerData_h__
