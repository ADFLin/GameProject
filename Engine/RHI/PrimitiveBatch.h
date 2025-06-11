#pragma once
#ifndef PrimitiveBatch_H_F2A726E4_2D4A_4BC8_B509_6B46823689A1
#define PrimitiveBatch_H_F2A726E4_2D4A_4BC8_B509_6B46823689A1

#include "RHICommon.h"
#include "ShaderProgram.h"
#include <vector>

namespace Render
{
	class RHICommandList;

	class  Material;
	struct ViewInfo;
	class RenderContext;
	class VertexFactory;

	struct MeshBatchElement
	{
		RHIBuffer*  indexBuffer;

		int              idxStart;
		int              numElement;

		//#TODO: use uniform buffer
		Matrix4    world;
		RHIBuffer* primitiveBuffer;

		MeshBatchElement()
		{
			indexBuffer = nullptr;
			primitiveBuffer = nullptr;
		}
	};

	struct MeshBatch
	{
		EPrimitive       primitiveType;
		RHIInputLayout*  inputLayout;
		Material*        material;
		RHIBuffer*       vertexBuffer;
		TArray< MeshBatchElement > elements;

		void draw( RenderContext& context);

		MeshBatch()
		{
			material = nullptr;
			vertexBuffer = nullptr;
			inputLayout = nullptr;
		}
	};

	struct LineBatch
	{
		Vector3 start;
		Vector3 end;
		LinearColor color;
		float   thickness;
	};

	struct SimpleVertex
	{
		Vector4 pos;
		LinearColor color;

		SimpleVertex(Vector3 const& inPos, LinearColor const& inColor)
			:pos(inPos,1), color(inColor){}
	};

	class SimpleElementRenderer
	{
	public:

		static int const MaxVertexSize = 1024;

		bool initializeRHI();
		void releaseRHI();
		void draw(RHICommandList& commandList, Matrix4 const& worldToClip, SimpleVertex* vertices, int numVertices);

		class SimpleElementShaderProgram* mProgram = nullptr;
		RHIInputLayoutRef  mInputLayout;
		RHIBufferRef       mVertexBuffer;
	};

	class PrimitivesCollection
	{
	public:
		void addMesh(MeshBatch& meshBatch)
		{
			mMeshBatchs.push_back(meshBatch);
		}
		void addLine(Vector3 const& posStart, Vector3 const& posEnd , LinearColor const& color , float thickness = 1)
		{
			LineBatch line;
			line.start = posStart;
			line.end = posEnd;
			line.color = color;
			line.thickness = thickness;
			mLineBatchs.push_back(line);
		}

		void addCubeLine(Vector3 const& pos, Quaternion const& rotation, Vector3 const& extents, LinearColor const& color, float thickness = 1);

		void clear()
		{
			mMeshBatchs.clear();
			mLineBatchs.clear();
		}

		void drawDynamic(RenderContext& context);
		void drawDynamic(RHICommandList& commandList, ViewInfo& view);
		void drawDynamic(RHICommandList& commandList, IntVector2 const& screenSize, Matrix4 const& worldToClip, Vector3 const& cameraX, Vector3 const& cameraY);

		bool initializeRHI();
		void releaseRHI();

		TArray< MeshBatch > mMeshBatchs;
		TArray< LineBatch > mLineBatchs;


	};



}//namespace Render

#endif // PrimitiveBatch_H_F2A726E4_2D4A_4BC8_B509_6B46823689A1