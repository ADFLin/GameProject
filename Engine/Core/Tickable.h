#pragma once
#ifndef Tickable_h__
#define Tickable_h__

#include "CoreShare.h"
#include "DataStructure/Array.h"

class Tickable
{
public:
	Tickable()
	{
		List.push_back(this);
	}
	~Tickable()
	{
		List.remove(this);
	}

	virtual void tick(float deltaTime) = 0;

	CORE_API static void Update(float deltaTime);	
	CORE_API static TArray<Tickable*> List;

};
#endif // Tickable_h__
