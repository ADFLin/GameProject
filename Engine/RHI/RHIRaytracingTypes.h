#pragma once

#ifndef RHIRaytracingTypes_H_C26C2808_4F9D_436E_B158_9C8A88A50A88
#define RHIRaytracingTypes_H_C26C2808_4F9D_436E_B158_9C8A88A50A88

#include "RHICommon.h"

namespace Render
{

	namespace ERaytracingShader
	{
		enum Type : uint8
		{
			RayGen,
			AnyHit,
			ClosestHit,
			Intersection,
			Miss,
		};
	};

	class RHIRaytracingShader : public RHIResource
	{
	public:

	};

	class RHIAccelerationStructureBuffer : public RHIResource
	{
	public:

	};

}
#endif // RHIRaytracingTypes_H_C26C2808_4F9D_436E_B158_9C8A88A50A88
