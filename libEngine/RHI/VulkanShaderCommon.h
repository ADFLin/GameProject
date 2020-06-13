#pragma once
#ifndef VulkanShaderCommon_H_49227E41_76E2_4638_AD24_612B97726E22
#define VulkanShaderCommon_H_49227E41_76E2_4638_AD24_612B97726E22

#include "ShaderCore.h"
#include "ShaderFormat.h"

#include "VulkanCommon.h"
#include "Template/ArrayView.h"

namespace Render
{
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

		bool initialize(VkDevice device, Shader::Type type , TArrayView< char const > code)
		{
			mDevice = device;

			VkShaderModuleCreateInfo moduleInfo = {};
			moduleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			moduleInfo.pCode = (uint32 const*)code.data();
			moduleInfo.codeSize = code.size();
			VERIFY_RETURN_FALSE(mModule.initialize(device, moduleInfo));

			return true;
		}

		void releaseResource()
		{
			mModule.destroy(mDevice);
		}

		ShaderParameterMap mParameterMap;
		VK_RESOURCE_TYPE(VkShaderModule) mModule;
		char const*    mEntryName;
		VkDevice       mDevice;
	};

	class VulkanShaderProgram : public TRefcountResource< RHIShaderProgram >
	{
	public:

		TRefCountPtr< VulkanShader > mShaders[Shader::Count - 1];
		std::vector< VkPipelineShaderStageCreateInfo > mStages;
		int mNumShaders;
	};

	class ShaderFormatSpirv : public ShaderFormat
	{
	public:

		virtual char const* getName() final { return "spir-v"; }


		virtual void setupShaderCompileOption(ShaderCompileOption& option)
		{


		}
		virtual void getHeadCode(std::string& inoutCode, ShaderCompileOption const& option, ShaderEntryInfo const& entry)
		{

		}

		virtual bool isSupportBinaryCode() const 
		{ 
			return true; 
		}
		virtual bool getBinaryCode(RHIShaderProgram& shaderProgram, std::vector<uint8>& outBinaryCode)
		{

		}
		virtual bool setupProgram(RHIShaderProgram& shaderProgram, std::vector<uint8> const& binaryCode)
		{

		}

		virtual void setupParameters(ShaderProgram& shaderProgram)
		{


		}
		virtual bool compileCode(Shader::Type type, RHIShader& shader, char const* path, ShaderCompileInfo* compileInfo, char const* def);
		virtual void postShaderLoaded(RHIShaderProgram& shaderProgram)
		{

		}


		VkDevice       mDevice;
	};

}//namespace Render


#endif // VulkanShaderCommon_H_49227E41_76E2_4638_AD24_612B97726E22
