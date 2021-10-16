#pragma once
#ifndef MineDefines_H_43DEEEC1_FD67_45B7_BDC4_969ADBF2A255
#define MineDefines_H_43DEEEC1_FD67_45B7_BDC4_969ADBF2A255

namespace Mine
{
	enum CellValue
	{
		CV_BOMB = -1,
		CV_FLAG = -2,
		CV_UNPROBLED = -3,
		CV_OUT_OF_BOUNDS = -4,

		CV_FLAG_NO_BOMB  = -5,
		CV_UNKNOWN = -6,
	};

	enum class EGameState
	{
		Run,
		Stop,
		Fail,
		Complete,
	};


}//namespace Mine

#endif // MineDefines_H_43DEEEC1_FD67_45B7_BDC4_969ADBF2A255