#ifndef TetrisActionId_h__0cd46f2f_06f0_4b43_a326_ecddcc1d6daf
#define TetrisActionId_h__0cd46f2f_06f0_4b43_a326_ecddcc1d6daf

#include "GameControl.h"

namespace Tetris
{
	enum ActionId
	{
		ACT_ROTATE_CCW = ACT_BUTTON0,
		ACT_ROTATE_CW  = ACT_BUTTON1,

		ACT_FALL_PIECE ,
		ACT_HOLD_PIECE ,

		ACT_MOVE_LEFT ,
		ACT_MOVE_RIGHT,
		ACT_MOVE_DOWN ,

		NUM_TETRIS_ACTION ,
	};


}//namespace Tetris

#endif // TetrisActionId_h__0cd46f2f_06f0_4b43_a326_ecddcc1d6daf
