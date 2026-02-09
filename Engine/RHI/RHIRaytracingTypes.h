#pragma once
#ifndef RHIRaytracingTypes_H_C26C2808_4F9D_436E_B158_9C8A88A50A88
#define RHIRaytracingTypes_H_C26C2808_4F9D_436E_B158_9C8A88A50A88

#include "RHICommon.h"
#include "Math/Matrix4.h"

namespace Render
{
	class ShaderParameter;

	enum class EAccelerationStructureBuildFlags
	{
		None = 0,
		AllowUpdate = 1 << 0,
		AllowCompaction = 1 << 1,
		PreferFastTrace = 1 << 2,
		PreferFastBuild = 1 << 3,
		MinimizeMemory = 1 << 4,
	};
	SUPPORT_ENUM_FLAGS_OPERATION(EAccelerationStructureBuildFlags)

	enum class ERayTracingInstanceFlags : uint32
	{
		None = 0,
		TriangleCullDisable = 1 << 0,
		TriangleFrontCounterClockwise = 1 << 1,
		ForceOpaque = 1 << 2,
		ForceNonOpaque = 1 << 3,
	};
	SUPPORT_ENUM_FLAGS_OPERATION(ERayTracingInstanceFlags)

	enum class ERayTracingGeometryType
	{
		Triangles,
		Procedural,
	};

	struct RayTracingGeometryDesc
	{
		ERayTracingGeometryType type = ERayTracingGeometryType::Triangles;
		RHIBuffer* vertexBuffer = nullptr;
		RHIBuffer* indexBuffer = nullptr;
		EVertex::Format vertexFormat = EVertex::Float3;
		uint32 vertexCount = 0;
		uint32 vertexStride = 0;
		uint32 indexCount = 0;
		uint32 vertexOffset = 0;
		uint32 indexOffset = 0; // Bytes
		EIndexBufferType indexType = EIndexBufferType::U16;
		bool bOpaque = true;
	};

	class RHIAccelerationStructure : public RHIResource
	{
	public:
#if RHI_USE_RESOURCE_TRACE
		RHIAccelerationStructure(char const* type = "RHIAccelerationStructure") : RHIResource(type) {}
#else
		RHIAccelerationStructure() {}
#endif
		virtual EResourceType getResourceType() const = 0;
	};

	class RHIBottomLevelAccelerationStructure : public RHIAccelerationStructure
	{
	public:
#if RHI_USE_RESOURCE_TRACE
		RHIBottomLevelAccelerationStructure() : RHIAccelerationStructure("RHIBottomLevelAccelerationStructure") {}
#else
		RHIBottomLevelAccelerationStructure() {}
#endif
		virtual EResourceType getResourceType() const override { return EResourceType::BottomLevelAccelerationStructure; }
	};

	struct RayTracingInstanceDesc
	{
		Matrix4 transform;
		uint32 instanceID : 24;
		uint32 instanceMask : 8;
		uint32 recordIndex : 24;
		uint32 flags : 8;
		RHIAccelerationStructure* blas = nullptr;

		RayTracingInstanceDesc()
		{
			instanceID = 0;
			instanceMask = 0xFF;
			recordIndex = 0;
			flags = 0;
			transform.setIdentity();
		}
	};

	class RHITopLevelAccelerationStructure : public RHIAccelerationStructure
	{
	public:
#if RHI_USE_RESOURCE_TRACE
		RHITopLevelAccelerationStructure() : RHIAccelerationStructure("RHITopLevelAccelerationStructure") {}
#else
		RHITopLevelAccelerationStructure() {}
#endif
		virtual EResourceType getResourceType() const override { return EResourceType::TopLevelAccelerationStructure; }
	};

	struct RayTracingShaderGroup
	{
		RHIShader* generalShader = nullptr;
		RHIShader* closestHitShader = nullptr;
		RHIShader* anyHitShader = nullptr;
		RHIShader* intersectionShader = nullptr;
		uint32     localDataSize = 0;
	};

	struct RayTracingPipelineStateInitializer
	{
		RHIShader* rayGenShader = nullptr;
		RHIShader* missShader = nullptr;
		RHIShader* hitGroupShader = nullptr;

		TArray<RayTracingShaderGroup> missShaders;
		TArray<RayTracingShaderGroup> hitGroups;

		uint32 maxPayloadSize = 0;
		uint32 maxAttributeSize = 2 * sizeof(float);
		uint32 maxRecursionDepth = 1;
	};

	class RHIRayTracingPipelineState : public RHIPipelineState
	{
	public:
	};

	class RHIRayTracingShaderTable : public RHIResource
	{
	public:
		RHIRayTracingShaderTable() : RHIResource(TRACE_TYPE_NAME("RayTracingShaderTable")) 
		{ 
			mResourceType = EResourceType::RayTracingShaderTable; 
		}




		virtual void setShader(uint32 recordIndex, RHIShader* shader, int optionalGroupIndex = -1) {}
		virtual void setValue(uint32 recordIndex, EShader::Type type, ShaderParameter const& parameter, void* data) {}
		virtual void setTexture(uint32 recordIndex, EShader::Type type, ShaderParameter const& parameter, RHITextureBase* texture) {}
		virtual void setRWTexture(uint32 recordIndex, EShader::Type type, ShaderParameter const& parameter, RHITextureBase* texture, EAccessOp accessOp) {}
		virtual void setStorageBuffer(uint32 recordIndex, EShader::Type type, ShaderParameter const& parameter, RHIBuffer* buffer, EAccessOp accessOp) {}
		virtual void setUniformBuffer(uint32 recordIndex, EShader::Type type, ShaderParameter const& parameter, RHIBuffer* buffer) {}
		virtual void setSampler(uint32 recordIndex, EShader::Type type, ShaderParameter const& parameter, RHISamplerState* sampler) {}
	};

	using RHIAccelerationStructureRef = TRefCountPtr<RHIAccelerationStructure>;
	using RHIBottomLevelAccelerationStructureRef = TRefCountPtr<RHIBottomLevelAccelerationStructure>;
	using RHITopLevelAccelerationStructureRef = TRefCountPtr<RHITopLevelAccelerationStructure>;
	using RHIRayTracingPipelineStateRef = TRefCountPtr<RHIRayTracingPipelineState>;
	using RHIRayTracingShaderTableRef = TRefCountPtr<RHIRayTracingShaderTable>;

} // namespace Render

#endif // RHIRaytracingTypes_H_C26C2808_4F9D_436E_B158_9C8A88A50A88
