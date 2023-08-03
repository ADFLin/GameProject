#pragma once
#ifndef MeshImportor_H_6755FFD3_B5D1_4293_9F6A_53348D1F4211
#define MeshImportor_H_6755FFD3_B5D1_4293_9F6A_53348D1F4211

#include "Renderer/Mesh.h"

#include "HashString.h"
#include "CoreShare.h"
#include "Meta/IsBaseOf.h"
#include "MarcoCommon.h"
#include "DataStructure/Array.h"

#include <memory>
#include <unordered_map>


namespace Render
{
	class Mesh;

	struct MeshImportData
	{
		TArray<uint8>  vertices;
		TArray<uint32> indices;
		int numVertices = 0;
		InputLayoutDesc desc;
		TArray<MeshSection> sections;

		void initSections()
		{
			MeshSection section;
			section.indexStart = 0;
			section.count = indices.size();
			sections.push_back(section);
		}

		void append(MeshImportData const& other)
		{
			if (sections.empty())
			{
				initSections();
			}

			uint32 baseIndex = numVertices;
			vertices.append(other.vertices);

			int start = indices.size();
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

		VertexElementReader makeAttributeReader( EVertex::Attribute attribute)
		{
			VertexElementReader result;
			result.vertexDataStride = desc.getVertexSize();
			result.pVertexData = vertices.data() + desc.getAttributeOffset(attribute);
			return result;
		}

	};

	class MeshImportSettings
	{
	public:
		virtual void setupMaterial(int indexSection, void* pMaterialInfo) = 0;
	};

	class IMeshImporter
	{
	public:
		virtual ~IMeshImporter() {}
		virtual bool importFromFile(char const* filePath, Mesh& outMesh, MeshImportSettings* settings) = 0; 
		virtual bool importFromFile(char const* filePath, MeshImportData& outMeshData) = 0;
		virtual bool importMultiFromFile(char const* filePath, TArray< Mesh >& outMeshes, MeshImportSettings* settings) = 0;
	};
	using IMeshImporterPtr = std::shared_ptr< IMeshImporter >;
	class IMeshImporterFactory
	{
	public:
		virtual IMeshImporterPtr create(HashString name) = 0;
	};

	using IMeshImporterFactoryPtr = std::shared_ptr< IMeshImporterFactory >;


	class CORE_API MeshImporterRegistry
	{
	public:

		static MeshImporterRegistry& Get();

		void registerMeshImporter(HashString name, IMeshImporterFactoryPtr const& importor);
		void unregisterMeshImporter(HashString name);

		template< class T >
		class TMeshImporterFactory : public IMeshImporterFactory
		{
		public:
			virtual IMeshImporterPtr create(HashString name)
			{
				return std::make_shared<T>();
			}
		};

		template< class T , TEnableIf_Type< TIsBaseOf< T, IMeshImporter >::Value  , bool > = true >
		void registerMeshImporterT(HashString name)
		{
			registerMeshImporter( name , std::make_shared< TMeshImporterFactory<T> >() );
		}

		void cleanup()
		{
			mFactoryMap.clear();
		}
		IMeshImporterPtr getMeshImproter(HashString name);

		std::unordered_map< HashString, IMeshImporterFactoryPtr > mFactoryMap;
	};

	template< class T >
	class TMeshImporterAutoRegistry
	{
	public:
		TMeshImporterAutoRegistry(char const* name)
		{
			MeshImporterRegistry::Get().registerMeshImporterT<T>(name);
		}
	};

#define REGISTER_MESH_IMPORTER( CLASS , NAME )\
	Render::TMeshImporterAutoRegistry< CLASS > ANONYMOUS_VARIABLE(GMeshImporter)( NAME );

}//namespace Render

#endif // MeshImportor_H_6755FFD3_B5D1_4293_9F6A_53348D1F4211
