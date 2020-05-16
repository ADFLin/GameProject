#pragma once
#ifndef MaterialShader_H_B9594273_3899_4630_B560_D67F4FA887CE
#define MaterialShader_H_B9594273_3899_4630_B560_D67F4FA887CE

#include "GlobalShader.h"

#include "Core/TypeHash.h"
#include "CoreShare.h"
#include <unordered_map>

#include "LogSystem.h"
//#MOVE

namespace Render
{
	class VertexFactoryType;
	class VertexFactory;

	class MaterialShaderProgram : public GlobalShaderProgram
	{
		typedef GlobalShaderProgram BaseClass;
	public:

		static void SetupShaderCompileOption(ShaderCompileOption& option)
		{
			BaseClass::SetupShaderCompileOption(option);
			option.addDefine(SHADER_PARAM(USE_MATERIAL), true);
		}

		struct ShaderParamBind
		{
			ShaderParameter parameter;
		};

	};


	class MaterialShaderProgramClass : public GlobalShaderProgramClass
	{
	public:
		CORE_API MaterialShaderProgramClass(
			CreateShaderFunc inCreateShader,
			SetupShaderCompileOptionFunc inSetupShaderCompileOption,
			GetShaderFileNameFunc inGetShaderFileName,
			GetShaderEntriesFunc inGetShaderEntries);

		CORE_API static std::vector< MaterialShaderProgramClass* > ClassList;
	};

}//namespace Render

#endif // MaterialShader_H_B9594273_3899_4630_B560_D67F4FA887CE
