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

	void ShaderPorgramParameterMap::clear()
	{
		ShaderParameterMap::clear();
		mParamEntryMap.clear();
		mParamEntries.clear();
	}

	void ShaderPorgramParameterMap::addShaderParameterMap(EShader::Type shaderType, ShaderParameterMap const& parameterMap)
	{
		for (auto const& pair : parameterMap.mMap)
		{
			auto& param = mMap[pair.first];

			if (param.mLoc == -1)
			{
				param.mLoc = mParamEntryMap.size();
#if _DEBUG
				param.mbindType = pair.second.mbindType;
#endif
				ParameterEntry entry;
				entry.numParam = 0;
				mParamEntryMap.push_back(entry);
			}

			ParameterEntry& entry = mParamEntryMap[param.mLoc];

			entry.numParam += 1;
			ShaderParamEntry paramEntry;
			paramEntry.type = shaderType;
			paramEntry.param = pair.second;
			paramEntry.loc = param.mLoc;
			mParamEntries.push_back(paramEntry);
		}
	}

	void ShaderPorgramParameterMap::finalizeParameterMap()
	{
		std::sort(mParamEntries.begin(), mParamEntries.end(), [](ShaderParamEntry const& lhs, ShaderParamEntry const& rhs)
		{
			if (lhs.loc < rhs.loc)
				return true;
			else if (lhs.loc > rhs.loc)
				return false;
			return lhs.type < rhs.type;
		});

		int curLoc = -1;
		for (int index = 0; index < mParamEntries.size(); ++index)
		{
			auto& paramEntry = mParamEntries[index];
			if (paramEntry.loc != curLoc)
			{
				mParamEntryMap[paramEntry.loc].paramIndex = index;
				curLoc = paramEntry.loc;
			}
		}
	}



}//namespace Render