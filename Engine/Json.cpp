#include "Json.h"


#include "nlohmann/json.hpp"

#include <fstream>
#include "DataStructure/Array.h"
#include "Meta/Select.h"

template< bool bReference >
class TJsonObjectImpl : public JsonObject
{
public:
	typedef typename TSelect<bReference , nlohmann::json& , nlohmann::json >::Type ImplType;

	TJsonObjectImpl( ImplType&& j )
		:mImpl(j){ }

	void release() override { delete this; }

	template< class T >
	bool tryGetT(char const* key, T& outValue)
	{
		auto iter = mImpl.find(key);
		if( iter == mImpl.end() || !iter->is_string() )
			return false;
		outValue = iter->get<T>();
		return true;
	}

	bool tryGet(char const* key, std::string& outValue) override 
	{
		auto iter = mImpl.find(key);
		if (iter == mImpl.end() || !iter->is_string())
			return false;
		outValue = iter->get<std::string>();
		return true;
	}

	bool tryGet(char const* key, int& outValue) override
	{
		auto iter = mImpl.find(key);
		if (iter == mImpl.end() || !iter->is_number_integer())
			return false;
		outValue = iter->get<int>();
		return true;
	}

	bool tryGet(char const* key, float& outValue) override 
	{
		auto iter = mImpl.find(key);
		if (iter == mImpl.end() || !iter->is_number_float())
			return false;
		outValue = iter->get<float>();
		return true;
	}

	bool tryGet(char const* key, bool& outValue) override 
	{
		auto iter = mImpl.find(key);
		if (iter == mImpl.end() || !iter->is_boolean())
			return false;
		outValue = iter->get<bool>();
		return true;
	}

	using SubObject = TJsonObjectImpl<true>;

	JsonObject* getObject(char const* key) override
	{
		auto iter = mImpl.find(key);
		if (iter == mImpl.end() || !iter->is_object())
			return nullptr;

		auto ptr = std::make_unique<SubObject>(*iter);
		mSubObjects.push_back(std::move(ptr));

		return mSubObjects.back().get();
	}

	TArray< std::unique_ptr<SubObject> > mSubObjects;
	ImplType mImpl;
};

JsonObject* JsonObject::LoadFromFile(char const* path)
{
	std::ifstream fs(path);

	if( !fs.is_open() )
		return nullptr;
	nlohmann::json j;
	fs >> j;
	if( !fs )
		return nullptr;

	return new TJsonObjectImpl<false>(std::move(j));
}

