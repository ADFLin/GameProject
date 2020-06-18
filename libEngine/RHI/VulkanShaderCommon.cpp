#include "VulkanShaderCommon.h"

#include "FixString.h"
#include "SystemPlatform.h"
#include "Platform/Windows/WindowsProcess.h"
#include "FileSystem.h"
#include "Core/ScopeExit.h"
#include "Serialize/StreamBuffer.h"

#include "RHICommand.h"

#include "SpirvReflect/spirv_reflect.h"
#include "ShaderProgram.h"

namespace Render
{
	struct FSpvReflect
	{
		static char const* ToString(SpvReflectDescriptorType type)
		{
			switch (type)
			{
			case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER: return "Sampler";
			case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER: return "CombinedImageSampler";
			case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE: return "SampledImage";
			case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE: return "StorageImage";
			case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER: return "UniformTexelBuffer";
			case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER: return "StorageTexelBuffer";
			case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER: return "UniformBuffer";
			case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER: return "StorageBuffer";
			case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC: return "UniformBufferDynamic";
			case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC: return "StorageBufferDynamic";
			case SPV_REFLECT_DESCRIPTOR_TYPE_INPUT_ATTACHMENT: return "InputAttachment";
			}


			return "unknown descriptor type";
		}
		static VkDescriptorType Translate(SpvReflectDescriptorType type)
		{
			switch (type)
			{
			case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER: return VK_DESCRIPTOR_TYPE_SAMPLER;
			case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER: return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE: return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
			case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE: return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER: return VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
			case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER: return VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
			case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER: return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER: return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC: return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
			case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC: return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
			case SPV_REFLECT_DESCRIPTOR_TYPE_INPUT_ATTACHMENT: return VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
			}
		}
	};



	struct SpirvShaderCode
	{

		bool createModel(VkDevice device, VK_RESOURCE_TYPE(VkShaderModule)& outModule) const
		{
			VkShaderModuleCreateInfo moduleInfo = {};
			moduleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			moduleInfo.pCode = (uint32 const*)codeBuffer.data();
			moduleInfo.codeSize = codeBuffer.size();
			VERIFY_RETURN_FALSE(outModule.initialize(device, moduleInfo));
			return true;
		}


		bool getSetLayoutBindings(Shader::Type type, std::vector<VkDescriptorSetLayoutBinding>& outBindings) const
		{
			spv_reflect::ShaderModule moduleReflect(codeBuffer.size(), codeBuffer.data());
			if (moduleReflect.GetResult() != SPV_REFLECT_RESULT_SUCCESS)
			{
				return false;
			}
			uint32 count;
			moduleReflect.EnumerateDescriptorBindings(&count, nullptr);
			if (count)
			{
				auto stageStage = VulkanTranslate::To(type);
				std::vector< SpvReflectDescriptorBinding* > descrBindings(count);
				moduleReflect.EnumerateDescriptorBindings(&count, descrBindings.data());
				for (uint32 i = 0; i < count; ++i)
				{
					SpvReflectDescriptorBinding* descrBinding = descrBindings[i];
					outBindings.push_back(
						FVulkanInit::descriptorSetLayoutBinding(
							FSpvReflect::Translate(descrBinding->descriptor_type),
							stageStage,
							descrBinding->binding));
				}
			}
			return true;
		}

		void dumpReflectInformation()
		{
			spv_reflect::ShaderModule moduleReflect(codeBuffer.size(), codeBuffer.data());
			if (moduleReflect.GetResult() != SPV_REFLECT_RESULT_SUCCESS)
			{
				return;
			}
			uint32 count;
			moduleReflect.EnumerateDescriptorBindings(&count, nullptr);
			if (count)
			{
				std::vector< SpvReflectDescriptorBinding* > descrBindings(count);
				moduleReflect.EnumerateDescriptorBindings(&count, descrBindings.data());
				for (uint32 i = 0; i < count; ++i)
				{
					SpvReflectDescriptorBinding* descrBinding = descrBindings[i];
					LogMsg("binding: name = %s, set=%u, binding = %u, desc type=%s", 
						descrBinding->name, descrBinding->set, descrBinding->binding, FSpvReflect::ToString(descrBinding->descriptor_type));
				}
			}

			moduleReflect.EnumerateInputVariables(&count, nullptr);
			if (count)
			{
				std::vector< SpvReflectInterfaceVariable* > variables(count);
				moduleReflect.EnumerateInputVariables(&count, variables.data());
				for (uint32 i = 0; i < count; ++i)
				{
					SpvReflectInterfaceVariable* variable = variables[i];
					LogMsg("Input Variable: name=%s, location=%u", variable->name, variable->location);
				}
			}

			moduleReflect.EnumerateOutputVariables(&count, nullptr);
			if (count)
			{
				std::vector< SpvReflectInterfaceVariable* > variables(count);
				moduleReflect.EnumerateOutputVariables(&count, variables.data());
				for (uint32 i = 0; i < count; ++i)
				{
					SpvReflectInterfaceVariable* variable = variables[i];
					LogMsg("Output Variable: name=%s, location=%u", variable->name, variable->location);
				}
			}
		}
		std::vector< char > codeBuffer;
	};

	void ShaderFormatSpirv::setupShaderCompileOption(ShaderCompileOption& option)
	{

	}

	void ShaderFormatSpirv::getHeadCode(std::string& inoutCode, ShaderCompileOption const& option, ShaderEntryInfo const& entry)
	{

	}

	class VulkanShaderCompileIntermediates : public ShaderCompileIntermediates
	{
	public:
		VulkanShaderCompileIntermediates()
		{
			numShaders = 0;
		}
		SpirvShaderCode shaderCodes[Shader::MaxStorageSize];
		int numShaders;
	};


	void ShaderFormatSpirv::precompileCode(ShaderProgramSetupData& setupData)
	{
		setupData.intermediateData = std::make_unique<VulkanShaderCompileIntermediates>();
	}

	bool ShaderFormatSpirv::compileCode(ShaderCompileInput const& input, ShaderCompileOutput& output)
	{
#if 1
		std::string pathFull = FileUtility::GetFullPath(input.path);
		char const* posExtension = FileUtility::GetExtension(pathFull.c_str());

		std::string pathCompile;
		if (bUsePreprocess)
		{
			std::string pathPrep = pathFull.substr(0, posExtension - &pathFull[0]) + "prep" + SHADER_FILE_SUBNAME;

			std::vector<char> codeBuffer;
			if (!FileUtility::LoadToBuffer(input.path, codeBuffer, true))
			{
				LogWarning(0, "Can't load shader file %s", input.path);
				return false;
			}
			if (!PreprocessCode(input.path, output.compileInfo, input.definition, codeBuffer))
			{



			}

			FileUtility::SaveFromBuffer(pathPrep.c_str(), codeBuffer.data(), codeBuffer.size());

			pathCompile = std::move(pathPrep);
		}
		else
		{
			pathCompile = pathFull;
		}


		std::string pathSpv = pathFull.substr(0, posExtension - &pathFull[0]) + "spv";
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
		command.format(" -V -S %s -o %s %s", ShaderNames[input.type], pathSpv.c_str(), pathCompile.c_str());

		ChildProcess process;
		VERIFY_RETURN_FALSE(process.create(GlslangValidatorPath.c_str(), command));

		process.waitCompletion();
		int32 exitCode;
		if (!process.getExitCode(exitCode) || exitCode != 0)
		{
			char outputBuffer[4096];
			int readSize = 0;
			process.readOutputStream(outputBuffer, ARRAY_SIZE(outputBuffer), readSize);
			outputBuffer[readSize] = 0;

			OutputError(outputBuffer);
			return false;
		}

		ON_SCOPE_EXIT
		{
			FileSystem::DeleteFile(pathSpv.c_str());
		};

		if (input.setupData)
		{
			VulkanShaderCompileIntermediates* myIntermediates = static_cast<VulkanShaderCompileIntermediates*>(input.setupData->intermediateData.get());
			SpirvShaderCode& code = myIntermediates->shaderCodes[ myIntermediates->numShaders ];
			VERIFY_RETURN_FALSE(FileUtility::LoadToBuffer(pathSpv.c_str(), code.codeBuffer));
			++myIntermediates->numShaders;
			output.formatData = &code;
		}
		else
		{
			SpirvShaderCode code;
			VERIFY_RETURN_FALSE(FileUtility::LoadToBuffer(pathSpv.c_str(), code.codeBuffer));
			auto* shaderImpl = static_cast<VulkanShader*>(RHICreateShader(input.type));
			output.resource = shaderImpl;
			VERIFY_RETURN_FALSE(shaderImpl->initialize(mDevice, input.type, code));
		}


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



	bool ShaderFormatSpirv::initializeProgram(ShaderProgram& shaderProgram, ShaderProgramSetupData& setupData)
	{
		auto& shaderProgramImpl = static_cast<VulkanShaderProgram&>(*shaderProgram.mRHIResource);

		if (!shaderProgramImpl.setupShaders(mDevice, setupData.shaders.data(), setupData.shaders.size()))
			return false;

		return true;
	}

	bool ShaderFormatSpirv::initializeProgram(ShaderProgram& shaderProgram, std::vector< ShaderCompileInfo > const& shaderCompiles, std::vector<uint8> const& binaryCode)
	{
		VulkanShaderProgram& shaderProgramImpl = static_cast<VulkanShaderProgram&>(*shaderProgram.mRHIResource);

		auto serializer = CreateBufferSerializer<SimpleReadBuffer>(MakeView(binaryCode));
		uint8 numShaders = 0;
		serializer.read(numShaders);

		ShaderResourceInfo shaders[Shader::MaxStorageSize];
		SpirvShaderCode shaderCodes[Shader::MaxStorageSize];
		for (int i = 0; i < numShaders; ++i)
		{
			uint8 shaderType;
			serializer.read(shaderType);
			serializer.read(shaderCodes[i].codeBuffer);
			shaders[i].formatData = &shaderCodes[i];
			shaders[i].type  = Shader::Type(shaderType);
			shaders[i].entry = shaderCompiles[i].entryName.c_str();
		}

		if (!shaderProgramImpl.setupShaders(mDevice, shaders, numShaders))
			return false;


		return true;
	}

	void ShaderFormatSpirv::postShaderLoaded(ShaderProgram& shaderProgram)
	{

	}

	bool ShaderFormatSpirv::isSupportBinaryCode() const
	{

		return true;
	}

	bool ShaderFormatSpirv::getBinaryCode(ShaderProgram& shaderProgram, ShaderProgramSetupData& setupData, std::vector<uint8>& outBinaryCode)
	{
		auto serializer = CreateBufferSerializer<VectorWriteBuffer>(outBinaryCode);
		uint8 numShaders = setupData.shaders.size();
		serializer.write(numShaders);
		for (int i = 0; i < numShaders; ++i)
		{
			SpirvShaderCode* shaderCode = static_cast<SpirvShaderCode*>(setupData.shaders[i].formatData);

			uint8 shaderType = setupData.shaders[i].type;
			serializer.write(shaderType);
			serializer.write(shaderCode->codeBuffer);
		}

		return true;
	}

	bool VulkanShader::initialize(VkDevice device, Shader::Type type, SpirvShaderCode const& code)
	{
		mDevice = device;
		VERIFY_RETURN_FALSE( code.createModel(mDevice, mModule) );
		return true;
	}

	bool VulkanShaderProgram::setupShaders(VkDevice device, ShaderResourceInfo shaders[], int numShaders)
	{
		mDevice = device;
		assert( mNumShaders == 0);

		for (int i = 0; i < numShaders; ++i)
		{
			SpirvShaderCode* shaderCode = static_cast<SpirvShaderCode*>(shaders[i].formatData);
			if (!shaderCode->createModel(device, mShaderModules[i]))
				return false;

			VkPipelineShaderStageCreateInfo shaderStage = {};
			shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			shaderStage.module = mShaderModules[i].mHandle;
			shaderStage.stage = VulkanTranslate::To(shaders[i].type);
			shaderStage.pName = "main";
			mStages.push_back(shaderStage);
			++mNumShaders;
		}

		return true;
	}

}//namespace Render

