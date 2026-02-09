#include "ShaderCore.h"

#include "RHICommand.h"
#include "FileSystem.h"
#include "LogSystem.h"
#include "ShaderFormat.h"
#include "InlineString.h"

#include "CoreShare.h"

namespace Render
{


	char const* const GShaderNames[] =
	{
		"VERTEX_SHADER" ,
		"PIXEL_SHADER" ,
		"GEOMETRY_SHADER" ,
		"COMPUTE_SHADER" ,
		"HULL_SHADER" ,
		"DOMAIN_SHADER" ,
		"TASK_SHADER" ,
		"MESH_SHADER" ,
		"RAY_GEN_SHADER" ,
		"RAY_HIT_SHADER" ,
		"RAY_MISS_SHADER" ,
		"RAY_CLOSEST_HIT_SHADER" ,
		"RAY_ANY_HIT_SHADER" ,
		"RAY_INTERSECTION_SHADER" ,
		"CALLABLE_SHADER" ,
	};

	bool ShaderParameter::bind(ShaderParameterMap const& paramMap, char const* name)
	{
#if SHADER_DEBUG
		mName = name;
#endif
		auto iter = paramMap.mMap.find(name);
		if( iter == paramMap.mMap.end() )
			return false;

		*this = iter->second;
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

			if (param.mLoc == INDEX_NONE)
			{
				param.mLoc = mParamEntryMap.size();
#if SHADER_DEBUG
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

	std::string GetFilePath(char const* name)
	{
		std::string path("Material/");
		path += name;
		return path;
	}

	void MaterialShaderCompileInfo::setup(ShaderCompileOption& option) const
	{
		option.addDefine(SHADER_PARAM(MATERIAL_FILENAME) , InlineString<>::Make("\"Material/%s%s\"" , name, SHADER_FILE_SUBNAME) );
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

	std::string ShaderCompileOption::GetIncludeFileName(char const* name)
	{
		return InlineString<>::Make("\"%s%s\"", name, SHADER_FILE_SUBNAME);
	}

	std::string ShaderCompileOption::getCode(ShaderEntryInfo const& entry, char const* defCode) const
	{
		std::string result;
		if (defCode)
		{
			result += defCode;
		}

		auto AddCode = [&](EShaderCodePos pos)
		{
			for (auto const& code : mCodes)
			{
				if ( code.pos != pos )
					continue;

				result += code.content;
				result += '\n';
			}
		};

		result += "#define SHADER_COMPILING 1\n";
		result += InlineString<>::Make("#define SHADER_ENTRY_%s 1\n", entry.name);
		result += InlineString<>::Make("#define %s 1\n", GShaderNames[entry.type]);
		result += InlineString<>::Make("#define %s %d\n", SHADER_PARAM(USE_INVERSE_ZBUFFER) , (int)FRHIZBuffer::IsInverted);

		for (auto const& var : mConfigVars)
		{
			result += "#define ";
			result += var.name;
			if (var.value.length())
			{
				result += " ";
				result += var.value;
			}
			result += "\n";
		}

		AddCode(EShaderCodePos::BeforeCommon);

		result += "#include \"Common" SHADER_FILE_SUBNAME "\"\n";

		AddCode(EShaderCodePos::AfterCommon);
		AddCode(EShaderCodePos::BeforeInclude);

		for (auto& name : mIncludeFiles)
		{
			result += "#include \"";
			result += name;
			result += SHADER_FILE_SUBNAME;
			result += "\"\n";
		}

		AddCode(EShaderCodePos::AfterInclude);
		AddCode(EShaderCodePos::Last);
		return result;
	}

	void ShaderCompileOption::setup(ShaderFormat& shaderFormat) const
	{
		if (bHadShaderFormatSetup == false)
		{
			ShaderCompileOption* mutableThis = const_cast<ShaderCompileOption*>(this);
			mutableThis->bHadShaderFormatSetup = true;
			shaderFormat.setupShaderCompileOption(*mutableThis);
		}
	}

}//namespace Render