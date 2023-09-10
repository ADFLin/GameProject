#include "MaterialShader.h"

namespace Render
{

#if CORE_SHARE_CODE
	CORE_API TArray< MaterialShaderProgramClass* > MaterialShaderProgramClass::ClassList;

	CORE_API MaterialShaderProgramClass::MaterialShaderProgramClass(
		CreateShaderObjectFunc inCreateShaderObject,
		SetupShaderCompileOptionFunc inSetupShaderCompileOption, 
		GetShaderFileNameFunc inGetShaderFileName, 
		GetShaderEntriesFunc inGetShaderEntries,
		uint32 inPermutationCount)
		:GlobalShaderProgramClass(inCreateShaderObject, inSetupShaderCompileOption, inGetShaderFileName, inGetShaderEntries, inPermutationCount)
	{
		ClassList.push_back(this);
	}

	CORE_API GlobalShaderObjectClass::GlobalShaderObjectClass(
		CreateShaderObjectFunc inCreateShaderObject,
		SetupShaderCompileOptionFunc inSetupShaderCompileOption,
		GetShaderFileNameFunc inGetShaderFileName,
		uint32 inPermutationCount)
		: CreateShaderObject(inCreateShaderObject)
		, SetupShaderCompileOption(inSetupShaderCompileOption)
		, GetShaderFileName(inGetShaderFileName)
		, permutationCount(inPermutationCount)
	{

	}

	CORE_API GlobalShaderProgramClass::GlobalShaderProgramClass(
		CreateShaderObjectFunc inCreateShaderObject,
		SetupShaderCompileOptionFunc inSetupShaderCompileOption,
		GetShaderFileNameFunc inGetShaderFileName,
		GetShaderEntriesFunc inGetShaderEntries,
		uint32 inPermutationCount)
		: GlobalShaderObjectClass(inCreateShaderObject, inSetupShaderCompileOption, inGetShaderFileName, inPermutationCount)
		, GetShaderEntries(inGetShaderEntries)
	{
		ShaderManager::Get().registerGlobalShader(*this, inPermutationCount);
	}


	CORE_API GlobalShaderClass::GlobalShaderClass(
		CreateShaderObjectFunc inCreateShaderObject,
		SetupShaderCompileOptionFunc inSetupShaderCompileOption, 
		GetShaderFileNameFunc inGetShaderFileName,
		ShaderEntryInfo inEntry,
		uint32 inPermutationCount)
		: GlobalShaderObjectClass(inCreateShaderObject, inSetupShaderCompileOption, inGetShaderFileName, inPermutationCount)
		, entry(inEntry)
	{
		ShaderManager::Get().registerGlobalShader(*this, inPermutationCount);
	}

#endif //CORE_SHARE_CODE


}//namespace RenderGL

