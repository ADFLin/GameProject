#pragma once
#ifndef Json_H_C6F5228B_5BA8_421D_81FD_2BAD0BCEA769
#define Json_H_C6F5228B_5BA8_421D_81FD_2BAD0BCEA769

#include "DataStructure/Array.h"
#include <string>


enum class EJsonType : uint8
{
	Null,
	Object,
	Array,
	String,
	Bool,
	NumberInt,
	NumberUInt,
	NumberFloat,
};

class JsonObject;
class JsonValue
{
public:
	bool isObject() const { return type == EJsonType::Object; }
	JsonObject asObject() const;


	EJsonType type = EJsonType::Null;
	void* ptr = nullptr;
};

class JsonObject
{
public:

	bool isVaild() const { return !!mPtr;}

	bool tryGet(char const* key, std::string& outValue);
	bool tryGet(char const* key, int& outValue);
	bool tryGet(char const* key, float& outValue);
	bool tryGet(char const* key, bool& outValue);
	JsonObject getObject(char const* key);
	TArray<JsonValue> getArray(char const* key);

	void* mPtr = nullptr;

};

class JsonFile : public JsonObject
{
public:
	virtual void release() = 0;
	static JsonFile* Load(char const* path);
};


#endif // Json_H_C6F5228B_5BA8_421D_81FD_2BAD0BCEA769