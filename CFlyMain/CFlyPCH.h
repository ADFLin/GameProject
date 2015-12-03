#ifndef CFlyPCH_h__
#define CFlyPCH_h__

#ifdef _MSC_VER
#	ifdef _DEBUG
#		define _SECURE_SCL 1
#	else
#		define _SECURE_SCL 1
#	endif
#endif

#include <cassert>
#include <algorithm>
#include <exception>
#include <string>
#include <vector>
#include <map>
#include <list>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <d3dx9effect.h>

#endif // CFlyPCH_h__
