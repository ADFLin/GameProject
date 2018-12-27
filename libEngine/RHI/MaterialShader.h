#pragma once
#ifndef MaterialShader_H_B9594273_3899_4630_B560_D67F4FA887CE
#define MaterialShader_H_B9594273_3899_4630_B560_D67F4FA887CE

#include "GlobalShader.h"

#include "TypeHash.h"
#include "CoreShare.h"
#include <unordered_map>

//#MOVE

namespace Render
{
	class VertexFactoryType;
	class VertexFactory;

	class MaterialShaderProgram : public GlobalShaderProgram
	{
	public:
		static void SetupShaderCompileOption(ShaderCompileOption& option)
		{
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
			FunCreateShader inFunCreateShader,
			FunSetupShaderCompileOption inFunSetupShaderCompileOption,
			FunGetShaderFileName inFunGetShaderFileName,
			FunGetShaderEntries inFunGetShaderEntries);

		CORE_API static std::vector< MaterialShaderProgramClass* > ClassList;
	};


#define DECLARE_MATERIAL_SHADER( CLASS ) \
	public: \
		static MaterialShaderProgramClass& GetShaderClass(); \
		static CLASS* CreateShader() { return new CLASS; }

#define IMPLEMENT_MATERIAL_SHADER( CLASS )\
	static MaterialShaderProgramClass gMaterialName##CLASS = \
	MaterialShaderProgramClass{ \
		(GlobalShaderProgramClass::FunCreateShader) &CLASS::CreateShader,\
		CLASS::SetupShaderCompileOption,\
		CLASS::GetShaderFileName, \
		CLASS::GetShaderEntries \
	}; \
	Render::MaterialShaderProgramClass& CLASS::GetShaderClass(){ return gMaterialName##CLASS; }

#define IMPLEMENT_MATERIAL_SHADER_T( TEMPLATE_ARGS , CLASS )\
	TEMPLATE_ARGS\
	IMPLEMENT_MATERIAL_SHADER( CLASS )





}//namespace Render

#endif // MaterialShader_H_B9594273_3899_4630_B560_D67F4FA887CE
