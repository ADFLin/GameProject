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
	struct TVulkanResourceTraits<EVulkanResourceType::eVkDescriptorSetLayout>
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
	struct TVulkanResourceTraits<EVulkanResourceType::eVkPipelineLayout>
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

		bool initialize(VkDevice device, EShader::Type type, SpirvShaderCode const& code);
		void releaseResource()
		{
			mModule.destroy(mDevice);
			mDescriptorSetLayout.destroy(mDevice);
		}

		virtual bool getParameter(char const* name, ShaderParameter& outParam) { return false; }
		virtual bool getResourceParameter(EShaderResourceType resourceType, char const* name, ShaderParameter& outParam){ return false; }
		
		ShaderParameterMap mParameterMap;
		VK_RESOURCE_TYPE(VkShaderModule)        mModule;
		VK_RESOURCE_TYPE(VkDescriptorSetLayout) mDescriptorSetLayout;
		VkDevice       mDevice;
	};

	class VulkanShaderProgram : public TRefcountResource< RHIShaderProgram >
	{
	public:

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

		VK_RESOURCE_TYPE(VkShaderModule) mShaderModules[EShader::MaxStorageSize];
		int mNumShaders = 0;
		VK_RESOURCE_TYPE(VkDescriptorSetLayout) mDescriptorSetLayout;
		VK_RESOURCE_TYPE(VkPipelineLayout)      mPipelineLayout;

		std::vector< VkPipelineShaderStageCreateInfo > mStages;
		VkDevice mDevice;
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

		virtual bool compileCode(ShaderCompileInput const& input, ShaderCompileOutput& output) final;
		virtual void precompileCode(ShaderProgramSetupData& setupData);
		virtual bool initializeProgram(ShaderProgram& shaderProgram, ShaderProgramSetupData& setupData);
		virtual bool initializeProgram(ShaderProgram& shaderProgram, std::vector< ShaderCompileInfo > const& shaderCompiles, std::vector<uint8> const& binaryCode) final;

		virtual void postShaderLoaded(ShaderProgram& shaderProgram) final;

		virtual bool isBinaryCodeSupported() const final;
		virtual bool getBinaryCode(ShaderProgram& shaderProgram, ShaderProgramSetupData& setupData, std::vector<uint8>& outBinaryCode) final;



		VkDevice       mDevice;
	};

}//namespace Render


#endif // VulkanShaderCommon_H_49227E41_76E2_4638_AD24_612B97726E22
