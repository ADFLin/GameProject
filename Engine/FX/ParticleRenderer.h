#pragma once

#include "Renderer/Mesh.h"
#include "Renderer/SceneView.h"

#include "RHI/RHICommand.h"
#include "RHI/ShaderManager.h"

namespace Render
{
	class ParticleSpriteProgram : public GlobalShaderProgram
	{
		DECLARE_SHADER_PROGRAM(ParticleSpriteProgram, Global);

	public:
		static void SetupShaderCompileOption(ShaderCompileOption& option) {}
		static char const* GetShaderFileName() { return "Shader/ParticleSprite"; }

		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] =
			{
				{ EShader::Vertex , SHADER_ENTRY(MainVS) },
				{ EShader::Pixel  , SHADER_ENTRY(MainPS) },
			};
			return entries;
		}

		void bindParameters(ShaderParameterMap const& parameterMap)
		{
			BIND_SHADER_PARAM(parameterMap, ParticleOffset);
			BIND_SHADER_PARAM(parameterMap, ParticleRadius);
			BIND_SHADER_PARAM(parameterMap, ParticleColor);
		}

		void setParameters(RHICommandList& commandList, Vector3 const& pos, float radius, Vector3 const& color)
		{
			SET_SHADER_PARAM(commandList, *this, ParticleOffset, pos);
			SET_SHADER_PARAM(commandList, *this, ParticleRadius, radius);
			SET_SHADER_PARAM(commandList, *this, ParticleColor, color);
		}

		DEFINE_SHADER_PARAM(ParticleOffset);
		DEFINE_SHADER_PARAM(ParticleRadius);
		DEFINE_SHADER_PARAM(ParticleColor);
	};

	class ParticleSpriteRenderer
	{
	public:
		struct InstanceData
		{
			Vector3 pos;
			float   radius;
			Color4f color;
		};

		bool init();
		void release();
		void render(RHICommandList& commandList, class ViewInfo& view, const InstanceData* instances, int numInstances);
		InstanceData* lockInstanceBuffer(int totalParticles);
		void renderWithBuffer(RHICommandList& commandList, class ViewInfo& view, int numInstances);

		class ParticleSpriteProgram* mProgParticleSprite = nullptr;
		RHIBufferRef mInstanceBuffer;
		RHIInputLayoutRef mInstancedInputLayout;
		Mesh mMesh;
	};
}
