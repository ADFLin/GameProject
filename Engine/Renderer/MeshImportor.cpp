#include "MeshImportor.h"

namespace Render
{

#if CORE_SHARE_CODE
	MeshImporterRegistry& MeshImporterRegistry::Get()
	{
		static MeshImporterRegistry Instance;
		return Instance;
	}

	void MeshImporterRegistry::registerMeshImporter(HashString name, IMeshImporterFactoryPtr const& importor)
	{
		mFactoryMap.insert_or_assign(name, importor);
	}

	void MeshImporterRegistry::unregisterMeshImporter(HashString name)
	{
		mFactoryMap.erase(name);
	}

	IMeshImporterPtr MeshImporterRegistry::getMeshImproter(HashString name)
	{
		auto iter = mFactoryMap.find(name);
		if (iter != mFactoryMap.end())
		{
			if (iter->second != nullptr)
			{
				return iter->second->create(name);
			}
		}
		return IMeshImporterPtr();
	}
#endif

	void MeshImportData::initSections()
	{
		MeshSection section;
		section.indexStart = 0;
		section.count = indices.size();
		sections.push_back(section);
	}

	void MeshImportData::append(MeshImportData const& other)
	{
		if (sections.empty())
		{
			initSections();
		}

		uint32 baseIndex = numVertices;
		vertices.append(other.vertices);

		int start = (int)indices.size();
		indices.append(other.indices);
		for (int i = start; i < indices.size(); ++i)
		{
			indices[i] += baseIndex;
		}
		numVertices += other.numVertices;

		MeshSection section;
		section.indexStart = start;
		section.count = other.indices.size();
		sections.push_back(section);
	}

	VertexElementReader MeshImportData::makeAttributeReader(EVertex::Attribute attribute)
	{
		VertexElementReader result;
		result.vertexDataStride = desc.getVertexSize();
		result.pVertexData = vertices.data() + desc.getAttributeOffset(attribute);
		return result;
	}

}//namespace Render


