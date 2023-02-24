#pragma once

#include "HashString.h"
#include "CoreShare.h"
#include "Delegate.h"
#include "DataStructure/Array.h"


class IModularFeature
{
public:

};


DECLARE_DELEGATE(ModularFeatureEvent, void(IModularFeature*, bool bRemove));
class IModularFeatures
{

public:
	CORE_API static IModularFeatures& Get();

	virtual void registerFeature(HashString name, IModularFeature* feature) = 0;
	virtual void unregisterFeature(HashString name, IModularFeature* feature) = 0;
	virtual void addEvent(HashString name, ModularFeatureEvent delegate) = 0;
	virtual void removeEvent(HashString name) = 0;

	virtual void getFeatures(HashString name, TArray<IModularFeature*>& outFeatures) = 0;

};