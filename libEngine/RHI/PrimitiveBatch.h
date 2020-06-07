#pragma once
#ifndef PrimitiveBatch_H_F2A726E4_2D4A_4BC8_B509_6B46823689A1
#define PrimitiveBatch_H_F2A726E4_2D4A_4BC8_B509_6B46823689A1

#include "OpenGLCommon.h"
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
		RHIIndexBuffer*  indexBuffer;

		int              idxStart;
		int              numElement;

		//TODO: use uniform buffer
		Matrix4  world;
		RHIVertexBuffer* primitiveBuffer;

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
		RHIVertexBuffer* vertexBuffer;
		std::vector< MeshBatchElement > elements;
		//#TODO
		GLuint VAOHandle;

		void draw( RenderContext& context);

		MeshBatch()
		{
			material = nullptr;
			vertexBuffer = nullptr;
			inputLayout = nullptr;
			VAOHandle;
		}
	};

	struct LineBatch
	{
		Vector3 start;
		Vector3 end;
		Vector3 color;
		float   thickness;
	};

	struct SimpleVertex
	{
		Vector4 pos;
		Vector4 color;

		SimpleVertex(Vector3 const& inPos, Vector3 const& inColor)
			:pos(inPos,1), color(inColor){}
	};

	class SimpleElementRenderer
	{
	public:

		static int const MaxVertexSize = 1024;
		bool init();
		void draw(RenderContext& context, SimpleVertex* vertices, int numVertices);

		ShaderProgram mShader;
		RHIInputLayoutRef  mInputLayout;

		RHIVertexBufferRef mVertexBuffer;
		GLuint        mVAO;
	};

	class PrimitivesCollection
	{
	public:
		void addMesh(MeshBatch& meshBatch)
		{
			mMeshBatchs.push_back(meshBatch);
		}
		void addLine(Vector3 const& posStart, Vector3 const& posEnd , Vector3 const& color , float thickness = 1)
		{
			LineBatch line;
			line.start = posStart;
			line.end = posEnd;
			line.color = color;
			line.thickness = thickness;
			mLineBatchs.push_back(line);
		}
		void clear()
		{
			mMeshBatchs.clear();
			mLineBatchs.clear();
		}

		void drawDynamic(RenderContext& context);
		std::vector< MeshBatch > mMeshBatchs;
		std::vector< LineBatch > mLineBatchs;

		std::vector< SimpleVertex > mCacheBuffer;
	};



}//namespace Render

#endif // PrimitiveBatch_H_F2A726E4_2D4A_4BC8_B509_6B46823689A1