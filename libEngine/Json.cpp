#include "Json.h"


#include "nlohmann/json.hpp"

#include <fstream>

class JsonObjectImpl : public JsonObject
{
public:
	typedef nlohmann::json ImplType;
	JsonObjectImpl( ImplType&& j )
		:mImpl(j){ }

	void release() override { delete this; }
	bool tryGet(char const* key, std::string& str) override
	{
		auto iter = mImpl.find(key);
		if( iter == mImpl.end() || !iter->is_string() )
			return false;
		str = iter->get<std::string>();
		return true;
	}
	ImplType mImpl;
};

JsonObject* JsonObject::LoadFromFile(char const* path)
{
	std::ifstream fs(path);

	if( !fs.is_open() )
		return false;
	nlohmann::json j;
	fs >> j;
	if( !fs )
		return false;

	return new JsonObjectImpl(std::move(j));
}

