#include "Json.h"


#include "nlohmann/json.hpp"

#include <fstream>
#include "DataStructure/Array.h"
#include "Meta/Select.h"

using ObjectImpl = nlohmann::json::object_t;

void StaticCheck()
{
	static_assert((uint8)EJsonType::Null == (uint8)nlohmann::detail::value_t::null);
	static_assert((uint8)EJsonType::Object == (uint8)nlohmann::detail::value_t::object);
	static_assert((uint8)EJsonType::Array == (uint8)nlohmann::detail::value_t::array);
	static_assert((uint8)EJsonType::String == (uint8)nlohmann::detail::value_t::string);
	static_assert((uint8)EJsonType::Bool == (uint8)nlohmann::detail::value_t::boolean);
	static_assert((uint8)EJsonType::NumberInt == (uint8)nlohmann::detail::value_t::number_integer);
	static_assert((uint8)EJsonType::NumberUInt == (uint8)nlohmann::detail::value_t::number_unsigned);
	static_assert((uint8)EJsonType::NumberFloat == (uint8)nlohmann::detail::value_t::number_float);
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
	virtual std::string toString() override { return mImpl.dump(4); }
	virtual void release() override { delete this; }

	nlohmann::json mImpl;
};

JsonFile* JsonFile::Create()
{
	return new JsonFileImpl(nlohmann::json::object());
}

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

bool JsonObject::tryGet(char const* key, std::string& outValue) const
{
	ObjectImpl::iterator iter;
	if ( !FJsonObjectImpl::Find(mPtr, key, iter) )
		return false;
	if (!iter->second.is_string())
		return false;
	outValue = iter->second.get<std::string>();
	return true;
}

bool JsonObject::tryGet(char const* key, int& outValue) const
{
	ObjectImpl::iterator iter;
	if (!FJsonObjectImpl::Find(mPtr, key, iter))
		return false;
	if (!iter->second.is_number())
		return false;
	outValue = iter->second.get<int>();
	return true;
}

bool JsonObject::tryGet(char const* key, float& outValue) const
{
	ObjectImpl::iterator iter;
	if (!FJsonObjectImpl::Find(mPtr, key, iter))
		return false;
	if (!iter->second.is_number())
		return false;
	outValue = iter->second.get<float>();
	return true;
}

bool JsonObject::tryGet(char const* key, bool& outValue) const
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

JsonObject JsonObject::getOrAddObject(char const* key)
{
	JsonObject result;
	if (!mPtr) return result;

	auto* obj = FJsonObjectImpl::GetImpl(mPtr);
	auto iter = obj->find(key);
	if (iter != obj->end())
	{
		if (iter->second.is_object())
		{
			result.mPtr = iter->second.get_ptr<ObjectImpl*>();
		}
	}
	else
	{
		(*obj)[key] = nlohmann::json::object();
		result.mPtr = (*obj)[key].get_ptr<ObjectImpl*>();
	}
	return result;
}

std::vector<std::string> JsonObject::getKeys() const
{
	std::vector<std::string> keys;
	if (!mPtr) return keys;

	auto* obj = FJsonObjectImpl::GetImpl(mPtr);
	for (auto& pair : *obj)
	{
		keys.push_back(pair.first);
	}
	return keys;
}

void JsonObject::set(char const* key, std::string const& value)
{
	if (!mPtr) return;
	(*FJsonObjectImpl::GetImpl(mPtr))[key] = value;
}

void JsonObject::set(char const* key, int value)
{
	if (!mPtr) return;
	(*FJsonObjectImpl::GetImpl(mPtr))[key] = value;
}

void JsonObject::set(char const* key, float value)
{
	if (!mPtr) return;
	(*FJsonObjectImpl::GetImpl(mPtr))[key] = value;
}

void JsonObject::set(char const* key, bool value)
{
	if (!mPtr) return;
	(*FJsonObjectImpl::GetImpl(mPtr))[key] = value;
}

JsonArray JsonObject::getJsonArray(char const* key) const
{
	JsonArray result;
	ObjectImpl::iterator iter;
	if (FJsonObjectImpl::Find(mPtr, key, iter) &&
		iter->second.is_array())
	{
		result.mPtr = iter->second.get_ptr<nlohmann::json::array_t*>();
	}
	return result;
}

JsonArray JsonObject::getOrAddArray(char const* key)
{
	JsonArray result;
	if (!mPtr) return result;

	auto* obj = FJsonObjectImpl::GetImpl(mPtr);
	auto iter = obj->find(key);
	if (iter != obj->end())
	{
		if (iter->second.is_array())
		{
			result.mPtr = iter->second.get_ptr<nlohmann::json::array_t*>();
		}
	}
	else
	{
		(*obj)[key] = nlohmann::json::array();
		result.mPtr = (*obj)[key].get_ptr<nlohmann::json::array_t*>();
	}
	return result;
}

// ============= JsonArray =============

size_t JsonArray::size() const
{
	if (!mPtr) return 0;
	return static_cast<nlohmann::json::array_t*>(mPtr)->size();
}

void JsonArray::resize(size_t sz)
{
	if (!mPtr) 
		return;
	auto* arr = static_cast<nlohmann::json::array_t*>(mPtr);
	arr->resize(sz);
}

bool JsonArray::tryGet(size_t index, std::string& outValue) const
{
	if (!mPtr) 
		return false;

	auto* arr = static_cast<nlohmann::json::array_t*>(mPtr);
	if (index >= arr->size() || !(*arr)[index].is_string()) 
		return false;
	
	outValue = (*arr)[index].get<std::string>();
	return true;
}

bool JsonArray::tryGet(size_t index, int& outValue) const
{
	if (!mPtr) 
		return false;

	auto* arr = static_cast<nlohmann::json::array_t*>(mPtr);
	if (index >= arr->size() || !(*arr)[index].is_number()) 
		return false;
	outValue = (*arr)[index].get<int>();
	return true;
}

bool JsonArray::tryGet(size_t index, float& outValue) const
{
	if (!mPtr) 
		return false;

	auto* arr = static_cast<nlohmann::json::array_t*>(mPtr);
	if (index >= arr->size() || !(*arr)[index].is_number()) 
		return false;
	outValue = (*arr)[index].get<float>();
	return true;
}

bool JsonArray::tryGet(size_t index, bool& outValue) const
{
	if (!mPtr) 
		return false;

	auto* arr = static_cast<nlohmann::json::array_t*>(mPtr);
	if (index >= arr->size() || !(*arr)[index].is_boolean()) 
		return false;
	outValue = (*arr)[index].get<bool>();
	return true;
}

void JsonArray::set(size_t index, std::string const& value)
{
	if (!mPtr) 
		return;
	auto* arr = static_cast<nlohmann::json::array_t*>(mPtr);
	if (index < arr->size()) (*arr)[index] = value;
}

void JsonArray::set(size_t index, int value)
{
	if (!mPtr) 
		return;
	auto* arr = static_cast<nlohmann::json::array_t*>(mPtr);
	if (index < arr->size()) (*arr)[index] = value;
}

void JsonArray::set(size_t index, float value)
{
	if (!mPtr) 
		return;
	auto* arr = static_cast<nlohmann::json::array_t*>(mPtr);
	if (index < arr->size()) (*arr)[index] = value;
}

void JsonArray::set(size_t index, bool value)
{
	if (!mPtr) 
		return;
	auto* arr = static_cast<nlohmann::json::array_t*>(mPtr);
	if (index < arr->size()) (*arr)[index] = value;
}

JsonObject JsonArray::getObject(size_t index)
{
	JsonObject result;
	if (!mPtr) 
		return result;

	auto* arr = static_cast<nlohmann::json::array_t*>(mPtr);
	if (index < arr->size() && (*arr)[index].is_object())
	{
		result.mPtr = (*arr)[index].get_ptr<ObjectImpl*>();
	}
	return result;
}

JsonObject JsonArray::getOrAddObject(size_t index)
{
	JsonObject result;
	if (!mPtr) 
		return result;

	auto* arr = static_cast<nlohmann::json::array_t*>(mPtr);
	if (index < arr->size())
	{
		if (!(*arr)[index].is_object())
			(*arr)[index] = nlohmann::json::object();
		result.mPtr = (*arr)[index].get_ptr<ObjectImpl*>();
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
