#include "ModularFeature.h"

#include "StdUtility.h"

#include <vector>
#include <unordered_map>


class ModularFeaturesImpl : public IModularFeatures
{

public:
	void registerFeature(HashString name, IModularFeature* feature)
	{
		auto& data = mNameMap[name];
		data.add(feature);
		if (data.event)
		{
			data.event(feature, false);
		}
	}

	void unregisterFeature(HashString name, IModularFeature* feature)
	{
		auto& data = mNameMap[name];
		data.remove(feature);
		if (data.event)
		{
			data.event(feature, true);
		}
	}

	void getFeatures(HashString name, std::vector<IModularFeature*>& outFeatures)
	{
		auto iter = mNameMap.find(name);
		if (iter == mNameMap.end())
			return;

		outFeatures = iter->second.list;
	}

	void addEvent(HashString name, ModularFeatureEvent delegate)
	{
		auto& data = mNameMap[name];
		data.event = delegate;
	}

	void removeEvent(HashString name)
	{
		auto iter = mNameMap.find(name);
		if (iter == mNameMap.end())
			return;

		iter->second.event.clear();
	}

	struct FeatureData
	{
		std::vector<IModularFeature*> list;
		ModularFeatureEvent event;

		void add(IModularFeature* feature)
		{
			list.push_back(feature);
		}
		void remove(IModularFeature* feature)
		{
			RemoveValue(list, feature);
		}
	};
	std::unordered_map<HashString, FeatureData> mNameMap;

};


#if CORE_SHARE_CODE
IModularFeatures& IModularFeatures::Get()
{
	static ModularFeaturesImpl StaticInstance;
	return StaticInstance;
}



#endif
