#include "Tickable.h"

#if CORE_SHARE_CODE

TArray<Tickable*> Tickable::List;

void Tickable::Update(float deltaTime)
{
	for (auto tickable : List)
	{
		tickable->tick(deltaTime);
	}
}

#endif