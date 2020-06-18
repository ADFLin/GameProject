#pragma once
#ifndef VulkanShaderCommon_H_49227E41_76E2_4638_AD24_612B97726E22
#define VulkanShaderCommon_H_49227E41_76E2_4638_AD24_612B97726E22

#include "ShaderCore.h"
#include "ShaderFormat.h"

#include "VulkanCommon.h"
#include "Template/ArrayView.h"

namespace Render
{
	struct SpirvShaderCode;

	template<>
	struct TVulkanResourceTraits<EVulkanResourceType::eVkShaderModule>
	{
		typedef VkShaderModule HandleType;

		static bool Create(VkDevice device, VkShaderModule& module, VkShaderModuleCreateInfo const& info)
		{
			VK_VERIFY_RETURN_FALSE(vkCreateShaderModule(device, &info, gAllocCB, &module));
			return true;
		}
		static void Destroy(VkDevice device, VkShaderModule& module)
		{
			vkDestroyShaderModule(device, module, gAllocCB);
		}
	};


	class VulkanShader : public TRefcountResource< RHIShader >
	{
	public:

		bool initialize(VkDevice device, Shader::Type type, SpirvShaderCode const& code);
		void releaseResource()
		{
			mModule.destroy(mDevice);
		}

		ShaderParameterMap mParameterMap;
		VK_RESOURCE_TYPE(VkShaderModule) mModule;
		VkDevice       mDevice;
	};

	class VulkanShaderProgram : public TRefcountResource< RHIShaderProgram >
	{
	public:

		VK_RESOURCE_TYPE(VkShaderModule) mShaderModules[Shader::MaxStorageSize];
		std::vector< VkPipelineShaderStageCreateInfo > mStages;
		int mNumShaders = 0;

		bool setupShaders(VkDevice device, ShaderResourceInfo shaders[], int numShaders);
		virtual bool getParameter(char const* name, ShaderParameter& outParam) override
		{

			return false;
		}
		virtual bool getResourceParameter(EShaderResourceType resourceType, char const* name, ShaderParameter& outParam) override
		{
			return false;
		}

		void releaseResource()
		{
			for (int i = 0; i < mNumShaders; ++i)
			{
				mShaderModules[i].destroy(mDevice);
			}
		}
		VkDevice mDevice;
	};

	class ShaderFormatSpirv : public ShaderFormat
	{
	public:
		ShaderFormatSpirv(VkDevice device)
			:mDevice(device)
		{
			bUsePreprocess = false;
		}
		virtual char const* getName() final { return "spir-v"; }
		virtual void setupShaderCompileOption(ShaderCompileOption& option) final;
		virtual void getHeadCode(std::string& inoutCode, ShaderCompileOption const& option, ShaderEntryInfo const& entry) final;

		virtual bool compileCode(ShaderCompileInput const& input, ShaderCompileOutput& output) final;
		virtual void precompileCode(ShaderProgramSetupData& setupData);
		virtual bool initializeProgram(ShaderProgram& shaderProgram, ShaderProgramSetupData& setupData);
		virtual bool initializeProgram(ShaderProgram& shaderProgram, std::vector< ShaderCompileInfo > const& shaderCompiles, std::vector<uint8> const& binaryCode) final;

		virtual void postShaderLoaded(ShaderProgram& shaderProgram) final;

		virtual bool isSupportBinaryCode() const final;
		virtual bool getBinaryCode(ShaderProgram& shaderProgram, ShaderProgramSetupData& setupData, std::vector<uint8>& outBinaryCode) final;



		VkDevice       mDevice;
	};

}//namespace Render


#endif // VulkanShaderCommon_H_49227E41_76E2_4638_AD24_612B97726E22
