#include "VulkanShaderCommon.h"

#include "InlineString.h"
#include "SystemPlatform.h"
#include "Platform/Windows/WindowsProcess.h"
#include "FileSystem.h"
#include "Core/ScopeGuard.h"
#include "Serialize/StreamBuffer.h"

#include "RHICommand.h"

#include "SpirvReflect/spirv_reflect.h"

#define USE_SHADERC_COMPILE 1

#if USE_SHADERC_COMPILE
#include "shaderc/shaderc.h"
#pragma comment(lib , "shaderc_shared.lib")
#endif


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

			return VK_DESCRIPTOR_TYPE_SAMPLER;
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


		bool getSetLayoutBindings(EShader::Type type, TArray<VkDescriptorSetLayoutBinding>& outBindings) const
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
				TArray< SpvReflectDescriptorBinding* > descrBindings(count);
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
				TArray< SpvReflectDescriptorBinding* > descrBindings(count);
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
				TArray< SpvReflectInterfaceVariable* > variables(count);
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
				TArray< SpvReflectInterfaceVariable* > variables(count);
				moduleReflect.EnumerateOutputVariables(&count, variables.data());
				for (uint32 i = 0; i < count; ++i)
				{
					SpvReflectInterfaceVariable* variable = variables[i];
					LogMsg("Output Variable: name=%s, location=%u", variable->name, variable->location);
				}
			}
		}
		TArray< char > codeBuffer;
	};

	void ShaderFormatSpirv::setupShaderCompileOption(ShaderCompileOption& option)
	{

	}

	void ShaderFormatSpirv::getHeadCode(std::string& inoutCode, ShaderCompileOption const& option, ShaderEntryInfo const& entry)
	{
		inoutCode += "#version 450\n";
		inoutCode += "#extension GL_ARB_separate_shader_objects : enable\n";

		inoutCode += "#define COMPILER_GLSL 1\n";
		inoutCode += "#define SYSTEM_VULKAN 1\n";

		if (entry.name && FCString::Compare(entry.name, "main") != 0)
		{
			inoutCode += "#define ";
			inoutCode += entry.name;
			inoutCode += " main\n";
		}
	}

	class VulkanShaderCompileIntermediates : public ShaderCompileIntermediates
	{
	public:
		VulkanShaderCompileIntermediates()
		{
			numShaders = 0;
		}
		SpirvShaderCode shaderCodes[EShader::MaxStorageSize];
		int numShaders;
	};


	void ShaderFormatSpirv::precompileCode(ShaderProgramSetupData& setupData)
	{
		setupData.intermediateData = std::make_unique<VulkanShaderCompileIntermediates>();
	}

	EShaderCompileResult ShaderFormatSpirv::compileCode(ShaderCompileContext const& context)
	{
#if USE_SHADERC_COMPILE
		CHECK(context.codes.size() == 1);

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

		shaderc_compilation_result_t resultHandle = shaderc_compile_into_spv(
			compilerHandle, context.codes[0].data(), context.codes[0].size(),
			ShaderKindMap[context.getType()], context.getPath(), "main", optionHandle);

		ON_SCOPE_EXIT
		{
			shaderc_result_release(resultHandle);
		};

		if (shaderc_result_get_compilation_status(resultHandle) != shaderc_compilation_status_success)
		{
			emitCompileError(context, shaderc_result_get_error_message(resultHandle) );
			return EShaderCompileResult::CodeError;
		}

		char const* pCodeText = shaderc_result_get_bytes(resultHandle);
		size_t codeLength = shaderc_result_get_length(resultHandle);
		if (context.programSetupData)
		{
			VulkanShaderCompileIntermediates* myIntermediates = static_cast<VulkanShaderCompileIntermediates*>(context.programSetupData->intermediateData.get());
			SpirvShaderCode& code = myIntermediates->shaderCodes[myIntermediates->numShaders];
			code.codeBuffer.assign(pCodeText, pCodeText + codeLength);
			++myIntermediates->numShaders;
		}
		else
		{
			SpirvShaderCode code;
			code.codeBuffer.assign(pCodeText, pCodeText + codeLength);
			auto& shaderImpl = static_cast<VulkanShader&>(*context.shaderSetupData->resource);
			VERIFY_FAILCODE(shaderImpl.initialize(mDevice, context.getType(), code) , return EShaderCompileResult::ResourceError);
		}

#else
		std::string pathFull = FFileSystem::ConvertToFullPath(context.getPath());
		char const* posExtension = FFileUtility::GetExtension(pathFull.c_str());
		std::string pathCompile;

		pathCompile = pathFull.substr(0, posExtension - &pathFull[0]) + "prep" + SHADER_FILE_SUBNAME;
		FFileUtility::SaveFromBuffer(pathCompile.c_str(), context.codes[0].data(), context.codes[0].size());

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

		InlineString<1024> command;
		command.format(" -V -S %s -o %s %s", ShaderNames[context.type], pathSpv.c_str(), pathCompile.c_str());

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

			emitCompileError(context, outputBuffer);
			return EShaderCompileResult::CodeError;
		}

		ON_SCOPE_EXIT
		{
			FFileSystem::DeleteFile(pathSpv.c_str());
		};

		if (context.programSetupData)
		{
			VulkanShaderCompileIntermediates* myIntermediates = static_cast<VulkanShaderCompileIntermediates*>(context.programSetupData->intermediateData.get());
			SpirvShaderCode& code = myIntermediates->shaderCodes[ myIntermediates->numShaders ];
			VERIFY_FAILCODE(FFileUtility::LoadToBuffer(pathSpv.c_str(), code.codeBuffer), return EShaderCompileResult::ResourceError);
			++myIntermediates->numShaders;
		}
		else
		{
			SpirvShaderCode code;
			VERIFY_FAILCODE(FFileUtility::LoadToBuffer(pathSpv.c_str(), code.codeBuffer), return EShaderCompileResult::ResourceError);
			auto& shaderImpl = static_cast<VulkanShader&>(*context.shaderSetupData->resource);
			VERIFY_FAILCODE(shaderImpl.initialize(mDevice, context.type, code), return EShaderCompileResult::ResourceError);

			context.shaderSetupData->resource = shaderImpl;
		}

#endif
		return EShaderCompileResult::Ok;
	}



	ShaderParameterMap* ShaderFormatSpirv::initializeProgram(RHIShaderProgram& shaderProgram, ShaderProgramSetupData& setupData)
	{
		auto& shaderProgramImpl = static_cast<VulkanShaderProgram&>(shaderProgram);
		auto* intermediates = static_cast<VulkanShaderCompileIntermediates*>(setupData.intermediateData.get());
		if (!shaderProgramImpl.setupShaders(mDevice, setupData.descList, intermediates->shaderCodes))
			return nullptr;

		return nullptr;
	}

	ShaderParameterMap* ShaderFormatSpirv::initializeProgram(RHIShaderProgram& shaderProgram, TArray< ShaderCompileDesc > const& descList, TArray<uint8> const& binaryCode)
	{
		VulkanShaderProgram& shaderProgramImpl = static_cast<VulkanShaderProgram&>(shaderProgram);

		auto serializer = CreateBufferSerializer<SimpleReadBuffer>(MakeConstView(binaryCode));
		uint8 numShaders = 0;
		serializer.read(numShaders);

		SpirvShaderCode shaderCodes[EShader::MaxStorageSize];
		for (int i = 0; i < numShaders; ++i)
		{
			uint8 shaderType;
			serializer.read(shaderType);
			serializer.read(shaderCodes[i].codeBuffer);
		}

		if (!shaderProgramImpl.setupShaders(mDevice, MakeConstView(descList) , shaderCodes))
			return nullptr;


		return nullptr;
	}

	void ShaderFormatSpirv::postShaderLoaded(RHIShaderProgram& shaderProgram)
	{

	}

	bool ShaderFormatSpirv::doesSuppurtBinaryCode() const
	{

		return true;
	}

	bool ShaderFormatSpirv::getBinaryCode(ShaderProgramSetupData& setupData, TArray<uint8>& outBinaryCode)
	{
		auto* intermediates = static_cast<VulkanShaderCompileIntermediates*>(setupData.intermediateData.get());

		auto serializer = CreateBufferSerializer<ArrayWriteBuffer>(outBinaryCode);
		uint8 numShaders = intermediates->numShaders;
		serializer.write(numShaders);
		for (int i = 0; i < numShaders; ++i)
		{
			SpirvShaderCode& shaderCode = intermediates->shaderCodes[i];
			uint8 shaderType = setupData.descList[i].type;
			serializer.write(shaderType);
			serializer.write(shaderCode.codeBuffer);
		}

		return true;
	}

	bool VulkanShader::initialize(VkDevice device, EShader::Type type, SpirvShaderCode const& code)
	{
		mDevice = device;
		VERIFY_RETURN_FALSE( code.createModel(mDevice, mModule) );
		TArray<VkDescriptorSetLayoutBinding> setLayoutBindings;
		VERIFY_RETURN_FALSE(code.getSetLayoutBindings(type, setLayoutBindings) );
		VkDescriptorSetLayoutCreateInfo descriptorLayout =
			FVulkanInit::descriptorSetLayoutCreateInfo(
				setLayoutBindings.data(),
				static_cast<uint32>(setLayoutBindings.size()));
		VERIFY_RETURN_FALSE(mDescriptorSetLayout.initialize(device, descriptorLayout));

		return true;
	}

	bool VulkanShaderProgram::setupShaders(VkDevice device, TArrayView< ShaderCompileDesc const >& descList, SpirvShaderCode shaderCodes[])
	{
		mDevice = device;
		assert( mNumShaders == 0);

		TArray<VkDescriptorSetLayoutBinding> setLayoutBindings;
		for (int i = 0; i < descList.size(); ++i)
		{
			SpirvShaderCode& shaderCode = shaderCodes[i];
			VERIFY_RETURN_FALSE(shaderCode.createModel(device, mShaderModules[i]));
			VERIFY_RETURN_FALSE(shaderCode.getSetLayoutBindings(descList[i].type, setLayoutBindings));

			VkPipelineShaderStageCreateInfo shaderStage = {};
			shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			shaderStage.module = mShaderModules[i].mHandle;
			shaderStage.stage = VulkanTranslate::To(descList[i].type);
			shaderStage.pName = "main";
			mStages.push_back(shaderStage);
		}

		mNumShaders = descList.size();
		VkDescriptorSetLayoutCreateInfo descriptorLayout =
			FVulkanInit::descriptorSetLayoutCreateInfo(
				setLayoutBindings.data(),
				static_cast<uint32>(setLayoutBindings.size()));
		VERIFY_RETURN_FALSE(mDescriptorSetLayout.initialize(device, descriptorLayout));

		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo =
			FVulkanInit::pipelineLayoutCreateInfo(
				&mDescriptorSetLayout.mHandle,
				1);
		VERIFY_RETURN_FALSE(mPipelineLayout.initialize(device, pipelineLayoutCreateInfo));

		return true;
	}

}//namespace Render

