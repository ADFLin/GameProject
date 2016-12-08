#ifndef GameDefines_h__b6b703c3_77d6_4143_b3b2_b76eee58d347
#define GameDefines_h__b6b703c3_77d6_4143_b3b2_b76eee58d347

enum StageModeType
{
	SMT_UNKNOW      ,
	SMT_SINGLE_GAME ,
	SMT_NET_GAME    ,
	SMT_REPLAY      ,
};

enum GameAttrib
{
	//Game
	ATTR_GAME_VERSION    ,
	ATTR_NET_SUPPORT     ,
	ATTR_SINGLE_SUPPORT  ,
	ATTR_REPLAY_SUPPORT  ,
	ATTR_AI_SUPPORT      ,

	ATTR_CONTROLLER_DEFUAULT_SETTING ,

	//GameStage
	ATTR_REPLAY_INFO_DATA ,
	ATTR_REPLAY_INFO     ,
	ATTR_TICK_TIME       ,
	ATTR_GAME_INFO       ,
	ATTR_REPLAY_UI_POS   ,
	
	ATTR_NEXT_ID ,
};


struct AttribValue
{
	AttribValue( int id ): id( id ){ ptr = 0; }
	AttribValue( int id , int val ):id( id ),iVal( val ){}
	AttribValue( int id , long val ):id( id ),iVal( val ){}
	AttribValue( int id , float val ):id( id ),fVal( val ){}
	AttribValue( int id , char const* val ):id( id ),str( val ){}
	AttribValue( int id , void* val ):id( id ),ptr( val ){}
	int id;
	union
	{
		struct
		{
			int x , y;
		} v2;
		long  iVal;
		float fVal;
		void* ptr;
		char const* str;
	};
};


#endif // GameDefines_h__b6b703c3_77d6_4143_b3b2_b76eee58d347
