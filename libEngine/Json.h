#pragma once
#ifndef Json_H_C6F5228B_5BA8_421D_81FD_2BAD0BCEA769
#define Json_H_C6F5228B_5BA8_421D_81FD_2BAD0BCEA769


#include <string>

class JsonObject
{
public:
	virtual ~JsonObject(){}
	virtual void release() = 0;

	virtual bool tryGet(char const* key , std::string& str) = 0;

	static JsonObject* LoadFromFile(char const* path);

};


#endif // Json_H_C6F5228B_5BA8_421D_81FD_2BAD0BCEA769