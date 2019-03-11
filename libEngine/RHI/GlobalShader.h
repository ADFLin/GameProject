#pragma once
#ifndef GlobalShader_H_99D977DA_B0C5_46A1_8282_C63EA1B52228
#define GlobalShader_H_99D977DA_B0C5_46A1_8282_C63EA1B52228

#include "ShaderCore.h"

#include "Template/ArrayView.h"

namespace Render
{
	class ShaderCompileOption;
	struct ShaderEntryInfo;
	
	class GlobalShaderProgram : public ShaderProgram
	{
	public:
		static GlobalShaderProgram* CreateShader() { assert(0); return nullptr; }
		static void SetupShaderCompileOption(ShaderCompileOption& option) {}
		static char const* GetShaderFileName()
		{
			assert(0);
			return nullptr;
		}
		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			assert(0);
#if 0
			static ShaderEntryInfo const entries[] =
			{
				{ Shader::eVertex , SHADER_ENTRY(MainVS) },
				{ Shader::ePixel  , SHADER_ENTRY(MainPS) },
			};
			return entries;
#else
			return TArrayView< ShaderEntryInfo const >();
#endif
		}
		class GlobalShaderProgramClass* myClass;
	};



	class GlobalShaderProgramClass
	{
	public:
		typedef GlobalShaderProgram* (*FunCreateShader)();
		typedef void(*FunSetupShaderCompileOption)(ShaderCompileOption&);
		typedef char const* (*FunGetShaderFileName)();
		typedef TArrayView< ShaderEntryInfo const > (*FunGetShaderEntries)();

		FunCreateShader funCreateShader;
		FunSetupShaderCompileOption funSetupShaderCompileOption;
		FunGetShaderFileName funGetShaderFileName;
		FunGetShaderEntries funGetShaderEntries;

		CORE_API GlobalShaderProgramClass(
			FunCreateShader inFunCreateShader,
			FunSetupShaderCompileOption inFunSetupShaderCompileOption,
			FunGetShaderFileName inFunGetShaderFileName,
			FunGetShaderEntries inFunGetShaderEntries);
	};

#define DECLARE_GLOBAL_SHADER( CLASS )\
	public:\
		static GlobalShaderProgramClass& GetShaderClass();\
		static GlobalShaderProgram* CreateShader() { return new CLASS; }

#define IMPLEMENT_GLOBAL_SHADER( CLASS )\
	Render::GlobalShaderProgramClass& CLASS::GetShaderClass()\
	{\
		static GlobalShaderProgramClass staticClass\
		{\
			CLASS::CreateShader,\
			CLASS::SetupShaderCompileOption,\
			CLASS::GetShaderFileName,\
			CLASS::GetShaderEntries,\
		};\
		return staticClass;\
	}

#define IMPLEMENT_GLOBAL_SHADER_T( TEMPLATE_ARGS , CLASS )\
	TEMPLATE_ARGS\
	IMPLEMENT_GLOBAL_SHADER( CLASS )

}//namespace Render

#endif // GlobalShader_H_99D977DA_B0C5_46A1_8282_C63EA1B52228