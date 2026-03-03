#include "FX/ParticleRenderer.h"
#include "Renderer/MeshBuild.h"
#include "RHI/DrawUtility.h"

namespace Render
{
	IMPLEMENT_SHADER_PROGRAM(ParticleSpriteProgram);

	bool ParticleSpriteRenderer::init()
	{
		if (!mInstancedInputLayout.isValid())
		{
			InputLayoutDesc desc;
			desc.addElement(0, EVertex::ATTRIBUTE_POSITION, EVertex::Float3);
			desc.addElement(1, EVertex::ATTRIBUTE1, EVertex::Float3, false, true, 1); // pos (offset 0)
			desc.addElement(1, EVertex::ATTRIBUTE2, EVertex::Float1, false, true, 1); // radius (offset 12)
			desc.addElement(1, EVertex::ATTRIBUTE3, EVertex::Float4, false, true, 1); // color (offset 16)
			mInstancedInputLayout = RHICreateInputLayout(desc);
		}

		VERIFY_RETURN_FALSE(mProgParticleSprite = ShaderManager::Get().getGlobalShaderT<ParticleSpriteProgram>(true));
		VERIFY_RETURN_FALSE(FMeshBuild::SpritePlane(mMesh));

		return true;
	}

	void ParticleSpriteRenderer::release()
	{
		mInstanceBuffer.release();
		mInstancedInputLayout.release();
		mMesh.releaseRHIResource();
		mProgParticleSprite = nullptr;
	}

	ParticleSpriteRenderer::InstanceData* ParticleSpriteRenderer::lockInstanceBuffer(int totalParticles)
	{
		if (totalParticles <= 0) return nullptr;

		if (!mInstanceBuffer.isValid() || mInstanceBuffer->getSize() < totalParticles * sizeof(InstanceData))
		{
			mInstanceBuffer = RHICreateVertexBuffer(sizeof(InstanceData), Math::Max((size_t)totalParticles + 4096, size_t(2048)), BCF_CpuAccessWrite);
		}

		return (InstanceData*)RHILockBuffer(mInstanceBuffer, ELockAccess::WriteDiscard);
	}

	void ParticleSpriteRenderer::renderWithBuffer(RHICommandList& commandList, class ViewInfo& view, int numInstances)
	{
		RHIUnlockBuffer(mInstanceBuffer);

		RHISetBlendState(commandList, TStaticBlendState<Render::CWM_RGBA, Render::EBlend::SrcAlpha, Render::EBlend::One>::GetRHI());
		RHISetDepthStencilState(commandList, TStaticDepthStencilState<false>::GetRHI());
		RHISetRasterizerState(commandList, TStaticRasterizerState<Render::ECullMode::None>::GetRHI());

		RHISetShaderProgram(commandList, mProgParticleSprite->getRHI());
		view.setupShader(commandList, *mProgParticleSprite);

		// Draw Setup
		InputStreamInfo inputStreams[2];
		inputStreams[0].buffer = mMesh.mVertexBuffer;
		inputStreams[0].stride = mMesh.mInputLayoutDesc.getVertexSize(0);
		inputStreams[0].offset = 0;
		inputStreams[1].buffer = mInstanceBuffer;
		inputStreams[1].stride = sizeof(InstanceData);
		inputStreams[1].offset = 0;

		RHISetInputStream(commandList, mInstancedInputLayout, inputStreams, 2);
		RHISetIndexBuffer(commandList, mMesh.mIndexBuffer);
		RHIDrawIndexedPrimitiveInstanced(commandList, mMesh.mType, 0, mMesh.mIndexBuffer->getNumElements(), numInstances);
	}

	void ParticleSpriteRenderer::render(RHICommandList& commandList, ViewInfo& view, const InstanceData* instances, int numInstances)
	{
		InstanceData* pData = lockInstanceBuffer(numInstances);
		if (pData)
		{
			std::memcpy(pData, instances, sizeof(InstanceData) * numInstances);
			renderWithBuffer(commandList, view, numInstances);
		}
	}
}
