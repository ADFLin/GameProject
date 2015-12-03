#ifndef shareDef_h__
#define shareDef_h__

extern char const* actionName[];

enum ActivityType
{
	ACT_IDLE = 0    ,
	ACT_MOVING      ,
	ACT_ATTACK      ,
	ACT_BUF_SKILL   ,
	ACT_DEFENSE     ,
	ACT_JUMP        ,
	ACT_LADDER      ,
	ACT_DYING       ,
};

enum ActorState
{
	AS_NORMAL  = 0,
	AS_POISON  = BIT( 0 ), //¤¤¬r
	AS_STUPOR  = BIT( 1 ), //³Â·ô
	AS_WILD    = BIT( 2 ), //¨g¼É
	AS_DIE     = BIT( 3 ), //¦º¤`

	AS_CATCH_OBJ = BIT( 4 ),
	AS_TOUCH_BAG = BIT( 5 ),
	AS_LADDER    = BIT( 6 ),
};


enum EntityType
{
	ET_ENTITY_BASE  = BIT(0) ,
	ET_LEVEL_ENTITY = BIT(1) ,
	ET_PHYENTITY    = BIT(2) ,
	ET_OBJECT       = BIT(3) ,
	ET_ACTOR        = BIT(4) ,
	ET_NPC          = BIT(5) ,
	ET_PLAYER       = BIT(6) ,
	ET_TRIGGER      = BIT(7) ,
	ET_BULLET       = BIT(8) ,
	ET_TERRAIN      = BIT(9) ,
};





#endif // shareDef_h__