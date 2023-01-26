#pragma once
#ifndef Tickable_h__
#define Tickable_h__

#include "CoreShare.h"
#include "StdUtility.h"
#include <vector>

class Tickable
{
public:
	Tickable()
	{
		List.push_back(this);
	}
	~Tickable()
	{
		RemoveValue(List, this);
	}

	virtual void tick(float deltaTime) = 0;

	CORE_API static void Update(float deltaTime);	
	CORE_API static std::vector<Tickable*> List;

};
#endif // Tickable_h__
