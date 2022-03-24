#pragma once
#ifndef WindowsHeader_H_F167F40F_DEC3_4817_AE61_61F50A06BB51
#define WindowsHeader_H_F167F40F_DEC3_4817_AE61_61F50A06BB51

#define  NOMINMAX
#define  WIN32_LEAN_AND_MEAN

#include <windows.h>

#undef GetOpenFileName
#undef GetEnvironmentVariable
#undef CreateDirectory
#undef GetFileAttributes
#undef DeleteFile
#undef LoadImage

#endif // WindowsHeader_H_F167F40F_DEC3_4817_AE61_61F50A06BB51