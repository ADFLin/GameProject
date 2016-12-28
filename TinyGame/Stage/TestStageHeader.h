#pragma once

#include "StageBase.h"
#include "StageRegister.h"

#include "GameGUISystem.h"
#include "DrawEngine.h"
#include "RenderUtility.h"
#include "GameInstance.h"

#include "IntegerType.h"

class MiscTestRegister
{
public:
	GAME_API MiscTestRegister(char const* name, std::function< void() > const& fun);
};

#define  REGISTER_MISC_TEST( name , fun )\
	MiscTestRegister MARCO_NAME_COMBINE_3( gMiscTestREgister , __LINE__ )( name , fun );