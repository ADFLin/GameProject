#pragma once
#ifndef MeshCommon_H_500CA21B_BC86_413C_8302_BB01100EFCC9
#define MeshCommon_H_500CA21B_BC86_413C_8302_BB01100EFCC9

#include "RHI/RHIType.h"

#define MESH_USE_PACKED_CONE 0

namespace Render
{
	struct VertexElementReader
	{
		template< class T = Vector3 >
		T const& get(int idx) const
		{
			return *(T const*)(pVertexData + idx * vertexDataStride);
		}

		Vector3 const& operator[](int idx) const { return get(idx); }

		int32  getNum() const { return numVertex; }
		uint8 const* pVertexData;
		uint32 vertexDataStride;
		int32  numVertex;
	};


	enum class EAdjacencyType
	{
		Vertex,
		Tessellation,
	};

	struct MeshSection
	{
		int indexStart;
		int count;
	};


	struct MeshletData
	{
		uint32 vertexCount;
		uint32 vertexOffset;
		uint32 primitiveCount;
		uint32 primitveOffset;
	};

	union PackagedTriangleIndices
	{
		struct
		{
			uint32 i0 : 10;
			uint32 i1 : 10;
			uint32 i2 : 10;
			uint32 unused : 2;
		};
		uint32 value;
	};


	struct GPU_ALIGN MeshletCullData
	{
		Vector4  boundingSphere; // xyz = center, w = radius
#if MESH_USE_PACKED_CONE
		uint8    normalCone[4];  // xyz = axis, w = sin(a + 90)
#else
		Vector4  normalCone;
#endif
		float    ApexOffset;     // apex = center - axis * offset
	};


}//namespace Render

#endif // MeshCommon_H_500CA21B_BC86_413C_8302_BB01100EFCC9
