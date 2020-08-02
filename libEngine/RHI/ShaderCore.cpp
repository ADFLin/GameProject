#include "ShaderCore.h"

#include "RHICommand.h"
#include "FileSystem.h"
#include "LogSystem.h"

namespace Render
{
	bool ShaderParameter::bind(ShaderParameterMap const& paramMap, char const* name)
	{

		auto iter = paramMap.mMap.find(name);
		if( iter == paramMap.mMap.end() )
		{
#if _DEBUG
			mName = name;
#endif
			return false;
		}

		*this = iter->second;
#if _DEBUG
		mName = name;
#endif
		return true;
	}

}//namespace Render