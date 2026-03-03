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
	struct TVulkanResourceTraits<EVulkanResourceType::VkShaderModule>
	{
		typedef VkShaderModule HandleType;

		static bool Create(VkDevice device, HandleType& handle, VkShaderModuleCreateInfo const& info)
		{
			VK_VERIFY_RETURN_FALSE(vkCreateShaderModule(device, &info, gAllocCB, &handle));
			return true;
		}
		static void Destroy(VkDevice device, HandleType& handle)
		{
			vkDestroyShaderModule(device, handle, gAllocCB);
		}
	};

	template<>
	struct TVulkanResourceTraits<EVulkanResourceType::VkDescriptorSetLayout>
	{
		typedef VkDescriptorSetLayout HandleType;

		static bool Create(VkDevice device, HandleType& handle, VkDescriptorSetLayoutCreateInfo const& info)
		{
			VK_VERIFY_RETURN_FALSE(vkCreateDescriptorSetLayout(device, &info, gAllocCB, &handle));
			return true;
		}
		static void Destroy(VkDevice device, HandleType& handle)
		{
			vkDestroyDescriptorSetLayout(device, handle, gAllocCB);
		}
	};

	template<>
	struct TVulkanResourceTraits<EVulkanResourceType::VkPipelineLayout>
	{
		typedef VkPipelineLayout HandleType;

		static bool Create(VkDevice device, HandleType& handle, VkPipelineLayoutCreateInfo const& info)
		{
			VK_VERIFY_RETURN_FALSE(vkCreatePipelineLayout(device, &info, gAllocCB, &handle));
			return true;
		}
		static void Destroy(VkDevice device, HandleType& handle)
		{
			vkDestroyPipelineLayout(device, handle, gAllocCB);
		}
	};

	class VulkanShader : public TRefcountResource< RHIShader >
	{
	public:

		bool initialize(VkDevice device, EShader::Type type, char const* entryName, SpirvShaderCode const& code);
		void releaseResource() override
		{
			mModule.destroy(mDevice);
			mDescriptorSetLayout.destroy(mDevice);
			mPipelineLayout.destroy(mDevice);
		}

		virtual bool getParameter(char const* name, ShaderParameter& outParam) override;
		virtual bool getResourceParameter(EShaderResourceType resourceType, char const* name, ShaderParameter& outParam) override;

		ShaderParameterMap mParameterMap;
		VK_RESOURCE_TYPE(VkShaderModule)        mModule;
		VK_RESOURCE_TYPE(VkDescriptorSetLayout) mDescriptorSetLayout;
		VK_RESOURCE_TYPE(VkPipelineLayout)      mPipelineLayout;
		TArray<VkDescriptorSetLayoutBinding>    mDescriptorBindings;
		std::string     mEntryPoint;
		VkDevice        mDevice;
	};

	class VulkanShaderProgram : public TRefcountResource< RHIShaderProgram >
	{
	public:

		bool setupShaders(VkDevice device, TArrayView< ShaderCompileDesc const >& descList, SpirvShaderCode shaderCodes[]);
		virtual bool getParameter(char const* name, ShaderParameter& outParam) override
		{
			ShaderParameter* param = mParameterMap.findParameter(name);
			if (param)
			{
				outParam = *param;
				return true;
			}
			return false;
		}

		virtual bool getResourceParameter(EShaderResourceType resourceType, char const* name, ShaderParameter& outParam) override
		{
			return getParameter(name, outParam);
		}

		void releaseResource() override
		{
			for (int i = 0; i < mNumShaders; ++i)
			{
				mShaderModules[i].destroy(mDevice);
			}
			mDescriptorSetLayout.destroy(mDevice);
			mPipelineLayout.destroy(mDevice);
		}

		ShaderPorgramParameterMap mParameterMap;
		VK_RESOURCE_TYPE(VkShaderModule) mShaderModules[EShader::MaxStorageSize];
		int mNumShaders = 0;

		VK_RESOURCE_TYPE(VkDescriptorSetLayout) mDescriptorSetLayout;
		VK_RESOURCE_TYPE(VkPipelineLayout)      mPipelineLayout;
		TArray<VkDescriptorSetLayoutBinding>    mDescriptorBindings;

		TArray< VkPipelineShaderStageCreateInfo > mStages;
		VkDevice mDevice;
	};

	class VulkanShaderBoundState
	{
	public:
		VulkanShaderBoundState() = default;

		void release(VkDevice device)
		{
			mDescriptorSetLayout.destroy(device);
			mPipelineLayout.destroy(device);
		}

		VK_RESOURCE_TYPE(VkDescriptorSetLayout) mDescriptorSetLayout;
		VK_RESOURCE_TYPE(VkPipelineLayout)      mPipelineLayout;
		TArray<VkDescriptorSetLayoutBinding>    mDescriptorBindings;
		TArray<VkPipelineShaderStageCreateInfo> mStages;
	};

	class ShaderFormatSpirv : public ShaderFormat
	{
	public:
		ShaderFormatSpirv(VkDevice device)
			:mDevice(device)
		{
			//bUsePreprocess = false;
		}
		virtual char const* getName() final { return "spir-v"; }
		virtual void setupShaderCompileOption(ShaderCompileOption& option) final;
		virtual void getHeadCode(std::string& inoutCode, ShaderCompileOption const& option, ShaderEntryInfo const& entry) final;

		virtual EShaderCompileResult compileCode(ShaderCompileContext const& context) final;
		virtual void precompileCode(ShaderProgramSetupData& setupData) override;
		virtual void precompileCode(ShaderSetupData& setupData) override;
		virtual ShaderParameterMap* initializeProgram(RHIShaderProgram& shaderProgram, ShaderProgramSetupData& setupData) final;
		virtual ShaderParameterMap* initializeProgram(RHIShaderProgram& shaderProgram, TArray< ShaderCompileDesc > const& descList, TArray<uint8> const& binaryCode) final;
		virtual ShaderParameterMap* initializeShader(RHIShader& shader, ShaderSetupData& setupData) override;
		virtual ShaderParameterMap* initializeShader(RHIShader& shader, ShaderCompileDesc const& desc, TArray<uint8> const& binaryCode) override;

		virtual void postShaderLoaded(RHIShaderProgram& shaderProgram) final;

		virtual bool doesSuppurtBinaryCode() const final;
		virtual bool getBinaryCode(ShaderProgramSetupData& setupData, TArray<uint8>& outBinaryCode) final;

		virtual ShaderPreprocessSettings getPreprocessSettings() override
		{
			ShaderPreprocessSettings settings;
			settings.bSupportLineFilePath = false; // shaderc / glslangValidator does not support string file paths in #line directives
			return settings;
		}



		VkDevice       mDevice;
	};

}//namespace Render


#endif // VulkanShaderCommon_H_49227E41_76E2_4638_AD24_612B97726E22
