#include "ShaderCore.h"

#include "RHICommand.h"
#include "FileSystem.h"
#include "LogSystem.h"

namespace Render
{
	ShaderResource::~ShaderResource()
	{

	}

	bool ShaderParameter::bind(ShaderParameterMap const& paramMap, char const* name)
	{
#if _DEBUG
		mName = name;
#endif
		auto iter = paramMap.mMap.find(name);
		if( iter == paramMap.mMap.end() )
		{
			return false;
		}

		mLoc = iter->second.mLoc;
		return true;
	}

}//namespace Render