#pragma once
#ifndef GlobalShader_H_99D977DA_B0C5_46A1_8282_C63EA1B52228
#define GlobalShader_H_99D977DA_B0C5_46A1_8282_C63EA1B52228

#include "ShaderProgram.h"

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
		class GlobalShaderProgramClass const* myClass;
	};

	class GlobalShaderProgramClass
	{
	public:
		typedef GlobalShaderProgram* (*CreateShaderFunc)();
		typedef void(*SetupShaderCompileOptionFunc)(ShaderCompileOption&);
		typedef char const* (*GetShaderFileNameFunc)();
		typedef TArrayView< ShaderEntryInfo const > (*GetShaderEntriesFunc)();

		CreateShaderFunc CreateShader;
		SetupShaderCompileOptionFunc SetupShaderCompileOption;
		GetShaderFileNameFunc GetShaderFileName;
		GetShaderEntriesFunc GetShaderEntries;

		CORE_API GlobalShaderProgramClass(
			CreateShaderFunc inCreateShader,
			SetupShaderCompileOptionFunc inSetupShaderCompileOption,
			GetShaderFileNameFunc inGetShaderFileName,
			GetShaderEntriesFunc inGetShaderEntries);
	};


}//namespace Render

#endif // GlobalShader_H_99D977DA_B0C5_46A1_8282_C63EA1B52228