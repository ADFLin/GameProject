#ifndef TetrisGameInfo_h__
#define TetrisGameInfo_h__

#include "GameReplay.h"

namespace Tetris
{
	typedef unsigned ModeID;

	enum TetrisModeID
	{
		MODE_TS_CHALLENGE ,
		MODE_TS_BATTLE   ,
		MODE_TS_PRACTICE , 
	};

	struct  NormalModeInfo
	{
		long startGravityLevel;
	};

	struct  TetrisGameInfoNEW
	{
		static uint32 const LastVersion = VERSION(0,0,1);
		ModeID   mode;
		union
		{
			NormalModeInfo modeNormal;
		};
		uint8  userID;
		uint8  numPlayer;
		uint8  playerID[ MAX_PLAYER_NUM ];
	};


	struct  GameInfo
	{
		static uint32 const LastVersion = VERSION(0,0,1);

		ModeID   mode;
		union
		{
			NormalModeInfo modeNormal;
		};
		unsigned userID;
		int      numLevel;
	};


	union  GameInfoData
	{
		GameInfo       info;
	};



}//namespace Tetris

#endif // TetrisGameInfo_h__
