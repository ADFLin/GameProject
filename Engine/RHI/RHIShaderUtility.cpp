#include "RHIShaderUtility.h"
#include "ShaderManager.h"
#include "Math/Base.h"
#include "RHICommon.h"

namespace Render
{

	class GenerateMipsCS : public GlobalShader
	{
		DECLARE_SHADER(GenerateMipsCS, Global);
	public:
		SHADER_PERMUTATION_TYPE_BOOL(IsSRGB, "IS_SRGB");
		using PermutationDomain = TShaderPermutationDomain<IsSRGB>;
		static char const* GetShaderFileName() { return "Shader/GenerateMips"; }
	};

	IMPLEMENT_SHADER(GenerateMipsCS, EShader::Compute, SHADER_ENTRY(GenerateMipsCS));

	void RHIShaderUtility::GenerateMips(RHICommandList& commandList, RHITextureBase& texture)
	{
		auto const& desc = texture.getDesc();
		if (desc.numMipLevel <= 1)
			return;

		RHIResourceTransition(commandList, { &texture }, EResourceTransition::UAV);

		for (int i = 1; i < (int)desc.numMipLevel; ++i)
		{
			uint32 destW = Math::Max(1u, (uint32)desc.dimension.x >> i);
			uint32 destH = Math::Max(1u, (uint32)desc.dimension.y >> i);

			GenerateMipsCS::PermutationDomain domain;
			domain.set<GenerateMipsCS::IsSRGB>(ETexture::IsSRGB(desc.format));
			auto* shader = ShaderManager::Get().getGlobalShaderT<GenerateMipsCS>(domain);
			if (!shader) continue;

			RHISetComputeShader(commandList, shader->getRHI());
			shader->setTexture(commandList, SHADER_PARAM(SourceTexture), texture);
			shader->setParam(commandList, SHADER_PARAM(DestTextureSize), IntVector2((int)destW, (int)destH));
			shader->setParam(commandList, SHADER_PARAM(TexelSize), Vector2(1.0f / destW, (float)1.0f / destH));
			shader->setParam(commandList, SHADER_PARAM(SrcMipLevel), (float)(i - 1));
			shader->setRWTexture(commandList, SHADER_PARAM(DestTexture), texture, i, EAccessOp::WriteOnly);

			RHIDispatchCompute(commandList, (destW + 7) / 8, (destH + 7) / 8, 1);
			RHIResourceTransition(commandList, { &texture }, EResourceTransition::UAVBarrier);
		}

		RHIResourceTransition(commandList, { &texture }, EResourceTransition::SRV);
	}
}
