#pragma once
#ifndef ShadowDepthRendering_H_D0420239_63F3_4485_AF53_6673ADCDF9D2
#define ShadowDepthRendering_H_D0420239_63F3_4485_AF53_6673ADCDF9D2

#include "RHI/MaterialShader.h"

namespace Render
{
	class ShadowDepthProgram : public MaterialShaderProgram
	{
	public:
		using BaseClass = MaterialShaderProgram;
		DECLARE_EXPORTED_SHADER_PROGRAM(ShadowDepthProgram, Material, CORE_API);

		static void SetupShaderCompileOption(ShaderCompileOption& option)
		{
			BaseClass::SetupShaderCompileOption(option);
		}

		static char const* GetShaderFileName()
		{
			return "Shader/ShadowDepthRender";
		}
		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] =
			{
				{ EShader::Vertex , SHADER_ENTRY(MainVS) },
				{ EShader::Pixel  , SHADER_ENTRY(MainPS) },
			};
			return entries;
		}
	};

}//namespace Render

#endif // ShadowDepthRendering_H_D0420239_63F3_4485_AF53_6673ADCDF9D2