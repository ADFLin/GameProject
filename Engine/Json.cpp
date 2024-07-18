#include "Json.h"


#include "nlohmann/json.hpp"

#include <fstream>
#include "DataStructure/Array.h"
#include "Meta/Select.h"

using ObjectImpl = nlohmann::json::object_t;

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

class FJsonObjectImpl
{
public:
	static bool Find(void* ptr, char const* key, ObjectImpl::iterator& outIter)
	{
		outIter = GetImpl(ptr)->find(key);
		return outIter != GetImpl(ptr)->end();
	}

	static ObjectImpl* GetImpl(void* ptr)
	{
		return static_cast<ObjectImpl*>(ptr);
	}
};

class JsonFileImpl : public JsonFile
{
public:
	JsonFileImpl(nlohmann::json&& j)
		:mImpl(j)
	{

	}
	virtual bool isObject(){ return mImpl.is_object(); }
	virtual bool isArray() { return mImpl.is_array(); }

	virtual JsonObject getObject()
	{
		JsonObject result;
		result.mPtr = mImpl.get_ptr<ObjectImpl*>();
		return result;
	}

	virtual TArray<JsonValue> getArray()
	{
		TArray<JsonValue> result;
		auto arrayPtr = mImpl.get_ptr<nlohmann::json::array_t*>();
		if ( arrayPtr )
		{
			for (auto& element : *arrayPtr)
			{
				result.push_back(ToValue(element));
			}
		}
		return result;
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

	if (j.is_object() || j.is_array() )
	{
		return new JsonFileImpl(std::move(j));
	}

	return nullptr;
}

bool JsonObject::tryGet(char const* key, std::string& outValue)
{
	ObjectImpl::iterator iter;
	if ( !FJsonObjectImpl::Find(mPtr, key, iter) )
		return false;
	if (!iter->second.is_string())
		return false;
	outValue = iter->second.get<std::string>();
	return true;
}

bool JsonObject::tryGet(char const* key, int& outValue)
{
	ObjectImpl::iterator iter;
	if (!FJsonObjectImpl::Find(mPtr, key, iter))
		return false;
	if (!iter->second.is_number())
		return false;
	outValue = iter->second.get<int>();
	return true;
}

bool JsonObject::tryGet(char const* key, float& outValue)
{
	ObjectImpl::iterator iter;
	if (!FJsonObjectImpl::Find(mPtr, key, iter))
		return false;
	if (!iter->second.is_number())
		return false;
	outValue = iter->second.get<float>();
	return true;
}

bool JsonObject::tryGet(char const* key, bool& outValue)
{
	ObjectImpl::iterator iter;
	if (!FJsonObjectImpl::Find(mPtr, key, iter))
		return false;
	if (iter->second.is_boolean())
	{
		outValue = iter->second.get<bool>();
		return true;
	}
	else if (iter->second.is_string())
	{
		auto strPtr = iter->second.get_ptr<nlohmann::json::string_t*>();
		if (*strPtr == "true")
		{
			outValue = true;
			return true;
		}
		if (*strPtr == "false")
		{
			outValue = false;
			return true;
		}
	}
	return false;
}

JsonObject JsonObject::getObject(char const* key)
{
	JsonObject result;
	ObjectImpl::iterator iter;
	if (FJsonObjectImpl::Find(mPtr, key, iter) &&
		iter->second.is_object())
	{
		result.mPtr = iter->second.get_ptr<ObjectImpl*>();
	}
	return result;
}



TArray<JsonValue> JsonObject::getArray(char const* key)
{
	TArray<JsonValue> result;
	ObjectImpl::iterator iter;
	if (FJsonObjectImpl::Find(mPtr, key, iter) &&
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
