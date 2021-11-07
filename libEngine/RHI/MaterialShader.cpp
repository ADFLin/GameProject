#include "MaterialShader.h"

namespace Render
{

#if CORE_SHARE_CODE
	std::vector< MaterialShaderProgramClass* > MaterialShaderProgramClass::ClassList;

	MaterialShaderProgramClass::MaterialShaderProgramClass(
		CreateShaderObjectFunc inCreateShaderObject,
		SetupShaderCompileOptionFunc inSetupShaderCompileOption, 
		GetShaderFileNameFunc inGetShaderFileName, 
		GetShaderEntriesFunc inGetShaderEntries)
		:GlobalShaderProgramClass(inCreateShaderObject, inSetupShaderCompileOption, inGetShaderFileName, inGetShaderEntries, 1)
	{
		ClassList.push_back(this);
	}

	GlobalShaderObjectClass::GlobalShaderObjectClass(
		CreateShaderObjectFunc inCreateShaderObject,
		SetupShaderCompileOptionFunc inSetupShaderCompileOption,
		GetShaderFileNameFunc inGetShaderFileName)
		: CreateShaderObject(inCreateShaderObject)
		, SetupShaderCompileOption(inSetupShaderCompileOption)
		, GetShaderFileName(inGetShaderFileName)
	{

	}

	GlobalShaderProgramClass::GlobalShaderProgramClass(
		CreateShaderObjectFunc inCreateShaderObject,
		SetupShaderCompileOptionFunc inSetupShaderCompileOption,
		GetShaderFileNameFunc inGetShaderFileName,
		GetShaderEntriesFunc inGetShaderEntries,
		uint32 inPermutationCount)
		: GlobalShaderObjectClass(inCreateShaderObject, inSetupShaderCompileOption, inGetShaderFileName)
		, GetShaderEntries(inGetShaderEntries)
	{
		ShaderManager::Get().registerGlobalShader(*this, inPermutationCount);
	}


	GlobalShaderClass::GlobalShaderClass(
		CreateShaderObjectFunc inCreateShaderObject,
		SetupShaderCompileOptionFunc inSetupShaderCompileOption, 
		GetShaderFileNameFunc inGetShaderFileName,
		ShaderEntryInfo inEntry,
		uint32 inPermutationCount)
		: GlobalShaderObjectClass(inCreateShaderObject, inSetupShaderCompileOption, inGetShaderFileName)
		, entry(inEntry)
	{
		ShaderManager::Get().registerGlobalShader(*this, inPermutationCount);
	}

#endif //CORE_SHARE_CODE


}//namespace RenderGL

