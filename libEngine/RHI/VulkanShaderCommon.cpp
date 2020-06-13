#include "VulkanShaderCommon.h"

#include "FixString.h"
#include "SystemPlatform.h"
#include "Platform/Windows/WindowsProcess.h"
#include "FileSystem.h"

namespace Render
{

	bool ShaderFormatSpirv::compileCode(Shader::Type type, RHIShader& shader, char const* path, ShaderCompileInfo* compileInfo, char const* def)
	{
#if 1
		std::string fullPath = FileUtility::GetFullPath(path);
		char const* dirEndPos = FileUtility::GetFileName(fullPath.c_str());
		std::string outPath = fullPath.substr(0, dirEndPos - &fullPath[0]) + "/shader.tmp";

		char const* VulkanSDKDir = SystemPlatform::GetEnvironmentVariable("VULKAN_SDK");
		if (VulkanSDKDir == nullptr)
		{
			VulkanSDKDir = "C:/VulkanSDK/1.1.130.0";
		}
		std::string GlslangValidatorPath = VulkanSDKDir;
		GlslangValidatorPath += "/Bin/glslangValidator.exe";
		static char const* const ShaderNames[] =
		{
			"vert","frag","geom","comp","tesc","tese"
		};

		FixString<1024> command;
		command.format(" -V -S %s -o %s %s", ShaderNames[type], outPath.c_str(), path);

		ChildProcess process;
		VERIFY_RETURN_FALSE(process.create(GlslangValidatorPath.c_str(), command));

		process.waitCompletion();
		int32 exitCode;
		if (!process.getExitCode(exitCode) || exitCode != 0)
		{
			return false;
		}

		std::vector<char> code;
		VERIFY_RETURN_FALSE(FileUtility::LoadToBuffer(outPath.c_str(), code));

		auto& shaderImpl = static_cast<VulkanShader&>(shader);
		VERIFY_RETURN_FALSE(shaderImpl.initialize(mDevice, type, MakeConstView(code)));


		return true;
#else
		std::vector<char> scoreCode;
		if (!FileUtility::LoadToBuffer(path, scoreCode))
			return VK_NULL_HANDLE;

		shaderc_compiler_t compilerHandle = shaderc_compiler_initialize();
		shaderc_compile_options_t optionHandle = shaderc_compile_options_initialize();

		ON_SCOPE_EXIT
		{
			shaderc_compiler_release(compilerHandle);
			shaderc_compile_options_release(optionHandle);
		};
		shaderc_shader_kind ShaderKindMap[] =
		{
			shaderc_vertex_shader,
			shaderc_fragment_shader,
			shaderc_geometry_shader,
			shaderc_compute_shader,
			shaderc_tess_control_shader,
			shaderc_tess_evaluation_shader,
		};

		shaderc_compilation_result_t resultHandle = shaderc_compile_into_spv(compilerHandle, scoreCode.data(), scoreCode.size(), ShaderKindMap[type], nullptr, nullptr, optionHandle);
		ON_SCOPE_EXIT
		{
			shaderc_result_release(resultHandle);
		};

		if (shaderc_result_get_compilation_status(resultHandle) != shaderc_compilation_status_success)
		{
			return false;
		}

		return true;
#endif
	}


}//namespace Render

