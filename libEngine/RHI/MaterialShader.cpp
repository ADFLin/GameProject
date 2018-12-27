#include "MaterialShader.h"

namespace Render
{

#if CORE_SHARE_CODE
	std::vector< MaterialShaderProgramClass* > MaterialShaderProgramClass::ClassList;

	MaterialShaderProgramClass::MaterialShaderProgramClass(FunCreateShader inFunCreateShader, FunSetupShaderCompileOption inFunSetupShaderCompileOption, FunGetShaderFileName inFunGetShaderFileName, FunGetShaderEntries inFunGetShaderEntries)
		:GlobalShaderProgramClass(inFunCreateShader, inFunSetupShaderCompileOption, inFunGetShaderFileName, inFunGetShaderEntries)
	{
		ClassList.push_back(this);
	}

#if 1
	GlobalShaderProgramClass::GlobalShaderProgramClass(
		FunCreateShader inFunCreateShader,
		FunSetupShaderCompileOption inFunSetupShaderCompileOption,
		FunGetShaderFileName inFunGetShaderFileName,
		FunGetShaderEntries inFunGetShaderEntries)
		: funCreateShader(inFunCreateShader)
		, funSetupShaderCompileOption(inFunSetupShaderCompileOption)
		, funGetShaderFileName(inFunGetShaderFileName)
		, funGetShaderEntries(inFunGetShaderEntries)
	{
		ShaderManager::Get().registerGlobalShader(*this);
	}
#endif

#endif //CORE_SHARE_CODE



}//namespace RenderGL

