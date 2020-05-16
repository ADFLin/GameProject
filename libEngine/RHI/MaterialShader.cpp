#include "MaterialShader.h"

namespace Render
{

#if CORE_SHARE_CODE
	std::vector< MaterialShaderProgramClass* > MaterialShaderProgramClass::ClassList;

	MaterialShaderProgramClass::MaterialShaderProgramClass(
		CreateShaderFunc inCreateShader, 
		SetupShaderCompileOptionFunc inSetupShaderCompileOption, 
		GetShaderFileNameFunc inGetShaderFileName, 
		GetShaderEntriesFunc inGetShaderEntries)
		:GlobalShaderProgramClass(inCreateShader, inSetupShaderCompileOption, inGetShaderFileName, inGetShaderEntries)
	{
		ClassList.push_back(this);
	}

#if 1
	GlobalShaderProgramClass::GlobalShaderProgramClass(
		CreateShaderFunc inCreateShader,
		SetupShaderCompileOptionFunc inSetupShaderCompileOption,
		GetShaderFileNameFunc inGetShaderFileName,
		GetShaderEntriesFunc inGetShaderEntries)
		: CreateShader(inCreateShader)
		, SetupShaderCompileOption(inSetupShaderCompileOption)
		, GetShaderFileName(inGetShaderFileName)
		, GetShaderEntries(inGetShaderEntries)
	{
		ShaderManager::Get().registerGlobalShader(*this);
	}
#endif

#endif //CORE_SHARE_CODE


}//namespace RenderGL

