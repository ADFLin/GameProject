#include "SceneObject.h"

namespace Render
{
	void StaticMesh::render(Matrix4 const& worldTrans, RenderContext& context, Material* material)
	{
		context.setMaterial(material);
		context.setWorld(worldTrans);
		{
			//GPU_PROFILE("MeshDraw %s", name.c_str());
			drawShader(context.getCommnadList());
		}
	}

	void StaticMesh::render(Matrix4 const& worldTrans, RenderContext& context)
	{
		for( int i = 0; i < mSections.size(); ++i )
		{
			Material* material = getMaterial(i);
			context.setMaterial(material);
			context.setWorld(worldTrans);

			{
				char const* matName = material ? material->getMaster()->mName.c_str() : "DefalutMaterial";
				//GPU_PROFILE( "MeshDraw %s %s %d" , name.c_str() , matName , mSections[i].num);
				drawSection(context.getCommnadList(),i, true);
			}
		}
	}

	void SimpleMeshObject::getPrimitives(PrimitivesCollection& primitiveCollection)
	{
		if( mesh->mVertexBuffer == nullptr )
			return;

		MeshBatch meshBatch;
		meshBatch.primitiveType = mesh->mType;
		meshBatch.inputLayout = mesh->mInputLayout;
		meshBatch.vertexBuffer = mesh->mVertexBuffer;
		meshBatch.material = material.get();

		MeshBatchElement meshElement;
		meshElement.indexBuffer = mesh->mIndexBuffer;
		meshElement.idxStart = 0;
		if( mesh->mIndexBuffer )
			meshElement.numElement = mesh->mIndexBuffer->getNumElements();
		else
			meshElement.numElement = mesh->mVertexBuffer->getNumElements();
		meshElement.world = worldTransform;

		meshBatch.elements.push_back(meshElement);
		primitiveCollection.addMesh(meshBatch);
	}

	void StaticMeshObject::getPrimitives(PrimitivesCollection& primitiveCollection)
	{
		StaticMesh* mesh = meshPtr;
		if( mesh->mVertexBuffer == nullptr )
			return;

		if( !mesh->mSections.empty() )
		{

			for( int i = 0; i < mesh->mSections.size(); ++i )
			{
				MeshBatch meshBatch;
				meshBatch.primitiveType = mesh->mType;
				meshBatch.inputLayout = mesh->mInputLayout;
				meshBatch.vertexBuffer = mesh->mVertexBuffer;
				meshBatch.material = mesh->getMaterial(i);

				MeshBatchElement meshElement;
				meshElement.indexBuffer = mesh->mIndexBuffer;
				meshElement.idxStart = mesh->mSections[i].start;
				meshElement.numElement = mesh->mSections[i].num;
				meshElement.world = worldTransform;

				meshBatch.elements.push_back(meshElement);
				primitiveCollection.addMesh(meshBatch);
			}
		}
		else
		{
			MeshBatch meshBatch;
			meshBatch.primitiveType = mesh->mType;
			meshBatch.inputLayout = mesh->mInputLayout;
			meshBatch.vertexBuffer = mesh->mVertexBuffer;
			meshBatch.material = mesh->getMaterial(0);

			MeshBatchElement meshElement;
			meshElement.indexBuffer = mesh->mIndexBuffer;
			meshElement.idxStart = 0;
			if( mesh->mIndexBuffer )
				meshElement.numElement = mesh->mIndexBuffer->getNumElements();
			else
				meshElement.numElement = mesh->mVertexBuffer->getNumElements();
			meshElement.world = worldTransform;

			meshBatch.elements.push_back(meshElement);
			primitiveCollection.addMesh(meshBatch);
		}
	}

	void DebugGeometryObject::getDynamicPrimitives(PrimitivesCollection& primitiveCollection, ViewInfo& view)
	{
		for( auto& line : mLines )
		{
			primitiveCollection.addLine(line.start, line.end, line.color, line.thickness);
		}
	}

	void DebugGeometryObject::addLine(Vector3 const& posStart, Vector3 const& posEnd, Vector3 const& color, float thickness /*= 1*/, float time /*= -1*/)
	{
		LineElement line;
		line.start = posStart;
		line.end = posEnd;
		line.color = color;
		line.thickness = thickness;
		line.time = time;
		mLines.push_back(line);
	}

	void DebugGeometryObject::tick(float deltaTime)
	{
		for( int idx = 0; idx < mLines.size(); ++idx )
		{
			auto& line = mLines[idx];
			if( line.time > 0 )
			{
				line.time -= deltaTime;
				if( line.time < 0 )
				{
					if( idx != mLines.size() - 1 )
					{
						mLines[idx] = mLines.back();
					}
					mLines.pop_back();
					--idx;
				}
			}
		}


		for( int idx = 0; idx < mPoints.size(); ++idx )
		{
			auto& point = mPoints[idx];
			if( point.time > 0 )
			{
				point.time -= deltaTime;
				if( point.time < 0 )
				{
					if( idx != mPoints.size() - 1 )
					{
						mPoints[idx] = mPoints.back();
					}
					mPoints.pop_back();
					--idx;
				}
			}
		}
	}

}


