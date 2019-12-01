#pragma once
#ifndef MiscTestRegister_H_CCC98420_823A_45F0_BE96_D571753C9773
#define MiscTestRegister_H_CCC98420_823A_45F0_BE96_D571753C9773

#include "GameConfig.h"

#include "FixString.h"

#include <functional>
#include <vector>


struct MiscTestEntry
{
	FixString< 128 > name;
	std::function< void() > func;
};

class MiscTestRegister
{
public:
	TINY_API MiscTestRegister(char const* name, std::function< void() > const& func);
	static TINY_API std::vector<MiscTestEntry>& GetList();
};

#define  REGISTER_MISC_TEST_INNER( name , func )\
	static MiscTestRegister ANONYMOUS_VARIABLE( MARCO_NAME_COMBINE_2( g_MiscTest , __LINE__ ) )( name , func );

#define  REGISTER_MISC_TEST( name , func )\
	REGISTER_MISC_TEST_INNER( name , func )



#endif // MiscTestRegister_H_CCC98420_823A_45F0_BE96_D571753C9773
