#pragma once
#ifndef CubeBlockRender_H_40E60BB2_467B_44A1_98F1_D8076A6291FA
#define CubeBlockRender_H_40E60BB2_467B_44A1_98F1_D8076A6291FA

#include "CubeBase.h"
#include "CubeMesh.h"

#include "Core/TypeHash.h"
#include "Math/GeometryPrimitive.h"

#include "CubeWorld.h"

namespace Cube
{
	class Mesh;


	struct BlockVertex
	{
		Vec3f     pos;
		union
		{
			struct
			{
				uint32    face  : 4;
				uint32    matId : 28;
			};
			uint32 meta;
		};

		bool operator == (BlockVertex const& rhs) const
		{
			return pos == rhs.pos && meta == rhs.meta;
		}

		uint32 getTypeHash() const
		{
			uint32 result = HashValues(pos.x, pos.y, pos.z, meta);
			return result;
		}
	};

	struct PaddedBlockAccess : public IBlockAccess
	{
		BlockId blocks[ChunkSize + 2][ChunkSize + 2][Chunk::LayerSize + 2];
		Vec3i basePos;

		void fill(class NeighborChunkAccess const& chunkAccess, Chunk* center, int layerIdx);

		virtual BlockId  getBlockId(int x, int y, int z) override
		{
			int lx = x - basePos.x + 1;
			int ly = y - basePos.y + 1;
			int lz = z - basePos.z + 1;
			if (lx < 0 || lx >= (ChunkSize + 2) || ly < 0 || ly >= (ChunkSize + 2) || lz < 0 || lz >= (Chunk::LayerSize + 2))
				return BLOCK_NULL;
			return blocks[lx][ly][lz];
		}
		virtual MetaType getBlockMeta(int x, int y, int z) override { return 0; }
		virtual void  getNeighborBlockIds(Vec3i const& pos, BlockId outIds[]) override
		{
			int lx = pos.x - basePos.x + 1;
			int ly = pos.y - basePos.y + 1;
			int lz = pos.z - basePos.z + 1;
			outIds[FACE_X] = blocks[lx + 1][ly][lz];
			outIds[FACE_NX] = blocks[lx - 1][ly][lz];
			outIds[FACE_Y] = blocks[lx][ly + 1][lz];
			outIds[FACE_NY] = blocks[lx][ly - 1][lz];
			outIds[FACE_Z] = blocks[lx][ly][lz + 1];
			outIds[FACE_NZ] = blocks[lx][ly][lz - 1];
		}
	};

	class BlockRenderer
	{
	public:
		BlockRenderer()
		{
			mVertexOffset = Vec3f::Zero();
			bound.invalidate();
		}

		void     drawLayer(Chunk& chunk, int layerIdx);
		void     draw(Vec3i const& offset, BlockId id );
		void     drawSimple(unsigned faceMask, uint32 matId);
		void     drawUnknown(unsigned faceMask);
		void     setBasePos( Vec3i const& pos ){ mBasePos = pos; }
		Mesh&    getMesh(){ return *mMesh; }

		Color4ub      mDebugColor;
		Vec3i         mBasePos;
		IBlockAccess* mBlockAccess;
		Mesh*         mMesh;

		void setVertexOffset(Vec3f const& offset) { mVertexOffset = offset; }


		struct BlockSurfaceQuad 
		{
			FaceSide face;
			uint32   matId;
			Vec3f    offset;
		};
		void addBlockFace(FaceSide face, uint32 matId);


		void addMeshVertex(Vec3f const& pos);
		void addMeshVertex(float x, float y, float z);
		void addMeshTriangle(int i1, int i2, int i3);
		void addMeshQuad(int i1, int i2, int i3, int i4);


		void addBlockTriangle(int i1, int i2, int i3)
		{
			mIndices.push_back(i1);
			mIndices.push_back(i2);
			mIndices.push_back(i3);
		}

		void addBlockQuad(int i1, int i2, int i3, int i4)
		{
			addBlockTriangle(i1, i2, i3);
			addBlockTriangle(i1, i3, i4);
		}

		void setColor(uint8 r, uint8 g, uint8 b);
		void setColor(Color4ub const& color)
		{
			mCurVertex.color = color;
		}
		void setNormal(Vec3f const& noraml)
		{
			mCurVertex.normal[0] = int16(Math::RoundToInt(32767.0f * Math::Clamp(noraml.x, -1.0f, 1.0f)));
			mCurVertex.normal[1] = int16(Math::RoundToInt(32767.0f * Math::Clamp(noraml.y, -1.0f, 1.0f)));
			mCurVertex.normal[2] = int16(Math::RoundToInt(32767.0f * Math::Clamp(noraml.z, -1.0f, 1.0f)));
			mCurVertex.normal[3] = 0;
		}


		void mergeBlockPrimitives();
		void fillBlockPrimitivesToMesh();


		void finalizeMesh()
		{
			//mergeBlockPrimitives();
			fillBlockPrimitivesToMesh();
			mQuads.clear();
			mVertices.clear();
			mIndices.clear();
		}


		Vec3f   mVertexOffset;
		Mesh::Vertex mCurVertex;
		Math::TAABBox< Vec3f > bound;
		TArray< BlockSurfaceQuad > mQuads;
		TArray< BlockVertex > mVertices;
		TArray< uint32 >      mIndices;

	};

}//namespace Cube

#endif // CubeBlockRender_H_40E60BB2_467B_44A1_98F1_D8076A6291FA
