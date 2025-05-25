#pragma once
#ifndef MeshCommon_H_500CA21B_BC86_413C_8302_BB01100EFCC9
#define MeshCommon_H_500CA21B_BC86_413C_8302_BB01100EFCC9

#include "RHI/RHIType.h"

#define MESH_USE_PACKED_CONE 0

namespace Render
{
	struct VertexElementReader
	{
		VertexElementReader() = default;
		VertexElementReader(uint8 const* pData, int32 stride)
			:pVertexData(pData),vertexDataStride(stride){}

		template< class T = Vector3 >
		T const& get(int idx) const
		{
			return *(T const*)(pVertexData + idx * vertexDataStride);
		}

		template< class T = Vector3 >
		T const* getPtr(int idx) const
		{
			return (T const*)(pVertexData + idx * vertexDataStride);
		}


		Vector3 const& operator[](int idx) const { return get(idx); }


		bool isValid() const
		{
			return vertexDataStride > 0;
		}

		uint8 const* pVertexData;
		int32  vertexDataStride;
	};

	struct VertexElementWriter
	{
		VertexElementWriter() = default;
		VertexElementWriter(uint8* pData, int32 stride)
			:pVertexData(pData), vertexDataStride(stride) {}

		template< class T = Vector3 >
		T& get(int idx) const
		{
			return *(T*)(pVertexData + idx * vertexDataStride);
		}

		template< class T = Vector3 >
		T* getPtr(int idx) const
		{
			return (T*)(pVertexData + idx * vertexDataStride);
		}

		Vector3& operator[](int idx) const { return get(idx); }

		bool isValid() const
		{
			return vertexDataStride > 0;
		}

		uint8* pVertexData;
		int32  vertexDataStride;
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
