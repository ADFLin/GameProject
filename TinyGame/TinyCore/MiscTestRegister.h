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
	std::function< void() > fun;
};

class MiscTestRegister
{
public:
	TINY_API MiscTestRegister(char const* name, std::function< void() > const& fun);
	static TINY_API std::vector<MiscTestEntry>& GetList();
};

#define  REGISTER_MISC_TEST_INNER( name , fun )\
	static MiscTestRegister ANONYMOUS_VARIABLE( MARCO_NAME_COMBINE_2( g_MiscTest , __LINE__ ) )( name , fun );

#define  REGISTER_MISC_TEST( name , fun )\
	REGISTER_MISC_TEST_INNER( name , fun )



#endif // MiscTestRegister_H_CCC98420_823A_45F0_BE96_D571753C9773
