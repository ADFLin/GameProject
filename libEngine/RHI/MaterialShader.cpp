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
		:GlobalShaderProgramClass(inCreateShaderObject, inSetupShaderCompileOption, inGetShaderFileName, inGetShaderEntries)
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
		GetShaderEntriesFunc inGetShaderEntries)
		: GlobalShaderObjectClass(inCreateShaderObject, inSetupShaderCompileOption, inGetShaderFileName)
		, GetShaderEntries(inGetShaderEntries)
	{
		ShaderManager::Get().registerGlobalShader(*this);
	}


	GlobalShaderClass::GlobalShaderClass(
		CreateShaderObjectFunc inCreateShaderObject,
		SetupShaderCompileOptionFunc inSetupShaderCompileOption, 
		GetShaderFileNameFunc inGetShaderFileName,
		ShaderEntryInfo inEntry)
		: GlobalShaderObjectClass(inCreateShaderObject, inSetupShaderCompileOption, inGetShaderFileName)
		, entry(inEntry)
	{
		ShaderManager::Get().registerGlobalShader(*this);
	}

#endif //CORE_SHARE_CODE


}//namespace RenderGL

