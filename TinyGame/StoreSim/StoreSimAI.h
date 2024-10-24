#pragma once

#ifndef StoreSimAI_H_6F831B29_D13A_4837_BE8B_BF5AF33C1208
#define StoreSimAI_H_6F831B29_D13A_4837_BE8B_BF5AF33C1208

#include "StoreSimCore.h"
#include "StoreSimNav.h"
#include "BehaviorTree/BTNode.h"

namespace StoreSim
{

	class MoveToAction : public BT::BaseActionNodeT< MoveToAction >
	{


		TArray< Vector2 > Path;
	};
	class AIController
	{
	public:



	};

}

#endif // StoreSimAI_H_6F831B29_D13A_4837_BE8B_BF5AF33C1208
