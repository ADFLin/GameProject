#pragma once

#ifndef TypeHash_h__
#define TypeHash_h__

#include "IntegerType.h"

namespace Type
{
	uint32 Hash(char const* str)
	{
		int32 hash = 5381;
		int c;

		while( c = *str++ )
		{
			hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
		}
		return hash;
	}

	uint32 Hash(char const* str , int num )
	{
		uint32 hash = 5381;

		while( num )
		{
			uint32 c = (uint32)*str++;
			hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
			--num;
		}
		return hash;
	}
}


#endif // TypeHash_h__