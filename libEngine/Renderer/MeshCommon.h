#pragma once
#ifndef MeshCommon_H_500CA21B_BC86_413C_8302_BB01100EFCC9
#define MeshCommon_H_500CA21B_BC86_413C_8302_BB01100EFCC9

#include "RHI/RHIType.h"

namespace Render
{
	struct VertexElementReader
	{
		template< class T = Vector3 >
		T const& get(int idx) const
		{
			return *(T const*)(pVertexData + idx * vertexDataStride);
		}
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
		int start;
		int num;
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



}//namespace Render

#endif // MeshCommon_H_500CA21B_BC86_413C_8302_BB01100EFCC9
