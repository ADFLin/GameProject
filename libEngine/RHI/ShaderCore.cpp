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

	void ShaderPorgramParameterMap::addShaderParameterMap(int shaderIndex, ShaderParameterMap const& parameterMap)
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
			paramEntry.shaderIndex = shaderIndex;
			paramEntry.param = pair.second;
			paramEntry.loc = param.mLoc;
			mParamEntries.push_back(paramEntry);
		}
	}

	void ShaderPorgramParameterMap::finalizeParameterMap()
	{
		std::sort(mParamEntries.begin(), mParamEntries.end(), [](ShaderParamEntry const& lhs, ShaderParamEntry const& rhs)
		{
			if (lhs.loc != rhs.loc)
				return lhs.loc < rhs.loc;
			return lhs.shaderIndex < rhs.shaderIndex;
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


	void MaterialShaderCompileInfo::setup(ShaderCompileOption& option) const
	{
		switch (tessellationMode)
		{
		case ETessellationMode::Flat:
			option.addDefine(SHADER_PARAM(USE_TESSELLATION), true);
			break;
		case ETessellationMode::PNTriangle:
			option.addDefine(SHADER_PARAM(USE_TESSELLATION), true);
			option.addDefine(SHADER_PARAM(USE_PN_TRIANGLE), true);
			break;
		}

	}

}//namespace Render