#include "Json.h"


#include "nlohmann/json.hpp"

#include <fstream>
#include "DataStructure/Array.h"
#include "Meta/Select.h"


using ObjectImpl = nlohmann::json::object_t;

class JsonObjectImpl : public JsonObject
{
public:
	bool find(char const* key, ObjectImpl::iterator& outIter)
	{
		outIter = getImpl()->find(key);
		return outIter != getImpl()->end();
	}

	ObjectImpl* getImpl()
	{
		return static_cast<ObjectImpl*>(mPtr);
	}
};

class JsonFileImpl : public JsonFile
{
public:
	JsonFileImpl(nlohmann::json&& j)
		:mImpl(j)
	{
		mPtr = mImpl.get_ptr<ObjectImpl*>();
	}

	virtual void release() override { delete this; }

	nlohmann::json mImpl;
};

JsonFile* JsonFile::Load(char const* path)
{
	std::ifstream fs(path);

	if (!fs.is_open())
		return nullptr;
	nlohmann::json j;
	fs >> j;
	if (!fs)
		return nullptr;

	if (!j.is_object())
		return nullptr;
	return new JsonFileImpl(std::move(j));
}

bool JsonObject::tryGet(char const* key, std::string& outValue)
{
	ObjectImpl::iterator iter;
	if ( !static_cast<JsonObjectImpl*>(this)->find(key, iter) )
		return false;
	if (!iter->second.is_string())
		return false;
	outValue = iter->second.get<std::string>();
	return true;
}

bool JsonObject::tryGet(char const* key, int& outValue)
{
	ObjectImpl::iterator iter;
	if (!static_cast<JsonObjectImpl*>(this)->find(key, iter))
		return false;
	if (!iter->second.is_number())
		return false;
	outValue = iter->second.get<int>();
	return true;
}

bool JsonObject::tryGet(char const* key, float& outValue)
{
	ObjectImpl::iterator iter;
	if (!static_cast<JsonObjectImpl*>(this)->find(key, iter))
		return false;
	if (!iter->second.is_number())
		return false;
	outValue = iter->second.get<float>();
	return true;
}

bool JsonObject::tryGet(char const* key, bool& outValue)
{
	ObjectImpl::iterator iter;
	if (!static_cast<JsonObjectImpl*>(this)->find(key, iter))
		return false;
	if (!iter->second.is_boolean())
		return false;
	outValue = iter->second.get<int>();
	return true;
}

JsonObject JsonObject::getObject(char const* key)
{
	JsonObject result;
	ObjectImpl::iterator iter;
	if (static_cast<JsonObjectImpl*>(this)->find(key, iter) &&
		iter->second.is_object())
	{
		result.mPtr = iter->second.get_ptr<ObjectImpl*>();
	}
	return result;
}


JsonValue ToValue(nlohmann::json& json)
{
	using namespace nlohmann::detail;
	if (json.type() == value_t::discarded)
		return JsonValue();
	JsonValue result;
	result.type = static_cast<EJsonType>(static_cast<uint8>(json.type()));
	result.ptr = &json;
	return result;
}

TArray<JsonValue> JsonObject::getArray(char const* key)
{
	TArray<JsonValue> result;
	ObjectImpl::iterator iter;
	if (static_cast<JsonObjectImpl*>(this)->find(key, iter) &&
		iter->second.is_array())
	{
		auto arrayPtr = iter->second.get_ptr<nlohmann::json::array_t*>();
		for (auto& element : *arrayPtr)
		{
			result.push_back(ToValue(element));
		}
	}
	return result;
}

JsonObject JsonValue::asObject() const
{
	if (!isObject())
		return JsonObject();

	JsonObject object;
	object.mPtr = static_cast<nlohmann::json*>(ptr)->get_ptr<ObjectImpl*>();
	return object;
}
