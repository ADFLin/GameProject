#include "MiscTestRegister.h"

std::vector< MiscTestEntry>& GetRegisterMiscTestEntries()
{
	static std::vector< MiscTestEntry> gRegisterMiscTestEntries;
	return gRegisterMiscTestEntries;
}

MiscTestRegister::MiscTestRegister(char const* name, std::function< void() > const& func)
{
	GetRegisterMiscTestEntries().push_back({ name , func });
}

std::vector<MiscTestEntry>& MiscTestRegister::GetList()
{
	return GetRegisterMiscTestEntries();
}
